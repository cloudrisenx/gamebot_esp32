#pragma once

// Satu-satunya sumber versi firmware: dipakai layar Info, endpoint /info,
// penanda FWVER di file .bin (dibaca halaman OTA), dan tools/copy_ota.py.
#define FW_VERSION "1.9.29"

// Satu-satunya sumber nama perangkat: Bluetooth (Android & iPhone), WiFi OTA (+ "_OTA"), teks layar.
// Ganti juga web/ota.html & docs/IPHONE_GUIDE.md bila nama ini diubah.
#define DEVICE_NAME "cute_device"
