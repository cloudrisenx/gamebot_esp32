# cute_device — jam pintar ESP32-C3 + OLED

Firmware jam pintar buatan sendiri berbasis **ESP32-C3** dengan layar **OLED 128x64**: menampilkan jam,
notifikasi WhatsApp, musik, cuaca, dan navigasi Google Maps dari HP (Android lewat aplikasi **Chronos**,
atau iPhone lewat Bluetooth bawaan), ditambah animasi, remote kamera, dan 7 game (beberapa bisa 2 pemain).
Firmware diperbarui tanpa kabel lewat **OTA WiFi**.

Versi firmware ada di [`src/version.h`](src/version.h) (`FW_VERSION`). Riwayat fitur per versi dan ceklist uji
di perangkat ada di [`RIWAYAT_PENGEMBANGAN.md`](RIWAYAT_PENGEMBANGAN.md).

---

## Perangkat keras

| Komponen | Detail |
|---|---|
| Mikrokontroler | ESP32-C3 (board `esp32-c3-devkitm-1`), CPU 80 MHz, flash 4 MB |
| Layar | OLED SSD1306 128x64, I2C alamat `0x3C`, SDA = GPIO 4, SCL = GPIO 5 |
| Tombol | 4 tombol aktif LOW (pull-up internal), urutan fisik kiri → kanan: **L (kiri)** = GPIO 3, **R (kanan)** = GPIO 1, **OK** = GPIO 10, **BACK** = GPIO 6 |
| Bangun dari deep sleep | tombol L / R |

Arah L/R dapat dibalik lewat `MENU_SWAP_LR` / `PORTRAIT_SWAP_LR` di [`src/screen.h`](src/screen.h).
Catatan untuk kode: di [`src/buttons.h`](src/buttons.h) tombol L/R masih bernama `BTN_UP` / `BTN_DOWN`
(nama lama), dan konstanta deep sleep `WAKE_PIN_UP` / `WAKE_PIN_DOWN`.

## Perangkat lunak yang dipakai

| Kategori | Yang dipakai |
|---|---|
| Build | [PlatformIO](https://platformio.org/) (VS Code), platform `espressif32`, framework **Arduino** (core 2.0.x) |
| Grafik OLED | Adafruit SSD1306 2.5.17, Adafruit GFX Library 1.12.6, Adafruit BusIO 1.17.4 |
| Bluetooth HP (Android) | ChronosESP32 1.9.1 (protokol aplikasi Chronos) |
| Bluetooth (stack) | NimBLE-Arduino 2.5.1 (dipakai Chronos, mode iPhone, dan remote kamera HID) |
| Jam sistem | ESP32Time 2.0.6 |
| QR code (halaman OTA) | QRCode 0.0.1 |
| Aplikasi HP | **Chronos** (Android) — notifikasi, jam, cuaca, musik, navigasi; iPhone: tanpa aplikasi (ANCS/AMS/CTS) |
| Skrip bantu | Python 3 (`scripts/copy_ota.py`, `tools/make_frames.py`), Node.js (`tools/test_web_converter.mjs`) |
| Peta kode | [graphify](https://github.com/Graphify-Labs/graphify) → folder `graphify-out/` (lihat di bawah) |

Library dipasang otomatis oleh PlatformIO dari `lib_deps` di [`platformio.ini`](platformio.ini).

## Fitur

**Beranda & jam**
- Jam besar, tanggal, status BT + baterai HP, jumlah notifikasi belum dibaca, cuaca / navigasi aktif.
- Jam disinkron dari HP (Chronos / iPhone CTS) setiap tersambung; **Setting > Jam**: Sync HP sekarang atau atur manual.
- Jam & cuaca terakhir disimpan: setelah mati/restart jam tampil sebagai perkiraan (tanda `~`), bukan `--:--`.

**HP**
- Notifikasi: pop-up WhatsApp (atau semua aplikasi), riwayat 10 notifikasi, pesan panjang bergulir.
- Panggilan masuk (nama penelepon); iPhone: angkat/tolak.
- Musik: judul/artis + kontrol putar, volume.
- Cuaca (Android), navigasi Google Maps belok-demi-belok (Android).
- Remote kamera (Bluetooth HID "Volume Up") dengan timer.

**Animasi** — animasi boot & menu dari partisi `frames` (format FRM1/FRM2 terkompresi), dibuat dengan
`tools/make_frames.py` dari GIF / PNG / frame lama, diunggah lewat halaman OTA.

**Game** (menu Game, hi-score tersimpan)
| Game | Mode |
|---|---|
| Breakout | 1P |
| Duel (adu refleks "DOR!") | 1P vs CPU / 2P — pemenang ditentukan stempel waktu interrupt (µs) |
| Dino Run | 1P |
| Hitung (hitung benda sekilas) | 1P vs CPU / 2P serentak — level 1–5, waktu & jumlah pengecoh bisa diatur |
| Flappy | 1P |
| Pong | 1P vs CPU / 2P potret — smash: kiri + kanan bersamaan |
| XO (tic-tac-toe) | 1P vs CPU / 2P |

Pop-up WhatsApp ditahan selama di menu Game (tetap masuk riwayat); panggilan tetap tampil.

**Daya**
- Layar redup lalu mati otomatis; BACK di Beranda = layar mati.
- Auto sleep: 2/5/10 menit deep sleep, atau **Jaga notif** (tanpa deep sleep, BLE tetap tersambung, loop
  dibangunkan interrupt tombol/notifikasi, parameter koneksi BLE dilonggarkan saat layar mati).
- Jadwal **Malam**: deep sleep total di jam yang diatur sendiri, bangun otomatis di jam selesai.

**Setting** — notif pop-up, Bluetooth on/off, restart Bluetooth, mode HP (Android/iPhone), jam, layar mati,
auto sleep, malam, kecerahan, **OLED I2C** (800/400/200 kHz) & **Uji OLED** (pola uji + statistik error I2C),
animasi boot, reset hi-score, tidur sekarang, update OTA, panduan iPhone, hapus pairing, info.

**Update OTA** — Setting > Update OTA menyalakan WiFi AP `cute_device_OTA`; sambungkan HP lalu buka
`http://192.168.4.1` (atau pindai QR di layar) untuk mengunggah `firmware_ota.bin` atau `frames.bin`.

## Build & update

```bash
# build (hasil siap OTA disalin otomatis ke firmware_ota.bin oleh scripts/copy_ota.py)
pio run

# uji konverter animasi halaman web
node tools/test_web_converter.mjs

# buat animasi
python tools/make_frames.py kucing.gif -o frames.bin --dither
```

Pemasangan pertama lewat USB (`pio run -t upload`); setelahnya cukup OTA. Build gagal bila firmware melebihi
slot OTA (`app0` 1,5 MB di [`partitions.csv`](partitions.csv)).

| Partisi | Ukuran | Isi |
|---|---|---|
| nvs | 20 KB | setting, hi-score, jam & cuaca tersimpan, pairing |
| app0 / app1 | 1,5 MB × 2 | firmware (OTA bergantian) |
| frames | 896 KB | animasi |
| coredump | 64 KB | jejak crash |

## Struktur folder

```
src/            kode firmware (main, menu, ble_link, ios_link, power, clockstore, game_*.cpp, ...)
web/ota.html    halaman upload OTA (ditanam ke firmware)
scripts/        skrip build PlatformIO (copy_ota.py)
tools/          make_frames.py, test_web_converter.mjs, legacy_frames/ (frame animasi lama)
graphify-out/   peta kode (graph.html, GRAPH_REPORT.md, graph.json) dari graphify
PANDUAN_IPHONE.md        cara pairing iPhone
RIWAYAT_PENGEMBANGAN.md  riwayat fitur per versi + ceklist uji perangkat
```

## Peta kode (graphify)

`graphify-out/graph.html` dapat dibuka di browser untuk melihat hubungan fungsi antar file; ringkasannya di
`graphify-out/GRAPH_REPORT.md`. Perbarui setelah mengubah kode (hanya memproses `src/`, sesuai `.graphifyignore`):

```bash
graphify update .
```
