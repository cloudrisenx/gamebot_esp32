#pragma once
#include "menu.h"
#include "version.h"


// Mode OTA: WiFi softAP + halaman upload di 192.168.4.1. BACK = batal.
void ota_enter();
AppState ota_update(bool redraw);
