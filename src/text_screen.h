#pragma once
#include <Arduino.h>
#include "menu.h"

// Menu "Teks": running text besar. Kiri/kanan = kecepatan (3 tingkat), OK = jeda/lanjut, BACK = keluar.
// Teks diatur lewat halaman OTA (tab Teks) dan disimpan di NVS "rtext".
static const int RTEXT_MAX = 200;

void text_enter();
AppState text_update(bool redraw);
const char *text_get();                 // teks tersimpan sekarang
bool text_set(const char *s, size_t n);  // simpan (non-ASCII dibuang, maks RTEXT_MAX); false bila hasilnya kosong
