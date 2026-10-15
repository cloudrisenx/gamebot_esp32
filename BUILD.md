# Build & Upload

Ada **dua firmware** dari kode yang sama (branch `main`):

| Firmware | Isi | Build | Hasil |
|---|---|---|---|
| **Biasa** (env `esp32-c3-devkitm-1`) | Arduino 2, semua fitur **tanpa light sleep** | tombol PlatformIO di VS Code | `firmware_ota.bin` |
| **Light sleep** (env `lightsleep`) | Arduino 3, semua fitur **+ light sleep + Mode hemat** | `tools\flash_lightsleep.ps1` | `lightsleepbuild\firmware.bin` |

Yang dipakai di jam sekarang: **light sleep**.

## Firmware light sleep (yang dipakai)

Dari terminal VS Code di `G:\esp32 c3\ESP32`:

```powershell
# build + upload lewat USB (COM7). Nyalakan layar jam dulu (USB mati saat light sleep)
powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1

# build saja, lalu upload lewat OTA (WiFi)
powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1 -NoFlash

# port lain
powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1 -Port COM5
```

Upload lewat USB gagal? Di jam: **Setting → Mode flash → OK, OK lagi** (layar gelap), lalu jalankan perintah lagi.

Upload lewat **OTA**: jam **Setting → Update OTA** → sambung HP/laptop ke WiFi yang tampil di layar →
buka `http://192.168.4.1` → tab **Firmware** → pilih `lightsleepbuild\firmware.bin` → Update firmware.

**JANGAN** build env `lightsleep` lewat tombol VS Code atau dari folder ini: pioarduino gagal di path berspasi
(`esp32 c3`) dan menimpa PlatformIO global. Skrip di atas build di `G:\esp32_lightsleep\wt` (salinan kerja
otomatis, jangan diedit) dengan PlatformIO terpisah `G:\esp32_lightsleep\pio`. Build pertama ±25-40 menit, berikutnya ±1-2 menit.

## Firmware biasa (tanpa light sleep)

Tombol PlatformIO **Build** (✓) / **Upload** (→) di status bar VS Code. `default_envs` di `platformio.ini`
memastikan tombol ini hanya memakai env biasa. Hasil OTA: `firmware_ota.bin` (disalin otomatis setelah build).

## Alur kerja

1. Edit kode di `G:\esp32 c3\ESP32` (bukan di `G:\esp32_lightsleep\wt`), commit.
2. `tools\flash_lightsleep.ps1` — otomatis mengambil commit terbaru, build, upload.
3. Push: `git push origin main` (tanpa baris Co-Authored-By).

## Pertama kali / ganti tabel partisi

Partisi baru (app 1,75 MB, animasi 384 KB) wajib lewat USB sekali: lihat [UJI.md](UJI.md) bagian 1
(`firmware.factory.bin` di 0x0). Setelah itu cukup OTA atau skrip di atas.

## Lokasi PlatformIO

| Folder | Untuk |
|---|---|
| `G:\pio_global` (setelah `G:\esp32 c3\pindah_pio_global.ps1`) atau `C:\Users\<user>\.platformio` | PlatformIO global (tombol VS Code) |
| `G:\esp32_lightsleep\pio` | khusus build light sleep |
| `G:\esp32_lightsleep\wt` | salinan kerja untuk build light sleep |
