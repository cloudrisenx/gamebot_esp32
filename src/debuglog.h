#pragma once
#include <stddef.h>

// Log kecil di RAM untuk layar Setting > Debug log (pengganti Serial monitor).
// 10 baris terakhir, maks 21 karakter (lebar OLED), diawali detik sejak boot. Hilang saat restart.
// Aman dipanggil dari task BLE (tanpa heap/I/O).
static const int DLOG_LINES = 10, DLOG_COLS = 21;

void dlog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int dlog_count();
unsigned dlog_seq();  // naik setiap baris baru (untuk gambar ulang)
void dlog_line(int i, char *out, size_t len);  // i = 0 paling lama
void dlog_clear();
