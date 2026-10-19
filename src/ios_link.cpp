#include "ios_link.h"
#include <NimBLEDevice.h>
#include <esp_task_wdt.h>
#include "ble_link.h"
#include "power.h"
#include "settings.h"

// =====================================================================================
// UUID & konstanta layanan Apple
//   ANCS: Apple Notification Center Service Specification
//   AMS : Apple Media Service Specification
//   CTS : Bluetooth SIG Current Time Service (0x1805 / 0x2A2B)
// =====================================================================================
static const char *ANCS_SERVICE = "7905F431-B5CE-4E99-A40F-4B1E122D00D0";
static const char *ANCS_NOTIFICATION_SOURCE = "9FBF120D-6301-42D9-8C58-25E699A21DBD";
static const char *ANCS_CONTROL_POINT = "69D1D8F3-45E1-49A8-9821-9BBDFDAAD9D9";
static const char *ANCS_DATA_SOURCE = "22EAC6E9-24D6-4BB5-BE44-B36ACE7C7BFB";

static const char *AMS_SERVICE = "89D3502B-0F36-433A-8EF4-C502AD55F8DC";
static const char *AMS_REMOTE_COMMAND = "9B3C81D8-57B1-4A8A-B8DF-0E56F7CA51C2";
static const char *AMS_ENTITY_UPDATE = "2F7CABCE-808D-411F-9A0C-BB92BA96C102";

static const uint16_t CTS_SERVICE = 0x1805;
static const uint16_t CTS_CURRENT_TIME = 0x2A2B;
static const uint16_t BATTERY_SERVICE = 0x180F;
static const uint16_t BATTERY_LEVEL = 0x2A19;

// ANCS
static const uint8_t ANCS_EVENT_ADDED = 0, ANCS_EVENT_REMOVED = 2;
static const uint8_t ANCS_FLAG_PRE_EXISTING = 1 << 2;
static const uint8_t ANCS_CAT_INCOMING_CALL = 1;
static const uint8_t ANCS_CMD_GET_NOTIF_ATTRIBUTES = 0, ANCS_CMD_PERFORM_ACTION = 2;
static const uint8_t ANCS_ATTR_APP_ID = 0, ANCS_ATTR_TITLE = 1, ANCS_ATTR_MESSAGE = 3;
static const uint8_t ANCS_ACTION_POSITIVE = 0, ANCS_ACTION_NEGATIVE = 1;
static const uint16_t ANCS_TITLE_MAX = 31, ANCS_MESSAGE_MAX = NOTIF_MSG_LEN - 1;

// AMS
static const uint8_t AMS_ENTITY_PLAYER = 0, AMS_ENTITY_TRACK = 2;
static const uint8_t AMS_PLAYER_NAME = 0, AMS_PLAYER_PLAYBACK_INFO = 1;
static const uint8_t AMS_TRACK_ARTIST = 0, AMS_TRACK_TITLE = 2, AMS_TRACK_DURATION = 3;
static const uint8_t AMS_CMD_PLAY = 0, AMS_CMD_PAUSE = 1, AMS_CMD_TOGGLE = 2, AMS_CMD_NEXT = 3,
                     AMS_CMD_PREVIOUS = 4, AMS_CMD_VOLUME_UP = 5, AMS_CMD_VOLUME_DOWN = 6;

static const int8_t TX_DBM = 0;
static const uint16_t ADV_MIN_UNITS = 800, ADV_MAX_UNITS = 1600;  // 500-1000 ms (x 0,625 ms)
static const uint16_t APPEARANCE_GENERIC_WATCH = 0x00C0;
static const uint32_t ATTR_REQUEST_TIMEOUT_MS = 3000;
static const uint32_t NO_UID = 0xFFFFFFFF;

// =====================================================================================
// Parser murni
// =====================================================================================
static uint32_t le32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

bool ancs_parseNotificationSource(const uint8_t *d, size_t n, AncsEvent &e) {
  if (n < 8) return false;
  e.eventId = d[0];
  e.flags = d[1];
  e.category = d[2];
  // d[3] = CategoryCount
  e.uid = le32(d + 4);
  return true;
}

static void copyAttr(char *dst, size_t dstLen, const uint8_t *src, size_t len) {
  if (!dst || !dstLen) return;
  size_t n = min(len, dstLen - 1);
  memcpy(dst, src, n);
  dst[n] = '\0';
}

int ancs_parseAttributes(const uint8_t *d, size_t n, uint32_t &uid, char *appId, size_t appLen,
                         char *title, size_t titleLen, char *msg, size_t msgLen) {
  if (n < 1) return 0;
  if (d[0] != ANCS_CMD_GET_NOTIF_ATTRIBUTES) return -1;
  if (n < 5) return 0;
  uid = le32(d + 1);
  size_t pos = 5;
  const int expected = 3;  // AppIdentifier, Title, Message (urutan sesuai permintaan)
  for (int i = 0; i < expected; i++) {
    if (pos + 3 > n) return 0;
    uint8_t id = d[pos];
    size_t len = d[pos + 1] | (d[pos + 2] << 8);
    if (pos + 3 + len > n) return 0;
    const uint8_t *val = d + pos + 3;
    if (id == ANCS_ATTR_APP_ID) copyAttr(appId, appLen, val, len);
    else if (id == ANCS_ATTR_TITLE) copyAttr(title, titleLen, val, len);
    else if (id == ANCS_ATTR_MESSAGE) copyAttr(msg, msgLen, val, len);
    else return -1;
    pos += 3 + len;
  }
  return (int)pos;
}

bool ams_parsePlaybackInfo(const char *s, int &state, float &rate, float &elapsed) {
  char *end;
  long st = strtol(s, &end, 10);
  if (end == s || *end != ',') return false;
  const char *p = end + 1;
  float r = strtof(p, &end);
  if (end == p || *end != ',') return false;
  p = end + 1;
  float el = strtof(p, &end);
  if (end == p) return false;
  state = (int)st;
  rate = r;
  elapsed = el;
  return true;
}

bool cts_parse(const uint8_t *d, size_t n, int &year, int &month, int &day, int &hour, int &minute, int &second) {
  if (n < 7) return false;
  year = d[0] | (d[1] << 8);
  month = d[2];
  day = d[3];
  hour = d[4];
  minute = d[5];
  second = d[6];
  return year >= 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour < 24 && minute < 60 && second < 60;
}

// =====================================================================================
// Status koneksi (diisi callback NimBLE = task host BLE; dibaca loop utama)
// =====================================================================================
static portMUX_TYPE iosMux = portMUX_INITIALIZER_UNLOCKED;
static bool running = false;
static volatile bool linkUp = false, encrypted = false, setupPending = false, ready = false;
static volatile uint32_t linkUpAt = 0;          // millis() saat tersambung, untuk batas waktu pairing
static const uint32_t PAIR_TIMEOUT_MS = 60000;  // tersambung tapi belum siap selama ini -> putus, iPhone coba lagi
static volatile uint16_t connHandle = 0;
static NimBLEServer *server = nullptr;
static NimBLERemoteCharacteristic *ancsControl = nullptr, *amsCommand = nullptr;

// Antrean kecil dari callback notify -> loop
static const int NS_QUEUE = 8;
static AncsEvent nsQueue[NS_QUEUE];
static volatile int nsHead = 0, nsCount = 0;

static const int DS_BUF = 720;  // respons ANCS lengkap: ~616 byte dengan pesan 511
static uint8_t dsBuf[DS_BUF];
static volatile size_t dsLen = 0;

struct AmsUpdate {
  uint8_t entity, attr;
  char value[64];
};
static const int EU_QUEUE = 8;
static AmsUpdate euQueue[EU_QUEUE];
static volatile int euHead = 0, euCount = 0;

static uint8_t ctsPending[10];
static volatile bool ctsHave = false;
static volatile uint8_t battery = 0;

// Permintaan atribut ANCS: satu per satu (iOS menjawab berurutan)
static uint32_t reqQueue[8];
static int reqCount = 0;
static uint32_t reqOutstanding = NO_UID, reqSentAt = 0;
static uint32_t callUid = NO_UID;

// Musik (hanya diubah di loop)
static MusicInfo music;
static float playRate = 0, elapsedAtUpdate = 0, durationSec = 0;
static uint32_t playbackUpdateMs = 0;
static bool havePlayback = false, haveDuration = false;

// ---------------- callback notify (task host BLE): hanya salin data ----------------
static void onNotificationSource(NimBLERemoteCharacteristic *, uint8_t *d, size_t n, bool) {
  AncsEvent e;
  if (!ancs_parseNotificationSource(d, n, e)) return;
  portENTER_CRITICAL(&iosMux);
  if (nsCount == NS_QUEUE) { nsHead = (nsHead + 1) % NS_QUEUE; nsCount--; }
  nsQueue[(nsHead + nsCount) % NS_QUEUE] = e;
  nsCount++;
  portEXIT_CRITICAL(&iosMux);
  power_loopWake();  // proses segera walau layar mati (sama dengan jalur Chronos)
}

static void onDataSource(NimBLERemoteCharacteristic *, uint8_t *d, size_t n, bool) {
  portENTER_CRITICAL(&iosMux);
  if (dsLen + n > DS_BUF) dsLen = 0;  // luapan: buang, permintaan berikutnya mulai bersih
  memcpy(dsBuf + dsLen, d, n);
  dsLen += n;
  portEXIT_CRITICAL(&iosMux);
  power_loopWake();
}

static void onEntityUpdate(NimBLERemoteCharacteristic *, uint8_t *d, size_t n, bool) {
  if (n < 3) return;  // EntityID, AttributeID, Flags, Value...
  portENTER_CRITICAL(&iosMux);
  if (euCount == EU_QUEUE) { euHead = (euHead + 1) % EU_QUEUE; euCount--; }
  AmsUpdate &u = euQueue[(euHead + euCount) % EU_QUEUE];
  u.entity = d[0];
  u.attr = d[1];
  copyAttr(u.value, sizeof(u.value), d + 3, n - 3);
  euCount++;
  portEXIT_CRITICAL(&iosMux);
}

static void onCurrentTime(NimBLERemoteCharacteristic *, uint8_t *d, size_t n, bool) {
  if (n < 7) return;
  portENTER_CRITICAL(&iosMux);
  memcpy(ctsPending, d, min(n, sizeof(ctsPending)));
  ctsHave = true;
  portEXIT_CRITICAL(&iosMux);
}

static void onBattery(NimBLERemoteCharacteristic *, uint8_t *d, size_t n, bool) {
  if (n >= 1) battery = d[0];
}

// ---------------- callback server ----------------
class IosServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *, NimBLEConnInfo &info) override {
    connHandle = info.getConnHandle();
    linkUpAt = millis();
    linkUp = true;
    Serial.printf("[iOS] terhubung %s handle=%u, bonded=%d | layar %s, light sleep %s\n",
                  info.getAddress().toString().c_str(), info.getConnHandle(), info.isBonded(),
                  power_screenOn() ? "ON" : "OFF", power_lightSleepActive() ? "aktif" : "ditahan");
    // ANCS/AMS hanya bisa diakses setelah link terenkripsi: minta pairing/enkripsi
    NimBLEDevice::startSecurity(info.getConnHandle());
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int reason) override {
    linkUp = encrypted = setupPending = ready = false;
    ancsControl = amsCommand = nullptr;
    ble_setRinging(nullptr, false);
    // NimBLE: alasan = 0x200 + kode HCI
    int hci = reason & 0xFF;
    const char *why = hci == 0x13 ? "HP memutus" : hci == 0x08 ? "timeout (sinyal/jam tidur)" : hci == 0x3D ? "MIC failure (kunci salah)"
                      : hci == 0x05 ? "auth failure (pairing ditolak)" : hci == 0x16 ? "diputus jam" : hci == 0x3E ? "gagal membuka koneksi" : "lain";
    Serial.printf("[iOS] terputus alasan=%d (HCI 0x%02X %s) | layar %s, light sleep %s, advertising lagi\n", reason, hci, why,
                  power_screenOn() ? "ON" : "OFF", power_lightSleepActive() ? "aktif" : "ditahan");
  }
  void onAuthenticationComplete(NimBLEConnInfo &info) override {
    Serial.printf("[iOS] auth selesai: encrypted=%d bonded=%d authenticated=%d keysize=%u\n", info.isEncrypted(),
                  info.isBonded(), info.isAuthenticated(), info.getSecKeySize());
    if (info.isEncrypted()) {
      encrypted = true;
      setupPending = true;  // discovery GATT dilakukan di loop (blocking, jangan di task BLE)
    } else {
      // Jangan biarkan link tak terenkripsi menggantung (iPhone berputar "Connecting" terus): putus
      Serial.println("[iOS] pairing gagal / tidak terenkripsi, memutus koneksi");
      server->disconnect(info.getConnHandle());
    }
  }
};
static IosServerCallbacks serverCallbacks;

// Operasi GATT client NimBLE menunggu jawaban iPhone tanpa batas (sampai timeout ATT 30 dtk / putus).
// iPhone bisa diam lama (dialog izin notifikasi, sambung-putus beruntun) -> loop macet > TWDT 8 dtk -> reset.
// Selama operasi itu loop dikeluarkan dari pengawasan TWDT; batas atasnya tetap timeout ATT 30 dtk.
struct WdtPause {
  WdtPause() { esp_task_wdt_delete(NULL); }
  ~WdtPause() { esp_task_wdt_add(NULL); }
};

// =====================================================================================
// Setup GATT client di koneksi yang sama (dijalankan di loop utama, blocking singkat)
// =====================================================================================
static NimBLERemoteCharacteristic *findChar(NimBLEClient *c, const NimBLEUUID &svc, const NimBLEUUID &chr) {
  NimBLERemoteService *s = c->getService(svc);
  return s ? s->getCharacteristic(chr) : nullptr;
}

static void requestAmsAttributes(NimBLERemoteCharacteristic *eu) {
  const uint8_t player[] = {AMS_ENTITY_PLAYER, AMS_PLAYER_NAME, AMS_PLAYER_PLAYBACK_INFO};
  const uint8_t track[] = {AMS_ENTITY_TRACK, AMS_TRACK_ARTIST, AMS_TRACK_TITLE, AMS_TRACK_DURATION};
  eu->writeValue(player, sizeof(player), true);
  eu->writeValue(track, sizeof(track), true);
}

static void applyTime(const uint8_t *d, size_t n) {
  int y, mo, dd, h, mi, s;
  if (cts_parse(d, n, y, mo, dd, h, mi, s)) {
    ble_setLocalTime(s, mi, h, dd, mo, y);
    Serial.printf("[iOS] jam disetel %04d-%02d-%02d %02d:%02d:%02d\n", y, mo, dd, h, mi, s);
  }
}

// Baca ulang jam iPhone (CTS) atas permintaan pengguna; false bila tidak tersambung / CTS tidak ada
bool ios_updateConnParams(uint16_t minItv, uint16_t maxItv, uint16_t latency, uint16_t timeout) {
  if (!ios_connected()) return false;
  server->updateConnParams(connHandle, minItv, maxItv, latency, timeout);
  return true;
}

bool ios_readTime() {
  if (!ios_connected()) return false;
  WdtPause wp;
  NimBLEClient *c = server->getClient(connHandle);
  if (!c) return false;
  NimBLERemoteCharacteristic *ct = findChar(c, NimBLEUUID(CTS_SERVICE), NimBLEUUID(CTS_CURRENT_TIME));
  if (!ct) return false;
  NimBLEAttValue v = ct->readValue();
  applyTime(v.data(), v.length());
  return true;
}

static void setupGatt() {
  WdtPause wp;
  NimBLEClient *c = server->getClient(connHandle);
  if (!c) { Serial.println("[iOS] getClient gagal"); return; }
  Serial.println("[iOS] terenkripsi, mencari layanan iPhone...");

  if (NimBLERemoteCharacteristic *ct = findChar(c, NimBLEUUID(CTS_SERVICE), NimBLEUUID(CTS_CURRENT_TIME))) {
    NimBLEAttValue v = ct->readValue();
    applyTime(v.data(), v.length());
    if (ct->canNotify()) ct->subscribe(true, onCurrentTime);
  } else {
    Serial.println("[iOS] CTS tidak ditemukan (jam tidak sinkron)");
  }

  if (NimBLERemoteCharacteristic *bl = findChar(c, NimBLEUUID(BATTERY_SERVICE), NimBLEUUID(BATTERY_LEVEL))) {
    NimBLEAttValue v = bl->readValue();
    if (v.length()) battery = v.data()[0];
    if (bl->canNotify()) bl->subscribe(true, onBattery);
  }

  NimBLERemoteService *ancs = c->getService(ANCS_SERVICE);
  if (ancs) {
    NimBLERemoteCharacteristic *ns = ancs->getCharacteristic(ANCS_NOTIFICATION_SOURCE);
    NimBLERemoteCharacteristic *ds = ancs->getCharacteristic(ANCS_DATA_SOURCE);
    ancsControl = ancs->getCharacteristic(ANCS_CONTROL_POINT);
    // Data Source dulu, agar jawaban atribut tidak terlewat
    bool ok = ds && ds->subscribe(true, onDataSource) && ns && ns->subscribe(true, onNotificationSource) && ancsControl;
    Serial.printf("[iOS] ANCS %s\n", ok ? "aktif" : "gagal subscribe");
  } else {
    Serial.println("[iOS] ANCS tidak ditemukan (izinkan 'Bagikan Notifikasi Sistem' di iPhone)");
  }

  NimBLERemoteService *ams = c->getService(AMS_SERVICE);
  if (ams) {
    amsCommand = ams->getCharacteristic(AMS_REMOTE_COMMAND);
    NimBLERemoteCharacteristic *eu = ams->getCharacteristic(AMS_ENTITY_UPDATE);
    if (amsCommand) amsCommand->subscribe(true, nullptr);  // daftar perintah yang didukung (tidak dipakai)
    if (eu && eu->subscribe(true, onEntityUpdate)) {
      requestAmsAttributes(eu);
      Serial.println("[iOS] AMS aktif");
    }
  }
  ready = true;
  ble_markPhoneInfoChanged();
}

// =====================================================================================
// Pemrosesan di loop
// =====================================================================================
static void appLabel(const char *appId, String &label, int &icon) {
  if (!strcmp(appId, "net.whatsapp.WhatsApp")) { label = "WhatsApp"; icon = 0x0A; return; }
  if (!strcmp(appId, "net.whatsapp.WhatsAppSMB")) { label = "WhatsApp Business"; icon = 0x20; return; }
  if (!strcmp(appId, "com.apple.mobilephone")) { label = "Telepon"; icon = 0xF0; return; }
  const char *dot = strrchr(appId, '.');
  label = dot ? dot + 1 : appId;
  icon = 0xFF;  // tidak dikenal: tampil sebagai 0xFF di layar Info
}

static void queueAttrRequest(uint32_t uid, bool front) {
  if (reqCount == (int)(sizeof(reqQueue) / sizeof(reqQueue[0]))) reqCount--;  // penuh: buang yang terakhir
  if (front) {
    memmove(reqQueue + 1, reqQueue, reqCount * sizeof(uint32_t));
    reqQueue[0] = uid;
  } else {
    reqQueue[reqCount] = uid;
  }
  reqCount++;
}

static void sendNextAttrRequest() {
  if (!ancsControl) return;
  if (reqOutstanding != NO_UID && millis() - reqSentAt < ATTR_REQUEST_TIMEOUT_MS) return;
  if (reqOutstanding != NO_UID) {  // tidak dijawab: lanjut, bersihkan sisa data
    portENTER_CRITICAL(&iosMux);
    dsLen = 0;
    portEXIT_CRITICAL(&iosMux);
    reqOutstanding = NO_UID;
  }
  if (!reqCount) return;
  uint32_t uid = reqQueue[0];
  memmove(reqQueue, reqQueue + 1, --reqCount * sizeof(uint32_t));
  const uint8_t cmd[] = {ANCS_CMD_GET_NOTIF_ATTRIBUTES,
                         (uint8_t)uid, (uint8_t)(uid >> 8), (uint8_t)(uid >> 16), (uint8_t)(uid >> 24),
                         ANCS_ATTR_APP_ID,
                         ANCS_ATTR_TITLE, (uint8_t)ANCS_TITLE_MAX, (uint8_t)(ANCS_TITLE_MAX >> 8),
                         ANCS_ATTR_MESSAGE, (uint8_t)ANCS_MESSAGE_MAX, (uint8_t)(ANCS_MESSAGE_MAX >> 8)};
  WdtPause wp;
  if (ancsControl->writeValue(cmd, sizeof(cmd), true)) {
    reqOutstanding = uid;
    reqSentAt = millis();
  }
}

static void processNotificationSource() {
  while (true) {
    AncsEvent e;
    portENTER_CRITICAL(&iosMux);
    bool have = nsCount > 0;
    if (have) { e = nsQueue[nsHead]; nsHead = (nsHead + 1) % NS_QUEUE; nsCount--; }
    portEXIT_CRITICAL(&iosMux);
    if (!have) return;

    if (e.eventId == ANCS_EVENT_ADDED && e.category == ANCS_CAT_INCOMING_CALL) {
      callUid = e.uid;
      ble_setRinging("", true);       // tampilkan segera, nama menyusul
      queueAttrRequest(e.uid, true);  // prioritas
    } else if (e.eventId == ANCS_EVENT_ADDED && !(e.flags & ANCS_FLAG_PRE_EXISTING)) {
      queueAttrRequest(e.uid, false);
    } else if (e.eventId == ANCS_EVENT_REMOVED && e.uid == callUid) {
      callUid = NO_UID;
      ble_setRinging(nullptr, false);  // diangkat / ditolak / berhenti berdering
    }
  }
}

static void processDataSource() {
  uint8_t buf[DS_BUF];
  size_t n;
  portENTER_CRITICAL(&iosMux);
  n = dsLen;
  memcpy(buf, dsBuf, n);
  portEXIT_CRITICAL(&iosMux);
  if (!n) return;

  uint32_t uid;
  char appId[48] = "", title[ANCS_TITLE_MAX + 1] = "", msg[ANCS_MESSAGE_MAX + 1] = "";
  int used = ancs_parseAttributes(buf, n, uid, appId, sizeof(appId), title, sizeof(title), msg, sizeof(msg));
  if (used == 0) return;  // masih menunggu fragmen berikutnya
  portENTER_CRITICAL(&iosMux);
  if (used < 0 || (size_t)used >= dsLen) dsLen = 0;
  else { memmove(dsBuf, dsBuf + used, dsLen - used); dsLen -= used; }
  portEXIT_CRITICAL(&iosMux);
  if (used < 0) { reqOutstanding = NO_UID; return; }
  if (uid == reqOutstanding) reqOutstanding = NO_UID;

  if (uid == callUid) {
    ble_setRinging(title, true);
    return;
  }
  String label;
  int icon;
  appLabel(appId, label, icon);
  Serial.printf("[iOS] notif app=%s\n", appId);
  ble_pushNotification(icon, label.c_str(), title, msg);
}

static void processEntityUpdates() {
  bool changed = false;
  while (true) {
    AmsUpdate u;
    portENTER_CRITICAL(&iosMux);
    bool have = euCount > 0;
    if (have) { u = euQueue[euHead]; euHead = (euHead + 1) % EU_QUEUE; euCount--; }
    portEXIT_CRITICAL(&iosMux);
    if (!have) break;
    changed = true;
    if (u.entity == AMS_ENTITY_PLAYER && u.attr == AMS_PLAYER_NAME) {
      music.appName = u.value;
    } else if (u.entity == AMS_ENTITY_PLAYER && u.attr == AMS_PLAYER_PLAYBACK_INFO) {
      int state;
      float rate, elapsed;
      if (ams_parsePlaybackInfo(u.value, state, rate, elapsed)) {
        music.state = rate > 0 ? 1 : 0;
        playRate = rate;
        elapsedAtUpdate = elapsed;
        playbackUpdateMs = millis();
        havePlayback = true;
      }
    } else if (u.entity == AMS_ENTITY_TRACK && u.attr == AMS_TRACK_ARTIST) {
      music.artist = u.value;
    } else if (u.entity == AMS_ENTITY_TRACK && u.attr == AMS_TRACK_TITLE) {
      music.title = u.value;
    } else if (u.entity == AMS_ENTITY_TRACK && u.attr == AMS_TRACK_DURATION) {
      durationSec = atof(u.value);
      haveDuration = durationSec > 0;
    }
  }
  if (changed) ble_markPhoneInfoChanged();
}

// =====================================================================================
// API publik
// =====================================================================================
void ios_begin() {
  NimBLEDevice::init(settings_bleName().c_str());
  // Just Works + bonding + Secure Connections: iPhone menampilkan dialog "Pasangkan"
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setPower(TX_DBM);

  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks, false);
  server->advertiseOnDisconnect(true);

  // Data iklan: flags + solicitation ANCS (agar iOS mau memasangkan & berbagi notifikasi)
  // + appearance jam. Nama di scan response karena data iklan maks 31 byte.
  NimBLEAdvertisementData adv, scan;
  adv.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  uint8_t solicit[18] = {17, 0x15};  // panjang, tipe 0x15 = daftar UUID 128-bit yang diminta
  NimBLEUUID ancsUuid(ANCS_SERVICE);
  memcpy(solicit + 2, ancsUuid.getValue(), 16);  // NimBLE menyimpan UUID 128-bit little-endian
  adv.addData(solicit, sizeof(solicit));
  adv.setAppearance(APPEARANCE_GENERIC_WATCH);
  scan.setName(settings_bleName().c_str());

  NimBLEAdvertising *a = NimBLEDevice::getAdvertising();
  a->setAdvertisementData(adv);
  a->setScanResponseData(scan);
  a->setMinInterval(ADV_MIN_UNITS);
  a->setMaxInterval(ADV_MAX_UNITS);
  a->start();
  running = true;
  Serial.printf("[iOS] advertising sebagai \"%s\", bond tersimpan: %d\n", settings_bleName().c_str(), NimBLEDevice::getNumBonds());
}

void ios_stop() {
  if (!running) return;
  NimBLEDevice::deinit(true);
  running = false;
  server = nullptr;
  linkUp = encrypted = setupPending = ready = false;
  ancsControl = amsCommand = nullptr;
  reqCount = 0;
  reqOutstanding = callUid = NO_UID;
  dsLen = nsCount = euCount = 0;
}

bool ios_running() { return running; }

bool ios_connected() { return running && linkUp && encrypted; }

bool ios_pairing() { return running && linkUp && !ready; }

void ios_loop() {
  if (!running) return;
  if (setupPending && linkUp) {
    setupPending = false;
    setupGatt();
  }
  if (ctsHave) {
    uint8_t d[10];
    portENTER_CRITICAL(&iosMux);
    memcpy(d, ctsPending, sizeof(d));
    ctsHave = false;
    portEXIT_CRITICAL(&iosMux);
    applyTime(d, sizeof(d));
  }
  if (!ready) {
    if (linkUp && millis() - linkUpAt > PAIR_TIMEOUT_MS) {
      Serial.println("[iOS] pairing/setup tidak selesai 60 dtk, memutus koneksi");
      linkUpAt = millis();  // jangan kirim disconnect berulang sebelum onDisconnect datang
      server->disconnect(connHandle);
    }
    return;
  }
  processNotificationSource();
  processDataSource();
  sendNextAttrRequest();
  processEntityUpdates();
}

String ios_address() { return running ? String(NimBLEDevice::getAddress().toString().c_str()) : String(); }

uint8_t ios_battery() { return ios_connected() ? battery : 0; }

MusicInfo &ios_music() { return music; }

bool ios_musicProgress(uint32_t &elapsedMs, uint32_t &durationMs) {
  if (!ios_connected() || !havePlayback || !haveDuration) return false;
  float el = elapsedAtUpdate + (millis() - playbackUpdateMs) / 1000.0f * playRate;
  el = constrain(el, 0.0f, durationSec);
  elapsedMs = (uint32_t)(el * 1000);
  durationMs = (uint32_t)(durationSec * 1000);
  return true;
}

void ios_musicControl(Control cmd) {
  if (!ios_connected() || !amsCommand) return;
  uint8_t id;
  switch (cmd) {
    case MUSIC_PLAY:     id = AMS_CMD_PLAY; break;
    case MUSIC_PAUSE:    id = AMS_CMD_PAUSE; break;
    case MUSIC_TOGGLE:   id = AMS_CMD_TOGGLE; break;
    case MUSIC_NEXT:     id = AMS_CMD_NEXT; break;
    case MUSIC_PREVIOUS: id = AMS_CMD_PREVIOUS; break;
    case VOLUME_UP:      id = AMS_CMD_VOLUME_UP; break;
    case VOLUME_DOWN:    id = AMS_CMD_VOLUME_DOWN; break;
    default: return;
  }
  WdtPause wp;
  amsCommand->writeValue(&id, 1, true);
}

void ios_callAction(bool accept) {
  if (!ios_connected() || !ancsControl || callUid == NO_UID) return;
  uint32_t uid = callUid;
  const uint8_t cmd[] = {ANCS_CMD_PERFORM_ACTION, (uint8_t)uid, (uint8_t)(uid >> 8), (uint8_t)(uid >> 16),
                         (uint8_t)(uid >> 24), accept ? ANCS_ACTION_POSITIVE : ANCS_ACTION_NEGATIVE};
  WdtPause wp;
  ancsControl->writeValue(cmd, sizeof(cmd), true);
}

bool ios_deleteBonds() {
  // NimBLE harus aktif untuk menghapus bond yang tersimpan di NVS. Jika belum aktif
  // (mis. Bluetooth OFF), nyalakan sebentar lalu matikan lagi; jika sudah aktif (iPhone
  // atau Chronos), biarkan apa adanya.
  bool initHere = !NimBLEDevice::isInitialized();
  if (initHere) NimBLEDevice::init(settings_bleName().c_str());
  bool ok = NimBLEDevice::deleteAllBonds();
  if (initHere) NimBLEDevice::deinit(true);
  return ok;
}
