#include "game_breakout.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include "settings.h"
#include "screen.h"
#include "games.h"

extern Adafruit_SSD1306 display;

// Koordinat area main 64x64 (0..63), digambar dengan offset FIELD_X0; skor/nyawa di panel samping
static const int W = FIELD_W, H = 64;
static const int TOP_WALL = 0;
static const int COLS = 6, ROWS = 4;
static const int BRICK_W = 9, BRICK_H = 3, BRICK_X0 = 2, BRICK_Y0 = 3;
static const int PADDLE_W = 14, PADDLE_H = 2, PADDLE_Y = 60;
static const float PADDLE_SPEED = 2.2f;
static const int BALL = 2;
static const float BASE_SPEED = 1.1f;   // lebih gesit (dulu 0.7 di area 128 px)
static const float LEVEL_SPEEDUP = 0.15f;
static const float MAX_BOUNCE_RAD = 1.05f;  // ~60° dari vertikal
static const int START_LIVES = 3;
// Gerak bola dipecah jadi beberapa langkah kecil per tick agar tidak menembus bata (4px)
// atau jendela tangkap paddle (4px): tiap langkah maks MAX_SUBSTEP_PX.
static const float MAX_SUBSTEP_PX = 1.8f;
static const int MIN_SUBSTEPS = 2;
static const float MAX_SPEED = 3.5f;

static GamePhase phase;
static FixedStep ticker;
static bool bricks[ROWS][COLS];
static int bricksLeft;
static float paddleX;
static float bx, by, vx, vy, speed;
static int substeps = MIN_SUBSTEPS;

static void setSpeed(float s) {
  speed = min(s, MAX_SPEED);
  substeps = max(MIN_SUBSTEPS, (int)ceilf(speed / MAX_SUBSTEP_PX));
}
static bool attached;
static int lives, level;
static uint32_t score;

static void buildLevel() {
  bricksLeft = 0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++) {
      bool on;
      switch (level % 3) {
        case 0: on = true; break;                                // penuh
        case 1: on = (r + c) % 2 == 0; break;                    // papan catur
        default: on = c >= r / 2 && c < COLS - r / 2; break;     // piramida terbalik
      }
      bricks[r][c] = on;
      bricksLeft += on;
    }
}

// Taruh bola di tengah atas paddle. Posisi diisi di sini (bukan menunggu step()),
// karena launch() bisa dipanggil sebelum tick berikutnya (OK di layar READY).
static void attachBall() {
  attached = true;
  vx = vy = 0;
  bx = paddleX + PADDLE_W / 2.0f - BALL / 2.0f;
  by = PADDLE_Y - BALL;
}

static void resetRound() {
  paddleX = (W - PADDLE_W) / 2.0f;
  lives = START_LIVES;
  level = 0;
  score = 0;
  setSpeed(BASE_SPEED);
  buildLevel();
  attachBall();
  phase = PH_READY;
}

static void launch() {
  attached = false;
  float angle = random(-40, 41) / 100.0f;
  vx = speed * sinf(angle);
  vy = -speed * cosf(angle);
}

static void gameOver() {
  phase = PH_OVER;
  if (score > settings_hiscore(GAME_BREAKOUT)) settings_setHiscore(GAME_BREAKOUT, score);
}

static bool hitBrick() {
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++) {
      if (!bricks[r][c]) continue;
      float x = BRICK_X0 + c * (BRICK_W + 1);
      float y = BRICK_Y0 + r * (BRICK_H + 1);
      if (bx + BALL <= x || bx >= x + BRICK_W || by + BALL <= y || by >= y + BRICK_H) continue;
      bricks[r][c] = false;
      bricksLeft--;
      score += 10;
      // Posisi sebelumnya di luar bata secara horizontal -> kena sisi samping
      float px = bx - vx / substeps;
      if (px + BALL <= x || px >= x + BRICK_W) vx = -vx;
      else vy = -vy;
      return true;
    }
  return false;
}

static void step() {
  if (btn_landLeftHeld()) paddleX -= PADDLE_SPEED;
  if (btn_landRightHeld()) paddleX += PADDLE_SPEED;
  paddleX = constrain(paddleX, 0.0f, (float)(W - PADDLE_W));

  if (attached) {
    bx = paddleX + PADDLE_W / 2.0f - BALL / 2.0f;
    by = PADDLE_Y - BALL;
    return;
  }

  for (int s = 0; s < substeps; s++) {
    bx += vx / substeps;
    by += vy / substeps;

    if (bx < 0) { bx = 0; vx = fabsf(vx); }
    if (bx > W - BALL) { bx = W - BALL; vx = -fabsf(vx); }
    if (by < TOP_WALL) { by = TOP_WALL; vy = fabsf(vy); }

    // Paddle: sudut pantul tergantung titik kena
    if (vy > 0 && by + BALL >= PADDLE_Y && by + BALL <= PADDLE_Y + PADDLE_H + 2 &&
        bx + BALL >= paddleX && bx <= paddleX + PADDLE_W) {
      float rel = ((bx + BALL / 2.0f) - (paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f);
      float angle = constrain(rel, -1.0f, 1.0f) * MAX_BOUNCE_RAD;
      vx = speed * sinf(angle);
      vy = -speed * cosf(angle);
      by = PADDLE_Y - BALL;
    }

    if (hitBrick() && bricksLeft == 0) {
      level++;
      setSpeed(speed + LEVEL_SPEEDUP);
      buildLevel();
      attachBall();
      return;
    }

    if (by > H) {
      if (--lives <= 0) { gameOver(); return; }
      attachBall();
      return;
    }
  }
}

static void draw() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  const int fx = FIELD_X0;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      if (bricks[r][c])
        display.fillRect(fx + BRICK_X0 + c * (BRICK_W + 1), BRICK_Y0 + r * (BRICK_H + 1), BRICK_W, BRICK_H, SSD1306_WHITE);

  display.fillRect(fx + (int)paddleX, PADDLE_Y, PADDLE_W, PADDLE_H, SSD1306_WHITE);
  display.fillRect(fx + (int)bx, (int)by, BALL, BALL, SSD1306_WHITE);
  games_drawSidePanels(score, settings_hiscore(GAME_BREAKOUT), lives, level + 1);

  if (attached && phase == PH_PLAY) games_centerText(40, T("OK:luncur", "OK:launch"));
  if (phase == PH_OVER) games_drawOver(score, settings_hiscore(GAME_BREAKOUT));
  oled_flush();
}

void breakout_enter() {
  resetRound();
  ticker.reset();
}

void breakout_resume() { ticker.reset(); }

// Bola yang menempel di paddle tidak dihitung "main", supaya deep sleep tetap bisa terjadi
bool breakout_isPlaying() { return phase == PH_PLAY && !attached; }

bool breakout_update(bool redraw) {
  if (btn_pressed(BTN_BACK)) {
    if (phase == PH_PLAY) gameOver();  // simpan high score bila keluar di tengah game
    return false;
  }
  bool ok = btn_pressed(BTN_OK);
  btn_pressed(BTN_UP);  // gerak memakai status tahan, buang event tekan
  btn_pressed(BTN_DOWN);

  if (phase == PH_OVER) {
    if (!ok) {
      if (redraw) draw();
      return true;
    }
    resetRound();  // OK: kembali ke layar READY
    ok = false;
    redraw = true;
  }

  if (phase == PH_READY) {
    if (!ok) {
      if (redraw) {
        static const char *const CONTROLS[] = {"Kiri/kanan: gerak", "OK: luncur"};
        static const char *const CONTROLS_EN[] = {"Left/right: move", "OK: launch"};
        games_drawReady("BREAKOUT", LT(CONTROLS, CONTROLS_EN), 2, settings_hiscore(GAME_BREAKOUT));
      }
      return true;
    }
    phase = PH_PLAY;
    ticker.reset();
    launch();
    redraw = true;
  }

  if (ok && attached) launch();
  int n = ticker.due();
  for (int i = 0; i < n && phase == PH_PLAY; i++) step();
  if (n || redraw) draw();
  return true;
}
