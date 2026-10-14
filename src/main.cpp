/*
 * ESP32-C3 + OLED SSD1306: animasi boot, menu Game / Navigasi / Animasi / Setting,
 * pop-up notifikasi WhatsApp via aplikasi Chronos (BLE), dan OTA firmware/animasi via WiFi.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <esp_ota_ops.h>
#include <esp_sleep.h>

#include "frames_store.h"
#include "buttons.h"
#include "settings.h"
#include "ble_link.h"
#include "clockstore.h"
#include "menu.h"
#include "games.h"
#include "power.h"
#include "anim.h"
#include "ota.h"
#include "phone.h"
#include "home.h"
#include "screen.h"
#include "version.h"

// Penanda versi di dalam file .bin; halaman OTA mencari "FWVER:" untuk menampilkan versi file.
// Dicetak saat boot supaya linker tidak membuangnya.
const char FW_TAG[] __attribute__((used)) = "FWVER:" FW_VERSION ";";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define I2C_SDA 4
#define I2C_SCL 5

#define POPUP_MS 5000               // lama tampil minimum pop-up WA
#define POPUP_MAX_MS 20000
#define POPUP_SCROLL_DELAY_MS 2000  // jeda sebelum isi panjang mulai bergulir
#define POPUP_LINE_MS 1500          // gulir 1 baris tiap 1,5 dtk

// Clock I2C OLED. Satu frame penuh (1 KB) ±23 ms di 400 kHz, ±12 ms di 800 kHz.
// Jika OLED tidak menjawab di 800 kHz (pull-up lemah, kabel panjang), turun otomatis ke 400 kHz.
#define OLED_I2C_HZ 800000
#define OLED_I2C_FALLBACK_HZ 400000

#define HEALTH_LOG 1             // log heap & stack ke Serial tiap HEALTH_LOG_MS
#define HEALTH_LOG_MS 10000

// 80 MHz = minimum untuk BLE/WiFi; cukup karena yang lambat adalah I2C OLED, bukan CPU.
// Diatur saat runtime (aman via OTA), bukan lewat bootloader/flash.
#define CPU_MHZ 80

// Jeda di akhir loop() agar CPU bisa istirahat (idle task) alih-alih berputar 100%.
#define LOOP_IDLE_ACTIVE_MS 1    // game/animasi/boot/OTA/pop-up: tetap responsif
#define LOOP_IDLE_NORMAL_MS 5    // menu & layar statis
#define LOOP_IDLE_SCREEN_OFF_MS 60   // tombol & notif tetap seketika: loop dibangunkan ISR / BLE
#define LOOP_IDLE_LIGHT_SLEEP_MS 1000  // light sleep: bangun lebih jarang (ISR tombol / BLE tetap seketika; TWDT 8 dtk)
#define LOOP_SLOW_WARN_MS 25     // putaran lebih lama dari ini saat game = peringatan di [HEALTH]

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


static AppState state = ST_BOOT;
static bool redraw = true;

static int bootFrame = 0;
static uint32_t bootNextAt = 0;

static bool popupActive = false;
static uint32_t popupStart = 0;
static char popupSender[32];
static char popupMsg[NOTIF_MSG_LEN];
static int popupTop = 0, popupLines = 0;
static uint32_t popupDuration = POPUP_MS;
static uint32_t popupLastScroll = 0;
static bool popupManual = false;  // UP/DOWN dipakai: gulir otomatis berhenti

// Rollback aktif di core: firmware hasil OTA baru dianggap sah setelah loop pertama
// berjalan. Jika crash/WDT sebelum itu, bootloader kembali ke firmware sebelumnya.
extern "C" bool verifyRollbackLater() { return true; }

static void markAppValid() {
  static bool done = false;
  if (done) return;
  done = true;
  esp_ota_img_states_t st;
  if (esp_ota_get_state_partition(esp_ota_get_running_partition(), &st) == ESP_OK &&
      st == ESP_OTA_IMG_PENDING_VERIFY) {
    esp_ota_mark_app_valid_cancel_rollback();
    Serial.println("Firmware OTA ditandai valid");
  }
}

static void enterState(AppState next) {
  if (next == state) return;
  // Semua layar mulai landscape; game & animasi potret memutar layar sendiri
  screen_landscape();
  ble_setNavScreen(next == ST_NAV);
  if (next == ST_GAME) games_enter();
  if (next == ST_ANIM) anim_enter();
  if (next == ST_OTA) ota_enter();
  if (next == ST_PHONE) phone_enter();
  state = next;
  redraw = true;
  marquee_reset();
  buttons_clear();
}

static AppState bootUpdate() {
  if (btn_any_pressed()) return ST_HOME;
  uint32_t now = millis();
  if ((int32_t)(now - bootNextAt) < 0) return ST_BOOT;
  // Jadwal tetap (telat kecil tidak menumpuk); tertinggal > 2 interval -> lompati frame
  uint32_t interval = 1000 / frames_fps();
  uint32_t late = now - bootNextAt;
  if (late > 2 * interval) {
    bootFrame += late / interval;
    if (bootFrame >= frames_count()) return ST_HOME;
  }
  bootNextAt += interval * (1 + (late > 2 * interval ? late / interval : 0));

  frames_drawFrame(bootFrame);  // ikut memutar layar bila animasi potret
  oled_flush();
  if (++bootFrame >= frames_count()) return ST_HOME;
  return ST_BOOT;
}

// Kembali ke layar di bawah pop-up/panggilan: lanjutkan game/animasi & gambar ulang
static void resumeAfterOverlay() {
  buttons_clear();
  games_resume();
  anim_resume();
  bootNextAt = millis();
  redraw = true;
  if (popupActive) popup_draw(popupSender, popupMsg, popupTop);  // pop-up WA yang tertutup panggilan
}

// Panggilan masuk: prioritas di atas pop-up WA. true selama layar panggilan tampil.
static bool callShown = false, callDismissed = false;
static char callName[32];
static uint32_t callLastDraw = 0;  // 0 = gambar ulang segera

static bool handleCall() {
  uint32_t &lastDraw = callLastDraw;
  bool ringing = settings_notifMode() != NOTIF_OFF && ble_ringing(callName, sizeof(callName));
  if (!ringing) {
    callDismissed = false;
    if (callShown) { callShown = false; resumeAfterOverlay(); }
    return false;
  }
  if (callDismissed) return false;  // BACK: sembunyikan sampai panggilan berikutnya
  if (!callShown) { callShown = true; lastDraw = 0; marquee_reset(); buttons_clear(); }
  power_activity(PR_CALL);  // layar tetap nyala selama berdering
  // iPhone (ANCS): OK = angkat, BACK = tolak. Android (Chronos): BACK = sembunyikan saja.
  bool iphone = ble_isIphone();
  bool answer = iphone && btn_pressed(BTN_OK);
  if (answer || btn_pressed(BTN_BACK)) {
    if (iphone) ble_callAnswer(answer);
    callDismissed = true;
    callShown = false;
    resumeAfterOverlay();
    return false;
  }
  buttons_clear();
  if (lastDraw == 0 || millis() - lastDraw >= 500 || marquee_frameDue()) {  // kedip ikon / nama berjalan
    lastDraw = millis();
    call_draw(callName, (millis() / 500) % 2 == 0, iphone ? "OK angkat BACK tolak" : "BACK: sembunyikan");
  }
  return true;
}

// Pop-up baru: gulir dari atas, lama tampil = maks(5 dtk, baris x 1,5 dtk + 2 dtk), maks 20 dtk
static void startPopup() {
  popupStart = popupLastScroll = millis();
  popupTop = 0;
  popupManual = false;
  power_activity(PR_POPUP);
  marquee_reset();
  popupLines = popup_draw(popupSender, popupMsg, popupTop);
  popupDuration = constrain((uint32_t)popupLines * POPUP_LINE_MS + POPUP_SCROLL_DELAY_MS, (uint32_t)POPUP_MS,
                            (uint32_t)POPUP_MAX_MS);
}

// true selama pop-up tampil (state di bawahnya dijeda)
static bool handlePopup() {
  if (!popupActive) {
    if (!ble_takeWaNotif(popupSender, sizeof(popupSender), popupMsg, sizeof(popupMsg))) return false;
    // Sedang di menu Game: jangan ganggu. Notifikasi sudah masuk riwayat (HP > Notifikasi) dan
    // penghitung belum dibaca di Beranda; pop-up dibuang, tidak ditunda.
    if (state == ST_GAME) {
      Serial.println("[NOTIF] pop-up ditahan (game)");
      return false;
    }
    popupActive = true;
    buttons_clear();
    startPopup();
    return true;
  }
  if (ble_takeWaNotif(popupSender, sizeof(popupSender), popupMsg, sizeof(popupMsg))) {
    startPopup();  // pesan baru menggantikan yang lama
    return true;
  }
  if (btn_pressed(BTN_OK) || millis() - popupStart >= popupDuration) {
    popupActive = false;
    resumeAfterOverlay();
    return false;
  }
  uint32_t now = millis();
  int maxTop = max(0, popupLines - POPUP_LINES);
  int top = popupTop;
  // UP/DOWN: gulir manual; waktu tampil dihitung ulang dari sekarang agar sempat dibaca
  bool up = btn_pressed(BTN_UP), down = btn_pressed(BTN_DOWN);
  if (up || down) {
    top = constrain(top + (down ? 1 : -1), 0, maxTop);
    popupManual = true;
    popupStart = now;
  } else if (!popupManual && top < maxTop && now - popupStart >= POPUP_SCROLL_DELAY_MS &&
             now - popupLastScroll >= POPUP_LINE_MS) {
    top++;
    popupLastScroll = now;
  }
  if (top != popupTop || marquee_frameDue()) {  // gulir baru / nama pengirim panjang berjalan
    popupTop = top;
    popup_draw(popupSender, popupMsg, popupTop);
  }
  buttons_clear();  // tombol lain diabaikan selama pop-up
  return true;
}

void setup() {
  setCpuFrequencyMhz(CPU_MHZ);
  Serial.begin(115200);
  Serial.printf("CPU %u MHz, APB %u MHz\n", getCpuFrequencyMhz(), getApbFrequency() / 1000000);
  power_begin();
  buttons_begin();
  settings_begin();
  Serial.printf("[POWER] mode tidur=%s\n", settings_sleepName());

  Wire.begin(I2C_SDA, I2C_SCL);
  // Clock dari Setting "OLED I2C" (800/400/200 kHz); tidak menjawab -> turun otomatis (screen.cpp)
  oled_begin(SCREEN_ADDRESS);
  uint32_t oledHz = oled_applyClock(settings_i2cHz());
  bool oledOk = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  if (!oledOk && oledHz != OLED_I2C_FALLBACK_HZ) {
    oledHz = oled_applyClock(OLED_I2C_FALLBACK_HZ);
    oledOk = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  }
  if (!oledOk) {
    // Tanpa layar tidak ada yang bisa dilakukan; WDT akan me-reset dan mencoba lagi
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) delay(1000);
  }
  power_applyContrast();
  display.clearDisplay();
  uint32_t t0 = micros();
  oled_flush();  // layar menyala saat boot, jadi ini mengukur satu frame penuh
  Serial.printf("OLED I2C %u kHz, kirim 1 frame %.1f ms\n", oledHz / 1000, (micros() - t0) / 1000.0f);

  ble_begin();
  clock_begin();
  bool animOk = frames_begin();
  Serial.printf("%s reset: %s, app: %s, animasi: %s\n", FW_TAG, power_resetReason(),
                esp_ota_get_running_partition()->label, animOk ? "OK" : "kosong");

  // Bangun dari deep sleep, animasi dimatikan, atau frames.bin belum ada: langsung ke Beranda
  if (power_wokeFromSleep() || !settings_bootAnim() || !animOk) state = ST_HOME;
  // Bangun oleh timer jadwal malam: BLE tersambung lagi tanpa menyalakan layar. Bila masih
  // di dalam jam malam (jam dimundurkan), power_update menidurkan lagi setelah 1 mnt.
  if (power_wokeByTimer()) power_screenOffNow();
  power_loopWaitBegin();
  bootNextAt = millis();
}

static void loopBody() {
  power_feed();
  buttons_update();
  ble_update();
  clock_loop();
  ble_setLowPowerLink(!power_screenOn());

  // Saat layar mati, tombol pertama hanya menyalakan layar
  if (btn_any_pending()) {
    // Tombol mana (masih ditahan saat event tekan diproses) untuk log "[POWER]"
    static const char *const BTN_NAMES[] = {"UP", "DOWN", "OK", "BACK"};
    const char *which = "?";
    for (int b = 0; b < BTN_COUNT; b++)
      if (btn_held((Btn)b)) which = BTN_NAMES[b];
    if (!power_screenOn()) buttons_clear();
    power_activity(PR_BUTTON, which);
  }

  bool gamePlaying = state == ST_GAME && games_isPlaying();
  // Mode OTA: layar tetap nyala & tanpa deep sleep, agar WiFi tidak hilang
  PowerReason keepAwake = state == ST_BOOT ? PR_KEEP_BOOT
                         : state == ST_OTA ? PR_KEEP_OTA
                         : ble_navActive() ? PR_KEEP_NAV
                         : (state == ST_ANIM && anim_isPlaying()) ? PR_KEEP_ANIM
                         : PR_NONE;
  power_update(keepAwake, !popupActive && !callShown && !gamePlaying && state != ST_OTA);
  power_lightSleep(settings_lightSleep() && !power_screenOn() && keepAwake == PR_NONE && !popupActive &&
                   !callShown && !gamePlaying && state != ST_OTA);

  // Layar baru menyala lagi: isi buffer saat mati tidak dikirim, jadi gambar ulang semuanya
  if (power_takeScreenWoke()) {
    redraw = true;
    marquee_reset();
    callLastDraw = 0;
    if (popupActive && !callShown) popup_draw(popupSender, popupMsg, popupTop);
  }

  if (handleCall()) return;
  if (handlePopup()) return;

  bool r = redraw;
  redraw = false;
  AppState next = state;
  switch (state) {
    case ST_BOOT:     next = bootUpdate(); break;
    case ST_MENU:     next = menu_update(r); break;
    case ST_GAME:     next = games_update(r); break;
    case ST_NAV:      next = nav_screen_update(r); break;
    case ST_ANIM:     next = anim_update(r); break;
    case ST_SETTINGS: next = settings_screen_update(r); break;
    case ST_OTA:      next = ota_update(r); break;
    case ST_PHONE:    next = phone_update(r); break;
    case ST_HOME:     next = home_update(r); break;
  }
  enterState(next);
  markAppValid();
}

static uint32_t loopIdleMs() {
  // Tombol baru berubah: polling cepat 0,5 dtk agar debounce 25 ms (dua pembacaan) tidak melewatkan tekan singkat
  if (!power_screenOn())
    return power_buttonRecentlyChanged(500) ? LOOP_IDLE_NORMAL_MS
           : power_lightSleepActive()       ? LOOP_IDLE_LIGHT_SLEEP_MS
                                            : LOOP_IDLE_SCREEN_OFF_MS;
  bool active = state == ST_GAME || state == ST_ANIM || state == ST_BOOT || state == ST_OTA ||
                popupActive || callShown;
  return active ? LOOP_IDLE_ACTIVE_MS : LOOP_IDLE_NORMAL_MS;
}

void loop() {
  uint32_t t0 = micros();
  loopBody();
  uint32_t loopUs = micros() - t0;

#if HEALTH_LOG
  static uint32_t lastHealth = 0, loops = 0, maxLoopUs = 0;
  static bool slowInGame = false;
  loops++;
  maxLoopUs = max(maxLoopUs, loopUs);
  // Rata-rata kerja loop saat layar mati (daya idle "Jaga notif")
  static uint32_t offLoops = 0, offLoopUs = 0;
  if (!power_screenOn()) { offLoops++; offLoopUs += loopUs; }
  if (state == ST_GAME && loopUs > LOOP_SLOW_WARN_MS * 1000UL) slowInGame = true;
  if (millis() - lastHealth >= HEALTH_LOG_MS) {
    static const char *const STATE_NAMES[] = {"BOOT", "MENU", "GAME", "NAV", "ANIM", "SETTINGS", "OTA", "HP", "HOME"};
    uint32_t elapsed = millis() - lastHealth;
    lastHealth = millis();
    Serial.printf("[HEALTH] CPU %u MHz, loop %u/dtk, maks %.1f ms, flush maks %.1f ms | heap %u B, min %u B, "
                  "stack sisa %u B | layar %s%s, state %s\n",
                  getCpuFrequencyMhz(), (unsigned)(loops * 1000UL / max(elapsed, 1UL)), maxLoopUs / 1000.0f,
                  power_takeFlushMaxUs() / 1000.0f, ESP.getFreeHeap(), ESP.getMinFreeHeap(),
                  (unsigned)uxTaskGetStackHighWaterMark(NULL), power_screenOn() ? "ON" : "OFF",
                  power_dimmed() ? " (redup)" : "", STATE_NAMES[state]);
    if (slowInGame)
      Serial.printf("[HEALTH] PERINGATAN: putaran loop > %d ms saat game\n", LOOP_SLOW_WARN_MS);
    if (offLoops)
      Serial.printf("[POWER] mode tidur=%s, layar mati: kerja loop rata-rata %.2f ms + idle %d ms\n",
                    settings_sleepName(), offLoopUs / 1000.0f / offLoops, LOOP_IDLE_SCREEN_OFF_MS);
#if LIGHT_SLEEP_SUPPORTED
    Serial.printf("[PM] light sleep %s (setting %s), sebab bangun terakhir=%d\n",
                  power_lightSleepActive() ? "aktif" : "ditahan", settings_lightSleep() ? "ON" : "OFF",
                  (int)esp_sleep_get_wakeup_cause());
#endif
    offLoops = offLoopUs = 0;
    loops = maxLoopUs = 0;
    slowInGame = false;
  }
#endif

  power_loopWait(loopIdleMs());
}
