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

## 6. Pet (v0.9.38)

| # | Step | Expected result |
|---|---|---|
| 1 | Home → tap R once | Pet opens after ±0.3 s; smooth animation, loop not stuck |
| 2 | Home → hold R ±0.6 s / tap R 3x fast | Animation / Music opens (not Pet) |
| 3 | In Pet: tap L, tap R, tap OK, OK 3x fast | glance left, glance right, happy + hearts, tickled |
| 4 | Hold L / hold R / hold OK | feeding (fish, chewing, full) / face morphs smoothly into the 8-bit cat chasing the mouse / purr + kneading |
| 5 | Leave Pet alone for a few minutes | random cat activities (walk, tail chase, stretch, scratch, hide & peek, zoomies), boredom, sleep (max 3 min) |
| 6 | No button for 5 minutes | the screen then dims/turns off as usual; Pet freezes |
| 7 | Screen off → press a button | screen on, Pet continues where it was (no jump); stays on again for 5 minutes |
| 8 | WhatsApp arrives while in Pet | the cat looks surprised first, the pop-up appears ±1 s later |
| 9 | BACK in Pet | back to Home |

PC tests (no watch needed): `tools/pet_host_test.cpp` (behaviour) and `tools/pet_parity.py` (pixel-exact vs the
simulation `tools/pet_sim.py`); commands in [PET.md](PET.md).

## 7. Camera, iPhone notifications, Debug log (v0.9.39)

Logs are on the watch: **Settings → Debug log** (last 10 lines, newest at the bottom; OK = clear). There is no Serial
output any more.

| # | Step | Expected result |
|---|---|---|
| 1 | Android: forget "cute_device Remote", open Phone → Camera, pair | log `CAM sambung`, `CAM auth ok`; screen shows ready |
| 2 | Android: the phone disconnects/reconnects by itself (or Bluetooth off → on) | log `CAM putus ..`, then `CAM sambung` again without leaving Camera |
| 3 | Camera open, OK (timer OFF) | the phone's camera takes a photo |
| 4 | Exit Camera → open it again → OK | still snaps (no new pairing) |
| 5 | iPhone: Forget This Device + Settings → Forget phones; pair, tap **Allow** | log `ANCS tak ada 1/12` may appear first, then `ANCS aktif (ke-N)` |
| 6 | Send a WhatsApp to the iPhone (screen locked) | pop-up on the watch |
| 7 | Settings → Clock → sync with phone, then another WhatsApp | pop-up still arrives |
| 8 | Debug log screen: L/R, OK, BACK | scroll / clear ("No log yet") / back |
| 9 | v0.9.40 iPhone: Forget phones, pair with the **screen off** (slow pairing) | `ANCS aktif`; after it no `v0.9.40 Task WDT` boot line (no reset) |
| 10 | v0.9.40 iPhone mode, 3 pairings stored: another (unknown) phone connects | `iOS tolak: penuh`; the iPhone still gets notifications |
| 11 | v0.9.40 Camera: exit/enter the Camera screen 5x, Android Bluetooth off/on | never a restart; at most `CAM reset ..` / `CAM reset habis` |
| 12 | v0.9.40 Debug log with long lines | every line readable, last character not hidden by the scroll marker |
| 13 | v0.9.41 iPhone: pair, then right away send 5 WhatsApps in a row while pressing buttons | no restart (no new `v0.9.41 ...` boot line), notifications arrive |
| 14 | v0.9.41 iPhone far away / weak signal while pairing | menus keep reacting; at worst `iOS setup lama` after 60 s |
| 15 | v0.9.41 Pet: WhatsApp with the screen off | screen turns on, the cat jumps, then the pop-up |
| 16 | v0.9.42 Android: WhatsApp with screen on and off | pop-up as fast as before (no extra delay) |
| 17 | v0.9.42 Android: Music open, skip songs quickly for 5 minutes | title/artist update; no restart (no new boot line in Debug log) |
| 18 | v0.9.42 Android: Google Maps navigation, Phone > Weather, Phone > Camera | all still work as before |
| 19 | v0.9.42 Open a notification in Phone > Notifications, then receive a new WhatsApp | the open notification stays the same |
| 20 | v0.9.43 Android: 3-4 WhatsApps in a row while music is playing | every pop-up shows the right sender and text |
| 21 | v0.9.43 iPhone: turn the iPhone's Bluetooth off/on right after it connects, a few times | pairing still finishes, Debug log `ANCS aktif` |
| 22 | v0.9.43 Android: change Phone mode / enter and leave Camera, then come back | no old notification pops up again |
| 23 | v0.9.44 Animation paused, hold OK = OTA on, wait until the screen turns off, then upload an animation | the upload still finishes |
| 24 | v0.9.44 OTA web, Text tab: paste more than 1024 characters | rejected ("Teks terlalu panjang"), old text kept |
| 25 | v0.9.44 Pet: hold R a few times (morph), then BACK | Debug log `Pet maks N ms`: note N (≤ 33 = smooth 30 fps) |

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
