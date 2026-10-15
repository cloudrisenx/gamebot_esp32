#include "game_flappy.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include "settings.h"
#include "games.h"

extern Adafruit_SSD1306 display;

static const uint8_t PROGMEM BIRD_UP[] = {0x1C, 0x2A, 0xE2, 0xFF, 0x7C, 0x38};    // 8x6, sayap naik
static const uint8_t PROGMEM BIRD_DOWN[] = {0x1C, 0x2A, 0x62, 0xFF, 0xFC, 0x38};  // 8x6, sayap turun

static const int W = 128;
static const int GROUND_Y = 58;  // tanah 58..63
static const int BIRD_X = 24, BIRD_W = 8, BIRD_H = 6;
// Fisika per tick 30 ms (diuji dengan simulasi: celah 16 px + geser celah maks tetap bisa dilalui)
static const float GRAVITY = 0.18f;
static const float FLAP_V = -2.0f;   // satu kepak naik ~10 px
static const float MAX_FALL = 3.0f;
static const float PIPE_SPEED = 1.2f;
static const int PIPE_W = 10;
static const int PIPE_SPACING = 48;  // jarak antar pipa (px)
static const int GAP_START = 22, GAP_MIN = 16, GAP_SHRINK_EVERY = 10;
static const int GAP_MAX_SHIFT = 14;  // perubahan tengah celah maks dari pipa sebelumnya
static const int MAX_PIPES = 4;       // 128 / 48 + 1
static const int PIPE_MIN = 12;       // panjang pipa atas/bawah minimum (px)

struct Pipe { bool on; bool scored; float x; int gapY, gapH; };  // gapY = atas celah

static GamePhase phase;
static FixedStep ticker;
static float birdY, birdV;
static Pipe pipes[MAX_PIPES];
static float distSinceSpawn;
static int lastGapMid;
static uint32_t score;
static uint32_t tickCount;
static bool flapQueued;

static int gapHeight() { return max(GAP_MIN, GAP_START - (int)(score / GAP_SHRINK_EVERY)); }

static void spawnPipe() {
  for (auto &p : pipes) {
    if (p.on) continue;
    int h = gapHeight();
    // Pipa atas & bawah minimal PIPE_MIN px: celah tidak pernah menempel atap/tanah & pipa tetap terlihat
    int lo = h / 2 + PIPE_MIN, hi = GROUND_Y - PIPE_MIN - (h - h / 2);
    int mid = constrain(lastGapMid + (int)random(-GAP_MAX_SHIFT, GAP_MAX_SHIFT + 1), lo, hi);
    lastGapMid = mid;
    p = {true, false, (float)W, mid - h / 2, h};
    return;
  }
}

static void resetRound() {
  birdY = GROUND_Y / 2 - BIRD_H / 2;
  birdV = 0;
  memset(pipes, 0, sizeof(pipes));
  distSinceSpawn = PIPE_SPACING;  // pipa pertama langsung muncul di tepi kanan
  lastGapMid = GROUND_Y / 2;
  score = 0;
  tickCount = 0;
  flapQueued = false;
  phase = PH_READY;
}

static void gameOver() {
  phase = PH_OVER;
  if (score > settings_hiscore(GAME_FLAPPY)) settings_setHiscore(GAME_FLAPPY, score);
}

static void step() {
  tickCount++;
  if (flapQueued) { birdV = FLAP_V; flapQueued = false; }
  birdV = min(birdV + GRAVITY, MAX_FALL);
  birdY += birdV;
  if (birdY < 0 || birdY + BIRD_H > GROUND_Y) { gameOver(); return; }

  distSinceSpawn += PIPE_SPEED;
  if (distSinceSpawn >= PIPE_SPACING) {
    distSinceSpawn -= PIPE_SPACING;
    spawnPipe();
  }
  for (auto &p : pipes) {
    if (!p.on) continue;
    p.x -= PIPE_SPEED;
    if (p.x + PIPE_W < 0) { p.on = false; continue; }
    // Tabrakan: kotak burung tumpang tindih pipa di luar celah
    bool overlapX = BIRD_X + BIRD_W > p.x && BIRD_X < p.x + PIPE_W;
    if (overlapX && (birdY < p.gapY || birdY + BIRD_H > p.gapY + p.gapH)) { gameOver(); return; }
    if (!p.scored && p.x + PIPE_W < BIRD_X) { p.scored = true; score++; }
  }
}

static void drawPipe(const Pipe &p) {
  int x = (int)p.x;
  // Badan pipa + bibir 2 px lebih lebar di ujung celah
  display.fillRect(x, 0, PIPE_W, p.gapY - 3, SSD1306_WHITE);
  display.fillRect(x - 1, p.gapY - 3, PIPE_W + 2, 3, SSD1306_WHITE);
  int by = p.gapY + p.gapH;
  display.fillRect(x - 1, by, PIPE_W + 2, 3, SSD1306_WHITE);
  display.fillRect(x, by + 3, PIPE_W, GROUND_Y - by - 3, SSD1306_WHITE);
  display.drawFastVLine(x + 2, 0, p.gapY - 3, SSD1306_BLACK);  // kilap
  display.drawFastVLine(x + 2, by + 3, GROUND_Y - by - 3, SSD1306_BLACK);
}

static void draw() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  for (auto &p : pipes)
    if (p.on) drawPipe(p);

  // Tanah bergaris bergerak
  display.drawFastHLine(0, GROUND_Y, W, SSD1306_WHITE);
  int off = (int)(tickCount * PIPE_SPEED) % 6;
  for (int x = -off; x < W; x += 6) display.drawLine(x, 63, x + 4, GROUND_Y + 1, SSD1306_WHITE);

  bool wingUp = birdV < 0 || (tickCount / 4) % 2 == 0;
  display.drawBitmap(BIRD_X, (int)birdY, wingUp ? BIRD_UP : BIRD_DOWN, BIRD_W, BIRD_H, SSD1306_WHITE);

  // Skor besar di tengah atas dengan outline hitam 1 px (tanpa kotak) agar pipa di belakangnya tetap terlihat
  char buf[11];
  snprintf(buf, sizeof(buf), "%u", (unsigned)score);
  int tx = (W - (int)strlen(buf) * 12) / 2;
  display.setTextSize(2);
  display.setTextColor(SSD1306_BLACK);
  static const int8_t OUTLINE[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (auto &o : OUTLINE) {
    display.setCursor(tx + o[0], 2 + o[1]);
    display.print(buf);
  }
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(tx, 2);
  display.print(buf);
  display.setTextSize(1);

  if (phase == PH_OVER) games_drawOver(score, settings_hiscore(GAME_FLAPPY));
  oled_flush();
}

void flappy_enter() {
  resetRound();
  ticker.reset();
}

void flappy_resume() { ticker.reset(); }

bool flappy_isPlaying() { return phase == PH_PLAY; }

bool flappy_update(bool redraw) {
  if (btn_pressed(BTN_BACK)) {
    if (phase == PH_PLAY) gameOver();  // simpan high score bila keluar di tengah game
    return false;
  }
  bool flap = btn_pressed(BTN_OK) | btn_pressed(BTN_UP);
  btn_pressed(BTN_DOWN);

  if (phase == PH_OVER) {
    if (!flap) {
      if (redraw) draw();
      return true;
    }
    resetRound();  // OK/kiri: kembali ke layar READY
    flap = false;
    redraw = true;
  }

  if (phase == PH_READY) {
    if (!flap) {
      if (redraw) {
        static const char *const CONTROLS[] = {"OK/kiri: kepak"};
        static const char *const CONTROLS_EN[] = {"OK/left: flap"};
        games_drawReady("FLAPPY", LT(CONTROLS, CONTROLS_EN), 1, settings_hiscore(GAME_FLAPPY));
      }
      return true;
    }
    phase = PH_PLAY;
    ticker.reset();
    redraw = true;  // tekan pertama juga langsung kepak
  }

  if (flap) flapQueued = true;
  int n = ticker.due();
  for (int i = 0; i < n && phase == PH_PLAY; i++) step();
  if (n || redraw) draw();
  return true;
}
