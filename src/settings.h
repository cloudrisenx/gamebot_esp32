#pragma once
#include <Arduino.h>

void settings_begin();

// Pop-up notifikasi: OFF, hanya WhatsApp, atau semua aplikasi
enum NotifMode : uint8_t { NOTIF_OFF, NOTIF_WHATSAPP, NOTIF_ALL, NOTIF_MODE_COUNT };
NotifMode settings_notifMode();
void settings_setNotifMode(NotifMode m);

// Jenis HP pendamping: Android = aplikasi Chronos, iPhone = layanan bawaan iOS (ANCS/AMS/CTS)
enum PhoneMode : uint8_t { PHONE_ANDROID, PHONE_IPHONE, PHONE_MODE_COUNT };
PhoneMode settings_phoneMode();
void settings_setPhoneMode(PhoneMode m);

bool settings_bt();
void settings_setBt(bool on);

// Hi-score disimpan per NAMA key NVS (HI_KEYS): enum & HI_KEYS diubah bersamaan, data game lain tidak bergeser
enum GameId : uint8_t { GAME_BREAKOUT, GAME_DINO, GAME_HITUNG, GAME_FLAPPY, GAME_PONG, GAME_DUEL, GAME_TTT, GAME_COUNT };  // tambah game baru di akhir

uint32_t settings_hiscore(GameId g);
void settings_setHiscore(GameId g, uint32_t score);
void settings_resetHiscores();

// Opsi pilihan disimpan sebagai indeks; OK pada menu Setting memutar indeks.
#define SCREEN_OPT_COUNT 4    // 15s / 30s / 60s / OFF
#define SLEEP_OPT_COUNT 4     // 2 / 5 / 10 mnt deep sleep / Jaga notif (tanpa deep sleep)
#define CONTRAST_OPT_COUNT 3  // Rendah / Sedang / Tinggi

uint8_t settings_screenIdx();
void settings_setScreenIdx(uint8_t idx);
uint32_t settings_screenTimeoutMs();  // 0 = tidak pernah

uint8_t settings_sleepIdx();
void settings_setSleepIdx(uint8_t idx);
uint32_t settings_sleepTimeoutMs();   // 0 = tidak pernah deep sleep (Jaga notif)
const char *settings_sleepName();     // "2mnt" / "5mnt" / "10mnt" / "Jaga notif"

// Jadwal tidur malam: di dalam jendela [mulai, akhir) jam deep sleep & bangun sendiri di jam akhir
// Waktu dalam menit sejak 00:00 (boleh melewati tengah malam)
bool settings_nightOn();
uint16_t settings_nightStartMin();
uint16_t settings_nightEndMin();
bool settings_setNight(bool on, uint16_t startMin, uint16_t endMin);  // false bila ON dan mulai == selesai
void settings_nightLabel(char *buf, size_t len);                     // "OFF" / "22:00-06:00"

// Hitung: waktu tampil Lv5 (level sebelumnya +200 ms per level)
#define HITUNG_LV5_OPT_COUNT 5  // 2,0s / 1,5s / 1,2s / 1,0s / 0,8s
uint8_t settings_hitungLv5Idx();
void settings_setHitungLv5Idx(uint8_t idx);
uint16_t settings_hitungLv5Ms();
uint16_t settings_hitungLv5MsAt(uint8_t idx);  // ms untuk pilihan idx (layar WAKTU)
const char *settings_hitungLv5Name();
const char *settings_hitungLv5NameAt(uint8_t idx);
// Hitung: level pengecoh (Tanpa / Sedikit / Sedang / Banyak)
#define HITUNG_DIST_OPT_COUNT 4
uint8_t settings_hitungDistIdx();
void settings_setHitungDistIdx(uint8_t idx);
const char *settings_hitungDistNameAt(uint8_t idx);

// Clock I2C OLED: 800 / 400 / 200 kHz
#define I2C_OPT_COUNT 3
uint8_t settings_i2cIdx();
void settings_setI2cIdx(uint8_t idx);
uint32_t settings_i2cHz();

uint8_t settings_contrastIdx();
void settings_setContrastIdx(uint8_t idx);
uint8_t settings_contrastValue();

bool settings_bootAnim();
void settings_setBootAnim(bool on);

String settings_otaPassword();  // 8 karakter acak, dibuat sekali lalu disimpan
