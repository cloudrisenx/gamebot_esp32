#pragma once
#include <Arduino.h>

// XO (Tic-tac-toe), landscape, papan 3x3 di tengah. Mode dipilih di layar READY (kiri/kanan, tersimpan):
//  1P vs CPU: kamu X, CPU O (minimax, kadang salah langkah); hi-score = menang beruntun; BACK = keluar.
//  2P       : X dan O bergantian di perangkat yang sama; keluar = tahan tombol kanan + OK 5 dtk.
// Kiri/kanan = pindah kursor ke kotak kosong sebelumnya/berikutnya, OK = taruh tanda.
void ttt_enter();
bool ttt_update(bool redraw);  // false = keluar ke daftar game
void ttt_resume();
bool ttt_isPlaying();
