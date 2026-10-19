#include "game_duel.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "settings.h"
#include "screen.h"
#include "games.h"
#include <esp_timer.h>

extern Adafruit_SSD1306 display;

static const int WIN_POINTS = 5;
static const uint32_t WAIT_MIN_MS = 1500, WAIT_MAX_MS = 5000;  // jeda acak "Siap..." sampai DOR
static const uint32_t RESULT_MS = 1500;                         // jeda hasil ronde
static const uint32_t FLASH_MS = 120;                           // kedip terbalik saat DOR
static const uint32_t EXIT_HOLD_MS = 5000;                      // 2P: tahan kanan + OK untuk keluar
// CPU: reaksi acak [lo, hi] ms, turun 20 ms per ronde yang dimenangkan pemain
static const int CPU_LO = 450, CPU_HI = 650, CPU_STEP = 20, CPU_LO_MIN = 230, CPU_HI_MIN = 330;

enum DuelMode : uint8_t { MODE_CPU, MODE_2P };
static DuelMode mode = MODE_CPU;  // disimpan di NVS "duel"/"mode"
static bool modeLoaded = false;

enum RoundState : uint8_t { RS_WAIT, RS_FIRE, RS_RESULT };

static GamePhase phase;
static RoundState rs;
static uint32_t stateAt;          // awal state ronde sekarang
static uint32_t fireAt;           // waktu DOR (millis, jadwal)
static uint32_t cpuDelayMs;       // reaksi CPU ronde ini
static uint64_t fireUs = 0;       // waktu DOR sebenarnya (esp_timer µs), 0 = belum
static bool conf1, conf2;         // tekan P1/P2 sudah dikonfirmasi debounce (stempel bukan derau)
static bool roundTie;             // seri (< 1 ms): ronde diulang tanpa poin
static int pts1, pts2;
static uint8_t roundWinner;       // 1 / 2
static bool foul;                 // ronde dimenangkan karena lawan curang
static uint32_t reactTenths;     // waktu reaksi pemenang, 0,1 ms
static int matchWinner;
static bool exitHolding = false;
static uint32_t exitHoldStart = 0;

static void loadMode() {
  if (modeLoaded) return;
  Preferences p;
  if (p.begin("duel", true)) { mode = p.getUChar("mode", MODE_CPU) == MODE_2P ? MODE_2P : MODE_CPU; p.end(); }
  modeLoaded = true;
}

static void saveMode() {
  Preferences p;
  if (p.begin("duel", false)) { p.putUChar("mode", mode); p.end(); }
}

static void startRound() {
  rs = RS_WAIT;
  stateAt = millis();
  fireAt = stateAt + WAIT_MIN_MS + random(WAIT_MAX_MS - WAIT_MIN_MS + 1);
  int lo = max(CPU_LO_MIN, CPU_LO - CPU_STEP * pts1), hi = max(CPU_HI_MIN, CPU_HI - CPU_STEP * pts1);
  cpuDelayMs = lo + random(hi - lo + 1);
  fireUs = 0;
  conf1 = conf2 = false;
  roundTie = false;
  btn_armTiming();  // tekan sebelum DOR pun tercatat (= curang)
}

static void resetGame() {
  loadMode();
  pts1 = pts2 = 0;
  matchWinner = 0;
  exitHolding = false;
  phase = PH_READY;
}

static void endRound(uint8_t winner, bool byFoul, uint64_t shotUs) {
  roundWinner = winner;
  foul = byFoul;
  roundTie = false;
  reactTenths = !byFoul && fireUs && shotUs >= fireUs ? (uint32_t)((shotUs - fireUs) / 100) : 0;
  if (winner == 1) pts1++; else pts2++;
  rs = RS_RESULT;
  stateAt = millis();
}

static void tieRound() {
  roundTie = true;
  roundWinner = 0;
  rs = RS_RESULT;
  stateAt = millis();
}

// ---- Penentuan pemenang dari stempel waktu interrupt (fungsi murni; disalin ke uji Python) ----
static const uint64_t TIE_US = 1000;      // selisih < 1 ms = seri -> ulang ronde
static const uint64_t CONFIRM_US = 60000;  // tunggu 60 ms sesudah tembakan tercepat (debounce lawan)
struct Shot { bool valid; uint64_t us; };  // valid = ada stempel & terkonfirmasi (CPU selalu)
enum VerdictKind : uint8_t { V_NONE, V_FOUL, V_WIN, V_TIE };
struct Verdict { VerdictKind kind; int who; uint64_t us; };  // who = indeks penembak (0 = P1, 1 = P2/CPU)

static Verdict judge(const Shot *shots, int n, uint64_t fireUsNow, uint64_t nowUs) {
  Verdict v = {V_NONE, -1, 0};
  for (int i = 0; i < n; i++)
    if (shots[i].valid && (v.who < 0 || shots[i].us < v.us)) { v.who = i; v.us = shots[i].us; }
  if (v.who < 0 || nowUs - v.us < CONFIRM_US) { v.who = -1; return v; }  // tunggu lawan yang hampir bersamaan
  for (int i = 0; i < n; i++)
    if (i != v.who && shots[i].valid && shots[i].us - v.us < TIE_US) { v.kind = V_TIE; return v; }
  v.kind = (!fireUsNow || v.us < fireUsNow) ? V_FOUL : V_WIN;
  return v;
}

static void afterResult() {
  if (pts1 >= WIN_POINTS || pts2 >= WIN_POINTS) {
    matchWinner = pts1 >= WIN_POINTS ? 1 : 2;
    phase = PH_OVER;
    if (mode == MODE_CPU && (uint32_t)pts1 > settings_hiscore(GAME_DUEL)) settings_setHiscore(GAME_DUEL, pts1);
    return;
  }
  startRound();
}

static void centerLine(int y, const char *s) {
  display.setCursor((128 - (int)strlen(s) * 6) / 2, y);
  display.print(s);
}

// Koboi 20x36 di (x, y kiri atas). facingRight = pistol ke kanan. fallen = rebah di tanah.
static void drawCowboy(int x, bool facingRight, bool fallen, uint16_t c) {
  if (fallen) {
    int gy = 56;
    display.fillRect(x, gy - 3, 20, 3, c);                     // badan rebah
    display.fillCircle(facingRight ? x + 22 : x - 3, gy - 3, 3, c);  // kepala (rebah ke arah tengah)
    display.drawFastHLine(facingRight ? x + 19 : x - 9, gy - 8, 10, c);  // topi jatuh
    display.fillRect(facingRight ? x + 21 : x - 7, gy - 11, 6, 3, c);
    return;
  }
  const int y = 25;                                            // kaki di garis tanah (y 61)
  int cx = x + 10;
  display.drawFastHLine(cx - 7, y + 6, 15, c);                 // pinggir topi
  display.fillRect(cx - 4, y, 9, 6, c);                        // topi
  display.fillCircle(cx, y + 10, 3, c);                        // kepala
  display.fillRect(cx - 3, y + 14, 7, 11, c);                  // badan
  display.drawLine(cx - 2, y + 25, cx - 5, y + 35, c);          // kaki
  display.drawLine(cx + 2, y + 25, cx + 5, y + 35, c);
  int d = facingRight ? 1 : -1;                                // lengan + pistol ke arah lawan
  display.drawLine(cx + d * 3, y + 16, cx + d * 9, y + 18, c);
  display.fillRect(facingRight ? cx + 8 : cx - 11, y + 16, 4, 2, c);
}

static void draw() {
  uint32_t now = millis();
  bool flash = rs == RS_FIRE && now - fireAt < FLASH_MS && phase == PH_PLAY;
  uint16_t fg = flash ? SSD1306_BLACK : SSD1306_WHITE;
  display.clearDisplay();
  if (flash) display.fillRect(0, 0, 128, 64, SSD1306_WHITE);
  display.setTextSize(1);
  display.setTextColor(fg);

  char buf[24];
  snprintf(buf, sizeof(buf), "P1 %d", pts1);
  display.setCursor(2, 0); display.print(buf);
  snprintf(buf, sizeof(buf), "%s %d", mode == MODE_2P ? "P2" : "CPU", pts2);
  display.setCursor(126 - (int)strlen(buf) * 6, 0); display.print(buf);
  display.drawFastHLine(0, 61, 128, fg);

  bool show = rs == RS_RESULT || phase == PH_OVER;
  drawCowboy(6, true, show && roundWinner == 2, fg);
  drawCowboy(102, false, show && roundWinner == 1, fg);

  if (phase == PH_PLAY || phase == PH_OVER) {
    if (rs == RS_WAIT) {
      centerLine(28, T("Siap...", "Ready..."));
    } else if (rs == RS_FIRE) {
      display.setTextSize(2);
      const char *fire = T("DOR!", "BANG!");
      display.setCursor(64 - (int)strlen(fire) * 6, 26);
      display.print(fire);
      display.setTextSize(1);
    } else {
      const char *who = roundWinner == 1 ? "P1" : (mode == MODE_2P ? "P2" : "CPU");
      if (roundTie) {
        centerLine(20, T("SERI! ulang", "DRAW! again"));
        snprintf(buf, sizeof(buf), T("beda < 1ms", "diff < 1ms"));  // maks 10 karakter: tidak menabrak tangan koboi
      } else if (foul) {
        centerLine(20, T("CURANG!", "FOUL!"));
        snprintf(buf, sizeof(buf), "%s +1", who);
      } else {
        snprintf(buf, sizeof(buf), "%s +1", who);
        centerLine(20, buf);
        snprintf(buf, sizeof(buf), "%lu,%lu ms", (unsigned long)(reactTenths / 10), (unsigned long)(reactTenths % 10));
      }
      centerLine(32, buf);
    }
  }

  if (phase == PH_OVER) {
    if (mode == MODE_CPU) games_drawOver(pts1, settings_hiscore(GAME_DUEL));
    else {
      const char *l1 = matchWinner == 1 ? T("P1 MENANG", "P1 WINS") : T("P2 MENANG", "P2 WINS");
      int w = 9 * 6 + 10, x = (128 - w) / 2;
      display.fillRect(x, 20, w, 24, SSD1306_BLACK);
      display.drawRect(x, 20, w, 24, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
      centerLine(24, l1);
      centerLine(33, T("OK: ulang", "OK: again"));
    }
  }
  if (exitHolding) {  // tahan kanan + OK: bar menuju keluar
    uint32_t held = min((uint32_t)(now - exitHoldStart), EXIT_HOLD_MS);
    display.fillRect(24, 48, 80, 14, SSD1306_BLACK);
    display.drawRect(24, 48, 80, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(28, 50); display.print(T("Keluar...", "Exiting..."));
    display.fillRect(28, 58, held * 72 / EXIT_HOLD_MS, 2, SSD1306_WHITE);
  }
  oled_flush();
}

static void drawReady() {
  static char modeLine[16];
  snprintf(modeLine, sizeof(modeLine), "< %s >", mode == MODE_2P ? "2P vs 2P" : "1P vs CPU");
  static const char *controls[3];
  controls[0] = modeLine;
  controls[1] = T("Tunggu DOR!, tekan", "Wait for BANG!, press");
  controls[2] = mode == MODE_2P ? T("P1 kiri, P2 BACK", "P1 left, P2 BACK") : T("Kiri: tembak", "Left: shoot");
  games_drawReady("DUEL", controls, 3, settings_hiscore(GAME_DUEL));
}

void duel_enter() { resetGame(); }

void duel_resume() {
  // Pop-up memotong ronde: ulang ronde agar waktu reaksi tidak kacau
  if (phase == PH_PLAY && rs != RS_RESULT) startRound();
}

bool duel_isPlaying() { return phase == PH_PLAY; }

bool duel_update(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  bool ok = btn_pressed(BTN_OK);
  bool p1 = btn_pressed(leftBtn);
  bool right = btn_pressed(rightBtn);
  bool back = btn_pressed(BTN_BACK);

  if (phase == PH_READY) {
    if (back) return false;
    if (p1 || right) { mode = mode == MODE_2P ? MODE_CPU : MODE_2P; saveMode(); redraw = true; }
    if (!ok) { if (redraw) drawReady(); return true; }
    pts1 = pts2 = 0;
    phase = PH_PLAY;
    startRound();
    draw();
    return true;
  }

  bool p2 = false;
  if (mode == MODE_2P) {
    p2 = back;  // BACK = tombol P2; keluar hanya tahan kanan + OK 5 dtk
    if (btn_held(rightBtn) && btn_held(BTN_OK)) {
      if (!exitHolding) { exitHolding = true; exitHoldStart = millis(); }
      if (millis() - exitHoldStart >= EXIT_HOLD_MS) { exitHolding = false; return false; }
      redraw = true;
    } else if (exitHolding) {
      exitHolding = false;
      redraw = true;
    }
  } else if (back) {
    if (phase == PH_PLAY && (uint32_t)pts1 > settings_hiscore(GAME_DUEL)) settings_setHiscore(GAME_DUEL, pts1);
    return false;
  }

  if (phase == PH_OVER) {
    if (ok && !exitHolding) { resetGame(); drawReady(); return true; }
    if (redraw) draw();
    return true;
  }

  uint32_t now = millis();
  uint64_t nowUs = esp_timer_get_time();
  RoundState before = rs;
  if (rs == RS_WAIT || rs == RS_FIRE) {
    if (rs == RS_WAIT && (int32_t)(now - fireAt) >= 0) { rs = RS_FIRE; stateAt = now; fireUs = nowUs; }
    // Stempel interrupt menentukan siapa duluan; debounce (btn_pressed/btn_held) hanya membuktikan bukan derau
    conf1 |= p1 || btn_held(leftBtn);
    if (mode == MODE_2P) conf2 |= p2 || btn_held(BTN_BACK);
    Shot shots[2];
    uint64_t t;
    shots[0].valid = btn_pressTimeUs(leftBtn, t) && conf1; shots[0].us = t;
    if (mode == MODE_2P) {
      shots[1].valid = btn_pressTimeUs(BTN_BACK, t) && conf2; shots[1].us = t;
    } else {  // CPU tidak pernah curang: tembak cpuDelayMs sesudah DOR
      shots[1].us = fireUs + (uint64_t)cpuDelayMs * 1000;
      shots[1].valid = fireUs && nowUs >= shots[1].us;
    }
    Verdict v = judge(shots, 2, fireUs, nowUs);
    if (v.kind != V_NONE) {
      uint64_t s1 = 0, s2 = 0;
      btn_pressTimeUs(leftBtn, s1);
      s2 = mode == MODE_2P ? (btn_pressTimeUs(BTN_BACK, s2), s2) : (shots[1].valid ? shots[1].us : 0);
      float r1 = s1 && fireUs ? ((int64_t)(s1 - fireUs)) / 1000.0f : 0, r2 = s2 && fireUs ? ((int64_t)(s2 - fireUs)) / 1000.0f : 0;
      float diff = s1 && s2 ? fabsf(((int64_t)(s1 - s2)) / 1000.0f) : 0;
    }
    if (v.kind == V_TIE) tieRound();
    else if (v.kind == V_FOUL) endRound(v.who == 0 ? 2 : 1, true, v.us);  // yang curang kalah ronde
    else if (v.kind == V_WIN) endRound(v.who == 0 ? 1 : 2, false, v.us);
  } else if (now - stateAt >= RESULT_MS) {
    if (roundTie) startRound();
    else afterResult();
  }
  if (rs != before || rs == RS_FIRE) redraw = true;  // RS_FIRE: kedip terbalik lalu normal
  if (redraw) draw();
  return true;
}
