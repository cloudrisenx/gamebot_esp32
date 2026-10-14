#include "home.h"
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include "buttons.h"
#include "ble_link.h"
#include "power.h"
#include "phone.h"
#include "screen.h"
#include "clockstore.h"

extern Adafruit_SSD1306 display;

// Ikon 8x8 cuaca sesuai kode Chronos 0-7 (lihat ChronosESP32.h)
static const uint8_t PROGMEM ICON_SUN_CLOUD[] = {0x40, 0xE0, 0x58, 0x24, 0x42, 0x81, 0x7E, 0x00};
static const uint8_t PROGMEM ICON_SUN[] = {0x10, 0x44, 0x38, 0xBA, 0x38, 0x44, 0x10, 0x00};
static const uint8_t PROGMEM ICON_SNOW[] = {0x18, 0x24, 0x42, 0x7E, 0x00, 0x54, 0x28, 0x54};
static const uint8_t PROGMEM ICON_RAIN[] = {0x18, 0x24, 0x42, 0x7E, 0x00, 0x54, 0xA8, 0x00};
static const uint8_t PROGMEM ICON_CLOUD[] = {0x00, 0x18, 0x24, 0x42, 0x81, 0x81, 0x7E, 0x00};
static const uint8_t PROGMEM ICON_STORM[] = {0x18, 0x24, 0x7E, 0x08, 0x10, 0x3C, 0x08, 0x10};
static const uint8_t PROGMEM ICON_WIND[] = {0x00, 0xFC, 0x02, 0xFC, 0x00, 0xF8, 0x04, 0xF8};
static const uint8_t PROGMEM ICON_FOG[] = {0x00, 0xFF, 0x00, 0x7E, 0x00, 0xFF, 0x00, 0x7E};
static const uint8_t *const WEATHER_ICONS[] = {ICON_SUN_CLOUD, ICON_SUN, ICON_SNOW, ICON_RAIN,
                                               ICON_CLOUD, ICON_STORM, ICON_WIND, ICON_FOG};
static const char *const WEATHER_NAMES[] = {"Cerah brwn", "Cerah", "Salju", "Hujan",
                                            "Berawan", "Ekstrem", "Berangin", "Kabut"};
static const uint8_t PROGMEM ICON_MAIL[] = {0x00, 0xFF, 0xC3, 0xA5, 0x99, 0x81, 0xFF, 0x00};
static const uint8_t PROGMEM ICON_POWER[] = {0x10, 0x54, 0x92, 0x92, 0x82, 0x44, 0x38};  // 7x7, BACK = layar mati
static const uint8_t PROGMEM ICON_TURN[] = {0x04, 0x06, 0xFF, 0x86, 0x84, 0x80, 0x80, 0x00};

static const char *const DAYS[] = {"Min", "Sen", "Sel", "Rab", "Kam", "Jum", "Sab"};
static const char *const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "Mei", "Jun",
                                     "Jul", "Agu", "Sep", "Okt", "Nov", "Des"};

// Tata letak (y): baris atas 0-7, jam 13-37 (baseline 37), info 44-51, garis 54, legenda 56-63
static const int CLOCK_BASELINE = 37;
static const int INFO_Y = 44;
static const int RULE_Y = 54;
static const int LEGEND_Y = 56;

static bool colonOn() { return (millis() / 1000) % 2 == 0; }

static void drawClock(const char *hhmm) {
  display.setFont(&FreeSansBold18pt7b);
  display.setTextSize(1);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(hhmm, 0, CLOCK_BASELINE, &x1, &y1, &w, &h);
  display.setCursor((128 - (int)w) / 2 - x1, CLOCK_BASELINE);
  // Gambar per bagian; titik dua "dimatikan" dengan warna hitam agar posisi angka tidak bergeser
  display.setTextColor(SSD1306_WHITE);
  display.print(String(hhmm).substring(0, 2));
  display.setTextColor(colonOn() ? SSD1306_WHITE : SSD1306_BLACK);
  display.print(':');
  display.setTextColor(SSD1306_WHITE);
  display.print(hhmm + 3);
  display.setFont(nullptr);
}

// Baris info: teks panjang berjalan di area sebelum penghitung notifikasi di kanan (rightLimit)
static void drawInfoText(int x, const String &text, int rightLimit) {
  marquee_draw(x, INFO_Y, max(0, rightLimit - x), text.c_str());
}

static void draw() {
  display.clearDisplay();
  display.setFont(nullptr);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Baris atas: tanggal kiri, status BT/baterai HP kanan
  int wday, day, month;
  if (ble_date(wday, day, month)) {
    display.setCursor(0, 0);
    display.printf("%s, %d %s", DAYS[wday % 7], day, MONTHS[month % 12]);
  }
  printRight(128, 0, header_status());

  char hhmm[6];
  bool synced = ble_timeStr(hhmm, sizeof(hhmm));
  drawClock(synced ? hhmm : "--:--");
  // Jam perkiraan (dipulihkan dari NVS setelah mati): tanda "~" di kiri jam
  bool estimate = synced && clock_source() == CLK_ESTIMATE;
  if (estimate) {
    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print("~");
    display.setTextSize(1);
  }

  // Kanan baris info: amplop + jumlah notifikasi belum dibaca
  int rightLimit = 128;
  int unread = ble_unreadCount();
  if (unread > 0) {
    String n = unread > 99 ? "99+" : String(unread);
    int nx = 128 - n.length() * 6;
    display.drawBitmap(nx - 10, INFO_Y, ICON_MAIL, 8, 8, SSD1306_WHITE);
    display.setCursor(nx, INFO_Y);
    display.print(n);
    rightLimit = nx - 12;
  }

  // Kiri baris info: belum sinkron > navigasi aktif > cuaca
  if (!synced) {
    drawInfoText(0, "Sambungkan Chronos", rightLimit);
  } else if (ble_navActive()) {
    Navigation &nav = ble_nav();
    display.drawBitmap(0, INFO_Y, ICON_TURN, 8, 8, SSD1306_WHITE);
    drawInfoText(11, asciiOnly(nav.title.c_str()) + " " + asciiOnly(nav.directions.c_str()), rightLimit);
  } else if (ble_weatherCount() > 0) {
    Weather &w = ble_weatherAt(0);
    int icon = constrain(w.icon, 0, 7);
    display.drawBitmap(0, INFO_Y, WEATHER_ICONS[icon], 8, 8, SSD1306_WHITE);
    String text = String(w.temp) + "C " + WEATHER_NAMES[icon];
    if (ble_weatherIsCache()) {  // data lama dari cache: beri umur
      int age = ble_weatherCacheAgeH();
      text += age < 0 ? " (lama)" : " (" + String(age) + "j lalu)";
    }
    drawInfoText(11, text, rightLimit);
  } else if (estimate) {
    drawInfoText(0, "Jam perkiraan, sambungkan Chronos", rightLimit);
  }

  display.drawFastHLine(0, RULE_Y, 128, SSD1306_WHITE);
  // Legenda = urutan tombol fisik kiri -> kanan: UP, DOWN, OK, BACK (MENU_SWAP_LR menukar label UP/DOWN).
  // Lebar 36 + 36 + 24 + 7 = 103 px; sisa 25 px dibagi rata ke 3 celah (8, 8, 9): x 0, 44, 88, 121.
  const char *upLabel = MENU_SWAP_LR ? "<Anim" : "<Notif";
  const char *downLabel = MENU_SWAP_LR ? "Notif>" : "Anim>";
  display.setCursor(0, LEGEND_Y);
  display.print(upLabel);
  display.setCursor(44, LEGEND_Y);
  display.print(downLabel);
  display.setCursor(88, LEGEND_Y);
  display.print("Menu");
  display.drawBitmap(128 - 7, LEGEND_Y, ICON_POWER, 7, 7, SSD1306_WHITE);
  oled_flush();
}

// Tanda isi Beranda: kedip ":" (detik), jam/BT/baterai, tanggal, cuaca, navigasi, belum dibaca
static uint32_t homeSignature() {
  uint32_t h = sig_mix(header_signature(), colonOn());
  int wday = 0, day = 0, month = 0;
  h = sig_mix(h, ble_date(wday, day, month));
  h = sig_mix(h, day * 100 + month);
  h = sig_mix(h, ble_unreadCount());
  h = sig_mix(h, ble_weatherCount());
  h = sig_mix(h, clock_source());
  h = sig_mix(h, ble_weatherIsCache() ? ble_weatherCacheAgeH() + 1 : 0);
  if (ble_weatherCount() > 0) {
    h = sig_mix(h, ble_weatherAt(0).temp);
    h = sig_mix(h, ble_weatherAt(0).icon);
  }
  h = sig_mix(h, ble_navActive());
  if (ble_navActive()) {
    h = sig_str(h, ble_nav().title.c_str());
    h = sig_str(h, ble_nav().directions.c_str());
  }
  return h;
}

AppState home_update(bool redraw) {
  static RedrawWatch watch;
  if (btn_pressed(BTN_OK)) return ST_MENU;
  if (btn_pressed(BTN_UP)) { phone_requestStart(PHONE_START_NOTIF); return ST_PHONE; }
  if (btn_pressed(BTN_DOWN)) return ST_ANIM;  // musik tetap lewat HP > Musik
  if (btn_pressed(BTN_BACK)) power_screenOffNow();

  if (marquee_frameDue()) redraw = true;
  if (redraw_needed(watch, redraw, homeSignature, 250)) draw();
  return ST_HOME;
}
