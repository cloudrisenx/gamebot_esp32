# Roadmap

Plans that are **not** done yet, with their status and reasons. Finished work is recorded in
[`CHANGELOG.md`](CHANGELOG.md).

| Plan | Status | Notes |
|---|---|---|
| Light sleep + BLE (more power-efficient "keep notifications") | Done in the light sleep firmware; still being tested on the device | Uses pioarduino (Arduino 3.x / ESP-IDF 5.x); see [TESTING.md](TESTING.md) |
| Virtual pet in the main menu | Planned | Fits in memory |
| Running text over Bluetooth | Optional extra | Currently set over WiFi (Text tab on the OTA page, or hold OK on the Text screen) |
| Song lyrics | Not possible | The phone sends no lyrics data |

---

## Light sleep + BLE

Goal: while the screen is off, the CPU sleeps (light sleep) but Bluetooth stays connected,
and the watch wakes on a WhatsApp notification or a button.

- Arduino core 2.0.17 cannot do it: `CONFIG_PM_ENABLE`, tickless idle and BLE modem sleep are off in its
  prebuilt libraries.
- Route: pioarduino with `custom_sdkconfig` (PM + tickless idle + BT modem sleep), built by
  `tools/flash_lightsleep.ps1`. Backup of the old version: tag `v1.9.19-arduino2` and `backup/v1.9.19/`.
- A 32 kHz crystal cannot be fitted (its pins are GPIO0/GPIO1, and GPIO1 is the R button), so the BLE sleep clock
  uses the main crystal. Official Espressif ESP32-C3 example: modem sleep ±12 mA → light sleep (main crystal)
  ±2.3 mA (not measured on this device yet).
- Buttons have to wake the chip through a GPIO level (`gpio_wakeup_enable`), because edge interrupts do not run during sleep.
- Status: BLE disappearing during light sleep was seen once and could not be reproduced later. **Settings → Power save**
  (Normal / DFS / Light sleep) exists to test it and as a fallback.

## Virtual pet

A small pet in the main menu: hatches from an egg the first time, needs feeding, grows up over 7 days,
and has expressions and moods.

- Memory is enough: the app slot is now 1.75 MB with ±25–30% free. 1-bit sprites are small (±0.5–1 KB per expression).
- The pet's time has to keep running while the watch sleeps (needs a valid clock and NVS storage).

## Running text over Bluetooth

Today the text is set over WiFi (Settings → OTA update → **Text** tab, or hold OK on the Text screen), on Android and iPhone.
Over Bluetooth it would need a custom GATT service + a Web Bluetooth page, but Web Bluetooth only works in
Chrome on Android (not on iPhone/Safari), so it would only be an extra.

## Song lyrics

Not possible. Music data from the Chronos app (Android) is only title, artist, album and play state;
Apple Media Service (iPhone) does not send lyrics either. Lyrics would only be possible through a custom
companion app on the phone.
