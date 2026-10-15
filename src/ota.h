#pragma once
#include "menu.h"
#include "version.h"


// Mode OTA: WiFi softAP + halaman upload di 192.168.4.1. BACK = batal.
void ota_enter();
AppState ota_update(bool redraw);

// OTA di latar (menu Teks): WiFi + halaman web sama, tanpa mengambil alih layar
void ota_bgStart();
void ota_bgStop();               // aman dipanggil walau tidak berjalan
void ota_bgPoll();               // panggil tiap loop selama berjalan (restart sendiri setelah update firmware)
bool ota_bgRunning();
const String &ota_ssid();        // berlaku setelah ota_bgStart / ota_enter
const String &ota_password();
