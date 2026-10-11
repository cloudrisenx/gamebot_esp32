#include "remote_hid.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <Preferences.h>
#include <esp_mac.h>
#include "version.h"

static const uint8_t REPORT_CONSUMER = 1;
static const uint16_t USAGE_VOLUME_UP = 0x00E9;  // = tombol jepret di aplikasi Kamera iPhone & Android

// Report map: hanya ID 1 = consumer control (Kamera, input 2 byte)
static const uint8_t REPORT_MAP[] = {
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, REPORT_CONSUMER,  // Consumer Control
  0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A, 0xFF, 0x03,
  0x75, 0x10, 0x95, 0x01, 0x81, 0x00, 0xC0,
};

// ---------------- Bond swap ----------------
// Mode Remote memakai alamat Bluetooth sendiri, tetapi store bond NimBLE dikunci per alamat HP
// (peer). Tanpa pemisahan, pairing Remote dengan iPhone yang sama menimpa kunci ANCS-nya.
// Karena itu isi store ditukar: mode biasa <-> NVS "bond_main", mode Remote <-> NVS "bond_hid".
// NVS "bond_state"/"inhid" = 1 berarti store NimBLE sedang berisi bond Remote (dipulihkan saat boot).
static const int MAX_SEC = CONFIG_BT_NIMBLE_MAX_BONDS, MAX_CCCD = CONFIG_BT_NIMBLE_MAX_CCCDS;
static const uint8_t BOND_SET_VERSION = 1;

struct BondSet {
  uint8_t version, nOur, nPeer, nCccd;
  ble_store_value_sec our[MAX_SEC];
  ble_store_value_sec peer[MAX_SEC];
  ble_store_value_cccd cccd[MAX_CCCD];
};
static BondSet setA, setB;  // statis: ~1 kB, tidak di stack loop

static int collect(int type, union ble_store_value *val, void *cookie) {
  BondSet &s = *(BondSet *)cookie;
  if (type == BLE_STORE_OBJ_TYPE_OUR_SEC && s.nOur < MAX_SEC) s.our[s.nOur++] = val->sec;
  else if (type == BLE_STORE_OBJ_TYPE_PEER_SEC && s.nPeer < MAX_SEC) s.peer[s.nPeer++] = val->sec;
  else if (type == BLE_STORE_OBJ_TYPE_CCCD && s.nCccd < MAX_CCCD) s.cccd[s.nCccd++] = val->cccd;
  return 0;  // lanjut
}

static bool readStore(BondSet &s) {
  memset(&s, 0, sizeof(s));
  s.version = BOND_SET_VERSION;
  return ble_store_iterate(BLE_STORE_OBJ_TYPE_OUR_SEC, collect, &s) == 0 &&
         ble_store_iterate(BLE_STORE_OBJ_TYPE_PEER_SEC, collect, &s) == 0 &&
         ble_store_iterate(BLE_STORE_OBJ_TYPE_CCCD, collect, &s) == 0;
}

static bool writeStore(const BondSet &s) {
  bool ok = true;
  for (int i = 0; i < s.nOur; i++) ok &= ble_store_write_our_sec(&s.our[i]) == 0;
  for (int i = 0; i < s.nPeer; i++) ok &= ble_store_write_peer_sec(&s.peer[i]) == 0;
  for (int i = 0; i < s.nCccd; i++) ok &= ble_store_write_cccd(&s.cccd[i]) == 0;
  return ok;
}

static bool clearStore(const BondSet &s) {
  bool ok = true;
  for (int i = 0; i < s.nOur; i++) {
    ble_store_key_sec k;
    ble_store_key_from_value_sec(&k, &s.our[i]);
    ok &= ble_store_delete_our_sec(&k) == 0;
  }
  for (int i = 0; i < s.nPeer; i++) {
    ble_store_key_sec k;
    ble_store_key_from_value_sec(&k, &s.peer[i]);
    ok &= ble_store_delete_peer_sec(&k) == 0;
  }
  for (int i = 0; i < s.nCccd; i++) {
    ble_store_key_cccd k;
    ble_store_key_from_value_cccd(&k, &s.cccd[i]);
    ok &= ble_store_delete_cccd(&k) == 0;
  }
  return ok;
}

static bool nvsSave(const char *ns, const BondSet &s) {
  Preferences p;
  if (!p.begin(ns, false)) return false;
  bool ok = p.putBytes("set", &s, sizeof(s)) == sizeof(s);
  p.end();
  return ok;
}

// Kosong (belum pernah disimpan) = set kosong yang sah
static bool nvsLoad(const char *ns, BondSet &s) {
  memset(&s, 0, sizeof(s));
  s.version = BOND_SET_VERSION;
  Preferences p;
  if (!p.begin(ns, true)) return true;
  size_t n = p.getBytesLength("set");
  bool ok = n == 0 || (n == sizeof(s) && p.getBytes("set", &s, sizeof(s)) == sizeof(s) && s.version == BOND_SET_VERSION);
  p.end();
  return ok;
}

static void setInHid(bool inHid) {
  Preferences p;
  if (p.begin("bond_state", false)) { p.putUChar("inhid", inHid); p.end(); }
}

static bool inHid() {
  Preferences p;
  if (!p.begin("bond_state", true)) return false;
  bool v = p.getUChar("inhid", 0);
  p.end();
  return v;
}

// Tukar isi store: simpan set sekarang ke saveNs, lalu isi store dari loadNs. Atomik: bila gagal,
// store dikembalikan ke isi semula. Host NimBLE harus sudah berjalan (setelah NimBLEDevice::init).
static bool swapBonds(const char *saveNs, const char *loadNs) {
  BondSet &cur = setA, &next = setB;
  if (!readStore(cur)) { Serial.println("[BOND] gagal membaca store"); return false; }
  if (!nvsSave(saveNs, cur)) { Serial.printf("[BOND] gagal simpan ke %s\n", saveNs); return false; }
  if (!nvsLoad(loadNs, next)) { Serial.printf("[BOND] data %s rusak\n", loadNs); return false; }
  if (!clearStore(cur)) {
    writeStore(cur);  // pulihkan entri yang terlanjur terhapus
    Serial.println("[BOND] gagal mengosongkan store, dipulihkan");
    return false;
  }
  if (!writeStore(next)) {
    clearStore(next);
    writeStore(cur);
    // Data NVS loadNs rusak/tidak kompatibel (mis. sisa percobaan yang gagal, atau layout
    // ble_store_value_sec berubah setelah upgrade versi NimBLE-Arduino): hapus agar percobaan
    // berikutnya mulai dari set kosong yang sah, bukan gagal berulang dengan data yang sama.
    Preferences pc;
    if (pc.begin(loadNs, false)) { pc.clear(); pc.end(); }
    Serial.printf("[BOND] gagal menulis %s, store dipulihkan, data %s dihapus\n", loadNs, loadNs);
    return false;
  }
  Serial.printf("[BOND] %s -> NVS (%d bond), NVS %s -> store (%d bond)\n", saveNs, cur.nOur, loadNs, next.nOur);
  return true;
}

// ---------------- HID ----------------
static NimBLEHIDDevice *hid = nullptr;
static NimBLECharacteristic *consumerIn = nullptr;
static bool running = false;
static bool bondError = false;
static volatile bool connected = false;
static volatile bool rejectedFull = false;

class RemoteCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *server, NimBLEConnInfo &info) override {
    Serial.printf("[REMOTE] connect %s\n", info.getIdAddress().toString().c_str());
    // HP baru + memori bond Remote penuh: NimBLE akan menghapus bond TERTUA saat menyimpan
    // bond baru. Tolak sebelum pairing terjadi.
    if (!NimBLEDevice::isBonded(info.getIdAddress()) && NimBLEDevice::getNumBonds() >= CONFIG_BT_NIMBLE_MAX_BONDS) {
      rejectedFull = true;
      Serial.println("[REMOTE] ditolak: memori pairing penuh");
      server->disconnect(info.getConnHandle());
      return;
    }
    rejectedFull = false;
  }
  void onDisconnect(NimBLEServer *, NimBLEConnInfo &info, int reason) override {
    connected = false;
    Serial.printf("[REMOTE] disconnect %s reason=%d\n", info.getIdAddress().toString().c_str(), reason);
  }
  void onAuthenticationComplete(NimBLEConnInfo &info) override {
    connected = info.isEncrypted();
    Serial.printf("[REMOTE] auth %s %s\n", info.getIdAddress().toString().c_str(), connected ? "terenkripsi" : "gagal enkripsi");
  }
};
static RemoteCallbacks callbacks;

// Alamat random static deterministik dari MAC chip (2 bit teratas = 11), beda dari alamat mode biasa
static void remoteAddress(uint8_t addr[6]) {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  for (int i = 0; i < 6; i++) addr[i] = mac[5 - i];  // NimBLE: little-endian, addr[5] = byte teratas
  addr[0] ^= 0x5A;
  addr[5] |= 0xC0;
}

void remote_recoverBonds() {
  if (!inHid()) return;
  // Reset/tidur saat mode Remote: store masih berisi bond Remote -> kembalikan bond mode biasa
  NimBLEDevice::init("");
  if (swapBonds("bond_hid", "bond_main")) setInHid(false);
  NimBLEDevice::deinit(true);
  Serial.println("[BOND] pemulihan setelah reset di mode Remote");
}

void remote_begin() {
  if (running) return;
  NimBLEDevice::init(DEVICE_NAME " Remote");
  // inhid masih 1 = pemulihan sebelumnya gagal: store berisi bond Remote. Menukar lagi akan
  // menimpa cadangan bond_main (bond iPhone) -> tolak.
  bondError = inHid() || !swapBonds("bond_main", "bond_hid");
  if (bondError) {
    // Tanpa store terpisah, pairing Remote bisa menimpa bond iPhone: jangan advertise
    NimBLEDevice::deinit(true);
    running = true;  // layar tetap di mode Remote untuk menampilkan error
    return;
  }
  setInHid(true);

  uint8_t addr[6];
  remoteAddress(addr);
  bool addrOk = NimBLEDevice::setOwnAddr(addr) && NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM);
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEServer *server = NimBLEDevice::createServer();
  server->setCallbacks(&callbacks, false);

  hid = new NimBLEHIDDevice(server);
  consumerIn = hid->getInputReport(REPORT_CONSUMER);
  hid->setManufacturer(DEVICE_NAME);
  hid->setPnp(0x02, 0xE502, 0xA111, 0x0210);
  hid->setHidInfo(0x00, 0x01);
  hid->setReportMap((uint8_t *)REPORT_MAP, sizeof(REPORT_MAP));
  hid->setBatteryLevel(100);
  server->start();

  NimBLEAdvertising *adv = NimBLEDevice::getAdvertising();
  NimBLEAdvertisementData data;
  data.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  data.setAppearance(GENERIC_HID);
  data.addServiceUUID(hid->getHidService()->getUUID());
  data.setName(DEVICE_NAME " Remote");
  adv->setAdvertisementData(data);
  bool advOk = adv->start();
  running = true;
  connected = rejectedFull = false;
  Serial.printf("[REMOTE] alamat %s (%s), adv %s, payload %u/31 B, bond %d/%d\n",
                NimBLEDevice::getAddress().toString().c_str(), addrOk ? "random static" : "GAGAL set random",
                advOk ? "OK" : "GAGAL", (unsigned)data.getPayload().size(), NimBLEDevice::getNumBonds(),
                CONFIG_BT_NIMBLE_MAX_BONDS);
}

void remote_end() {
  if (!running) return;
  running = connected = false;
  if (bondError) { bondError = false; return; }  // stack sudah dimatikan di remote_begin
  NimBLEServer *server = NimBLEDevice::getServer();
  if (server) {
    NimBLEDevice::getAdvertising()->stop();
    for (uint16_t h : server->getPeerDevices()) server->disconnect(h);
  }
  if (swapBonds("bond_hid", "bond_main")) setInHid(false);  // gagal: dipulihkan lagi saat boot
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);  // tipe alamat disimpan statis: jangan terbawa ke Chronos/ANCS
  delete hid;  // objek milik kita; server & karakteristik dihapus deinit(true)
  hid = nullptr;
  consumerIn = nullptr;
  NimBLEDevice::deinit(true);
}

bool remote_running() { return running; }
bool remote_connected() { return running && connected; }
bool remote_rejectedFull() { return running && rejectedFull; }
bool remote_bondError() { return running && bondError; }

static void sendConsumer(uint16_t usage) {
  if (!remote_connected()) return;
  uint8_t r[2] = {(uint8_t)usage, (uint8_t)(usage >> 8)};
  consumerIn->setValue(r, sizeof(r));
  consumerIn->notify();
  r[0] = r[1] = 0;  // lepas tombol
  consumerIn->setValue(r, sizeof(r));
  consumerIn->notify();
}

void remote_volumeUp() { sendConsumer(USAGE_VOLUME_UP); }
