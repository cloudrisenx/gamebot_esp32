#include "screen.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include <Wire.h>
#include "debuglog.h"

extern Adafruit_SSD1306 display;

// ---- OLED I2C ----
// Adafruit_SSD1306 memasang clock I2C miliknya sendiri (wireClk/restoreClk, protected) di setiap
// perintah, jadi Wire.setClock() saja tidak cukup. Diubah lewat pointer-to-member.
struct OledClock : Adafruit_SSD1306 {
  static void set(Adafruit_SSD1306 &d, uint32_t hz) {
    d.*(&OledClock::wireClk) = hz;
    d.*(&OledClock::restoreClk) = hz;
  }
};

static uint8_t oledAddr = 0x3C;
static uint32_t oledHz = 400000;
static uint32_t nErrors = 0, nFrames = 0, nReinits = 0, lastUs = 0, maxUs = 0;
static const int CHUNK = 127;  // buffer Wire ESP32 = 128 byte: 1 byte kontrol + 127 data

void oled_begin(uint8_t addr) { oledAddr = addr; }

static bool responds(uint32_t hz) {
  Wire.setClock(hz);
  Wire.beginTransmission(oledAddr);
  return Wire.endTransmission() == 0;
}

uint32_t oled_applyClock(uint32_t hz) {
  if (!responds(hz)) hz = responds(400000) ? 400000 : 200000;
  OledClock::set(display, hz);
  Wire.setClock(hz);
  oledHz = hz;
  return hz;
}

uint32_t oled_clockHz() { return oledHz; }

// Jendela alamat penuh disetel ulang SETIAP frame (byte hilang di frame lalu tidak menggeser frame ini),
// lalu 1024 byte data dalam potongan 127 byte. Tiap endTransmission dicek.
static bool sendOnce() {
  static const uint8_t CMDS[] = {0x00, 0x21, 0, 127, 0x22, 0, 7};  // kontrol perintah, COLUMNADDR, PAGEADDR
  bool ok = true;
  Wire.beginTransmission(oledAddr);
  Wire.write(CMDS, sizeof(CMDS));
  ok &= Wire.endTransmission() == 0;
  const uint8_t *buf = display.getBuffer();
  for (int pos = 0; pos < 1024; pos += CHUNK) {
    int n = min(CHUNK, 1024 - pos);
    Wire.beginTransmission(oledAddr);
    Wire.write((uint8_t)0x40);
    Wire.write(buf + pos, n);
    ok &= Wire.endTransmission() == 0;
  }
  return ok;
}

bool oled_sendFrame() {
  uint32_t t0 = micros();
  bool ok = sendOnce();
  if (!ok) {
    nErrors++;
    ok = sendOnce();  // kirim ulang sekali seluruh frame
    if (!ok) nErrors++;
  }
  nFrames++;
  lastUs = micros() - t0;
  maxUs = max(maxUs, lastUs);
  return ok;
}

bool oled_reinit() {
  static uint8_t backup[1024];  // begin() menghapus buffer: simpan dulu
  memcpy(backup, display.getBuffer(), sizeof(backup));
  bool ok = display.begin(SSD1306_SWITCHCAPVCC, oledAddr, false, false);
  memcpy(display.getBuffer(), backup, sizeof(backup));
  OledClock::set(display, oledHz);
  Wire.setClock(oledHz);
  nReinits++;
  dlog("OLED re-init %s", ok ? "ok" : "gagal");
  return ok;
}

uint32_t oled_errors() { return nErrors; }
uint32_t oled_frames() { return nFrames; }
uint32_t oled_reinits() { return nReinits; }
uint32_t oled_lastUs() { return lastUs; }
uint32_t oled_maxUs() { return maxUs; }

void screen_portrait() { display.setRotation(PORTRAIT_ROTATION); }

void screen_landscape() { display.setRotation(0); }

bool btn_leftHeld() { return btn_held(PORTRAIT_SWAP_LR ? BTN_DOWN : BTN_UP); }

bool btn_rightHeld() { return btn_held(PORTRAIT_SWAP_LR ? BTN_UP : BTN_DOWN); }

bool btn_landLeftHeld() { return btn_held(MENU_SWAP_LR ? BTN_DOWN : BTN_UP); }

bool btn_landRightHeld() { return btn_held(MENU_SWAP_LR ? BTN_UP : BTN_DOWN); }
