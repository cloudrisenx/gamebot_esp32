# ChronosESP32: local fork for cute_device

Source: [fbiego/chronos-esp32](https://github.com/fbiego/chronos-esp32) **v1.9.1** (copied from the PlatformIO
package). License unchanged (`LICENSE`, MIT). `platformio.ini` no longer lists `fbiego/ChronosESP32`, so PlatformIO
builds this folder. Do not update it through `lib_deps`; if you move to a newer upstream version, re-apply the patches
below. Every changed spot is marked `PATCH` in the source.

| # | File / place | Why |
|---|---|---|
| 1 | `ChronosESP32.cpp` `onWrite()`: start packet | Reject packets with `len < 4`, `len > CS_DATA_SIZE` or a declared length > `CS_DATA_SIZE` (the RX characteristic is writable without encryption; the original code copied up to 514 bytes into the 512-byte buffer). |
| 2 | `ChronosESP32.cpp` `onWrite()`: continuation packet | Accept only after a start packet and only if `20 + n*19 + len - 1 <= CS_DATA_SIZE` (original offset could reach 4865 bytes). The copy no longer writes the extra byte past the packet. |
| 3 | `ChronosESP32.cpp` `queuePacket()` (new) + `loop()` | Packets are assembled in `_assembly` (BLE task only); a complete packet is copied into a 4-slot queue; `loop()` (called from the app's main loop) copies it into `_incomingData` and runs `dataReceived()` (`_incomingData` is used only in the loop task). All Strings (music, navigation, weather, contacts...) and all app callbacks now run in the loop task, not concurrently with the app reading them. |
| 4 | `ChronosESP32.h` `setDataQueuedCallback()` (new) | Called when a packet is queued, so the app can wake its loop at once (`power_loopWake`). |
| 5 | `ChronosESP32.cpp` `dataReceived()` 0xA2 / 0xA3 | Contact index `% CS_CONTACTS_SIZE` (index 255 overflowed the 255-entry array). |
| 6 | `ChronosESP32.cpp` `dataReceived()` QR data | QR index `% CS_QR_SIZE` (index up to 255 into a 9-entry array). |
| 7 | `ChronosESP32.cpp` `dataReceived()` 0xEE navigation icon | Skip when `(pos + 1) * 96 > CS_ICON_DATA_SIZE` or the packet is shorter than the icon chunk. |
| 8 | `ChronosESP32.cpp` `stop()` | Clear the packet queue and both buffers, so packets of the old connection are not processed after the next `begin()`. |

Still not changed: the RX characteristic does not require encryption (a nearby device can still send valid Chronos
packets, e.g. a fake notification, while the phone is not connected).
