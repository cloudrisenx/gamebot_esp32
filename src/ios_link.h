#pragma once
#include <Arduino.h>
#include "version.h"
#include <ChronosESP32.h>  // struct MusicInfo & enum Control dipakai bersama mode Android

// Mode iPhone: tanpa aplikasi, memakai layanan bawaan iOS lewat BLE
//   ANCS (notifikasi & panggilan), AMS (musik), CTS (jam), Battery Service (bila ada).
#define IOS_BLE_NAME DEVICE_NAME

void ios_begin();
void ios_stop();
bool ios_running();
bool ios_connected();
bool ios_readTime();
// Minta iPhone mengubah parameter koneksi (unit 1,25 ms / 10 ms); false bila tidak tersambung
bool ios_updateConnParams(uint16_t minItv, uint16_t maxItv, uint16_t latency, uint16_t timeout);    // baca ulang jam (CTS); false bila tidak tersambung  // terhubung & terenkripsi (sudah di-pair)
void ios_loop();       // panggil tiap loop: setup GATT & proses data dari iPhone
String ios_address();
uint8_t ios_battery();  // 0 = tidak tersedia
MusicInfo &ios_music();
bool ios_musicProgress(uint32_t &elapsedMs, uint32_t &durationMs);
void ios_musicControl(Control cmd);
void ios_callAction(bool accept);  // angkat / tolak panggilan yang sedang berdering
bool ios_deleteBonds();

// ---- Parser format Apple (fungsi murni; logikanya juga diuji di Python) ----
struct AncsEvent {
  uint8_t eventId;   // 0 Added, 1 Modified, 2 Removed
  uint8_t flags;     // bit2 = PreExisting
  uint8_t category;  // 1 IncomingCall, 2 MissedCall, 4 Social, ...
  uint32_t uid;
};
bool ancs_parseNotificationSource(const uint8_t *d, size_t n, AncsEvent &e);
// Respons "Get Notification Attributes" (Data Source), bisa terpecah beberapa notifikasi.
// Kembalian: >0 = jumlah byte satu respons lengkap, 0 = belum lengkap, -1 = format rusak.
int ancs_parseAttributes(const uint8_t *d, size_t n, uint32_t &uid, char *appId, size_t appLen,
                         char *title, size_t titleLen, char *msg, size_t msgLen);
// AMS PlaybackInfo "state,rate,elapsed" (mis. "1,1.0,123.4")
bool ams_parsePlaybackInfo(const char *s, int &state, float &rate, float &elapsed);
// CTS Current Time (10 byte): tahun LE, bulan 1-12, hari, jam, menit, detik, ...
bool cts_parse(const uint8_t *d, size_t n, int &year, int &month, int &day, int &hour, int &minute, int &second);
