#pragma once
// Mesin Pet (kucing peliharaan di layar): logika + gambar, TANPA ketergantungan Arduino.
// Port dari tools/pet_sim.py (acuan perilaku & bentuk). Dites di PC: tools/pet_host_test.cpp.
#include <stddef.h>
#include <stdint.h>

namespace pet {

static const int W = 128, H = 64;

// Framebuffer 1 byte per piksel (ukuran kecil, tanpa heap: penyimpanan disediakan pemanggil).
struct Fb {
  int w, h;
  uint8_t *px;
  int ox = 0, oy = 0;  // geser seluruh gambar (loncat, napas, getar)
  Fb(int w_, int h_, uint8_t *buf) : w(w_), h(h_), px(buf) {}
  void clear();
  void pixel(int x, int y, uint8_t c = 1);
  void vline(int x, int y, int n, uint8_t c = 1);
  void hline(int x, int y, int n, uint8_t c = 1);
  void fillRect(int x, int y, int rw, int rh, uint8_t c = 1);
  void line(int x0, int y0, int x1, int y1, uint8_t c = 1);
  void fillCircleHelper(int x0, int y0, int r, int corners, int delta, uint8_t c = 1);
  void fillCircle(int x0, int y0, int r, uint8_t c = 1);
  void fillRoundRect(int x, int y, int rw, int rh, int r, uint8_t c = 1);
  void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c = 1);
  void curve(double x0, double y0, double x1, double y1, double x2, double y2, int thick = 1, uint8_t c = 1);
  void ch(int x, int y, char g, int size = 1, uint8_t c = 1);  // 'z', 'Z', '!'
  void blit(const Fb &src, int x, int y, bool flip = false);
};

// Parameter wajah yang berubah mulus (pegas)
enum ParamId {
  P_W, P_H, P_R, P_LT, P_TILT, P_LB, P_PW, P_PH, P_CURVE, P_WFLAT, P_WLEN,
  P_MC, P_MO, P_MW, P_TONGUE, P_WH, P_AMP, P_EARL, P_EARR, P_HTILT, P_BLUSH,
  P_GX, P_GY, P_FY, NPARAM
};

struct Pose {  // pose kucing 8-bit
  bool sitting = false, hasGait = false, hasLegs = false, wiggle = false, eyesClosed = false, mouth = false;
  bool prey = false, noLegs = false, pupil2 = false, hasScratch = false;
  double crouch = 0, headDy = 0, ear = 0, tail = 0.5, tilt = 0, arch = 0, bob = 0;  // yang diperhalus
  double gaitAmp = 0, gaitLift = 0;
  double legs[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
  double wagAmp = -1, wagSpd = -1;  // -1 = bawaan
  double scratch = 0;
};

enum CatMode : uint8_t { CM_NONE, CM_MORPH, CM_FADE, CM_SPRITE, CM_PLAY, CM_DOTS, CM_PEEK, CM_PEEKEDGE };
enum Edge : uint8_t { EDGE_ATAS, EDGE_KIRI, EDGE_KANAN, EDGE_BAWAH };

struct Cat {
  CatMode mode = CM_NONE;
  bool flip = false, ground = false;
  double x = 64, y = 0, ph = 0, u = 0, rise = 0, look = 0;
  int scroll = 0;
  Pose pose;
  bool hasToy = false, toyHidden = false;
  double toyX = 0, toyPh = 0;
  int toyDir = 1;
  Edge edge = EDGE_ATAS;
  int vis = 0, pos = 0;
  int nzs = 0;
  double zs[2][3] = {{0, 0, 0}, {0, 0, 0}};  // x, y, besar
};

static const int MAX_HEARTS = 8, MAX_CRUMBS = 12, MAX_DUST = 10;

struct Face {  // satu "gambar penuh": wajah, atau adegan kucing 8-bit (cat.mode != CM_NONE)
  double t = 0;
  double p[NPARAM] = {0};
  double bL = 0, bR = 0;
  uint8_t twitch = 0;  // 0 tidak, 1 kiri, 2 kanan, 3 keduanya
  double shakeX = 0, shakeY = 0, breath = 0;
  int nose = 0;
  bool geli = false, exclaim = false, jaw = false;
  int nHearts = 0;
  double hearts[MAX_HEARTS][2];
  bool hasFish = false;
  double fishX = 0, fishY = 0;
  int nCrumbs = 0;
  double crumbs[MAX_CRUMBS][2];
  bool hasPaw = false, hasKnead = false, hasBird = false;
  double pawX = 0, pawY = 0, kneadL = 0, kneadR = 0, birdX = 0, birdY = 0;
  int birdFlap = 0, birdDir = 1;
  int nzs = 0;
  double zs[2][3];
  bool hasCat = false;
  Cat cat;
  int nDust = 0;
  double dust[MAX_DUST][3];  // x, y, umur (ms)
};

void drawFace(Fb &fb, const Face &F);  // gambar penuh ke fb
const Pose &posePreset(int id);  // tabel pose (urutan sama dengan POSES di tools/pet_sim.py); untuk tes
static const int NPOSE_PUBLIC = 15;

enum Mood : uint8_t { M_NORMAL, M_BOSAN, M_TIDUR, M_SENANG, M_TERKEJUT, M_GELI, M_PURR, M_MAKAN, M_MAIN, M_KABUR };
enum Kind : uint8_t { K_KABUR, K_ZOOMIES, K_JALAN, K_EKOR, K_STRETCH, K_GARUK, K_LOAF, K_NGUMPET, K_COUNT };
enum Ev : uint8_t { EV_NONE, EV_WINK, EV_BLEP, EV_SNIFF, EV_KEPO, EV_YAWN, EV_SLOWBLINK, EV_TILT, EV_GROOM, EV_BIRD };

struct Rng {
  uint32_t s = 2463534242u;
  uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
  int randint(int a, int b) { return a + (int)(next() % (uint32_t)(b - a + 1)); }
  double uniform(double a, double b) { return a + (b - a) * ((next() >> 8) / 16777216.0); }
  double random() { return (next() >> 8) / 16777216.0; }
};

struct Spring { float x, t, v, k, c; };

struct Peek { Edge edge; double t0, dur, hold; int pos; };

struct Esc {
  bool active = false, woke = false;
  Kind kind = K_KABUR;
  double t0 = 0, poof = 0, run = 0, gone = 0, peek = 0, ret = 0, sit = 0, unm = 0, end = 0, sleepDur = 0;
  int dir = 1, back = 1, peekX = 64, nPeeks = 0;
  Peek peeks[4];
};

struct Play {
  uint8_t st = 0;
  double t0 = 0, mpause = 0;
  float mx = 64, mv = 0, mtarget = 64, mspeed = 75, ph = 0, zx0 = 0, zx1 = 0;
  int mdir = 1;
  bool pinned = false, hidden = false;
};

struct Engine {
  // ---- pengaturan (tes mengubahnya)
  double boredAfter = 15000, sleepAfter = 30000, sleepMin = 30000, sleepMax = 180000;
  double kaburProb = 0.35, zoomProb = 0.04, bodyProb = 0.3, loafProb = 0.5;
  bool lively = true;

  // ---- keadaan
  Rng rng;
  Spring sp[NPARAM];
  double t = 0, lastInput = 0;
  bool isDown[3] = {false, false, false};
  double downAt[3] = {0, 0, 0};
  double happyUntil = 0, geliUntil = 0, surpriseUntil = 0, lookUntil = 0;
  int lookGx = 0;
  double okTimes[8];
  int nOk = 0;
  double blinkAt = 0, nextEv = 0, gzNext = 0, driftAt = 0, twitchAt = 0, twitchUntil = 0, earAt = 0, dreamAt = 0,
         dreamUntil = 0, heartAt = 0, dustAt = 0, autoPlayUntil = 0, sleepT0 = 0, sleepDur = 0, pendingGroom = 0,
         pendingZoom = 0, feedT0 = -1, yawnRolled = -1;
  bool blink2 = false, feeding = false, hearty = false;
  Ev evKind = EV_NONE;
  double evStart = 0, evUntil = 0;
  int evSide = -1, evDir = 1;  // evSide: -1 kiri, +1 kanan
  float gzX = 0, gzY = 0, driftX = 0, driftY = 0;
  int twitchSide = 1;
  Mood prevMood = M_NORMAL, moodNow = M_NORMAL;
  float hearts[MAX_HEARTS][3];  // x, y, t0
  int nHearts = 0;
  float dust[MAX_DUST][3];      // x, y, t0
  int nDust = 0;
  float crumbs[MAX_CRUMBS][4];  // x, y, vx, vy
  int nCrumbs = 0;
  Esc esc;
  Play pl;
  bool plActive = false, sceneMain = false;
  float morph = 0, morphTarget = 0, catX = 64, catY = 0, catPh = 0, feedGy = 0;
  int catDir = 1, chewK = -1;
  Pose poseCur;
  Face F;
  bool haveF = false;
  const char *labelA = "normal", *labelB = nullptr;
  uint32_t stats[32] = {0};  // untuk tes (kejadian, 10+kegiatan, 20 lirik, 21 kedip, 22 main sendiri)

  void begin(uint32_t seed, double now = 0);
  void press(double now, int btn);    // btn 0 = UP (kiri), 1 = DOWN (kanan), 2 = OK
  void release(double now, int btn);
  void notify(double now);
  void sleepNow(double now);
  void startEscape(double now, Kind kind);
  void startEvent(Ev ev, double now);
  void advance(double now);
  const Face &frame(double now);  // advance + keadaan terbaru
  Mood mood() const { return moodNow; }

  // ---- dalam
  double held(int btn, double now) const { return isDown[btn] ? now - downAt[btn] : -1; }
  bool anyDown() const { return isDown[0] || isDown[1] || isDown[2]; }
  Mood moodAt(double t) const;
  void step(double t);
  void resetPose();
  Pose easePose(const Pose &target, double dt);
  void escapeStep(double t, Cat &cat, bool &hasCat, int &fp, const char *&la, const char *&lb);
  void activityStep(double t, double e, Cat &cat, bool &hasCat, int &fp, const char *&la, const char *&lb);
  void peekScene(double s, Cat &cat, const char *&la, const char *&lb);
  void playStep(double t, double dt, Cat &cat, int &fp, const char *&la, const char *&lb);
  void feedStep(double t, int &fp, bool &hasFish, float &fx, float &fy, const char *&la, const char *&lb, bool &hasMo,
                float &mo);
  void callEscape(double t);
  void puff(double t, double x, double y);
};

}  // namespace pet
