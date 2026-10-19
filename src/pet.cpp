#include "pet.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include "pet_core.h"
#include "power.h"
#include "screen.h"

extern Adafruit_SSD1306 display;

static const uint32_t FRAME_MS = 33;        // maks 30 fps
static const uint32_t PAUSE_GAP_MS = 200;   // loop terhenti lebih lama dari ini (pop-up, panggilan) = Pet dibekukan
static const uint32_t AWAKE_MS = 300000;    // layar tetap menyala 5 menit sejak tombol terakhir, lalu aturan "Layar mati" biasa

static pet::Engine engine;
static uint8_t fbBuf[pet::W * pet::H];
static pet::Fb fb(pet::W, pet::H, fbBuf);
static uint32_t frozenMs = 0;   // total waktu beku: waktu Pet = millis() - frozenMs
static uint32_t lastSeen = 0, lastFrame = 0, lastButton = 0;
static bool wasOn = true;
static bool held[3] = {false, false, false};
static const Btn BTNS[3] = {BTN_UP, BTN_DOWN, BTN_OK};  // urutan tombol mesin: 0 = kiri, 1 = kanan, 2 = OK

static double petNow() { return (double)(uint32_t)(millis() - frozenMs); }

void pet_enter() {
  screen_landscape();
  uint32_t now = millis();
  frozenMs = now;  // waktu Pet mulai dari 0
  lastSeen = lastButton = now;
  lastFrame = 0;
  wasOn = true;
  for (int i = 0; i < 3; i++) held[i] = false;
  engine.begin(esp_random(), 0);
}

void pet_notify() { engine.notify(petNow()); }

bool pet_keepAwake() { return millis() - lastButton < AWAKE_MS; }

// Salin gambar Pet langsung ke buffer SSD1306 (rotasi 0, landscape): byte = 8 piksel tegak
static void present() {
  uint8_t *buf = display.getBuffer();
  memset(buf, 0, pet::W * pet::H / 8);
  for (int y = 0; y < pet::H; y++) {
    const uint8_t *row = fbBuf + y * pet::W;
    uint8_t bit = 1 << (y & 7);
    uint8_t *page = buf + (y >> 3) * pet::W;
    for (int x = 0; x < pet::W; x++)
      if (row[x]) page[x] |= bit;
  }
  oled_flush();
}

AppState pet_update(bool redraw) {
  uint32_t now = millis();
  uint32_t gap = now - lastSeen;
  lastSeen = now;
  bool on = power_screenOn();
  // Layar mati, atau loop sempat tertahan (pop-up, panggilan): bekukan Pet supaya tidak melompat
  if (!on || gap > PAUSE_GAP_MS) frozenMs += gap;
  if (!on) {
    wasOn = false;
    return ST_PET;
  }
  if (!wasOn) {  // layar baru menyala lagi: hitung sebagai interaksi (layar tidak langsung padam lagi)
    wasOn = true;
    lastButton = now;
    redraw = true;
  }
  if (btn_pressed(BTN_BACK)) return ST_HOME;

  double t = petNow();
  for (int i = 0; i < 3; i++) {
    if (btn_pressed(BTNS[i])) {
      engine.press(t, i);
      held[i] = true;
      lastButton = now;
    }
    // Tombol dilepas (tekan & lepas dalam satu putaran loop tetap terhitung ketukan)
    if (held[i] && !btn_held(BTNS[i])) {
      engine.release(t, i);
      held[i] = false;
      lastButton = now;
    }
  }
  if (!redraw && now - lastFrame < FRAME_MS) return ST_PET;
  lastFrame = now;
  pet::drawFace(fb, engine.frame(t));
  present();
  return ST_PET;
}
