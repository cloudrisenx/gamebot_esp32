#include "buttons.h"
#include <esp_timer.h>
#include <driver/gpio.h>

// Urutan sesuai enum Btn: UP, DOWN, OK, BACK
static const uint8_t PINS[BTN_COUNT] = {3, 1, 10, 6};
static const uint32_t DEBOUNCE_MS = 25;

static bool rawState[BTN_COUNT];
static bool stableState[BTN_COUNT];
static uint32_t lastChange[BTN_COUNT];
static bool pending[BTN_COUNT];

void buttons_begin() {
  for (int i = 0; i < BTN_COUNT; i++) {
    pinMode(PINS[i], INPUT_PULLUP);
    rawState[i] = stableState[i] = digitalRead(PINS[i]);
    lastChange[i] = millis();
    pending[i] = false;
  }
}

void buttons_update() {
  uint32_t now = millis();
  for (int i = 0; i < BTN_COUNT; i++) {
    bool r = digitalRead(PINS[i]);
    if (r != rawState[i]) {
      rawState[i] = r;
      lastChange[i] = now;
    }
    if (r != stableState[i] && now - lastChange[i] >= DEBOUNCE_MS) {
      stableState[i] = r;
      if (r == LOW) pending[i] = true;  // aktif LOW: HIGH -> LOW = ditekan
    }
  }
}

bool btn_pressed(Btn b) {
  bool p = pending[b];
  pending[b] = false;
  return p;
}

bool btn_any_pressed() {
  bool any = false;
  for (int i = 0; i < BTN_COUNT; i++) {
    any |= pending[i];
    pending[i] = false;
  }
  return any;
}

bool btn_held(Btn b) { return stableState[b] == LOW; }

bool btn_any_pending() {
  for (int i = 0; i < BTN_COUNT; i++)
    if (pending[i]) return true;
  return false;
}

void buttons_clear() {
  for (int i = 0; i < BTN_COUNT; i++) pending[i] = false;
}

// ---- Interrupt per pin: bangunkan loop + stempel waktu tekan pertama (Duel) ----
// Polling + debounce di atas tetap dipakai menu & game lain; stempel hanya dibaca bila diminta.
static void (*wakeCb)() = nullptr;
static portMUX_TYPE stampMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint64_t pressUs[BTN_COUNT];  // 0 = belum ada tekan sejak btn_armTiming()
// Double-tap: dua tekan sah terakhir per tombol (tepi turun; pantulan < 30 ms diabaikan)
// Tap = ditekan lalu dilepas dalam 10..150 ms. Menahan tombol (mis. untuk bergerak) bukan tap.
static const uint64_t TAP_BOUNCE_US = 30000;    // tepi turun < 30 ms setelah tepi turun sah = pantulan
static const uint64_t RELEASE_QUIET_US = 20000; // tepi turun < 20 ms setelah lepas = pantulan saat lepas
static const uint64_t TAP_MIN_US = 10000, TAP_MAX_US = 150000;
static volatile uint64_t lastFallUs[BTN_COUNT], lastRiseUs[BTN_COUNT], downUs[BTN_COUNT];
static volatile bool downOpen[BTN_COUNT];       // ada tekan yang belum dilepas
static volatile uint64_t lastUs[BTN_COUNT], prevUs[BTN_COUNT];  // waktu tekan dua tap sah terakhir
static volatile uint8_t tapSeq[BTN_COUNT];      // naik tiap tap sah
static volatile uint8_t fallSeq[BTN_COUNT];     // naik tiap tekan sah (tepi turun setelah filter pantulan)
static uint8_t chordUsed[BTN_COUNT];           // fallSeq yang sudah dipakai sebuah chord (loop saja)
static uint8_t tapReported[BTN_COUNT];         // tapSeq saat double-tap terakhir dilaporkan (loop saja)

static void IRAM_ATTR onPinEdge(void *arg) {
  int i = (int)(intptr_t)arg;
  uint64_t t = esp_timer_get_time();
  portENTER_CRITICAL_ISR(&stampMux);
  if (gpio_get_level((gpio_num_t)PINS[i]) == 0) {
    // Tepi turun (ditekan). Duel: catat yang PERTAMA saja; pantulan kontak tidak menimpanya
    if (pressUs[i] == 0) pressUs[i] = t;
    if (t - lastFallUs[i] >= TAP_BOUNCE_US && t - lastRiseUs[i] >= RELEASE_QUIET_US) {
      lastFallUs[i] = t;
      downUs[i] = t;
      downOpen[i] = true;
      fallSeq[i]++;
    }
  } else {
    // Tepi naik (dilepas): tekan singkat = tap. Naik < 10 ms setelah turun = pantulan (tekan tetap terbuka)
    lastRiseUs[i] = t;
    if (downOpen[i] && t - downUs[i] >= TAP_MIN_US) {
      downOpen[i] = false;
      if (t - downUs[i] <= TAP_MAX_US) {
        prevUs[i] = lastUs[i];
        lastUs[i] = downUs[i];
        tapSeq[i]++;
      }
    }
  }
  portEXIT_CRITICAL_ISR(&stampMux);
  if (wakeCb) wakeCb();
}

// Bangunkan loop seketika saat tombol berubah (tekan/lepas); debounce tetap di buttons_update()
void buttons_attachWake(void (*isr)()) {
  wakeCb = isr;
  for (int i = 0; i < BTN_COUNT; i++)
    attachInterruptArg(digitalPinToInterrupt(PINS[i]), onPinEdge, (void *)(intptr_t)i, CHANGE);
}

void btn_armTiming() {
  portENTER_CRITICAL(&stampMux);
  for (int i = 0; i < BTN_COUNT; i++) pressUs[i] = 0;
  portEXIT_CRITICAL(&stampMux);
}

bool btn_doubleTap(Btn b, uint32_t windowMs) {
  portENTER_CRITICAL(&stampMux);
  uint8_t seq = tapSeq[b];
  uint64_t last = lastUs[b], prev = prevUs[b];
  portEXIT_CRITICAL(&stampMux);
  // Pasangan harus dua tekan BARU sejak laporan terakhir (3 tekan cepat = 1 double-tap)
  if ((uint8_t)(seq - tapReported[b]) < 2 || !prev || last - prev > (uint64_t)windowMs * 1000) return false;
  tapReported[b] = seq;
  return true;
}

void btn_tapSync() {
  portENTER_CRITICAL(&stampMux);
  for (int i = 0; i < BTN_COUNT; i++) {
    tapReported[i] = tapSeq[i];
    chordUsed[i] = fallSeq[i];
  }
  portEXIT_CRITICAL(&stampMux);
}

bool btn_chord(Btn a, Btn b, uint32_t windowMs) {
  portENTER_CRITICAL(&stampMux);
  uint8_t sa = fallSeq[a], sb = fallSeq[b];
  uint64_t ta = lastFallUs[a], tb = lastFallUs[b];
  portEXIT_CRITICAL(&stampMux);
  // Kedua tekan harus BARU (belum dipakai chord) dan hampir bersamaan
  if (sa == chordUsed[a] || sb == chordUsed[b]) return false;
  uint64_t d = ta > tb ? ta - tb : tb - ta;
  if (d > (uint64_t)windowMs * 1000) return false;
  chordUsed[a] = sa;
  chordUsed[b] = sb;
  return true;
}

bool btn_pressTimeUs(Btn b, uint64_t &us) {
  portENTER_CRITICAL(&stampMux);
  us = pressUs[b];
  portEXIT_CRITICAL(&stampMux);
  return us != 0;
}
