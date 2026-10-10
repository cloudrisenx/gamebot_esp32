#pragma once
#include "menu.h"

// Pemutar frames.bin. OK = pause/lanjut, UP/DOWN = kecepatan, BACK = keluar.
void anim_enter();
AppState anim_update(bool redraw);
void anim_resume();     // panggil setelah dijeda pop-up
bool anim_isPlaying();  // sedang diputar (bukan pause / kosong)
