# Build firmware light sleep (env lightsleep, Arduino 3 / pioarduino) lalu flash ke jam lewat USB.
# Hasil build disalin ke <proyek>\lightsleepbuild\ (firmware.bin juga bisa dipakai untuk OTA).
#
#   powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1             # build + flash COM7
#   powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1 -Port COM5
#   powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1 -NoFlash    # build saja
#
# Kenapa tidak lewat tombol PlatformIO: pioarduino gagal di path berspasi dan menimpa PlatformIO global,
# jadi build dilakukan di worktree G:\esp32_lightsleep\wt dengan core terpisah G:\esp32_lightsleep\pio (path tanpa spasi).
# Hanya app (0x10000) yang di-flash: jam harus sudah memakai partisi baru (lihat UJI.md untuk flash penuh).
param([string]$Port = 'COM7', [switch]$NoFlash)
$ErrorActionPreference = 'Stop'

$repo = Split-Path $PSScriptRoot -Parent
$wt = 'G:\esp32_lightsleep\wt'
$out = Join-Path $repo 'lightsleepbuild'
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"

if (Get-Process pio -ErrorAction SilentlyContinue) { throw 'Masih ada proses pio berjalan: tunggu selesai dulu (jangan dua build bersamaan).' }

# 1. Worktree mengikuti commit terbaru branch main
Write-Host "== 1/3 samakan $wt dengan main"
git -C $wt merge --ff-only main
if ($LASTEXITCODE) { throw "Gagal menyamakan $wt dengan main (ada perubahan lokal di sana?)" }

# 2. Build dengan core terpisah; PlatformIO global tidak disentuh
Write-Host '== 2/3 build env lightsleep (build penuh pertama kali bisa +-25 menit)'
$oldCore = $env:PLATFORMIO_CORE_DIR
$env:PLATFORMIO_CORE_DIR = 'G:\esp32_lightsleep\pio'
$env:PYTHONIOENCODING = 'utf-8'
Push-Location $wt
try {
  & $pio run -e lightsleep
  $rc = $LASTEXITCODE
  git checkout -- web/ota.html  # build menambah byte NUL di akhir file ini
} finally {
  Pop-Location
  $env:PLATFORMIO_CORE_DIR = $oldCore
}
if ($rc) { throw 'Build lightsleep gagal (lihat pesan error di atas).' }

$b = Join-Path $wt '.pio\build\lightsleep'
New-Item -ItemType Directory -Force $out | Out-Null
foreach ($f in 'firmware.bin', 'bootloader.bin', 'partitions.bin', 'firmware.factory.bin') { Copy-Item (Join-Path $b $f) $out -Force }
Write-Host "Hasil build: $out"
if ($NoFlash) { Write-Host 'Selesai (tanpa flash). Untuk OTA pakai lightsleepbuild\firmware.bin'; exit 0 }

# 3. Flash app lewat USB. Jam harus menyala (layar ON) atau di Setting > Mode flash
Write-Host "== 3/3 flash ke $Port (nyalakan layar jam dulu, atau pilih Setting > Mode flash)"
& $pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 --port $Port --baud 460800 --after no_reset write_flash 0x10000 (Join-Path $out 'firmware.bin')
if ($LASTEXITCODE) { throw "Flash gagal. Coba: Setting > Mode flash di jam, lalu jalankan lagi." }
# Jam masih di bootloader: hapus bit paksa-download (sisa Mode flash) lalu reset agar boot normal
& $pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 --port $Port --before no_reset write_mem 0x600080F4 0 | Out-Null
Write-Host 'Selesai: jam restart dengan firmware light sleep.'
