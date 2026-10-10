#pragma once
#include <Arduino.h>

// Breakout, area tengah layar landscape. Tahan kiri/kanan = gerak, OK = luncurkan bola.
void breakout_enter();
bool breakout_update(bool redraw);  // false = BACK (keluar ke daftar game)
void breakout_resume();
bool breakout_isPlaying();
