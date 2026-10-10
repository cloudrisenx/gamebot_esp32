#pragma once
#include <Arduino.h>

// Dino Run, mode landscape 128x64. OK/kiri = lompat, tahan kanan = merunduk.
void dino_enter();
bool dino_update(bool redraw);  // false = BACK (keluar ke daftar game)
void dino_resume();
bool dino_isPlaying();
