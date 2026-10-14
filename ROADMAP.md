# Roadmap

Rencana yang **belum** dikerjakan, beserta status dan alasannya. Yang sudah jadi tercatat di
[`RIWAYAT_PENGEMBANGAN.md`](RIWAYAT_PENGEMBANGAN.md).

| Rencana | Status | Catatan |
|---|---|---|
| Light sleep + BLE ("Jaga notif" lebih hemat) | Sedang dikerjakan (branch `lightsleep`) | Butuh pindah ke pioarduino (Arduino 3.x / ESP-IDF 5.x) |
| Pet virtual di menu utama | Direncanakan, setelah light sleep | Layak secara memori |
| Running text lewat Bluetooth | Opsi lanjutan | Sekarang diatur lewat WiFi (tab Teks halaman OTA) |
| Lirik lagu | Tidak bisa | Tidak ada data lirik dari HP |

---

## Light sleep + BLE

Tujuan: saat layar mati di mode "Jaga notif", CPU tidur (light sleep) tetapi Bluetooth tetap tersambung,
lalu bangun saat notifikasi WA atau tombol.

- Arduino core 2.0.17 (dipakai sekarang) tidak bisa: `CONFIG_PM_ENABLE`, tickless idle, dan BLE modem sleep
  mati di library bawaannya.
- Jalur: pioarduino dengan `custom_sdkconfig` (PM + tickless idle + BT modem sleep). Branch `lightsleep`,
  cadangan versi lama: tag `v1.9.19-arduino2` dan `backup/v1.9.19/`.
- Kristal 32 kHz tidak bisa dipasang (pinnya GPIO0/GPIO1, GPIO1 dipakai tombol R), jadi clock tidur BLE
  memakai kristal utama. Contoh resmi Espressif ESP32-C3: modem sleep ±12 mA → light sleep (kristal utama)
  ±2,3 mA (belum diukur di perangkat ini).
- Tombol harus membangunkan lewat level GPIO (`gpio_wakeup_enable`), karena interrupt tepi tidak jalan saat tidur.
- Pemasangan pertama sebaiknya lewat USB (bootloader lama ESP-IDF 4.4).
- Status terakhir: build percobaan gagal saat memasang alat build pioarduino (`tool-esptoolpy`), belum sampai
  mengompilasi kode jam.

## Pet virtual

Hewan peliharaan kecil di menu utama: menetas dari telur saat pertama kali, perlu diberi makan, tumbuh
sampai dewasa dalam 7 hari, punya ekspresi dan emosi.

- Memori cukup: flash terpakai ±81% dari slot 1,5 MB (±295 KB tersisa; ±335 KB tanpa
  `CORE_DEBUG_LEVEL=5`). Sprite 1-bit kecil (±0,5–1 KB per ekspresi).
- Dikerjakan setelah migrasi light sleep jelas, karena firmware Arduino 3.x biasanya lebih besar dan
  waktu pet harus tetap berjalan saat jam tidur (perlu jam yang valid dan simpanan NVS).

## Running text lewat Bluetooth

Sekarang teks diatur lewat WiFi (Setting → Update OTA → tab **Teks**), jalan di Android maupun iPhone.
Lewat Bluetooth bisa memakai layanan GATT sendiri + halaman Web Bluetooth, tetapi Web Bluetooth hanya
jalan di Chrome Android (tidak di iPhone/Safari), jadi hanya sebagai tambahan.

## Lirik lagu

Tidak bisa. Data musik dari aplikasi Chronos (Android) hanya judul, artis, album, dan status putar;
Apple Media Service (iPhone) juga tidak mengirim lirik. Lirik hanya mungkin lewat aplikasi pendamping
buatan sendiri di HP.
