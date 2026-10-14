#include <esp_partition.h>
#include "clockstore.h"
#include "ota.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Update.h>
#include <Adafruit_SSD1306.h>
#include <qrcode.h>
#include "buttons.h"
#include "settings.h"
#include "ble_link.h"
#include "power.h"
#include "frames_store.h"
#include "version.h"
#include <esp_ota_ops.h>

extern Adafruit_SSD1306 display;

// web/ota.html di-embed oleh PlatformIO (board_build.embed_txtfiles), diakhiri byte NUL
extern const char OTA_HTML_START[] asm("_binary_web_ota_html_start");
extern const char OTA_HTML_END[] asm("_binary_web_ota_html_end");

// Ukuran buffer modul QR versi v (sama dengan qrcode_getBufferSize(v))
#define QR_BUF_SIZE(v) ((((17 + 4 * (v)) * (17 + 4 * (v))) + 7) / 8)
static const uint8_t QR_WIFI_VERSION = 3;  // 29x29 modul: string WIFI:... (maks 53 byte ECC_LOW)
static const uint8_t QR_URL_VERSION = 2;   // 25x25 modul: "http://192.168.4.1" (maks 20 byte ECC_MEDIUM)
static_assert(QR_BUF_SIZE(3) == 106 && QR_BUF_SIZE(2) == 79, "ukuran buffer QR");
static const int QR_SCALE = 2;
static const int QR_BOX = 62;  // kotak putih di kiri; QR dipusatkan di dalamnya (quiet zone)
static const char OTA_URL[] = "http://192.168.4.1";
static const uint32_t STATUS_SHOW_MS = 4000;  // lama hasil upload tampil sebelum kembali ke langkah 2

static WebServer server(80);
static DNSServer dns;
static QRCode qrWifi, qrUrl;
static uint8_t qrWifiData[QR_BUF_SIZE(QR_WIFI_VERSION)];
static uint8_t qrUrlData[QR_BUF_SIZE(QR_URL_VERSION)];
static String password;
static String status;
static int progress = -1;           // -1 = tidak ada upload
static uint32_t lastProgressDraw = 0;
static bool rebootPending = false;
static uint32_t rebootAt = 0;
static uint32_t statusUntil = 0;    // tampilkan layar status sampai waktu ini
static int otaStep = 1;             // 1 = sambung WiFi, 2 = buka web
static int lastStations = 0;

// Upload frames: header 16 byte dikumpulkan dulu (bisa terpecah antar chunk)
static FramesHeader framesHdr;
static size_t framesHdrLen = 0;
static bool uploadOk = false;
static String uploadErr;

// QR gelap di atas kotak putih (quiet zone) agar bisa dibaca semua kamera HP
static void drawQr(QRCode &q) {
  display.fillRect(0, 1, QR_BOX, QR_BOX, SSD1306_WHITE);
  int off = (QR_BOX - q.size * QR_SCALE) / 2;
  for (uint8_t y = 0; y < q.size; y++)
    for (uint8_t x = 0; x < q.size; x++)
      if (qrcode_getModule(&q, x, y))
        display.fillRect(off + x * QR_SCALE, 1 + off + y * QR_SCALE, QR_SCALE, QR_SCALE, SSD1306_BLACK);
}

// Teks kolom kanan (x=63, lebar 65 px = maks 10-11 karakter)
static void rightText(int y, const String &s) {
  display.setCursor(63, y);
  display.print(s);
}

// Layar status: upload berjalan, restart, atau hasil upload terakhir
static void drawStatus() {
  display.setCursor(0, 0);
  display.print("UPDATE OTA");
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
  display.setCursor(0, 16);
  display.print(status.substring(0, 21));
  if (progress >= 0) {
    display.drawRect(0, 30, 100, 9, SSD1306_WHITE);
    display.fillRect(2, 32, progress * 96 / 100, 5, SSD1306_WHITE);
    display.setCursor(104, 31);
    display.print(progress);
    display.print("%");
    display.setCursor(0, 48);
    display.print("Jangan tutup halaman");
  } else if (status == "Gagal" && uploadErr.length()) {
    display.setCursor(0, 28);
    display.print(uploadErr.substring(0, 21));
    display.setCursor(0, 37);
    display.print(uploadErr.substring(21, 42));
  }
}

static void draw() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (progress >= 0 || rebootPending || millis() < statusUntil) {
    drawStatus();
  } else if (otaStep == 1) {
    drawQr(qrWifi);
    rightText(0, "1/2 WiFi");
    // 11 karakter = 66 px: print() biasa membungkus huruf terakhir ke baris bawah, jadi digambar
    // lewat kanvas tanpa wrap (yang terpotong hanya kolom kosong sesudah huruf terakhir)
    marquee_draw(63, 11, 65, DEVICE_NAME, SSD1306_WHITE, nullptr, false);
    rightText(20, "_OTA");
    rightText(31, "Pass:");
    rightText(40, password);
    rightText(55, "OK:lanjut");
  } else {
    drawQr(qrUrl);
    rightText(0, "2/2 Web");
    rightText(11, "Buka:");
    rightText(20, "192.168.4.1");
    rightText(31, "HP: " + String(WiFi.softAPgetStationNum()));
    rightText(40, "Data sel");
    rightText(48, "OFF");
    rightText(56, "OK:kembali");
  }
  oled_flush();
}

// Upload berjalan di dalam handleClient() (blocking), jadi layar & WDT diurus di sini
static void onChunk(const HTTPUpload &up) {
  power_feed();
  size_t total = server.clientContentLength();
  if (total) progress = min(100, (int)(up.totalSize * 100 / total));
  if (millis() - lastProgressDraw > 250) {
    lastProgressDraw = millis();
    draw();
  }
}

static void sendResult(bool ok, const String &msg) {
  String esc;
  for (char c : msg) {
    if (c == '"' || c == '\\') esc += '\\';
    if ((uint8_t)c >= 0x20) esc += c;
  }
  server.send(ok ? 200 : 400, "application/json",
              String("{\"ok\":") + (ok ? "true" : "false") + ",\"msg\":\"" + esc + "\"}");
}

static void handleFirmwareUpload() {
  HTTPUpload &up = server.upload();
  switch (up.status) {
    case UPLOAD_FILE_START:
      status = "Firmware";
      uploadErr = "";
      uploadOk = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
      if (!uploadOk) uploadErr = Update.errorString();
      break;
    case UPLOAD_FILE_WRITE:
      if (uploadOk && Update.write(up.buf, up.currentSize) != up.currentSize) {
        uploadOk = false;
        uploadErr = Update.errorString();
        Update.abort();
      }
      onChunk(up);
      break;
    case UPLOAD_FILE_END:
      if (uploadOk && !Update.end(true)) {
        uploadOk = false;
        uploadErr = Update.errorString();
      }
      break;
    case UPLOAD_FILE_ABORTED:
      Update.abort();
      uploadOk = false;
      uploadErr = "Upload terputus";
      break;
  }
}

static void handleFirmwareDone() {
  progress = -1;
  otaStep = 2;
  statusUntil = millis() + STATUS_SHOW_MS;
  if (uploadOk) {
    status = "Restart...";
    sendResult(true, "Firmware tersimpan. Perangkat restart dalam 2 detik.");
    rebootPending = true;
    rebootAt = millis() + 2000;
  } else {
    status = "Gagal";
    sendResult(false, uploadErr);
  }
  draw();
}

static void handleFramesUpload() {
  HTTPUpload &up = server.upload();
  switch (up.status) {
    case UPLOAD_FILE_START:
      status = "Animasi";
      framesHdrLen = 0;
      uploadOk = true;
      uploadErr = "";
      break;
    case UPLOAD_FILE_WRITE: {
      const uint8_t *data = up.buf;
      size_t len = up.currentSize;
      if (uploadOk && framesHdrLen < sizeof(FramesHeader)) {
        size_t n = min(len, sizeof(FramesHeader) - framesHdrLen);
        memcpy((uint8_t *)&framesHdr + framesHdrLen, data, n);
        framesHdrLen += n;
        data += n;
        len -= n;
        if (framesHdrLen == sizeof(FramesHeader)) uploadOk = frames_writeBegin(framesHdr, uploadErr);
      }
      if (uploadOk && len) uploadOk = frames_writeData(data, len, uploadErr);
      onChunk(up);
      break;
    }
    case UPLOAD_FILE_END:
      if (uploadOk && framesHdrLen < sizeof(FramesHeader)) { uploadOk = false; uploadErr = "File terlalu kecil"; }
      if (uploadOk) uploadOk = frames_writeEnd(uploadErr);
      break;
    case UPLOAD_FILE_ABORTED:
      uploadOk = false;
      uploadErr = "Upload terputus";
      break;
  }
}

static void handleFramesDone() {
  progress = -1;
  otaStep = 2;
  statusUntil = millis() + STATUS_SHOW_MS;
  if (uploadOk) {
    status = "Anim OK";
    sendResult(true, "Animasi tersimpan: " + String(frames_count()) + " frame @ " + String(frames_fps()) + " fps.");
  } else {
    status = "Gagal";
    sendResult(false, uploadErr);
  }
  draw();
}

// Info untuk tab Firmware/Animasi di halaman OTA
static void handleInfo() {
  const esp_partition_t *next = esp_ota_get_next_update_partition(nullptr);
  String json = String("{\"fw\":\"" FW_VERSION "\",\"app\":\"") + esp_ota_get_running_partition()->label +
                "\",\"slot\":" + (next ? next->size : 0) + ",\"anim\":";
  if (frames_valid()) {
    json += String("{\"count\":") + frames_count() + ",\"fps\":" + frames_fps() +
            ",\"w\":" + frames_width() + ",\"h\":" + frames_height() + "}";
  } else {
    json += "null";
  }
  const esp_partition_t *fp = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x40, "frames");
  json += String(",\"frames\":") + (fp ? fp->size : 0) + "}";
  server.send(200, "application/json", json);
}

static void redirectHome() {
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", "");
}

void ota_enter() {
  ble_setSuspended(true);
  password = settings_otaPassword();
  progress = -1;
  rebootPending = false;
  status = "Menunggu";
  statusUntil = 0;
  otaStep = 1;
  lastStations = 0;

  // Kedua QR disiapkan sekali: langkah 1 = sambung WiFi, langkah 2 = link halaman
  String wifiQr = "WIFI:T:WPA;S:" OTA_SSID ";P:" + password + ";;";
  qrcode_initText(&qrWifi, qrWifiData, QR_WIFI_VERSION, ECC_LOW, wifiQr.c_str());
  qrcode_initText(&qrUrl, qrUrlData, QR_URL_VERSION, ECC_MEDIUM, OTA_URL);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(OTA_SSID, password.c_str());
  // Captive portal: semua nama domain diarahkan ke ESP32 agar halaman terbuka otomatis
  dns.start(53, "*", WiFi.softAPIP());

  static bool routesAdded = false;
  if (!routesAdded) {
    server.on("/", HTTP_GET, [] {
      server.send_P(200, "text/html", OTA_HTML_START, OTA_HTML_END - OTA_HTML_START - 1);
    });
    server.on("/info", HTTP_GET, handleInfo);
    server.on("/fw", HTTP_POST, handleFirmwareDone, handleFirmwareUpload);
    server.on("/frames", HTTP_POST, handleFramesDone, handleFramesUpload);
    // URL cek koneksi Android / iOS / Windows
    for (const char *path : {"/generate_204", "/gen_204", "/hotspot-detect.html", "/connecttest.txt", "/ncsi.txt"})
      server.on(path, HTTP_GET, redirectHome);
    server.onNotFound(redirectHome);
    routesAdded = true;
  }
  server.begin();
}

static void otaExit() {
  server.stop();
  dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  ble_setSuspended(false);  // ble_update() menyalakan lagi sesuai setting
}

// Tanda isi layar OTA: langkah, jumlah HP, status/progres upload
static uint32_t otaSignature() {
  uint32_t h = sig_mix(2166136261u, otaStep);
  h = sig_mix(h, WiFi.softAPgetStationNum());
  h = sig_mix(h, progress);
  h = sig_mix(h, rebootPending);
  h = sig_mix(h, millis() < statusUntil);
  return sig_str(h, status.c_str());
}

AppState ota_update(bool redraw) {
  static RedrawWatch watch;
  dns.processNextRequest();
  server.handleClient();

  if (rebootPending) {
    buttons_clear();
    if ((int32_t)(millis() - rebootAt) >= 0) {
      clock_save("restart OTA");
      ESP.restart();
    }
    return ST_OTA;
  }
  if (btn_pressed(BTN_BACK)) {
    otaExit();
    return ST_SETTINGS;
  }
  if (btn_pressed(BTN_OK)) {
    otaStep = otaStep == 1 ? 2 : 1;
    statusUntil = 0;  // OK juga menutup layar hasil upload
    redraw = true;
  }
  buttons_clear();

  // HP pertama tersambung ke WiFi: langsung tunjukkan langkah 2 (buka web)
  static uint32_t lastStationCheck = 0;
  if (millis() - lastStationCheck >= 500) {
    lastStationCheck = millis();
    int stations = WiFi.softAPgetStationNum();
    if (lastStations == 0 && stations > 0 && otaStep == 1) { otaStep = 2; redraw = true; }
    lastStations = stations;
  }
  if (redraw_needed(watch, redraw, otaSignature, 500)) draw();
  return ST_OTA;
}
