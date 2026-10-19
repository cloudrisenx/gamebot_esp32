#include "game_hitung.h"
#include "lang.h"
#include "power.h"
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include "buttons.h"
#include "settings.h"
#include "screen.h"
#include "games.h"

extern Adafruit_SSD1306 display;

static const int QUESTIONS = 10, PER_LEVEL = 2;
static const int MAX_ANSWER = 30;
static const uint32_t INTRO_MS = 1200;
static const uint32_t ANSWER_MS = 8000;
static const uint32_t ANSWER_2P_MS = 6000;                     // 2P serentak
static const uint32_t LOCK_HOLD_MS = 500;                      // 2P: tahan kedua tombol sendiri = kunci
static const uint32_t REPEAT_DELAY_MS = 400, REPEAT_MS = 150;  // tahan kiri/kanan: ulang otomatis
static const uint32_t BACK_HOLD_MS = 1000;                     // keluar di tengah permainan
static const int SPR = 8, GAP = 2;
static const int AREA_Y0 = 10, AREA_Y1 = 63;                   // area tampil benda (inklusif)
static const int MAX_ITEMS = 32;

// Sprite 8x8: hati, bintang, smiley, kotak, bulat
static const int KINDS = 5;
static const uint8_t PROGMEM SPRITES[KINDS][8] = {
  {0x66, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00},  // hati
  {0x18, 0x18, 0xFF, 0x7E, 0x3C, 0x7E, 0x66, 0x42},  // bintang
  {0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C},  // smiley
  {0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF},  // kotak
  {0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C},  // bulat
};

// Waktu tampil (layar ATUR): Lv k = Lv5 + (5 - k) x 200 ms.
struct TargetDef { uint8_t tMin, tMax; };
static const TargetDef TARGETS[5] = {{3, 5}, {5, 8}, {6, 10}, {8, 12}, {10, 15}};
// Pengecoh per level pengecoh (Tanpa/Sedikit/Sedang/Banyak) x level: jenis x jumlah per jenis
struct DistDef { uint8_t kinds, dMin, dMax; };
static const DistDef DISTS[HITUNG_DIST_OPT_COUNT][5] = {
  {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},  // Tanpa
  {{0, 0, 0}, {1, 1, 2}, {1, 2, 3}, {1, 2, 3}, {2, 2, 3}},  // Sedikit
  {{1, 1, 2}, {1, 2, 3}, {2, 2, 3}, {2, 2, 4}, {2, 3, 4}},  // Sedang
  {{1, 2, 3}, {1, 3, 5}, {2, 2, 4}, {2, 3, 5}, {3, 3, 4}},  // Banyak
};
static const uint16_t SHOW_STEP_MS = 200;

static uint32_t showMs(int lv) { return settings_hitungLv5Ms() + (4 - lv) * SHOW_STEP_MS; }  // lv 0..4
static const uint8_t CPU_EXACT_PCT[5] = {85, 75, 65, 55, 45};

enum HitungMode : uint8_t { MODE_CPU, MODE_2P };
static HitungMode mode = MODE_CPU;  // disimpan di NVS "hitung"/"mode"
static bool modeLoaded = false;

enum Sub : uint8_t { SB_INTRO, SB_SHOW, SB_ANSWER, SB_RESULT };

struct Item { int8_t x; int8_t y; uint8_t kind; };
static Item items[MAX_ITEMS];
static int itemCount;

static GamePhase phase;
static Sub sub;
static uint32_t subAt;
static int question;               // 0..QUESTIONS-1
static uint8_t target;            // jenis benda yang dihitung
static int correct;
static int value;                 // 1P: angka yang sedang diisi
static int vals[2];               // 2P: angka P1 / P2 (diisi serentak)
static bool locked[2];
static uint32_t lockHoldStart[2];
static int anchor[2];              // 2P: angka sebelum kombinasi tahan-2-tombol dimulai
static int answers[2], gained[2];
static int total[2];
static uint32_t holdStart[4], lastRepeat[4];  // [P1-, P1+, P2-, P2+]
static uint32_t backHoldStart = 0;
static bool backHolding = false;

static int level() { return question / PER_LEVEL; }  // 0..4

static void loadMode() {
  if (modeLoaded) return;
  Preferences p;
  if (p.begin("hitung", true)) { mode = p.getUChar("mode", MODE_CPU) == MODE_2P ? MODE_2P : MODE_CPU; p.end(); }
  modeLoaded = true;
}

static void saveMode() {
  Preferences p;
  if (p.begin("hitung", false)) { p.putUChar("mode", mode); p.end(); }
}

static bool fits(int x, int y) {
  for (int i = 0; i < itemCount; i++)
    if (abs(items[i].x - x) < SPR + GAP && abs(items[i].y - y) < SPR + GAP) return false;
  return true;
}

static bool placeOne(uint8_t kind) {
  if (itemCount >= MAX_ITEMS) return false;
  for (int t = 0; t < 200; t++) {
    int x = random(128 - SPR + 1), y = AREA_Y0 + random(AREA_Y1 - AREA_Y0 + 2 - SPR);
    if (fits(x, y)) { items[itemCount++] = {(int8_t)x, (int8_t)y, kind}; return true; }
  }
  return false;
}

// Grid cadangan 12x5 sel 10 px (selalu muat 15 target)
static void placeGrid(int n) {
  itemCount = 0;
  uint8_t used[60] = {0};
  for (int i = 0; i < n; i++) {
    int s;
    do s = random(60); while (used[s]);
    used[s] = 1;
    items[itemCount++] = {(int8_t)(4 + (s % 12) * 10), (int8_t)(AREA_Y0 + 1 + (s / 12) * 10), target};
  }
}

static void makeQuestion() {
  const TargetDef &L = TARGETS[level()];
  const DistDef &D = DISTS[settings_hitungDistIdx()][level()];
  target = random(KINDS);
  correct = L.tMin + random(L.tMax - L.tMin + 1);
  bool ok = false;
  for (int attempt = 0; attempt < 10 && !ok; attempt++) {
    itemCount = 0;
    ok = true;
    for (int i = 0; i < correct && ok; i++) ok = placeOne(target);
  }
  if (!ok) placeGrid(correct);
  // Pengecoh: jenis lain; bila tidak muat dikurangi (target tidak pernah dikurangi)
  bool used[KINDS] = {false};
  used[target] = true;
  for (int k = 0; k < D.kinds; k++) {
    uint8_t kind;
    do kind = random(KINDS); while (used[kind]);  // tiap pengecoh jenis berbeda (maks KINDS - 1)
    used[kind] = true;
    int n = D.dMin + random(D.dMax - D.dMin + 1);
    for (int i = 0; i < n; i++) placeOne(kind);
  }
  // Acak urutan gambar (tidak berpengaruh ke posisi, hanya agar tidak ada pola)
  for (int i = itemCount - 1; i > 0; i--) { int j = random(i + 1); Item t = items[i]; items[i] = items[j]; items[j] = t; }
}

static void setSub(Sub s) { sub = s; subAt = millis(); }

static void startQuestion() {
  makeQuestion();
  setSub(SB_INTRO);
}

static void startAnswer() {
  value = 0;
  vals[0] = vals[1] = 0;
  locked[0] = locked[1] = false;
  lockHoldStart[0] = lockHoldStart[1] = 0;
  anchor[0] = anchor[1] = 0;
  memset(holdStart, 0, sizeof(holdStart));
  setSub(SB_ANSWER);
}

static int pointsFor(int a) { int d = abs(a - correct); return d == 0 ? 2 : d == 1 ? 1 : 0; }

static void finishQuestion() {
  if (mode == MODE_CPU) {
    int a = correct;
    if ((int)random(100) >= CPU_EXACT_PCT[level()]) {
      int off = 1 + random(2);
      a = correct + (random(2) ? off : -off);
      if (a < 0) a = correct + off;
    }
    answers[1] = a;
  }
  for (int i = 0; i < 2; i++) { gained[i] = pointsFor(answers[i]); total[i] += gained[i]; }
  setSub(SB_RESULT);
}

static void lockAnswer() {
  if (mode == MODE_2P) { answers[0] = vals[0]; answers[1] = vals[1]; }
  else answers[0] = value;
  finishQuestion();
}

static void saveHi() {
  if (mode == MODE_CPU && (uint32_t)total[0] > settings_hiscore(GAME_HITUNG)) settings_setHiscore(GAME_HITUNG, total[0]);
}

static void resetGame() {
  loadMode();
  total[0] = total[1] = 0;
  question = 0;
  backHolding = false;
  phase = PH_READY;
}

static void centerLine(int y, const char *s) {
  display.setCursor((128 - (int)strlen(s) * 6) / 2, y);
  display.print(s);
}

static void drawSprite(int x, int y, uint8_t kind) { display.drawBitmap(x, y, SPRITES[kind], SPR, SPR, SSD1306_WHITE); }

static const char *nameOf(int who) {
  if (mode == MODE_2P) return who == 0 ? "P1" : "P2";
  return who == 0 ? T("Kamu", "You") : "CPU";
}

// 2P: separuh layar kiri (x 0..62) dalam koordinat rotasi aktif; who 1 digambar dengan rotasi 180
static void drawHalf(int who, uint32_t now) {
  char buf[8];
  display.setTextSize(1);
  display.setCursor(2, 0); display.print(who == 0 ? "P1" : "P2");
  display.setCursor(40, 0); display.print("=");
  drawSprite(50, 0, target);
  if (locked[who]) {
    display.drawLine(20, 26, 27, 33, SSD1306_WHITE);  // centang
    display.drawLine(21, 26, 28, 33, SSD1306_WHITE);
    display.drawLine(27, 33, 41, 17, SSD1306_WHITE);
    display.drawLine(28, 33, 42, 17, SSD1306_WHITE);
    display.setCursor(17, 40); display.print(T("KUNCI", "LOCK"));
  } else {
    display.setTextSize(3);
    snprintf(buf, sizeof(buf), "%d", vals[who]);
    display.setCursor(31 - (int)strlen(buf) * 9, 14);
    display.print(buf);
    display.setTextSize(1);
    display.setCursor(1, 40); display.print("-");
    display.setCursor(56, 40); display.print("+");
    display.setCursor(7, 48); display.print(T("Tahan -+", "Hold -+"));  // maks 10 karakter (separuh layar)
  }
  uint32_t left = now - subAt >= ANSWER_2P_MS ? 0 : ANSWER_2P_MS - (now - subAt);
  display.drawRect(0, 58, 62, 5, SSD1306_WHITE);
  display.fillRect(1, 59, (int)(left * 60 / ANSWER_2P_MS), 3, SSD1306_WHITE);
}

static void draw() {
  uint32_t now = millis();
  char buf[24];
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (phase == PH_OVER) {
    if (mode == MODE_CPU) {
      const char *res = total[0] > total[1] ? T("KAMU MENANG", "YOU WIN") : total[0] < total[1] ? T("CPU MENANG", "CPU WINS") : T("SERI", "DRAW");
      snprintf(buf, sizeof(buf), "%s %d-%d", res, total[0], total[1]);
      centerLine(0, buf);
      games_drawOver(total[0], settings_hiscore(GAME_HITUNG));
    } else {
      centerLine(8, total[0] > total[1] ? T("P1 MENANG", "P1 WINS") : total[0] < total[1] ? T("P2 MENANG", "P2 WINS") : T("SERI", "DRAW"));
      display.setTextSize(2);
      snprintf(buf, sizeof(buf), "%d - %d", total[0], total[1]);
      display.setCursor((128 - (int)strlen(buf) * 12) / 2, 24);
      display.print(buf);
      display.setTextSize(1);
      centerLine(46, T("OK: ulang", "OK: again"));
      centerLine(55, T("BACK: keluar", "BACK: exit"));
    }
  } else if (sub == SB_INTRO) {
    snprintf(buf, sizeof(buf), T("Lv %d - Soal %d", "Lv %d - Q %d"), level() + 1, question + 1);
    centerLine(10, buf);
    display.setCursor(34, 34); display.print(T("Hitung:", "Count:"));
    // ikon target diperbesar 2x
    for (int r = 0; r < SPR; r++) {
      uint8_t bits = pgm_read_byte(&SPRITES[target][r]);
      for (int c = 0; c < SPR; c++)
        if (bits & (0x80 >> c)) display.fillRect(80 + c * 2, 30 + r * 2, 2, 2, SSD1306_WHITE);
    }
  } else if (sub == SB_SHOW) {
    snprintf(buf, sizeof(buf), "Lv%d %d/%d", level() + 1, question + 1, QUESTIONS);
    display.setCursor(0, 0); display.print(buf);
    drawSprite(120, 0, target);
    display.setCursor(108, 0); display.print("=");
    for (int i = 0; i < itemCount; i++) drawSprite(items[i].x, items[i].y, items[i].kind);
  } else if (sub == SB_ANSWER && mode == MODE_2P) {
    drawHalf(0, now);
    display.setRotation(2);  // sisi P2 terbaca dari seberang meja
    drawHalf(1, now);
    display.setRotation(0);
    display.drawFastVLine(63, 0, 64, SSD1306_WHITE);
  } else if (sub == SB_ANSWER) {
    snprintf(buf, sizeof(buf), T("%s jawab", "%s answers"), nameOf(0));
    display.setCursor(0, 0); display.print(buf);
    display.setCursor(108, 0); display.print("=");
    drawSprite(120, 0, target);
    display.setTextSize(3);
    snprintf(buf, sizeof(buf), "%d", value);
    display.setCursor((128 - (int)strlen(buf) * 18) / 2, 16);
    display.print(buf);
    display.setTextSize(1);
    display.setCursor(4, 22); display.print("<-");
    display.setCursor(112, 22); display.print("+>");
    centerLine(44, T("Kanan+ Kiri- OK kunci", "Right+ Left- OK lock"));
    uint32_t left = now - subAt >= ANSWER_MS ? 0 : ANSWER_MS - (now - subAt);
    display.drawRect(0, 56, 128, 6, SSD1306_WHITE);
    display.fillRect(2, 58, (int)(left * 124 / ANSWER_MS), 2, SSD1306_WHITE);
  } else {  // SB_RESULT
    snprintf(buf, sizeof(buf), T("Benar: %d", "Correct: %d"), correct);
    centerLine(2, buf);
    drawSprite(60, 12, target);
    for (int i = 0; i < 2; i++) {
      snprintf(buf, sizeof(buf), "%-4s %2d  +%d  =%2d", nameOf(i), answers[i], gained[i], total[i]);
      display.setCursor(4, 26 + i * 10); display.print(buf);
    }
    centerLine(54, question + 1 >= QUESTIONS ? T("OK: hasil akhir", "OK: final result") : T("OK: lanjut", "OK: next"));
  }
  if (backHolding) {  // tahan BACK: bar menuju keluar
    uint32_t held = min((uint32_t)(now - backHoldStart), BACK_HOLD_MS);
    display.fillRect(24, 48, 80, 14, SSD1306_BLACK);
    display.drawRect(24, 48, 80, 14, SSD1306_WHITE);
    display.setCursor(28, 50); display.print(T("Keluar...", "Exiting..."));
    display.fillRect(28, 58, held * 72 / BACK_HOLD_MS, 2, SSD1306_WHITE);
  }
  oled_flush();
}

// READY: tekan OK = mulai; TAHAN OK >= 1 dtk = layar WAKTU (atur waktu tampil Lv1-Lv5)
static const uint32_t TIME_HOLD_MS = 1000;
static bool okHolding = false;
static uint32_t okDownAt = 0;
static bool timeOpen = false;  // layar ATUR HITUNG
static uint8_t timeIdx = 0;    // waktu Lv5 yang sedang digeser
static uint8_t distIdx = 0;    // level pengecoh yang sedang digeser
static uint8_t atField = 0;    // 0 = Waktu, 1 = Pengecoh

static void drawReady() {
  static char modeLine[24];  // "< 2P serentak > 1,0s" = 20 karakter
  snprintf(modeLine, sizeof(modeLine), "< %s > %s", mode == MODE_2P ? T("2P serentak", "2P together") : "1P vs CPU",
           settings_hitungLv5Name());
  static const char *controls[3];
  controls[0] = modeLine;
  // Baris 4 tidak muat (Best & "OK: mulai" tetap di tempatnya): petunjuk waktu menggantikan baris
  // yang juga dijelaskan di layar main (2P: "Tahan -+" tampil saat menjawab)
  controls[1] = mode == MODE_2P ? "P1 <>   P2 OK/BACK" : T("Kanan+ Kiri- OK kunci", "Right+ Left- OK lock");
  controls[2] = T("Tahan OK: atur", "Hold OK: setup");
  games_drawReady(T("HITUNG", "MATH"), controls, 3, settings_hiscore(GAME_HITUNG));
  if (okHolding) {  // bar tahan OK di baris paling bawah
    uint32_t held = min((uint32_t)(millis() - okDownAt), TIME_HOLD_MS);
    display.fillRect(0, 63, (int)(held * 128 / TIME_HOLD_MS), 1, SSD1306_WHITE);
    oled_flush();
  }
}

static void printSeconds(char *buf, size_t len, uint32_t ms) {
  snprintf(buf, len, "%u,%us", (unsigned)(ms / 1000), (unsigned)(ms % 1000 / 100));
}

// Layar ATUR HITUNG: tiap level = waktu tampil + pengecoh (jenis x jumlah) untuk pilihan yang digeser;
// baris bawah (dibalik warna) = isian aktif
static void drawTime() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  centerLine(0, T("ATUR HITUNG", "MATH SETUP"));
  char sec[8], dist[10], line[24];
  for (int lv = 0; lv < 5; lv++) {
    printSeconds(sec, sizeof(sec), settings_hitungLv5MsAt(timeIdx) + (4 - lv) * SHOW_STEP_MS);
    const DistDef &D = DISTS[distIdx][lv];
    if (D.kinds) snprintf(dist, sizeof(dist), "%ux%u-%u", D.kinds, D.dMin, D.dMax);
    else snprintf(dist, sizeof(dist), "-");
    snprintf(line, sizeof(line), "Lv%d %s  %s", lv + 1, sec, dist);  // maks "Lv1 2,8s  3x3-4" = 15
    display.setCursor(19, 10 + lv * 9);
    display.print(line);
  }
  snprintf(line, sizeof(line), atField == 0 ? T("< Waktu %s >", "< Time %s >") : T("< Pengecoh %s >", "< Decoy %s >"),
           atField == 0 ? settings_hitungLv5NameAt(timeIdx) : settings_hitungDistNameAt(distIdx));
  display.fillRect(0, 55, 128, 9, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  centerLine(56, line);
  display.setTextColor(SSD1306_WHITE);
  oled_flush();
}

void hitung_enter() { resetGame(); }

void hitung_resume() {
  // Pop-up memotong tampilan sekilas / waktu jawab: ulang tahap itu
  if (phase == PH_PLAY && (sub == SB_INTRO || sub == SB_SHOW || sub == SB_ANSWER)) subAt = millis();
}

bool hitung_isPlaying() { return phase == PH_PLAY; }

// Tekan = 1 langkah; tahan = ulang otomatis setelah REPEAT_DELAY_MS
static int stepFor(int i, Btn b, bool pressed, uint32_t now) {  // i = indeks holdStart
  if (pressed) { holdStart[i] = now; lastRepeat[i] = now; return 1; }
  if (!btn_held(b)) { holdStart[i] = 0; return 0; }
  if (holdStart[i] && now - holdStart[i] >= REPEAT_DELAY_MS && now - lastRepeat[i] >= REPEAT_MS) {
    lastRepeat[i] = now;
    return 1;
  }
  return 0;
}

bool hitung_update(bool redraw) {
  Btn leftBtn = MENU_SWAP_LR ? BTN_DOWN : BTN_UP, rightBtn = MENU_SWAP_LR ? BTN_UP : BTN_DOWN;
  bool ok = btn_pressed(BTN_OK);
  bool left = btn_pressed(leftBtn), right = btn_pressed(rightBtn);
  bool back = btn_pressed(BTN_BACK);
  uint32_t now = millis();

  if (phase == PH_READY && timeOpen) {
    if (back) {
      if (atField > 0) { atField--; redraw = true; }
      else { timeOpen = false; drawReady(); return true; }  // batal: nilai lama
    }
    if (left || right) {
      if (atField == 0) timeIdx = (timeIdx + (right ? 1 : HITUNG_LV5_OPT_COUNT - 1)) % HITUNG_LV5_OPT_COUNT;
      else distIdx = (distIdx + (right ? 1 : HITUNG_DIST_OPT_COUNT - 1)) % HITUNG_DIST_OPT_COUNT;
      redraw = true;
    }
    if (ok) {
      if (atField == 0) {
        atField = 1;
        redraw = true;
      } else {
        settings_setHitungLv5Idx(timeIdx);
        settings_setHitungDistIdx(distIdx);
        timeOpen = false;
        drawReady();
        return true;
      }
    }
    if (redraw) drawTime();
    return true;
  }

  if (phase == PH_READY) {
    if (back) { okHolding = false; return false; }
    if (left || right) { mode = mode == MODE_2P ? MODE_CPU : MODE_2P; saveMode(); redraw = true; }
    if (ok) { okHolding = true; okDownAt = now; }  // keputusan saat dilepas / 1 dtk
    if (okHolding) {
      if (btn_held(BTN_OK)) {
        if (now - okDownAt >= TIME_HOLD_MS) {  // ditahan: buka layar WAKTU
          okHolding = false;
          timeIdx = settings_hitungLv5Idx();
          distIdx = settings_hitungDistIdx();
          atField = 0;
          timeOpen = true;
          drawTime();
          return true;
        }
        static uint32_t lastBar = 0;
        if (now - lastBar >= 50) { lastBar = now; redraw = true; }
        if (redraw) drawReady();
        return true;
      }
      okHolding = false;  // dilepas < 1 dtk: mulai main
    } else {
      if (redraw) drawReady();
      return true;
    }
    total[0] = total[1] = 0;
    question = 0;
    phase = PH_PLAY;
    startQuestion();
    draw();
    return true;
  }

  if (phase == PH_OVER) {
    if (back) return false;
    if (ok) { resetGame(); drawReady(); return true; }
    if (redraw) draw();
    return true;
  }

  // Di tengah permainan: tahan BACK 1 dtk untuk keluar (2P: tidak di fase jawab, BACK = tombol P2)
  bool backIsP2 = mode == MODE_2P && sub == SB_ANSWER;
  if (!backIsP2 && btn_held(BTN_BACK)) {
    if (!backHolding) { backHolding = true; backHoldStart = now; }
    if (now - backHoldStart >= BACK_HOLD_MS) { backHolding = false; saveHi(); return false; }
    redraw = true;
  } else if (backHolding) {
    backHolding = false;
    redraw = true;
  }

  Sub before = sub;
  switch (sub) {
    case SB_INTRO:
      if (now - subAt >= INTRO_MS) setSub(SB_SHOW);
      break;
    case SB_SHOW:
      if (now - subAt >= showMs(level())) startAnswer();
      break;
    case SB_ANSWER: {
      if (mode == MODE_2P) {
        bool okPressed = ok;  // OK = tombol P2 (plus), bukan kunci
        const Btn minus[2] = {leftBtn, games_p2MinusBtn()}, plus[2] = {rightBtn, games_p2PlusBtn()};
        const bool minusP[2] = {left, games_p2MinusBtn() == BTN_BACK ? back : okPressed};
        const bool plusP[2] = {right, games_p2PlusBtn() == BTN_OK ? okPressed : back};
        for (int p = 0; p < 2; p++) {
          if (locked[p]) continue;
          if (btn_held(minus[p]) && btn_held(plus[p])) {  // tahan kedua tombol sendiri = kunci
            if (!lockHoldStart[p]) { lockHoldStart[p] = now; vals[p] = anchor[p]; redraw = true; }  // batalkan langkah +/- dari kombinasi
            if (now - lockHoldStart[p] >= LOCK_HOLD_MS) { locked[p] = true; redraw = true; }
            continue;
          }
          lockHoldStart[p] = 0;
          int inc = stepFor(p * 2 + 1, plus[p], plusP[p], now), dec = stepFor(p * 2, minus[p], minusP[p], now);
          if (inc || dec) {
            if (!btn_held(inc ? minus[p] : plus[p])) anchor[p] = vals[p];  // langkah tunggal: simpan titik awal
            vals[p] = constrain(vals[p] + inc - dec, 0, MAX_ANSWER);
            redraw = true;
          }
        }
        if ((locked[0] && locked[1]) || now - subAt >= ANSWER_2P_MS) lockAnswer();
        else {
          static uint32_t lastBar2 = 0;
          if (now - lastBar2 >= 100) { lastBar2 = now; redraw = true; }
        }
        break;
      }
      int inc = stepFor(1, rightBtn, right, now), dec = stepFor(0, leftBtn, left, now);
      if (inc || dec) { value = constrain(value + inc - dec, 0, MAX_ANSWER); redraw = true; }
      if (ok || now - subAt >= ANSWER_MS) lockAnswer();
      else {
        static uint32_t lastBar = 0;
        if (now - lastBar >= 100) { lastBar = now; redraw = true; }  // bar waktu menyusut
      }
      break;
    }
    case SB_RESULT:
      if (ok) {
        if (++question >= QUESTIONS) { phase = PH_OVER; saveHi(); }
        else startQuestion();
        redraw = true;
      }
      break;
  }
  if (sub != before) redraw = true;
  if (redraw) draw();
  return true;
}
