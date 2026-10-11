#include "clockstore.h"
#include <Preferences.h>
#include "ble_link.h"

static const uint32_t VALID_EPOCH = 1704067200UL;     // 1 Jan 2024: sebelum ini = jam belum disetel
static const uint32_t SAVE_EVERY_MS = 15UL * 60 * 1000;
static const uint32_t SYNC_MIN_GAP_S = 15UL * 60;     // sinkron beruntun (sering tiap sambung ulang): tulis maks 1x/15 mnt
// Tunda tulis NVS sesudah sinkron: writeNvs() kadang blocking ~1-3 dtk (NVS wear-leveling),
// dan stall sepanjang itu tepat sesudah connect bisa membuat HP supervision-timeout koneksi
// BLE (disconnect), lalu callback GATT yang masih nyangkut dari percobaan sebelumnya crash
// NimBLE-Arduino saat reconnect. Menunda tulis sampai link sempat stabil menghindari tabrakan itu.
static const uint32_t SYNC_SAVE_DELAY_MS = 5000UL;

static ClockSource source = CLK_NONE;
static uint32_t lastSync = 0;
static uint32_t lastSavedEpoch = 0;
static uint32_t lastSaveMs = 0;
static uint32_t pendingSaveAtMs = 0;  // 0 = tidak ada tulis tertunda

static bool valid(uint32_t e) { return e >= VALID_EPOCH; }

static void writeNvs(const char *why) {
  uint32_t now = ble_epoch();
  if (!valid(now)) return;
  Preferences p;
  if (!p.begin("clock", false)) return;
  p.putUInt("epoch", now);
  p.putUChar("src", source);
  p.putUInt("sync", lastSync);
  p.end();
  lastSavedEpoch = now;
  lastSaveMs = millis();
  Serial.printf("[JAM] simpan (%s): epoch %lu, sumber %s\n", why, (unsigned long)now, clock_sourceName());
}

void clock_begin() {
  Preferences p;
  uint32_t saved = 0;
  uint8_t src = CLK_NONE;
  if (p.begin("clock", true)) {
    saved = p.getUInt("epoch", 0);
    src = p.getUChar("src", CLK_NONE);
    lastSync = p.getUInt("sync", 0);
    p.end();
  }
  lastSavedEpoch = saved;
  lastSaveMs = millis();
  uint32_t now = ble_epoch();
  if (valid(now)) {
    // Jam sistem masih berjalan (bangun dari deep sleep / reset software): sumber tetap seperti tersimpan
    source = src == CLK_NONE ? CLK_ESTIMATE : (ClockSource)src;
    Serial.printf("[JAM] boot: jam sistem masih valid (epoch %lu), sumber %s\n", (unsigned long)now, clock_sourceName());
  } else if (valid(saved)) {
    ble_setClock(saved);
    source = CLK_ESTIMATE;
    Serial.printf("[JAM] boot: jam hilang, pakai tersimpan epoch %lu (PERKIRAAN, tertinggal sejak mati)\n",
                  (unsigned long)saved);
  } else {
    source = CLK_NONE;
    Serial.println("[JAM] boot: belum ada jam tersimpan, tunggu sinkron / setel manual");
  }
}

void clock_loop() {
  if (ble_takeTimeSynced()) {
    ClockSource before = source;
    source = CLK_SYNC;
    lastSync = ble_epoch();
    bool write = before != CLK_SYNC || !valid(lastSavedEpoch) || lastSync < lastSavedEpoch ||
                 lastSync - lastSavedEpoch >= SYNC_MIN_GAP_S;
    if (write) pendingSaveAtMs = millis() + SYNC_SAVE_DELAY_MS;  // tunda, lihat SYNC_SAVE_DELAY_MS
    else Serial.println("[JAM] sinkron HP (NVS tidak ditulis: < 15 mnt sejak simpan terakhir)");
  }
  if (pendingSaveAtMs && (int32_t)(millis() - pendingSaveAtMs) >= 0) {
    pendingSaveAtMs = 0;
    writeNvs("sinkron");
  }
  if (source != CLK_NONE && millis() - lastSaveMs >= SAVE_EVERY_MS) writeNvs("berkala");
}

void clock_save(const char *why) {
  if (source != CLK_NONE) writeNvs(why);
}

ClockSource clock_source() { return source; }

const char *clock_sourceName() {
  switch (source) {
    case CLK_SYNC:     return "Sinkron";
    case CLK_MANUAL:   return "Manual";
    case CLK_ESTIMATE: return "Perkiraan";
    default:           return "Belum";
  }
}

uint32_t clock_lastSync() { return lastSync; }

bool clock_setManual(int hour, int minute, int day, int month, int year) {
  static const uint8_t DAYS_IN[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
  int maxDay = month == 2 && !leap ? 28 : DAYS_IN[month - 1];
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || month < 1 || month > 12 || day < 1 || day > maxDay ||
      year < 2024 || year > 2099)
    return false;
  ble_setClockParts(0, minute, hour, day, month, year);
  source = CLK_MANUAL;
  writeNvs("manual");
  return true;
}
