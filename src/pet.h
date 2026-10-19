#pragma once
#include "menu.h"

// Layar Pet: kucing peliharaan (mesin di pet_core). Dari Beranda: ketuk tombol kanan.
// UP (kiri) / DOWN (kanan) / OK: ketuk & tahan (lihat docs/PET.md). BACK = kembali ke Beranda.
void pet_enter();
AppState pet_update(bool redraw);
void pet_notify();      // notifikasi baru masuk saat layar Pet tampil: pet terkejut
bool pet_keepAwake();   // layar dipertahankan menyala selama < 5 menit sejak tombol terakhir
