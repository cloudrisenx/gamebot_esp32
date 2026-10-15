#include "anim.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include "frames_store.h"
#include "screen.h"
#include "ota.h"

extern Adafruit_SSD1306 display;

static const uint8_t SPEED_X2[] = {1, 2, 4};  // kecepatan x2: 0.5x, 1x, 2x
static const char *SPEED_LABEL[] = {"0.5x", "1x", "2x"};
static const uint32_t OVERLAY_MS = 1000;

static uint16_t frame = 0;
static bool paused = false;
static uint8_t speedIdx = 1;
static uint32_t nextAtUs = 0;       // jadwal frame berikutnya (micros): tetap, tidak menumpuk telat
static uint32_t overlayUntil = 0;
static uint16_t lastCount = 0;     // jumlah frame terakhir: berubah = animasi baru diupload lewat OTA

// Statistik untuk log "[ANIM]" tiap STATS_MS
static const uint32_t STATS_MS = 5000;
static uint32_t statsStart = 0, statsFrames = 0, statsDecodeMaxUs = 0, statsFlushMaxUs = 0;

void anim_enter() {
  frame = 0;
  paused = false;
  speedIdx = 1;
  nextAtUs = micros();
  overlayUntil = 0;
  lastCount = frames_count();
  ota_holdMenuReset();
  statsStart = millis();
  statsFrames = statsDecodeMaxUs = statsFlushMaxUs = 0;
}

void anim_resume() { nextAtUs = micros(); }

bool anim_isPlaying() { return frames_valid() && !paused; }

static void drawFrame() {
  uint32_t t0 = micros();
  frames_drawFrame(frame);  // ikut memutar layar bila animasi potret (FRM2: termasuk decode)
  statsDecodeMaxUs = max(statsDecodeMaxUs, micros() - t0);
  if (paused || millis() < overlayUntil) {
    String label = paused ? String("II ") + SPEED_LABEL[speedIdx] : String(SPEED_LABEL[speedIdx]);
    int w = label.length() * 6 + 4;
    int right = display.width();
    display.fillRect(right - w, 0, w, 11, SSD1306_BLACK);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(right - w + 2, 2);
    display.print(label);
  }
  uint32_t t1 = micros();
  oled_flush();
  statsFlushMaxUs = max(statsFlushMaxUs, micros() - t1);
  statsFrames++;
}

static void logStats() {
  uint32_t elapsed = millis() - statsStart;
  if (elapsed < STATS_MS) return;
  if (!paused)
    Serial.printf("[ANIM] fps target=%.1f aktual=%.1f decode maks=%.1f ms flush maks=%.1f ms\n",
                  frames_fps() * SPEED_X2[speedIdx] / 2.0f, statsFrames * 1000.0f / elapsed,
                  statsDecodeMaxUs / 1000.0f, statsFlushMaxUs / 1000.0f);
  statsStart = millis();
  statsFrames = statsDecodeMaxUs = statsFlushMaxUs = 0;
}

AppState anim_update(bool redraw) {
  // Tahan OK = menu WiFi (OTA latar: kirim animasi dari HP); ketuk OK = jeda
  OtaMenu m = ota_holdMenu(T("WIFI ANIMASI", "WIFI ANIMATION"), T("Kirim animasi dari HP:", "Send animation from"),
                           T("tab Animasi di web OTA", "phone: Animation tab"));
  if (m == OM_OPEN) return ST_ANIM;
  if (m == OM_CLOSED) { nextAtUs = micros(); redraw = true; }
  if (btn_pressed(BTN_BACK)) {
    ota_bgStop();  // keluar dari Animasi = WiFi OTA mati
    screen_landscape();
    return ST_MENU;
  }
  // Animasi baru selesai diupload: putar dari awal
  if (frames_count() != lastCount) {
    lastCount = frames_count();
    frame = 0;
    nextAtUs = micros();
    redraw = true;
  }

  if (!frames_valid()) {
    buttons_clear();
    if (redraw) {
      screen_landscape();
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      const char *lines[4] = {T("Belum ada animasi", "No animation yet"), T("tahan OK = WiFi OTA,", "hold OK = WiFi OTA,"),
                              T("kirim di tab Animasi", "send in Animation tab"), "BACK: menu"};
      static const int Y[4] = {16, 28, 38, 54};
      for (int i = 0; i < 4; i++) {
        display.setCursor((128 - (int)strlen(lines[i]) * 6) / 2, Y[i]);
        display.print(lines[i]);
      }
      oled_flush();
    }
    return ST_ANIM;
  }

  if (m == OM_TAP) { paused = !paused; nextAtUs = micros(); redraw = true; }
  if (btn_pressed(BTN_UP) && speedIdx < 2) { speedIdx++; overlayUntil = millis() + OVERLAY_MS; redraw = true; }
  if (btn_pressed(BTN_DOWN) && speedIdx > 0) { speedIdx--; overlayUntil = millis() + OVERLAY_MS; redraw = true; }

  uint32_t now = micros();
  if (!paused && (int32_t)(now - nextAtUs) >= 0) {
    // Jadwal tetap: nextAt += interval (telat kecil tidak menumpuk). Tertinggal > 2 interval
    // (mis. setelah pop-up): lompati frame ke posisi seharusnya, jangan mengejar satu per satu.
    uint32_t interval = 2000000UL / ((uint32_t)frames_fps() * SPEED_X2[speedIdx]);
    uint32_t late = now - nextAtUs;
    uint32_t steps = 1;
    if (late > 2 * interval) steps += late / interval;
    nextAtUs += steps * interval;
    frame = (frame + steps) % frames_count();
    redraw = true;
  }
  if (redraw) drawFrame();
  logStats();
  return ST_ANIM;
}
