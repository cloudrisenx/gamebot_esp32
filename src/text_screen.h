#pragma once
#include <Arduino.h>
#include "menu.h"

// Menu "Teks": running text besar. Kiri/kanan = kecepatan (3 tingkat), OK = jeda/lanjut, BACK = keluar.
// Teks diatur lewat halaman OTA (tab Teks) dan disimpan di NVS "rtext".
static const int RTEXT_MAX = 200;

void text_enter();
AppState text_update(bool redraw);
const char *text_get();                 // teks tersimpan sekarang (emoji = byte kode 0x01..)
String text_getUtf8();                  // sama, emoji dikembalikan ke UTF-8 (untuk halaman web)
bool text_set(const char *s, size_t n);  // simpan (emoji dikenal -> ikon, non-ASCII lain dibuang, maks RTEXT_MAX); false bila hasilnya kosong
