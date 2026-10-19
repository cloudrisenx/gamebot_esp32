#include "phone.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include "buttons.h"
#include "ble_link.h"
#include "remote_hid.h"
#include "settings.h"
#include "lang.h"
#include <Preferences.h>
#include "screen.h"

extern Adafruit_SSD1306 display;

enum PhoneScreen { PS_LIST, PS_NOTIF_LIST, PS_NOTIF_DETAIL, PS_MUSIC, PS_CAMERA, PS_WEATHER };

// "Mode HP" (Android/iPhone) diganti langsung di kartunya (OK), tanpa layar sendiri
enum PhoneItem { PI_NOTIF, PI_MUSIC, PI_CAMERA, PI_WEATHER, PI_MODE, ITEM_COUNT };
static const char *ITEMS[] = {"Notifikasi", "Musik", "Kamera", "Cuaca", "Mode HP"};
static const char *ITEMS_EN[] = {"Notifications", "Music", "Camera", "Weather", "Phone mode"};
static const PhoneScreen ITEM_SCREENS[] = {PS_NOTIF_LIST, PS_MUSIC, PS_CAMERA, PS_WEATHER, PS_LIST};
static const uint8_t PROGMEM ICON_PHONE_NOTIF[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0x00, 0x0F, 0xF0, 0x00, 0x00, 0x3C, 0x3C, 0x00, 0x00, 0x70, 0x0E, 0x00,
  0x00, 0x60, 0x06, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x01, 0x80, 0x01, 0x80,
  0x01, 0xFF, 0xFF, 0x80, 0x01, 0xFF, 0xFF, 0x80, 0x01, 0xFF, 0xFF, 0x80, 0x00, 0xC0, 0x01, 0x80,
  0x00, 0xC0, 0x01, 0x80, 0x00, 0xC0, 0x01, 0x80, 0x00, 0xC0, 0x01, 0x80, 0x00, 0xC0, 0x01, 0x80,
  0x00, 0xC0, 0x01, 0x80, 0x00, 0xC0, 0x01, 0x80, 0x00, 0xC0, 0x01, 0x80, 0x0F, 0xFF, 0xFF, 0xF0,
  0x0F, 0xFF, 0xFF, 0xF0, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x07, 0xE0, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0x00, 0x03, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_PHONE_MUSIC[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0xF0,
  0x00, 0x00, 0x1F, 0xF0, 0x00, 0x03, 0xFF, 0xF0, 0x00, 0x7F, 0xFF, 0xF0, 0x00, 0x7F, 0xFF, 0xF0,
  0x00, 0x7F, 0xFF, 0x60, 0x00, 0x7F, 0xE0, 0x60, 0x00, 0x7E, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60,
  0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60,
  0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x00, 0x60, 0x03, 0xE0, 0x00, 0x60, 0x0F, 0xE0,
  0x00, 0x60, 0x1F, 0xF0, 0x03, 0xE0, 0x1F, 0xF0, 0x0F, 0xE0, 0x1F, 0xF0, 0x1F, 0xF0, 0x1F, 0xF0,
  0x1F, 0xF0, 0x0F, 0xE0, 0x1F, 0xF0, 0x03, 0x80, 0x1F, 0xF0, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00,
  0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_PHONE_WEATHER[] = {
  0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x40, 0x00, 0x10, 0x00, 0xE0, 0x00, 0x38, 0x78, 0x40, 0x00,
  0x11, 0xFE, 0x00, 0x00, 0x03, 0x87, 0x00, 0x00, 0x06, 0x01, 0x80, 0x00, 0x06, 0x01, 0x80, 0x00,
  0x0C, 0x00, 0xC0, 0x00, 0xCC, 0x00, 0xC0, 0x00, 0xCC, 0x00, 0xDE, 0x00, 0x0C, 0x00, 0xFF, 0x80,
  0x06, 0x01, 0xE1, 0xC0, 0x06, 0x01, 0xC0, 0xE0, 0x03, 0x01, 0x80, 0x60, 0x01, 0x0F, 0x00, 0x30,
  0x00, 0x3F, 0x00, 0x30, 0x00, 0x70, 0x00, 0x38, 0x00, 0x60, 0x00, 0x1C, 0x00, 0xC0, 0x00, 0x0E,
  0x00, 0xC0, 0x00, 0x06, 0x00, 0xC0, 0x00, 0x06, 0x00, 0xC0, 0x00, 0x06, 0x00, 0x60, 0x00, 0x0E,
  0x00, 0x70, 0x00, 0x1C, 0x00, 0x3F, 0xFF, 0xF8, 0x00, 0x0F, 0xFF, 0xF0, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_PHONE_CAMERA[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x3F, 0xF0, 0x00, 0x00, 0x3F, 0xF0, 0x00, 0x00, 0x3F, 0xF0, 0x00, 0x00, 0x3F, 0xF0, 0x00,
  0x0F, 0xFF, 0xFF, 0xF0, 0x1F, 0xFF, 0xFF, 0xF8, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x03, 0xC0, 0xEC,
  0x30, 0x0F, 0xF0, 0xEC, 0x30, 0x1C, 0x38, 0x0C, 0x30, 0x30, 0x0C, 0x0C, 0x30, 0x31, 0x8C, 0x0C,
  0x30, 0x63, 0xC6, 0x0C, 0x30, 0x67, 0xE6, 0x0C, 0x30, 0x67, 0xE6, 0x0C, 0x30, 0x63, 0xC6, 0x0C,
  0x30, 0x31, 0x8C, 0x0C, 0x30, 0x30, 0x0C, 0x0C, 0x30, 0x1C, 0x38, 0x0C, 0x30, 0x0F, 0xF0, 0x0C,
  0x30, 0x03, 0xC0, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x1F, 0xFF, 0xFF, 0xF8, 0x0F, 0xFF, 0xFF, 0xF0,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_PHONE_MODE[] = {  // dua HP + panah tukar (Android <-> iPhone)
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xF8, 0x1F, 0xFC, 0x20, 0x08, 0x10, 0x04,
  0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x90, 0x04,
  0x20, 0x0B, 0xD0, 0x04, 0x20, 0x08, 0x90, 0x04, 0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04,
  0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04, 0x20, 0x09, 0x10, 0x04, 0x20, 0x0B, 0xD0, 0x04,
  0x20, 0x09, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04, 0x20, 0x08, 0x10, 0x04,
  0x23, 0x88, 0x11, 0xC4, 0x20, 0x08, 0x10, 0x04, 0x3F, 0xF8, 0x1F, 0xFC, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t *const ITEM_ICONS[] = {ICON_PHONE_NOTIF, ICON_PHONE_MUSIC, ICON_PHONE_CAMERA, ICON_PHONE_WEATHER,
                                              ICON_PHONE_MODE};
static const uint32_t DELETE_HOLD_MS = 800;  // tahan OK di daftar Notifikasi = hapus
static const uint32_t DELETED_SHOW_MS = 800;
static const int DETAIL_LINES = 5;
static const int DETAIL_COLS = 20;     // detail: kolom ke-21 untuk penanda scroll v/^
static const int CARD_COLS = 21;
// Musik: ketuk kiri/kanan = lagu sebelumnya/berikutnya, TAHAN = volume (berulang)
static const uint32_t HOLD_MS = 600;
static const uint32_t REPEAT_MS = 300;

static PhoneScreen screen = PS_LIST;
static Carousel itemCar = {};
static Carousel notifCar = {};          // kartu Notifikasi (sel = indeks riwayat)
static int detailTop = 0;
static bool okHolding = false;          // OK sedang ditahan di daftar Notifikasi
static uint32_t okHoldStart = 0;
static uint32_t deletedUntil = 0;       // tampilkan "Dihapus" sampai waktu ini
static bool fromHome = false;          // dibuka lewat pintasan Beranda: BACK kembali ke Beranda
static PhoneStart pendingStart = PHONE_START_LIST;

// Kamera (HID Volume Up): kiri/kanan = pilih timer, OK = jepret (langsung atau setelah hitung mundur)
static const uint8_t CAM_TIMER_OPTS[] = {0, 3, 5, 10};  // detik, 0 = langsung
static const int CAM_TIMER_COUNT = sizeof(CAM_TIMER_OPTS) / sizeof(CAM_TIMER_OPTS[0]);
static const uint32_t CAM_FLASH_MS = 100, CAM_SHOT_MS = 800;
static int camTimerIdx = 0;         // disimpan di NVS "camera"/"timer" (nilai detik)
static uint32_t camTimerEnd = 0;   // 0 = tidak ada hitung mundur
static void camLoadTimer();
static uint32_t camShotAt = 0;     // waktu jepret terakhir (kilat + "Jepret!")
static bool camShotOk = false;


// Tahan/ketuk kiri-kanan di layar Musik
static const Btn MUSIC_LEFT = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, MUSIC_RIGHT = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
static int holdBtn = -1;
static uint32_t holdStart = 0;
static uint32_t lastRepeat = 0;
static bool repeated = false;       // tahan ini sudah mengubah volume (lepas tidak mengganti lagu)

// ---------- Layar Musik ----------
static const uint8_t PROGMEM ICON_PREV[] = {0x00, 0x89, 0x9B, 0xBF, 0xFF, 0xBF, 0x9B, 0x89};
static const uint8_t PROGMEM ICON_NEXT[] = {0x00, 0x91, 0xD9, 0xFD, 0xFF, 0xFD, 0xD9, 0x91};
static const uint8_t PROGMEM ICON_PLAY[] = {0x40, 0x70, 0x7C, 0x7F, 0x7C, 0x70, 0x40, 0x00};
static const uint8_t PROGMEM ICON_PAUSE[] = {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x00};
static const uint8_t PROGMEM ICON_NOTE[] = {0x0C, 0x0E, 0x0B, 0x09, 0x08, 0x78, 0xF8, 0x70};

// Tata letak (y): atas 0-7, garis 9, judul 11-30 (baseline 25), artis 32-39,
// status 41-50, garis 52, kontrol 55-62
static const int TITLE_BASELINE = 25;
static const int TITLE_LEFT = 2, TITLE_W = 124;       // area judul 2..125
static const int EQ_BARS = 7, EQ_X = 100, EQ_BASE = 50, EQ_MAX = 10;
static const uint32_t EQ_STEP_MS = 120;
static const int CTRL_Y = 55;
// Legenda 4 kolom (32 px) = urutan tombol fisik: kiri, kanan, OK, BACK
static const int CTRL_X[3] = {12, 44, 76};             // ikon prev / next / play-pause (x kiri ikon 8 px)
static const int CTRL_BACK_X = 100;                    // "BACK" (24 px) di kolom ke-4
static const uint32_t FEEDBACK_MS = 300, VOL_SHOW_MS = 800;
static const int VOL_SEGMENTS = 8;
static const uint32_t ANIM_FRAME_MS = 50;

static uint32_t trackHash = 0;      // judul yang sedang dilacak
static uint32_t elapsedMs = 0;      // timer perkiraan (lihat musicTick)
static uint32_t lastTickMs = 0;
static bool lastPlaying = false;
static uint8_t eqHeight[EQ_BARS];
static uint32_t eqLast = 0, eqSeed = 1;
static int feedbackIcon = -1;       // 0 prev, 1 next, 2 play/pause
static uint32_t feedbackUntil = 0;
static int volLevel = VOL_SEGMENTS / 2;
static int volDir = 0;              // +1 / -1 arah terakhir
static uint32_t volUntil = 0;

static const char *DAY_NAMES[] = {"Min", "Sen", "Sel", "Rab", "Kam", "Jum", "Sab"};
static const char *DAY_NAMES_EN[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *WEATHER_NAMES[] = {"Cerah berawan", "Cerah", "Salju", "Hujan", "Berawan", "Ekstrem", "Berangin", "Kabut"};
static const char *WEATHER_NAMES_EN[] = {"Partly cloudy", "Sunny", "Snow", "Rain", "Cloudy", "Extreme", "Windy", "Fog"};

static void startScreen(PhoneScreen s) {
  screen = s;
  holdBtn = -1;
  feedbackUntil = volUntil = 0;
  okHolding = false;
  camTimerEnd = 0;
  if (s == PS_CAMERA) camLoadTimer();
  bool wantsHid = s == PS_CAMERA;  // Kamera = Volume Up lewat HID
  if (wantsHid && !remote_running()) ble_remoteStart();
  else if (!wantsHid && remote_running()) ble_remoteStop();
  marquee_reset();
  if (s == PS_NOTIF_LIST) { notifCar.sel = notifCar.from = 0; ble_markRead(); }
  if (s == PS_NOTIF_DETAIL) detailTop = 0;
}

void phone_requestStart(PhoneStart s) { pendingStart = s; }

void phone_enter() {
  fromHome = pendingStart != PHONE_START_LIST;
  startScreen(pendingStart == PHONE_START_NOTIF ? PS_NOTIF_LIST
              : pendingStart == PHONE_START_MUSIC ? PS_MUSIC : PS_LIST);
  pendingStart = PHONE_START_LIST;
}

// ---------- Gambar ----------

static void beginScreen(const char *title) {
  display.clearDisplay();
  drawHeader(title);
  display.setTextColor(SSD1306_WHITE);
}

static void centerLine(int y, const String &s) {
  display.setCursor((128 - (int)s.length() * 6) / 2, y);
  display.print(s);
}

static bool showNotConnected() {
  if (ble_connected()) return false;
  centerLine(30, T("HP belum terhubung", "Phone not connected"));
  return true;
}

// Kartu yang tampil: iPhone tidak mengirim data cuaca, jadi Cuaca disembunyikan
static int visibleItems(int *idx) {
  int n = 0;
  for (int i = 0; i < ITEM_COUNT; i++)
    if (!(i == PI_WEATHER && settings_phoneMode() == PHONE_IPHONE)) idx[n++] = i;
  return n;
}

static void drawList() {
  int vis[ITEM_COUNT], n = visibleItems(vis);
  if (itemCar.sel >= n) itemCar.sel = n - 1;
  const uint8_t *icons[ITEM_COUNT];
  for (int i = 0; i < n; i++) icons[i] = ITEM_ICONS[vis[i]];
  int item = vis[itemCar.sel];
  // Notifikasi: jumlah riwayat ikut di nama, mis. "Notifikasi (3)"; Mode HP: pilihan sekarang
  char name[24];
  if (item == PI_NOTIF && ble_historyCount()) snprintf(name, sizeof(name), "%s (%d)", LT(ITEMS, ITEMS_EN)[item], ble_historyCount());
  else if (item == PI_MODE) snprintf(name, sizeof(name), "%s: %s", T("Mode", "Mode"), settings_phoneMode() == PHONE_IPHONE ? "iPhone" : "Android");
  else snprintf(name, sizeof(name), "%s", LT(ITEMS, ITEMS_EN)[item]);
  carousel_draw(itemCar, T("HP", "PHONE"), icons, n, name);
}

// Isi satu kartu notifikasi: pengirim (berjalan bila panjang) + cuplikan pesan 3 baris
static void notifCard(int i, int dx) {
  const NotifEntry &e = ble_historyAt(i);
  marquee_draw(dx, 14, 128, asciiOnly(e.title).c_str(), SSD1306_WHITE, nullptr, dx == 0);
  display.drawFastHLine(dx, 23, 128, SSD1306_WHITE);
  drawWrappedAscii(dx, 26, CARD_COLS, 3, e.msg);
}

static void drawNotifList() {
  int n = ble_historyCount();
  // Hint pendek: "OK buka  tahan=hapus" + "10/10" tidak muat 128 px; ketuk OK = buka tetap berlaku
  card_draw(notifCar, n ? ble_historyAt(notifCar.sel).time : "NOTIF", n, notifCard, T("tahan OK=hapus", "hold OK=delete"));
  if (!n) {
    display.setTextColor(SSD1306_WHITE);
    centerLine(30, T("Belum ada notifikasi", "No notifications yet"));
  }
  // Tahan OK: bar terisi di dasar kartu sampai DELETE_HOLD_MS
  if (n && okHolding) {
    uint32_t held = min((uint32_t)(millis() - okHoldStart), DELETE_HOLD_MS);
    display.fillRect(0, 51, held * 128 / DELETE_HOLD_MS, 2, SSD1306_WHITE);
  }
  if (millis() < deletedUntil) {
    const int bx = 30, by = 20, bw = 68, bh = 18;
    display.fillRect(bx, by, bw, bh, SSD1306_BLACK);
    display.drawRect(bx, by, bw, bh, SSD1306_WHITE);
    centerLine(by + 5, T("Dihapus", "Deleted"));
  }
}

static void drawNotifDetail() {
  const NotifEntry &e = ble_historyAt(notifCar.sel);
  beginScreen(e.time);
  marquee_draw(0, 13, 128, asciiOnly(e.title).c_str());
  display.drawFastHLine(0, 22, 128, SSD1306_WHITE);
  int n = drawWrappedAscii(0, 0, DETAIL_COLS, 0, e.msg);  // hitung baris saja
  detailTop = constrain(detailTop, 0, max(0, n - DETAIL_LINES + 1));
  drawWrappedAscii(0, 25, DETAIL_COLS, DETAIL_LINES - 1, e.msg, detailTop);
  if (n > DETAIL_LINES - 1) printRight(128, 56, detailTop + DETAIL_LINES - 1 < n ? "v" : "^");
}

static bool musicPlaying() { return ble_connected() && ble_music().state == 1; }

static bool musicAnimating() {
  if (!power_screenOn()) return false;
  uint32_t now = millis();
  if (now < feedbackUntil || now < volUntil) return true;
  return musicPlaying();  // equalizer
}

// Ikon 8x8 versi "abu": hanya piksel papan catur (tidak terhubung)
static void drawIconGray(int x, int y, const uint8_t *icon) {
  display.drawBitmap(x, y, icon, 8, 8, SSD1306_WHITE);
  for (int yy = 0; yy < 8; yy++)
    for (int xx = (yy & 1); xx < 8; xx += 2) display.drawPixel(x + xx, y + yy, SSD1306_BLACK);
}

static void drawEqualizer(bool playing) {
  for (int i = 0; i < EQ_BARS; i++) {
    int h = playing ? max(1, (int)eqHeight[i]) : 1;
    display.fillRect(EQ_X + i * 4, EQ_BASE - h + 1, 3, h, SSD1306_WHITE);
  }
}

static void drawControls(bool connected, bool playing) {
  const uint8_t *icons[3] = {ICON_PREV, ICON_NEXT, playing ? ICON_PAUSE : ICON_PLAY};
  for (int i = 0; i < 3; i++) {
    int x = CTRL_X[i];
    if (!connected) { drawIconGray(x, CTRL_Y, icons[i]); continue; }
    bool active = i == feedbackIcon && millis() < feedbackUntil;
    if (active) display.fillRoundRect(x - 3, CTRL_Y - 2, 14, 11, 2, SSD1306_WHITE);
    display.drawBitmap(x, CTRL_Y, icons[i], 8, 8, active ? SSD1306_BLACK : SSD1306_WHITE);
  }
  display.setCursor(CTRL_BACK_X, CTRL_Y);
  display.print("BACK");
}

static void drawVolumeOverlay() {
  const int bx = 24, by = 14, bw = 80, bh = 28;
  display.fillRect(bx, by, bw, bh, SSD1306_BLACK);
  display.drawRect(bx, by, bw, bh, SSD1306_WHITE);
  centerLine(by + 4, volDir > 0 ? "VOL +" : "VOL -");
  // Indikator relatif (bukan level asli HP: Chronos tidak mengirim level volume)
  for (int i = 0; i < VOL_SEGMENTS; i++) {
    int x = bx + 8 + i * 8;
    if (i < volLevel) display.fillRect(x, by + 16, 6, 6, SSD1306_WHITE);
    else display.drawRect(x, by + 16, 6, 6, SSD1306_WHITE);
  }
}

static void drawMusic() {
  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  bool connected = ble_connected();
  MusicInfo &m = ble_music();

  // Baris atas: ikon not + nama aplikasi, jam di kanan
  display.drawBitmap(0, 0, ICON_NOTE, 8, 8, SSD1306_WHITE);
  String app = connected ? asciiOnly(m.appName.c_str()) : String();
  marquee_draw(10, 0, 84, app.length() ? app.c_str() : T("Musik", "Music"));  // sampai sebelum jam (x=98)
  char hhmm[6];
  printRight(128, 0, ble_timeStr(hhmm, sizeof(hhmm)) ? hhmm : "--:--");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

  String title = connected ? asciiOnly(m.title.c_str()) : String();
  bool playing = connected && m.state == 1;
  int progressW = -1;  // >= 0 bila posisi lagu asli tersedia (iPhone)
  if (!connected) {
    centerLine(17, T("HP belum terhubung", "Phone not connected"));
  } else if (!title.length()) {
    centerLine(17, T("Belum ada lagu", "No song playing"));
    centerLine(32, T("Putar musik di HP", "Play music on phone"));
  } else {
    marquee_draw(TITLE_LEFT, TITLE_BASELINE, TITLE_W, title.c_str(), SSD1306_WHITE, &FreeSansBold9pt7b);
    marquee_draw(0, 32, 128, asciiOnly(m.artist.c_str()).c_str());
  }

  // Status: ikon + timer perkiraan, equalizer di kanan
  if (connected && title.length()) {
    display.drawBitmap(0, 42, playing ? ICON_PLAY : ICON_PAUSE, 8, 8, SSD1306_WHITE);
    uint32_t posMs, durMs;
    display.setCursor(11, 42);
    if (ble_musicProgress(posMs, durMs)) {
      // iPhone (AMS) mengirim posisi & durasi asli: tampilkan apa adanya + progress bar
      uint32_t p = posMs / 1000, d = durMs / 1000;
      display.printf("%u:%02u/%u:%02u", (unsigned)(p / 60), (unsigned)(p % 60), (unsigned)(d / 60), (unsigned)(d % 60));
      progressW = durMs ? (int)((uint64_t)posMs * 128 / durMs) : 0;
    } else {
      uint32_t sec = elapsedMs / 1000;
      display.printf("~%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
    }
    drawEqualizer(playing);
  }

  if (progressW >= 0) {
    // Progress bar tipis menggantikan garis pemisah: terisi = sudah diputar, titik-titik = sisa
    for (int x = 0; x < 128; x += 2) display.drawPixel(x, 52, SSD1306_WHITE);
    display.drawFastHLine(0, 52, progressW, SSD1306_WHITE);
    display.drawFastHLine(0, 53, progressW, SSD1306_WHITE);
  } else {
    display.drawFastHLine(0, 52, 128, SSD1306_WHITE);
  }
  drawControls(connected, playing);
  if (connected && millis() < volUntil) drawVolumeOverlay();
}

static void drawWeather() {
  if (ble_isIphone()) { draw_androidOnly(T("CUACA", "WEATHER"), T("iPhone tidak mengirim", "iPhone does not send"), T("data cuaca.", "weather data.")); return; }
  beginScreen(T("CUACA", "WEATHER"));
  int n = ble_weatherCount();
  if (!n && showNotConnected()) return;  // cache cuaca tetap tampil walau belum tersambung
  if (!n) {
    centerLine(24, T("Belum ada data cuaca", "No weather data yet"));
    centerLine(36, T("Aktifkan di Chronos", "Enable it in Chronos"));
    return;
  }
  marquee_draw(0, 13, 128, asciiOnly(ble_weatherCity().c_str()).c_str());
  Weather &t = ble_weatherAt(0);
  display.setCursor(0, 23);
  display.printf("%dC %s", t.temp, LT(WEATHER_NAMES, WEATHER_NAMES_EN)[constrain(t.icon, 0, 7)]);
  display.setCursor(0, 33);
  if (t.high != 0 || t.low != 0) display.printf(T("Min %dC Maks %dC", "Low %dC High %dC"), t.low, t.high);
  // 3 hari berikutnya: "Sen 30C Sel 29C ..."
  String next;
  for (int i = 1; i <= 3 && i < n; i++) {
    Weather &w = ble_weatherAt(i);
    next += String(LT(DAY_NAMES, DAY_NAMES_EN)[w.day % 7]) + " " + w.temp + "C ";
  }
  marquee_draw(0, 44, 128, next.c_str());
  display.setCursor(0, 56);
  display.print(T("Update ", "Updated "));
  display.print(ble_weatherTime());
  if (ble_weatherIsCache()) {
    int age = ble_weatherCacheAgeH();
    display.print(age < 0 ? T(" lama", " old") : (" " + String(age) + T("j lalu", "h ago")).c_str());
  }
}

static void camLoadTimer() {
  Preferences p;
  uint8_t sec = 0;
  if (p.begin("camera", true)) { sec = p.getUChar("timer", 0); p.end(); }
  camTimerIdx = 0;
  for (int i = 0; i < CAM_TIMER_COUNT; i++)
    if (CAM_TIMER_OPTS[i] == sec) camTimerIdx = i;
}

static void camSaveTimer() {
  Preferences p;
  if (p.begin("camera", false)) { p.putUChar("timer", CAM_TIMER_OPTS[camTimerIdx]); p.end(); }
}

static void camCapture() {
  camShotOk = remote_connected();
  remote_volumeUp();  // Volume Up = tombol jepret di aplikasi Kamera bawaan iPhone & Android
  camShotAt = millis();
  camTimerEnd = 0;
}

static bool camAnimating() {
  uint32_t now = millis();
  return camTimerEnd || (camShotAt && now - camShotAt < CAM_SHOT_MS + 100);
}

// Status pairing HID di layar Kamera (teks di kanan ikon, x=36). true bila belum siap dipakai.
static bool drawHidStatus() {
  const char *l[4] = {nullptr, nullptr, nullptr, nullptr};
  if (remote_bondError()) { l[0] = T("Gagal siapkan", "Pairing setup"); l[1] = T("pairing.", "failed."); l[2] = T("Lihat Setting", "See Settings"); l[3] = "> Debug log"; }
  else if (remote_rejectedFull()) { l[0] = T("Memori pairing", "Pairing memory"); l[1] = T("penuh (3 HP)", "full (3 phones)"); l[2] = T("Lupakan HP lama", "Forget old one"); l[3] = T("lalu coba lagi", "then try again"); }
  else if (!remote_connected()) { l[0] = T("Pair di", "Pair in phone"); l[1] = T("Bluetooth HP:", "Bluetooth:"); static String bt; bt = settings_bleName(); l[2] = bt.c_str(); l[3] = "Remote"; }
  else return false;
  static const int Y[4] = {14, 23, 34, 43};
  for (int i = 0; i < 4; i++) {
    display.setCursor(36, Y[i]);
    display.print(l[i]);
  }
  return true;
}

static void drawCamera() {
  uint32_t now = millis();
  if (camShotAt && now - camShotAt < CAM_FLASH_MS) {  // kilat putih
    display.clearDisplay();
    display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
    return;
  }
  beginScreen(T("KAMERA", "CAMERA"));
  if (camShotAt && now - camShotAt < CAM_SHOT_MS) {
    display.setTextSize(2);
    const char *msg = camShotOk ? T("Jepret!", "Snap!") : T("Gagal", "Failed");
    display.setCursor((128 - (int)strlen(msg) * 12) / 2, 26);
    display.print(msg);
    display.setTextSize(1);
    if (!camShotOk) centerLine(48, T("HP belum terhubung", "Phone not connected"));
    return;
  }
  if (camTimerEnd) {  // hitung mundur besar (10..1)
    char digit[4];
    snprintf(digit, sizeof(digit), "%u", (unsigned)((camTimerEnd - now) / 1000 + 1));
    display.setTextSize(4);
    display.setCursor(64 - (int)strlen(digit) * 12, 18);
    display.print(digit);
    display.setTextSize(1);
    centerLine(56, T("BACK: batal", "BACK: cancel"));
    return;
  }
  display.drawBitmap(0, 14, ICON_PHONE_CAMERA, 32, 32, SSD1306_WHITE);
  if (drawHidStatus()) return;
  display.setCursor(36, 14); display.print("Timer:");
  char t[8];
  uint8_t sec = CAM_TIMER_OPTS[camTimerIdx];
  if (sec) snprintf(t, sizeof(t), T("%u dtk", "%u sec"), sec);
  else snprintf(t, sizeof(t), "OFF");
  display.setTextSize(2);
  display.setCursor(36, 25);
  display.print(t);
  display.setTextSize(1);
  display.setCursor(36, 43); display.print(T("Buka Kamera HP", "Open camera"));
  display.drawFastHLine(0, 53, 128, SSD1306_WHITE);
  centerLine(56, T("<> timer  OK jepret", "<> timer  OK snap"));
}

static void draw() {
  switch (screen) {
    case PS_LIST:         drawList(); break;
    case PS_NOTIF_LIST:   drawNotifList(); break;
    case PS_NOTIF_DETAIL: drawNotifDetail(); break;
    case PS_MUSIC:        drawMusic(); break;
    case PS_CAMERA:       drawCamera(); break;
    case PS_WEATHER:      drawWeather(); break;
  }
  oled_flush();
}

// ---------- Input ----------

static void feedback(int icon) {
  feedbackIcon = icon;
  feedbackUntil = millis() + FEEDBACK_MS;
}

static void volumeStep(int dir) {
  if (millis() >= volUntil) volLevel = VOL_SEGMENTS / 2;  // sesi baru: mulai dari tengah
  volDir = dir;
  volLevel = constrain(volLevel + dir, 0, VOL_SEGMENTS);
  volUntil = millis() + VOL_SHOW_MS;
}

// Timer perkiraan "sudah diputar": Chronos TIDAK mengirim posisi/durasi lagu, jadi ESP32
// menghitung sendiri sejak judul terakhir berganti, hanya selama status memutar. Jika layar
// HP ditutup lalu dibuka lagi dengan judul sama & tetap memutar, waktu selama ditutup
// dianggap ikut berputar. Skip/geser posisi di HP tidak terdeteksi -> ditandai "~".
static void musicTick() {
  uint32_t now = millis();
  bool connected = ble_connected();
  MusicInfo &m = ble_music();
  uint32_t hash = connected ? sig_str(2166136261u, m.title.c_str()) : trackHash;
  bool playing = connected && m.state == 1;
  if (hash != trackHash) {
    trackHash = hash;
    elapsedMs = 0;
    marquee_reset();  // lagu baru: judul mulai lagi dari jeda awal
  } else if (playing && lastPlaying) {
    elapsedMs += now - lastTickMs;
  }
  lastTickMs = now;
  lastPlaying = playing;

  // Equalizer pseudo-acak tiap EQ_STEP_MS (hanya saat memutar & layar menyala)
  if (playing && power_screenOn() && now - eqLast >= EQ_STEP_MS) {
    eqLast = now;
    for (int i = 0; i < EQ_BARS; i++) {
      eqSeed = eqSeed * 1103515245u + 12345u;
      eqHeight[i] = 1 + (eqSeed >> 16) % EQ_MAX;
    }
  }
}

// Musik: ketuk kiri/kanan = lagu sebelumnya/berikutnya, OK = putar/jeda, tahan kiri/kanan = volume -/+ (berulang)
static bool musicInput() {
  bool changed = false;
  uint32_t now = millis();
  if (btn_pressed(BTN_OK)) { ble_musicControl(MUSIC_TOGGLE); feedback(2); changed = true; }
  if (btn_pressed(MUSIC_LEFT)) { holdBtn = MUSIC_LEFT; holdStart = now; repeated = false; }
  if (btn_pressed(MUSIC_RIGHT)) { holdBtn = MUSIC_RIGHT; holdStart = now; repeated = false; }
  if (holdBtn >= 0) {
    bool right = holdBtn == MUSIC_RIGHT;
    if (btn_held((Btn)holdBtn)) {
      if (now - holdStart >= HOLD_MS && now - lastRepeat >= REPEAT_MS) {
        ble_musicControl(right ? VOLUME_UP : VOLUME_DOWN);
        volumeStep(right ? 1 : -1);
        lastRepeat = now;
        repeated = changed = true;
      }
    } else {
      if (!repeated) {  // ketukan (dilepas sebelum 0,6 dtk) = ganti lagu
        ble_musicControl(right ? MUSIC_NEXT : MUSIC_PREVIOUS);
        feedback(right ? 1 : 0);
        changed = true;
      }
      holdBtn = -1;
    }
  }
  return changed;
}

// Tanda isi layar HP: header, jumlah/notifikasi terbaru, koneksi, info Musik
static uint32_t phoneSignature() {
  uint32_t h = sig_mix(header_signature(), ble_historyCount());
  if (ble_historyCount()) {  // riwayat penuh (10): jumlah tetap, tapi isi terbaru berganti
    h = sig_str(h, ble_historyAt(0).time);
    h = sig_str(h, ble_historyAt(0).title);
  }
  if (screen == PS_MUSIC) {  // judul, artis, aplikasi, status, detik timer
    MusicInfo &m = ble_music();
    h = sig_str(h, m.title.c_str());
    h = sig_str(h, m.artist.c_str());
    h = sig_str(h, m.appName.c_str());
    h = sig_mix(h, m.state);
    h = sig_mix(h, elapsedMs / 1000);
  }
  if (screen == PS_CAMERA)
    h = sig_mix(h, remote_connected() * 4 + remote_rejectedFull() * 2 + remote_bondError());
  return h;
}

AppState phone_update(bool redraw) {
  static RedrawWatch watch;
  bool back = btn_pressed(BTN_BACK);

  switch (screen) {
    case PS_LIST:
      if (back) return ST_MENU;
    {
      int vis[ITEM_COUNT], n = visibleItems(vis);
      if (itemCar.sel >= n) itemCar.sel = n - 1;
      if (carousel_input(itemCar, n)) { marquee_reset(); redraw = true; }
      if (btn_pressed(BTN_OK)) {
        int item = vis[itemCar.sel];
        // ble_update() menghentikan BLE lama & memulai mode baru secara otomatis
        if (item == PI_MODE) settings_setPhoneMode((PhoneMode)(settings_phoneMode() + 1));
        else startScreen(ITEM_SCREENS[item]);
        redraw = true;
      }
    }
      if (carousel_frameDue(itemCar)) redraw = true;
      break;

    case PS_NOTIF_LIST: {
      int n = ble_historyCount();
      if (back) {
        if (fromHome) return ST_HOME;
        startScreen(PS_LIST);
        redraw = true;
        break;
      }
      if (n && carousel_input(notifCar, n)) { okHolding = false; marquee_reset(); redraw = true; }
      if (carousel_frameDue(notifCar)) redraw = true;
      // Ketuk OK = buka detail, tahan DELETE_HOLD_MS = hapus notifikasi terpilih
      if (n && btn_pressed(BTN_OK)) { okHolding = true; okHoldStart = millis(); redraw = true; }
      if (okHolding) {
        if (!btn_held(BTN_OK)) {
          okHolding = false;
          startScreen(PS_NOTIF_DETAIL);
          redraw = true;
          break;
        }
        if (millis() - okHoldStart >= DELETE_HOLD_MS) {
          okHolding = false;
          ble_historyDelete(notifCar.sel);
          n = ble_historyCount();
          notifCar.sel = notifCar.from = n ? min(notifCar.sel, n - 1) : 0;  // item berikutnya naik ke posisi yang sama
          deletedUntil = millis() + DELETED_SHOW_MS;
          marquee_reset();
        }
        redraw = true;  // bar tahan OK bergerak
      }
      if (millis() < deletedUntil + 100) redraw = true;  // "Dihapus" muncul lalu hilang tepat waktu
      break;
    }

    case PS_NOTIF_DETAIL:
      if (back || btn_pressed(BTN_OK)) { screen = PS_NOTIF_LIST; redraw = true; break; }
      if (btn_pressed(BTN_UP) && detailTop > 0) { detailTop--; redraw = true; }
      if (btn_pressed(BTN_DOWN)) { detailTop++; redraw = true; }
      break;

    case PS_MUSIC:
      if (back) {
        if (fromHome) return ST_HOME;
        startScreen(PS_LIST);
        redraw = true;
        break;
      }
      if (ble_connected() && musicInput()) redraw = true;
      break;

    case PS_CAMERA: {
      if (back) {
        if (camTimerEnd) { camTimerEnd = 0; redraw = true; break; }  // BACK membatalkan timer dulu
        startScreen(PS_LIST);
        redraw = true;
        break;
      }
      uint32_t now = millis();
      Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP;
      Btn rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
      // Kiri/kanan = timer turun/naik (tidak saat hitung mundur), disimpan
      int dir = btn_pressed(leftBtn) ? -1 : btn_pressed(rightBtn) ? 1 : 0;
      if (dir && !camTimerEnd) {
        camTimerIdx = constrain(camTimerIdx + dir, 0, CAM_TIMER_COUNT - 1);
        camSaveTimer();
        redraw = true;
      }
      // OK = jepret langsung (timer OFF) atau mulai hitung mundur
      if (btn_pressed(BTN_OK) && !camTimerEnd && remote_connected()) {
        uint8_t sec = CAM_TIMER_OPTS[camTimerIdx];
        if (sec) camTimerEnd = now + sec * 1000UL;
        else camCapture();
        redraw = true;
      }
      if (camTimerEnd && (int32_t)(now - camTimerEnd) >= 0) camCapture();
      if (camAnimating()) redraw = true;  // angka hitung mundur, kilat
      break;
    }

    case PS_WEATHER:
      if (back) { startScreen(PS_LIST); redraw = true; }
      break;
  }
  if (screen != PS_MUSIC) buttons_clear();

  // Data baru dari HP, status koneksi/jam: gambar ulang berbasis tanda
  musicTick();
  if (ble_phoneInfoChanged()) redraw = true;
  if (marquee_frameDue()) redraw = true;
  // Musik beranimasi (equalizer/umpan balik/volume) hanya saat layar menyala
  static uint32_t lastAnimFrame = 0;
  if (screen == PS_MUSIC && musicAnimating() && millis() - lastAnimFrame >= ANIM_FRAME_MS) {
    lastAnimFrame = millis();
    redraw = true;
  }
  uint32_t period = screen == PS_MUSIC ? 250 : 1000;
  if (redraw_needed(watch, redraw, phoneSignature, period)) draw();
  return ST_PHONE;
}
