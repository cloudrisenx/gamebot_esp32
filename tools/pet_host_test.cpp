// Tes mesin Pet di PC (bukan bagian firmware). Dibangun dengan:
//   python -m ziglang c++ -std=c++17 -O2 -ffp-contract=off -Isrc tools/pet_host_test.cpp src/pet_core.cpp -o tools/pet_host_test.exe
// Mode:
//   pet_host_test parity in.txt out.txt   gambar tiap "keadaan wajah" dari in.txt -> out.txt (hex), dibandingkan
//                                         dengan render Python oleh tools/pet_parity.py
//   pet_host_test behave                  tes perilaku (seperti selftest simulasi Python)
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "pet_core.h"

using namespace pet;

static uint8_t gBuf[W * H];

static void dumpHex(FILE *f, const Fb &fb) {
  // 1 bit per piksel, baris demi baris, 128 bit = 32 hex per baris
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x += 4) {
      int v = 0;
      for (int b = 0; b < 4; b++) v = v * 2 + (fb.px[y * W + x + b] ? 1 : 0);
      fputc("0123456789abcdef"[v], f);
    }
  }
  fputc('\n', f);
}

struct Tok {
  FILE *f;
  double next() {
    double v = 0;
    if (fscanf(f, "%lf", &v) != 1) { fprintf(stderr, "format salah\n"); exit(2); }
    return v;
  }
  int ni() { return (int)llround(next()); }
};

static void readPose(Tok &k, Pose &P) {
  P.sitting = k.ni(); P.hasGait = k.ni(); P.hasLegs = k.ni(); P.wiggle = k.ni(); P.eyesClosed = k.ni(); P.mouth = k.ni();
  P.prey = k.ni(); P.noLegs = k.ni(); P.pupil2 = k.ni(); P.hasScratch = k.ni();
  P.crouch = k.next(); P.headDy = k.next(); P.ear = k.next(); P.tail = k.next(); P.tilt = k.next(); P.arch = k.next();
  P.bob = k.next(); P.gaitAmp = k.next(); P.gaitLift = k.next();
  for (int i = 0; i < 4; i++) { P.legs[i][0] = k.next(); P.legs[i][1] = k.next(); }
  P.wagAmp = k.next(); P.wagSpd = k.next(); P.scratch = k.next();
}

static bool readFace(Tok &k, Face &F) {
  double t;
  if (fscanf(k.f, "%lf", &t) != 1) return false;
  F = Face();
  F.t = t;
  for (int i = 0; i < NPARAM; i++) F.p[i] = k.next();
  F.bL = k.next(); F.bR = k.next(); F.twitch = k.ni(); F.shakeX = k.next(); F.shakeY = k.next(); F.breath = k.next();
  F.nose = k.ni(); F.geli = k.ni(); F.exclaim = k.ni(); F.jaw = k.ni();
  F.nHearts = k.ni();
  for (int i = 0; i < F.nHearts; i++) { F.hearts[i][0] = k.next(); F.hearts[i][1] = k.next(); }
  F.hasFish = k.ni();
  if (F.hasFish) { F.fishX = k.next(); F.fishY = k.next(); }
  F.nCrumbs = k.ni();
  for (int i = 0; i < F.nCrumbs; i++) { F.crumbs[i][0] = k.next(); F.crumbs[i][1] = k.next(); }
  F.hasPaw = k.ni();
  if (F.hasPaw) { F.pawX = k.next(); F.pawY = k.next(); }
  F.hasKnead = k.ni();
  if (F.hasKnead) { F.kneadL = k.next(); F.kneadR = k.next(); }
  F.hasBird = k.ni();
  if (F.hasBird) { F.birdX = k.next(); F.birdY = k.next(); F.birdFlap = k.ni(); F.birdDir = k.ni(); }
  F.nzs = k.ni();
  for (int i = 0; i < F.nzs; i++) for (int j = 0; j < 3; j++) F.zs[i][j] = k.next();
  F.nDust = k.ni();
  for (int i = 0; i < F.nDust; i++) for (int j = 0; j < 3; j++) F.dust[i][j] = k.next();
  F.hasCat = k.ni();
  if (F.hasCat) {
    Cat &c = F.cat;
    c.mode = (CatMode)k.ni(); c.flip = k.ni(); c.ground = k.ni();
    c.x = k.next(); c.y = k.next(); c.ph = k.next(); c.u = k.next(); c.rise = k.next(); c.look = k.next();
    c.scroll = k.ni();
    c.nzs = k.ni();
    for (int i = 0; i < c.nzs; i++) for (int j = 0; j < 3; j++) c.zs[i][j] = k.next();
    c.hasToy = k.ni();
    if (c.hasToy) { c.toyX = k.next(); c.toyPh = k.next(); c.toyDir = k.ni(); c.toyHidden = k.ni(); }
    c.edge = (Edge)k.ni(); c.vis = k.ni(); c.pos = k.ni();
    readPose(k, c.pose);
  }
  return true;
}

static int parity(const char *in, const char *out) {
  FILE *fi = fopen(in, "r"), *fo = fopen(out, "w");
  if (!fi || !fo) { fprintf(stderr, "tidak bisa buka berkas\n"); return 2; }
  Tok k{fi};
  Fb fb(W, H, gBuf);
  Face F;
  int n = 0;
  while (readFace(k, F)) {
    drawFace(fb, F);
    dumpHex(fo, fb);
    n++;
  }
  fclose(fi);
  fclose(fo);
  printf("%d frame digambar\n", n);
  return 0;
}

// ------------------------------------------------------------------ tes perilaku
static int fails = 0;
#define CHECK(cond, ...) do { if (!(cond)) { fails++; printf("GAGAL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static std::string lab(const Engine &e) {
  std::string s = e.labelA ? e.labelA : "";
  if (e.labelB) { s += ": "; s += e.labelB; }
  return s;
}

static bool hasLabel(Engine &e, double &t, const char *want, double limit) {
  double end = t + limit;
  while (t < end) {
    t += 33.3;
    e.frame(t);
    if (lab(e) == want) return true;
  }
  return false;
}

static Engine *mk(uint32_t seed, bool lively = true) {
  Engine *e = new Engine();
  e->begin(seed, 0);
  e->lively = lively;
  e->zoomProb = 0;
  return e;
}

static unsigned lit(const Fb &fb) { unsigned n = 0; for (int i = 0; i < fb.w * fb.h; i++) n += fb.px[i] ? 1 : 0; return n; }

static int behave() {
  Fb fb(W, H, gBuf);
  // --- wajah diam tergambar & telinga ada
  {
    Engine *e = mk(1, false);
    double t = 0;
    for (int i = 0; i < 180; i++) e->frame(t += 16.7);
    drawFace(fb, e->frame(t));
    CHECK(lit(fb) > 300, "wajah kosong");
    unsigned ears = 0;
    for (int y = 0; y < 10; y++) for (int x = 4; x < 26; x++) ears += fb.px[y * W + x];
    CHECK(ears > 20, "telinga tidak tergambar");
    delete e;
  }
  // --- interaksi: ketuk UP = lirik + miring, OK = senang, OK 3x = geli, notifikasi = terkejut
  {
    Engine *e = mk(2, false);
    e->press(0, 0); e->release(80, 0);
    const Face &F = e->frame(500);
    CHECK(F.p[P_GX] < -0.5f && F.p[P_HTILT] < -0.3f && lab(*e) == "lirik kiri", "lirik %f %f %s", F.p[P_GX], F.p[P_HTILT], lab(*e).c_str());
    e->press(5000, 2); e->release(5060, 2);
    e->frame(5100);
    CHECK(e->mood() == M_SENANG, "OK tidak senang");
    Engine *g = mk(3, false);
    for (int i = 0; i < 3; i++) { g->press(i * 200, 2); g->release(i * 200 + 60, 2); }
    g->frame(600);
    CHECK(g->mood() == M_GELI, "OK 3x tidak geli");
    g->notify(5000); g->frame(5100);
    CHECK(g->mood() == M_TERKEJUT, "notifikasi tidak terkejut");
    g->sleepNow(9000); g->frame(9001);
    CHECK(g->mood() == M_TIDUR, "sleepNow");
    g->press(9100, 0); g->frame(9200);
    CHECK(g->mood() == M_NORMAL, "tombol tidak membangunkan");
    delete e; delete g;
  }
  // --- tahan OK: dielus + kneading, ketiduran, bangun menguap
  {
    Engine *e = mk(4, false);
    e->press(0, 2);
    const Face &F = e->frame(1000);
    CHECK(e->mood() == M_PURR && F.hasKnead, "purr/kneading");
    e->frame(4000);
    CHECK(e->mood() == M_TIDUR, "ketiduran saat dielus");
    e->release(4100, 2); e->frame(4200);
    CHECK(e->mood() != M_TIDUR && lab(*e) == "menguap", "bangun menguap: %s", lab(*e).c_str());
    delete e;
  }
  // --- kegiatan sendiri muncul semua, gambar terus berubah (4 menit simulasi + 15 menit untuk kegiatan badan)
  {
    Engine *e = mk(5, true);
    e->boredAfter = e->sleepAfter = 1e9;
    double t = 0;
    for (long i = 0; i < 15 * 60 * 30; i++) { e->frame(t += 33.3); }
    const uint32_t *s = e->stats;
    CHECK(s[EV_WINK] && s[EV_BLEP] && s[EV_SNIFF] && s[EV_KEPO] && s[EV_SLOWBLINK] && s[EV_TILT] && s[EV_GROOM] && s[EV_BIRD],
          "kejadian wajah kurang: %u %u %u %u %u %u %u %u", s[1], s[2], s[3], s[4], s[6], s[7], s[8], s[9]);
    CHECK(s[10 + K_JALAN] && s[10 + K_EKOR] && s[10 + K_STRETCH] && s[10 + K_GARUK] && s[10 + K_NGUMPET] && s[22],
          "kegiatan badan kurang: jalan %u ekor %u stretch %u garuk %u ngumpet %u main %u", s[12], s[13], s[14], s[15], s[17], s[22]);
    CHECK(s[20] && s[21], "gaze/kedip");
    delete e;
  }
  // --- bosan lalu tidur; tidur ada batasnya; bangun menguap
  {
    Engine *e = mk(6, true);
    e->boredAfter = 1000; e->sleepAfter = 8000; e->kaburProb = 0; e->bodyProb = 0; e->loafProb = 0;
    e->sleepMin = e->sleepMax = 5000;
    double t = 0; bool slept = false, woke = false, yawn = false;
    for (long i = 0; i < 60 * 30; i++) {
      e->frame(t += 33.3);
      if (e->mood() == M_TIDUR) slept = true;
      if (slept && e->mood() != M_TIDUR) woke = true;
      if (woke && lab(*e) == "menguap") yawn = true;
    }
    CHECK(slept && woke && yawn, "tidur-bangun: %d %d %d", slept, woke, yawn);
    delete e;
  }
  // --- kegiatan berskrip: tiap tahap muncul, tergambar, kembali jadi wajah normal
  {
    struct { Kind k; const char *want[4]; } T[] = {
        {K_JALAN, {"jalan: berjalan", "jalan: duduk melihat sekeliling", "jalan: kembali", nullptr}},
        {K_EKOR, {"ekor: melihat ekor", "ekor: berputar", "ekor: pusing", nullptr}},
        {K_STRETCH, {"stretch: menunduk", "stretch: punggung melengkung", nullptr, nullptr}},
        {K_GARUK, {"garuk: menggaruk telinga", nullptr, nullptr, nullptr}}};
    for (auto &c : T) {
      Engine *e = mk(7, false);
      e->startEscape(0, c.k);
      double end = e->esc.end, t = 0;
      bool seen[4] = {false, false, false, false};
      unsigned minLit = 99999;
      while (t < end + 500) {
        t += 33.3;
        const Face &F = e->frame(t);
        drawFace(fb, F);
        std::string l = lab(*e);
        for (int i = 0; i < 4; i++) if (c.want[i] && l == c.want[i]) seen[i] = true;
        if (F.hasCat && F.cat.mode == CM_SPRITE) minLit = lit(fb) < minLit ? lit(fb) : minLit;
      }
      for (int i = 0; i < 4; i++) CHECK(!c.want[i] || seen[i], "kegiatan %d: tahap '%s' tidak muncul", (int)c.k, c.want[i]);
      CHECK(minLit > 80, "sprite tidak tergambar (%u)", minLit);
      CHECK(!e->esc.active && e->mood() == M_NORMAL, "tidak kembali normal");
      delete e;
    }
  }
  // --- tidur melingkar: z melayang, bangun saat tombol / otomatis paling lama sleepMax
  {
    Engine *e = mk(8, false);
    e->startEscape(0, K_LOAF);
    double t = 0; bool zs = false;
    while (t < 5000) { const Face &F = e->frame(t += 33.3); drawFace(fb, F); zs |= F.hasCat && F.cat.nzs > 0; }
    CHECK(lab(*e) == "loaf: tidur melingkar" && zs, "loaf %s", lab(*e).c_str());
    e->press(5100, 0);
    t = 5100;
    while (e->esc.active && t < 9000) e->frame(t += 33.3);
    CHECK(!e->esc.active, "tidak bangun saat tombol");
    Engine *g = mk(9, false);
    g->sleepMin = g->sleepMax = 4000;
    g->startEscape(0, K_LOAF);
    t = 0;
    while (g->esc.active && t < 20000) g->frame(t += 33.3);
    CHECK(!g->esc.active && t > 4000 && t < 9000, "loaf tidak bangun sendiri (t=%.0f)", t);
    delete e; delete g;
  }
  // --- kasih makan: ikan, kunyah, kenyang, lalu mencuci muka
  {
    Engine *e = mk(10, false);
    e->press(0, 0);
    double t = 0; bool fish = false, nyam = false, full = false, crumbs = false;
    while (t < 8000) {
      const Face &F = e->frame(t += 33.3);
      drawFace(fb, F);
      std::string l = lab(*e);
      fish |= l == "makan: ikan jatuh" && F.hasFish; nyam |= l == "makan: nyam nyam"; full |= l == "makan: kenyang";
      crumbs |= F.nCrumbs > 0;
    }
    CHECK(fish && nyam && full && crumbs, "makan %d %d %d %d", fish, nyam, full, crumbs);
    e->release(8100, 0); e->frame(8300);
    CHECK(e->mood() == M_SENANG, "setelah makan");
    CHECK(hasLabel(*e, t, "mencuci muka", 6000), "tidak mencuci muka");
    delete e;
  }
  // --- ajak main: morph bertahap, kejar tikus: mengendap, goyang pantat, terkam, menangkap, mengejar; lepas = kembali wajah
  {
    Engine *e = mk(11, false);
    e->press(0, 1);
    double t = 0, prev = 0; int nm = 0; bool jump = false;
    while (t < 1500) {
      const Face &F = e->frame(t += 33.3);
      drawFace(fb, F);
      if (F.hasCat && F.cat.mode == CM_MORPH) { if (F.cat.u < prev || F.cat.u - prev > 0.1f) jump = true; prev = F.cat.u; nm++; }
    }
    CHECK(nm > 15 && !jump && prev > 0.95, "morph %d %d %f", nm, jump, prev);
    const char *need[] = {"main: mengamati", "main: mengendap", "main: goyang pantat", "main: terkam!", "main: menangkap", "main: mengejar"};
    bool got[6] = {false}; float minY = 0; int ndiff = 0; float lastX = -1;
    while (t < 90000) {
      const Face &F = e->frame(t += 33.3);
      std::string l = lab(*e);
      for (int i = 0; i < 6; i++) if (l == need[i]) got[i] = true;
      if (F.hasCat && F.cat.mode == CM_PLAY) {
        if (F.cat.y < minY) minY = F.cat.y;
        if ((int)F.cat.x != (int)lastX) ndiff++;
        lastX = F.cat.x;
        CHECK(F.cat.x >= 18 && F.cat.x <= 110 && F.cat.toyX >= 10 && F.cat.toyX <= 118, "posisi di luar batas");
      }
      bool all = true; for (int i = 0; i < 6; i++) all = all && got[i];
      if (all && ndiff > 12) break;
    }
    for (int i = 0; i < 6; i++) CHECK(got[i], "tahap main tidak muncul: %s", need[i]);
    CHECK(minY < -8, "kucing tidak melompat");
    e->release(t, 1);
    double u0 = -1, u1 = 2; double t2 = t;
    while (t2 < t + 1400) {
      const Face &F = e->frame(t2 += 33.3);
      if (F.hasCat && F.cat.mode == CM_MORPH) { if (u0 < 0) u0 = F.cat.u; u1 = F.cat.u; }
    }
    CHECK(u0 > 0.9 && u1 < 0.2 && !e->sceneMain, "kembali jadi wajah %f %f", u0, u1);
    delete e;
  }
  // --- kabur (digelitik 6x), dipanggil pulang, bosan -> pergi main (bukan kabur)
  {
    Engine *e = mk(12, false);
    e->startEscape(0, K_KABUR);
    const char *need[] = {"kabur: kesal", "kabur: berubah jadi kucing", "kabur: lari", "kabur: sepi", "kabur: mengintip",
                          "kabur: pulang", "kabur: duduk", "kabur: jadi wajah lagi"};
    bool got[8] = {false}; double t = 0, end0 = e->esc.end; bool happy = false;
    while (t < end0 + 3000) {
      e->frame(t += 33.3);
      std::string l = lab(*e);
      for (int i = 0; i < 8; i++) if (l == need[i]) got[i] = true;
      if (!e->esc.active && e->mood() == M_SENANG) happy = true;
    }
    for (int i = 0; i < 8; i++) CHECK(got[i], "kabur tahap hilang: %s", need[i]);
    CHECK(happy, "tidak senang setelah pulang");
    Engine *g = mk(13, false);
    g->startEscape(0, K_KABUR);
    g->frame(g->esc.gone + 1500);
    g->press(g->esc.gone + 1500, 2); g->release(g->esc.gone + 1560, 2);
    CHECK(g->esc.end < end0, "dipanggil tapi tidak lebih cepat pulang");
    Engine *h = mk(14, false);
    for (int i = 0; i < 6; i++) { h->press(i * 150, 2); h->release(i * 150 + 50, 2); }
    h->frame(1000);
    CHECK(h->mood() == M_KABUR, "digelitik 6x tidak kabur");
    Engine *b = mk(15, true);
    b->boredAfter = 1000; b->sleepAfter = 1e9; b->kaburProb = 1.0; b->bodyProb = 0;
    t = 0;
    for (int i = 0; i < 90; i++) b->frame(t += 33.3);
    CHECK(b->stats[10 + K_NGUMPET] + b->stats[10 + K_ZOOMIES] == 1 && b->stats[10 + K_KABUR] == 0, "bosan -> pergi main");
    delete e; delete g; delete h; delete b;
  }
  // --- ngumpet: menghilang, mengintip dari >= 3 sisi, muncul lagi senang
  {
    Engine *e = mk(16, false);
    e->startEscape(0, K_NGUMPET);
    double t = 0, end0 = e->esc.end; bool vanish = false, back = false, quiet = false, happy = false; int edges = 0;
    int edgeSeen[4] = {0};
    while (t < end0 + 500) {
      const Face &F = e->frame(t += 33.3);
      drawFace(fb, F);
      std::string l = lab(*e);
      vanish |= l == "ngumpet: menghilang"; back |= l == "ngumpet: muncul lagi"; quiet |= l == "ngumpet: sepi";
      if (F.hasCat && F.cat.mode == CM_PEEKEDGE && F.cat.vis >= 15) { edgeSeen[F.cat.edge] = 1; CHECK(lit(fb) > 150, "intipan kosong"); }
      if (!e->esc.active && e->mood() == M_SENANG) happy = true;
    }
    for (int i = 0; i < 4; i++) edges += edgeSeen[i];
    CHECK(vanish && back && quiet && edges >= 3 && happy, "ngumpet %d %d %d %d %d", vanish, back, quiet, edges, happy);
    delete e;
  }
  // --- zoomies
  {
    Engine *e = mk(17, false);
    e->startEscape(0, K_ZOOMIES);
    double t = 0, end0 = e->esc.end; bool lari = false, pulang = false, duduk = false;
    while (t < end0 + 500) { e->frame(t += 33.3); std::string l = lab(*e); lari |= l == "zoomies: lari"; pulang |= l == "zoomies: pulang"; duduk |= l == "zoomies: duduk"; }
    CHECK(lari && pulang && duduk && !e->esc.active, "zoomies");
    delete e;
  }
  printf(fails ? "%d GAGAL\n" : "semua tes perilaku lolos\n", fails);
  return fails ? 1 : 0;
}

static int poses() {  // cetak tabel pose C++ untuk dicocokkan dengan Python (tools/pet_parity.py)
  for (int i = 0; i < NPOSE_PUBLIC; i++) {
    const Pose &P = posePreset(i);
    printf("%d %d %d %d %d %d %d %d %d %g %g %g %g %g %g %g %g %g", P.sitting, P.hasGait, P.hasLegs, P.wiggle, P.eyesClosed,
           P.mouth, P.prey, P.noLegs, P.pupil2, P.crouch, P.headDy, P.ear, P.tail, P.tilt, P.arch, P.bob, P.gaitAmp, P.gaitLift);
    for (int k = 0; k < 4; k++) printf(" %g %g", P.legs[k][0], P.legs[k][1]);
    printf(" %g %g\n", P.wagAmp, P.wagSpd);
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc >= 4 && !strcmp(argv[1], "parity")) return parity(argv[2], argv[3]);
  if (argc >= 2 && !strcmp(argv[1], "behave")) return behave();
  if (argc >= 2 && !strcmp(argv[1], "poses")) return poses();
  fprintf(stderr, "pakai: pet_host_test parity in out | behave\n");
  return 2;
}
