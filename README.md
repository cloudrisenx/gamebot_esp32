# Project Lucu Lucuan

Home-made smartwatch firmware for the **ESP32-C3** with a **128x64 OLED**: shows the time, WhatsApp
notifications, music, weather and Google Maps navigation from your phone (Android through the **Chronos** app,
or iPhone through built-in Bluetooth), plus animations, a camera remote and 7 games (some for 2 players).
Firmware is updated wirelessly over **WiFi OTA**. The on-screen language can be **Indonesian or English**.

The firmware version lives in [`src/version.h`](src/version.h) (`FW_VERSION`). Feature history per version and the
on-device test checklist are in [`CHANGELOG.md`](docs/CHANGELOG.md); planned work is in [`ROADMAP.md`](docs/ROADMAP.md).
How to build & upload (normal and light sleep firmware): [`BUILD.md`](docs/BUILD.md).
Much of the code was written together with Claude (Anthropic): [`ACKNOWLEDGEMENTS.md`](docs/ACKNOWLEDGEMENTS.md).

---

## Hardware

| Part | Details |
|---|---|
| Microcontroller | ESP32-C3 (board `esp32-c3-devkitm-1`), 80 MHz CPU, 4 MB flash |
| Display | SSD1306 OLED 128x64, I2C address `0x3C`, SDA = GPIO 4, SCL = GPIO 5 |
| Buttons | 4 active-LOW buttons (internal pull-up), physical order left → right: **L (left)** = GPIO 3, **R (right)** = GPIO 1, **OK** = GPIO 10, **BACK** = GPIO 6 |
| Wake from deep sleep | L / R button |

L/R can be swapped with `MENU_SWAP_LR` / `PORTRAIT_SWAP_LR` in [`src/screen.h`](src/screen.h).
Code note: in [`src/buttons.h`](src/buttons.h) the L/R buttons still use their old names `BTN_UP` / `BTN_DOWN`,
and the deep sleep constants are `WAKE_PIN_UP` / `WAKE_PIN_DOWN`.

## Software used

| Category | What |
|---|---|
| Build | [PlatformIO](https://platformio.org/) (VS Code), platform `espressif32`, **Arduino** framework (core 2.0.x; the light sleep build uses Arduino 3.x / pioarduino) |
| OLED graphics | Adafruit SSD1306 2.5.17, Adafruit GFX Library 1.12.6, Adafruit BusIO 1.17.4 |
| Phone Bluetooth (Android) | ChronosESP32 1.9.1 (Chronos app protocol) |
| Bluetooth stack | NimBLE-Arduino 2.5.1 (used by Chronos, iPhone mode and the HID camera remote) |
| System clock | ESP32Time 2.0.6 |
| QR code (OTA screen) | QRCode 0.0.1 |
| Phone app | **Chronos** (Android): notifications, time, weather, music, navigation. iPhone: no app needed (ANCS/AMS/CTS) |
| Helper scripts | Python 3 (`tools/copy_ota.py`, `tools/make_frames.py`), Node.js (`tools/test_web_converter.mjs`) |
| Code map | [graphify](https://github.com/Graphify-Labs/graphify) → `graphify-out/` folder (see below) |

Libraries are installed automatically by PlatformIO from `lib_deps` in [`platformio.ini`](platformio.ini).

## Features

**Home & clock**
- Large clock, date, BT status + phone battery, unread notification count, weather / active navigation.
- Buttons: L = Notifications, R = Animation (▶ icon; **hold R** = Music), OK = Menu, BACK = screen off.
- Time is synced from the phone (Chronos / iPhone CTS) on every connection; **Settings > Clock**: sync now or set manually.
- Last time & weather are saved: after power loss/restart the clock shows an estimate (marked `~`) instead of `--:--`.

**Phone**
- Notifications: WhatsApp pop-up (or all apps), history of 10 notifications, long messages scroll.
- Incoming calls (caller name); on iPhone: answer/reject.
- Music: title/artist; left = previous song, right = next, OK = play/pause, BACK = back; hold left/right = volume -/+.
- Weather (Android), turn-by-turn Google Maps navigation (Android).
- Camera remote (Bluetooth HID "Volume Up") with a timer.
- **Phone mode** (Android / iPhone) is set in Menu → Phone. iPhone mode hides Navigation and Weather (iPhone does not send that data).

**Animation**: boot & menu animation from the `frames` partition (FRM1 / compressed FRM2), made with
`tools/make_frames.py` from GIF / PNG / old frame headers, or uploaded from the OTA page. On the Animation screen:
OK = pause, left/right = speed, **hold OK** = WiFi menu (turn OTA on, then send an animation from your phone; it plays
from the start right away), BACK = exit (OTA off).

**Text**: big running text in the main menu (left/right = speed, OK = pause). The text (max 200 characters,
plain letters/digits/punctuation plus 24 emoji drawn as 32 px icons: ❤️ 😀 😂 😍 😢 😡 👍 🙏 🔥 ⭐ 🎉 ☀️ 🌙 ✨ 😎 🤣
💩 😑 🤔 😴 😱 💀 👋 💯;
a few others become ASCII like `:)` `;)` `:P`, the rest are dropped) is set on the OTA page, **Text** tab (tap the emoji buttons). **Hold OK** = WiFi menu: turn OTA on (the text keeps running and changes from the phone show up
immediately; Bluetooth is off while OTA is on), BACK returns to the text. Leaving Text turns OTA off.

**Games** (Game menu, hi-scores are saved)
| Game | Mode |
|---|---|
| Breakout | 1P |
| Duel (reflex "BANG!" duel) | 1P vs CPU / 2P, winner decided by interrupt timestamps (µs) |
| Dino Run | 1P |
| Math (count the objects at a glance) | 1P vs CPU / 2P at once, levels 1–5, time & number of decoys adjustable |
| Flappy | 1P |
| Pong | 1P vs CPU / 2P portrait, smash: left + right together |
| XO (tic-tac-toe) | 1P vs CPU / 2P |

WhatsApp pop-ups are held while in the Game menu (they still go into the history); calls are still shown.

**Power**
- The screen dims, then turns off automatically; BACK on Home = screen off.
- Screen off = **light sleep + BLE** (light sleep firmware): BLE stays connected, buttons/notifications wake it
  instantly, BLE connection parameters are relaxed. **Settings > Power save**: Normal / DFS / Light sleep.
- **Night** schedule: full deep sleep at the hours you set, wakes up automatically at the end time.

**Settings**: notification pop-up, Bluetooth on/off, restart Bluetooth, clock, **language** (Indonesian / English),
screen timeout, power save, night, brightness, **OLED I2C** (800/400/200 kHz) & **OLED test** (test pattern + I2C error
stats), boot animation, reset hi-scores, sleep now, **Restart**, **Flash mode** (reboots into download mode for USB
flashing without the BOOT button), **Reset data** (erases all settings, hi-scores, saved time, text and pairing;
the animation stays), OTA update, iPhone guide, remove pairing, info.

**OTA update**: Settings > OTA update starts the WiFi AP `cute_device_OTA`; connect your phone and open
`http://192.168.4.1` (or scan the QR code on screen) to upload `firmware_ota.bin` or `frames.bin`
and change the running text (Text tab). The **Device** tab sets the device name (shown on the watch), the
Bluetooth name (Android/Chronos/iPhone; the remote is name + " Remote"), the OTA WiFi name and OTA WiFi password
(8–63 characters); changes apply after a restart, and an empty name means back to `cute_device`. **Reset data**
restores all of them to the defaults. (The OTA web page itself is in Indonesian.)

## Using it with Android

1. Install the **Chronos** app from the Play Store.
2. On the watch: **Menu → Phone → Phone mode → Android** (default) and **Settings → Bluetooth → ON**.
3. Open Chronos → add watch / scan → pick **cute_device**. Once connected, the top-right corner of the watch changes
   from `bt` to `BT` (+ phone battery) and the clock on Home syncs right away.
4. Grant the permissions Chronos asks for, especially **notification access**, then enable notifications for
   **WhatsApp** (and other apps if you like) in Chronos.
5. Choose the pop-up type on the watch: **Settings → Notif pop-up** = WhatsApp / All / OFF.
6. Optional in Chronos: enable **weather** (shown on Home and in Phone → Weather) and **navigation** (Google Maps
   routes show in the Navigation menu).
7. To stay connected in the background, turn off battery optimisation for Chronos in Android settings.

Clock not synced? **Settings → Clock → Phone sync**, or **Settings → Restart BT**.

## Using it with iPhone

No app needed (uses iOS built-in Bluetooth: notifications, music, time). In short:

1. On the watch: **Menu → Phone → Phone mode → iPhone** (OK switches the mode; guide in Settings → iPhone guide).
2. On the iPhone: **Settings → Bluetooth → cute_device → Pair**, then **Allow** "Share System Notifications".
3. To see WhatsApp message text: **Settings → Notifications → WhatsApp → Show Previews → Always**.

Calls can be answered (OK) / rejected (BACK) from the watch. **Weather and navigation are Android only.**
Full steps, changing phones and troubleshooting: [`IPHONE_GUIDE.md`](docs/IPHONE_GUIDE.md).

## Changing the animation

The animation lives in the `frames` partition and is replaced **without a cable**, straight from your phone's browser:

1. On the watch: **Settings → OTA update** (or hold OK on the Animation screen). The watch starts the WiFi
   **cute_device_OTA**; the password is shown on screen (or scan the WiFi QR code on screen).
2. Connect your phone to that WiFi, then open **http://192.168.4.1** (usually opens by itself; or scan the second QR code).
3. **Animasi** tab → choose a file: **GIF**, **video** (MP4/WebM/MOV), **image** (PNG/JPG/BMP), or **frames.bin**.
4. Adjust before sending:
   - Video: set **Start / End** (the "here" buttons while playing), then **Grab frames**.
   - **Speed** (fps), **Threshold** (black/white cut-off), **Orientation** (landscape 128x64 / portrait 64x128),
     **Size** (fit with black borders / fill and crop / stretch).
   - The preview shows the frame count, duration and **% of capacity** after compression. Above 100%,
     shorten the clip, lower the fps or turn off dithering.
5. Press **Send animation**. When done, the new animation is used in the **Animation** menu and as the boot
   animation (**Settings → Boot animation** turns it on/off at power-up).

From a computer: make a `frames.bin` with the script, then upload that file in the same Animasi tab, or flash it.

```bash
python tools/make_frames.py cat.gif -o frames.bin --dither              # from a GIF
python tools/make_frames.py png_folder/ -o frames.bin --fps 12          # from a folder of images
python tools/make_frames.py portrait.gif -o frames.bin --portrait       # portrait 64x128
python tools/make_frames.py tools/legacy_frames/VideoFrame_R4.h -o frames.bin --invert  # one .h with many frames
```

## Build & update

Full details (normal & light sleep firmware): [`docs/BUILD.md`](docs/BUILD.md).

```bash
# normal firmware (VS Code Build button); OTA file: firmware/firmware_ota.bin
pio run

# light sleep firmware (the one in use); output: firmware/lightsleep/firmware.bin
powershell -ExecutionPolicy Bypass -File tools/flash_lightsleep.ps1 -NoFlash

# test the web page animation converter
node tools/test_web_converter.mjs
```

The first install is over USB; after that OTA is enough. The build fails if the firmware is larger than the
OTA slot (`app0` 1.75 MB in [`partitions.csv`](partitions.csv)).

| Partition | Size | Contents |
|---|---|---|
| nvs | 20 KB | settings, hi-scores, saved time & weather, pairing |
| app0 / app1 | 1.75 MB × 2 | firmware (OTA alternates) |
| frames | 384 KB | animation (compressed FRM2) |
| coredump | 64 KB | crash trace |

## Folder layout

```
README.md       start here
docs/           BUILD.md (build & upload), TESTING.md (light sleep tests), IPHONE_GUIDE.md,
                CHANGELOG.md (history + test checklist), ROADMAP.md, ACKNOWLEDGEMENTS.md
src/            firmware code (main, menu, ble_link, ios_link, power, clockstore, lang.h, game_*.cpp, ...)
web/ota.html    OTA upload page (embedded in the firmware)
tools/          flash_lightsleep.ps1, copy_ota.py (called by PlatformIO), make_frames.py,
                test_web_converter.mjs, legacy_frames/ (old animation frames)
firmware/       ready-to-use build output (not committed): firmware_ota.bin, lightsleep/, frames*.bin
backup/         old firmware to roll back to (not committed)
platformio.ini  normal env + lightsleep env;  partitions.csv  partition table
graphify-out/   code map (graph.html, GRAPH_REPORT.md, graph.json) from graphify
```
Other folders are generated automatically, just ignore them: `.pio/`, `.pio_lightsleep/` (light sleep PlatformIO),
`.dummy/`, `sdkconfig.*`, `include/`, `lib/`, `test/` (empty PlatformIO defaults).

## Code map (graphify)

Open `graphify-out/graph.html` in a browser to see how functions connect across files; the summary is in
`graphify-out/GRAPH_REPORT.md`. Refresh it after changing code (it only processes `src/`, per `.graphifyignore`):

```bash
graphify update .
```
