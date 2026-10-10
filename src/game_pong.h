#pragma once
#include <Arduino.h>

// Pong. Mode dipilih di layar READY (kiri/kanan, tersimpan):
//  1P vs CPU: landscape, area 64x64 di tengah + panel retro; bola lolos atas = poin, bawah = nyawa -1.
//  2P vs 2P : potret 64x128; P1 bawah (kiri/kanan potret), P2 atas (BACK = kiri P2, OK = kanan P2,
//             dari sudut pandang P2 yang berhadapan; lihat games_p2Dir); menang 7 poin;
//             keluar hanya dari layar menang: OK = ulang, BACK = keluar (BACK adalah kontrol P2 saat main).
void pong_enter();
bool pong_update(bool redraw);  // false = BACK (keluar ke daftar game)
void pong_resume();
bool pong_isPlaying();
