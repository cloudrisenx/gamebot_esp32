#pragma once
#include <Arduino.h>

// Orientasi potret (64x128) untuk game. Jika layar terbalik 180°, ganti 1 <-> 3.
#define PORTRAIT_ROTATION 1
// Jika arah kiri/kanan terbalik saat perangkat diputar, ganti ke 1.
#define PORTRAIT_SWAP_LR 0
// Carousel menu (landscape): 0 = UP item sebelumnya / DOWN item berikutnya, sama dengan potret
// (UP = kiri). Jika arah geser terasa terbalik di perangkat, ganti ke 1.
#define MENU_SWAP_LR 0

void screen_portrait();
void screen_landscape();

// Kontrol mode potret: UP/DOWN fisik menjadi kiri/kanan
bool btn_leftHeld();
bool btn_rightHeld();
// Kontrol landscape (Breakout, Shooter): pemetaan sama dengan carousel menu (MENU_SWAP_LR)
bool btn_landLeftHeld();
bool btn_landRightHeld();

// ---- OLED I2C: kirim frame dengan cek error, clock dapat diganti saat jalan, statistik debug ----
void oled_begin(uint8_t addr);              // sekali di setup() sebelum display.begin
uint32_t oled_applyClock(uint32_t hz);      // pasang clock (cek ACK; gagal -> 400 kHz, lalu 200 kHz). Hasil = clock aktif
uint32_t oled_clockHz();
bool oled_sendFrame();                      // kirim buffer display; false bila ada error I2C
bool oled_reinit();                         // inisialisasi ulang controller, buffer dipertahankan
uint32_t oled_errors();
uint32_t oled_frames();
uint32_t oled_reinits();
uint32_t oled_lastUs();
uint32_t oled_maxUs();                      // maks sejak boot (berbeda dari power_takeFlushMaxUs)
