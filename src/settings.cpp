#include "settings.h"
#include <Preferences.h>
#include "version.h"

static const uint16_t SCREEN_SEC[SCREEN_OPT_COUNT] = {15, 30, 60, 0};
// Indeks 3 dulu bernama "OFF". Deep sleep mematikan Bluetooth (WA tidak bisa membangunkan jam) dan
// light sleep otomatis tidak tersedia di core Arduino ini (CONFIG_PM_ENABLE tidak aktif), jadi
// "Jaga notif" = tanpa deep sleep: BLE tetap tersambung, layar tetap mati sendiri.
static const uint8_t SLEEP_MIN[SLEEP_OPT_COUNT] = {2, 5, 10, 0};
static const char *const SLEEP_NAMES[SLEEP_OPT_COUNT] = {"2mnt", "5mnt", "10mnt", "Jaga notif"};
static const uint8_t SLEEP_DEFAULT = 3;  // Jaga notif
// Jadwal malam: menit sejak 00:00. Key lama "night" (indeks preset OFF/22-06/23-06/00-06) hanya dibaca
// sekali untuk migrasi ke "nightOn"/"nightStart"/"nightEnd"; tidak dihapus.
static const uint16_t LEGACY_NIGHT_START[4] = {22 * 60, 22 * 60, 23 * 60, 0};
static bool nightOn = false;
static uint16_t nightStart = 22 * 60, nightEnd = 6 * 60;
static const uint8_t CONTRAST[CONTRAST_OPT_COUNT] = {0x10, 0x7F, 0xFF};

static Preferences prefs;
static NotifMode notifMode = NOTIF_WHATSAPP;
static PhoneMode phoneMode = PHONE_ANDROID;
static bool bt = true;
static const char *HI_KEYS[GAME_COUNT] = {"hi_brk", "hi_dino", "hi_hitung", "hi_flap", "hi_pong", "hi_duel", "hi_ttt"};
static uint32_t hiscore[GAME_COUNT];
static uint8_t screenIdx = 1;    // 30s
static uint8_t sleepIdx = SLEEP_DEFAULT;
static uint8_t contrastIdx = 1;  // Sedang
static const uint32_t I2C_HZ[I2C_OPT_COUNT] = {800000, 400000, 200000};
static uint8_t i2cIdx = 0;  // 800 kHz (sama dengan sebelum ada setting)
// Waktu Lv5 disimpan sebagai ms di "hitLv5ms". Key lama "hitLv5" (indeks 0..3 = 1,5/1,2/1,0/0,8 dtk)
// hanya dibaca sekali untuk migrasi; tidak dihapus.
static const uint16_t HITUNG_LV5_MS[HITUNG_LV5_OPT_COUNT] = {2000, 1500, 1200, 1000, 800};
static const char *const HITUNG_LV5_NAMES[HITUNG_LV5_OPT_COUNT] = {"2,0s", "1,5s", "1,2s", "1,0s", "0,8s"};
static const uint16_t LEGACY_LV5_MS[4] = {1500, 1200, 1000, 800};
static uint8_t hitungLv5Idx = 2;  // 1,2s
static const char *const HITUNG_DIST_NAMES[HITUNG_DIST_OPT_COUNT] = {"Tanpa", "Sedikit", "Sedang", "Banyak"};
static uint8_t hitungDistIdx = 2;  // Sedang
static bool bootAnim = true;
static String devName, bleName, otaSsid;  // kosong = nama bawaan (DEVICE_NAME)

void settings_begin() {
  prefs.begin("cfg", false);
  // Migrasi dari setting lama "wa" (ON/OFF)
  uint8_t legacy = prefs.getBool("wa", true) ? NOTIF_WHATSAPP : NOTIF_OFF;
  notifMode = (NotifMode)(prefs.getUChar("nmode", legacy) % NOTIF_MODE_COUNT);
  phoneMode = (PhoneMode)(prefs.getUChar("phone", PHONE_ANDROID) % PHONE_MODE_COUNT);
  bt = prefs.getBool("bt", true);
  if (prefs.isKey("hi")) prefs.remove("hi");  // high score Flappy Bird (game sudah dihapus)
  for (int g = 0; g < GAME_COUNT; g++) hiscore[g] = prefs.getUInt(HI_KEYS[g], 0);
  screenIdx = prefs.getUChar("scr", 1) % SCREEN_OPT_COUNT;
  sleepIdx = prefs.getUChar("slp", SLEEP_DEFAULT) % SLEEP_OPT_COUNT;
  if (prefs.isKey("nightOn")) {
    nightOn = prefs.getBool("nightOn", false);
    nightStart = prefs.getUShort("nightStart", 22 * 60) % (24 * 60);
    nightEnd = prefs.getUShort("nightEnd", 6 * 60) % (24 * 60);
  } else {
    uint8_t legacy = prefs.getUChar("night", 0) % 4;
    nightOn = legacy != 0;
    nightStart = LEGACY_NIGHT_START[legacy];
    nightEnd = 6 * 60;
    prefs.putBool("nightOn", nightOn);
    prefs.putUShort("nightStart", nightStart);
    prefs.putUShort("nightEnd", nightEnd);
    Serial.printf("[POWER] migrasi jadwal malam lama (indeks %u)\n", legacy);
  }
  if (nightStart == nightEnd) nightOn = false;
  contrastIdx = prefs.getUChar("con", 1) % CONTRAST_OPT_COUNT;
  i2cIdx = prefs.getUChar("i2cHz", 0) % I2C_OPT_COUNT;
  uint16_t lv5Ms = 1200;
  if (prefs.isKey("hitLv5ms")) {
    lv5Ms = prefs.getUShort("hitLv5ms", 1200);
  } else {
    if (prefs.isKey("hitLv5")) lv5Ms = LEGACY_LV5_MS[prefs.getUChar("hitLv5", 2) % 4];
    prefs.putUShort("hitLv5ms", lv5Ms);
    Serial.printf("[HITUNG] migrasi waktu Lv5 -> %u ms\n", lv5Ms);
  }
  hitungLv5Idx = 2;
  for (uint8_t i = 0; i < HITUNG_LV5_OPT_COUNT; i++)
    if (HITUNG_LV5_MS[i] == lv5Ms) hitungLv5Idx = i;
  hitungDistIdx = prefs.getUChar("hitDist", 2) % HITUNG_DIST_OPT_COUNT;
  bootAnim = prefs.getBool("anim", true);
  devName = prefs.getString("devName", "");
  bleName = prefs.getString("bleName", "");
  otaSsid = prefs.getString("ssid", "");
}

NotifMode settings_notifMode() { return notifMode; }

PhoneMode settings_phoneMode() { return phoneMode; }

void settings_setPhoneMode(PhoneMode m) {
  phoneMode = (PhoneMode)(m % PHONE_MODE_COUNT);
  prefs.putUChar("phone", phoneMode);
}

void settings_setNotifMode(NotifMode m) {
  notifMode = (NotifMode)(m % NOTIF_MODE_COUNT);
  prefs.putUChar("nmode", notifMode);
}

bool settings_bt() { return bt; }

void settings_setBt(bool on) {
  bt = on;
  prefs.putBool("bt", on);
}

uint32_t settings_hiscore(GameId g) { return hiscore[g]; }

void settings_setHiscore(GameId g, uint32_t score) {
  hiscore[g] = score;
  prefs.putUInt(HI_KEYS[g], score);
}

void settings_resetHiscores() {
  for (int g = 0; g < GAME_COUNT; g++) settings_setHiscore((GameId)g, 0);
}

uint8_t settings_screenIdx() { return screenIdx; }

void settings_setScreenIdx(uint8_t idx) {
  screenIdx = idx % SCREEN_OPT_COUNT;
  prefs.putUChar("scr", screenIdx);
}

uint32_t settings_screenTimeoutMs() { return SCREEN_SEC[screenIdx] * 1000UL; }

uint8_t settings_sleepIdx() { return sleepIdx; }

void settings_setSleepIdx(uint8_t idx) {
  sleepIdx = idx % SLEEP_OPT_COUNT;
  prefs.putUChar("slp", sleepIdx);
  Serial.printf("[POWER] mode tidur=%s\n", SLEEP_NAMES[sleepIdx]);
}

uint32_t settings_sleepTimeoutMs() { return SLEEP_MIN[sleepIdx] * 60000UL; }

const char *settings_sleepName() { return SLEEP_NAMES[sleepIdx]; }

bool settings_nightOn() { return nightOn; }
uint16_t settings_nightStartMin() { return nightStart; }
uint16_t settings_nightEndMin() { return nightEnd; }

bool settings_setNight(bool on, uint16_t startMin, uint16_t endMin) {
  startMin %= 24 * 60;
  endMin %= 24 * 60;
  if (on && startMin == endMin) return false;
  nightOn = on;
  nightStart = startMin;
  nightEnd = endMin;
  prefs.putBool("nightOn", nightOn);
  prefs.putUShort("nightStart", nightStart);
  prefs.putUShort("nightEnd", nightEnd);
  char buf[12];
  settings_nightLabel(buf, sizeof(buf));
  Serial.printf("[POWER] jadwal malam=%s\n", buf);
  return true;
}

void settings_nightLabel(char *buf, size_t len) {
  if (!nightOn) snprintf(buf, len, "OFF");
  else snprintf(buf, len, "%02u:%02u-%02u:%02u", nightStart / 60, nightStart % 60, nightEnd / 60, nightEnd % 60);
}

uint8_t settings_hitungLv5Idx() { return hitungLv5Idx; }
void settings_setHitungLv5Idx(uint8_t idx) {
  hitungLv5Idx = idx % HITUNG_LV5_OPT_COUNT;
  prefs.putUShort("hitLv5ms", HITUNG_LV5_MS[hitungLv5Idx]);
}
uint16_t settings_hitungLv5Ms() { return HITUNG_LV5_MS[hitungLv5Idx]; }
uint16_t settings_hitungLv5MsAt(uint8_t idx) { return HITUNG_LV5_MS[idx % HITUNG_LV5_OPT_COUNT]; }
const char *settings_hitungLv5Name() { return HITUNG_LV5_NAMES[hitungLv5Idx]; }
const char *settings_hitungLv5NameAt(uint8_t idx) { return HITUNG_LV5_NAMES[idx % HITUNG_LV5_OPT_COUNT]; }

uint8_t settings_hitungDistIdx() { return hitungDistIdx; }
void settings_setHitungDistIdx(uint8_t idx) {
  hitungDistIdx = idx % HITUNG_DIST_OPT_COUNT;
  prefs.putUChar("hitDist", hitungDistIdx);
}
const char *settings_hitungDistNameAt(uint8_t idx) { return HITUNG_DIST_NAMES[idx % HITUNG_DIST_OPT_COUNT]; }

uint8_t settings_i2cIdx() { return i2cIdx; }
void settings_setI2cIdx(uint8_t idx) {
  i2cIdx = idx % I2C_OPT_COUNT;
  prefs.putUChar("i2cHz", i2cIdx);
}
uint32_t settings_i2cHz() { return I2C_HZ[i2cIdx]; }

uint8_t settings_contrastIdx() { return contrastIdx; }

void settings_setContrastIdx(uint8_t idx) {
  contrastIdx = idx % CONTRAST_OPT_COUNT;
  prefs.putUChar("con", contrastIdx);
}

uint8_t settings_contrastValue() { return CONTRAST[contrastIdx]; }

bool settings_bootAnim() { return bootAnim; }

void settings_setBootAnim(bool on) {
  bootAnim = on;
  prefs.putBool("anim", on);
}

String settings_otaPassword() {
  String pw = prefs.getString("otapw", "");
  if (pw.length() < 8 || pw.length() > 63) {
    static const char CHARS[] = "abcdefghjkmnpqrstuvwxyz23456789";  // tanpa i/l/o/0/1
    pw = "";
    for (int i = 0; i < 8; i++) pw += CHARS[esp_random() % (sizeof(CHARS) - 1)];
    prefs.putString("otapw", pw);
  }
  return pw;
}

String settings_deviceName() { return devName.length() ? devName : String(DEVICE_NAME); }
String settings_bleName() { return bleName.length() ? bleName : String(DEVICE_NAME); }
String settings_otaSsid() { return otaSsid.length() ? otaSsid : String(DEVICE_NAME "_OTA"); }

// Hanya ASCII cetak 0x20..0x7E, panjang min..max
static bool asciiOk(const String &s, unsigned minLen, unsigned maxLen) {
  if (s.length() < minLen || s.length() > maxLen) return false;
  for (unsigned i = 0; i < s.length(); i++)
    if (s[i] < 0x20 || s[i] > 0x7E) return false;
  return true;
}

bool settings_setNames(const String &dev, const String &ble, const String &ssid, const String &pw, String &err) {
  // Semua divalidasi dulu: tidak ada yang tersimpan sebagian
  if (dev.length() && !asciiOk(dev, 1, NAME_MAX_LEN)) err = "Nama device: 1-20 karakter (huruf/angka/tanda baca biasa)";
  else if (ble.length() && !asciiOk(ble, 1, NAME_MAX_LEN)) err = "Nama Bluetooth: 1-20 karakter (huruf/angka/tanda baca biasa)";
  else if (ssid.length() && !asciiOk(ssid, 1, 32)) err = "Nama WiFi: 1-32 karakter (huruf/angka/tanda baca biasa)";
  else if (pw.length() && !asciiOk(pw, 8, 63)) err = "Password WiFi: 8-63 karakter (huruf/angka/tanda baca biasa)";
  else err = "";
  if (err.length()) return false;
  devName = dev;
  bleName = ble;
  otaSsid = ssid;
  prefs.putString("devName", dev);
  prefs.putString("bleName", ble);
  prefs.putString("ssid", ssid);
  if (pw.length()) prefs.putString("otapw", pw);
  return true;
}
