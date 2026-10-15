#pragma once
#include <stdint.h>

// Bahasa layar (Setting > Bahasa). Log Serial, kunci NVS, dan halaman web OTA tetap bahasa Indonesia.
enum Lang : uint8_t { LANG_ID, LANG_EN, LANG_COUNT };
extern uint8_t g_lang;  // settings.cpp (NVS "lang"); dibaca tiap gambar, jadi ganti bahasa langsung berlaku

// T("teks Indonesia", "English text"): pilih sesuai bahasa. Untuk tabel: LT(TABEL_ID, TABEL_EN)[i].
inline const char *T(const char *id, const char *en) { return g_lang == LANG_EN ? en : id; }
#define LT(id, en) (g_lang == LANG_EN ? (en) : (id))
