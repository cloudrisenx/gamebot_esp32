# Build & Upload

There are **two firmware builds** from the same code (branch `main`):

| Firmware | Contents | Build | Output |
|---|---|---|---|
| **Normal** (env `esp32-c3-devkitm-1`) | Arduino 2, all features **without light sleep** | PlatformIO buttons in VS Code | `firmware\firmware_ota.bin` |
| **Light sleep** (env `lightsleep`) | Arduino 3, all features **+ light sleep + Power save** | `tools\flash_lightsleep.ps1` | `firmware\lightsleep\firmware.bin` |

The one running on the watch now: **light sleep**.

## Light sleep firmware (the one in use)

From the VS Code terminal in `G:\esp32_c3\ESP32`:

```powershell
# build + upload over USB (COM7). Turn the watch screen on first (USB is off during light sleep)
powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1

# build only, then upload over OTA (WiFi)
powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1 -NoFlash

# another port
powershell -ExecutionPolicy Bypass -File tools\flash_lightsleep.ps1 -Port COM5
```

USB upload fails? On the watch: **Settings → Flash mode → OK, OK again** (screen goes dark), then run the command again.

Upload over **OTA**: watch **Settings → OTA update** → connect your phone/laptop to the WiFi shown on screen →
open `http://192.168.4.1` → **Firmware** tab → pick `firmware\lightsleep\firmware.bin` → Update firmware.

**DO NOT** build the `lightsleep` env with the VS Code buttons: pioarduino would overwrite the global PlatformIO
(`G:\pio_global`) and break the normal firmware build. The script builds straight from the project folder with a
separate PlatformIO in `.pio_lightsleep\` (inside the project, ignored by git). The first build takes about 25–40 minutes,
later ones about 1–2 minutes. Building one env wipes the other env's `.pio` build folder; that only costs a rebuild,
the files in `firmware\` are kept.

## Normal firmware (without light sleep)

PlatformIO **Build** (✓) / **Upload** (→) buttons in the VS Code status bar. `default_envs` in `platformio.ini`
makes sure these buttons only use the normal env. OTA file: `firmware\firmware_ota.bin` (copied automatically after a build).

## Workflow

1. Edit code in `G:\esp32_c3\ESP32`, commit.
2. `tools\flash_lightsleep.ps1`: builds the current code and uploads it.
3. Push: `git push origin main` (no Co-Authored-By lines).

## First install / changing the partition table

The new partition table (1.75 MB app, 384 KB animation) must be flashed over USB once: see
[TESTING.md](TESTING.md) section 1 (`firmware.factory.bin` at 0x0). After that, OTA or the script above is enough.

## PlatformIO locations

| Folder | Used for |
|---|---|
| `G:\pio_global` (user variable `PLATFORMIO_CORE_DIR`) or `C:\Users\<user>\.platformio` | global PlatformIO (VS Code buttons) |
| `G:\esp32_c3\ESP32\.pio_lightsleep` | light sleep build only (ignored by git) |

## Libraries

ChronosESP32 (Android / Chronos app) is a **local patched fork** in `lib/ChronosESP32` (security fixes, see
`lib/ChronosESP32/PATCHES.md`). It is not in `lib_deps`: do not delete the folder and do not add `fbiego/ChronosESP32`
back, or the unpatched version is used again. Its dependencies are pinned in `platformio.ini`
(`h2zero/NimBLE-Arduino@2.5.1`, `fbiego/ESP32Time@2.0.6`).
