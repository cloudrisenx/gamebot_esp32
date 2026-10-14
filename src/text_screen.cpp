#include "text_screen.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "power.h"
#include "screen.h"

extern Adafruit_SSD1306 display;

static const char *const DEFAULT_TEXT = "Halo dari cute_device";
static const int CHAR_W = 18, TEXT_Y = 18;          // font bawaan ukuran 3: 18x24 px
static const uint8_t SPEED_PX[3] = {1, 2, 4};       // geser per langkah 30 ms
static const uint32_t STEP_MS = 30;

static char text[RTEXT_MAX + 1];
static bool loaded = false;
static int speedIdx = 1;
static bool paused = false;
static int scrollX = 128;                            // posisi kiri teks
static uint32_t lastStep = 0;

static void load() {
  if (loaded) return;
  loaded = true;
  strncpy(text, DEFAULT_TEXT, sizeof(text) - 1);
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
  // Status bawah: kecepatan (titik) & jeda
  for (int i = 0; i < 3; i++) {
    if (i <= speedIdx) display.fillRect(2 + i * 5, 58, 3, 3, SSD1306_WHITE);
    else display.drawRect(2 + i * 5, 58, 3, 3, SSD1306_WHITE);
  }
  if (paused) {
    display.fillRect(120, 56, 2, 7, SSD1306_WHITE);
    display.fillRect(124, 56, 2, 7, SSD1306_WHITE);
  }
  display.setTextWrap(true);
  oled_flush();
}

void text_enter() {
  load();
  scrollX = 128;
  paused = false;
  lastStep = millis();
  screen_landscape();
  draw();
}

AppState text_update(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  if (btn_pressed(BTN_BACK)) return ST_MENU;
  if (btn_pressed(leftBtn) && speedIdx > 0) { speedIdx--; redraw = true; }
  if (btn_pressed(rightBtn) && speedIdx < 2) { speedIdx++; redraw = true; }
  if (btn_pressed(BTN_OK)) { paused = !paused; redraw = true; }

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
