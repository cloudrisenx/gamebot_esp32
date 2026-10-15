# Testing light sleep + BLE

Light sleep firmware: Arduino 3.3.12 / ESP-IDF 5.5 (pioarduino), automatic light sleep while the screen is off.
It contains every feature of `main` plus light sleep + BLE and the **Power save** setting.

## What is different

| Part | Details |
|---|---|
| Partitions | app0/app1 1.75 MB (0x1C0000), frames **384 KB** (0x390000/0x60000). nvs/otadata/coredump unchanged → settings & pairing are kept |
| Size | firmware ±1.39 MB = ±76% of the 1.75 MB slot |
| sdkconfig | PM + tickless idle, BT modem sleep (main XTAL clock, no 32 kHz crystal), WiFi without WPA3/enterprise |
| Settings | Screen off = light sleep + BLE; automatic deep sleep only through the **Night** schedule ("Sleep now" still exists). **Power save**: Normal / DFS / Light sleep |
| While the screen is off | PM locks released: CPU may drop to 40 MHz & the chip light-sleeps when idle; the loop wakes every 1 s (not 60 ms); buttons wake it through a LOW level |
| Held awake (like the old firmware) | screen on, OTA, navigation, animation, game, pop-up/call |

## 1. Flash over USB (USB REQUIRED, not OTA)

> The partition table changed. **Do not** send this firmware through the OTA page to a watch with the old partitions:
> the 1.75 MB app does not fit the old 1.5 MB slot and frames are at a different address.

Build output is in `firmware/lightsleep/` (not committed; made by `tools\flash_lightsleep.ps1 -NoFlash`).
First switch to the new partitions (from that folder, replace `COM5` with the watch port):

```powershell
cd "G:\esp32_c3\ESP32\firmware\lightsleep"
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 --port COM5 --baud 460800 write_flash 0x0 firmware.factory.bin
# Erase the old animation left in the new frames area (settings/NVS are NOT erased)
pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32c3 --port COM5 erase_region 0x390000 0x60000
```

`firmware.factory.bin` = bootloader + partition table + boot_app0 + app in one file.
Already on the new partitions? Just run `powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1`
(build + flash app), or OTA `firmware\lightsleep\firmware.bin`.

Download mode without buttons: **Settings → Flash mode → OK, OK again** (the watch reboots and the screen goes dark),
then run the commands above. Usually esptool can also enter download mode by itself over USB.
If the watch stays in download mode after flashing: `esptool.py --chip esp32c3 --port COM5 write_mem 0x600080F4 0`
(clears the FORCE_DOWNLOAD_BOOT bit); esptool then resets the watch.

## 2. Upload the animation again (≤ 384 KB)

The old default `frames.bin` (790 KB, uncompressed FRM1) **does not fit**. Settings → OTA update → http://192.168.4.1 →
Animasi tab → pick the source video/GIF → the page compresses it to FRM2 and shows the capacity for the device's
partition (read from `/info`). Still too big: shorten it or lower the fps.

## 3. Test list

| # | Step | Expected result |
|---|---|---|
| 1 | Boot | Log `[PM] configure 40-80 MHz, light sleep: ESP_OK` |
| 2 | Wait for the screen to turn off (Settings → Screen off) | `[PM] mode Light slp: DFS ON, light sleep ON`; every 10 s `[PM] mode Light slp, light sleep aktif, ...` |
| 3 | Screen off → press L / R / OK / BACK (each one) | Screen turns on immediately (the first press only wakes the screen), `light sleep off` |
| 4 | Screen off → hold a button for 3 s, then release | No freeze / no WDT reset; screen turns on |
| 5 | Screen off → send a WhatsApp / phone notification | Pop-up appears right away as before |
| 6 | Screen off → incoming call | Call screen appears |
| 7 | Screen off for 10 minutes, then check the clock | Clock is not slow/jumping; BLE still connected (BT icon) |
| 8 | Duel & Pong (screen on) | Buttons/interrupts normal (light sleep is not active while the screen is on) |
| 9 | OTA update (enter & upload) | WiFi stable, light sleep held off |
| 10 | Settings → Power save: Normal / DFS / Light sleep | Each mode: screen off ±30 s, the watch stays visible in a BLE scanner |
| 11 | Deep sleep (Sleep now / Night schedule) | Still sleeps and wakes with L/R as before |

USB note [uncertain]: `CONFIG_USJ_NO_AUTO_LS_ON_CONNECTION` is off, so the USB console may drop/lag during light sleep.
`[PM]` logs are most reliable while the screen is on; judge tests 3–7 by the watch's behaviour, not the log.
`sebab bangun terakhir` (last wake cause) = an `esp_sleep_wakeup_cause_t` number (e.g. 7 = GPIO, 4 = timer).

## 4. Measuring current

Measure on **battery** (USB unplugged — USB powers the board and holds back part of the sleep), multimeter in series
with the battery lead, screen off ≥ 1 minute, BLE connected to the phone:

| Condition (screen off, BLE connected) | Power save | Estimate (Espressif example, not a promise) |
|---|---|---|
| Baseline | Normal | ~12 mA or more (modem sleep) |
| Test | Light sleep | much lower; Espressif example BLE + light sleep (XTAL) ~2–3 mA average |

Also note the spikes at BLE connection events; a normal multimeter only shows a rough average.

## 5. Back to the old partitions (v1.9.19) over USB

```powershell
cd "G:\esp32_c3\ESP32"
git checkout v1.9.19-arduino2
pio run -e esp32-c3-devkitm-1 -t upload   # bootloader + old partitions + boot_app0 + app v1.9.19
git switch main
```

Then upload the old animation `backup/v1.9.19/frames.bin` through the OTA page (Animasi tab); the old frames area
(0x310000) was overwritten if the light sleep firmware was ever OTA'd into app1. Settings & pairing are kept (NVS did not move).

## Rebuilding (developer notes)

Quick way: `tools\flash_lightsleep.ps1` (does the steps below). Manual: PowerShell (not Git Bash), a path without
spaces, a separate core. DO NOT use the VS Code Build/Upload buttons for this env:

```powershell
cd G:\esp32_c3\ESP32
$env:PLATFORMIO_CORE_DIR = 'G:\esp32_c3\ESP32\.pio_lightsleep'
pio run -e lightsleep
# the build appends a NUL byte to web/ota.html: remove that byte (do not git checkout: it wipes uncommitted edits)
```

Changing `custom_sdkconfig`? Delete `sdkconfig.lightsleep` first (otherwise the old values are used). Never run
two builds at the same time in the same folder.
