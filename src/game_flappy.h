#pragma once
#include <Arduino.h>

// Flappy, mode landscape 128x64. OK/kiri = kepak.
void flappy_enter();
bool flappy_update(bool redraw);  // false = BACK (keluar ke daftar game)
void flappy_resume();
bool flappy_isPlaying();
