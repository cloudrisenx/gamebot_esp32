#include "game_dino.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include "buttons.h"
#include "settings.h"
#include "games.h"

extern Adafruit_SSD1306 display;

static const uint8_t PROGMEM DINO_RUN1[] = {0x07, 0x80, 0x0D, 0xC0, 0x0F, 0xC0, 0x0E, 0x00, 0x8F, 0x80, 0xDE, 0x00,
                                            0xFE, 0x00, 0x7E, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x11, 0x00, 0x30, 0x00};
static const uint8_t PROGMEM DINO_RUN2[] = {0x07, 0x80, 0x0D, 0xC0, 0x0F, 0xC0, 0x0E, 0x00, 0x8F, 0x80, 0xDE, 0x00,
                                            0xFE, 0x00, 0x7E, 0x00, 0x3C, 0x00, 0x1E, 0x00, 0x11, 0x00, 0x01, 0x80};
static const uint8_t PROGMEM DINO_DUCK[] = {0x80, 0xF8, 0xC7, 0xDC, 0xFF, 0xFC, 0x7F, 0xE0, 0x3F, 0xC0, 0x11, 0x00, 0x19, 0x80};
static const uint8_t PROGMEM BIRD1[] = {0x08, 0x00, 0x0C, 0x00, 0x7F, 0x80, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t PROGMEM BIRD2[] = {0x00, 0x00, 0x00, 0x00, 0x7F, 0x80, 0xFE, 0x00, 0x0C, 0x00, 0x08, 0x00};

static const int W = 128;
static const int GROUND_Y = 56;
static const int DINO_X = 10;
static const int DINO_W = 10, DINO_H = 12;
static const int DUCK_W = 14, DUCK_H = 7;
static const int BIRD_W = 9, BIRD_H = 6;
// Burung setinggi kepala (y 40..46): hitbox dino berdiri mulai y=45 -> tabrak;
// dino merunduk mulai y=50 -> lolos. Puncak lompatan biasa (bawah hitbox ~y=40.4) masih
// menyentuh burung, jadi burung dilewati dengan merunduk ATAU double jump.
static const int BIRD_Y = 40;
static const int BIRD_MIN_SCORE = 100;
static const float GRAVITY = 0.25f;
static const float JUMP_V = -2.7f;   // tinggi lompatan ~14.6px: cukup untuk kaktus 12px
static const float JUMP2_V = -2.4f;  // lompatan kedua di udara (double jump)
static const float FAST_FALL = 0.5f; // tahan DOWN di udara = turun cepat
static const float START_SPEED = 2.5f;
static const float MAX_SPEED = 5.0f;
static const int MAX_OBS = 3;
static const float BIRD_EXTRA_SPEED = 0.5f;  // burung terbang sedikit lebih cepat dari tanah
// Jarak antar rintangan diukur dari jarak tempuh (bukan ruang kosong di layar 128px, yang
// membuat spawn macet saat jarak acak > lebar layar). Jarak minimum = jarak satu lompatan
// penuh + waktu reaksi, supaya setiap rintangan selalu bisa dilompati satu per satu.
static const float JUMP_TICKS = 2.0f * -JUMP_V / GRAVITY;  // lama di udara satu lompatan (21,6 tick)
static const float REACT_TICKS = 6.0f;                      // jeda mendarat -> lompat lagi
static const float GAP_MARGIN = 20.0f;                      // ruang lebar rintangan + dino
static const int GAP_RANDOM = 60;                           // variasi jarak acak (px)

enum ObsType { OBS_SMALL, OBS_TALL, OBS_DOUBLE, OBS_BIRD };
struct Obstacle { bool on; ObsType type; float x; };

static GamePhase phase;
static FixedStep ticker;
static float dinoY, dinoV;  // dinoY = posisi kaki (0 = di tanah, negatif = di udara)
static bool ducking;
static Obstacle obs[MAX_OBS];
static float nextGap;         // jarak tempuh sampai rintangan berikutnya
static float distSinceSpawn;  // jarak tempuh sejak rintangan terakhir muncul
static ObsType nextType;      // jenis rintangan berikutnya (memengaruhi jaraknya)
static float speed;
static uint32_t score;
static uint32_t tickCount;
static bool jumpQueued;
static bool airJumpUsed;  // jatah double jump sudah dipakai sejak lepas dari tanah
static bool dustFrame;    // efek debu 1 frame saat double jump

static void obsSize(ObsType t, int &w, int &h, int &y) {
  switch (t) {
    case OBS_SMALL:  w = 4; h = 8;  y = GROUND_Y - h; break;
    case OBS_TALL:   w = 6; h = 12; y = GROUND_Y - h; break;
    case OBS_DOUBLE: w = 8; h = 9;  y = GROUND_Y - h; break;
    default:         w = BIRD_W; h = BIRD_H; y = BIRD_Y; break;
  }
}

static ObsType randomType() { return (ObsType)random(score >= BIRD_MIN_SCORE ? 4 : 3); }

static void resetRound() {
  dinoY = dinoV = 0;
  ducking = false;
  memset(obs, 0, sizeof(obs));
  speed = START_SPEED;
  score = 0;
  nextType = randomType();
  nextGap = 40;  // rintangan pertama muncul cepat
  distSinceSpawn = 0;
  tickCount = 0;
  jumpQueued = false;
  airJumpUsed = false;
  dustFrame = false;
  phase = PH_READY;
}

static void gameOver() {
  phase = PH_OVER;
  if (score > settings_hiscore(GAME_DINO)) settings_setHiscore(GAME_DINO, score);
}

// Jarak sebelum rintangan `type` muncul. Burung lebih cepat sehingga mendekati rintangan
// di depannya selama keduanya di layar; selisih itu ditambahkan agar jaraknya tetap aman.
static float gapFor(ObsType type) {
  float gap = (JUMP_TICKS + REACT_TICKS) * speed + GAP_MARGIN + random(0, GAP_RANDOM);
  if (type == OBS_BIRD) gap += BIRD_EXTRA_SPEED * (W + GAP_MARGIN) / speed;
  return gap;
}

// Munculkan rintangan yang sudah dijadwalkan. false bila semua slot terpakai (dicoba lagi tick berikutnya).
static bool spawn() {
  for (auto &o : obs) {
    if (o.on) continue;
    o.on = true;
    o.x = W;
    o.type = nextType;
    nextType = randomType();
    nextGap = gapFor(nextType);
    return true;
  }
  return false;
}

static void step() {
  tickCount++;
  score++;
  speed = min(MAX_SPEED, START_SPEED + score / 400.0f * 0.25f);

  bool onGround = dinoY >= 0;
  bool down = btn_held(BTN_DOWN);
  dustFrame = false;
  if (jumpQueued) {
    if (onGround) {
      dinoV = JUMP_V;
      onGround = false;
    } else if (!airJumpUsed) {
      dinoV = JUMP2_V;
      airJumpUsed = true;
      dustFrame = true;
    }
  }
  jumpQueued = false;
  if (!onGround) {
    dinoV += GRAVITY + (down ? FAST_FALL : 0);
    dinoY += dinoV;
    if (dinoY >= 0) { dinoY = 0; dinoV = 0; airJumpUsed = false; }
  }
  ducking = down && dinoY >= 0;

  for (auto &o : obs) {
    if (!o.on) continue;
    o.x -= o.type == OBS_BIRD ? speed + BIRD_EXTRA_SPEED : speed;
    int w, h, y;
    obsSize(o.type, w, h, y);
    if (o.x + w < 0) o.on = false;
  }
  distSinceSpawn += speed;
  if (distSinceSpawn >= nextGap && spawn()) distSinceSpawn = 0;

  // Hitbox dino sedikit diperkecil supaya terasa adil
  int dw = ducking ? DUCK_W : DINO_W;
  int dh = ducking ? DUCK_H : DINO_H;
  float dx = DINO_X + 1;
  float dy = GROUND_Y - dh + dinoY + 1;
  for (auto &o : obs) {
    if (!o.on) continue;
    int w, h, y;
    obsSize(o.type, w, h, y);
    if (dx < o.x + w && dx + dw - 2 > o.x && dy < y + h && dy + dh - 2 > y) {
      gameOver();
      return;
    }
  }
}

static void drawObstacle(const Obstacle &o) {
  int w, h, y;
  obsSize(o.type, w, h, y);
  int x = (int)o.x;
  switch (o.type) {
    case OBS_SMALL:
      display.fillRect(x + 1, y, 2, h, SSD1306_WHITE);
      display.drawPixel(x, y + 3, SSD1306_WHITE);
      display.drawPixel(x + 3, y + 2, SSD1306_WHITE);
      break;
    case OBS_TALL:
      display.fillRect(x + 2, y, 2, h, SSD1306_WHITE);
      display.drawFastVLine(x, y + 3, 4, SSD1306_WHITE);
      display.drawFastHLine(x, y + 6, 2, SSD1306_WHITE);
      display.drawFastVLine(x + 5, y + 2, 4, SSD1306_WHITE);
      display.drawFastHLine(x + 4, y + 5, 2, SSD1306_WHITE);
      break;
    case OBS_DOUBLE:
      display.fillRect(x + 1, y + 1, 2, h - 1, SSD1306_WHITE);
      display.fillRect(x + 5, y, 2, h, SSD1306_WHITE);
      display.drawPixel(x, y + 4, SSD1306_WHITE);
      display.drawPixel(x + 7, y + 3, SSD1306_WHITE);
      break;
    case OBS_BIRD:
      display.drawBitmap(x, y, (tickCount / 6) % 2 ? BIRD1 : BIRD2, BIRD_W, BIRD_H, SSD1306_WHITE);
      break;
  }
}

static void draw() {
  display.clearDisplay();
  display.drawFastHLine(0, GROUND_Y, W, SSD1306_WHITE);
  // Kerikil bergerak agar terasa berlari
  int offset = (tickCount * (int)speed) % 16;
  for (int x = -offset; x < W; x += 16) display.drawPixel(x + 7, GROUND_Y + 3, SSD1306_WHITE);

  for (auto &o : obs)
    if (o.on) drawObstacle(o);

  if (ducking) {
    display.drawBitmap(DINO_X, GROUND_Y - DUCK_H, DINO_DUCK, DUCK_W, DUCK_H, SSD1306_WHITE);
  } else {
    bool frame2 = dinoY >= 0 && (tickCount / 4) % 2;
    display.drawBitmap(DINO_X, GROUND_Y - DINO_H + (int)dinoY, frame2 ? DINO_RUN2 : DINO_RUN1, DINO_W, DINO_H, SSD1306_WHITE);
    if (dustFrame) {  // debu di bawah kaki saat double jump
      int fy = GROUND_Y + (int)dinoY + 2;
      display.drawPixel(DINO_X + 2, fy, SSD1306_WHITE);
      display.drawPixel(DINO_X + 7, fy + 1, SSD1306_WHITE);
    }
  }

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  String s = String(score);
  display.setCursor(W - s.length() * 6, 0);
  display.print(s);

  if (phase == PH_OVER) games_drawOver(score, settings_hiscore(GAME_DINO));
  oled_flush();
}

void dino_enter() {
  resetRound();
  ticker.reset();
}

void dino_resume() { ticker.reset(); }

bool dino_isPlaying() { return phase == PH_PLAY; }

bool dino_update(bool redraw) {
  if (btn_pressed(BTN_BACK)) {
    if (phase == PH_PLAY) gameOver();
    return false;
  }
  bool jump = btn_pressed(BTN_OK) | btn_pressed(BTN_UP);
  btn_pressed(BTN_DOWN);  // merunduk memakai status tahan

  if (phase == PH_OVER) {
    if (!jump) {
      if (redraw) draw();
      return true;
    }
    resetRound();
    jump = false;
    redraw = true;
  }

  if (phase == PH_READY) {
    if (!jump) {
      if (redraw) {
        static const char *const CONTROLS[] = {"OK/kiri: lompat (2x)", "Tahan kanan: merunduk"};
        static const char *const CONTROLS_EN[] = {"OK/left: jump (2x)", "Hold right: duck"};
        games_drawReady("DINO RUN", LT(CONTROLS, CONTROLS_EN), 2, settings_hiscore(GAME_DINO));
      }
      return true;
    }
    phase = PH_PLAY;
    ticker.reset();
    redraw = true;
  }

  if (jump) jumpQueued = true;
  int n = ticker.due();
  for (int i = 0; i < n && phase == PH_PLAY; i++) step();
  if (n || redraw) draw();
  return true;
}
