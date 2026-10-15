# Changelog & Test Checklist

Current firmware version: **v1.9.28** (`src/version.h`). Updates reach the device over **OTA** (or USB, see [BUILD.md](BUILD.md)).
Serial log lines are quoted exactly as the firmware prints them (in Indonesian).

---

## 1. Long text scrolls sideways (marquee) instead of wrapping / being cut off

Goal: every single-line text longer than the screen **scrolls sideways**, like the song title on the Music screen,
instead of wrapping to the next line or being cut off.

- [x] Make **one shared marquee helper** (move the logic from `drawTitle()` in `src/phone.cpp`
      to a helper in `src/menu.cpp`/`menu.h`), used by every screen:
      x/y/width area, font, 1 s initial pause, 1 px per 50 ms, two copies of the text for a smooth loop,
      animating only while the screen is on (saves power).
- [ ] Check on the device first: the **song title** has scrolled since v1.5.2. If it still wraps on the device,
      the device is still on old firmware → update first.

Places that wrapped or cut text, switched to the marquee:

| Screen | Location | Before |
|---|---|---|
| [x] Music: artist | `src/phone.cpp` (`artist.substring(0, 18) + "..."`) | cut with "..." |
| [x] Music: app name | `src/phone.cpp` (`app.substring(0, 12)`) | cut |
| [x] Notifications: list (title) | `src/phone.cpp` (`e.title ... substring(0, 15)`) | cut |
| [x] Notifications: detail (title) | `src/phone.cpp` (`e.title ... substring(0, 21)`) | cut |
| [x] Weather: city name | `src/phone.cpp` (`ble_weatherCity() ... substring(0, 21)`) | cut |
| [x] Weather: next 3 days | `src/phone.cpp` (`next.substring(0, 21)`) | cut |
| [x] Navigation: distance/title | `src/menu.cpp` (`title.substring(0, cols)`) | cut |
| [x] Navigation: street name | `src/menu.cpp` (`drawWrapped(... nav.directions ...)`) | wrapped (2 lines) |
| [x] WhatsApp pop-up: sender | `src/menu.cpp` `popup_draw()` (`sender ... substring(0, 20)`) | cut |
| [x] Call pop-up: name | `src/menu.cpp` `call_draw()` (`drawWrapped(22, 20, 17, 2, name)`) | wrapped |
| [x] Home: info line (weather / navigation) | `src/home.cpp` `drawInfoText()` | cut |
| [x] Info: last notification | `src/menu.cpp` (`ble_lastNotifInfo().substring(0, 21)`) | cut |

**Still wrapped on purpose, because the content is a paragraph:**
- The message body in the **WhatsApp pop-up** and **notification detail** (the detail scrolls with up/down).
- Fixed hint text such as "Start a route in Google Maps".

> Decision: if WhatsApp message bodies should scroll sideways too (instead of a paragraph), tick here: [ ]

---

## 2. Delete a notification by **holding OK** in the Notifications list

Location: `src/phone.cpp` (`PS_NOTIF_LIST` screen), history in `src/ble_link.cpp` (`history[]`).

- [x] **Tap OK** = open the detail (as before).
- [x] **Hold OK ±0.8 s** = delete the selected notification from the list.
  - [x] Feedback while holding: a small bar fills under the selected row (so nothing is deleted by accident).
  - [x] After deleting: "Deleted" shows briefly and the selection moves to the next item.
- [x] Add `ble_historyDelete(index)` to `ble_link` (shift the ring buffer, decrease the count, adjust the unread counter).
- [x] List empty after deleting everything → shows "No notifications yet".
- [x] On-screen hint: "hold OK=delete" (the 23-character version does not fit 128 px; the list became 3 rows).

> Optional decision: also a **"Delete all"** (e.g. hold OK for 2 s)? [ ]

---

## 3. Rename the device to `cute_device` everywhere

The device name used to differ from place to place. Everything now uses **`cute_device`**.

- [x] **One constant** `DEVICE_NAME "cute_device"` (in `src/version.h`), used everywhere below,
      so a future rename only touches one place. (Since v1.9.23 the names can also be changed on the OTA page, Device tab.)

| Place | Location | Before | After |
|---|---|---|---|
| [x] Bluetooth in Android mode (found in the Chronos app) | `src/ble_link.h` `BLE_NAME` | `ESP32C3 Nav` | `cute_device` |
| [x] Bluetooth in iPhone mode (Settings → Bluetooth) | `src/ios_link.h` `IOS_BLE_NAME` | `ESP32C3 Watch` | `cute_device` |
| [x] WiFi name during OTA update (+ WiFi QR) | `src/ota.h` `OTA_SSID` | `ESP32C3-OTA` | `cute_device_OTA` |
| [x] OTA screen step 1 on the OLED | `src/ota.cpp` `draw()` ("ESP32C3-" + "OTA" lines) | 2 lines | 2 lines `cute_device` + `_OTA` (the 15-character WiFi name does not fit one line) |
| [x] Navigation screen "Name: …" | `src/menu.cpp` (`"Nama: " BLE_NAME`) | `ESP32C3 Nav` | follows the constant |
| [x] iPhone guide on the device | `src/menu.cpp` `GUIDE_LINES` (`"   ESP32C3 Watch"`) | `ESP32C3 Watch` | `   cute_device` |
| [x] OTA web page (tab title & page title) | `web/ota.html` `<title>` and `<h1>` | `ESP32-C3 Update` | `cute_device Update` |
| [x] Messages on the OTA web page (2 places) | `web/ota.html` ("WiFi ESP32C3-OTA" text) | `ESP32C3-OTA` | `cute_device_OTA` |
| [x] iPhone guide document (3 places) | `IPHONE_GUIDE.md` | `ESP32C3 Watch` | `cute_device` |

> Decision: OTA WiFi name exactly `cute_device`, or different, e.g. `cute_device_OTA`, so it is not confused with
> the Bluetooth name? Chosen: [ ] `cute_device`  [x] `cute_device_OTA`

After the rename (note in the tests):
- [ ] **Android:** in the Chronos app, remove the old device and add **cute_device** again.
- [ ] **iPhone:** "Forget This Device" for the old one in Settings → Bluetooth + **Settings → Forget phones** on the watch,
      then pair **cute_device** again (iPhone often caches the old name).
- [ ] **OTA WiFi:** phones that joined `ESP32C3-OTA` before must join the new WiFi name (scan the QR again).

---

## 4. Wrapping up that work

- [x] Bump the version in `src/version.h` (e.g. **1.6.1**).
- [x] `pio run` succeeds → `firmware_ota.bin` ready (now in `firmware/`).
- [x] `node tools/test_web_converter.mjs` still PASSES.
- [x] Render PNGs of the changed screens (without hardware) to check that no text overlaps or is cut off.
- [x] Find Phone feature removed (did not work on the device).
- [ ] Update the device through **Settings → OTA update → Firmware tab**.

---

## 5. Pending tests on the device (from earlier work)

**iPhone mode (v1.6.0), needs an iPhone + `pio device monitor`:**
- [ ] Menu → Phone → Phone mode → iPhone; top-right corner shows "iOS".
- [ ] Pair from Settings → Bluetooth → **cute_device** → Pair → **Allow Share System Notifications**.
- [ ] Serial shows: `terenkripsi` → `jam disetel` → `ANCS aktif` → `AMS aktif`; the Home clock follows the iPhone.
- [ ] WhatsApp (Show Previews = Always): pop-up + history + envelope.
- [ ] Calls: pop-up, **OK answer / BACK reject**.
- [ ] Music: title, artist, `position/length` + progress bar, all control buttons.
- [ ] Navigation / Weather hidden in iPhone mode (since v1.9.25).
- [ ] Forget phones → iPhone asks to pair again; back to Android mode → Chronos works normally.
- [ ] If the watch does not show in the iPhone Bluetooth list → try the nRF Connect / LightBlue app (note the result).

**Power saving (v1.5.1):**
- [ ] Serial: `CPU 80 MHz`, `BLE: daya 0 dBm`; `[HEALTH]` max loop < 25 ms while playing a game.
- [ ] The screen dims at half of the "Screen off" time, then turns off.
- [ ] (Optional) measure the current with a USB power meter.

**Other:**
- [ ] Text emoji v1.9.33: faces drawn as outlines (white outline + eyes/mouth, black inside), icons 24 px = same height as the letters; other icons stay filled.
- [ ] Text emoji v1.9.32: icons are 16x16 drawn at 2x (32 px, a bit taller than the letters), made from the Windows emoji font; faces show eyes/mouth and are easy to tell apart; scrolling stays smooth.
- [ ] Text emoji v1.9.31: 8 more icons (💩 😑 🤔 😴 😱 💀 👋 💯), 24 in total; 😐 now shows as the -_- icon.
- [ ] Text emoji v1.9.30: OTA page Text tab: tap emoji buttons (or type from the phone keyboard) → Save; on the watch the 16 emoji show as icons the same height as the letters, scrolling smoothly; 🙂 shows as ":)"; reopening the tab shows the emoji again; old text without emoji looks the same. Icons: `python tools/emoji_glyphs.py`, check: `python tools/test_emoji.py`.
- [ ] Forget phones v1.9.29: Settings → Forget phones (Hapus pairing HP), OK, OK again → "Deleted"; works in Android and iPhone mode; the connected phone drops, Bluetooth restarts, and every phone (Chronos, iPhone, Camera remote) has to pair again. Serial `[BT] hapus pairing: bond aktif dihapus, bond_main/bond_hid dihapus, BLE restart`.
- [ ] Language v1.9.28: Settings → Language (Bahasa) → OK switches Indonesia ⇄ English immediately, without a restart; every screen (menus, settings, Home, Phone, games, OTA, Animation, Text) follows; English text is not cut off on the right; the choice survives a restart; Reset data → back to Indonesian.
- [ ] Animation v1.9.27: hold OK = "WIFI ANIMASI" menu (turn OTA on/off, WiFi info), tap OK = pause, BACK in the menu returns to the animation; sending an animation from the Animasi tab → plays from the start right away; leaving Animation = OTA off.
- [ ] Music v1.9.27: left = previous song, right = next, OK = play/pause, BACK = back, hold left/right = volume (VOL box). Bottom legend: |<<  >>|  play  BACK.
- [ ] Home v1.9.27: tap right = Animation, hold right 0.6 s = Music (BACK returns to Home); right legend = play icon.
- [ ] Text v1.9.26: running text moves evenly at every speed and does not speed up after a pop-up/menu/OTA.
- [ ] Text v1.9.25: hold OK = "WIFI UBAH TEKS" menu; with OTA on, text changed from the phone shows immediately; leaving Text = OTA off.
- [ ] Phone mode v1.9.25: Menu → Phone → Phone mode toggles Android/iPhone; iPhone hides Navigation and Weather.
- [ ] Device names v1.9.23: OTA page Device tab: device name, Bluetooth name, OTA WiFi name and password apply after a restart; invalid input (e.g. a 5-character password) is rejected; Reset data restores the defaults.
- [ ] Flash mode v1.9.22: Settings → Flash mode → OK, OK again → screen goes dark, buttons dead, `esptool ... chip_id` answers.
- [ ] Power save v1.9.24: Settings → Power save Normal / DFS / Light sleep; with each one the watch stays visible in a BLE scanner while the screen is off.
- [ ] Settings v1.9.20: "Restart" (OK, OK again) → "Restart..." then the watch comes back, time still correct. Serial `[SETTING] restart`.
- [ ] Settings v1.9.20: "Reset data" (OK, OK again) → "Resetting..." → restart: settings back to default, hi-scores 0, game modes default, clock not set/needs sync, text back to "Halo dari cute_device", pairing gone (iPhone: Forget This Device then pair again; Android: reconnect from Chronos). THE ANIMATION STAYS. Serial `[SETTING] reset data ... nvs_flash_erase: ESP_OK`.
- [ ] Text v1.9.20: OTA page Text tab: the current text shows, change and Save → "Tersimpan"; emoji/non-ASCII letters are removed, max 200; empty text is rejected. Serial `[TEKS] disimpan N karakter`.
- [ ] OLED v1.9.19: Settings > OLED I2C (800/400/200 kHz) applies immediately without a restart; if the OLED does not answer it drops automatically and the card shows "800>400k". Serial `[OLED] I2C 400 kHz`.
- [ ] OLED v1.9.19: Settings > OLED test: left/right pattern (frame+cross / checkerboard / animated box), OK = overlay (kHz, last/max send time, frames, errors, re-inits), hold OK 1 s = re-init the OLED, BACK exits.
- [ ] OLED v1.9.19: compare 800 / 400 / 200 kHz on the animated pattern for 1–2 minutes: note which never shifts and keeps 0 errors → use that one. Then check games & Home stay smooth. Info: line "H.../...k I2C800 e0".
- [ ] Pong v1.9.18 2P: no way to exit while playing (a long hold of right + OK does NOT exit, no "Exiting..." bar); exit only from the win screen: OK again, BACK exits after 1.5 s. READY 2P: "Exit: after a win". Duel/XO still have hold right + OK.
- [ ] Pong v1.9.17: smash = press LEFT + RIGHT together (≤ 0.12 s apart); P2 in 2P = OK + BACK. Smash is READY until the ball hits the paddle (max 3 s), then a 3 s cooldown. While both buttons are held the paddle stays still; releasing one = normal movement. READY: "Left+right: smash".
- [ ] Pong v1.9.17: holding left and pressing right later (while moving) does NOT trigger a smash; P1 & P2 chords at the same time in 2P both get READY.
- [ ] Pong v1.9.16 2P: small score next to the net; P2's score is upside down (readable from P2's side).
- [ ] Pong v1.9.16 2P: on a winner the screen turns LANDSCAPE "P1/P2 WINS", score "7 - 4", "OK: again"; "BACK: exit" only appears & works 1.5 s later.
- [ ] Pong v1.9.15 1P: the CPU sometimes misses now (aim error ±7.5 px) → beatable without a smash (simulation ±35% of points), much easier with smash. Tell me if it is too easy/hard.
- [ ] Buttons v1.9.15: menus, Duel, Math, other games still normal after the interrupt change.
- [ ] Duel v1.9.14: the winner is decided by interrupt timestamps (µs), not loop order; reaction time shown with 1 decimal ("241,3 ms"). Serial: `[DUEL] P1 x ms, P2 y ms, selisih z ms -> MENANG/CURANG/SERI`.
- [ ] Duel v1.9.14 2P: pressing almost together → the one who was really first wins; < 1 ms apart → "DRAW! again" (no point). No more random draw.
- [ ] Duel v1.9.14: foul → whoever pressed FIRST before BANG loses the round. Menu buttons & other games still normal (check!).
- [ ] Math v1.9.13: hold OK on READY → MATH SETUP: each line "Lv3 1,6s  2x2-3" (time + decoy kinds x count). Field 1 "< Time 1,2s >" (2.0/1.5/1.2/1.0/0.8 s), OK → field 2 "< Decoy Some >" (None/Few/Some/Many), OK saves; BACK = previous field/cancel. Serial: `[HITUNG] atur: Lv5 1,2s, pengecoh Sedang`.
- [ ] Math v1.9.13: decoys None → no other emoji at all; Many = same as v1.9.11. 1P & 2P the same.
- [ ] Math v1.9.13: after updating, the old Lv5 time carries over (Serial `[HITUNG] migrasi waktu Lv5 -> 1000 ms` once). Never set → default 1.2 s & decoys Some.
- [ ] Math v1.9.11: decoys (other emoji) appear from Lv1 (Lv1 1 kind 2–3, Lv5 3 kinds 3–4); 1P & 2P follow the setting.
- [ ] Phone sync v1.9.10: Settings > Clock > OK while Chronos is connected → "Synced" (not "Failed"). Serial: `[JAM] sinkron HP selesai, paket jam ada/tidak dikirim`. If the source is Manual/Estimate and the phone sends no time → "No clock" (use Restart BT so Chronos sends the time on reconnect).
- [ ] Power v1.9.9: screen off → WhatsApp still pops up instantly (BLE wakes the loop), calls still show.
- [ ] Power v1.9.9: screen off → a quick/short press still turns the screen on (no need to press twice).
- [ ] Power v1.9.9: ±5 s after the screen turns off `[BLE] conn param: hemat (itv 60-120 ms, lat 4)` appears; screen on → `cepat (itv 15-30 ms, lat 0)`. Navigation & Music stay smooth.
- [ ] Power v1.9.9: leave the screen off for 1 hour → Chronos/iPhone does not disconnect often; notifications keep arriving.
- [ ] Menu v1.9.7: carousel order Animation, Text, Game, Phone, Navigation, Settings; OK opens the right screen.
- [ ] Clock v1.9.7: Settings > Clock > OK → "Phone sync" line selected; OK → "Requesting" then "Synced" (Android connected) / "No connect" (BT off) / "Failed" (no answer in 5 s). Right = continue to manual setting. Serial: `[JAM] minta sinkron HP: ok`.
- [ ] Clock v1.9.7 iPhone: phone sync re-reads the iPhone time (CTS) → "Synced".
- [ ] Night v1.9.7: old setting (e.g. 23-06) migrated to "23:00-06:00" (Serial `[POWER] migrasi jadwal malam lama`). Settings > Night > OK: On ON/OFF, Start, End (5-minute steps); OFF + OK saves right away; start = end is rejected.
- [ ] Night v1.9.7: test a short daytime window (e.g. 13:00-13:10): screen off 1 min → sleeps, wakes by itself at 13:10 (`[POWER] bangun: timer`). Also across midnight (22:00-06:00).
- [ ] Night v1.9.7: after a power cut (clock "~" Estimate) the night schedule does NOT put the watch to sleep until it is synced/set manually.
- [ ] Clock v1.9.6: unplug power/press EN reset and power on → Home shows the last time with "~" + "Estimated time, connect Chronos" (not --:--). Serial: `[JAM] boot: jam hilang, pakai tersimpan`.
- [ ] Clock v1.9.6: connect Chronos → the "~" disappears, Settings > Clock = "Synced". Serial: `[JAM] simpan (sinkron)`.
- [ ] Clock v1.9.6: deep sleep then wake → time still correct without "~" (Serial: `[JAM] boot: jam sistem masih valid`). Also check after an OTA update.
- [ ] Clock v1.9.6: Settings > Clock > OK: left/right change (hold = fast), OK next, BACK previous field/cancel; save → source "Manual"; 31/02 rejected "Invalid date".
- [ ] Weather v1.9.6: after weather arrives, restart/power off → Home & Phone > Weather still show it with "(Nh ago)" even before Chronos connects; new data replaces it.
- [ ] Restart BT v1.9.6: Settings > Restart BT > OK, OK again → "Restart.." then "Ok"; Chronos reconnects by itself, iPhone pairing is kept. Serial: `[BT] restart modul Bluetooth`.
- [ ] Pong v1.9.4: P2 (top, facing) BACK = P2's left, OK = P2's right from P2's view. If reversed, swap them in games_p2MinusBtn/PlusBtn (src/games.h).
- [ ] Math v1.9.4 2P at once: P1 left/right, P2 BACK(-)/OK(+); P2's side is drawn rotated 180 degrees; hold your own 2 buttons 0.5 s = LOCK (numbers do not change while locking); 6 s; exit by holding BACK only outside the answer phase.
- [ ] Math v1.9.3 (replaces Racer): objects flash briefly then disappear; right +1, left -1 (hold = fast), OK locks, 8 s bar; Lv1–5 more/faster + decoys; 1P vs CPU (Hi = total points) & 2P; exit by holding BACK 1 s; other games' hi-scores kept.
- [ ] Duel v1.9.2 (replaces Tron): "Ready..." then BANG! at a random time; 1P vs CPU (left = shoot, BACK exits, Hi = rounds won), 2P (P1 left, P2 BACK, exit by holding right+OK 5 s); pressing before BANG = FOUL! (point to the opponent); 5 points wins; other games' hi-scores kept.
- [ ] Controls text v1.9.1: READY screens of Breakout/Dino/Flappy/Pong use left/right (Dino: OK/left jump, hold right duck) and match the physical buttons.
- [ ] XO v1.9.1: 1P vs CPU (you are X, BACK exits, the CPU can be beaten, Hi = win streak) & 2P taking turns (exit by holding right+OK 5 s); left/right moves between empty cells, OK places, the winning line is highlighted, the starting player alternates.
- [ ] Game v1.9.0: WhatsApp does not pop up in the Game menu but goes into the history (Phone > Notifications) & the Home envelope; calls still show. Serial: `[NOTIF] pop-up ditahan (game)`.
- [ ] Pong v1.8.9: 2P portrait (P1 left/right, P2 OK/BACK), 7 points wins; 1P vs CPU kept (hi-score). Also check a WhatsApp pop-up during 2P, then the screen returns to portrait.
- [ ] Game v1.8.8: Flappy gaps do not touch the ground/ceiling & the top pipe is visible behind the score; Breakout centre area + retro panel; Breakout faster; 6th game Pong vs CPU (hi-score saved, other games' scores kept).
- [ ] Night v1.8.7: Settings > Night = 23:00-06:00; scheduled deep sleep (screen off 1 min) & automatic wake at 06:00. Note: during night deep sleep WhatsApp/calls do NOT arrive (Android: lost, iPhone: re-sent when connected). Serial: `[POWER] malam: tidur sampai 06:00 (n mnt)`, `[POWER] bangun: timer`.
- [ ] v1.8.5: Scroll feature removed. Forget & re-pair "cute_device Remote" once, then Camera still snaps (timer left/right, OK).
- [ ] Home v1.8.4: BACK turns the screen off and it stays off (except buttons/calls/WhatsApp). If it turns on by itself: send the Serial line `[POWER] layar ON alasan=..`.
- [ ] Animation v1.8.2: update the firmware FIRST, then upload a 30 fps MP4 (FRM2) through the Animasi tab; longer duration (see the "compression ratio" on the page), Serial log `[ANIM] fps target=30.0 aktual=~30`, old frames.bin (FRM1) still works.
- [ ] Camera v1.8.0: Forget & re-pair "cute_device Remote", timer left/right (OFF/3/5/10), OK snaps in the built-in iPhone/Android camera, BACK cancels the countdown.
- [ ] After Camera, iPhone notifications (ANCS) still work without re-pairing (Serial on exit: `[BOND] bond_hid -> NVS ..., NVS bond_main -> store`).
- [ ] Flappy: flap (OK/left), collisions with pipes/ground/ceiling, hi-score saved after a restart & other games' hi-scores kept.
- [ ] Long WhatsApp message: check the log [NOTIF] len=... (`pio device monitor`), the pop-up scrolls (up/down manually), the detail reaches the end. If len ±52 → cut on the phone/Chronos, not by the firmware.
- [ ] Notification & Settings cards: swipe, hold OK to delete, change values (tap OK on a Notification card = detail; "OK again?" confirmation in Settings still works).
- [ ] Breakout landscape: controls & difficulty (same up/down direction as the menu; the ball does not feel too slow or fast).
- [ ] Menu carousel: button direction & animation (Menu, Phone, Game). Left = previous item; if reversed set `MENU_SWAP_LR` to `1` in `src/screen.h`. Movement feels smooth, quick presses are not held back by the animation.
- [ ] Breakout: still restarts/hangs? If so, send the Serial log (the crash decoder is enabled).
- [ ] Dino past a score of 1500: obstacles still appear.
- [ ] OLED 800 kHz: no random lines/flicker (if there are, choose 400 kHz in Settings > OLED I2C).

---

Note: the full iPhone user guide is in [`IPHONE_GUIDE.md`](IPHONE_GUIDE.md).
