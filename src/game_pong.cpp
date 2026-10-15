#include "game_pong.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "settings.h"
#include "screen.h"
#include "games.h"

extern Adafruit_SSD1306 display;

// ===== Umum =====
static const int PADDLE_W = 14, PADDLE_H = 2;
static const int BALL = 2;
static const float MAX_BOUNCE_RAD = 1.0f;
static const float MAX_SUBSTEP_PX = 1.5f;
static const int SERVE_TICKS = 25;     // jeda sebelum bola meluncur (±0,75 dtk)

enum PongMode : uint8_t { MODE_CPU, MODE_2P };
static PongMode mode = MODE_CPU;       // disimpan di NVS "pong"/"mode"
static bool modeLoaded = false;

static GamePhase phase;
static FixedStep ticker;
static float bx, by, vx, vy, speed;
static int serveWait;

// ===== Smash: kiri+kanan bersamaan -> SIAP (maks 3 dtk); pukulan berikutnya bola x1,7 =====
// Smash: tekan kedua tombol gerak bersamaan (selisih <= 120 ms, dari interrupt); SIAP sampai pukulan berikutnya
static const uint32_t CHORD_WINDOW_MS = 120, SMASH_READY_MS = 3000, SMASH_COOLDOWN_MS = 3000;
static const float SMASH_MULT = 1.7f, SMASH_ANGLE = 0.85f;  // rad, menjauhi paddle lawan
static uint32_t smashReadyUntil[2], smashCoolUntil[2];       // [0] = P1, [1] = P2 (2P)
static bool smashBall = false;                               // bola sedang melaju smash
static float normalSpeed;                                    // kecepatan sebelum smash (dipulihkan)
static float trailX[3], trailY[3];                           // ekor bola smash

// ===== 1P vs CPU (landscape, area 64x64 di tengah + panel retro) =====
static const int W1 = FIELD_W, H1 = 64;
static const int PLAYER_Y = 60, CPU_Y = 2;
static const float PLAYER_SPEED = 2.2f;
static const float BASE_SPEED = 1.2f, LEVEL_SPEEDUP = 0.12f, MAX_SPEED = 2.6f;
static const float CPU_BASE = 0.9f, CPU_LEVEL = 0.12f, CPU_MAX = 1.9f;  // CPU sedikit lebih lambat dari bola
// Galat bidik CPU (acak per pukulan, px dari tengah paddle). Tanpa galat CPU praktis tak terkalahkan
// (simulasi: 0% poin pemain); 7,5 px (> setengah paddle) = kadang meleset: ±35% tanpa smash.
static const float CPU_AIM_ERR = 7.5f;
static float cpuErr = 0;
static const int START_LIVES = 3;
static const int POINTS_PER_LEVEL = 3;

static float playerX, cpuX;
static int lives, level;
static uint32_t score;

// ===== 2P (potret 64x128) =====
static const int W2 = 64, H2 = 128;
static const int P1_Y = 124, P2_Y = 2;          // P1 = bawah, P2 = atas
static const float P2P_SPEED = 2.2f;
static const float RALLY_BASE = 1.4f, RALLY_SPEEDUP = 0.15f, RALLY_MAX = 3.0f;
static const int RALLY_STEP = 3;                 // tiap 3 pukulan beruntun bola sedikit lebih cepat
static const int WIN_POINTS = 7;

static float p1X, p2X;
static int p1Score, p2Score, rallyHits;
static int winner = 0;                           // 0 = belum, 1/2 = pemenang
static uint32_t overAt = 0;                      // waktu menang (BACK keluar baru aktif sesudah jeda)
static const uint32_t OVER_BACK_DELAY_MS = 1500;  // P2 yang masih menekan BACK tidak langsung keluar

// Tombol "kanan" P1 dalam orientasi potret (sama dengan btn_rightHeld)

static void loadMode() {
  if (modeLoaded) return;
  Preferences p;
  if (p.begin("pong", true)) { mode = p.getUChar("mode", MODE_CPU) == MODE_2P ? MODE_2P : MODE_CPU; p.end(); }
  modeLoaded = true;
}

static void saveMode() {
  Preferences p;
  if (p.begin("pong", false)) { p.putUChar("mode", mode); p.end(); }
}

static int substeps() { return max(2, (int)ceilf(speed / MAX_SUBSTEP_PX)); }

static void launch(float s, int fieldW, int fieldH, bool down) {
  smashBall = false;
  cpuErr = random(-1000, 1001) / 1000.0f * CPU_AIM_ERR;
  bx = fieldW / 2.0f - BALL / 2.0f;
  by = fieldH / 2.0f - BALL / 2.0f;
  speed = s;
  float angle = random(-45, 46) / 100.0f;
  vx = speed * sinf(angle);
  vy = (down ? 1 : -1) * speed * cosf(angle);
  serveWait = SERVE_TICKS;
}

// Pantulan paddle: sudut tergantung titik kena (tengah = lurus)
static void bounce(float paddleX, bool up) {
  float rel = ((bx + BALL / 2.0f) - (paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f);
  float angle = constrain(rel, -1.0f, 1.0f) * MAX_BOUNCE_RAD;
  vx = speed * sinf(angle);
  vy = (up ? -1 : 1) * speed * cosf(angle);
}

static bool smashReady(int p) { return millis() < smashReadyUntil[p]; }

// Kedua tombol gerak ditekan bersamaan: isi smash bila tidak sedang cooldown/siap
static void pollSmash(int p, Btn a, Btn b) {
  bool tap = btn_chord(a, b, CHORD_WINDOW_MS);
  uint32_t now = millis();
  if (smashReadyUntil[p] && now >= smashReadyUntil[p]) {  // kedaluwarsa tanpa dipakai: cooldown
    smashReadyUntil[p] = 0;
    smashCoolUntil[p] = now + SMASH_COOLDOWN_MS;
  }
  if (tap && !smashReady(p) && now >= smashCoolUntil[p]) smashReadyUntil[p] = now + SMASH_READY_MS;
}

// Paddle pemain p memukul bola: smash bila SIAP, atau pulihkan kecepatan bila bola smash lawan
static void onHit(int p, float paddleX, bool up, float oppX, int fieldW) {
  if (smashBall) { speed = normalSpeed; smashBall = false; }
  if (smashReady(p)) {
    normalSpeed = speed;
    speed *= SMASH_MULT;
    // Arahkan ke sisi yang jauh dari paddle lawan
    float side = (oppX + PADDLE_W / 2.0f) < fieldW / 2.0f ? 1.0f : -1.0f;
    vx = side * speed * sinf(SMASH_ANGLE);
    vy = (up ? -1 : 1) * speed * cosf(SMASH_ANGLE);
    smashBall = true;
    smashReadyUntil[p] = 0;
    smashCoolUntil[p] = millis() + SMASH_COOLDOWN_MS;
    for (int i = 0; i < 3; i++) { trailX[i] = bx; trailY[i] = by; }
    Serial.printf("[PONG] smash P%d\n", p + 1);
    return;
  }
  bounce(paddleX, up);
}

static void drawTrail(int ox) {
  if (!smashBall) return;
  for (int i = 0; i < 3; i++) display.drawPixel(ox + (int)trailX[i] + BALL / 2, (int)trailY[i] + BALL / 2, SSD1306_WHITE);
}

static void pushTrail() {
  trailX[2] = trailX[1]; trailY[2] = trailY[1];
  trailX[1] = trailX[0]; trailY[1] = trailY[0];
  trailX[0] = bx - vx * 1.5f; trailY[0] = by - vy * 1.5f;
}

// Paddle: SIAP = berkedip tebal (tinggi 4 px)
static void drawPaddle(int x, int y, bool ready, bool growUp) {
  bool thick = ready && (millis() / 120) % 2 == 0;
  display.fillRect(x, thick && growUp ? y - 2 : y, PADDLE_W, thick ? PADDLE_H + 2 : PADDLE_H, SSD1306_WHITE);
}

static void resetRound() {
  loadMode();
  smashReadyUntil[0] = smashReadyUntil[1] = smashCoolUntil[0] = smashCoolUntil[1] = 0;
  smashBall = false;
  playerX = cpuX = (W1 - PADDLE_W) / 2.0f;
  lives = START_LIVES;
  level = 0;
  score = 0;
  p1X = p2X = (W2 - PADDLE_W) / 2.0f;
  p1Score = p2Score = rallyHits = 0;
  winner = 0;
  phase = PH_READY;
}

static void gameOver() {
  phase = PH_OVER;
  if (mode == MODE_CPU && score > settings_hiscore(GAME_PONG)) settings_setHiscore(GAME_PONG, score);
}

// ---------------- 1P vs CPU ----------------
static void step1P() {
  pollSmash(0, BTN_UP, BTN_DOWN);
  if (btn_landLeftHeld()) playerX -= PLAYER_SPEED;
  if (btn_landRightHeld()) playerX += PLAYER_SPEED;
  playerX = constrain(playerX, 0.0f, (float)(W1 - PADDLE_W));

  // CPU mengejar bola hanya saat bola menuju ke atas; selain itu pelan kembali ke tengah
  float cpuSpeed = min(CPU_BASE + level * CPU_LEVEL, CPU_MAX);
  float target = vy < 0 ? bx + BALL / 2.0f - PADDLE_W / 2.0f + cpuErr : (W1 - PADDLE_W) / 2.0f;
  cpuX += constrain(target - cpuX, -cpuSpeed, cpuSpeed);
  cpuX = constrain(cpuX, 0.0f, (float)(W1 - PADDLE_W));

  if (serveWait > 0) { serveWait--; return; }

  int n = substeps();
  for (int s = 0; s < n; s++) {
    bx += vx / n;
    by += vy / n;
    if (bx < 0) { bx = 0; vx = fabsf(vx); }
    if (bx > W1 - BALL) { bx = W1 - BALL; vx = -fabsf(vx); }
    if (vy > 0 && by + BALL >= PLAYER_Y && by + BALL <= PLAYER_Y + PADDLE_H + 2 &&
        bx + BALL >= playerX && bx <= playerX + PADDLE_W) {
      onHit(0, playerX, true, cpuX, W1);
      by = PLAYER_Y - BALL;
    }
    if (vy < 0 && by <= CPU_Y + PADDLE_H && by >= CPU_Y - 2 && bx + BALL >= cpuX && bx <= cpuX + PADDLE_W) {
      if (smashBall) { speed = normalSpeed; smashBall = false; }  // CPU tidak bisa smash; kecepatan kejar tetap
      bounce(cpuX, false);
      by = CPU_Y + PADDLE_H;
      cpuErr = random(-1000, 1001) / 1000.0f * CPU_AIM_ERR;  // bidikan untuk bola berikutnya
    }
    if (by < -BALL) {  // CPU gagal: poin untuk pemain
      score++;
      level = score / POINTS_PER_LEVEL;
      launch(min(BASE_SPEED + level * LEVEL_SPEEDUP, MAX_SPEED), W1, H1, true);
      return;
    }
    if (by > H1) {  // pemain gagal
      if (--lives <= 0) { gameOver(); return; }
      launch(min(BASE_SPEED + level * LEVEL_SPEEDUP, MAX_SPEED), W1, H1, false);
      return;
    }
  }
  pushTrail();
}

static void draw1P() {
  screen_landscape();
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  const int fx = FIELD_X0;
  for (int x = 0; x < W1; x += 4) display.drawFastHLine(fx + x, H1 / 2, 2, SSD1306_WHITE);  // net
  display.fillRect(fx + (int)cpuX, CPU_Y, PADDLE_W, PADDLE_H, SSD1306_WHITE);
  drawPaddle(fx + (int)playerX, PLAYER_Y, smashReady(0), true);
  if (serveWait == 0 || (serveWait / 4) % 2 == 0) display.fillRect(fx + (int)bx, (int)by, BALL, BALL, SSD1306_WHITE);
  drawTrail(fx);
  games_drawSidePanels(score, settings_hiscore(GAME_PONG), lives, level + 1);
  // Indikator smash di bawah panel kiri (menggantikan titik hias): SIAP / bar cooldown
  display.fillRect(0, 47, FIELD_X0 - 3, 17, SSD1306_BLACK);
  uint32_t now = millis();
  if (smashReady(0)) {
    display.fillRect(0, 49, 26, 10, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(1, 50); display.print(T("SIAP", "RDY"));
    display.setTextColor(SSD1306_WHITE);
  } else {
    display.setCursor(0, 49); display.print("SMSH");
    int fill = now >= smashCoolUntil[0] ? 24 : 24 - (int)((smashCoolUntil[0] - now) * 24 / SMASH_COOLDOWN_MS);
    display.drawRect(0, 59, 26, 4, SSD1306_WHITE);
    display.fillRect(1, 60, fill, 2, SSD1306_WHITE);
  }
  if (phase == PH_OVER) games_drawOver(score, settings_hiscore(GAME_PONG));
  oled_flush();
}

// ---------------- 2P (potret) ----------------
static float rallySpeed() { return min(RALLY_BASE + (rallyHits / RALLY_STEP) * RALLY_SPEEDUP, RALLY_MAX); }

static void point2P(int scorer) {
  if (scorer == 1) p1Score++; else p2Score++;
  rallyHits = 0;
  if (p1Score >= WIN_POINTS || p2Score >= WIN_POINTS) {
    winner = p1Score >= WIN_POINTS ? 1 : 2;
    phase = PH_OVER;
    overAt = millis();
    return;
  }
  // Bola ke arah pemain yang baru kalah poin
  launch(RALLY_BASE, W2, H2, scorer == 2);
}

static void step2P() {
  pollSmash(0, BTN_UP, BTN_DOWN);
  pollSmash(1, games_p2MinusBtn(), games_p2PlusBtn());
  // P1 (bawah): pemetaan potret yang sama dengan game potret lain (btn_leftHeld/RightHeld)
  if (btn_leftHeld()) p1X -= P2P_SPEED;
  if (btn_rightHeld()) p1X += P2P_SPEED;
  // P2 (atas) berhadapan dengan P1: kiri P2 = kanan layar. games_p2Dir(): BACK = kiri P2 (+x layar),
  // OK = kanan P2 (-x layar)
  p2X -= games_p2Dir() * P2P_SPEED;
  p1X = constrain(p1X, 0.0f, (float)(W2 - PADDLE_W));
  p2X = constrain(p2X, 0.0f, (float)(W2 - PADDLE_W));

  if (serveWait > 0) { serveWait--; return; }

  int n = substeps();
  for (int s = 0; s < n; s++) {
    bx += vx / n;
    by += vy / n;
    if (bx < 0) { bx = 0; vx = fabsf(vx); }
    if (bx > W2 - BALL) { bx = W2 - BALL; vx = -fabsf(vx); }
    if (vy > 0 && by + BALL >= P1_Y && by + BALL <= P1_Y + PADDLE_H + 2 && bx + BALL >= p1X && bx <= p1X + PADDLE_W) {
      rallyHits++;
      speed = rallySpeed();
      if (smashBall) { normalSpeed = speed; }
      onHit(0, p1X, true, p2X, W2);
      by = P1_Y - BALL;
    }
    if (vy < 0 && by <= P2_Y + PADDLE_H && by >= P2_Y - 2 && bx + BALL >= p2X && bx <= p2X + PADDLE_W) {
      rallyHits++;
      speed = rallySpeed();
      if (smashBall) { normalSpeed = speed; }
      onHit(1, p2X, false, p1X, W2);
      by = P2_Y + PADDLE_H;
    }
    if (by < -BALL) { point2P(1); return; }  // lolos atas: poin P1
    if (by > H2) { point2P(2); return; }     // lolos bawah: poin P2
  }
  pushTrail();
}

static void centerText(int y, const char *s, int size = 1) {
  display.setTextSize(size);
  display.setCursor((display.width() - (int)strlen(s) * 6 * size) / 2, y);
  display.print(s);
  display.setTextSize(1);
}

// Layar menang 2P: landscape (dibaca bersama), skor akhir, OK ulang / BACK keluar
static void drawOver2P() {
  screen_landscape();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.drawRect(14, 6, 100, 52, SSD1306_WHITE);
  centerText(12, winner == 1 ? T("P1 MENANG", "P1 WINS") : T("P2 MENANG", "P2 WINS"), 1);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d - %d", p1Score, p2Score);
  centerText(24, buf, 2);
  centerText(40, T("OK: ulang", "OK: again"));
  if (millis() - overAt >= OVER_BACK_DELAY_MS) centerText(48, T("BACK: keluar", "BACK: exit"));
  oled_flush();
}

static void draw2P() {
  if (phase == PH_OVER) { drawOver2P(); return; }
  screen_portrait();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  for (int x = 0; x < W2; x += 4) display.drawFastHLine(x, H2 / 2, 2, SSD1306_WHITE);  // net
  char buf[4];
  // Skor kecil di samping net: P1 di bawah net; P2 sama tetapi diputar 180 derajat (terbaca dari sisi P2)
  snprintf(buf, sizeof(buf), "%d", p1Score);
  centerText(H2 / 2 + 4, buf, 1);
  display.setRotation((PORTRAIT_ROTATION + 2) % 4);
  snprintf(buf, sizeof(buf), "%d", p2Score);
  centerText(H2 / 2 + 4, buf, 1);
  display.setRotation(PORTRAIT_ROTATION);
  drawPaddle((int)p2X, P2_Y, smashReady(1), false);
  drawPaddle((int)p1X, P1_Y, smashReady(0), true);
  if (phase == PH_PLAY && (serveWait == 0 || (serveWait / 4) % 2 == 0)) {
    display.fillRect((int)bx, (int)by, BALL, BALL, SSD1306_WHITE);
    drawTrail(0);
  }

  oled_flush();
}

static void draw() {
  if (mode == MODE_2P) draw2P();
  else draw1P();
}

static void drawReady() {
  screen_landscape();
  static char modeLine[16];
  snprintf(modeLine, sizeof(modeLine), "< %s >", mode == MODE_2P ? "2P vs 2P" : "1P vs CPU");
  static const char *controls[3];
  controls[0] = modeLine;
  controls[1] = mode == MODE_2P ? T("Keluar: saat menang", "Exit: after a win") : T("Kiri/kanan: gerak", "Left/right: move");
  controls[2] = T("Kiri+kanan: smash", "Left+right: smash");
  games_drawReady("PONG", controls, 3, settings_hiscore(GAME_PONG));
}

void pong_enter() {
  resetRound();
  ticker.reset();
}

void pong_resume() {
  if (mode == MODE_2P && phase == PH_PLAY) screen_portrait();  // setelah pop-up; layar menang tetap landscape
  ticker.reset();
}

bool pong_isPlaying() { return phase == PH_PLAY; }

bool pong_update(bool redraw) {
  bool in2P = mode == MODE_2P && phase != PH_READY;

  if (in2P && phase == PH_OVER) {
    // Sudah ada pemenang: tidak ada tahan-keluar (cegah keluar tak sengaja). OK = ulang,
    // BACK = keluar setelah jeda (P2 yang masih menekan BACK saat poin terakhir tidak ikut keluar)
    bool ok = btn_pressed(BTN_OK), back = btn_pressed(BTN_BACK);
    btn_pressed(BTN_UP);
    btn_pressed(BTN_DOWN);
    bool backReady = millis() - overAt >= OVER_BACK_DELAY_MS;
    if (ok) { resetRound(); drawReady(); return true; }
    if (back && backReady) return false;
    static bool shownReady = false;
    if (backReady != shownReady) { shownReady = backReady; redraw = true; }
    if (redraw) draw();
    return true;
  }

  if (in2P) {
    // Saat main semua tombol hanya kontrol (BACK & OK = P2). Keluar hanya dari layar menang.
    btn_pressed(BTN_BACK);
    btn_pressed(BTN_OK);
    btn_pressed(BTN_UP);
    btn_pressed(BTN_DOWN);
    int n = ticker.due();
    for (int i = 0; i < n && phase == PH_PLAY; i++) step2P();
    if (n || redraw) draw();
    return true;
  }

  if (btn_pressed(BTN_BACK)) {
    if (phase == PH_PLAY) gameOver();  // simpan high score bila keluar di tengah game
    return false;
  }
  bool ok = btn_pressed(BTN_OK);
  bool left = btn_pressed(MENU_SWAP_LR ? BTN_DOWN : BTN_UP);
  bool right = btn_pressed(MENU_SWAP_LR ? BTN_UP : BTN_DOWN);

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
    if (left || right) {  // pilih mode
      mode = mode == MODE_2P ? MODE_CPU : MODE_2P;
      saveMode();
      redraw = true;
    }
    if (!ok) {
      if (redraw) drawReady();
      return true;
    }
    phase = PH_PLAY;
    ticker.reset();
    btn_tapSync();  // tekan di menu/READY tidak memicu smash
    if (mode == MODE_2P) {
      p1X = p2X = (W2 - PADDLE_W) / 2.0f;
      launch(RALLY_BASE, W2, H2, random(2));
    } else {
      launch(BASE_SPEED, W1, H1, true);
    }
    draw();
    return true;
  }

  int n = ticker.due();
  for (int i = 0; i < n && phase == PH_PLAY; i++) step1P();
  if (n || redraw) draw();
  return true;
}
