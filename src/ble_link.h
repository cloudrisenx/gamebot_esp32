#pragma once
#include <Arduino.h>
#include <ChronosESP32.h>
#include "version.h"

#define BLE_NAME DEVICE_NAME

void ble_begin();
void ble_update();                 // panggil setiap loop(): start/stop BLE + watch.loop()
void ble_setNavScreen(bool open);  // layar Navigasi menyalakan BLE walau Setting BT = OFF
void ble_shutdown();               // matikan BLE sebelum deep sleep
void ble_setSuspended(bool s);     // paksa BLE mati (mode OTA WiFi), abaikan setting BT
void ble_remoteStart();            // layar Kamera (HID): hentikan Chronos/ANCS, mulai HID
void ble_remoteStop();             // hentikan HID, kembali ke Mode HP (Android/iPhone) sesuai setting

bool ble_running();
bool ble_connected();
String ble_address();
uint8_t ble_phoneBattery();        // 0 jika belum ada data
bool ble_timeStr(char *buf, size_t len);  // "HH:MM"; false jika jam belum disinkron Chronos
// Tanggal lokal: wday 0=Minggu..6, day 1..31, month 0..11; false jika jam belum disinkron
bool ble_date(int &wday, int &day, int &month);

bool ble_navChanged();             // true sekali setiap ada data navigasi baru
bool ble_navActive();              // terhubung dan rute Google Maps sedang berjalan
Navigation &ble_nav();
String ble_lastNotifInfo();         // "Notif: <app> 0x<ikon>" untuk layar Info (diagnostik)

// Ambil notifikasi WhatsApp yang menunggu. true jika ada (lalu dihapus dari antrean).
bool ble_takeWaNotif(char *sender, size_t senderLen, char *msg, size_t msgLen);

// Panjang maks isi pesan notifikasi (termasuk NUL). Paket Chronos maks 512 byte (CS_DATA_SIZE).
#define NOTIF_MSG_LEN 512

// Riwayat notifikasi yang lolos filter "Notif pop-up" (disimpan di RAM, indeks 0 = terbaru)
struct NotifEntry {
  char app[16];
  char title[32];
  char msg[NOTIF_MSG_LEN];
  char time[6];  // "HH:MM" atau "--:--"
};
int ble_historyCount();
int ble_unreadCount();   // notifikasi masuk sejak layar Notifikasi terakhir dibuka
void ble_markRead();
const NotifEntry &ble_historyAt(int index);
void ble_historyDelete(int index);  // hapus satu entri; entri yang lebih lama naik satu indeks

// Panggilan masuk: true selama HP berdering (nama penelepon disalin ke `name`)
bool ble_ringing(char *name, size_t len);

// Fitur HP lewat Chronos
MusicInfo &ble_music();
void ble_musicControl(Control cmd);
int ble_weatherCount();
Weather &ble_weatherAt(int index);
String ble_weatherCity();
String ble_weatherTime();
bool ble_weatherIsCache();    // data cuaca berasal dari cache NVS (Chronos belum mengirim sejak restart)
int ble_weatherCacheAgeH();   // umur cache (jam), -1 = tidak diketahui
bool ble_phoneInfoChanged();  // true sekali setiap ada info musik/cuaca baru

// Mode HP (Android = Chronos, iPhone = ANCS/AMS/CTS) & fitur khusus iPhone
bool ble_isIphone();
bool ble_musicProgress(uint32_t &elapsedMs, uint32_t &durationMs);  // posisi & durasi lagu (hanya iPhone)
void ble_callAnswer(bool accept);  // iPhone: angkat/tolak panggilan yang berdering
bool ble_deletePairings();         // hapus semua pairing (Android/iPhone/Remote) + restart BLE

// Dipakai ios_link agar notifikasi/panggilan/jam memakai jalur yang sama dengan mode Android
void ble_pushNotification(int icon, const char *app, const char *title, const char *msg);
void ble_setRinging(const char *name, bool on);  // name nullptr = tidak berubah
void ble_setLocalTime(int sec, int min, int hour, int day, int month, int year);
void ble_markPhoneInfoChanged();

// Jam sistem (dipakai clockstore): set tanpa menandai sinkron, baca epoch lokal, ambil tanda sinkron dari HP
void ble_setClock(uint32_t epoch);
void ble_setClockParts(int sec, int min, int hour, int day, int month, int year);
uint32_t ble_epoch();
bool ble_takeTimeSynced();
bool ble_requestTimeSync();
// true sekali saat Chronos melapor sinkron selesai (CF_SYNCED); withTime = jam ikut dikirim
bool ble_takeSyncDone(bool &withTime);
// Layar mati: minta HP memperlonggar koneksi (hemat radio); layar nyala: cepat lagi. Panggil tiap loop.
void ble_setLowPowerLink(bool saving);  // minta HP kirim ulang jam; false bila tidak tersambung
void ble_restartRadio();  // matikan stack BLE; ble_update() menyalakannya lagi bila BT diinginkan
