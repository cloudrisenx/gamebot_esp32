# Project Lucu Lucuan

Firmware jam pintar buatan sendiri berbasis **ESP32-C3** dengan layar **OLED 128x64**: menampilkan jam,
notifikasi WhatsApp, musik, cuaca, dan navigasi Google Maps dari HP (Android lewat aplikasi **Chronos**,
atau iPhone lewat Bluetooth bawaan), ditambah animasi, remote kamera, dan 7 game (beberapa bisa 2 pemain).
Firmware diperbarui tanpa kabel lewat **OTA WiFi**.

Versi firmware ada di [`src/version.h`](src/version.h) (`FW_VERSION`). Riwayat fitur per versi dan ceklist uji
di perangkat ada di [`RIWAYAT_PENGEMBANGAN.md`](RIWAYAT_PENGEMBANGAN.md); rencana berikutnya di [`ROADMAP.md`](ROADMAP.md).

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
- Tombol: L = Notifikasi, R = Animasi, OK = Menu, BACK = layar mati.
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

**Teks** — running text besar di menu utama (kiri/kanan = kecepatan, OK = jeda). Isi teks (maks 200 karakter)
diatur lewat halaman OTA, tab **Teks**.

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
- Layar mati = **light sleep + BLE** (branch lightsleep): BLE tetap tersambung, tombol/notifikasi membangunkan
  seketika, parameter koneksi BLE dilonggarkan. Tidak ada deep sleep otomatis selain jadwal Malam.
- Jadwal **Malam**: deep sleep total di jam yang diatur sendiri, bangun otomatis di jam selesai.

**Setting** — notif pop-up, Bluetooth on/off, restart Bluetooth, mode HP (Android/iPhone), jam, layar mati,
auto sleep, malam, kecerahan, **OLED I2C** (800/400/200 kHz) & **Uji OLED** (pola uji + statistik error I2C),
animasi boot, reset hi-score, tidur sekarang, **Restart**, **Mode flash** (reboot ke mode download untuk flash USB tanpa tombol BOOT), **Reset data** (hapus semua setting, hi-score,
jam tersimpan, teks, dan pairing; animasi tetap), update OTA, panduan iPhone, hapus pairing, info.

**Update OTA** — Setting > Update OTA menyalakan WiFi AP `cute_device_OTA`; sambungkan HP lalu buka
`http://192.168.4.1` (atau pindai QR di layar) untuk mengunggah `firmware_ota.bin` atau `frames.bin`,
dan mengubah isi running text (tab Teks). Tab **Perangkat** mengatur nama device (teks di jam), nama
Bluetooth (Android/Chronos/iPhone; Remote = nama + " Remote"), nama WiFi OTA, dan password WiFi OTA (8-63 karakter);
berlaku setelah restart, kosongkan nama = kembali ke `cute_device`. **Reset data** mengembalikan semuanya ke bawaan.

## Cara pakai di Android

1. Pasang aplikasi **Chronos** dari Play Store.
2. Di jam: **Setting → Mode HP → Android** (bawaan) dan **Setting → Bluetooth → ON**.
3. Buka Chronos → tambah jam / scan → pilih **cute_device**. Setelah tersambung, pojok kanan atas jam berubah
   dari `bt` menjadi `BT` (+ baterai HP) dan jam di Beranda langsung sinkron.
4. Beri izin yang diminta Chronos, terutama **akses notifikasi**, lalu di Chronos aktifkan notifikasi untuk
   **WhatsApp** (dan aplikasi lain bila ingin).
5. Pilih jenis pop-up di jam: **Setting → Notif pop-up** = WhatsApp / Semua / OFF.
6. Opsional di Chronos: aktifkan **cuaca** (muncul di Beranda dan HP → Cuaca) dan **navigasi** (rute Google
   Maps tampil di menu Navigasi).
7. Agar tetap tersambung di latar belakang, matikan penghemat baterai untuk Chronos di pengaturan Android.

Jam tidak tersinkron? **Setting → Jam → Sync HP**, atau **Setting → Restart BT**.

## Cara pakai di iPhone

Tidak perlu aplikasi (memakai Bluetooth bawaan iOS: notifikasi, musik, jam). Ringkas:

1. Di jam: **Setting → Mode HP → iPhone** (panduan singkat muncul di layar).
2. Di iPhone: **Pengaturan → Bluetooth → cute_device → Pasangkan**, lalu **Izinkan** "Bagikan Notifikasi Sistem".
3. Agar isi pesan WhatsApp ikut tampil: **Pengaturan → Notifikasi → WhatsApp → Tampilkan Pratinjau → Selalu**.

Panggilan dapat diangkat (OK) / ditolak (BACK) dari jam. **Cuaca dan navigasi hanya tersedia di Android.**
Langkah lengkap, ganti HP, dan pemecahan masalah: [`PANDUAN_IPHONE.md`](PANDUAN_IPHONE.md).

## Cara ganti animasi

Animasi disimpan di partisi `frames` dan diganti **tanpa kabel**, langsung dari browser HP:

1. Di jam: **Setting → Update OTA**. Jam menyalakan WiFi **cute_device_OTA**; password tampil di layar
   (atau pindai QR WiFi di layar).
2. Sambungkan HP ke WiFi itu, lalu buka **http://192.168.4.1** (biasanya terbuka sendiri; atau pindai QR kedua).
3. Tab **Animasi** → pilih file: **GIF**, **video** (MP4/WebM/MOV), **gambar** (PNG/JPG/BMP), atau **frames.bin**.
4. Atur sebelum kirim:
   - Video: tentukan **Mulai / Selesai** (tombol "di sini" saat memutar), lalu **Ambil frame**.
   - **Kecepatan** (fps), **Ambang** (batas hitam/putih), **Orientasi** (Landscape 128x64 / Potret 64x128),
     **Ukuran** (Pas dengan bingkai hitam / Penuh dipotong / Tarik).
   - Pratinjau menampilkan jumlah frame, durasi, dan **% kapasitas** setelah kompresi. Bila lebih dari 100%,
     pendekkan klip, turunkan fps, atau matikan dither.
5. Tekan **Kirim animasi**. Setelah selesai, animasi baru dipakai di menu **Animasi** dan animasi boot
   (**Setting → Animasi boot** untuk menyalakan/mematikan saat menyala).

Alternatif dari komputer: buat `frames.bin` dengan skrip, lalu unggah file itu di tab Animasi yang sama.

```bash
python tools/make_frames.py kucing.gif -o frames.bin --dither          # dari GIF
python tools/make_frames.py folder_png/ -o frames.bin --fps 12          # dari folder gambar
python tools/make_frames.py video_tegak.gif -o frames.bin --portrait    # potret 64x128
```

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
