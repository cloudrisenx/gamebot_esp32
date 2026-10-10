#include "game_ttt.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "settings.h"
#include "screen.h"
#include "games.h"

extern Adafruit_SSD1306 display;

// Papan 3x3, sel 20 px: x 34..93, y 2..61; panel X di kiri, panel O di kanan
static const int CELL = 20;
static const int BX = 34, BY = 2;
static const uint32_t EXIT_HOLD_MS = 5000;  // 2P: tahan kanan + OK untuk keluar
static const uint32_t CPU_DELAY_MS = 400;
static const int CPU_RANDOM_PCT = 20;       // peluang CPU memilih langkah acak (bisa dikalahkan)

enum TttMode : uint8_t { MODE_CPU, MODE_2P };
static TttMode mode = MODE_CPU;             // disimpan di NVS "ttt"/"mode"
static bool modeLoaded = false;

static const uint8_t LINES[8][3] = {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}, {0, 3, 6},
                                    {1, 4, 7}, {2, 5, 8}, {0, 4, 8}, {2, 4, 6}};

static GamePhase phase;
static uint8_t board[9];                    // 0 kosong, 1 = X, 2 = O
static uint8_t turn, firstTurn;             // pemain yang jalan / yang mulai ronde ini
static int cursor;
static int8_t winLine;                      // -1 = belum ada
static bool roundOver;
static uint8_t result;                      // 0 seri, 1 X menang, 2 O menang
static uint32_t winsX, winsO;               // 1P: winsX = menang beruntun, winsO = CPU menang
static uint32_t cpuAt;                      // waktu CPU boleh menaruh
static uint32_t roundOverAt;                // kotak hasil muncul 1 dtk setelahnya (garis menang terlihat dulu)
static bool exitHolding = false;
static uint32_t exitHoldStart = 0;

static void loadMode() {
  if (modeLoaded) return;
  Preferences p;
  if (p.begin("ttt", true)) { mode = p.getUChar("mode", MODE_CPU) == MODE_2P ? MODE_2P : MODE_CPU; p.end(); }
  modeLoaded = true;
}

static void saveMode() {
  Preferences p;
  if (p.begin("ttt", false)) { p.putUChar("mode", mode); p.end(); }
}

static int findLine(const uint8_t *b) {
  for (int i = 0; i < 8; i++) {
    uint8_t v = b[LINES[i][0]];
    if (v && v == b[LINES[i][1]] && v == b[LINES[i][2]]) return i;
  }
  return -1;
}

static bool boardFull() {
  for (int i = 0; i < 9; i++) if (!board[i]) return false;
  return true;
}

// Kursor ke kotak kosong berikutnya (dir +1) / sebelumnya (dir -1), berputar
static void moveCursor(int dir) {
  for (int k = 1; k <= 9; k++) {
    int c = ((cursor + dir * k) % 9 + 9) % 9;
    if (!board[c]) { cursor = c; return; }
  }
}

static void cursorToEmpty() {
  if (board[cursor]) moveCursor(1);
}

static bool cpuTurn() { return mode == MODE_CPU && turn == 2 && !roundOver; }

static void startRound() {
  memset(board, 0, sizeof(board));
  turn = firstTurn;
  cursor = 4;
  winLine = -1;
  roundOver = false;
  exitHolding = false;
  cpuAt = millis() + CPU_DELAY_MS;
}

static void resetGame() {
  loadMode();
  winsX = winsO = 0;
  firstTurn = 1;
  startRound();
  phase = PH_READY;
}

// Minimax + alpha-beta; nilai dari sudut pandang O (CPU), lebih cepat menang = lebih baik
static int minimax(uint8_t *b, uint8_t who, int depth, int alpha, int beta) {
  int l = findLine(b);
  if (l >= 0) return b[LINES[l][0]] == 2 ? 10 - depth : depth - 10;
  bool any = false;
  int best = who == 2 ? -100 : 100;
  for (int i = 0; i < 9; i++) {
    if (b[i]) continue;
    any = true;
    b[i] = who;
    int v = minimax(b, who == 2 ? 1 : 2, depth + 1, alpha, beta);
    b[i] = 0;
    if (who == 2) { best = max(best, v); alpha = max(alpha, v); }
    else { best = min(best, v); beta = min(beta, v); }
    if (beta <= alpha) break;
  }
  return any ? best : 0;
}

static int cpuMove() {
  int empty[9], n = 0;
  for (int i = 0; i < 9; i++) if (!board[i]) empty[n++] = i;
  if (n == 0) return -1;
  if (n == 9 || random(100) < CPU_RANDOM_PCT) return empty[random(n)];
  int bestV = -1000, cand[9], nc = 0;
  for (int k = 0; k < n; k++) {
    int i = empty[k];
    board[i] = 2;
    int v = minimax(board, 1, 1, -1000, 1000);
    board[i] = 0;
    if (v > bestV) { bestV = v; nc = 0; }
    if (v == bestV) cand[nc++] = i;
  }
  return cand[random(nc)];
}

static void place(int i) {
  board[i] = turn;
  winLine = findLine(board);
  if (winLine >= 0 || boardFull()) {
    roundOver = true;
    roundOverAt = millis();
    result = winLine >= 0 ? turn : 0;
    if (mode == MODE_CPU) {
      if (result == 1) {
        winsX++;
        if (winsX > settings_hiscore(GAME_TTT)) settings_setHiscore(GAME_TTT, winsX);
      } else if (result == 2) {
        winsX = 0;  // beruntun putus
        winsO++;
      }
    } else {
      if (result == 1) winsX++;
      if (result == 2) winsO++;
    }
    return;
  }
  turn = turn == 1 ? 2 : 1;
  cursorToEmpty();
  cpuAt = millis() + CPU_DELAY_MS;
}

static void centerAt(int cx, int y, const char *s) {
  display.setCursor(cx - (int)strlen(s) * 3, y);
  display.print(s);
}

static void drawMark(int x, int y, uint8_t v, uint16_t c) {
  if (v == 1) {
    for (int o = 0; o < 2; o++) {
      display.drawLine(x + 4 + o, y + 4, x + 15 + o, y + 15, c);
      display.drawLine(x + 15 + o, y + 4, x + 4 + o, y + 15, c);
    }
  } else if (v == 2) {
    display.drawCircle(x + 10, y + 10, 6, c);
    display.drawCircle(x + 10, y + 10, 5, c);
  }
}

static void drawPanel(int x0, uint8_t who) {
  int cx = x0 + 16;
  bool active = !roundOver && turn == who;
  display.setTextSize(2);
  if (active) display.fillRect(cx - 9, 2, 18, 18, SSD1306_WHITE);
  display.setTextColor(active ? SSD1306_BLACK : SSD1306_WHITE);
  display.setCursor(cx - 5, 4);
  display.print(who == 1 ? "X" : "O");
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  const char *name = mode == MODE_2P ? (who == 1 ? "P1" : "P2") : (who == 1 ? "KAMU" : "CPU");
  centerAt(cx, 24, name);
  char buf[12];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)(who == 1 ? winsX : winsO));
  centerAt(cx, 35, buf);
  if (active) centerAt(cx, 50, "MAIN");
  else if (mode == MODE_CPU && who == 1) {
    snprintf(buf, sizeof(buf), "Hi%lu", (unsigned long)settings_hiscore(GAME_TTT));
    centerAt(cx, 50, buf);
  }
}

static void draw() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  for (int k = 1; k < 3; k++) {
    display.drawFastVLine(BX + k * CELL - 1, BY, 3 * CELL, SSD1306_WHITE);
    display.drawFastHLine(BX, BY + k * CELL - 1, 3 * CELL, SSD1306_WHITE);
  }
  bool blink = (millis() / 300) % 2;
  for (int i = 0; i < 9; i++) {
    int x = BX + (i % 3) * CELL, y = BY + (i / 3) * CELL;
    bool hl = (winLine >= 0 && (LINES[winLine][0] == i || LINES[winLine][1] == i || LINES[winLine][2] == i));
    bool cur = !roundOver && !cpuTurn() && i == cursor;
    if (hl || (cur && blink)) {
      display.fillRect(x + 1, y + 1, CELL - 3, CELL - 3, SSD1306_WHITE);
      drawMark(x, y, board[i] ? board[i] : turn, SSD1306_BLACK);
    } else {
      drawMark(x, y, board[i], SSD1306_WHITE);
      if (cur) display.drawRect(x + 1, y + 1, CELL - 3, CELL - 3, SSD1306_WHITE);
    }
  }
  drawPanel(0, 1);
  drawPanel(96, 2);
  if (roundOver && millis() - roundOverAt >= 1000) {
    const char *msg = result == 0 ? "SERI"
                      : mode == MODE_2P ? (result == 1 ? "P1 MENANG" : "P2 MENANG")
                                        : (result == 1 ? "KAMU MENANG" : "CPU MENANG");
    int w = max((int)strlen(msg), 8) * 6 + 10;  // minimal selebar "OK: lagi"
    int x = (128 - w) / 2;
    display.fillRect(x, 22, w, 24, SSD1306_BLACK);
    display.drawRect(x, 22, w, 24, SSD1306_WHITE);
    centerAt(64, 26, msg);
    centerAt(64, 35, "OK: lagi");
  }
  if (exitHolding) {  // tahan kanan + OK: bar menuju keluar
    uint32_t held = min((uint32_t)(millis() - exitHoldStart), EXIT_HOLD_MS);
    display.fillRect(24, 48, 80, 14, SSD1306_BLACK);
    display.drawRect(24, 48, 80, 14, SSD1306_WHITE);
    display.setCursor(28, 50); display.print("Keluar...");
    display.fillRect(28, 58, held * 72 / EXIT_HOLD_MS, 2, SSD1306_WHITE);
  }
  oled_flush();
}

static void drawReady() {
  static char modeLine[16];
  snprintf(modeLine, sizeof(modeLine), "< %s >", mode == MODE_2P ? "2P vs 2P" : "1P vs CPU");
  static const char *controls[3];
  controls[0] = modeLine;
  controls[1] = "Kiri/kanan: kotak";
  controls[2] = mode == MODE_2P ? "Keluar: kanan+OK" : "OK: taruh";
  games_drawReady("XO", controls, 3, settings_hiscore(GAME_TTT));
}

void ttt_enter() { resetGame(); }

void ttt_resume() { cpuAt = millis() + CPU_DELAY_MS; }

bool ttt_isPlaying() { return phase == PH_PLAY; }

bool ttt_update(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  bool ok = btn_pressed(BTN_OK);
  bool left = btn_pressed(leftBtn), right = btn_pressed(rightBtn);
  bool back = btn_pressed(BTN_BACK);

  if (phase == PH_READY) {
    if (back) return false;
    if (left || right) { mode = mode == MODE_2P ? MODE_CPU : MODE_2P; saveMode(); redraw = true; }
    if (!ok) { if (redraw) drawReady(); return true; }
    startRound();
    phase = PH_PLAY;
    draw();
    return true;
  }

  if (mode == MODE_2P) {
    // BACK tidak dipakai; keluar hanya dengan tahan kanan + OK selama 5 dtk
    if (btn_held(rightBtn) && btn_held(BTN_OK)) {
      if (!exitHolding) { exitHolding = true; exitHoldStart = millis(); }
      if (millis() - exitHoldStart >= EXIT_HOLD_MS) { exitHolding = false; return false; }
      redraw = true;
    } else if (exitHolding) {
      exitHolding = false;
      redraw = true;
    }
  } else if (back) {
    return false;  // hi-score (menang beruntun) sudah tersimpan tiap menang
  }

  if (roundOver) {
    if (ok && !exitHolding) {
      firstTurn = firstTurn == 1 ? 2 : 1;  // yang mulai bergantian tiap ronde
      startRound();
      redraw = true;
    }
  } else if (cpuTurn()) {
    if ((int32_t)(millis() - cpuAt) >= 0) {
      int m = cpuMove();
      if (m >= 0) place(m);
      redraw = true;
    }
  } else {
    if (left) { moveCursor(-1); redraw = true; }
    if (right) { moveCursor(1); redraw = true; }
    if (ok && !exitHolding && !board[cursor]) { place(cursor); redraw = true; }
  }

  static uint32_t lastBlink = 0;
  if (millis() - lastBlink >= 150) { lastBlink = millis(); redraw = true; }  // kedip kursor
  if (redraw) draw();
  return true;
}
