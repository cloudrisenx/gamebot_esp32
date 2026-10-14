#include "ble_link.h"
#include "settings.h"
#include "ios_link.h"
#include "remote_hid.h"
#include "power.h"
#include <Preferences.h>

static ChronosESP32 watch(BLE_NAME);

static bool navScreenOpen = false;
static PhoneMode activeMode = PHONE_ANDROID;  // mode BLE yang sedang berjalan
static bool suspended = false;
static volatile bool connected = false;
static volatile bool navChanged = false;

// Antrean notifikasi mentah: diisi callback BLE (task NimBLE), dikosongkan di loop utama.
// Antrean (bukan satu buffer) karena WhatsApp sering mengirim pesan asli + ringkasannya
// hampir bersamaan; dengan satu buffer pesan asli tertimpa ringkasan.
struct RawNotif {
  int icon;
  char app[20];
  char title[40];
  char msg[NOTIF_MSG_LEN];
};
static const int QUEUE_LEN = 4;
static portMUX_TYPE notifMux = portMUX_INITIALIZER_UNLOCKED;
static RawNotif queue[QUEUE_LEN];
static volatile int qHead = 0, qCount = 0;

// Pop-up yang siap ditampilkan (hanya diakses dari loop utama)
static bool popupPending = false;
static String popupSender, popupMsg;

// Untuk membuang duplikat (pengirim + isi sama dalam DEDUP_MS)
static const uint32_t DEDUP_MS = 10000;
static String lastShownKey;
static uint32_t lastShownAt = 0;

// Diagnostik untuk layar Info
static int lastIcon = -1;
static String lastApp;

// Riwayat notifikasi (ring buffer, hanya diakses dari loop utama)
static const int HISTORY_LEN = 10;
static NotifEntry history[HISTORY_LEN];
static int histNewest = -1, histCount = 0;
static int unread = 0;

// Panggilan masuk: diisi callback BLE, dibaca loop utama
static volatile bool ringActive = false;
static char ringName[32];

static volatile bool phoneInfoChanged = false;
static volatile bool timeSynced = false;      // CF_TIME / CTS: jam baru dari HP (dibaca clockstore)
static volatile bool syncDone = false;        // CF_SYNCED: Chronos selesai sinkron (bisa tanpa kirim jam)
static volatile bool timeSeen = false;        // ada CF_TIME sejak ble_requestTimeSync() (diagnostik)
static volatile bool weatherChanged = false;  // CF_WEATHER: simpan cache bila isinya berubah

// Cache cuaca terakhir di NVS ("wcache"/"blob"): dipakai setelah restart/mati sampai Chronos kirim lagi
struct WeatherCache {
  uint32_t savedAt;  // epoch lokal saat disimpan (0 = jam belum valid)
  char city[24];
  char time[8];      // teks "Update" dari Chronos
  uint8_t count;
  Weather items[CS_WEATHER_SIZE];
};
static WeatherCache wcache = {};

static void onConnection(bool state) {
  connected = state;
  power_loopWake();
  navChanged = true;
  if (!state) ringActive = false;
}

void ble_setRinging(const char *name, bool on) {
  portENTER_CRITICAL(&notifMux);
  if (on && name) {
    strncpy(ringName, name, sizeof(ringName) - 1);
    ringName[sizeof(ringName) - 1] = '\0';
  }
  ringActive = on;
  portEXIT_CRITICAL(&notifMux);
}

static void onRinger(String caller, bool state) {
  ble_setRinging(caller.c_str(), state);
  power_loopWake();
}

// WhatsApp dikenali dari kode ikon Chronos (0x0A WhatsApp, 0x20 WhatsApp Business)
// atau nama aplikasinya, supaya tidak bergantung pada satu cara saja.
static bool isWhatsApp(int icon, const char *app) {
  return icon == 0x0A || icon == 0x20 || strncmp(app, "WhatsApp", 8) == 0;
}

static bool notifAccepted(int icon, const char *app) {
  switch (settings_notifMode()) {
    case NOTIF_ALL:      return true;
    case NOTIF_WHATSAPP: return isWhatsApp(icon, app);
    default:             return false;
  }
}

static void copyStr(char *dst, size_t len, const String &src) {
  strncpy(dst, src.c_str(), len - 1);
  dst[len - 1] = '\0';
}

static void copyCStr(char *dst, size_t len, const char *src) {
  strncpy(dst, src ? src : "", len - 1);
  dst[len - 1] = '\0';
}

// Masukkan notifikasi mentah ke antrean (Chronos: dari task BLE; iPhone: dari loop)
void ble_pushNotification(int icon, const char *app, const char *title, const char *msg) {
  portENTER_CRITICAL(&notifMux);
  if (qCount == QUEUE_LEN) {  // penuh: buang yang paling lama
    qHead = (qHead + 1) % QUEUE_LEN;
    qCount--;
  }
  RawNotif &r = queue[(qHead + qCount) % QUEUE_LEN];
  r.icon = icon;
  copyCStr(r.app, sizeof(r.app), app);
  copyCStr(r.title, sizeof(r.title), title);
  copyCStr(r.msg, sizeof(r.msg), msg);
  qCount++;
  portEXIT_CRITICAL(&notifMux);
  power_loopWake();
}

static void onNotification(Notification n) {
  ble_pushNotification(n.icon, n.app.c_str(), n.title.c_str(), n.message.c_str());
}

static String lower(String s) {
  s.toLowerCase();
  return s;
}

// Notifikasi ringkasan WhatsApp/Android, bukan pesan seseorang:
// "36 pesan dari 5 chat", "3 pesan baru", "12 new messages", "Memeriksa pesan baru", ...
static bool isSummary(const String &title, const String &msg, const String &app) {
  String m = lower(msg);
  if (m.indexOf("memeriksa pesan") >= 0 || m.indexOf("checking for new messages") >= 0) return true;
  // Seluruh isi hanya "N pesan (baru)" / "N (new) messages", atau "N pesan dari ..." / "N messages from ..."
  // (dibuat ketat supaya pesan asli seperti "5 chat lagi ya" tidak ikut terbuang)
  int i = 0;
  while (i < (int)m.length() && isDigit(m[i])) i++;
  if (i > 0 && i < (int)m.length() && m[i] == ' ') {
    String rest = m.substring(i + 1);
    if (rest == "pesan" || rest == "pesan baru" || rest == "message" || rest == "messages" ||
        rest == "new message" || rest == "new messages" || rest.startsWith("pesan dari ") ||
        rest.startsWith("pesan baru dari ") || rest.startsWith("messages from ") ||
        rest.startsWith("new messages from "))
      return true;
  }
  // Judul = nama aplikasi (tidak ada nama pengirim) dan isinya menyebut jumlah pesan/chat
  if (lower(title) == lower(app) && i > 0 &&
      (m.indexOf("pesan") >= 0 || m.indexOf("message") >= 0 || m.indexOf("chat") >= 0))
    return true;
  return false;
}

// "Budi (3 pesan)" / "Budi (3 messages)" -> "Budi"
static String stripCount(String title) {
  title.trim();
  int open = title.lastIndexOf('(');
  if (open > 0 && title.endsWith(")") && open + 1 < (int)title.length() && isDigit(title[open + 1])) {
    title = title.substring(0, open);
    title.trim();
  }
  return title;
}

static void addHistory(const String &app, const String &title, const String &msg) {
  histNewest = (histNewest + 1) % HISTORY_LEN;
  NotifEntry &e = history[histNewest];
  copyStr(e.app, sizeof(e.app), app);
  copyStr(e.title, sizeof(e.title), title);
  copyStr(e.msg, sizeof(e.msg), msg);
  if (!ble_timeStr(e.time, sizeof(e.time))) strcpy(e.time, "--:--");
  if (histCount < HISTORY_LEN) histCount++;
  unread++;
}

// Proses antrean di loop utama: log, filter aplikasi, buang ringkasan & duplikat
static void processQueue() {
  while (true) {
    RawNotif r;
    portENTER_CRITICAL(&notifMux);
    bool have = qCount > 0;
    if (have) {
      r = queue[qHead];
      qHead = (qHead + 1) % QUEUE_LEN;
      qCount--;
    }
    portEXIT_CRITICAL(&notifMux);
    if (!have) return;

    lastIcon = r.icon;
    lastApp = r.app;
    String app = r.app, title = stripCount(r.title), msg = r.msg;
    msg.trim();
    if (!title.length()) title = app;  // judul kosong: pakai nama aplikasi

    const char *verdict;
    if (!notifAccepted(r.icon, r.app)) {
      verdict = settings_notifMode() == NOTIF_OFF ? "diabaikan (pop-up OFF)" : "diabaikan (bukan WhatsApp)";
    } else if (isSummary(title, msg, app)) {
      verdict = "diabaikan (ringkasan)";
    } else if (title + "\n" + msg == lastShownKey && millis() - lastShownAt < DEDUP_MS) {
      verdict = "diabaikan (duplikat)";
    } else {
      verdict = "POP-UP";
      popupSender = title;
      popupMsg = msg;
      popupPending = true;
      addHistory(app, title, msg);
      lastShownKey = title + "\n" + msg;
      lastShownAt = millis();
    }
    // len = panjang pesan yang benar-benar diterima (sesudah dipotong NOTIF_MSG_LEN-1)
    Serial.printf("[NOTIF] app=%s icon=0x%02X title=\"%s\" len=%u msg=\"%.40s\" -> %s\n",
                  r.app, r.icon, r.title, (unsigned)strlen(r.msg), r.msg, verdict);
  }
}

static void onConfig(Config config, uint32_t a, uint32_t b) {
  if (config == CF_NAV_DATA || config == CF_NAV_ICON) navChanged = true;
  if (config == CF_MUSIC || config == CF_WEATHER) phoneInfoChanged = true;
  if (config == CF_WEATHER) weatherChanged = true;
  if (config == CF_TIME && a == 1) timeSynced = timeSeen = true;  // a = 1: sesudah jam disetel
  if (config == CF_SYNCED) syncDone = true;
}

static void loadWeatherCache() {
  Preferences p;
  if (!p.begin("wcache", true)) return;
  if (p.getBytesLength("blob") == sizeof(wcache)) p.getBytes("blob", &wcache, sizeof(wcache));
  p.end();
  if (wcache.count > CS_WEATHER_SIZE) wcache.count = 0;
  Serial.printf("[CUACA] cache: %u hari, kota \"%s\"\n", wcache.count, wcache.city);
}

// Salin cuaca Chronos ke cache; tulis NVS hanya bila isinya berbeda
static void saveWeatherCache() {
  int n = min(watch.getWeatherCount(), CS_WEATHER_SIZE);
  if (n <= 0) return;
  WeatherCache c = {};
  c.savedAt = wcache.savedAt;
  strncpy(c.city, watch.getWeatherCity().c_str(), sizeof(c.city) - 1);
  strncpy(c.time, watch.getWeatherTime().c_str(), sizeof(c.time) - 1);
  c.count = n;
  for (int i = 0; i < n; i++) c.items[i] = watch.getWeatherAt(i);
  uint32_t now = watch.getEpoch() >= 1704067200UL ? watch.getEpoch() : 0;
  // Isi sama: tulis ulang hanya untuk memperbarui umur, maks 1x/jam (hemat flash)
  if (memcmp(&c, &wcache, sizeof(c)) == 0 && (!now || now - wcache.savedAt < 3600)) return;
  c.savedAt = now;
  wcache = c;
  Preferences p;
  if (p.begin("wcache", false)) { p.putBytes("blob", &wcache, sizeof(wcache)); p.end(); }
  Serial.printf("[CUACA] cache disimpan: %d hari, %dC, kota \"%s\"\n", n, c.items[0].temp, c.city);
}

void ble_begin() {
  remote_recoverBonds();  // reset/tidur saat mode Remote: kembalikan bond mode biasa sebelum BLE mulai
  watch.setConnectionCallback(onConnection);
  watch.setNotificationCallback(onNotification);
  watch.setConfigurationCallback(onConfig);
  watch.setRingerCallback(onRinger);
  loadWeatherCache();
}

// Hemat daya BLE: daya pancar 0 dBm (cukup untuk HP di dekat badan) dan interval advertising
// 500-1000 ms saat belum terhubung (menyambung ulang sedikit lebih lambat, ±1 dtk).
static const int8_t BLE_TX_DBM = 0;
static const uint16_t ADV_MIN_UNITS = 800;   // 800 x 0,625 ms = 500 ms
static const uint16_t ADV_MAX_UNITS = 1600;  // 1600 x 0,625 ms = 1000 ms

static void tuneBlePower() {
  if (!NimBLEDevice::setPower(BLE_TX_DBM)) Serial.println("BLE: setPower gagal");
  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  adv->stop();  // interval baru berlaku setelah advertising dimulai ulang
  adv->setMinInterval(ADV_MIN_UNITS);
  adv->setMaxInterval(ADV_MAX_UNITS);
  adv->start();
  Serial.printf("BLE: daya %d dBm, advertising %u-%u ms\n", NimBLEDevice::getPower(),
                ADV_MIN_UNITS * 5 / 8, ADV_MAX_UNITS * 5 / 8);
}

static void stopAll() {
  if (watch.isRunning()) watch.stop();
  ios_stop();
  connected = false;
  ble_setRinging(nullptr, false);
}

void ble_update() {
  bool wanted = !suspended && (settings_bt() || navScreenOpen);
  PhoneMode mode = settings_phoneMode();
  if (mode != activeMode) {  // Mode HP diganti di Setting: hentikan BLE lama dulu
    stopAll();
    activeMode = mode;
    phoneInfoChanged = navChanged = true;
  }
  if (activeMode == PHONE_IPHONE) {
    if (wanted && !ios_running()) ios_begin();
    else if (!wanted && ios_running()) ios_stop();
    ios_loop();
  } else {
    if (wanted && !watch.isRunning()) {
      watch.setName(settings_bleName());
      watch.begin();
      tuneBlePower();
    } else if (!wanted && watch.isRunning()) {
      watch.stop();
      connected = false;
    }
    watch.loop();
  }
  if (weatherChanged) {
    weatherChanged = false;
    saveWeatherCache();
  }
  processQueue();
}

String ble_lastNotifInfo() {
  if (lastIcon < 0) return "Notif: belum ada";
  char buf[32];
  snprintf(buf, sizeof(buf), "Notif: %s 0x%02X", lastApp.c_str(), lastIcon);
  return String(buf);
}

void ble_setNavScreen(bool open) { navScreenOpen = open; }

void ble_shutdown() { stopAll(); }

void ble_setSuspended(bool s) {
  suspended = s;
  if (s) ble_shutdown();
}

void ble_remoteStart() {
  ble_setSuspended(true);  // Chronos/ANCS berhenti (deinit stack); bond di NVS tetap
  remote_begin();
}

void ble_remoteStop() {
  remote_end();
  remote_recoverBonds();    // pertukaran bond gagal saat keluar: coba lagi sekarang (no-op bila sudah benar)
  ble_setSuspended(false);  // ble_update() menyalakan lagi mode HP sesuai setting
}

bool ble_isIphone() { return activeMode == PHONE_IPHONE; }

bool ble_running() { return ble_isIphone() ? ios_running() : watch.isRunning(); }

bool ble_connected() { return ble_isIphone() ? ios_connected() : watch.isRunning() && connected; }

String ble_address() {
  if (ble_isIphone()) return ios_address();
  return watch.isRunning() ? watch.getAddress() : String();
}

uint8_t ble_phoneBattery() {
  if (ble_isIphone()) return ios_battery();
  return ble_connected() ? watch.getPhoneBattery() : 0;
}

// Jam sistem dipakai kedua mode; iPhone menyetelnya lewat CTS dengan cara yang sama seperti Chronos
void ble_setLocalTime(int sec, int min, int hour, int day, int month, int year) {
  watch.setTime(sec, min, hour, day, month, year);
  timeSynced = true;  // iPhone (CTS): sama dengan sinkron Chronos
}

void ble_setClock(uint32_t epoch) { watch.setTime(epoch, 0); }

void ble_setClockParts(int sec, int min, int hour, int day, int month, int year) {
  watch.setTime(sec, min, hour, day, month, year);
}

uint32_t ble_epoch() { return watch.getEpoch(); }

// Parameter koneksi (unit: interval 1,25 ms, timeout 10 ms). Memenuhi aturan Apple: interval kelipatan 15 ms,
// min + 15 <= max, latency <= 30, timeout 2-6 dtk, max x (latency + 1) <= 2 dtk.
static const uint16_t SAVE_MIN = 48, SAVE_MAX = 96, SAVE_LAT = 4, SAVE_TO = 600;  // 60-120 ms, lewati 4, 6 dtk
static const uint16_t FAST_MIN = 12, FAST_MAX = 24, FAST_LAT = 0, FAST_TO = 400;  // 15-30 ms, 4 dtk
static const uint32_t CONN_PARAM_GAP_MS = 5000;  // jeda antar-permintaan & sesudah tersambung (sinkron awal)
static int8_t linkMode = -1;                     // -1 = belum diminta di koneksi ini, 0 = cepat, 1 = hemat
static uint32_t linkModeAt = 0;
static bool linkWasConnected = false;

void ble_setLowPowerLink(bool saving) {
  bool conn = ble_connected();
  uint32_t now = millis();
  if (conn != linkWasConnected) {  // koneksi baru / putus: mulai lagi, tunggu sinkron awal selesai
    linkWasConnected = conn;
    linkMode = -1;
    linkModeAt = now;
  }
  if (!conn || linkMode == (int8_t)saving || now - linkModeAt < CONN_PARAM_GAP_MS) return;
  uint16_t mn = saving ? SAVE_MIN : FAST_MIN, mx = saving ? SAVE_MAX : FAST_MAX;
  uint16_t lat = saving ? SAVE_LAT : FAST_LAT, to = saving ? SAVE_TO : FAST_TO;
  bool sent = false;
  if (ble_isIphone()) {
    sent = ios_updateConnParams(mn, mx, lat, to);
  } else if (NimBLEServer *s = NimBLEDevice::getServer()) {
    if (s->getConnectedCount() > 0) {
      s->updateConnParams(s->getPeerInfo(0).getConnHandle(), mn, mx, lat, to);
      sent = true;
    }
  }
  linkMode = saving;
  linkModeAt = now;
  if (sent) Serial.printf("[BLE] conn param: %s (itv %u-%u ms, lat %u)\n", saving ? "hemat" : "cepat",
                          mn * 5 / 4, mx * 5 / 4, lat);
}

bool ble_takeSyncDone(bool &withTime) {
  if (!syncDone) return false;
  syncDone = false;
  withTime = timeSeen;
  return true;
}

bool ble_requestTimeSync() {
  syncDone = false;
  timeSeen = false;
  bool ok;
  if (ble_isIphone()) ok = ios_readTime();
  else {
    ok = ble_connected();
    if (ok) watch.syncRequest();  // Chronos mengirim ulang jam (CF_TIME) & data lain
  }
  Serial.printf("[JAM] minta sinkron HP: %s\n", ok ? "ok" : "tidak tersambung");
  return ok;
}

bool ble_takeTimeSynced() {
  bool s = timeSynced;
  timeSynced = false;
  return s;
}

void ble_restartRadio() {
  // Sama dengan saat Mode HP diganti: stack dimatikan (deinit), ble_update() menyalakan lagi.
  // Bond tersimpan di NVS NimBLE, tidak terhapus oleh deinit.
  Serial.println("[BT] restart modul Bluetooth");
  stopAll();
}

void ble_markPhoneInfoChanged() { phoneInfoChanged = true; }

bool ble_timeStr(char *buf, size_t len) {
  // Jam sistem tetap berjalan saat deep sleep; anggap belum sinkron jika masih sebelum 2024
  if (watch.getEpoch() < 1704067200UL) return false;
  snprintf(buf, len, "%02d:%02d", watch.getHour(true), watch.getMinute());
  return true;
}

bool ble_date(int &wday, int &day, int &month) {
  if (watch.getEpoch() < 1704067200UL) return false;  // sama dengan ble_timeStr: belum sinkron
  wday = watch.getDayofWeek();
  day = watch.getDay();
  month = watch.getMonth();
  return true;
}

bool ble_navActive() { return !ble_isIphone() && ble_connected() && watch.getNavigation().active; }

bool ble_navChanged() {
  bool c = navChanged;
  navChanged = false;
  return c;
}

Navigation &ble_nav() { return watch.getNavigation(); }

int ble_historyCount() { return histCount; }
int ble_unreadCount() { return unread; }
void ble_markRead() { unread = 0; }

const NotifEntry &ble_historyAt(int index) {
  return history[(histNewest - index + HISTORY_LEN) % HISTORY_LEN];
}

void ble_historyDelete(int index) {
  if (index < 0 || index >= histCount) return;
  // Geser entri yang lebih lama satu langkah ke arah terbaru, lalu buang slot tertua
  for (int i = index; i < histCount - 1; i++)
    history[(histNewest - i + HISTORY_LEN) % HISTORY_LEN] = history[(histNewest - i - 1 + HISTORY_LEN) % HISTORY_LEN];
  histCount--;
  if (index < unread) unread--;  // yang belum dibaca selalu entri paling baru
  unread = min(unread, histCount);
}

bool ble_ringing(char *name, size_t len) {
  portENTER_CRITICAL(&notifMux);
  bool on = ringActive && ble_connected();
  if (on) {
    strncpy(name, ringName, len - 1);
    name[len - 1] = '\0';
  }
  portEXIT_CRITICAL(&notifMux);
  return on;
}

MusicInfo &ble_music() { return ble_isIphone() ? ios_music() : watch.getMusicInfo(); }

void ble_musicControl(Control cmd) {
  if (ble_isIphone()) ios_musicControl(cmd);
  else if (ble_connected()) watch.musicControl(cmd);
}

bool ble_musicProgress(uint32_t &elapsedMs, uint32_t &durationMs) {
  return ble_isIphone() && ios_musicProgress(elapsedMs, durationMs);
}

void ble_callAnswer(bool accept) { if (ble_isIphone()) ios_callAction(accept); }

bool ble_deletePairings() { return ios_deleteBonds(); }

// Fitur khusus Android (Chronos): di mode iPhone tidak tersedia
// Tanpa data dari Chronos (mis. setelah restart): pakai cache terakhir
bool ble_weatherIsCache() { return !ble_isIphone() && watch.getWeatherCount() == 0 && wcache.count > 0; }
int ble_weatherCount() {
  if (ble_isIphone()) return 0;
  return ble_weatherIsCache() ? wcache.count : watch.getWeatherCount();
}
Weather &ble_weatherAt(int index) {
  return ble_weatherIsCache() ? wcache.items[constrain(index, 0, CS_WEATHER_SIZE - 1)] : watch.getWeatherAt(index);
}
String ble_weatherCity() { return ble_weatherIsCache() ? String(wcache.city) : watch.getWeatherCity(); }
String ble_weatherTime() { return ble_weatherIsCache() ? String(wcache.time) : watch.getWeatherTime(); }

// Umur cache dalam jam (-1 = tidak diketahui: jam belum valid saat disimpan / sekarang)
int ble_weatherCacheAgeH() {
  uint32_t now = watch.getEpoch();
  if (!wcache.savedAt || now < 1704067200UL || now < wcache.savedAt) return -1;
  return (now - wcache.savedAt) / 3600;
}

bool ble_phoneInfoChanged() {
  bool c = phoneInfoChanged;
  phoneInfoChanged = false;
  return c;
}

bool ble_takeWaNotif(char *sender, size_t senderLen, char *msg, size_t msgLen) {
  if (!popupPending) return false;
  copyStr(sender, senderLen, popupSender);
  copyStr(msg, msgLen, popupMsg);
  popupPending = false;
  return true;
}
