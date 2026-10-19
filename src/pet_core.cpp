// Mesin Pet: port dari tools/pet_sim.py (logika, gambar, pose kucing 8-bit). Lihat pet_core.h.
#include "pet_core.h"
#include <math.h>
#include <string.h>

namespace pet {

// ================================================================ util
static inline int rnd(double x) { return (int)floor(x + 0.5); }
static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline double dmin(double a, double b) { return a < b ? a : b; }
static inline double dmax(double a, double b) { return a > b ? a : b; }
static inline double clampd(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline int fdiv(int a, int b) { return (int)floor((double)a / b); }  // pembagian lantai (seperti // Python)
static inline int pmod(int a, int n) { int m = a % n; return m < 0 ? m + n : m; }
static inline int iabs(int a) { return a < 0 ? -a : a; }
static const double PI_ = 3.14159265358979323846;

static double smooth(double a, double b, double x) {
  x = clampd((x - a) / (b - a), 0.0, 1.0);
  return x * x * (3 - 2 * x);
}

// ================================================================ framebuffer + primitif ala Adafruit_GFX
void Fb::clear() { memset(px, 0, (size_t)w * h); }

void Fb::pixel(int x, int y, uint8_t c) {
  x += ox;
  y += oy;
  if ((unsigned)x < (unsigned)w && (unsigned)y < (unsigned)h) px[y * w + x] = c;
}

void Fb::vline(int x, int y, int n, uint8_t c) {
  for (int i = 0; i < n; i++) pixel(x, y + i, c);
}

void Fb::hline(int x, int y, int n, uint8_t c) {
  for (int i = 0; i < n; i++) pixel(x + i, y, c);
}

void Fb::fillRect(int x, int y, int rw, int rh, uint8_t c) {
  for (int i = 0; i < rw; i++) vline(x + i, y, rh, c);
}

void Fb::line(int x0, int y0, int x1, int y1, uint8_t c) {
  int dx = iabs(x1 - x0), dy = iabs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;
  while (true) {
    pixel(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 > -dy) { err -= dy; x0 += sx; }
    if (e2 < dx) { err += dx; y0 += sy; }
  }
}

void Fb::fillCircleHelper(int x0, int y0, int r, int corners, int delta, uint8_t c) {
  int f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
  int px_ = x, py_ = y;
  delta += 1;
  while (x < y) {
    if (f >= 0) { y -= 1; ddy += 2; f += ddy; }
    x += 1;
    ddx += 2;
    f += ddx;
    if (x < y + 1) {
      if (corners & 1) vline(x0 + x, y0 - y, 2 * y + delta, c);
      if (corners & 2) vline(x0 - x, y0 - y, 2 * y + delta, c);
    }
    if (y != py_) {
      if (corners & 1) vline(x0 + py_, y0 - px_, 2 * px_ + delta, c);
      if (corners & 2) vline(x0 - py_, y0 - px_, 2 * px_ + delta, c);
      py_ = y;
    }
    px_ = x;
  }
}

void Fb::fillCircle(int x0, int y0, int r, uint8_t c) {
  vline(x0, y0 - r, 2 * r + 1, c);
  fillCircleHelper(x0, y0, r, 3, 0, c);
}

void Fb::fillRoundRect(int x, int y, int rw, int rh, int r, uint8_t c) {
  if (rw <= 0 || rh <= 0) return;
  r = imax(0, imin(r, imin(rw / 2, rh / 2)));
  fillRect(x + r, y, rw - 2 * r, rh, c);
  if (r) {
    fillCircleHelper(x + rw - r - 1, y + r, r, 1, rh - 2 * r - 1, c);
    fillCircleHelper(x + r, y + r, r, 2, rh - 2 * r - 1, c);
  }
}

void Fb::fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c) {
  int px_[3] = {x0, x1, x2}, py_[3] = {y0, y1, y2};
  int ymin = imin(y0, imin(y1, y2)), ymax = imax(y0, imax(y1, y2));
  for (int y = ymin; y <= ymax; y++) {
    int xs[6], n = 0;
    for (int e = 0; e < 3; e++) {
      int a = e, b = (e + 1) % 3;
      int ax = px_[a], ay = py_[a], bx = px_[b], by = py_[b];
      if (ay == by) {
        if (ay == y) { xs[n++] = ax; xs[n++] = bx; }
      } else if (imin(ay, by) <= y && y <= imax(ay, by)) {
        xs[n++] = rnd(ax + (double)((bx - ax) * (y - ay)) / (by - ay));
      }
    }
    if (n) {
      int lo = xs[0], hi = xs[0];
      for (int i = 1; i < n; i++) { lo = imin(lo, xs[i]); hi = imax(hi, xs[i]); }
      hline(lo, y, hi - lo + 1, c);
    }
  }
}

void Fb::curve(double x0, double y0, double x1, double y1, double x2, double y2, int thick, uint8_t c) {
  bool have = false;
  int px_ = 0, py_ = 0;
  for (int i = 0; i < 11; i++) {
    double t = i / 10.0, u = 1 - t;
    double x = u * u * x0 + 2 * u * t * x1 + t * t * x2;
    double y = u * u * y0 + 2 * u * t * y1 + t * t * y2;
    int cx = rnd(x), cy = rnd(y);
    if (have)
      for (int d = 0; d < thick; d++) line(px_, py_ + d, cx, cy + d, c);
    px_ = cx;
    py_ = cy;
    have = true;
  }
}

static const uint8_t GLYPH_z[5] = {0x44, 0x64, 0x54, 0x4C, 0x44};
static const uint8_t GLYPH_Z[5] = {0x61, 0x51, 0x49, 0x45, 0x43};
static const uint8_t GLYPH_BANG[5] = {0x00, 0x00, 0x5F, 0x00, 0x00};

void Fb::ch(int x, int y, char g, int size, uint8_t c) {
  const uint8_t *gl = g == 'z' ? GLYPH_z : g == 'Z' ? GLYPH_Z : GLYPH_BANG;
  for (int col = 0; col < 5; col++)
    for (int row = 0; row < 8; row++)
      if (gl[col] >> row & 1) fillRect(x + col * size, y + row * size, size, size, c);
}

void Fb::blit(const Fb &src, int x, int y, bool flip) {
  for (int sy = 0; sy < src.h; sy++)
    for (int sx = 0; sx < src.w; sx++)
      if (src.px[sy * src.w + sx]) pixel(x + (flip ? src.w - 1 - sx : sx), y + sy);
}

// ================================================================ buffer kerja & sprite
static const int SPR_W = 42, SPR_H = 26, GROUND = 23, SPR_Y0 = 30;
static uint8_t bufFace[W * H], bufCat[SPR_W * SPR_H], bufMouse[12 * 6], bufFish[14 * 8], bufPeek[34 * 30];
static Fb gFace(W, H, bufFace), gCat(SPR_W, SPR_H, bufCat), gMouse(12, 6, bufMouse), gFish(14, 8, bufFish),
    gPeek(34, 30, bufPeek);
static const double SIT_HEAD_X = 21.0, SIT_HEAD_Y = 6.5;
static const int EYE_CY = 25;
static const char *HEART[5] = {"01010", "11111", "11111", "01110", "00100"};
static const int WHISK[3][4] = {{53, 43, 32, 40}, {53, 47, 29, 47}, {53, 51, 32, 54}};  // sx, sy, ex, ey

// ================================================================ pose kucing
enum PoseId { PS_SIT, PS_ALERT, PS_WALK, PS_RUN, PS_STALK, PS_FREEZE, PS_WIGGLE, PS_POUNCE, PS_CATCH, PS_SPIN,
              PS_DIZZY, PS_STRETCH1, PS_ARCH, PS_SCRATCH, PS_LOAF, NPOSE };
static Pose gPoses[NPOSE];
static bool gInit = false;

static void buildPoses() {
  Pose p;
  p = Pose(); p.sitting = true; p.headDy = -1; p.ear = 0; p.tail = -0.5f; gPoses[PS_SIT] = p;
  p = Pose(); p.crouch = 0.15f; p.headDy = -1; p.ear = 0; p.tail = 0.3f; p.pupil2 = true; gPoses[PS_ALERT] = p;
  p = Pose(); p.crouch = 0; p.ear = 0; p.tail = 0.9f; p.hasGait = true; p.gaitAmp = 3; p.gaitLift = 2; gPoses[PS_WALK] = p;
  p = Pose(); p.crouch = 0; p.ear = 0.4f; p.tail = 0.2f; p.hasGait = true; p.gaitAmp = 4; p.gaitLift = 3; gPoses[PS_RUN] = p;
  p = Pose(); p.crouch = 1; p.headDy = 2; p.ear = 0.35f; p.tail = -0.8f; p.hasGait = true; p.gaitAmp = 1.5f; p.gaitLift = 1;
  p.pupil2 = true; gPoses[PS_STALK] = p;
  p = Pose(); p.crouch = 1; p.headDy = 2; p.ear = 0.3f; p.tail = -0.8f; p.pupil2 = true; gPoses[PS_FREEZE] = p;
  p.wiggle = true; gPoses[PS_WIGGLE] = p;
  p = Pose(); p.crouch = 0.1f; p.headDy = -1; p.ear = 0.4f; p.tail = 0.5f; p.pupil2 = true; p.mouth = true; p.hasLegs = true;
  { static const double L[4][2] = {{-6, 0}, {-7, 0}, {6, 6}, {7, 7}}; memcpy(p.legs, L, sizeof(L)); }
  gPoses[PS_POUNCE] = p;
  p = Pose(); p.crouch = 0.9f; p.headDy = 3; p.ear = 0.6f; p.tail = -0.3f; p.mouth = true; p.prey = true; gPoses[PS_CATCH] = p;
  p = Pose(); p.sitting = true; p.headDy = 1; p.ear = 0.2f; p.tail = 0.9f; p.wagAmp = 3; p.wagSpd = 6; p.pupil2 = true;
  gPoses[PS_SPIN] = p;
  p = Pose(); p.sitting = true; p.headDy = 2; p.ear = 0.5f; p.tail = -0.2f; p.eyesClosed = true; gPoses[PS_DIZZY] = p;
  p = Pose(); p.crouch = 0.5f; p.headDy = 3; p.ear = 0.2f; p.tail = 0.9f; p.tilt = 4; p.hasLegs = true;
  { static const double L[4][2] = {{0, 0}, {1, 0}, {7, 0}, {8, 0}}; memcpy(p.legs, L, sizeof(L)); }
  gPoses[PS_STRETCH1] = p;
  p = Pose(); p.crouch = 0; p.headDy = 1; p.ear = 0.5f; p.tail = 1.0f; p.arch = 5; p.mouth = true; p.eyesClosed = true;
  gPoses[PS_ARCH] = p;
  p = Pose(); p.sitting = true; p.headDy = 0; p.ear = 0.3f; p.tail = -0.4f; p.eyesClosed = true; gPoses[PS_SCRATCH] = p;
  p = Pose(); p.crouch = 1.4f; p.headDy = 3; p.ear = 0.5f; p.tail = -0.5f; p.eyesClosed = true; p.noLegs = true;
  gPoses[PS_LOAF] = p;
}

static void ensureInit() {
  if (gInit) return;
  gInit = true;
  buildPoses();
  gFish.clear();
  gFish.fillRoundRect(0, 1, 10, 6, 3);
  gFish.fillTriangle(9, 4, 13, 0, 13, 7);
  gFish.pixel(2, 3, 0);
}

// ================================================================ sprite
static void catSitting(double ph, const Pose &P) {
  Fb &s = gCat;
  s.clear();
  double ear = P.ear;
  int top = 3 + rnd(P.headDy) + rnd(P.bob);
  double tail = P.tail;
  double wspd = P.wagSpd >= 0 ? P.wagSpd : 1.2, wamp = P.wagAmp >= 0 ? P.wagAmp : (tail > 0 ? 1.5 : 0.7);
  int flick = rnd(sin(ph * wspd) * wamp);
  s.curve(7, 20, -2, 25, 3, 14 + flick, 2);                       // ekor
  s.fillRoundRect(5, 12, 13, 12, 5);                              // paha belakang
  s.fillRoundRect(13, top + 7, 9, 24 - (top + 7), 3);             // dada & badan tegak
  s.fillRoundRect(15, top, 13, 10, 4);                            // kepala
  s.fillTriangle(16, top + 1, rnd(17 - ear * 2), rnd(top - 4 + ear * 3), 21, top + 1);   // telinga
  s.fillTriangle(23, top + 1, rnd(25 - ear * 2), rnd(top - 4 + ear * 3), 27, top + 2);
  if (P.eyesClosed) {
    s.hline(23, top + 4, 3, 0);
  } else {
    bool big = P.pupil2;
    s.fillRect(23, top + 3, 2, big ? 3 : 2, 0);
    s.pixel(23, top + 3);
  }
  s.pixel(27, top + 5, 0);
  if (P.mouth) s.hline(25, top + 7, 3, 0);
  s.fillRoundRect(18, 21, 7, 3, 1);                               // kaki depan
  s.fillRoundRect(3, 21, 9, 3, 1);                                // kaki belakang
  s.vline(17, top + 13, 8, 0);                                    // celah paha / kaki depan
  if (P.hasScratch) {
    int fx = 22 + rnd(P.scratch * 2), fy = top + 2;
    for (int ox = -1; ox <= 2; ox++) s.line(12 + ox, 18, fx + ox, fy, 0);
    for (int d = 0; d < 2; d++) s.line(12 + d, 18, fx + d, fy);
    s.fillRect(fx - 1, fy - 1, 3, 3);
  }
}

static void catSprite(double ph, const Pose &P) {
  if (P.sitting) {
    catSitting(ph, P);
    return;
  }
  Fb &s = gCat;
  s.clear();
  double cr = P.crouch, ear = P.ear, tail = P.tail, tilt = P.tilt, arch = P.arch;
  int dy = rnd(5 * cr);
  int wob = P.wiggle ? (sin(ph) > 0 ? 1 : 0) : 0;
  int base = 8 + dy + rnd(P.bob);
  int byR = base - rnd(tilt) + wob, byF = base;
  int hy = base - 4 + rnd(P.headDy);
  int hipR = byR + 7, hipF = byF + 7;
  double wspd = P.wagSpd >= 0 ? P.wagSpd : 1.0, wamp = P.wagAmp >= 0 ? P.wagAmp : (tail > 0.6 ? 2 : 1);
  double wag = sin(ph * 0.5 * wspd) * wamp;
  if (P.wiggle) wag += sin(ph * 3) * 1.5;
  s.curve(7, byR + 3, 0, byR + 8, 2, rnd(byR + 3 - tail * 9 + wag), 2);  // ekor
  for (int x = 6; x < 23; x++) {                                          // badan
    double u = (x - 6) / 16.0;
    int top = rnd(byR + (byF - byR) * u - arch * 4 * u * (1 - u));
    int trim = (x == 6 || x == 22) ? 1 : 0;
    s.fillRect(x, top + trim, 1, 8 - 2 * trim);
  }
  s.fillRoundRect(21, hy, 12, 9, 3);                                      // kepala
  s.fillTriangle(22, hy + 1, rnd(23 - ear * 2), rnd(hy - 4 + ear * 3), 26, hy + 1);
  s.fillTriangle(28, hy + 1, rnd(30 - ear * 2), rnd(hy - 4 + ear * 3), 32, hy + 2);
  if (P.eyesClosed) {
    s.hline(29, hy + 4, 3, 0);
  } else {
    bool big = P.pupil2;
    s.fillRect(29, hy + 3, 2, big ? 3 : 2, 0);
    s.pixel(29, hy + 3);
  }
  s.pixel(32, hy + 5, 0);
  if (P.mouth) s.hline(30, hy + 7, 3, 0);
  if (P.prey) {
    s.fillRect(33, hy + 6, 4, 3);
    s.line(37, hy + 7, 40, hy + 9);
  }
  if (!P.noLegs) {
    static const int HX[4] = {8, 11, 18, 21};
    static const double OFF[4] = {0.0, 2.1, 3.14, 5.2};
    for (int i = 0; i < 4; i++) {
      if (P.hasScratch && i == 1) continue;
      int dx;
      double lift;
      if (P.hasLegs) {
        dx = (int)P.legs[i][0];
        lift = P.legs[i][1];
      } else if (P.hasGait) {
        double a = ph + OFF[i];
        dx = rnd(sin(a) * P.gaitAmp);
        lift = dmax(0.0, cos(a)) * P.gaitLift;
      } else {
        dx = 0;
        lift = 0;
      }
      int hip = i < 2 ? hipR : hipF;
      for (int d = 0; d < 2; d++) s.line(HX[i] + d, hip, HX[i] + d + dx, rnd(GROUND - lift));
    }
  }
  if (P.hasScratch) {
    int fx = 22 + rnd(P.scratch * 2), fy = hy - 3;
    for (int d = 0; d < 2; d++) {
      s.line(13 + d, byR + 1, fx + d, fy);
      s.line(13 + d, byR + 1, fx + d, fy + 1);
    }
  }
}

static void mouseSprite(double ph) {
  Fb &m = gMouse;
  m.clear();
  m.fillRoundRect(3, 1, 7, 4, 2);
  m.pixel(10, 3);
  m.pixel(8, 0);
  m.pixel(9, 2, 0);
  m.line(3, 4, 0, 2);
  if ((int)(ph * 6) % 2) { m.pixel(4, 5); m.pixel(7, 5); }
  else { m.pixel(5, 5); m.pixel(8, 5); }
}

static void peekSprite(double look) {
  Fb &k = gPeek;
  k.clear();
  k.fillRoundRect(2, 7, 30, 23, 9);
  k.fillTriangle(3, 12, 5, 1, 12, 8);
  k.fillTriangle(31, 12, 29, 1, 22, 8);
  int lk = rnd(look * 2);
  static const int EX[2] = {9, 21};
  for (int i = 0; i < 2; i++) {
    k.fillRoundRect(EX[i] + lk, 13, 5, 8, 2, 0);
    k.pixel(EX[i] + lk + 1, 14);
  }
  k.fillRect(16, 23, 2, 1, 0);
}

static void drawPeek(Fb &fb, Edge edge, int vis, int pos, double look) {
  peekSprite(look);
  for (int r = 0; r < gPeek.h; r++)
    for (int c = 0; c < gPeek.w; c++)
      if (gPeek.px[r * gPeek.w + c]) {
        int x, y;
        if (edge == EDGE_BAWAH) { x = pos + c - 17; y = H - vis + r; }
        else if (edge == EDGE_ATAS) { x = pos + c - 17; y = vis - 1 - r; }
        else if (edge == EDGE_KIRI) { x = vis - 1 - r; y = pos + c - 17; }
        else { x = W - vis + r; y = pos + c - 17; }
        fb.pixel(x, y);
      }
}

// ================================================================ gambar wajah
static void eyeDraw(Fb &fb, const Face &F, int side, double b) {
  double w = F.p[P_W] * (1 + 0.14 * b);
  double h = F.p[P_H] + (2.5 - F.p[P_H]) * b;
  double cx = 64.5 + side * 24.5 + F.p[P_GX] * 6;
  double cy = EYE_CY + F.p[P_GY] * 4 + side * F.p[P_HTILT] * 2.5;
  int x0 = rnd(cx - w / 2), y0 = rnd(cy - h / 2), wi = imax(2, rnd(w)), hi = imax(2, rnd(h));
  y0 += rnd(hi * F.p[P_LB] * 0.5);

  if (F.geli) {  // "> <"
    int outer = rnd(cx + 7 * side), tip = rnd(cx - 5 * side);
    for (int d = 0; d < 2; d++) {
      fb.line(outer, rnd(cy) - 6 + d, tip, rnd(cy) + d);
      fb.line(tip, rnd(cy) + d, outer, rnd(cy) + 6 + d);
    }
    return;
  }
  if (hi <= 5) {  // terpejam
    double sag = F.p[P_CURVE] * 3;
    fb.curve(x0, rnd(cy), rnd(cx), rnd(cy + 2 * sag), x0 + wi, rnd(cy), 2);
    return;
  }
  fb.fillRoundRect(x0, y0, wi, hi, rnd(dmin(F.p[P_R], dmin(w / 2, h / 2))));
  if (F.p[P_PW] >= 1 && F.p[P_PH] >= 1) {
    int pw = rnd(F.p[P_PW]), ph = rnd(F.p[P_PH] * dmin(1.0, h / 22));
    int px = rnd(cx + F.p[P_GX] * 3 - pw / 2.0);
    int py = rnd(cy + F.p[P_GY] * 1.5 - ph / 2.0 + (F.p[P_LT] * h * 0.35));
    fb.fillRoundRect(px, py, pw, ph, pw / 2, 0);
    if (pw >= 6 && ph >= 8) fb.fillRect(side < 0 ? px + 1 : px + pw - 3, py + 2, 2, 2);
  }
  if (F.p[P_LT] > 0.02) {
    double top = y0 + F.p[P_LT] * hi;
    double outY = top + F.p[P_TILT] * hi * 0.3, inY = top - F.p[P_TILT] * hi * 0.3;
    int xo = side < 0 ? x0 - 1 : x0 + wi + 1, xi = side < 0 ? x0 + wi + 1 : x0 - 1;
    int yo = rnd(outY), yi = rnd(inY);
    fb.fillTriangle(xo, y0 - 3, xi, y0 - 3, xi, yi, 0);
    fb.fillTriangle(xo, y0 - 3, xo, yo, xi, yi, 0);
  }
  if (F.p[P_LB] > 0.02) {
    int yb = rnd(y0 + hi - F.p[P_LB] * hi);
    fb.fillRect(x0 - 1, yb, wi + 2, y0 + hi - yb + 2, 0);
  }
}

static void earsDraw(Fb &fb, const Face &F) {
  for (int side = -1; side <= 1; side += 2) {
    double e = clampd(side < 0 ? F.p[P_EARL] : F.p[P_EARR], -0.2, 1.0);
    double tx, ty;
    if (e <= 0.5) {
      double u = dmax(0.0, e) * 2;
      tx = 15 - 13 * u;
      ty = -3 + 10 * u;
    } else {
      double u = (e - 0.5) * 2;
      tx = 2 + 4 * u;
      ty = 7 + 6 * u;
    }
    double pts[3][2] = {{7, 11}, {24, 11}, {tx, ty}};
    double inner[3][2] = {{12, 10}, {19, 10}, {15 + (tx - 15) * 0.4, 10 + (ty - 10) * 0.3}};
    if (side > 0) {
      for (int i = 0; i < 3; i++) { pts[i][0] = 128 - pts[i][0]; inner[i][0] = 128 - inner[i][0]; }
    }
    fb.fillTriangle(rnd(pts[0][0]), rnd(pts[0][1]), rnd(pts[1][0]), rnd(pts[1][1]), rnd(pts[2][0]), rnd(pts[2][1]));
    if (e < 0.4)
      fb.fillTriangle(rnd(inner[0][0]), rnd(inner[0][1]), rnd(inner[1][0]), rnd(inner[1][1]), rnd(inner[2][0]),
                      rnd(inner[2][1]), 0);
  }
}

static void mouthDraw(Fb &fb, const Face &F, int mx) {
  const int my = 44;
  double mo = dmax(F.p[P_MO], F.jaw ? 0.45 : 0.0);
  if (mo > 0.25) {
    int mw = imax(5, rnd(F.p[P_MW])), mh = imax(5, rnd(mo * 14));
    int x0 = mx - mw / 2, y0 = my - 1;
    fb.vline(mx, 43, y0 - 43);
    fb.fillRoundRect(x0, y0, mw, mh, imin(mw, mh) / 2);
    fb.fillRoundRect(x0 + 2, y0 + 2, mw - 4, mh - 4, imax(0, imin(mw - 4, mh - 4) / 2), 0);
    if (mo > 0.5 && mw >= 8) fb.fillRoundRect(x0 + 3, y0 + mh - 5, mw - 6, 3, 1);
    return;
  }
  fb.vline(mx, 43, my - 43);
  for (int s = -1; s <= 1; s += 2)
    fb.curve(mx, my, mx + s * F.p[P_MW] / 2, my + 2 * F.p[P_MC], mx + s * F.p[P_MW], my, 1);
  if (F.p[P_TONGUE] > 0.3) fb.fillRoundRect(mx - 2, rnd(my + F.p[P_MC] + 1), 4, rnd(2 + F.p[P_TONGUE] * 3), 2);
}

static void whiskersDraw(Fb &fb, const Face &F, int shift) {
  for (int i = 0; i < 3; i++) {
    int sx = WHISK[i][0], sy = WHISK[i][1];
    double ex = sx + (WHISK[i][2] - sx) * F.p[P_WLEN], ey = sy + (WHISK[i][3] - sy) * F.p[P_WLEN];
    ey = ey + (sy - ey) * F.p[P_WFLAT] - F.p[P_WH] * 3 + sin(F.t / 85 + i * 1.7) * F.p[P_AMP];
    for (int side = -1; side <= 1; side += 2) {
      bool tw = F.twitch == 3 || F.twitch == (side < 0 ? 1 : 2);
      double e = ey - (tw ? 2 : 0) + side * F.p[P_HTILT] * 1.5;
      double xs = side < 0 ? sx : 128 - sx, xe = side < 0 ? ex : 128 - ex;
      double mx = (xs + xe) / 2, my = (sy + e) / 2 + 1.2;
      fb.curve(rnd(xs) + shift, sy, rnd(mx) + shift, rnd(my), rnd(xe) + shift, rnd(e), 1);
    }
  }
}

static void pawDraw(Fb &fb, int cx, int cy, int w = 12, int h = 14) {
  fb.fillRoundRect(cx - w / 2 - 1, cy - h / 2 - 1, w + 2, h + 2, w / 2 + 1, 0);
  fb.fillRoundRect(cx - w / 2, cy - h / 2, w, h, w / 2);
  int d[2] = {fdiv(-w, 6), w / 6};
  for (int i = 0; i < 2; i++) fb.vline(cx + d[i], cy + h / 8, imax(2, h / 3), 0);
}

static void birdDraw(Fb &fb, double bx, double by, int flap, int d) {
  int x = rnd(bx), y = rnd(by);
  auto px = [&](int v) { return x + (d > 0 ? v : 8 - v); };
  fb.fillRoundRect(imin(px(0), px(7)), y + 1, 7, 4, 2);
  fb.pixel(px(8), y + 2);
  fb.line(px(-3), y + 2, px(0), y + 2);
  int wing = flap ? -3 : 3;
  fb.line(px(2), y + 1, px(4), y + 1 + wing);
  fb.line(px(3), y + 1, px(5), y + 1 + wing);
}

static void faceDraw(Fb &fb, const Face &F) {
  fb.clear();
  fb.ox = rnd(F.shakeX);
  fb.oy = rnd(F.shakeY + F.p[P_FY] + F.breath);
  earsDraw(fb, F);
  eyeDraw(fb, F, -1, F.bL);
  eyeDraw(fb, F, 1, F.bR);
  int ny = F.nose, nx = rnd(F.p[P_HTILT] * 1.5);
  fb.fillTriangle(61 + nx, 39 + ny, 67 + nx, 39 + ny, 64 + nx, 43 + ny);
  mouthDraw(fb, F, 64 + nx);
  whiskersDraw(fb, F, nx);
  if (F.p[P_BLUSH] > 0.5) {
    for (int yy = 33; yy < 36; yy++)
      for (int xx = 28; xx < 37; xx++)
        if ((xx + yy) % 2 == 0) {
          fb.pixel(xx, yy);
          fb.pixel(128 - xx, yy);
        }
  }
  fb.ox = fb.oy = 0;
  for (int i = 0; i < F.nHearts; i++) {
    int hx = rnd(F.hearts[i][0]), hy = rnd(F.hearts[i][1]);
    for (int r = 0; r < 5; r++)
      for (int c = 0; c < 5; c++)
        if (HEART[r][c] == '1') fb.pixel(hx + c, hy + r);
  }
  if (F.hasFish) {
    int fx = rnd(F.fishX) - 7, fy = rnd(F.fishY);
    for (int sy = 0; sy < gFish.h; sy++)
      for (int sx = 0; sx < gFish.w; sx++)
        if (gFish.px[sy * gFish.w + sx] && fy + sy <= 41) fb.pixel(fx + sx, fy + sy);
  }
  for (int i = 0; i < F.nCrumbs; i++) fb.fillRect(rnd(F.crumbs[i][0]), rnd(F.crumbs[i][1]), 2, 1);
  if (F.hasPaw) pawDraw(fb, rnd(F.pawX), rnd(F.pawY));
  if (F.hasKnead) {
    pawDraw(fb, 46, rnd(F.kneadL), 10, 10);
    pawDraw(fb, 82, rnd(F.kneadR), 10, 10);
  }
  if (F.hasBird) birdDraw(fb, F.birdX, F.birdY, F.birdFlap, F.birdDir);
  if (F.exclaim) fb.ch(59, 0, '!', 2);
  for (int i = 0; i < F.nzs; i++) fb.ch(rnd(F.zs[i][0]), rnd(F.zs[i][1]), F.zs[i][2] > 0.5f ? 'Z' : 'z', F.zs[i][2] > 0.5f ? 2 : 1);
}

// ---------------------------------------------------------------- adegan kucing
// Dithering 4x4 untuk peralihan ala 8-bit. Ambang dihitung sekali per gambar (tanpa hitungan desimal per piksel).
static const uint8_t BAYER[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};

static void visTable(double alpha, bool tbl[16]) {
  for (int i = 0; i < 16; i++) tbl[i] = alpha > (BAYER[i] + 0.5) / 16;
}

static inline bool visAt(const bool tbl[16], int x, int y) { return tbl[pmod(y, 4) * 4 + pmod(x, 4)]; }

// Tempel src (128x64) diperkecil sc kali di sekitar (cx, cy); alpha < 1 = pudar dengan dithering.
// Posisi sumber per kolom/baris dihitung sekali (hasil sama persis dengan simulasi), loop dalam hanya bilangan bulat.
static void blitScaled(Fb &dst, const Fb &src, double cx, double cy, double sc, double alpha) {
  static int colSx[W], rowSy[H];
  bool tbl[16];
  visTable(alpha, tbl);
  double inv = 1 / sc;
  int blk = imax(1, (int)ceil(inv));
  double hw = 64 * sc, hh = 32 * sc;
  int dy0 = imax(0, (int)(cy - hh) - 1), dy1 = imin(H, (int)(cy + hh) + 2);
  int dx0 = imax(0, (int)(cx - hw) - 1), dx1 = imin(W, (int)(cx + hw) + 2);
  for (int dx = dx0; dx < dx1; dx++) colSx[dx] = rnd((dx - cx) * inv + 64 - blk / 2.0);
  for (int dy = dy0; dy < dy1; dy++) rowSy[dy] = rnd((dy - cy) * inv + 32 - blk / 2.0);
  int need = (int)ceil(dmax(1.0, blk * blk * 0.3));
  for (int dy = dy0; dy < dy1; dy++) {
    int sy0 = rowSy[dy], ya = imax(0, sy0), yb = imin(H, sy0 + blk);
    for (int dx = dx0; dx < dx1; dx++) {
      if (!visAt(tbl, dx, dy)) continue;
      int sx0 = colSx[dx], xa = imax(0, sx0), xb = imin(W, sx0 + blk), hit = 0;
      for (int yy = ya; yy < yb && hit < need; yy++) {
        const uint8_t *row = src.px + yy * W;
        for (int xx = xa; xx < xb; xx++) hit += row[xx];
      }
      if (hit >= need) dst.pixel(dx, dy);
    }
  }
}

static void dustDraw(Fb &fb, const Face &F) {
  for (int i = 0; i < F.nDust; i++) {
    double age = F.dust[i][2];
    int sz = age < 150 ? 3 : age < 300 ? 2 : 1;
    fb.fillRect(rnd(F.dust[i][0]) - sz / 2, rnd(F.dust[i][1]) - sz / 2, sz, sz);
  }
}

static void morphDraw(Fb &fb, const Face &F) {
  const Cat &c = F.cat;
  double u = c.u, e = smooth(0, 1, u);
  faceDraw(gFace, F);
  fb.clear();
  fb.ox = fb.oy = 0;
  dustDraw(fb, F);
  double hxSprite = c.flip ? (SPR_W - 1 - SIT_HEAD_X) : SIT_HEAD_X;
  double hx = rnd(c.x) - SPR_W / 2 + hxSprite, hy = SPR_Y0 + c.y + SIT_HEAD_Y;
  catSprite(0.0, gPoses[PS_SIT]);
  int x0 = rnd(c.x) - SPR_W / 2, y0 = SPR_Y0 + rnd(c.y);
  double reach = e * 44, aBody = smooth(0.1, 0.7, u);
  bool tbl[16];
  visTable(aBody, tbl);
  for (int sy = 0; sy < gCat.h; sy++)
    for (int sx = 0; sx < gCat.w; sx++)
      if (gCat.px[sy * gCat.w + sx] && fabs(sx - SIT_HEAD_X) <= reach) {
        int dx = x0 + (c.flip ? SPR_W - 1 - sx : sx);
        if (visAt(tbl, dx, y0 + sy)) fb.pixel(dx, y0 + sy);
      }
  double aFace = 1 - smooth(0.55, 1.0, u);
  if (aFace > 0) blitScaled(fb, gFace, 64 + (hx - 64) * e, 32 + (hy - 32) * e, 1 - 0.83 * e, aFace);
}

static void fadeDraw(Fb &fb, const Face &F) {
  const Cat &c = F.cat;
  faceDraw(gFace, F);
  fb.clear();
  fb.ox = fb.oy = 0;
  dustDraw(fb, F);
  double a = 1 - smooth(0, 1, c.u);
  if (a > 0) blitScaled(fb, gFace, 64, 32, 1.0 - 0.12 * c.u, a);
}

static void sceneCatDraw(Fb &fb, const Face &F) {
  const Cat &c = F.cat;
  if (c.mode == CM_MORPH) { morphDraw(fb, F); return; }
  if (c.mode == CM_FADE) { fadeDraw(fb, F); return; }
  fb.clear();
  fb.ox = fb.oy = 0;
  if (c.ground)
    for (int x = 0; x < W; x++)
      if (pmod(x + c.scroll, 10) < 5) fb.pixel(x, 55);
  dustDraw(fb, F);
  if (c.mode == CM_SPRITE || c.mode == CM_PLAY) {
    catSprite(c.ph, c.pose);
    fb.blit(gCat, rnd(c.x) - SPR_W / 2, SPR_Y0 + rnd(c.y), c.flip);
    if (c.hasToy && !c.toyHidden) {
      mouseSprite(c.toyPh);
      fb.blit(gMouse, rnd(c.toyX) - 6, 48, c.toyDir < 0);
    }
    for (int i = 0; i < c.nzs; i++)
      fb.ch(rnd(c.zs[i][0]), rnd(c.zs[i][1]), c.zs[i][2] > 0.5f ? 'Z' : 'z', c.zs[i][2] > 0.5f ? 2 : 1);
  } else if (c.mode == CM_DOTS && c.rise > 0.6) {
    int n = (int)(c.rise * 1.5) % 4;
    for (int i = 0; i < n; i++) fb.fillRect(54 + i * 9, 30, 3, 3);
  } else if (c.mode == CM_PEEKEDGE) {
    drawPeek(fb, c.edge, c.vis, c.pos, c.look);
  } else if (c.mode == CM_PEEK) {
    int px = rnd(c.x), base = 64 - rnd(15 * c.rise);
    fb.fillRoundRect(px - 15, base, 30, 24, 9);
    fb.fillTriangle(px - 14, base + 4, px - 12, base - 5, px - 6, base);
    fb.fillTriangle(px + 14, base + 4, px + 12, base - 5, px + 6, base);
    int lk = rnd(c.look * 2);
    fb.fillRoundRect(px - 8 + lk, base + 6, 4, 8, 1, 0);
    fb.fillRoundRect(px + 4 + lk, base + 6, 4, 8, 1, 0);
  }
}

const Pose &posePreset(int id) {
  ensureInit();
  return gPoses[id];
}

void drawFace(Fb &fb, const Face &F) {
  ensureInit();
  if (!F.hasCat) faceDraw(fb, F);
  else sceneCatDraw(fb, F);
}

// ================================================================ logika
static const double STEP_MS = 1000.0 / 60;
static const double HOLD_MS = 500, PURR_SLEEP_MS = 3500, HAPPY_MS = 1800, GELI_MS = 2000, SURPRISE_MS = 2000,
                    LOOK_MS = 900, MORPH_MS = 800, ESC_SIT = 300;
static const double FEED_FALL = 700, FEED_CHOMP = 1000, FEED_GAP = 300;
static const int FEED_MAX = 3;

enum Preset { PR_NORMAL, PR_BOSAN, PR_TIDUR, PR_SENANG, PR_PURR, PR_TERKEJUT, PR_GELI, PR_YAWN, PR_KESAL, PR_MAIN,
              PR_MAKAN_TUNGGU, PR_MAKAN, PR_KENYANG, PR_GROOM, PR_BIRD };

static const float BASE[NPARAM] = {
    22, 22, 11, 0, 0, 0, 8, 12, 0, 0, 1.0f,   // W H R LT TILT LB PW PH CURVE WFLAT WLEN
    2.2f, 0, 5, 0, 0, 0.35f, 0, 0, 0, 0,      // MC MO MW TONGUE WH AMP EARL EARR HTILT BLUSH
    0, 0, 0};                                  // GX GY FY

static const float SPRING_K[NPARAM] = {120, 120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
                                       110, 110, 110, 110, 90, 40, 220, 220, 150, 100, 260, 260, 180};
static const float SPRING_C[NPARAM] = {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
                                       13, 13, 13, 13, 12, 10, 16, 16, 14, 14, 22, 22, 9};

static void applyPreset(float *tg, int p) {
  switch (p) {
    case PR_BOSAN:
      tg[P_H] = 21; tg[P_LT] = 0.42f; tg[P_TILT] = 0.6f; tg[P_PW] = 5; tg[P_PH] = 9; tg[P_MC] = 1.2f; tg[P_MW] = 5;
      tg[P_WH] = -1; tg[P_AMP] = 0.15f; tg[P_EARL] = 0.35f; tg[P_EARR] = 0.35f; tg[P_WLEN] = 1.05f; break;
    case PR_TIDUR:
      tg[P_H] = 3; tg[P_R] = 1; tg[P_CURVE] = 1; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 2.2f; tg[P_WH] = -1;
      tg[P_AMP] = 0.1f; tg[P_EARL] = 0.45f; tg[P_EARR] = 0.45f; break;
    case PR_SENANG:
      tg[P_W] = 24; tg[P_R] = 9; tg[P_LB] = 0.6f; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 3; tg[P_MO] = 0.55f;
      tg[P_MW] = 9; tg[P_WH] = 1; tg[P_AMP] = 0.8f; tg[P_BLUSH] = 1; break;
    case PR_PURR:
      tg[P_W] = 24; tg[P_R] = 9; tg[P_LB] = 0.68f; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 3; tg[P_MO] = 0.15f;
      tg[P_MW] = 6; tg[P_WH] = 1; tg[P_AMP] = 1; tg[P_EARL] = 0.3f; tg[P_EARR] = 0.3f; tg[P_BLUSH] = 1; break;
    case PR_TERKEJUT:
      tg[P_W] = 26; tg[P_H] = 28; tg[P_R] = 13; tg[P_PW] = 4; tg[P_PH] = 4; tg[P_MC] = 0; tg[P_MO] = 0.5f; tg[P_MW] = 7;
      tg[P_WFLAT] = 1; tg[P_AMP] = 0.1f; tg[P_WLEN] = 0.85f; break;
    case PR_GELI:
      tg[P_H] = 22; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 1; tg[P_MO] = 0.9f; tg[P_MW] = 12; tg[P_WH] = 1;
      tg[P_AMP] = 1.6f; tg[P_EARL] = 0.2f; tg[P_EARR] = 0.2f; tg[P_BLUSH] = 1; break;
    case PR_YAWN:
      tg[P_H] = 3; tg[P_R] = 1; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 0; tg[P_MO] = 1; tg[P_MW] = 10; tg[P_WH] = -1;
      tg[P_AMP] = 0.1f; tg[P_EARL] = 0.3f; tg[P_EARR] = 0.3f; break;
    case PR_KESAL:
      tg[P_H] = 21; tg[P_LT] = 0.38f; tg[P_TILT] = -0.8f; tg[P_PW] = 3; tg[P_PH] = 10; tg[P_MC] = -2.2f; tg[P_MW] = 5;
      tg[P_AMP] = 1.5f; tg[P_EARL] = 1; tg[P_EARR] = 1; tg[P_WLEN] = 0.9f; break;
    case PR_MAIN:
      tg[P_W] = 24; tg[P_H] = 25; tg[P_R] = 12; tg[P_PW] = 10; tg[P_PH] = 14; tg[P_WH] = 1; tg[P_AMP] = 1.2f;
      tg[P_WLEN] = 0.85f; break;
    case PR_MAKAN_TUNGGU:
      tg[P_W] = 24; tg[P_H] = 26; tg[P_R] = 12; tg[P_PW] = 10; tg[P_PH] = 14; tg[P_MC] = 2; tg[P_MW] = 6; tg[P_WH] = 1;
      tg[P_AMP] = 0.8f; tg[P_WLEN] = 0.85f; break;
    case PR_MAKAN:
      tg[P_W] = 24; tg[P_R] = 9; tg[P_LB] = 0.5f; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 3; tg[P_MO] = 0.1f;
      tg[P_MW] = 8; tg[P_WH] = 1; tg[P_AMP] = 1; tg[P_EARL] = 0.15f; tg[P_EARR] = 0.15f; tg[P_BLUSH] = 0.6f; break;
    case PR_KENYANG:
      tg[P_W] = 24; tg[P_R] = 9; tg[P_LB] = 0.6f; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 3; tg[P_MO] = 0;
      tg[P_MW] = 8; tg[P_WH] = 1; tg[P_AMP] = 0.6f; tg[P_EARL] = 0.3f; tg[P_EARR] = 0.3f; tg[P_BLUSH] = 1; break;
    case PR_GROOM:
      tg[P_H] = 3; tg[P_R] = 1; tg[P_PW] = 0; tg[P_PH] = 0; tg[P_MC] = 2; tg[P_MO] = 0; tg[P_MW] = 6; tg[P_WH] = 0;
      tg[P_AMP] = 0.3f; tg[P_EARL] = 0.3f; tg[P_EARR] = 0.3f; break;
    case PR_BIRD:
      tg[P_W] = 24; tg[P_H] = 25; tg[P_R] = 12; tg[P_PW] = 10; tg[P_PH] = 14; tg[P_MC] = 2; tg[P_MW] = 6;
      tg[P_WH] = 0.5f; tg[P_AMP] = 1; tg[P_WLEN] = 0.8f; break;
    default: break;
  }
}

static int moodPreset(Mood m) {
  switch (m) {
    case M_BOSAN: return PR_BOSAN;
    case M_TIDUR: return PR_TIDUR;
    case M_SENANG: return PR_SENANG;
    case M_PURR: return PR_PURR;
    case M_TERKEJUT: return PR_TERKEJUT;
    case M_GELI: return PR_GELI;
    default: return PR_NORMAL;
  }
}

static const char *const MOOD_NAMES[] = {"normal", "bosan", "tidur", "senang", "terkejut", "geli", "purr", "makan", "main", "kabur"};

struct KindDef {
  bool script, fade, happy, backRun;
  int pre, prePreset;
  const char *name, *l1;
  int morph, dur, run, awayMin, awayMax, peek, back;
};
static const KindDef KINDS[K_COUNT] = {
    {false, false, true, false, 700, PR_KESAL, "kabur", "kesal", 800, 0, 1100, 3500, 5500, 2600, 2300},
    {false, false, true, true, 450, PR_MAIN, "zoomies", "siap", 500, 0, 900, 300, 300, 0, 1000},
    {true, false, false, false, 0, 0, "jalan", nullptr, 700, 7500, 0, 0, 0, 0, 0},
    {true, false, false, false, 0, 0, "ekor", nullptr, 700, 4600, 0, 0, 0, 0, 0},
    {true, false, false, false, 0, 0, "stretch", nullptr, 700, 4300, 0, 0, 0, 0, 0},
    {true, false, false, false, 0, 0, "garuk", nullptr, 700, 3400, 0, 0, 0, 0, 0},
    {true, false, false, false, 0, 0, "loaf", nullptr, 900, 1000000000, 0, 0, 0, 0, 0},
    {true, true, true, false, 0, 0, "ngumpet", nullptr, 350, 0, 0, 0, 0, 0, 0},
};

static const double EVENT_MS[10] = {0, 380, 1000, 800, 1100, 1800, 1200, 1600, 3800, 3600};
static const Ev EVENTS[11] = {EV_WINK, EV_BLEP, EV_SNIFF, EV_KEPO, EV_SLOWBLINK, EV_SLOWBLINK, EV_TILT, EV_TILT,
                              EV_GROOM, EV_BIRD, EV_BIRD};
static const float GAZES[9][2] = {{-1, 0}, {1, 0}, {0, -0.8f}, {-0.8f, -0.6f}, {0.8f, -0.6f}, {0.6f, 0.7f},
                                  {-0.6f, 0.7f}, {0, 0}, {0, 0}};

static double blinkValue(double dt, double hold = 0) {
  if (dt < 0) return 0;
  if (dt < 70) return dt / 70;
  if (dt < 70 + hold) return 1;
  dt -= 70 + hold;
  return dmax(0.0, 1 - dt / 110);
}

static double slowBlink(double e) {
  if (e < 0 || e >= 1200) return 0;
  if (e < 500) return smooth(0, 1, e / 500);
  if (e < 700) return 1;
  return smooth(0, 1, 1 - (e - 700) / 500);
}

// kaki naik ke mulut, menjilat, mengusap pipi/mata, turun. true = ada kaki; lick = sedang menjilat
static bool groomPaw(double e, double &x, double &y, bool &lick) {
  lick = false;
  if (e < 0 || e > 3800) return false;
  if (e < 600) {
    double u = smooth(0, 1, e / 600);
    x = 96 + (76 - 96) * u; y = 72 + (50 - 72) * u;
    return true;
  }
  if (e < 1500) {
    double j = e - 600;
    x = 76 + sin(j / 60) * 1.5; y = 50 + sin(j / 45) * 1.5; lick = true;
    return true;
  }
  if (e < 3200) {
    double a = (e - 1500) / 1000 * 2 * PI_ * 1.4 + PI_ / 2;
    double wx = 90 + 7 * cos(a), wy = 28 + 9 * sin(a), u = smooth(0, 1, (e - 1500) / 300);
    x = 76 + (wx - 76) * u; y = 50 + (wy - 50) * u;
    return true;
  }
  double u = smooth(0, 1, (e - 3200) / 600);
  double a = (3200 - 1500) / 1000.0 * 2 * PI_ * 1.4 + PI_ / 2;
  double wx = 90 + 7 * cos(a), wy = 28 + 9 * sin(a);
  x = wx + (98 - wx) * u; y = wy + (72 - wy) * u;
  return true;
}

enum PlaySt { W_WATCH, W_STALK, W_FREEZE, W_WIGGLE, W_POUNCE, W_CATCH, W_MISS, W_CHASE, W_REST };
static const char *const PLAY_LABELS[9] = {"mengamati", "mengendap", "membeku", "goyang pantat", "terkam!",
                                           "menangkap", "meleset", "mengejar", "istirahat"};

enum Stat { ST_GAZE = 20, ST_BLINK = 21, ST_MAINSENDIRI = 22 };

// ---------------------------------------------------------------- Engine: awal & masukan
void Engine::begin(uint32_t seed, double now) {
  ensureInit();
  rng.s = seed ? seed : 2463534242u;
  for (int i = 0; i < NPARAM; i++) {
    sp[i].x = sp[i].t = BASE[i];
    sp[i].v = 0;
    sp[i].k = SPRING_K[i];
    sp[i].c = SPRING_C[i];
  }
  t = now;
  lastInput = now;
  for (int i = 0; i < 3; i++) isDown[i] = false;
  happyUntil = geliUntil = surpriseUntil = lookUntil = 0;
  lookGx = 0;
  nOk = 0;
  blinkAt = now + rng.randint(2500, 5000);
  blink2 = false;
  evKind = EV_NONE; evStart = evUntil = 0; evSide = -1; evDir = 1;
  nextEv = now + rng.randint(3000, 6000);
  gzNext = now + 1500; gzX = gzY = 0;
  driftAt = now; driftX = driftY = 0;
  twitchAt = now + rng.randint(4000, 8000); twitchUntil = 0; twitchSide = 1;
  earAt = now + rng.randint(2000, 5000); dreamAt = dreamUntil = 0;
  nHearts = 0; heartAt = 0; nDust = 0; dustAt = 0; nCrumbs = 0;
  esc = Esc();
  plActive = false; sceneMain = false;
  morph = morphTarget = 0; catX = 64; catY = 0; catPh = 0; catDir = 1;
  feedT0 = -1; feedGy = 0; chewK = -1; hearty = false;
  yawnRolled = -1; sleepT0 = sleepDur = 0; pendingGroom = pendingZoom = 0; autoPlayUntil = 0;
  prevMood = moodNow = M_NORMAL;
  labelA = "normal"; labelB = nullptr;
  haveF = false;
  memset(stats, 0, sizeof(stats));
  resetPose();
}

void Engine::resetPose() {
  poseCur = Pose();
  const Pose &s = gPoses[PS_SIT];
  poseCur.crouch = s.crouch; poseCur.headDy = s.headDy; poseCur.ear = s.ear; poseCur.tail = s.tail;
  poseCur.tilt = s.tilt; poseCur.arch = s.arch; poseCur.bob = s.bob;
}

Pose Engine::easePose(const Pose &target, double dt) {
  double k = 1 - exp(-dt / 0.12);
  poseCur.crouch += (target.crouch - poseCur.crouch) * k;
  poseCur.headDy += (target.headDy - poseCur.headDy) * k;
  poseCur.ear += (target.ear - poseCur.ear) * k;
  poseCur.tail += (target.tail - poseCur.tail) * k;
  poseCur.tilt += (target.tilt - poseCur.tilt) * k;
  poseCur.arch += (target.arch - poseCur.arch) * k;
  poseCur.bob += (target.bob - poseCur.bob) * k;
  Pose P = target;
  P.crouch = poseCur.crouch; P.headDy = poseCur.headDy; P.ear = poseCur.ear; P.tail = poseCur.tail;
  P.tilt = poseCur.tilt; P.arch = poseCur.arch; P.bob = poseCur.bob;
  return P;
}

void Engine::press(double now, int btn) {
  advance(now);
  lastInput = now;
  autoPlayUntil = 0;
  isDown[btn] = true;
  downAt[btn] = now;
  if (btn == 0 || btn == 1) {
    lookUntil = now + LOOK_MS;
    lookGx = btn == 0 ? -1 : 1;
  }
}

void Engine::release(double now, int btn) {
  advance(now);
  if (!isDown[btn]) return;
  isDown[btn] = false;
  double held_ = now - downAt[btn];
  lastInput = now;
  if (btn != 2) return;
  if (held_ >= PURR_SLEEP_MS) {
    startEvent(EV_YAWN, now);
  } else if (held_ >= HOLD_MS) {
    happyUntil = now + 1000;
  } else {
    if (esc.active) { callEscape(now); return; }
    int n = 0;
    for (int i = 0; i < nOk; i++)
      if (now - okTimes[i] < 2500 && n < 7) okTimes[n++] = okTimes[i];
    okTimes[n++] = now;
    nOk = n;
    int recent = 0;
    for (int i = 0; i < nOk; i++) if (now - okTimes[i] < 1500) recent++;
    if (nOk >= 6) startEscape(now, K_KABUR);
    else if (recent >= 3) geliUntil = now + GELI_MS;
    else {
      happyUntil = now + HAPPY_MS;
      sp[P_FY].v -= 70;
      heartAt = now;
    }
  }
}

void Engine::notify(double now) {
  advance(now);
  surpriseUntil = now + SURPRISE_MS;
  lastInput = now;
  sp[P_FY].v -= 110;
  evUntil = 0;
}

void Engine::sleepNow(double now) {
  advance(now);
  lastInput = now - sleepAfter;
}

Mood Engine::moodAt(double tt) const {
  if (esc.active) return M_KABUR;
  if (sceneMain) return M_MAIN;
  if (held(0, tt) >= HOLD_MS) return M_MAKAN;
  double ok = held(2, tt);
  if (tt < geliUntil) return M_GELI;
  if (ok >= PURR_SLEEP_MS) return M_TIDUR;
  if (ok >= HOLD_MS) return M_PURR;
  if (tt < happyUntil) return M_SENANG;
  if (tt < surpriseUntil) return M_TERKEJUT;
  double idle = tt - lastInput;
  return idle >= sleepAfter ? M_TIDUR : idle >= boredAfter ? M_BOSAN : M_NORMAL;
}

void Engine::startEvent(Ev ev, double tt) {
  evKind = ev;
  evStart = tt;
  evUntil = tt + EVENT_MS[ev];
  evSide = rng.randint(0, 1) ? 1 : -1;
  evDir = rng.randint(0, 1) ? 1 : -1;
  stats[ev]++;
}

void Engine::puff(double tt, double x, double y) {
  if (nDust >= MAX_DUST) return;
  dust[nDust][0] = (float)x; dust[nDust][1] = (float)y; dust[nDust][2] = (float)tt;
  nDust++;
}

// ---------------------------------------------------------------- kabur / zoomies / kegiatan sendiri
void Engine::startEscape(double tt, Kind kind) {
  if (esc.active) return;
  const KindDef &k = KINDS[kind];
  Esc e;
  e.nPeeks = 0;
  double total = 0;
  if (kind == K_NGUMPET) {
    Edge edges[4] = {EDGE_ATAS, EDGE_KIRI, EDGE_KANAN, EDGE_BAWAH};
    for (int i = 3; i > 0; i--) { int j = rng.randint(0, i); Edge tmp = edges[i]; edges[i] = edges[j]; edges[j] = tmp; }
    int n = rng.randint(3, 4);
    total = 900;
    for (int i = 0; i < n; i++) {
      Peek &pk = e.peeks[i];
      pk.edge = edges[i];
      pk.hold = rng.randint(900, 1700);
      pk.pos = (pk.edge == EDGE_ATAS || pk.edge == EDGE_BAWAH) ? rng.randint(30, 98) : rng.randint(18, 46);
      pk.t0 = total;
      pk.dur = 450 + pk.hold + 450;
      total += pk.dur + rng.randint(700, 1500);
    }
    e.nPeeks = n;
  }
  e.poof = k.pre;
  e.run = e.poof + k.morph;
  if (k.script) {
    e.gone = e.run + (kind == K_NGUMPET ? total : k.dur);
    e.end = e.gone + k.morph;
  } else {
    e.gone = e.run + k.run;
    e.peek = e.gone + rng.randint(k.awayMin, k.awayMax);
    e.ret = e.peek + k.peek;
    e.sit = e.ret + k.back;
    e.unm = e.sit + ESC_SIT;
    e.end = e.unm + k.morph;
  }
  e.active = true;
  e.kind = kind;
  e.t0 = tt;
  e.dir = rng.randint(0, 1) ? 1 : -1;
  e.back = rng.randint(0, 1) ? 1 : -1;
  e.peekX = rng.randint(30, 98);
  e.sleepDur = kind == K_LOAF ? rng.uniform(sleepMin, sleepMax) : 0.0;
  e.woke = false;
  esc = e;
  catDir = esc.dir;
  resetPose();
  nDust = 0; dustAt = 0; nOk = 0;
  if (kind == K_KABUR) lastInput = tt;
  stats[10 + kind]++;
}

void Engine::callEscape(double tt) {
  double e = tt - esc.t0;
  const KindDef &k = KINDS[esc.kind];
  if (esc.kind == K_KABUR && esc.gone + 600 <= e && e < esc.ret) {
    esc.peek = dmin(esc.peek, e);
    esc.ret = e;
    esc.sit = e + k.back;
    esc.unm = esc.sit + ESC_SIT;
    esc.end = esc.unm + k.morph;
  }
}

static void setCatDefaults(Cat &c, bool flip) {
  c = Cat();
  c.mode = CM_NONE;
  c.flip = flip;
  c.pose = gPoses[PS_SIT];
}

void Engine::escapeStep(double tt, Cat &cat, bool &hasCat, int &fp, const char *&la, const char *&lb) {
  const KindDef &k = KINDS[esc.kind];
  double e = tt - esc.t0, dt = STEP_MS / 1000;
  int d = esc.dir;
  hasCat = false;
  fp = PR_NORMAL;
  la = k.name;
  if (e >= esc.end) {  // sudah jadi wajah lagi
    bool happy = k.happy;
    Kind kd = esc.kind;
    esc.active = false;
    nDust = 0;
    morph = 0;
    if (kd == K_KABUR || kd == K_LOAF) lastInput = tt;
    if (kd == K_LOAF) startEvent(EV_YAWN, tt);
    if (happy) { happyUntil = tt + HAPPY_MS; heartAt = tt; sp[P_FY].v -= 90; }
    fp = happy ? PR_SENANG : PR_NORMAL;
    la = "pulang"; lb = nullptr;
    return;
  }
  if (k.script) { activityStep(tt, e, cat, hasCat, fp, la, lb); return; }
  { // buang debu lama
    int n = 0;
    for (int i = 0; i < nDust; i++)
      if (tt - dust[i][2] < 450) { if (n != i) memcpy(dust[n], dust[i], sizeof(dust[0])); n++; }
    nDust = n;
  }
  setCatDefaults(cat, d < 0);
  if (e < esc.poof) {
    morph = 0;
    fp = k.prePreset; lb = k.l1;
    return;
  }
  if (e < esc.run) {  // wajah mengecil jadi kepala, badan tumbuh
    double u = (e - esc.poof) / k.morph;
    morph = (float)u;
    if (u > 0.85 && nDust == 0)
      for (int j = 0; j < 4; j++) puff(tt, 55 + j * 7 + rng.uniform(-2, 2), 52 - rng.randint(0, 4));
    cat.mode = CM_MORPH; cat.u = (float)u; hasCat = true;
    fp = k.prePreset; lb = "berubah jadi kucing";
    return;
  }
  morph = 1;
  hasCat = true;
  if (e < esc.gone) {  // lari makin cepat
    double tau = (e - esc.run) / 1000;
    double x = 64 + d * 70 * tau * tau;
    if (tt - dustAt >= 90) { dustAt = tt; puff(tt, x - d * 16 + rng.uniform(-2, 2), 52 - rng.randint(0, 4)); }
    catPh += (float)(dmin(31.0, 10 + 21 * tau) * dt);
    cat.mode = CM_SPRITE; cat.x = (float)x; cat.ph = catPh; cat.ground = true; cat.scroll = -(int)x; cat.pose = gPoses[PS_RUN];
    lb = "lari";
    return;
  }
  if (e < esc.peek) {
    cat.mode = CM_DOTS; cat.rise = (float)((e - esc.gone) / 1000);
    lb = "sepi";
    return;
  }
  if (e < esc.ret) {
    cat.mode = CM_PEEK; cat.rise = (float)dmin(1.0, (e - esc.peek) / 500); cat.x = (float)esc.peekX;
    cat.look = (float)sin((e - esc.peek) / 450);
    lb = "mengintip";
    return;
  }
  int bd = esc.back;
  bool run = k.backRun;
  if (e < esc.sit) {  // kembali, melambat sampai berhenti
    double u = (e - esc.ret) / k.back;
    double x0 = bd > 0 ? -20 : 148;
    double x = x0 + (64 - x0) * (1 - (1 - u) * (1 - u));
    double vel = fabs(64 - x0) * 2 * (1 - u) / (k.back / 1000.0);
    if (tt - dustAt >= 90 && run) { dustAt = tt; puff(tt, x - bd * 16 + rng.uniform(-2, 2), 52 - rng.randint(0, 4)); }
    catPh += (float)(dmin(run ? 31.0 : 14.0, 4 + vel * (run ? 0.12 : 0.08)) * dt);
    cat.mode = CM_SPRITE; cat.x = (float)x; cat.ph = catPh; cat.flip = bd < 0; cat.ground = true; cat.scroll = -(int)x;
    cat.pose = gPoses[run ? PS_RUN : PS_WALK];
    lb = "pulang";
    return;
  }
  catPh += (float)(3 * dt);
  if (e < esc.unm) {  // duduk
    cat.mode = CM_SPRITE; cat.x = 64; cat.ph = catPh; cat.flip = bd < 0; cat.pose = gPoses[PS_SIT];
    fp = PR_SENANG; lb = "duduk";
    return;
  }
  double u = 1 - (e - esc.unm) / k.morph;
  morph = (float)u;
  cat.mode = CM_MORPH; cat.u = (float)u; cat.flip = bd < 0;
  fp = PR_SENANG; lb = "jadi wajah lagi";
}

void Engine::peekScene(double s, Cat &cat, const char *&la, const char *&lb) {
  static const char *const NAMES[4] = {"mengintip atas", "mengintip kiri", "mengintip kanan", "mengintip bawah"};
  la = "ngumpet";
  for (int i = 0; i < esc.nPeeks; i++) {
    const Peek &pk = esc.peeks[i];
    double u = s - pk.t0;
    if (0 <= u && u < pk.dur) {
      double rise;
      if (u < 450) rise = smooth(0, 1, u / 450);
      else if (u < 450 + pk.hold) rise = 1.0;
      else rise = smooth(0, 1, 1 - (u - 450 - pk.hold) / 450);
      cat.mode = CM_PEEKEDGE; cat.edge = pk.edge; cat.pos = pk.pos; cat.vis = rnd(rise * 22);
      cat.look = (float)sin(u / 260);
      lb = NAMES[pk.edge];
      return;
    }
  }
  lb = "sepi";
}

void Engine::activityStep(double tt, double e, Cat &cat, bool &hasCat, int &fp, const char *&la, const char *&lb) {
  const KindDef &k = KINDS[esc.kind];
  double dt = STEP_MS / 1000;
  int d = esc.dir;
  fp = PR_NORMAL;
  la = k.name;
  { int n = 0;
    for (int i = 0; i < nDust; i++)
      if (tt - dust[i][2] < 450) { if (n != i) memcpy(dust[n], dust[i], sizeof(dust[0])); n++; }
    nDust = n; }
  setCatDefaults(cat, catDir < 0);
  cat.ph = catPh;
  hasCat = true;
  if (esc.kind == K_LOAF && !esc.woke && e >= esc.run &&
      (lastInput > esc.t0 + 300 || e - esc.run >= esc.sleepDur)) {
    esc.woke = true;
    esc.gone = e;
    esc.end = e + k.morph;
  }
  if (e < esc.run) {  // wajah mengecil jadi kepala + badan tumbuh / menghilang mendadak
    morph = (float)(e / k.morph);
    if (k.fade) {
      if (morph > 0.4f && nDust == 0)
        for (int j = 0; j < 5; j++) puff(tt, 50 + j * 7 + rng.uniform(-2, 2), 32 + rng.randint(-8, 8));
      cat.mode = CM_FADE; cat.u = morph;
      lb = "menghilang";
      return;
    }
    cat.mode = CM_MORPH; cat.u = morph;
    lb = "berubah jadi kucing";
    return;
  }
  if (e >= esc.gone) {  // kembali jadi wajah
    morph = (float)(1 - (e - esc.gone) / k.morph);
    if (k.fade) {
      cat.mode = CM_FADE; cat.u = morph;
      fp = PR_SENANG; lb = "muncul lagi";
      return;
    }
    cat.mode = CM_MORPH; cat.u = morph;
    lb = "jadi wajah lagi";
    return;
  }
  morph = 1;
  double s = e - esc.run;
  if (esc.kind == K_NGUMPET) { peekScene(s, cat, la, lb); return; }

  Pose P = gPoses[PS_SIT];
  float x = 64, y = 0;
  bool haveZ = false;
  switch (esc.kind) {
    case K_JALAN: {  // patroli
      double side = 64 + d * 44;
      if (s < 500) { lb = "duduk"; catPh += (float)(3 * dt); }
      else if (s < 2500) {
        double u = (s - 500) / 2000;
        x = (float)(64 + (side - 64) * smooth(0, 1, u)); P = gPoses[PS_WALK]; lb = "berjalan"; catDir = d;
        catPh += (float)((6 + 6 * sin(PI_ * clampd(u, 0, 1))) * dt);
      } else if (s < 4700) {
        x = (float)side; lb = "duduk melihat sekeliling";
        catDir = (((int)((s - 2500) / 800)) % 2 == 0) ? d : -d;
        catPh += (float)(3 * dt);
      } else if (s < 6700) {
        double u = (s - 4700) / 2000;
        x = (float)(side + (64 - side) * smooth(0, 1, u)); P = gPoses[PS_WALK]; lb = "kembali"; catDir = -d;
        catPh += (float)((6 + 6 * sin(PI_ * clampd(u, 0, 1))) * dt);
      } else { lb = "duduk"; catPh += (float)(3 * dt); }
      break;
    }
    case K_EKOR: {
      catPh += (float)(3 * dt);
      lb = "melihat ekor";
      if (s >= 600 && s < 3400) {
        P = gPoses[PS_SPIN]; lb = "berputar";
        catDir = ((int)((s - 600) / 230)) % 2 == 0 ? d : -d;
        catPh += (float)(14 * dt);
        x = (float)(64 + sin((s - 600) / 115) * 2);
        y = (float)(-fabs(sin((s - 600) / 230 * PI_)) * 2);
      } else if (s >= 3400) {
        P = gPoses[PS_DIZZY]; lb = "pusing";
        P.headDy = (float)(3 + sin(s / 90) * 1.5);
      }
      break;
    }
    case K_STRETCH: {
      catPh += (float)(3 * dt);
      catDir = d;
      if (s < 500) { lb = "duduk"; }
      else if (s < 2000) { P = gPoses[PS_STRETCH1]; lb = "menunduk"; }
      else if (s < 3200) { P = gPoses[PS_ARCH]; lb = "punggung melengkung"; }
      else { lb = "duduk"; }
      break;
    }
    case K_GARUK: {
      catPh += (float)(3 * dt);
      catDir = d;
      if (s >= 400 && s < 2900) { P = gPoses[PS_SCRATCH]; P.hasScratch = true; P.scratch = (float)sin(s / 55); lb = "menggaruk telinga"; }
      else lb = "duduk";
      break;
    }
    default: {  // K_LOAF
      catPh += (float)dt;
      catDir = d;
      if (s < 600) { P = gPoses[PS_SIT]; P.eyesClosed = true; }
      else { P = gPoses[PS_LOAF]; P.bob = (float)(sin(s / 480) * 0.9); }
      lb = "tidur melingkar";
      cat.nzs = 2;
      for (int i = 0; i < 2; i++) {
        double ph = fmod(s + (i ? 1300 : 0), 2600.0) / 2600;
        cat.zs[i][0] = (float)(64 + 8 + ph * 10 + i * 3);
        cat.zs[i][1] = (float)(32 - ph * 22);
        cat.zs[i][2] = i == 1 ? 1.0f : 0.0f;
      }
      haveZ = true;
      break;
    }
  }
  (void)haveZ;
  cat.mode = CM_SPRITE; cat.ground = true; cat.x = x; cat.y = y; cat.ph = catPh; cat.flip = catDir < 0;
  cat.pose = easePose(P, dt);
}

// ---------------------------------------------------------------- ajak main
void Engine::playStep(double tt, double dt, Cat &cat, int &fp, const char *&la, const char *&lb) {
  la = "main"; lb = nullptr;
  double u = morph;
  if (u < 1.0) {
    bool going = morphTarget >= 1.0;
    if (!going) catY = 0;
    setCatDefaults(cat, catDir < 0);
    cat.mode = CM_MORPH; cat.u = (float)u; cat.x = catX; cat.y = catY;
    fp = going ? PR_MAIN : PR_SENANG;
    lb = going ? "berubah jadi kucing" : "kembali jadi wajah";
    return;
  }
  if (!plActive) {  // tikus masuk dari sisi layar
    resetPose();
    int side = rng.randint(0, 1) ? 1 : -1;
    pl = Play();
    pl.st = W_WATCH; pl.t0 = tt; pl.mx = (float)(64 + side * 52.0); pl.mv = 0; pl.mdir = -side;
    pl.mtarget = (float)(64 + side * 30.0); pl.mpause = tt + 700; pl.mspeed = 75; pl.pinned = false; pl.hidden = false;
    pl.ph = 0;
    plActive = true;
  }
  // tikus: berlari pendek-pendek lalu membeku
  if (!pl.pinned) {
    if (tt < pl.mpause) pl.mv = 0;
    else {
      double dist = pl.mtarget - pl.mx;
      if (fabs(dist) < 2) {
        pl.mpause = tt + rng.randint(600, 1700);
        double far_ = -1;
        for (int i = 0; i < 4; i++) {
          double x = rng.uniform(14, 114);
          if (far_ < 0 && fabs(x - pl.mx) > 24) far_ = x;
        }
        pl.mtarget = (float)(far_ >= 0 ? far_ : dmin(114.0, dmax(14.0, 128 - pl.mx)));
        pl.mspeed = 75;
        pl.mv = 0;
      } else {
        pl.mdir = dist > 0 ? 1 : -1;
        pl.mv = pl.mspeed * pl.mdir;
        pl.mx = (float)clampd(pl.mx + pl.mv * dt, 10.0, 118.0);
      }
    }
    pl.ph += (float)(fabs(pl.mv) * dt * 0.06);
  }
  double mx = pl.mx, cx = catX, dist = mx - cx;
  int sdir = dist > 0 ? 1 : -1;
  double e = tt - pl.t0;
  int pose = PS_ALERT;
  auto go = [&](uint8_t n) { pl.st = n; pl.t0 = tt; };
  switch (pl.st) {
    case W_WATCH:
      catDir = sdir;
      if (e > 700 && fabs(dist) > 30) go(W_STALK);
      break;
    case W_STALK:
    case W_FREEZE: {
      catDir = sdir;
      uint8_t st2;
      if (fabs(pl.mv) > 5) { pose = PS_FREEZE; st2 = W_FREEZE; }
      else {
        pose = PS_STALK; st2 = W_STALK;
        catX = (float)clampd(cx + sdir * 20 * dt, 20.0, 108.0);
        catPh += (float)(2.2 * dt);
      }
      pl.st = st2;
      if (fabs(mx - catX) <= 30) go(W_WIGGLE);
      break;
    }
    case W_WIGGLE:
      pose = PS_WIGGLE;
      catDir = sdir;
      catPh += (float)(45 * dt);
      if (fabs(dist) > 44) go(W_STALK);
      else if (e > 900) { pl.zx0 = catX; pl.zx1 = (float)(mx - sdir * 5); go(W_POUNCE); }
      break;
    case W_POUNCE: {
      pose = PS_POUNCE;
      double f = dmin(1.0, e / 450);
      catX = (float)(pl.zx0 + (pl.zx1 - pl.zx0) * f);
      catY = (float)(-15 * 4 * f * (1 - f));
      if (f >= 1.0) {
        catY = 0;
        if (fabs(pl.mx - catX) < 14 && rng.random() < 0.8) { pl.pinned = true; go(W_CATCH); }
        else { pl.mspeed = 130; pl.mpause = 0; go(W_MISS); }
      }
      break;
    }
    case W_MISS:
      pose = PS_SIT;
      if (e > 600) go(W_STALK);
      break;
    case W_CATCH:
      pose = PS_CATCH;
      pl.hidden = e > 350;
      if (e > 1300) {
        pl.pinned = false; pl.hidden = false;
        pl.mx = catX + sdir * 6; pl.mdir = sdir;
        pl.mtarget = sdir > 0 ? 114.0f : 14.0f;
        pl.mspeed = 140; pl.mpause = 0;
        go(W_CHASE);
      }
      break;
    case W_CHASE:
      pose = PS_RUN;
      catDir = sdir;
      catX = (float)clampd(cx + sdir * 80 * dt, 20.0, 108.0);
      catPh += (float)(24 * dt);
      if (e > 650 || fabs(dist) < 8) go(W_REST);
      break;
    default:  // W_REST
      pose = PS_SIT;
      pl.mspeed = 75;
      if (e > 800) go(W_WATCH);
      break;
  }
  if (pl.st != W_POUNCE) catY = 0;
  setCatDefaults(cat, catDir < 0);
  cat.mode = CM_PLAY; cat.x = catX; cat.y = catY; cat.ph = catPh; cat.ground = true; cat.scroll = 0;
  cat.pose = easePose(gPoses[pose], dt);
  cat.hasToy = true; cat.toyX = pl.mx; cat.toyDir = pl.mdir; cat.toyPh = pl.ph; cat.toyHidden = pl.hidden;
  fp = PR_NORMAL;
  la = "main"; lb = PLAY_LABELS[pl.st];
}

// ---------------------------------------------------------------- kasih makan
void Engine::feedStep(double tt, int &fp, bool &hasFish, float &fx, float &fy, const char *&la, const char *&lb,
                      bool &hasMo, float &mo) {
  la = "makan"; lb = nullptr;
  hasFish = false; hasMo = false;
  if (feedT0 < 0) { feedT0 = tt; feedGy = 0; chewK = -1; nCrumbs = 0; hearty = false; }
  double e = tt - feedT0;
  double cyc = FEED_FALL + FEED_CHOMP + FEED_GAP;
  int n = (int)floor(e / cyc);
  double c = fmod(e, cyc);
  if (n >= FEED_MAX) {
    feedGy = 0; hearty = true;
    fp = PR_KENYANG; lb = "kenyang";
    return;
  }
  if (c < FEED_FALL) {
    double u = c / FEED_FALL;
    feedGy = (float)(-0.9 + 1.3 * u);
    chewK = -1;
    hasFish = true; fx = 64; fy = (float)(-9 + 50 * u * u);
    hasMo = true; mo = (float)(0.45 * u);
    fp = PR_MAKAN_TUNGGU; lb = "ikan jatuh";
    return;
  }
  feedGy = 0.4f;
  c -= FEED_FALL;
  fp = PR_MAKAN;
  if (c < FEED_CHOMP) {
    int k = (int)floor(c / 160);
    if (k != chewK) {
      chewK = k;
      if (k % 2 == 0)
        for (int i = 0; i < 3 && nCrumbs < MAX_CRUMBS; i++) {
          crumbs[nCrumbs][0] = (float)(64 + rng.uniform(-6, 6)); crumbs[nCrumbs][1] = 46;
          crumbs[nCrumbs][2] = (float)rng.uniform(-35, 35); crumbs[nCrumbs][3] = (float)-rng.uniform(30, 60);
          nCrumbs++;
        }
    }
    hasMo = true; mo = k % 2 == 0 ? 0.7f : 0.1f;
    lb = "nyam nyam";
  }
}

// ---------------------------------------------------------------- langkah utama
void Engine::advance(double now) {
  while (t + STEP_MS <= now) {
    t += STEP_MS;
    step(t);
  }
}

const Face &Engine::frame(double now) {
  advance(now);
  if (!haveF) step(t);
  return F;
}

struct Choice { int type; int id; };  // type 0 = kejadian wajah (Ev), 1 = kegiatan badan (Kind), 2 = main sendiri

static Choice pickKind(Engine &E, Mood mood) {
  Rng &rng = E.rng;
  if (mood == M_BOSAN) {
    if (rng.random() < E.bodyProb * 1.6) {
      static const Kind K[5] = {K_JALAN, K_EKOR, K_STRETCH, K_GARUK, K_NGUMPET};
      return {1, K[rng.randint(0, 4)]};
    }
    static const Ev V[4] = {EV_YAWN, EV_YAWN, EV_GROOM, EV_SLOWBLINK};
    return {0, V[rng.randint(0, 3)]};
  }
  if (E.zoomProb > 0 && rng.random() < E.zoomProb) return {1, K_ZOOMIES};
  if (rng.random() < E.bodyProb) {
    static const int B[7] = {K_JALAN, K_EKOR, K_STRETCH, K_GARUK, -1, K_NGUMPET, K_NGUMPET};
    int b = B[rng.randint(0, 6)];
    return b < 0 ? Choice{2, 0} : Choice{1, b};
  }
  return {0, EVENTS[rng.randint(0, 10)]};
}

static bool evAllowed(Mood mood, Ev ev) {
  if (mood == M_BOSAN) return ev == EV_YAWN || ev == EV_GROOM || ev == EV_SLOWBLINK;
  return ev != EV_NONE;
}

void Engine::step(double tt) {
  const double dt = STEP_MS / 1000;
  if (!esc.active) {  // tahan DOWN (atau main sendiri) = ajak main
    bool dn = held(1, tt) >= HOLD_MS || tt < autoPlayUntil;
    if (dn && !sceneMain) { sceneMain = true; catX = 64; catY = 0; morphTarget = 1; plActive = false; }
    else if (sceneMain && !dn) morphTarget = 0;
    if (sceneMain) {
      double stp = dt * 1000 / MORPH_MS;
      morph += (float)clampd(morphTarget - morph, -stp, stp);
      if (morphTarget == 0 && morph <= 0) {
        sceneMain = false; morph = 0; plActive = false;
        happyUntil = tt + 1200; heartAt = tt;
        if (zoomProb > 0 && rng.random() < 0.4) pendingZoom = tt + 1600;
      }
    }
  }
  Mood mood = moodAt(tt);
  if (mood == M_BOSAN && prevMood != M_BOSAN && lively && rng.random() < kaburProb) {
    startEscape(tt, rng.randint(0, 1) ? K_NGUMPET : K_ZOOMIES);
    mood = M_KABUR;
  }
  if (mood == M_TIDUR && prevMood != M_TIDUR && lively && !anyDown() && rng.random() < loafProb) {
    startEscape(tt, K_LOAF);
    mood = M_KABUR;
  }
  if (mood == M_TIDUR && prevMood != M_TIDUR) { sleepT0 = tt; sleepDur = rng.uniform(sleepMin, sleepMax); }
  if (mood == M_TIDUR && lively && !anyDown() && tt - sleepT0 >= sleepDur) {
    lastInput = tt;
    startEvent(EV_YAWN, tt);
    mood = M_NORMAL;
  }
  if (mood == M_NORMAL && tt >= evUntil) {
    if (pendingGroom > 0 && tt >= pendingGroom) { pendingGroom = 0; startEvent(EV_GROOM, tt); }
    else if (pendingZoom > 0 && tt >= pendingZoom) { pendingZoom = 0; startEscape(tt, K_ZOOMIES); mood = M_KABUR; }
  }
  if (prevMood == M_MAKAN && mood != M_MAKAN) {
    happyUntil = tt + 1000; heartAt = tt; feedT0 = -1; hearty = false;
    pendingGroom = tt + 1500;
  }
  prevMood = mood;
  if (anyDown()) lastInput = tt;

  float tg[NPARAM];
  memcpy(tg, BASE, sizeof(tg));
  int fp = moodPreset(mood);
  Cat cat;
  bool hasCat = false, hasFish = false, hasOverMo = false;
  float fishX = 0, fishY = 0, overMo = 0;
  const char *la = MOOD_NAMES[mood], *lb = nullptr;
  if (mood == M_KABUR) escapeStep(tt, cat, hasCat, fp, la, lb);
  else if (mood == M_MAIN) { hasCat = true; playStep(tt, dt, cat, fp, la, lb); }
  else if (mood == M_MAKAN) feedStep(tt, fp, hasFish, fishX, fishY, la, lb, hasOverMo, overMo);
  applyPreset(tg, fp);
  if (hasOverMo) tg[P_MO] = overMo;
  float gx = 0, gy = 0;

  // kegiatan acak (hanya saat santai, tanpa tombol apa pun)
  if (lively && (mood == M_NORMAL || mood == M_BOSAN) && tt >= nextEv && tt >= evUntil) {
    Choice ch = pickKind(*this, mood);
    if (ch.type == 1) {
      startEscape(tt, (Kind)ch.id);
      mood = prevMood = M_KABUR;
      escapeStep(tt, cat, hasCat, fp, la, lb);
      applyPreset(tg, fp);
      nextEv = tt + rng.randint(9000, 16000);
    } else if (ch.type == 2) {
      autoPlayUntil = tt + rng.randint(9000, 14000);
      stats[ST_MAINSENDIRI]++;
      nextEv = autoPlayUntil + rng.randint(8000, 14000);
    } else {
      startEvent((Ev)ch.id, tt);
      nextEv = evUntil + (mood == M_BOSAN ? rng.randint(4000, 8000) : rng.randint(2500, 6000));
    }
  }
  if (lively && (mood == M_NORMAL || mood == M_BOSAN) && tt >= gzNext) {
    int gi = rng.randint(0, 8);
    gzX = GAZES[gi][0]; gzY = GAZES[gi][1];
    gzNext = tt + rng.randint(700, 2200);
    stats[ST_GAZE]++;
  }
  if (lively && tt >= driftAt) {
    driftX = (float)rng.uniform(-0.25, 0.25); driftY = (float)rng.uniform(-0.2, 0.2);
    driftAt = tt + rng.randint(300, 900);
  }
  if (lively && mood != M_KABUR && mood != M_MAIN && tt >= earAt) {
    sp[rng.randint(0, 1) ? P_EARL : P_EARR].v += (float)rng.uniform(5, 8);
    earAt = tt + rng.randint(2500, 8000);
  }

  Ev ev = tt < evUntil ? evKind : EV_NONE;
  if (mood == M_BOSAN && ev == EV_NONE && evKind == EV_YAWN && yawnRolled != evStart && tt >= evUntil) {
    yawnRolled = evStart;
    if (lively && rng.random() < 0.3) lastInput = tt - sleepAfter;
  }
  double eT = tt - evStart;
  int sgn = evSide;
  bool hasPaw = false, hasBird = false, jaw = false;
  double pawX = 0, pawY = 0, birdX = 0, birdY = 0;
  int birdFlap = 0;
  double bL = 0, bR = 0;
  if (ev != EV_NONE && (mood == M_NORMAL || mood == M_BOSAN) && evAllowed(mood, ev)) {
    switch (ev) {
      case EV_YAWN: applyPreset(tg, PR_YAWN); la = "menguap"; lb = nullptr; break;
      case EV_BLEP: tg[P_TONGUE] = 1; la = "melet"; lb = nullptr; break;
      case EV_SNIFF: tg[P_AMP] = 1.8f; tg[P_WH] = 0.4f; la = "mengendus"; lb = nullptr; break;
      case EV_KEPO:
        tg[P_H] = 25; tg[P_W] = 24; tg[P_R] = 12; tg[P_PW] = 10; tg[P_PH] = 14; tg[P_WLEN] = 0.85f;
        tg[P_HTILT] = 0.5f * sgn; la = "kepo"; lb = nullptr; break;
      case EV_TILT:
        tg[P_HTILT] = (float)sgn; tg[P_H] = 26; tg[P_EARL] = sgn < 0 ? 0.0f : 0.3f; tg[P_EARR] = sgn > 0 ? 0.0f : 0.3f;
        la = "kepala miring"; lb = nullptr; break;
      case EV_SLOWBLINK:
        tg[P_MC] = 4.5f; tg[P_EARL] = 0.25f; tg[P_EARR] = 0.25f;
        bL = bR = slowBlink(eT);
        la = "kedip lambat"; lb = nullptr; break;
      case EV_GROOM: {
        applyPreset(tg, PR_GROOM);
        bool lick;
        hasPaw = groomPaw(eT, pawX, pawY, lick);
        if (hasPaw && lick) tg[P_TONGUE] = 1;
        la = "mencuci muka"; lb = nullptr;
        break;
      }
      case EV_BIRD: {
        double f = eT / EVENT_MS[EV_BIRD];
        birdX = -10 + 148 * (evDir > 0 ? f : 1 - f);
        birdY = 5 + sin(eT / 220) * 2.5;
        birdFlap = ((int)(eT / 90)) % 2;
        hasBird = true;
        applyPreset(tg, PR_BIRD);
        jaw = 500 < eT && eT < 3000 && ((int)(eT / 90)) % 2 == 0;
        la = "melihat burung"; lb = nullptr;
        break;
      }
      default: break;
    }
  }
  if ((mood == M_NORMAL || mood == M_BOSAN) && ev != EV_YAWN && ev != EV_GROOM) {
    if (lively) { gx = gzX + driftX; gy = gzY + driftY; }
    if (mood == M_BOSAN) gy = (float)(dmax(gy, 0) * 0.5);
    if (ev == EV_KEPO) { gx = gzX != 0 ? gzX : 1; gy = -0.6f; }
    if (ev == EV_BIRD && hasBird) { gx = (float)clampd((birdX - 64) / 48, -1.0, 1.0); gy = -0.85f; }
  }
  if (tt < lookUntil && (mood == M_NORMAL || mood == M_BOSAN)) {
    gx = (float)lookGx; gy = 0;
    tg[P_HTILT] = 0.7f * lookGx;
    la = lookGx < 0 ? "lirik kiri" : "lirik kanan"; lb = nullptr;
  }
  if (mood == M_MAKAN) { gx = 0; gy = feedGy; }

  // kedip
  if ((mood == M_NORMAL || mood == M_BOSAN) && ev != EV_YAWN && ev != EV_GROOM && ev != EV_SLOWBLINK && ev != EV_BIRD &&
      lively) {
    double dtb = tt - blinkAt, hold = mood == M_NORMAL ? 0 : 90;
    if (dtb >= 0) {
      bL = bR = blinkValue(dtb, hold);
      if (dtb > 180 + hold) {
        stats[ST_BLINK]++;
        if (!blink2 && rng.random() < 0.25) { blink2 = true; blinkAt = tt + 60; }
        else { blink2 = false; blinkAt = tt + rng.randint(2500, 5500); }
      }
    }
  }
  if (ev == EV_WINK && mood == M_NORMAL) {
    double v = blinkValue(tt - evStart, 140);
    if (evSide < 0) bL = dmax(bL, v); else bR = dmax(bR, v);
    la = "kedip sebelah"; lb = nullptr;
  }

  // kumis
  if (tt >= twitchAt) {
    twitchSide = rng.randint(0, 1) ? 2 : 1;
    twitchUntil = tt + 150;
    twitchAt = tt + rng.randint(4000, 8000);
  }
  uint8_t twitch = tt < twitchUntil ? (uint8_t)twitchSide : 0;
  if (ev == EV_SNIFF) twitch = ((int)(tt / 110)) % 2 ? 3 : 0;
  if (mood == M_TIDUR && lively) {
    if (tt >= dreamAt) {
      dreamUntil = tt + 350;
      dreamAt = tt + rng.randint(3000, 8000);
      sp[rng.randint(0, 1) ? P_EARL : P_EARR].v += 6;
    }
    if (tt < dreamUntil) twitch = ((int)(tt / 60)) % 2 ? 3 : 0;
  }

  // hati, remah, getar
  double shakeX = 0, shakeY = 0;
  if ((mood == M_SENANG || mood == M_PURR || hearty) && tt >= heartAt) {
    heartAt = tt + (mood == M_PURR ? 260 : 340);
    static const int HX[6] = {10, 20, 30, 92, 102, 112};
    if (nHearts < MAX_HEARTS) {
      hearts[nHearts][0] = (float)(HX[rng.randint(0, 5)] + rng.random() * 6);
      hearts[nHearts][1] = 36; hearts[nHearts][2] = (float)tt;
      nHearts++;
    }
  }
  { int n = 0;
    for (int i = 0; i < nHearts; i++) if (hearts[i][1] > -6) { if (n != i) memcpy(hearts[n], hearts[i], sizeof(hearts[0])); n++; }
    nHearts = n; }
  for (int i = 0; i < nHearts; i++) {
    hearts[i][1] -= (float)(20 * dt);
    hearts[i][0] += (float)(sin((tt - hearts[i][2]) / 260) * 0.35);
  }
  for (int i = 0; i < nCrumbs; i++) {
    crumbs[i][3] += (float)(160 * dt);
    crumbs[i][0] += (float)(crumbs[i][2] * dt);
    crumbs[i][1] += (float)(crumbs[i][3] * dt);
  }
  { int n = 0;
    for (int i = 0; i < nCrumbs; i++) if (crumbs[i][1] < 64) { if (n != i) memcpy(crumbs[n], crumbs[i], sizeof(crumbs[0])); n++; }
    nCrumbs = n; }
  bool hasKnead = false;
  double kneadL = 0, kneadR = 0;
  if (mood == M_KABUR && fp == PR_KESAL) shakeX = sin(tt / 20) * 1.3;
  else if (mood == M_GELI) shakeX = sin(tt / 22) * 1.6;
  else if (mood == M_PURR) {
    shakeX = sin(tt / 36) * 0.6 + sin(tt / 520) * 1.0;
    double a = sin(tt / 130);
    hasKnead = true; kneadL = 59 - 4 * dmax(0.0, a); kneadR = 59 - 4 * dmax(0.0, -a);
  }

  for (int i = 0; i < NPARAM; i++) {
    Spring &s = sp[i];
    s.t = i == P_GX ? gx : i == P_GY ? gy : i == P_FY ? 0.0f : tg[i];
    s.v += (s.k * (s.t - s.x) - s.c * s.v) * (float)dt;
    s.x += s.v * (float)dt;
  }

  double breath = sin(tt / (3000 / (2 * PI_))) * (mood == M_TIDUR ? 1.0 : 0.8);
  moodNow = mood;
  labelA = la; labelB = lb;

  // ---- susun Face
  Face &f = F;
  f.t = tt;
  for (int i = 0; i < NPARAM; i++) f.p[i] = sp[i].x;
  f.bL = (float)bL; f.bR = (float)bR; f.twitch = twitch;
  f.shakeX = (float)shakeX; f.shakeY = (float)shakeY; f.breath = (float)breath;
  f.nose = (ev == EV_SNIFF && ((int)(tt / 80)) % 2) ? 1 : 0;
  f.geli = mood == M_GELI; f.exclaim = mood == M_TERKEJUT; f.jaw = jaw;
  f.nHearts = nHearts;
  for (int i = 0; i < nHearts; i++) { f.hearts[i][0] = hearts[i][0]; f.hearts[i][1] = hearts[i][1]; }
  f.hasFish = hasFish; f.fishX = fishX; f.fishY = fishY;
  f.nCrumbs = nCrumbs;
  for (int i = 0; i < nCrumbs; i++) { f.crumbs[i][0] = crumbs[i][0]; f.crumbs[i][1] = crumbs[i][1]; }
  f.hasPaw = hasPaw; f.pawX = (float)pawX; f.pawY = (float)pawY;
  f.hasKnead = hasKnead; f.kneadL = (float)kneadL; f.kneadR = (float)kneadR;
  f.hasBird = hasBird; f.birdX = (float)birdX; f.birdY = (float)birdY; f.birdFlap = birdFlap; f.birdDir = evDir;
  f.nzs = 0;
  if (mood == M_TIDUR) {
    f.nzs = 2;
    for (int i = 0; i < 2; i++) {
      double ph = fmod(tt + (i ? 1200 : 0), 2400.0) / 2400;
      f.zs[i][0] = (float)(102 + ph * 8 + i * 4); f.zs[i][1] = (float)(20 - ph * 18); f.zs[i][2] = i == 1 ? 1.0f : 0.0f;
    }
  }
  f.hasCat = hasCat;
  if (hasCat) f.cat = cat;
  f.nDust = nDust;
  for (int i = 0; i < nDust; i++) { f.dust[i][0] = dust[i][0]; f.dust[i][1] = dust[i][1]; f.dust[i][2] = (float)(tt - dust[i][2]); }
  haveF = true;
}

}  // namespace pet
