#include "text_screen.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "power.h"
#include "screen.h"
#include "settings.h"
#include "ota.h"

extern Adafruit_SSD1306 display;

static const int CHAR_W = 18, TEXT_Y = 18;          // font bawaan ukuran 3: 18x24 px
static const uint8_t SPEED_PX[3] = {1, 2, 4};       // geser per langkah 30 ms
static const uint32_t STEP_MS = 30;

static char text[RTEXT_MAX + 1];
static bool loaded = false;
static int speedIdx = 1;
static bool paused = false;
static int scrollX = 128;                            // posisi kiri teks
static uint32_t lastStep = 0;

// Tahan OK = menu WiFi (OTA di latar: ubah teks dari HP, langsung tampil). Ketuk OK = jeda.
static const uint32_t MENU_HOLD_MS = 600;
static bool menuOpen = false;
static bool okDown = false;          // OK sedang ditahan (sejak okDownAt)
static bool okLong = false;          // tahan ini sudah membuka menu: lepasnya tidak menjeda
static uint32_t okDownAt = 0;

static void load() {
  if (loaded) return;
  loaded = true;
  snprintf(text, sizeof(text), "Halo dari %s", settings_deviceName().c_str());
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
  scrollX = 128;
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

// Menu WiFi: OK = hidupkan/matikan OTA, BACK = kembali ke teks (OTA tetap seperti dipilih)
static void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  bool on = ota_bgRunning();
  display.setCursor(0, 0);
  display.print("WIFI UBAH TEKS");
  display.drawFastHLine(0, 9, 128, SSD1306_WHITE);
  display.fillRoundRect(0, 13, 128, 13, 3, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(on ? 22 : 19, 16);
  display.print(on ? "Matikan OTA" : "Hidupkan OTA");
  display.setTextColor(SSD1306_WHITE);
  if (on) {
    display.setCursor(0, 30);
    display.print("WiFi: ");
    display.print(ota_ssid());
    display.setCursor(0, 40);
    display.print("Pass: ");
    display.print(ota_password());
    display.setCursor(0, 50);
    display.print("Buka 192.168.4.1");
  } else {
    display.setCursor(0, 32);
    display.print("Ubah teks dari HP:");
    display.setCursor(0, 42);
    display.print("tab Teks di web OTA");
  }
  display.setTextWrap(true);
  oled_flush();
}

void text_enter() {
  load();
  menuOpen = okDown = okLong = false;
  scrollX = 128;
  paused = false;
  lastStep = millis();
  screen_landscape();
  draw();
}

AppState text_update(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  ota_bgPoll();  // teks dari HP langsung berlaku (text_set)

  // OK: ketuk = jeda (dinilai saat dilepas), tahan MENU_HOLD_MS = buka menu WiFi (sekali per tahan)
  if (btn_pressed(BTN_OK)) { okDown = true; okLong = false; okDownAt = millis(); }
  bool okTap = false;
  if (okDown) {
    if (!btn_held(BTN_OK)) {
      okDown = false;
      okTap = !okLong;
    } else if (!okLong && !menuOpen && millis() - okDownAt >= MENU_HOLD_MS) {
      okLong = true;
      menuOpen = true;
      redraw = true;
    }
  }

  if (menuOpen) {
    if (btn_pressed(BTN_BACK)) { menuOpen = false; lastStep = millis(); draw(); return ST_TEXT; }
    if (okTap) {
      if (ota_bgRunning()) ota_bgStop();
      else ota_bgStart();
      redraw = true;
    }
    buttons_clear();
    static uint32_t lastMenuDraw = 0;
    if (redraw || millis() - lastMenuDraw >= 1000) { lastMenuDraw = millis(); drawMenu(); }
    return ST_TEXT;
  }

  if (btn_pressed(BTN_BACK)) {
    ota_bgStop();  // keluar dari Teks = WiFi OTA mati
    return ST_MENU;
  }
  if (btn_pressed(leftBtn) && speedIdx > 0) { speedIdx--; redraw = true; }
  if (btn_pressed(rightBtn) && speedIdx < 2) { speedIdx++; redraw = true; }
  if (okTap) { paused = !paused; redraw = true; }

  uint32_t now = millis();
  if (!paused && now - lastStep >= STEP_MS) {
    int steps = min((int)((now - lastStep) / STEP_MS), 5);
    lastStep += steps * STEP_MS;
    int width = (int)strlen(text) * CHAR_W;
    scrollX -= steps * SPEED_PX[speedIdx];
    if (scrollX < -width) scrollX = 128;  // habis keluar kiri: mulai lagi dari kanan
    redraw = true;
  } else if (paused) {
    lastStep = now;
  }
  if (redraw) draw();
  return ST_TEXT;
}
