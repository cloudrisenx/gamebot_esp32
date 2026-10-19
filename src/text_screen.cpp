#include "text_screen.h"
#include "lang.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "power.h"
#include "screen.h"
#include "settings.h"
#include "ota.h"
#include "emoji_glyphs.h"

extern Adafruit_SSD1306 display;

static const int CHAR_W = 18, TEXT_Y = 18;          // font bawaan ukuran 3: 18x24 px
// Emoji disimpan sebagai 1 byte kode 0x01..EMOJI_COUNT (byte kontrol, tidak dipakai teks biasa)
static const int EMOJI_W = 26;                      // ikon 8x8 diperbesar x3 (24 px) + 2 px jarak
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

static int charW(uint8_t c) { return c < 0x20 ? EMOJI_W : CHAR_W; }

static int textWidth() {
  int w = 0;
  for (const char *p = text; *p; p++) w += charW(*p);
  return w;
}

// Satu karakter UTF-8 -> codepoint; byte tidak valid -> 0 (dibuang). Kembali: jumlah byte dipakai.
static size_t utf8Next(const uint8_t *s, size_t n, uint32_t &cp) {
  uint8_t c = s[0];
  size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
  if (len > n) len = 1;
  cp = len == 1 ? (c < 0x80 ? c : 0) : (uint32_t)(c & (0x7F >> len));
  for (size_t k = 1; k < len; k++) cp = (cp << 6) | (s[k] & 0x3F);
  return len;
}

static uint8_t emojiCode(uint32_t cp) {
  for (const EmojiMap &e : EMOJI_MAP)
    if (e.cp == cp) return e.code;
  return 0;
}

static void appendUtf8(String &out, uint32_t cp) {
  if (cp < 0x800) { out += (char)(0xC0 | cp >> 6); }
  else if (cp < 0x10000) { out += (char)(0xE0 | cp >> 12); out += (char)(0x80 | ((cp >> 6) & 0x3F)); }
  else { out += (char)(0xF0 | cp >> 18); out += (char)(0x80 | ((cp >> 12) & 0x3F)); out += (char)(0x80 | ((cp >> 6) & 0x3F)); }
  out += (char)(0x80 | (cp & 0x3F));
}

String text_getUtf8() {
  load();
  String out;
  for (const char *p = text; *p; p++) {
    uint8_t c = *p;
    if (c >= 0x20) { out += (char)c; continue; }
    for (const EmojiMap &e : EMOJI_MAP)  // codepoint utama = entri pertama kode itu
      if (e.code == c) { appendUtf8(out, e.cp); break; }
  }
  return out;
}

const char *text_get() {
  load();
  return text;
}

bool text_set(const char *s, size_t n) {
  char buf[RTEXT_MAX + 1];
  size_t len = 0;
  for (size_t i = 0; i < n && len < RTEXT_MAX;) {
    uint32_t cp;
    i += utf8Next((const uint8_t *)s + i, n - i, cp);
    if (cp == '\n' || cp == '\r' || cp == '\t') cp = ' ';
    if (cp >= 0x20 && cp < 0x7F) { buf[len++] = cp; continue; }  // font bawaan hanya ASCII
    if (uint8_t code = emojiCode(cp)) { buf[len++] = code; continue; }
    for (const EmojiAscii &e : EMOJI_ASCII)  // emoji lain: emotikon ASCII bila ada padanan, selain itu dibuang
      if (e.cp == cp) {
        for (const char *t = e.text; *t && len < RTEXT_MAX; t++) buf[len++] = *t;
        break;
      }
  }
  while (len && buf[len - 1] == ' ') len--;
  buf[len] = '\0';
  if (!len) return false;
  load();
  strcpy(text, buf);
  Preferences p;
  if (p.begin("rtext", false)) { p.putString("t", text); p.end(); }
  rebase(128);
  return true;
}

static void draw() {
  display.clearDisplay();
  display.setTextWrap(false);
  display.setTextColor(SSD1306_WHITE);
  // Hanya karakter yang terlihat yang digambar
  int x = scrollX;
  for (const char *p = text; *p && x < 128; p++) {
    uint8_t c = *p;
    int w = charW(c);
    if (x + w > 0) {
      if (c >= 0x20) {
        display.drawChar(x, TEXT_Y, c, SSD1306_WHITE, SSD1306_WHITE, 3);
      } else if (c <= EMOJI_COUNT) {
        for (int r = 0; r < 8; r++) {
          uint8_t bits = pgm_read_byte(&EMOJI_BITMAPS[c - 1][r]);
          for (int k = 0; k < 8; k++)
            if (bits & (0x80 >> k)) display.fillRect(x + k * 3, TEXT_Y + r * 3, 3, 3, SSD1306_WHITE);
        }
      }
    }
    x += w;
  }
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
    if (x < -textWidth()) rebase(128);  // habis keluar kiri: mulai lagi dari kanan
    else if (x != scrollX) { scrollX = x; redraw = true; }
  }
  if (redraw) draw();
  return ST_TEXT;
}
