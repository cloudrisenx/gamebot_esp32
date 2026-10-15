#include "text_screen.h"
#include "lang.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "power.h"
#include "screen.h"
#include "settings.h"
#include "ota.h"

extern Adafruit_SSD1306 display;

static const int CHAR_W = 18, TEXT_Y = 18;          // font bawaan ukuran 3: 18x24 px
// Posisi dihitung dari waktu (px/detik tetap), bukan langkah per frame: frame lambat tidak membuat tersendat/ngebut
static const uint16_t SPEED_PPS[3] = {33, 67, 133};  // = 1/2/4 px per 30 ms
static const uint32_t GAP_MS = 100;  // loop berhenti lebih lama (pop-up, menu, OTA, BLE): lanjut dari posisi terakhir

static char text[RTEXT_MAX + 1];
static bool loaded = false;
static int speedIdx = 1;
static bool paused = false;
static int scrollX = 128;                            // posisi kiri teks
static int baseX = 128;                              // scrollX pada waktu baseT
static uint32_t baseT = 0, lastFrame = 0;

static void rebase(int x) {
  scrollX = baseX = x;
  baseT = lastFrame = millis();
}

static void load() {
  if (loaded) return;
  loaded = true;
  snprintf(text, sizeof(text), T("Halo dari %s", "Hello from %s"), settings_deviceName().c_str());
  Preferences p;
  if (p.begin("rtext", true)) {
    String s = p.getString("t", "");
    p.end();
    if (s.length()) strncpy(text, s.c_str(), sizeof(text) - 1);
  }
  text[sizeof(text) - 1] = '\0';
}

const char *text_get() {
  load();
  return text;
}

bool text_set(const char *s, size_t n) {
  char buf[RTEXT_MAX + 1];
  size_t len = 0;
  for (size_t i = 0; i < n && len < RTEXT_MAX; i++) {
    unsigned char c = s[i];
    if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    if (c >= 0x20 && c < 0x7F) buf[len++] = c;  // font bawaan hanya ASCII
  }
  while (len && buf[len - 1] == ' ') len--;
  buf[len] = '\0';
  if (!len) return false;
  load();
  strcpy(text, buf);
  Preferences p;
  if (p.begin("rtext", false)) { p.putString("t", text); p.end(); }
  rebase(128);
  Serial.printf("[TEKS] disimpan %u karakter\n", (unsigned)len);
  return true;
}

static void draw() {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  display.setCursor(scrollX, TEXT_Y);
  display.print(text);  // huruf di luar layar dilewati oleh GFX (tidak ada biaya gambar)
  display.setTextSize(1);
  display.setTextWrap(true);
  oled_flush();
}

void text_enter() {
  load();
  ota_holdMenuReset();
  paused = false;
  rebase(128);
  screen_landscape();
  draw();
}

AppState text_update(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  // Tahan OK = menu WiFi (OTA latar: teks dari HP langsung berlaku lewat text_set); ketuk OK = jeda
  OtaMenu m = ota_holdMenu(T("WIFI UBAH TEKS", "WIFI EDIT TEXT"), T("Ubah teks dari HP:", "Edit text on phone:"),
                           T("tab Teks di web OTA", "Text tab of OTA page"));
  if (m == OM_OPEN) return ST_TEXT;
  if (m == OM_CLOSED) { rebase(scrollX); draw(); return ST_TEXT; }
  bool okTap = m == OM_TAP;

  if (btn_pressed(BTN_BACK)) {
    ota_bgStop();  // keluar dari Teks = WiFi OTA mati
    return ST_MENU;
  }
  // Ganti kecepatan / jeda / lanjut / jeda loop panjang: posisi sekarang jadi titik awal baru (tidak mengejar)
  uint32_t now = millis();
  if (paused || now - lastFrame > GAP_MS) rebase(scrollX);
  if (btn_pressed(leftBtn) && speedIdx > 0) { rebase(scrollX); speedIdx--; }
  if (btn_pressed(rightBtn) && speedIdx < 2) { rebase(scrollX); speedIdx++; }
  if (okTap) { paused = !paused; rebase(scrollX); redraw = true; }
  lastFrame = now;

  if (!paused) {
    int x = baseX - (int)((now - baseT) * SPEED_PPS[speedIdx] / 1000);
    if (x < -(int)strlen(text) * CHAR_W) rebase(128);  // habis keluar kiri: mulai lagi dari kanan
    else if (x != scrollX) { scrollX = x; redraw = true; }
  }
  if (redraw) draw();
  return ST_TEXT;
}
