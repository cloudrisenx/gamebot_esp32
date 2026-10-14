# Uji light sleep (branch `lightsleep`)

Firmware percobaan: Arduino 3.3.12 / ESP-IDF 5.5 (pioarduino), light sleep otomatis saat layar mati.
Basis kode: v1.9.19 (bc684a5). **Belum** berisi fitur v1.9.20 (menu Teks, Restart/Reset data).

## Apa yang berubah

| Bagian | Isi |
|---|---|
| Partisi | app0/app1 1,75 MB (0x1C0000), frames **384 KB** (0x390000/0x60000). nvs/otadata/coredump tetap → setting & pairing tetap terbaca |
| Ukuran | firmware 1.366.252 B = 74,5% slot 1,75 MB |
| sdkconfig | PM + tickless idle, BT modem sleep (clock XTAL utama, tanpa kristal 32 kHz), WiFi tanpa WPA3/enterprise |
| Setting | item baru **Light sleep** ON/OFF (bawaan ON, NVS `lsleep`) di bawah "Auto sleep" |
| Saat layar mati | kunci PM dilepas: CPU boleh 40 MHz & chip light sleep saat idle; loop bangun tiap 1 dtk (bukan 60 ms); tombol membangunkan lewat level LOW |
| Ditahan (sama dengan firmware lama) | layar nyala, OTA, navigasi, animasi, game, pop-up/panggilan, atau Setting Light sleep OFF |

## 1. Flash lewat USB (WAJIB USB — jangan OTA)

> Tabel partisi berubah. **Jangan** kirim firmware ini lewat halaman OTA ke jam berpartisi lama:
> app 1,75 MB tidak muat di slot lama 1,5 MB dan frames di alamat lain.

File ada di `backup/lightsleep-test/` (repo utama, tidak di-commit). Dari folder itu, ganti `COM5` dengan port jam:

```powershell
cd "G:\esp32 c3\ESP32\backup\lightsleep-test"
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 --port COM5 --baud 460800 write_flash 0x0 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin
# Hapus sisa animasi lama di area frames baru (setting/NVS TIDAK terhapus)
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 --port COM5 erase_region 0x390000 0x60000
```

Alternatif satu file: `write_flash 0x0 firmware.factory.bin` (isi sama: bootloader + partisi + boot_app0 + app).

Jam tidak masuk mode download? Tahan tombol BOOT (GPIO9) saat colok USB.

## 2. Upload animasi lagi (≤ 384 KB)

`frames.bin` bawaan (790 KB, FRM1 tanpa kompresi) **tidak muat**. Setting → Update OTA → http://192.168.4.1 →
tab Animasi → pilih video/GIF sumber → halaman mengompresi ke FRM2 dan menampilkan kapasitas sesuai partisi
perangkat (dibaca dari `/info`). Kalau masih kebesaran: potong durasi atau turunkan fps.

## 3. Daftar uji

| # | Langkah | Hasil yang diharapkan |
|---|---|---|
| 1 | Boot | Log `[PM] configure 40-80 MHz, light sleep: ESP_OK` |
| 2 | Tunggu layar mati (Setting Layar mati) | `[PM] light sleep boleh`; tiap 10 dtk `[PM] light sleep aktif (setting ON), ...` |
| 3 | Layar mati → tekan L / R / OK / BACK (masing-masing) | Layar langsung menyala (tekan pertama hanya menyalakan), `[PM] light sleep ditahan` |
| 4 | Layar mati → tahan tombol 3 dtk lalu lepas | Tidak macet / tidak reset WDT; layar menyala |
| 5 | Layar mati → kirim WA / notif HP | Pop-up muncul seketika seperti sebelumnya |
| 6 | Layar mati → telepon masuk | Layar panggilan muncul |
| 7 | Layar mati 10 menit, lalu cek jam | Jam tidak melambat/melompat; BLE tetap tersambung (ikon BT) |
| 8 | Game Duel & Pong (layar nyala) | Tombol/interrupt normal (light sleep tidak aktif saat layar nyala) |
| 9 | Update OTA (masuk & upload) | WiFi stabil, `[PM] light sleep ditahan` |
| 10 | Setting Light sleep OFF, layar mati | Log `light sleep ditahan (setting OFF)`; arus seperti firmware lama |
| 11 | Deep sleep (Auto sleep / Tidur sekarang / jadwal Malam) | Masih tidur & bangun lewat tombol L/R seperti dulu |

Catatan USB [tidak pasti]: `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` tidak aktif, jadi saat light sleep konsol USB bisa
putus/terlambat. Log `[PM]` paling andal terlihat saat layar menyala; uji 3–7 nilai dari perilaku jam, bukan log.
`sebab bangun terakhir` = angka `esp_sleep_wakeup_cause_t` (mis. 7 = GPIO, 4 = timer).

## 4. Ukur arus

Ukur di **baterai** (USB dicabut — USB memberi daya dan menahan sebagian sleep), multimeter seri di kabel baterai,
layar mati ≥ 1 menit, BLE tersambung ke HP:

| Kondisi | Setting Light sleep | Perkiraan (contoh Espressif, bukan janji) |
|---|---|---|
| Layar mati, Jaga notif | OFF | ~12 mA ke atas (modem sleep) |
| Layar mati, Jaga notif | ON | jauh lebih rendah; contoh Espressif BLE + light sleep (XTAL) ~2–3 mA rata-rata |

Catat juga lonjakan saat BLE connection event; multimeter biasa menampilkan rata-rata kasar.

## 5. Kembali ke v1.9.20 (partisi lama) — lewat USB

```powershell
cd "G:\esp32 c3\ESP32"
git switch main
pio run -e esp32-c3-devkitm-1 -t upload   # bootloader + partisi lama + boot_app0 + app v1.9.20
```

Lalu upload animasi lama `backup/v1.9.19/frames.bin` lewat halaman OTA (tab Animasi) — area frames lama
(0x310000) tertimpa bila firmware percobaan pernah di-OTA ke app1. Setting & pairing tetap (NVS tidak dipindah).

## Build ulang (catatan untuk developer)

PowerShell (bukan Git Bash), path tanpa spasi, core terpisah:

```powershell
cd C:\cute_ls
$env:PLATFORMIO_CORE_DIR = "$env:USERPROFILE\.platformio-ls"
pio run -e lightsleep
git checkout -- web/ota.html   # build menambah byte NUL di akhir file ini
```

Ubah `custom_sdkconfig`? Hapus dulu `sdkconfig.lightsleep` (kalau tidak, nilai lama yang dipakai). Jangan jalankan
dua build bersamaan di folder yang sama.
