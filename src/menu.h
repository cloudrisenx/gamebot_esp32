#pragma once
#include <Arduino.h>
#include <gfxfont.h>

enum AppState { ST_BOOT, ST_MENU, ST_GAME, ST_NAV, ST_ANIM, ST_SETTINGS, ST_OTA, ST_PHONE, ST_HOME, ST_TEXT };

// Setiap fungsi *_update membaca tombol, menggambar bila perlu,
// dan mengembalikan state berikutnya.
AppState menu_update(bool redraw);
AppState settings_screen_update(bool redraw);
AppState nav_screen_update(bool redraw);

// Pop-up WhatsApp: isi mulai baris topLine (bergulir); mengembalikan jumlah baris isi
int popup_draw(const char *sender, const char *msg, int topLine = 0);
static const int POPUP_LINES = 5;  // baris isi yang terlihat di pop-up
void call_draw(const char *caller, bool blink, const char *hint);  // layar panggilan masuk
// Layar "Hanya untuk Android" untuk fitur Chronos saat Mode HP = iPhone (tanpa flush)
void draw_androidOnly(const char *title, const char *line1, const char *line2);

// Helper UI landscape yang juga dipakai submenu Game
void drawHeader(const String &center);
String header_status();  // "BT 85%" / "bt" / "" (kanan atas)
void beginRow(int y, int width, int h, bool selected);
void printRight(int rightX, int y, const String &s);
String asciiOnly(const char *s);  // buang karakter non-ASCII (font GFX hanya ASCII)
void drawWrapped(int x, int y, int cols, int maxLines, const String &text);

// Teks satu baris di area x..x+w-1 (terpotong rapi di tepi area, warna latar dibiarkan).
// y seperti setCursor: atas teks untuk font bawaan, baseline untuk font GFX.
// Teks yang lebih lebar dari area berjalan ke samping (marquee) bila animate=true:
// diam 1 dtk, lalu geser 1 px tiap 50 ms, dua salinan teks agar mulus.
void marquee_draw(int x, int y, int w, const char *text, uint16_t color = 1, const GFXfont *font = nullptr,
                  bool animate = true);
void marquee_reset();      // mulai lagi dari jeda awal (layar/pilihan/teks baru)
bool marquee_frameDue();   // marquee sedang berjalan & layar menyala & sudah waktunya frame berikutnya

// Carousel horizontal (menu utama, HP, Game): ikon 32x32 di tengah, nama di bawah, panah < >,
// titik halaman. Pindah item melingkar dengan animasi geser singkat.
struct Carousel {
  int sel;             // item aktif
  int from;            // item sebelum pindah (untuk animasi)
  int8_t dir;          // +1 = ke item berikutnya (masuk dari kanan), -1 = sebelumnya
  int8_t shown;        // frame animasi yang terakhir digambar
  uint32_t animStart;
};
bool carousel_input(Carousel &c, int count);  // tombol kiri/kanan (MENU_SWAP_LR); true bila pindah
bool carousel_frameDue(Carousel &c);           // animasi berjalan & frame berikutnya perlu digambar
// Gambar layar penuh (tanpa flush). icons: array ikon 32x32 PROGMEM; name: nama item aktif.
void carousel_draw(Carousel &c, const String &title, const uint8_t *const *icons, int count, const char *name);

// Kartu geser (Notifikasi, Setting): satu item per layar, memakai input & animasi Carousel.
// content(index, dx) menggambar isi kartu (y 12..52) bergeser dx px; baris bawah: hint kiri, "n/N" kanan.
void card_draw(Carousel &c, const String &title, int count, void (*content)(int index, int dx), const char *hint);
// Word-wrap teks UTF-8 tanpa alokasi (byte non-ASCII dibuang), font 6x8. Menggambar maxLines baris
// mulai baris ke-firstLine; mengembalikan jumlah baris seluruh teks (maxLines 0 = hanya hitung).
int drawWrappedAscii(int x, int y, int cols, int maxLines, const char *text, int firstLine = 0);

// Redraw berbasis perubahan: layar hanya dikirim ulang ke OLED bila ada input (redraw=true)
// atau "tanda" isinya berubah. Tanda dihitung paling sering tiap periodMs.
struct RedrawWatch {
  uint32_t lastCheck;
  uint32_t shown;
};
uint32_t sig_mix(uint32_t h, uint32_t v);         // gabung nilai ke tanda (FNV-1a)
uint32_t sig_str(uint32_t h, const char *s);
uint32_t header_signature();                      // jam HH:MM, status BT, baterai HP
bool redraw_needed(RedrawWatch &w, bool redraw, uint32_t (*sigFn)(), uint32_t periodMs = 1000);
