#include "screen.h"
#include "buttons.h"
#include "clockstore.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include <esp_task_wdt.h>
#include <esp_sleep.h>
#include <driver/gpio.h>
#include <esp_pm.h>
#include "settings.h"
#include "ble_link.h"

extern Adafruit_SSD1306 display;

static bool wokeFromSleep = false;
#if LIGHT_SLEEP_SUPPORTED
static esp_pm_lock_handle_t lsLock = nullptr, cpuLock = nullptr;
#endif
static bool lsActive = false;
static bool wokeByTimer = false;
static uint32_t screenOffAt = 0;  // millis() saat layar mati (jadwal malam: tidur 1 mnt sesudahnya)
static esp_reset_reason_t resetReason;
static bool screenOn = true;
static uint32_t lastActivity = 0;
static bool sleepPending = false;
static uint32_t sleepAt = 0;

void power_begin() {
  resetReason = esp_reset_reason();
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  wokeByTimer = cause == ESP_SLEEP_WAKEUP_TIMER;
  wokeFromSleep = cause == ESP_SLEEP_WAKEUP_GPIO || wokeByTimer;
  if (wokeFromSleep) Serial.printf("[POWER] bangun: %s\n", wokeByTimer ? "timer" : "tombol");

  // TWDT sudah diinisialisasi core Arduino untuk idle task; init ulang memperbarui timeout
#if ESP_IDF_VERSION_MAJOR >= 5
  // IDF 5: konfigurasi lewat struct; TWDT sudah aktif -> reconfigure (init akan ESP_ERR_INVALID_STATE)
  esp_task_wdt_config_t wdtCfg = {};
  wdtCfg.timeout_ms = WDT_TIMEOUT_S * 1000;
#ifdef CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0
  wdtCfg.idle_core_mask = 1 << 0;  // tetap awasi idle task seperti sebelumnya
#endif
  wdtCfg.trigger_panic = true;
  esp_task_wdt_reconfigure(&wdtCfg);
#else
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
#endif
  esp_task_wdt_add(NULL);  // loop task (setup() & loop() berjalan di task yang sama)
  lastActivity = millis();
#if LIGHT_SLEEP_SUPPORTED
  // Kunci dipegang sejak awal: perilaku sama dengan firmware lama sampai power_lightSleep(true)
  esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, 0, "awake", &lsLock);
  esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX, 0, "cpu", &cpuLock);
  esp_pm_lock_acquire(lsLock);
  esp_pm_lock_acquire(cpuLock);
  esp_pm_config_t pm = {};
  pm.max_freq_mhz = getCpuFrequencyMhz();  // CPU_MHZ dari setup()
  pm.min_freq_mhz = 40;                    // XTAL
  pm.light_sleep_enable = true;
  esp_err_t e = esp_pm_configure(&pm);
  esp_sleep_enable_gpio_wakeup();
  Serial.printf("[PM] configure %u-%u MHz, light sleep: %s\n", pm.min_freq_mhz, pm.max_freq_mhz, esp_err_to_name(e));
#endif
}

void power_feed() { esp_task_wdt_reset(); }

bool power_wokeFromSleep() { return wokeFromSleep; }
bool power_wokeByTimer() { return wokeByTimer; }

const char *power_resetReason() {
  switch (resetReason) {
    case ESP_RST_POWERON:   return "Power on";
    case ESP_RST_EXT:       return "Pin reset";
    case ESP_RST_SW:        return "Software";
    case ESP_RST_PANIC:     return "Crash/panic";
    case ESP_RST_INT_WDT:   return "Interrupt WDT";
    case ESP_RST_TASK_WDT:  return "Task WDT";
    case ESP_RST_WDT:       return "WDT lain";
    case ESP_RST_DEEPSLEEP: return "Deep sleep";
    case ESP_RST_BROWNOUT:  return "Brownout";
    default:                return "Lainnya";
  }
}

void power_applyContrast() {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(settings_contrastValue());
}

static bool screenWoke = false;
static bool manualOff = false;  // dimatikan BACK di Beranda: keepAwake tidak boleh menyalakan

static const char *const REASON_NAMES[] = {"-", "TOMBOL", "POPUP", "PANGGILAN", "KEEP_NAV", "KEEP_ANIM",
                                           "KEEP_BOOT", "KEEP_OTA", "TIMEOUT", "MANUAL"};

static void setScreen(bool on, PowerReason reason, const char *detail = nullptr) {
  if (on == screenOn) return;
  screenOn = on;
  if (on) {
    screenWoke = true;
    manualOff = false;
  } else {
    screenOffAt = millis();
  }
  display.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
  Serial.printf("[POWER] layar %s alasan=%s%s%s\n", on ? "ON" : "OFF", REASON_NAMES[reason],
                detail ? " " : "", detail ? detail : "");
}

static bool dimmed = false;
static const uint8_t DIM_CONTRAST = 0x01;
static uint32_t flushMaxUs = 0;

void oled_flush() {
  if (!screenOn) return;
  uint32_t t0 = micros();
  oled_sendFrame();  // cek error I2C + kirim ulang sekali (screen.cpp)
  flushMaxUs = max(flushMaxUs, micros() - t0);
}

uint32_t power_takeFlushMaxUs() {
  uint32_t m = flushMaxUs;
  flushMaxUs = 0;
  return m;
}

bool power_dimmed() { return dimmed; }

// OLED makin terang makin boros: redupkan dulu sebelum mati
static void setDim(bool on) {
  if (on == dimmed) return;
  dimmed = on;
  if (on) {
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(DIM_CONTRAST);
  } else {
    power_applyContrast();
  }
}

bool power_takeScreenWoke() {
  bool w = screenWoke;
  screenWoke = false;
  return w;
}

void power_activity(PowerReason reason, const char *detail) {
  lastActivity = millis();  // keepAwake tetap menunda deep sleep walau layar dimatikan manual
  bool keepReason = reason == PR_KEEP_NAV || reason == PR_KEEP_ANIM || reason == PR_KEEP_BOOT || reason == PR_KEEP_OTA;
  if (manualOff && keepReason) return;
  setScreen(true, reason, detail);
  setDim(false);
}

bool power_screenOn() { return screenOn; }

void power_screenOffNow() {
  setScreen(false, PR_MANUAL);
  manualOff = true;
}

void power_requestSleep(uint32_t delayMs) {
  sleepPending = true;
  sleepAt = millis() + delayMs;
}

bool power_sleepPending() { return sleepPending; }

// timerMin > 0: bangun juga lewat timer (akhir jadwal malam)
static void enterDeepSleep(uint32_t timerMin = 0) {
  // Tombol bangun yang masih ditekan akan langsung membangunkan lagi: tunggu dilepas
  if (digitalRead(WAKE_PIN_UP) == LOW || digitalRead(WAKE_PIN_DOWN) == LOW) return;
  if (timerMin) esp_sleep_enable_timer_wakeup((uint64_t)timerMin * 60ULL * 1000000ULL);

  setScreen(false, PR_TIMEOUT);
  clock_save("tidur");
  ble_shutdown();

  const gpio_num_t pins[] = {(gpio_num_t)WAKE_PIN_UP, (gpio_num_t)WAKE_PIN_DOWN};
  for (gpio_num_t p : pins) {
    gpio_pullup_en(p);
    gpio_pulldown_dis(p);
  }
  esp_deep_sleep_enable_gpio_wakeup(BIT(WAKE_PIN_UP) | BIT(WAKE_PIN_DOWN), ESP_GPIO_WAKEUP_GPIO_LOW);
  Serial.println("Deep sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}

static const uint32_t NIGHT_IDLE_MS = 60000;   // layar mati 1 mnt sebelum tidur malam
static const uint32_t NIGHT_MAX_MIN = 24 * 60;

// Menit sampai akhir jendela malam [start, end) dalam menit (boleh melewati tengah malam), 0 = di luar / OFF
uint32_t power_nightMinutesLeft(int nowMin, int start, int end) {
  if (start < 0 || start == end) return 0;
  bool inside = start <= end ? (nowMin >= start && nowMin < end) : (nowMin >= start || nowMin < end);
  if (!inside) return 0;
  uint32_t left = (uint32_t)((end - nowMin + 24 * 60) % (24 * 60));
  return min(left, NIGHT_MAX_MIN);
}

// Jam sekarang dalam menit sejak 00:00; false bila jam belum sinkron (jadwal malam diabaikan)
static bool clockMinutes(int &mins) {
  char hhmm[6];
  // Jam PERKIRAAN (dipulihkan setelah mati) bisa meleset jauh: jangan tidur malam memakai jam itu
  if (clock_source() == CLK_ESTIMATE || !ble_timeStr(hhmm, sizeof(hhmm))) return false;
  mins = atoi(hhmm) * 60 + atoi(hhmm + 3);
  return true;
}

void power_update(PowerReason keepAwake, bool sleepAllowed) {
  uint32_t now = millis();

  if (sleepPending) {
    if ((int32_t)(now - sleepAt) >= 0) enterDeepSleep();
    return;
  }

  if (keepAwake != PR_NONE) {
    power_activity(keepAwake);
    return;
  }

  uint32_t idle = now - lastActivity;
  uint32_t screenMs = settings_screenTimeoutMs();
  if (screenOn && screenMs && idle >= screenMs / 2) setDim(true);
  if (screenOn && screenMs && idle >= screenMs) setScreen(false, PR_TIMEOUT);

  uint32_t sleepMs = settings_sleepTimeoutMs();
  if (sleepAllowed && sleepMs && idle >= sleepMs) enterDeepSleep();

  // Jadwal malam: layar sudah mati >= 1 mnt tanpa aktivitas -> deep sleep sampai jam akhir malam
  int nowMin;
  if (sleepAllowed && !screenOn && now - screenOffAt >= NIGHT_IDLE_MS && idle >= NIGHT_IDLE_MS &&
      clockMinutes(nowMin)) {
    uint32_t mins = settings_nightOn() ? power_nightMinutesLeft(nowMin, settings_nightStartMin(), settings_nightEndMin()) : 0;
    if (mins) {
      uint32_t endMin = (nowMin + mins) % (24 * 60);
      Serial.printf("[POWER] malam: tidur sampai %02u:%02u (%u mnt)\n", endMin / 60, endMin % 60, mins);
      enterDeepSleep(mins);
    }
  }
}

// ---- Tunggu loop: tidur sampai batas waktu, atau bangun seketika oleh tombol (ISR) / BLE (task lain) ----
static TaskHandle_t loopTask = nullptr;
static volatile TickType_t lastEdgeTick = 0;

static void IRAM_ATTR onButtonEdge() {
  lastEdgeTick = xTaskGetTickCountFromISR();
  if (!loopTask) return;
  BaseType_t woken = pdFALSE;
  vTaskNotifyGiveFromISR(loopTask, &woken);
  if (woken) portYIELD_FROM_ISR();
}

void power_loopWaitBegin() {
  loopTask = xTaskGetCurrentTaskHandle();
  lastEdgeTick = xTaskGetTickCount();
  buttons_attachWake(onButtonEdge);
}

void power_loopWait(uint32_t ms) { ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(ms)); }

void power_loopWake() {
  if (loopTask) xTaskNotifyGive(loopTask);
}

bool power_buttonRecentlyChanged(uint32_t ms) { return xTaskGetTickCount() - lastEdgeTick < pdMS_TO_TICKS(ms); }

void power_lightSleep(bool allowed) {
#if LIGHT_SLEEP_SUPPORTED
  if (allowed) buttons_armWake(true);  // pin yang baru dilepas dipersenjatai lagi
  if (allowed == lsActive) return;
  lsActive = allowed;
  if (allowed) {
    esp_pm_lock_release(cpuLock);
    esp_pm_lock_release(lsLock);
  } else {
    esp_pm_lock_acquire(lsLock);
    esp_pm_lock_acquire(cpuLock);
    buttons_armWake(false);
  }
  Serial.printf("[PM] light sleep %s\n", allowed ? "boleh" : "ditahan");
#else
  (void)allowed;
#endif
}

bool power_lightSleepActive() { return lsActive; }
