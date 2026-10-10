#pragma once
#include "menu.h"

// Submenu HP: Notifikasi, Musik, Cuaca (lewat Chronos). BACK = kembali.
enum PhoneStart { PHONE_START_LIST, PHONE_START_NOTIF, PHONE_START_MUSIC };
void phone_requestStart(PhoneStart s);  // dipanggil sebelum masuk ST_PHONE (pintasan Beranda)
void phone_enter();
AppState phone_update(bool redraw);
