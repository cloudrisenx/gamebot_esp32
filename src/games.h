#pragma once
#include "menu.h"
#include "buttons.h"

// Submenu Game: daftar game, lalu meneruskan update ke game aktif.
void games_enter();
AppState games_update(bool redraw);
void games_resume();     // setelah pop-up: kembalikan orientasi & timer game aktif
bool games_isPlaying();  // game aktif sedang fase PLAY (blokir deep sleep)

// ---- Dipakai oleh tiap game ----
// Setiap game: xxx_enter(), bool xxx_update(redraw) (false = BACK, keluar ke daftar),
// xxx_resume(), xxx_isPlaying().

enum GamePhase { PH_READY, PH_PLAY, PH_OVER };

// Fixed-step 30 ms; jeda panjang (pop-up, macet) tidak membuat game melompat.
struct FixedStep {
  static const uint32_t TICK_MS = 30;
  uint32_t last = 0;
  void reset() { last = millis(); }
  int due() {
    uint32_t now = millis();
    if (now - last > 10 * TICK_MS) last = now - TICK_MS;
    int n = 0;
    while (now - last >= TICK_MS) { last += TICK_MS; n++; }
    return n;
  }
};

void games_centerText(int y, const String &s);
// Layar READY: judul, petunjuk "Putar layar" (potret), baris kontrol, Best, "OK: mulai"
void games_drawReady(const char *title, const char *const *controls, int count, uint32_t best);
// Kotak GAME OVER di atas layar game
void games_drawOver(uint32_t score, uint32_t best);
// Titik nyawa di kanan atas
void games_drawLives(int lives);

// Area main "potret" di tengah layar landscape (Breakout, Pong): x FIELD_X0..FIELD_X0+FIELD_W-1
static const int FIELD_X0 = 32, FIELD_W = 64;
// Panel retro statis kiri/kanan: SCORE/HI di kiri, LIVES/LV di kanan, bingkai garis ganda + hiasan piksel
void games_drawSidePanels(uint32_t score, uint32_t hi, int lives, int level);

// ---- Tombol P2 (mode 2P, P2 duduk berhadapan dengan P1) ----
// Satu-satunya sumber arah P2 (dipakai Pong & Hitung). Dari sudut pandang P2: BACK = kiri/minus, OK = kanan/plus.
// Bila di perangkat masih terasa terbalik, cukup tukar dua tombol ini.
inline Btn games_p2MinusBtn() { return BTN_BACK; }
inline Btn games_p2PlusBtn() { return BTN_OK; }
// -1 = ke kiri P2, +1 = ke kanan P2, 0 = diam/keduanya ditahan (status tahan)
inline int games_p2Dir() { return (btn_held(games_p2PlusBtn()) ? 1 : 0) - (btn_held(games_p2MinusBtn()) ? 1 : 0); }
