#pragma once
#include <Arduino.h>

// Hitung, landscape. Benda tampil sekilas, lalu tebak jumlah benda target. 10 soal (Lv 1-5, 2 soal per level).
// Jawab: kanan = +1, kiri = -1 (tahan = ulang otomatis), OK = kunci; batas 8 dtk. Tepat +2, selisih 1 +1.
// Mode dipilih di layar READY (kiri/kanan, tersimpan):
//  1P vs CPU: hi-score = total poin pemain.
//  2P gantian: P1 jawab, layar "Giliran P2" menutup jawaban P1, lalu P2 jawab dengan tombol yang sama.
// BACK = keluar (di tengah permainan tahan 1 dtk).
void hitung_enter();
bool hitung_update(bool redraw);  // false = keluar ke daftar game
void hitung_resume();
bool hitung_isPlaying();
