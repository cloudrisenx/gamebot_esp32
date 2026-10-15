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
#define CONTRAST_OPT_COUNT 3  // Rendah / Sedang / Tinggi

uint8_t settings_screenIdx();
void settings_setScreenIdx(uint8_t idx);
uint32_t settings_screenTimeoutMs();  // 0 = tidak pernah

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

String settings_otaPassword();  // 8 karakter acak dibuat sekali, atau yang diatur di halaman OTA (8-63)

// Nama dari tab "Perangkat" halaman OTA (NVS); kosong = bawaan DEVICE_NAME. Berlaku setelah restart.
#define NAME_MAX_LEN 20

// Mode hemat saat layar mati (env lightsleep): uji/cadangan bila BLE bermasalah saat light sleep
uint8_t settings_lang();
void settings_setLang(uint8_t l);  // Lang di lang.h

enum PmMode : uint8_t { PM_MODE_NORMAL, PM_MODE_DFS, PM_MODE_LIGHT_SLEEP, PM_MODE_COUNT };
uint8_t settings_pmMode();
void settings_setPmMode(uint8_t m);
const char *settings_pmModeName();
String settings_deviceName();   // teks layar & manufacturer Remote
String settings_bleName();      // Bluetooth Android/Chronos & iPhone; Remote = nama + " Remote"
String settings_otaSsid();      // WiFi OTA, bawaan DEVICE_NAME "_OTA"
// Validasi semua dulu (ASCII cetak; nama 1-20, SSID 1-32, password 8-63 / kosong = tetap), lalu simpan.
bool settings_setNames(const String &dev, const String &ble, const String &ssid, const String &pw, String &err);
