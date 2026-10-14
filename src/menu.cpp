#include <soc/rtc_cntl_reg.h>
#include <esp32c3/rom/rtc.h>
#include "clockstore.h"
#include "menu.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include "settings.h"
#include "ble_link.h"
#include "power.h"
#include "frames_store.h"
#include "screen.h"
#include <esp_ota_ops.h>

#include "version.h"

extern Adafruit_SSD1306 display;

// Ikon carousel 32x32 (1 = piksel nyala, baris demi baris, MSB = kiri)
static const uint8_t PROGMEM ICON_MENU_GAME[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xE0, 0x0C, 0x00, 0x00, 0x30, 0x18, 0x00, 0x0C, 0x18,
  0x30, 0x30, 0x1E, 0x0C, 0x30, 0x30, 0x1E, 0x0C, 0x30, 0x30, 0x0C, 0x0C, 0x31, 0xFE, 0x00, 0x0C,
  0x31, 0xFE, 0x01, 0x8C, 0x30, 0x30, 0x03, 0xCC, 0x30, 0x30, 0x03, 0xCC, 0x30, 0x30, 0x01, 0x8C,
  0x30, 0x00, 0x00, 0x0C, 0x18, 0x00, 0x00, 0x18, 0x0C, 0x00, 0x00, 0x30, 0x07, 0xFF, 0xFF, 0xE0,
  0x03, 0xFF, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_MENU_NAV[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00,
  0x00, 0x01, 0xC0, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x03, 0xE0, 0x00, 0x00, 0x03, 0xE0, 0x00,
  0x00, 0x07, 0xF0, 0x00, 0x00, 0x07, 0x70, 0x00, 0x00, 0x0F, 0x78, 0x00, 0x00, 0x0E, 0x38, 0x00,
  0x00, 0x0E, 0x38, 0x00, 0x00, 0x1C, 0x1C, 0x00, 0x00, 0x1C, 0x1C, 0x00, 0x00, 0x38, 0x0E, 0x00,
  0x00, 0x38, 0x0E, 0x00, 0x00, 0x78, 0x0F, 0x00, 0x00, 0x70, 0x07, 0x00, 0x00, 0xF0, 0x07, 0x80,
  0x00, 0xE0, 0x03, 0x80, 0x00, 0xE0, 0x83, 0x80, 0x01, 0xC3, 0xE1, 0xC0, 0x01, 0xC6, 0x31, 0xC0,
  0x03, 0x9C, 0x1C, 0xE0, 0x03, 0xB0, 0x06, 0xE0, 0x07, 0xC0, 0x01, 0xF0, 0x07, 0x00, 0x00, 0x70,
  0x0E, 0x00, 0x00, 0x38, 0x08, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_MENU_PHONE[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0xC0, 0x03, 0x00,
  0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00,
  0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00,
  0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00,
  0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00,
  0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00, 0x00, 0xC0, 0x03, 0x00,
  0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xC3, 0xC3, 0x00, 0x00, 0xC3, 0xC3, 0x00,
  0x00, 0xC0, 0x03, 0x00, 0x00, 0x7F, 0xFE, 0x00, 0x00, 0x3F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_MENU_ANIM[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x3F, 0xFF, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF, 0xFC, 0x30, 0x00, 0x00, 0x0C,
  0x33, 0x0C, 0x30, 0xCC, 0x33, 0x0C, 0x30, 0xCC, 0x30, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C,
  0x30, 0x08, 0x00, 0x0C, 0x30, 0x0F, 0x00, 0x0C, 0x30, 0x0F, 0xC0, 0x0C, 0x30, 0x0F, 0xF8, 0x0C,
  0x30, 0x0F, 0xE0, 0x0C, 0x30, 0x0F, 0x80, 0x0C, 0x30, 0x0E, 0x00, 0x0C, 0x30, 0x08, 0x00, 0x0C,
  0x30, 0x00, 0x00, 0x0C, 0x30, 0x00, 0x00, 0x0C, 0x33, 0x0C, 0x30, 0xCC, 0x33, 0x0C, 0x30, 0xCC,
  0x30, 0x00, 0x00, 0x0C, 0x3F, 0xFF, 0xFF, 0xFC, 0x3F, 0xFF, 0xFF, 0xFC, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t PROGMEM ICON_MENU_SETTINGS[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x0F, 0xE0, 0x00, 0x00, 0x0F, 0xE0, 0x00,
  0x0F, 0xCF, 0xE7, 0xE0, 0x0F, 0xCF, 0xE7, 0xE0, 0x0F, 0xDF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xE0,
  0x0F, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xE0, 0x01, 0xFF, 0xFF, 0x80, 0x03, 0xFC, 0x3F, 0xC0,
  0x03, 0xF8, 0x1F, 0xC0, 0x7F, 0xF0, 0x0F, 0xFE, 0x7F, 0xE0, 0x07, 0xFE, 0x7F, 0xE0, 0x07, 0xFE,
  0x7F, 0xE0, 0x07, 0xFE, 0x7F, 0xE0, 0x07, 0xFE, 0x7F, 0xF0, 0x0F, 0xFE, 0x03, 0xF8, 0x1F, 0xC0,
  0x03, 0xFC, 0x3F, 0xC0, 0x0F, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xE0, 0x0F, 0xFF, 0xFF, 0xE0,
  0x0F, 0xFF, 0xFF, 0xE0, 0x0F, 0xDF, 0xFF, 0xE0, 0x0F, 0xC7, 0xE7, 0xE0, 0x00, 0x07, 0xE0, 0x00,
  0x00, 0x07, 0xE0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x07, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const char *MENU_ITEMS[] = {"Animasi", "Game", "HP", "Navigasi", "Setting"};
static const uint8_t *const MENU_ICONS[] = {ICON_MENU_ANIM, ICON_MENU_GAME, ICON_MENU_PHONE, ICON_MENU_NAV,
                                            ICON_MENU_SETTINGS};
static const AppState MENU_TARGETS[] = {ST_ANIM, ST_GAME, ST_PHONE, ST_NAV, ST_SETTINGS};
static const int MENU_COUNT = 5;
static Carousel menuCar = {};

enum SettingItem {
  SI_WA, SI_BT, SI_BT_RESTART, SI_PHONE, SI_CLOCK, SI_SCREEN, SI_NIGHT, SI_CONTRAST, SI_OLED_I2C, SI_OLED_TEST, SI_BOOTANIM, SI_RESET_HI, SI_SLEEP_NOW, SI_FLASH_MODE, SI_OTA,
  SI_GUIDE, SI_UNPAIR, SI_INFO,
  SI_COUNT
};
static const char *SETTING_LABELS[SI_COUNT] = {
  "Notif pop-up", "Bluetooth", "Restart BT", "Mode HP", "Jam", "Layar mati", "Malam", "Kecerahan", "OLED I2C", "Uji OLED",
  "Animasi boot", "Reset hi-score", "Tidur sekarang", "Mode flash", "Update OTA",
  "Panduan iPhone", "Hapus pairing", "Info"
};
static const char *PHONE_OPTS[PHONE_MODE_COUNT] = {"Android", "iPhone"};

// Panduan pairing iPhone (maks 21 karakter/baris, sama dengan PANDUAN_IPHONE.md)
static const char *const GUIDE_LINES[] = {
  "1. Setting > Mode HP",
  "   pilih iPhone",
  "2. iPhone: Pengaturan",
  "   > Bluetooth >",
  "   cute_device",
  "   > Pasangkan",
  "3. Izinkan: Bagikan",
  "   Notifikasi Sistem",
  "4. Notifikasi >",
  "   WhatsApp >",
  "  Tampilkan Pratinjau",
  "   = Selalu",
  "5. Tak ada di daftar?",
  "   Buka nRF Connect/",
  "   LightBlue, Connect",
  "6. Ganti HP: Lupakan",
  "  Perangkat di iPhone",
  "   + Hapus pairing",
  "7. Hanya Android:",
  "   Navigasi & Cuaca"
};
static const int GUIDE_COUNT = sizeof(GUIDE_LINES) / sizeof(GUIDE_LINES[0]);
static const int GUIDE_ROWS = 5;
static const char *NOTIF_OPTS[NOTIF_MODE_COUNT] = {"OFF", "WhatsApp", "Semua"};
static const char *SCREEN_OPTS[SCREEN_OPT_COUNT] = {"15s", "30s", "60s", "OFF"};
static const char *CONTRAST_OPTS[CONTRAST_OPT_COUNT] = {"Rendah", "Sedang", "Tinggi"};
static const uint32_t RESET_CONFIRM_MS = 3000;

static Carousel settingCar = {};  // kartu Setting (sel = SettingItem)
static bool infoOpen = false;
// Layar Uji OLED: pola 0 = bingkai+silang, 1 = papan catur, 2 = animasi (kirim frame terus)
static bool oledTestOpen = false;
static int oledPattern = 0;
static bool oledOverlay = true;
static uint32_t oledOkDownAt = 0;
static int oledBoxX = 10, oledBoxY = 20, oledBoxDx = 3, oledBoxDy = 2;
static bool guideOpen = false;
static int guideTop = 0;
static bool otaRequested = false;
static uint32_t resetConfirmUntil = 0;
static uint32_t resetDoneUntil = 0;
static uint32_t unpairConfirmUntil = 0;
static uint32_t unpairDoneUntil = 0;
static bool unpairOk = false;
static uint32_t btRestartConfirmUntil = 0;
static uint32_t flashConfirmUntil = 0;
static uint32_t btRestartAt = 0;          // 0 = belum pernah; hasil tampil 4 dtk sesudahnya
static const uint32_t BT_RESTART_SHOW_MS = 4000, BT_RESTART_WAIT_MS = 1500;

// Editor jam manual: Jam, Menit, Tanggal, Bulan, Tahun
static bool clockOpen = false;
static int clockField = 0;
static int clockVal[5];
static const int CLOCK_MIN[5] = {0, 0, 1, 1, 2024}, CLOCK_MAX[5] = {23, 59, 31, 12, 2099};
static uint32_t clockHoldStart = 0, clockLastRepeat = 0;
static bool clockFailed = false;
// Isian 0 editor jam = "Sinkron HP"; isian 1..5 = clockVal[0..4]
enum SyncState : uint8_t { SYNC_IDLE, SYNC_ASKING, SYNC_OK, SYNC_FAIL, SYNC_NOCONN, SYNC_NOTIME };
static SyncState syncState = SYNC_IDLE;
static uint32_t syncAt = 0, syncBefore = 0;
static const uint32_t SYNC_WAIT_MS = 10000;

// Editor Malam: Aktif, Mulai jam, Mulai menit, Selesai jam, Selesai menit
static bool nightOpen = false;
static int nightField = 0;
static int nightVal[5];  // [0] 0/1, [1] jam, [2] menit, [3] jam, [4] menit
static bool nightFailed = false;

// Font bawaan GFX hanya ASCII: buang byte UTF-8 (emoji, aksen, dll.)
String asciiOnly(const char *s) {
  String out;
  for (; *s; s++) {
    unsigned char c = *s;
    if (c == '\n') out += ' ';
    else if (c >= 0x20 && c < 0x7F) out += (char)c;
  }
  return out;
}

// Word-wrap sederhana untuk font 6x8 (textSize 1)
void drawWrapped(int x, int y, int cols, int maxLines, const String &text) {
  int line = 0;
  int pos = 0;
  int len = text.length();
  while (pos < len && line < maxLines) {
    while (pos < len && text[pos] == ' ') pos++;
    int end = min(pos + cols, len);
    if (end < len) {
      int sp = text.lastIndexOf(' ', end);
      if (sp > pos) end = sp;
    }
    display.setCursor(x, y + line * 9);
    display.print(text.substring(pos, end));
    pos = end;
    line++;
  }
}

// ---------- Marquee ----------
static const int MARQUEE_GAP = 30;  // jarak antar salinan teks yang berjalan
static const uint32_t MARQUEE_PAUSE_MS = 1000, MARQUEE_STEP_MS = 50;
static const int MARQUEE_ASCENT = 18, MARQUEE_H = 24;  // ruang di atas baseline & tinggi kanvas (font GFX)
// GFX tidak punya clipping: teks digambar dulu ke kanvas 1-bit (sekali alokasi), lalu disalin ke layar
static GFXcanvas1 marqueeCanvas(128, MARQUEE_H);
static uint32_t marqueeStart = 0;
static uint32_t marqueeLastFrame = 0;
static bool marqueeScrolling = false;

void marquee_reset() { marqueeStart = millis(); }

bool marquee_frameDue() {
  if (!marqueeScrolling || !power_screenOn() || millis() - marqueeLastFrame < MARQUEE_STEP_MS) return false;
  marqueeScrolling = false;  // digambar ulang -> marquee_draw menyalakannya lagi bila masih berjalan
  return true;
}

void marquee_draw(int x, int y, int w, const char *text, uint16_t color, const GFXfont *font, bool animate) {
  GFXcanvas1 &c = marqueeCanvas;
  int16_t x1 = 0, y1;
  uint16_t tw, th;
  int baseY = font ? MARQUEE_ASCENT : 0, h = font ? MARQUEE_H : 8;
  c.setFont(font);
  c.setTextSize(1);
  c.setTextWrap(false);
  c.setTextColor(1);
  if (font) c.getTextBounds(text, 0, baseY, &x1, &y1, &tw, &th);
  else tw = strlen(text) * 6;
  w = min(w, 128);

  int offset = 0;
  bool scroll = animate && (int)tw > w;
  if (scroll) {
    uint32_t cycle = MARQUEE_PAUSE_MS + (tw + MARQUEE_GAP) * MARQUEE_STEP_MS;
    uint32_t t = (millis() - marqueeStart) % cycle;
    offset = t < MARQUEE_PAUSE_MS ? 0 : (t - MARQUEE_PAUSE_MS) / MARQUEE_STEP_MS;
    marqueeScrolling = true;
    marqueeLastFrame = millis();
  }
  c.fillScreen(0);
  c.setCursor(-x1 - offset, baseY);
  c.print(text);
  if (scroll) {
    c.setCursor(-x1 - offset + tw + MARQUEE_GAP, baseY);
    c.print(text);
  }
  if (w < 128) c.fillRect(w, 0, 128 - w, h, 0);
  display.drawBitmap(x, y - baseY, c.getBuffer(), 128, h, color);
}

// ---------- Carousel ----------
static const int CAR_ICON_X = 48, CAR_ICON_Y = 13, CAR_ICON = 32;
static const int CAR_NAME_Y = 47, CAR_DOT_Y = 59, CAR_DOT_GAP = 8;
static const int CAR_SLIDE = 80;                        // jarak geser ikon (px)
static const int CAR_FRAMES = 6;
static const uint32_t CAR_ANIM_MS = 120;

static int carouselFrame(const Carousel &c) {
  uint32_t t = millis() - c.animStart;
  return t >= CAR_ANIM_MS ? CAR_FRAMES : (int)(t * CAR_FRAMES / CAR_ANIM_MS);
}

bool carousel_input(Carousel &c, int count) {
  Btn prevBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP;
  Btn nextBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  int dir = btn_pressed(prevBtn) ? -1 : btn_pressed(nextBtn) ? 1 : 0;
  if (!dir || count < 2) return false;
  // Ditekan saat animasi berjalan: langsung lanjut dari item sekarang
  c.from = c.sel;
  c.sel = (c.sel + dir + count) % count;
  c.dir = dir;
  c.animStart = millis();
  c.shown = -1;
  return true;
}

bool carousel_frameDue(Carousel &c) { return power_screenOn() && carouselFrame(c) != c.shown; }

void carousel_draw(Carousel &c, const String &title, const uint8_t *const *icons, int count, const char *name) {
  display.clearDisplay();
  drawHeader(title);
  int f = power_screenOn() ? carouselFrame(c) : CAR_FRAMES;
  c.shown = f;
  if (f < CAR_FRAMES) {
    // Ikon lama keluar ke arah berlawanan, ikon baru masuk dari arah tombol
    int shift = CAR_SLIDE * f / CAR_FRAMES;
    display.drawBitmap(CAR_ICON_X - c.dir * shift, CAR_ICON_Y, icons[c.from], CAR_ICON, CAR_ICON, SSD1306_WHITE);
    display.drawBitmap(CAR_ICON_X + c.dir * (CAR_SLIDE - shift), CAR_ICON_Y, icons[c.sel], CAR_ICON, CAR_ICON,
                       SSD1306_WHITE);
    display.fillRect(0, CAR_ICON_Y, 8, CAR_ICON, SSD1306_BLACK);  // ruang panah tetap bersih
    display.fillRect(120, CAR_ICON_Y, 8, CAR_ICON, SSD1306_BLACK);
  } else {
    display.drawBitmap(CAR_ICON_X, CAR_ICON_Y, icons[c.sel], CAR_ICON, CAR_ICON, SSD1306_WHITE);
  }
  int midY = CAR_ICON_Y + CAR_ICON / 2;
  display.fillTriangle(2, midY, 5, midY - 3, 5, midY + 3, SSD1306_WHITE);
  display.fillTriangle(125, midY, 122, midY - 3, 122, midY + 3, SSD1306_WHITE);

  int nameW = min((int)strlen(name) * 6, 128);
  marquee_draw((128 - nameW) / 2, CAR_NAME_Y, nameW, name);

  int x0 = 64 - (count - 1) * CAR_DOT_GAP / 2;
  for (int i = 0; i < count; i++) {
    if (i == c.sel) display.fillCircle(x0 + i * CAR_DOT_GAP, CAR_DOT_Y, 2, SSD1306_WHITE);
    else display.drawCircle(x0 + i * CAR_DOT_GAP, CAR_DOT_Y, 2, SSD1306_WHITE);
  }
}

// ---------- Kartu geser ----------
static const int CARD_RULE_Y = 54, CARD_FOOT_Y = 56;

void card_draw(Carousel &c, const String &title, int count, void (*content)(int index, int dx), const char *hint) {
  display.clearDisplay();
  drawHeader(title);
  int f = power_screenOn() ? carouselFrame(c) : CAR_FRAMES;
  c.shown = f;
  if (count > 0) {
    display.setTextWrap(false);  // kartu yang sedang bergeser keluar layar tidak boleh membungkus ke bawah
    if (f < CAR_FRAMES && c.from < count) {
      // Kartu lama keluar utuh, kartu baru masuk dari arah tombol (selebar layar)
      int shift = 128 * f / CAR_FRAMES;
      content(c.from, -c.dir * shift);
      content(c.sel, c.dir * (128 - shift));
    } else {
      content(c.sel, 0);
    }
    display.setTextWrap(true);
    display.fillRect(0, CARD_RULE_Y - 1, 128, 64 - CARD_RULE_Y + 1, SSD1306_BLACK);  // isi tidak menimpa kaki
    char pos[8];
    snprintf(pos, sizeof(pos), "%d/%d", c.sel + 1, count);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.drawFastHLine(0, CARD_RULE_Y, 128, SSD1306_WHITE);
    display.setCursor(0, CARD_FOOT_Y);
    display.print(hint);
    display.setCursor(128 - strlen(pos) * 6, CARD_FOOT_Y);
    display.print(pos);
  }
}

int drawWrappedAscii(int x, int y, int cols, int maxLines, const char *text, int firstLine) {
  // Salin ke buffer ASCII di stack: '\n' jadi spasi, byte UTF-8 dibuang
  char buf[NOTIF_MSG_LEN];
  int len = 0;
  for (const char *s = text; *s && len < (int)sizeof(buf) - 1; s++) {
    unsigned char ch = *s;
    if (ch == '\n') buf[len++] = ' ';
    else if (ch >= 0x20 && ch < 0x7F) buf[len++] = ch;
  }
  buf[len] = '\0';
  int pos = 0, line = 0;
  while (pos < len) {
    while (pos < len && buf[pos] == ' ') pos++;
    if (pos >= len) break;
    int end = min(pos + cols, len);
    if (end < len && buf[end] != ' ') {  // jangan memotong di tengah kata (kecuali kata > cols)
      int sp = end;
      while (sp > pos && buf[sp] != ' ') sp--;
      if (sp > pos) end = sp;
    }
    int row = line - firstLine;
    if (row >= 0 && row < maxLines) {
      display.setCursor(x, y + row * 9);
      for (int i = pos; i < end; i++) display.write(buf[i]);
    }
    pos = end;
    line++;
  }
  return line;
}

void printRight(int rightX, int y, const String &s) {
  display.setCursor(rightX - s.length() * 6, y);
  display.print(s);
}

// Header: jam kiri, teks tengah, status BT + baterai HP kanan, garis di y=10
uint32_t sig_mix(uint32_t h, uint32_t v) { return (h ^ v) * 16777619u; }

uint32_t sig_str(uint32_t h, const char *s) {
  while (*s) h = sig_mix(h, (uint8_t)*s++);
  return h;
}

uint32_t header_signature() {
  char clock[6] = "";
  ble_timeStr(clock, sizeof(clock));
  uint32_t h = sig_str(2166136261u, clock);
  h = sig_mix(h, ble_running());
  h = sig_mix(h, ble_connected());
  return sig_mix(h, ble_phoneBattery());
}

bool redraw_needed(RedrawWatch &w, bool redraw, uint32_t (*sigFn)(), uint32_t periodMs) {
  uint32_t now = millis();
  if (!redraw && now - w.lastCheck < periodMs) return false;
  w.lastCheck = now;
  uint32_t sig = sigFn();
  if (!redraw && sig == w.shown) return false;
  w.shown = sig;
  return true;
}

String header_status() {
  String status;
  if (ble_connected()) {
    status = "BT";
    uint8_t batt = ble_phoneBattery();
    if (batt) status += " " + String(batt) + "%";
  } else if (ble_running()) {
    status = ble_isIphone() ? "iOS" : "bt";
  }
  return status;
}

void drawHeader(const String &center) {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  char clock[6];
  display.setCursor(0, 0);
  display.print(ble_timeStr(clock, sizeof(clock)) ? clock : "--:--");

  display.setCursor(64 - center.length() * 3, 0);
  display.print(center);

  printRight(128, 0, header_status());
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
}

// Baris pilihan setinggi h px; teks hitam di atas bar putih bila terpilih
void beginRow(int y, int width, int h, bool selected) {
  if (selected) {
    display.fillRect(0, y, width, h, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
}

AppState menu_update(bool redraw) {
  static RedrawWatch watch;
  if (carousel_input(menuCar, MENU_COUNT)) { marquee_reset(); redraw = true; }
  if (btn_pressed(BTN_BACK)) return ST_HOME;
  if (btn_pressed(BTN_OK)) return MENU_TARGETS[menuCar.sel];
  if (carousel_frameDue(menuCar) || marquee_frameDue()) redraw = true;

  if (!redraw_needed(watch, redraw, header_signature)) return ST_MENU;

  carousel_draw(menuCar, "MENU", MENU_ICONS, MENU_COUNT, MENU_ITEMS[menuCar.sel]);
  oled_flush();
  return ST_MENU;
}

static String settingValue(int item) {
  switch (item) {
    case SI_WA:       return NOTIF_OPTS[settings_notifMode()];
    case SI_BT:       return settings_bt() ? "ON" : "OFF";
    case SI_PHONE:    return PHONE_OPTS[settings_phoneMode()];
    case SI_CLOCK:    return clock_sourceName();
    case SI_BT_RESTART:
      if (btRestartAt && millis() - btRestartAt < BT_RESTART_SHOW_MS) {
        if (!settings_bt()) return "BT OFF";
        if (millis() - btRestartAt < BT_RESTART_WAIT_MS) return "Restart..";
        return ble_running() ? "Ok" : "Gagal";
      }
      if (millis() < btRestartConfirmUntil) return "OK lagi?";
      return ">";
    case SI_UNPAIR:
      if (settings_phoneMode() != PHONE_IPHONE) return "-";
      if (millis() < unpairDoneUntil) return unpairOk ? "Ok" : "Gagal";
      if (millis() < unpairConfirmUntil) return "OK?";
      return ">";
    case SI_SCREEN:   return SCREEN_OPTS[settings_screenIdx()];
    case SI_NIGHT: {
      char buf[12];
      settings_nightLabel(buf, sizeof(buf));
      return buf;
    }
    case SI_CONTRAST: return CONTRAST_OPTS[settings_contrastIdx()];
    case SI_OLED_I2C: {
      // Pilihan; bila OLED tidak menjawab di clock itu, tampil clock yang dipakai: "800>400k"
      uint32_t want = settings_i2cHz() / 1000, got = oled_clockHz() / 1000;
      char buf[12];
      if (want == got) snprintf(buf, sizeof(buf), "%u kHz", (unsigned)want);
      else snprintf(buf, sizeof(buf), "%u>%uk", (unsigned)want, (unsigned)got);
      return buf;
    }
    case SI_OLED_TEST: return ">";
    case SI_BOOTANIM: return settings_bootAnim() ? "ON" : "OFF";
    case SI_RESET_HI:
      if (millis() < resetDoneUntil) return "Reset!";
      if (millis() < resetConfirmUntil) return "OK lagi?";
      return String(GAME_COUNT) + " game";
    case SI_FLASH_MODE: return millis() < flashConfirmUntil ? "OK lagi?" : ">";
    case SI_OTA:
    case SI_GUIDE:
    case SI_INFO:     return ">";
    default:          return "";
  }
}

static void activateSetting(int item) {
  switch (item) {
    case SI_WA:       settings_setNotifMode((NotifMode)(settings_notifMode() + 1)); break;
    case SI_BT:       settings_setBt(!settings_bt()); break;
    case SI_PHONE:
      // ble_update() menghentikan BLE lama & memulai mode baru secara otomatis
      settings_setPhoneMode((PhoneMode)(settings_phoneMode() + 1));
      if (settings_phoneMode() == PHONE_IPHONE) { guideOpen = true; guideTop = 0; }
      break;
    case SI_GUIDE:    guideOpen = true; guideTop = 0; break;
    case SI_BT_RESTART:
      if (millis() < btRestartConfirmUntil) {
        btRestartConfirmUntil = 0;
        btRestartAt = millis();
        ble_restartRadio();
      } else {
        btRestartConfirmUntil = millis() + RESET_CONFIRM_MS;
      }
      break;
    case SI_CLOCK: {
      // Mulai dari jam sekarang (atau 1 Jan 2025 00:00 bila belum ada jam)
      time_t t = ble_epoch();
      if (t >= 1704067200) {
        struct tm tmv;
        gmtime_r(&t, &tmv);
        clockVal[0] = tmv.tm_hour; clockVal[1] = tmv.tm_min; clockVal[2] = tmv.tm_mday;
        clockVal[3] = tmv.tm_mon + 1; clockVal[4] = constrain(tmv.tm_year + 1900, 2024, 2099);
      } else {
        clockVal[0] = 0; clockVal[1] = 0; clockVal[2] = 1; clockVal[3] = 1; clockVal[4] = 2025;
      }
      clockField = 0;
      clockFailed = false;
      syncState = SYNC_IDLE;
      clockOpen = true;
      break;
    }
    case SI_UNPAIR:
      if (settings_phoneMode() != PHONE_IPHONE) break;
      if (millis() < unpairConfirmUntil) {
        unpairOk = ble_deletePairings();
        unpairConfirmUntil = 0;
        unpairDoneUntil = millis() + 1500;
      } else {
        unpairConfirmUntil = millis() + RESET_CONFIRM_MS;
      }
      break;
    case SI_SCREEN:   settings_setScreenIdx(settings_screenIdx() + 1); break;
    case SI_NIGHT:
      nightVal[0] = settings_nightOn();
      nightVal[1] = settings_nightStartMin() / 60; nightVal[2] = settings_nightStartMin() % 60 / 5 * 5;
      nightVal[3] = settings_nightEndMin() / 60;   nightVal[4] = settings_nightEndMin() % 60 / 5 * 5;
      nightField = 0;
      nightFailed = false;
      nightOpen = true;
      break;
    case SI_CONTRAST:
      settings_setContrastIdx(settings_contrastIdx() + 1);
      power_applyContrast();
      break;
    case SI_OLED_I2C:
      settings_setI2cIdx(settings_i2cIdx() + 1);
      oled_applyClock(settings_i2cHz());  // langsung berlaku (di antara flush, loop yang sama)
      break;
    case SI_OLED_TEST:
      oledTestOpen = true;
      oledPattern = 0;
      oledOverlay = true;
      oledOkDownAt = 0;
      break;
    case SI_BOOTANIM: settings_setBootAnim(!settings_bootAnim()); break;
    case SI_RESET_HI:
      if (millis() < resetConfirmUntil) {
        settings_resetHiscores();
        resetConfirmUntil = 0;
        resetDoneUntil = millis() + 1500;
      } else {
        resetConfirmUntil = millis() + RESET_CONFIRM_MS;
      }
      break;
    case SI_SLEEP_NOW: power_requestSleep(1000); break;
    case SI_FLASH_MODE:
      if (millis() < flashConfirmUntil) {
        // Reboot ke ROM download mode (sama dengan menahan BOOT): flash lewat USB tanpa tombol.
        // Keluar dari download mode: flash/esptool hard reset.
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(34, 20);
        display.print("Mode flash");
        display.setCursor(25, 36);
        display.print("flash via USB");
        oled_flush();
        Serial.println("[SETTING] reboot ke mode download (flash USB)");
        clock_save("flash");
        ble_shutdown();
        Serial.flush();
        delay(1000);
        display.ssd1306_command(SSD1306_DISPLAYOFF);  // layar gelap = sudah bukan aplikasi
        REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
        // esp_restart() di C3 hanya reset CPU dan ROM mengabaikan bit di atas; reset sistem membacanya
        software_reset();
      }
      flashConfirmUntil = millis() + RESET_CONFIRM_MS;
      break;
    case SI_OTA:       otaRequested = true; break;
    case SI_INFO:      infoOpen = true; break;
  }
}

static void drawInfo() {
  display.clearDisplay();
  drawHeader("INFO");
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 12);
  display.print("FW " FW_VERSION "  App ");
  display.print(esp_ota_get_running_partition()->label);
  display.setCursor(0, 21);
  String addr = ble_address();
  display.print("BT ");
  display.print(addr.length() ? addr : String("(mati)"));
  display.setCursor(0, 30);
  display.print("Reset: ");
  display.print(power_resetReason());
  display.setCursor(0, 39);
  if (frames_valid()) {
    display.printf("Anim: %u frm %ufps %s", frames_count(), frames_fps(), frames_portrait() ? "P" : "L");
  } else {
    display.print("Anim: kosong");
  }
  marquee_draw(0, 48, 128, ble_lastNotifInfo().c_str());
  display.setCursor(0, 57);
  // Heap bebas/minimum + clock & error I2C OLED (maks 21 karakter)
  display.printf("H%u/%uk I2C%u e%u", ESP.getFreeHeap() / 1024, ESP.getMinFreeHeap() / 1024,
                 (unsigned)(oled_clockHz() / 1000), (unsigned)min(oled_errors(), (uint32_t)999));
  oled_flush();
}

static uint32_t infoSignature() {
  uint32_t h = sig_mix(header_signature(), ble_address().length());
  h = sig_str(h, ble_lastNotifInfo().c_str());
  h = sig_mix(h, ESP.getFreeHeap() / 1024);
  return sig_mix(h, ESP.getMinFreeHeap() / 1024);
}

// Setting: teks sementara "OK lagi?" / "Reset!" ikut menentukan tanda
static uint32_t settingsSignature() {
  uint32_t h = sig_mix(header_signature(), millis() < resetConfirmUntil);
  h = sig_mix(h, millis() < btRestartConfirmUntil);
  h = sig_mix(h, millis() < flashConfirmUntil);
  h = sig_str(h, settingValue(SI_BT_RESTART).c_str());
  h = sig_mix(h, clock_source());
  h = sig_mix(h, millis() < unpairConfirmUntil);
  h = sig_mix(h, millis() < unpairDoneUntil);
  return sig_mix(h, millis() < resetDoneUntil);
}

static void drawGuide() {
  display.clearDisplay();
  drawHeader("PANDUAN iPHONE");
  display.setTextColor(SSD1306_WHITE);
  for (int i = 0; i < GUIDE_ROWS && guideTop + i < GUIDE_COUNT; i++) {
    display.setCursor(0, 13 + i * 10);
    display.print(GUIDE_LINES[guideTop + i]);
  }
  // Penanda scroll di kanan
  if (guideTop > 0) display.fillTriangle(124, 13, 127, 13, 125, 11, SSD1306_WHITE);
  if (guideTop + GUIDE_ROWS < GUIDE_COUNT) display.fillTriangle(124, 60, 127, 60, 125, 62, SSD1306_WHITE);
  oled_flush();
}

static void drawSleepMessage() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(22, 20);
  display.print("Tidur sekarang");
  display.setCursor(4, 36);
  display.print("Tekan UP/DOWN untuk");
  display.setCursor(46, 46);
  display.print("bangun");
  oled_flush();
}

// Isian editor; aktif = dibalik warnanya
static void printValue(int x, int y, int size, const char *text, bool active) {
  int w = strlen(text) * 6 * size;
  if (active) {
    display.fillRect(x - 1, y - 1, w + 1, 8 * size + 1, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.setTextColor(SSD1306_WHITE);
  }
  display.setTextSize(size);
  display.setCursor(x, y);
  display.print(text);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
}

static void printField(int x, int y, int size, int idx, const char *fmt) {
  char buf[6];
  snprintf(buf, sizeof(buf), fmt, clockVal[idx]);
  printValue(x, y, size, buf, clockField == idx + 1);
}

static void loadClockFromSystem() {
  time_t t = ble_epoch();
  if (t < 1704067200) return;
  struct tm tmv;
  gmtime_r(&t, &tmv);
  clockVal[0] = tmv.tm_hour; clockVal[1] = tmv.tm_min; clockVal[2] = tmv.tm_mday;
  clockVal[3] = tmv.tm_mon + 1; clockVal[4] = constrain(tmv.tm_year + 1900, 2024, 2099);
}

// Editor jam: baris "Sync HP" (isian 0), sumber + sinkron terakhir, HH:MM besar, tanggal
static void drawClockEditor() {
  display.clearDisplay();
  drawHeader("SET JAM");
  static const char *const SYNC_TEXT[] = {"OK = minta", "Meminta...", "Tersinkron", "Gagal", "Tdk sambung", "Tanpa jam"};
  char buf[24];
  snprintf(buf, sizeof(buf), "Sync HP: %s", SYNC_TEXT[syncState]);
  printValue(0, 13, 1, buf, clockField == 0);
  display.setCursor(0, 22);
  uint32_t ls = clock_lastSync();
  if (ls) {
    time_t t = ls;
    struct tm tmv;
    gmtime_r(&t, &tmv);
    display.printf("%s %02d/%02d %02d:%02d", clock_sourceName(), tmv.tm_mday, tmv.tm_mon + 1, tmv.tm_hour, tmv.tm_min);
  } else {
    display.printf("%s, blm sync", clock_sourceName());
  }
  // HH:MM ukuran 2 di tengah
  printField(34, 32, 2, 0, "%02d");
  display.setTextSize(2);
  display.setCursor(58, 32);
  display.print(":");
  display.setTextSize(1);
  printField(70, 32, 2, 1, "%02d");
  // DD/MM/YYYY (60 px) di tengah
  printField(34, 48, 1, 2, "%02d");
  display.setCursor(46, 48); display.print("/");
  printField(52, 48, 1, 3, "%02d");
  display.setCursor(64, 48); display.print("/");
  printField(70, 48, 1, 4, "%04d");
  display.setCursor(0, 57);
  display.print(clockFailed ? "Tanggal tdk valid"
                : clockField == 0 ? "OK:sync  >:manual"
                : clockField < 5  ? "<>ubah OK:lanjut" : "<>ubah OK:simpan");
  oled_flush();
}

static void clockStep(int dir) {
  int i = clockField - 1;
  int &v = clockVal[i];
  int span = CLOCK_MAX[i] - CLOCK_MIN[i] + 1;
  v = CLOCK_MIN[i] + ((v - CLOCK_MIN[i] + dir) % span + span) % span;
  clockFailed = false;
}

// Tahan kiri/kanan: ulang otomatis setelah 400 ms, tiap 100 ms. Hasil: -1/0/+1
static int heldStep(Btn leftBtn, Btn rightBtn, bool l, bool r, uint32_t now) {
  if (l || r) { clockHoldStart = clockLastRepeat = now; return r ? 1 : -1; }
  bool hl = btn_held(leftBtn), hr = btn_held(rightBtn);
  if ((hl ^ hr) && now - clockHoldStart >= 400 && now - clockLastRepeat >= 100) {
    clockLastRepeat = now;
    return hr ? 1 : -1;
  }
  return 0;
}

// true = editor masih terbuka
static bool clockEditorUpdate(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  uint32_t now = millis();
  if (btn_pressed(BTN_BACK)) {
    if (clockField > 0) clockField--;  // BACK: isian sebelumnya; di isian pertama = batal
    else { clockOpen = false; Serial.println("[JAM] setel manual dibatalkan"); return false; }
    redraw = true;
  }
  bool l = btn_pressed(leftBtn), r = btn_pressed(rightBtn);
  int step = heldStep(leftBtn, rightBtn, l, r, now);
  if (step) {
    if (clockField == 0) { if (step > 0) clockField = 1; }  // di baris Sync: kanan = lanjut ke atur manual
    else clockStep(step);
    redraw = true;
  }
  if (btn_pressed(BTN_OK)) {
    if (clockField == 0) {
      syncBefore = clock_lastSync();
      syncAt = now;
      syncState = ble_requestTimeSync() ? SYNC_ASKING : SYNC_NOCONN;
    } else if (clockField < 5) {
      clockField++;
    } else if (clock_setManual(clockVal[0], clockVal[1], clockVal[2], clockVal[3], clockVal[4])) {
      clockOpen = false;
      return false;
    } else {
      clockFailed = true;
      clockField = 3;  // tanggal melebihi jumlah hari dalam bulan
    }
    redraw = true;
  }
  if (syncState == SYNC_ASKING) {
    bool withTime = false;
    if (ble_takeSyncDone(withTime) && !(clock_lastSync() != syncBefore && clock_source() == CLK_SYNC)) {
      // Chronos menjawab "sinkron selesai" tanpa paket jam baru: jam sudah sinkron sejak tersambung
      // (sumber Sinkron) -> berhasil; bila sumber lain, HP memang tidak mengirim jam
      Serial.printf("[JAM] sinkron HP selesai, paket jam %s\n", withTime ? "ada" : "tidak dikirim");
      syncState = clock_source() == CLK_SYNC ? SYNC_OK : SYNC_NOTIME;
      redraw = true;
    } else if (clock_lastSync() != syncBefore && clock_source() == CLK_SYNC) {
      syncState = SYNC_OK;
      loadClockFromSystem();
      redraw = true;
    } else if (now - syncAt >= SYNC_WAIT_MS) {
      syncState = SYNC_FAIL;
      Serial.println("[JAM] sinkron HP: tidak ada balasan 10 dtk");
      redraw = true;
    }
  }
  static uint32_t lastDraw = 0;
  if (now - lastDraw >= 1000) redraw = true;  // jam di header tetap berjalan
  if (redraw) { lastDraw = now; drawClockEditor(); }
  return true;
}

// Editor Malam: Aktif ON/OFF, Mulai HH:MM, Selesai HH:MM (menit per 5)
static void drawNightEditor() {
  display.clearDisplay();
  drawHeader("MALAM");
  char buf[4];
  display.setCursor(0, 14);
  display.print("Aktif:");
  printValue(42, 14, 1, nightVal[0] ? "ON" : "OFF", nightField == 0);
  const char *labels[2] = {"Mulai", "Selesai"};
  for (int k = 0; k < 2; k++) {
    int y = 25 + k * 16;
    display.setCursor(0, y + 4);
    display.print(labels[k]);
    snprintf(buf, sizeof(buf), "%02d", nightVal[1 + k * 2]);
    printValue(56, y, 2, buf, nightField == 1 + k * 2);
    display.setTextSize(2);
    display.setCursor(80, y);
    display.print(":");
    display.setTextSize(1);
    snprintf(buf, sizeof(buf), "%02d", nightVal[2 + k * 2]);
    printValue(92, y, 2, buf, nightField == 2 + k * 2);
  }
  display.setCursor(0, 57);
  display.print(nightFailed ? "Mulai = selesai!" : "Tidur total: BT mati");
  oled_flush();
}

static bool nightEditorUpdate(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  uint32_t now = millis();
  if (btn_pressed(BTN_BACK)) {
    if (nightField > 0) nightField--;
    else { nightOpen = false; return false; }
    redraw = true;
  }
  bool l = btn_pressed(leftBtn), r = btn_pressed(rightBtn);
  int step = heldStep(leftBtn, rightBtn, l, r, now);
  if (step) {
    int &v = nightVal[nightField];
    if (nightField == 0) v = !v;
    else if (nightField % 2) v = (v + step + 24) % 24;        // jam
    else v = (v + step * 5 + 60) % 60;                         // menit per 5
    nightFailed = false;
    redraw = true;
  }
  if (btn_pressed(BTN_OK)) {
    bool last = nightField == 4 || (nightField == 0 && !nightVal[0]);  // OFF: langsung simpan
    if (!last) nightField++;
    else if (settings_setNight(nightVal[0], nightVal[1] * 60 + nightVal[2], nightVal[3] * 60 + nightVal[4])) {
      nightOpen = false;
      return false;
    } else {
      nightFailed = true;
      nightField = 3;
    }
    redraw = true;
  }
  static uint32_t lastDraw = 0;
  if (now - lastDraw >= 1000) redraw = true;
  if (redraw) { lastDraw = now; drawNightEditor(); }
  return true;
}

// ---- Uji OLED ----
static void drawOledTest() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  if (oledPattern == 0) {  // bingkai 1 px + silang + teks pojok: pergeseran langsung terlihat
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.drawLine(0, 0, 127, 63, SSD1306_WHITE);
    display.drawLine(127, 0, 0, 63, SSD1306_WHITE);
    display.setCursor(2, 2); display.print("KI");
    display.setCursor(114, 55); display.print("KA");
  } else if (oledPattern == 1) {  // papan catur 8x8
    for (int y = 0; y < 64; y += 8)
      for (int x = 0; x < 128; x += 8)
        if (((x + y) / 8) % 2 == 0) display.fillRect(x, y, 8, 8, SSD1306_WHITE);
  } else {  // kotak bergerak cepat + bingkai: beban seperti game
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    oledBoxX += oledBoxDx; oledBoxY += oledBoxDy;
    if (oledBoxX <= 1 || oledBoxX >= 114) oledBoxDx = -oledBoxDx;
    if (oledBoxY <= 1 || oledBoxY >= 50) oledBoxDy = -oledBoxDy;
    display.fillRect(oledBoxX, oledBoxY, 13, 13, SSD1306_WHITE);
  }
  if (oledOverlay) {
    char a[24], b[24];
    snprintf(a, sizeof(a), "%ukHz %u.%u/%u.%ums", (unsigned)(oled_clockHz() / 1000),
             (unsigned)(oled_lastUs() / 1000), (unsigned)(oled_lastUs() / 100 % 10),
             (unsigned)(oled_maxUs() / 1000), (unsigned)(oled_maxUs() / 100 % 10));
    snprintf(b, sizeof(b), "f%u e%u ri%u", (unsigned)(oled_frames() % 100000), (unsigned)oled_errors(),
             (unsigned)oled_reinits());
    display.fillRect(8, 20, 112, 21, SSD1306_BLACK);
    display.drawRect(8, 20, 112, 21, SSD1306_WHITE);
    display.setCursor(11, 22); display.print(a);
    display.setCursor(11, 31); display.print(b);
  }
  oled_flush();
}

// true = layar uji masih terbuka. Kiri/kanan pola, OK singkat overlay, tahan OK 1 dtk re-init, BACK keluar
static bool oledTestUpdate(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  uint32_t now = millis();
  if (btn_pressed(BTN_BACK)) { oledTestOpen = false; return false; }
  if (btn_pressed(leftBtn)) { oledPattern = (oledPattern + 2) % 3; redraw = true; }
  if (btn_pressed(rightBtn)) { oledPattern = (oledPattern + 1) % 3; redraw = true; }
  if (btn_pressed(BTN_OK)) oledOkDownAt = now;
  if (oledOkDownAt) {
    if (!btn_held(BTN_OK)) {  // dilepas < 1 dtk: tampil/sembunyikan overlay
      oledOverlay = !oledOverlay;
      oledOkDownAt = 0;
      redraw = true;
    } else if (now - oledOkDownAt >= 1000) {  // ditahan: re-init controller
      oledOkDownAt = 0;
      oled_reinit();
      power_applyContrast();
      redraw = true;
    }
  }
  static uint32_t lastDraw = 0;
  uint32_t period = oledPattern == 2 ? 30 : 250;  // animasi: kirim frame terus (seperti game)
  if (now - lastDraw >= period) redraw = true;
  if (redraw) { lastDraw = now; drawOledTest(); }
  return true;
}

// Isi satu kartu Setting: nama di atas, nilai sekarang besar di bawahnya
static void settingCard(int item, int dx) {
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  const char *label = SETTING_LABELS[item];
  display.setCursor(dx + (128 - (int)strlen(label) * 6) / 2, 16);
  display.print(label);
  String val = settingValue(item);
  int size = val.length() <= 10 ? 2 : 1;
  display.setTextSize(size);
  display.setCursor(dx + (128 - (int)val.length() * 6 * size) / 2, size == 2 ? 31 : 35);
  display.print(val);
  display.setTextSize(1);
}

AppState settings_screen_update(bool redraw) {
  static RedrawWatch watch, infoWatch;

  if (power_sleepPending()) {
    buttons_clear();
    if (redraw) drawSleepMessage();
    return ST_SETTINGS;
  }

  if (clockOpen) {
    if (clockEditorUpdate(redraw)) return ST_SETTINGS;
    redraw = true;
  }
  if (oledTestOpen) {
    if (oledTestUpdate(redraw)) return ST_SETTINGS;
    redraw = true;
  }
  if (nightOpen) {
    if (nightEditorUpdate(redraw)) return ST_SETTINGS;
    redraw = true;
  }

  if (guideOpen) {
    if (btn_pressed(BTN_BACK) || btn_pressed(BTN_OK)) { guideOpen = false; redraw = true; }
    else {
      bool moved = false;
      if (btn_pressed(BTN_UP) && guideTop > 0) { guideTop--; moved = true; }
      if (btn_pressed(BTN_DOWN) && guideTop + GUIDE_ROWS < GUIDE_COUNT) { guideTop++; moved = true; }
      buttons_clear();
      if (redraw || moved) drawGuide();
      return ST_SETTINGS;
    }
  }

  if (infoOpen) {
    if (btn_pressed(BTN_BACK)) { infoOpen = false; redraw = true; }
    else {
      buttons_clear();
      if (marquee_frameDue()) redraw = true;
      if (redraw_needed(infoWatch, redraw, infoSignature)) drawInfo();
      return ST_SETTINGS;
    }
  } else if (btn_pressed(BTN_BACK)) {
    resetConfirmUntil = unpairConfirmUntil = btRestartConfirmUntil = flashConfirmUntil = 0;
    return ST_MENU;
  }

  if (carousel_input(settingCar, SI_COUNT)) { resetConfirmUntil = btRestartConfirmUntil = flashConfirmUntil = 0; redraw = true; }
  if (carousel_frameDue(settingCar)) redraw = true;
  if (btn_pressed(BTN_OK)) {
    activateSetting(settingCar.sel);
    if (power_sleepPending()) { drawSleepMessage(); return ST_SETTINGS; }
    if (infoOpen) { marquee_reset(); redraw_needed(infoWatch, true, infoSignature); drawInfo(); return ST_SETTINGS; }
    if (guideOpen) { drawGuide(); return ST_SETTINGS; }
    if (clockOpen) { drawClockEditor(); return ST_SETTINGS; }
    if (oledTestOpen) { drawOledTest(); return ST_SETTINGS; }
    if (nightOpen) { drawNightEditor(); return ST_SETTINGS; }
    if (otaRequested) { otaRequested = false; return ST_OTA; }
    redraw = true;
  }

  // Teks konfirmasi aktif: cek lebih sering agar hilang tepat waktu
  bool tempText = millis() < resetConfirmUntil + 200 || millis() < resetDoneUntil + 200 ||
                  millis() < unpairConfirmUntil + 200 || millis() < unpairDoneUntil + 200 ||
                  millis() < btRestartConfirmUntil + 200 || millis() < flashConfirmUntil + 200 ||
                  (btRestartAt && millis() - btRestartAt < BT_RESTART_SHOW_MS + 200);
  if (!redraw_needed(watch, redraw, settingsSignature, tempText ? 100 : 1000)) return ST_SETTINGS;

  card_draw(settingCar, "SETTING", SI_COUNT, settingCard, "OK ubah");
  oled_flush();
  return ST_SETTINGS;
}

AppState nav_screen_update(bool redraw) {
  static RedrawWatch watch;
  if (btn_pressed(BTN_BACK)) return ST_MENU;
  btn_pressed(BTN_UP);
  btn_pressed(BTN_DOWN);
  btn_pressed(BTN_OK);

  // Gambar ulang saat ada data navigasi baru, atau bila jam/status koneksi berubah
  if (ble_navChanged()) redraw = true;
  if (marquee_frameDue()) redraw = true;
  if (!redraw_needed(watch, redraw, header_signature)) return ST_NAV;

  if (ble_isIphone()) {
    draw_androidOnly("NAV", "iPhone tidak mengirim", "data navigasi.");
    oled_flush();
    return ST_NAV;
  }

  display.clearDisplay();
  drawHeader("NAV");

  if (!ble_connected()) {
    display.setCursor(0, 16);
    display.print("Menunggu koneksi...");
    display.setCursor(0, 30);
    display.print("Buka app Chronos");
    display.setCursor(0, 42);
    display.print("Nama: " BLE_NAME);
  } else {
    Navigation &nav = ble_nav();
    if (!nav.active) {
      display.setCursor(0, 16);
      display.print("Terhubung.");
      drawWrapped(0, 30, 21, 3, "Mulai rute di Google Maps");
    } else {
      int textX = 0;
      int cols = 21;
      if (nav.hasIcon) {
        display.drawBitmap(0, 14, nav.icon, 48, 48, SSD1306_WHITE);
        textX = 52;
        cols = 12;
      }
      String title = asciiOnly(nav.title.c_str());
      if (title.length() <= (unsigned)(cols / 2)) {  // pendek: huruf besar
        display.setTextSize(2);
        display.setCursor(textX, 14);
        display.print(title);
        display.setTextSize(1);
      } else {
        marquee_draw(textX, 14, 128 - textX, title.c_str());
      }
      marquee_draw(textX, 32, 128 - textX, asciiOnly(nav.directions.c_str()).c_str());
      if (nav.eta.length()) printRight(128, 56, "ETA " + asciiOnly(nav.eta.c_str()).substring(0, cols - 4));
    }
  }
  oled_flush();
  return ST_NAV;
}

void draw_androidOnly(const char *title, const char *line1, const char *line2) {
  display.clearDisplay();
  drawHeader(title);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor((128 - 18 * 6) / 2, 16);
  display.print("Hanya untuk Android");
  display.drawFastHLine(10, 26, 108, SSD1306_WHITE);
  display.setCursor(0, 32);
  display.print(line1);
  display.setCursor(0, 42);
  display.print(line2);
  display.setCursor(0, 56);
  display.print("BACK: kembali");
}

void call_draw(const char *caller, bool blink, const char *hint) {
  screen_landscape();
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(37, 3);
  display.print("PANGGILAN");
  display.setTextColor(SSD1306_WHITE);
  // Ikon telepon berkedip
  if (blink) {
    display.fillRoundRect(6, 22, 6, 14, 2, SSD1306_WHITE);
    display.drawFastHLine(12, 22, 4, SSD1306_WHITE);
    display.drawFastHLine(12, 35, 4, SSD1306_WHITE);
  }
  String name = asciiOnly(caller);
  if (!name.length()) name = "Tidak dikenal";
  marquee_draw(22, 25, 104, name.c_str());
  display.setCursor((128 - (int)strlen(hint) * 6) / 2, 52);
  display.print(hint);
  oled_flush();
}

int popup_draw(const char *sender, const char *msg, int topLine) {
  screen_landscape();  // pop-up bisa muncul di tengah game potret
  display.clearDisplay();
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(SSD1306_BLACK);
  marquee_draw(3, 2, 122, asciiOnly(sender).c_str(), SSD1306_BLACK);
  display.setTextColor(SSD1306_WHITE);
  int lines = drawWrappedAscii(3, 14, 20, POPUP_LINES, msg, topLine);
  // Masih ada lanjutan di bawah: segitiga kecil di pojok kanan bawah (di bawah baris ke-5)
  if (topLine + POPUP_LINES < lines) display.fillTriangle(121, 59, 125, 59, 123, 61, SSD1306_WHITE);
  oled_flush();
  return lines;
}
