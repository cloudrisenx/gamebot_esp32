#pragma once
#include <Arduino.h>

// Duel (adu refleks), landscape. Mode dipilih di layar READY (kiri/kanan, tersimpan):
//  1P vs CPU: tombol kiri = tembak; BACK = keluar; hi-score = ronde dimenangkan dalam satu pertandingan.
//  2P       : P1 = tombol kiri, P2 = BACK; keluar = tahan tombol kanan + OK 5 dtk.
// Tunggu "DOR!" lalu tekan secepatnya; menekan sebelum DOR = curang (poin lawan). Pertama 5 poin menang.
void duel_enter();
bool duel_update(bool redraw);  // false = keluar ke daftar game
void duel_resume();
bool duel_isPlaying();
