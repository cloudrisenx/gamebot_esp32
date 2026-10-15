# Using the ESP32 watch with an iPhone

This watch works with **Android** (through the Chronos app) or **iPhone**.
On iPhone **no app is needed**: the watch uses iOS built-in Bluetooth features.

Menu names below are in English (**Settings → Language → English**); the Indonesian name is in brackets.

---

## 1. Pairing the iPhone

1. On the watch: open **Menu → Phone (HP) → Phone mode (Mode HP)** and press **OK** until it says **Mode: iPhone**.
2. On the iPhone: open **Settings → Bluetooth** and make sure Bluetooth is on.
3. Under **Other Devices**, tap **cute_device**.
4. A pairing request appears → tap **Pair**.
5. The question **"Share System Notifications?"** appears → tap **Allow**.
   (Without this permission notifications never reach the watch.)
6. Done. In the top-right corner of the watch **iOS** changes to **BT** and the clock on Home
   immediately matches the iPhone's time.

After pairing once, the watch reconnects automatically whenever it is near the iPhone.
The short guide is also on the watch: **Settings → iPhone guide (Panduan iPhone)**.

### To get the WhatsApp message text

By default the iPhone hides message text while it is locked. Change:

**Settings → Notifications → WhatsApp → Show Previews → Always**

Also make sure **Allow Notifications** for WhatsApp is **on** and the chats you want are not muted.

---

## 2. Features: Android vs iPhone

| Feature | Android (Chronos) | iPhone | Notes |
|---|:---:|:---:|---|
| WhatsApp & other app notifications | ✓ | ✓ | iPhone: through the "Share System Notifications" permission |
| Automatic time & date | ✓ | ✓ | |
| Incoming call pop-up | ✓ | ✓ | iPhone can **answer (OK)** / **reject (BACK)** from the watch |
| Music control (play/pause, songs, volume) | ✓ | ✓ | |
| Song position & length | ✗ (estimate "~") | ✓ | iPhone sends the real position → progress bar |
| Google Maps navigation | ✓ | ✗ | iPhone does not send navigation data to Bluetooth devices |
| Weather | ✓ | ✗ | Weather is a Chronos app feature (not in iOS) |
| Phone battery on screen | ✓ | ? | Shown only if the iPhone provides it |

In iPhone mode the **Navigation** menu and **Phone → Weather** are hidden.

---

## 3. Disconnecting / changing phones

1. On the iPhone: **Settings → Bluetooth → ⓘ next to cute_device → Forget This Device**.
2. On the watch: **Settings → Forget phones (Hapus pairing HP)**, press **OK** twice ("OK again?" then "Deleted").
   This works in Android and iPhone mode, removes every paired phone (also the Camera remote pairing)
   and restarts Bluetooth, so the phone has to pair from scratch.
3. Pair the new iPhone with the steps in section 1.

Back to Android: **Menu → Phone → Phone mode → Android**, then connect from the Chronos app as usual.

---

## 4. Problems & fixes

**The watch does not show up in the iPhone's Bluetooth list**
- On the watch make sure **Settings → Bluetooth = ON** and **Phone mode = iPhone** (top-right corner says **iOS**).
- Still missing: install the free **nRF Connect** or **LightBlue** app from the App Store,
  find **cute_device**, tap **Connect** once → the iPhone asks to pair. You can close the app afterwards.

**Notifications do not arrive**
- During pairing the **"Share System Notifications"** question must be answered **Allow**. If you missed it:
  forget the device on the iPhone, **Forget phones** on the watch, then pair again.
- Pairing keeps failing (e.g. a phone that was paired before): forget the device on the iPhone, run
  **Settings → Forget phones** on the watch, turn the iPhone's Bluetooth off and on, then pair again.
- On the watch: **Settings → Notif pop-up** must not be **OFF** (choose **WhatsApp** or **All**).
- The notification has to actually appear on the iPhone (chat not open, not muted, Focus/Do Not Disturb off).

**Message text is empty, only the sender's name**
- Set WhatsApp **Show Previews** to **Always** (see section 1).

**Clock not synced (Home shows "--:--")**
- The clock syncs only after pairing succeeds and it is connected (top-right **BT**).
- If it already says **BT** but the clock is still "--:--", reconnect: turn the iPhone's Bluetooth off and on.

**Music does not show**
- Play a song in an iPhone music app (Apple Music, Spotify, YouTube Music), then open **Phone → Music** on the watch.

---

## Camera (take a photo with the built-in camera from the watch): iPhone & Android

**Phone → Camera** makes the watch appear as a Bluetooth media button called **cute_device Remote**
(with its own Bluetooth address, so it shows up as a separate device from "cute_device"). While the Camera
screen is open, notifications/music/time from the phone stop; BACK exits and the watch returns to its normal phone mode.
Camera pairing is stored separately, so the iPhone pairing for notifications is not affected.

**REQUIRED after updating to v1.8.5:** the HID layout changed (the Scroll feature was removed). On the phone choose
**Forget device** for "cute_device Remote", then pair again.

**Pair once:**
1. Open **Phone → Camera** on the watch.
2. On the phone: **Settings → Bluetooth** → tap **cute_device Remote** → Pair.

**Taking a photo:** open the phone's **built-in Camera** app, then on the watch:

| Button | Action |
|---|---|
| Left / right | Timer down / up: OFF → 3 → 5 → 10 seconds (saved) |
| OK | Timer OFF = snap immediately; otherwise count down, then snap |
| BACK | During the countdown = cancel; otherwise = exit |

Snap = the **Volume Up** button, which works as the shutter in the iPhone & Android Camera apps.
The watch stores up to **3 Camera pairings** (separate from normal pairing); when full, a new phone is rejected.
