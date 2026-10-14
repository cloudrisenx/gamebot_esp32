#pragma once

// Satu-satunya sumber versi firmware: dipakai layar Info, endpoint /info,
// penanda FWVER di file .bin (dibaca halaman OTA), dan scripts/copy_ota.py.
#define FW_VERSION "1.9.22"

// Satu-satunya sumber nama perangkat: Bluetooth (Android & iPhone), WiFi OTA (+ "_OTA"), teks layar.
// Ganti juga web/ota.html & PANDUAN_IPHONE.md bila nama ini diubah.
#define DEVICE_NAME "cute_device"
