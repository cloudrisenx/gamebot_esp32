"""Simulasi layar Pet kucing (docs/PET_PROMPT.md) di OLED 0,96" 128x64 monokrom, 30 fps.

Jalankan:  python tools/pet_sim.py                           (jendela interaktif, butuh pygame)
           python tools/pet_sim.py --sheet tools/pet_sheet.png   (semua ekspresi & adegan jadi satu gambar)
           python tools/pet_sim.py --morph tools/pet_morph.png   (frame beruntun wajah -> kucing 8-bit)
           python tools/pet_sim.py --selftest                    (cek cepat tanpa jendela)

Tombol: klik dengan mouse, atau keyboard   1 = UP (kiri)   2 = DOWN (kanan)   3 = OK   4 = BACK (keluar)
  tekan sebentar:  UP / DOWN = melirik sambil memiringkan kepala      OK = elus (senang + hati)
                   OK 3x cepat = geli;  OK 6x cepat = digelitik terus -> pet KABUR
  TAHAN (>= 0,5 dtk):  UP (kiri)    = KASIH MAKAN: ikan jatuh ke mulut, dikunyah; 3 ikan = kenyang, lalu mencuci muka
                       DOWN (kanan) = AJAK MAIN: wajah berubah mulus jadi kucing 8-bit; tikus mainan berlari-lari,
                                      kucing mengamati, mengendap, goyang pantat, menerkam, menangkap, mengejar
                       OK           = dielus terus: mendengkur + kaki depan menguleni (kneading);
                                      > 3,5 dtk = ketiduran, dilepas = bangun sambil menguap
Perangkat aslinya HANYA punya 4 tombol, jadi jendela ini juga hanya menampilkan 4 tombol. Semuanya bisa dicapai
dengan 4 tombol itu (ketuk / tahan) atau dibiarkan saja.
Untuk menguji cepat (bukan fitur perangkat): python tools/pet_sim.py --debug  ->  tombol keyboard tersembunyi
  N = notifikasi baru   S = langsung tidur   K = kabur   Z = zoomies   R = kegiatan acak   L = tidur melingkar
  G = ngumpet (tiba-tiba menghilang lalu mengintip dari atas / kiri / kanan / bawah layar)

TANPA MENEKAN TOMBOL pun pet hidup sendiri (hanya 4 tombol, jadi sisanya kucing yang bertingkah): melirik, kedip
lambat, kepala miring, mencuci muka, melihat burung, menguap, mengendus ... dan sesekali berubah jadi kucing 8-bit
untuk jalan-jalan, mengejar ekor sendiri, peregangan, menggaruk telinga, main sendiri dengan tikus mainan, zoomies,
pergi main / ngumpet saat bosan, dan tidur melingkar (loaf) saat mengantuk. Kabur dengan KESAL hanya terjadi
kalau diganggu terus (OK ditekan 6x cepat), tidak pernah tiba-tiba.
Tidur (wajah saja maupun tidur melingkar) paling lama 3 menit (acak 0,5-3 menit), lalu bangun sendiri sambil menguap.

Perilaku mengikuti bahasa tubuh kucing sungguhan: telinga (tegak / menyamping / rata), pupil membesar saat
tertarik, kedip lambat = percaya, kepala miring = penasaran, kumis maju saat mengintai, mencuci muka setelah
makan, mengunyah-ngoceh melihat burung, kneading saat mendengkur, mengendap -> goyang pantat -> terkam,
zoomies. Gerak dibuat mulus (pegas) seperti animasi frames.bin, bukan ganti gambar kaku.
Gambar memakai algoritma Adafruit_GFX (lingkaran, round rect, segitiga) supaya sama dengan di jam.
"""
import math
import os
import random
import sys
from collections import defaultdict
from types import SimpleNamespace

W, H = 128, 64


def rnd(x):
    return int(math.floor(x + 0.5))


# ---------------------------------------------------------------- framebuffer + primitif ala Adafruit_GFX


class Fb:
    def __init__(self, w=W, h=H):
        self.w, self.h = w, h
        self.px = [bytearray(w) for _ in range(h)]
        self.ox = self.oy = 0  # geser seluruh gambar (loncat, napas, getar)

    def clear(self):
        for row in self.px:
            row[:] = bytes(self.w)

    def pixel(self, x, y, c=1):
        x, y = x + self.ox, y + self.oy
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def vline(self, x, y, h, c=1):
        for i in range(h):
            self.pixel(x, y + i, c)

    def hline(self, x, y, w, c=1):
        for i in range(w):
            self.pixel(x + i, y, c)

    def fill_rect(self, x, y, w, h, c=1):
        for i in range(w):
            self.vline(x + i, y, h, c)

    def line(self, x0, y0, x1, y1, c=1):
        dx, dy = abs(x1 - x0), abs(y1 - y0)
        sx, sy = (1 if x0 < x1 else -1), (1 if y0 < y1 else -1)
        err = dx - dy
        while True:
            self.pixel(x0, y0, c)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x0 += sx
            if e2 < dx:
                err += dx
                y0 += sy

    def _fill_circle_helper(self, x0, y0, r, corners, delta, c=1):
        f, ddx, ddy, x, y = 1 - r, 1, -2 * r, 0, r
        px, py = x, y
        delta += 1
        while x < y:
            if f >= 0:
                y -= 1
                ddy += 2
                f += ddy
            x += 1
            ddx += 2
            f += ddx
            if x < y + 1:
                if corners & 1:
                    self.vline(x0 + x, y0 - y, 2 * y + delta, c)
                if corners & 2:
                    self.vline(x0 - x, y0 - y, 2 * y + delta, c)
            if y != py:
                if corners & 1:
                    self.vline(x0 + py, y0 - px, 2 * px + delta, c)
                if corners & 2:
                    self.vline(x0 - py, y0 - px, 2 * px + delta, c)
                py = y
            px = x

    def fill_circle(self, x0, y0, r, c=1):
        self.vline(x0, y0 - r, 2 * r + 1, c)
        self._fill_circle_helper(x0, y0, r, 3, 0, c)

    def fill_round_rect(self, x, y, w, h, r, c=1):
        if w <= 0 or h <= 0:
            return
        r = max(0, min(r, w // 2, h // 2))
        self.fill_rect(x + r, y, w - 2 * r, h, c)
        if r:
            self._fill_circle_helper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, c)
            self._fill_circle_helper(x + r, y + r, r, 2, h - 2 * r - 1, c)

    def fill_triangle(self, x0, y0, x1, y1, x2, y2, c=1):
        pts = [(x0, y0), (x1, y1), (x2, y2)]
        for y in range(min(p[1] for p in pts), max(p[1] for p in pts) + 1):
            xs = []
            for (ax, ay), (bx, by) in ((pts[0], pts[1]), (pts[1], pts[2]), (pts[2], pts[0])):
                if ay == by:
                    if ay == y:
                        xs += [ax, bx]
                elif min(ay, by) <= y <= max(ay, by):
                    xs.append(rnd(ax + (bx - ax) * (y - ay) / (by - ay)))  # Adafruit membulatkan sedikit beda: <= 1 px
            if xs:
                self.hline(min(xs), y, max(xs) - min(xs) + 1, c)

    def curve(self, p0, p1, p2, thick=1, c=1):
        """Kurva Bezier kuadratik p0 -> p2 (kontrol p1), digambar dengan garis pendek."""
        prev = None
        for i in range(11):
            t = i / 10
            x = (1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0]
            y = (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1]
            cur = (rnd(x), rnd(y))
            if prev:
                for d in range(thick):
                    self.line(prev[0], prev[1] + d, cur[0], cur[1] + d, c)
            prev = cur

    # Font bawaan Adafruit 5x7 (kolom, bit0 = baris atas): hanya glyph yang dipakai Pet
    _GLYPHS = {"z": (0x44, 0x64, 0x54, 0x4C, 0x44), "Z": (0x61, 0x51, 0x49, 0x45, 0x43),
               "!": (0x00, 0x00, 0x5F, 0x00, 0x00)}

    def char(self, x, y, ch, size=1, c=1):
        for col, bits in enumerate(self._GLYPHS[ch]):
            for row in range(8):
                if bits >> row & 1:
                    self.fill_rect(x + col * size, y + row * size, size, size, c)

    def bitmap(self, x, y, rows, c=1):
        for r, row in enumerate(rows):
            for col, ch in enumerate(row):
                if ch == "1":
                    self.pixel(x + col, y + r, c)

    def blit(self, src, x, y, flip=False):
        """Tempel sprite (piksel mati = transparan); flip = cermin kiri-kanan."""
        for sy in range(src.h):
            row = src.px[sy]
            for sx in range(src.w):
                if row[sx]:
                    self.pixel(x + (src.w - 1 - sx if flip else sx), y + sy)


# ---------------------------------------------------------------- gambar wajah (dari parameter yang berubah mulus)

HEART = ("01010", "11111", "11111", "01110", "00100")
WHISKERS_L = [((53, 43), (32, 40)), ((53, 47), (29, 47)), ((53, 51), (32, 54))]  # pendek: pangkal di moncong, ujung dekat pipi
EYE_CY = 25


def _eye(fb, F, side, b):
    """side: -1 mata kiri, +1 mata kanan. b: 0 = terbuka .. 1 = terpejam (kedip)."""
    w = F.w * (1 + 0.14 * b)  # kedip: memipih lalu melebar (squash & stretch)
    h = F.h + (2.5 - F.h) * b
    cx = 64.5 + side * 24.5 + F.gx * 6
    cy = EYE_CY + F.gy * 4 + side * F.htilt * 2.5  # kepala miring: satu mata lebih rendah
    x0, y0, wi, hi = rnd(cx - w / 2), rnd(cy - h / 2), max(2, rnd(w)), max(2, rnd(h))
    y0 += rnd(hi * F.lb * 0.5)  # kubah (kelopak bawah naik) tetap berpusat di tinggi mata yang sama

    if F.geli:  # "> <"
        outer, tip = rnd(cx + 7 * side), rnd(cx - 5 * side)
        for d in (0, 1):
            fb.line(outer, rnd(cy) - 6 + d, tip, rnd(cy) + d)
            fb.line(tip, rnd(cy) + d, outer, rnd(cy) + 6 + d)
        return
    if hi <= 5:  # terpejam: garis/busur tebal 2
        sag = F.curve * 3
        fb.curve((x0, rnd(cy)), (rnd(cx), rnd(cy + 2 * sag)), (x0 + wi, rnd(cy)), 2)
        return
    fb.fill_round_rect(x0, y0, wi, hi, rnd(min(F.r, w / 2, h / 2)))
    if F.pw >= 1 and F.ph >= 1:  # pupil celah / bulat (melebar saat tertarik atau terkejut)
        pw, ph = rnd(F.pw), rnd(F.ph * min(1.0, h / 22))
        px = rnd(cx + F.gx * 3 - pw / 2)
        py = rnd(cy + F.gy * 1.5 - ph / 2 + (F.lt * h * 0.35))
        fb.fill_round_rect(px, py, pw, ph, pw // 2, 0)
        if pw >= 6 and ph >= 8:  # kilau cahaya di pupil: mata terlihat basah dan imut
            fb.fill_rect(px + 1 if side < 0 else px + pw - 3, py + 2, 2, 2)
    if F.lt > 0.02:  # kelopak atas: miring, sudut luar lebih rendah (sayu)
        top = y0 + F.lt * hi
        out_y, in_y = top + F.tilt * hi * 0.3, top - F.tilt * hi * 0.3
        xo, xi = (x0 - 1, x0 + wi + 1) if side < 0 else (x0 + wi + 1, x0 - 1)
        yo, yi = rnd(out_y), rnd(in_y)
        fb.fill_triangle(xo, y0 - 3, xi, y0 - 3, xi, yi, 0)
        fb.fill_triangle(xo, y0 - 3, xo, yo, xi, yi, 0)
    if F.lb > 0.02:  # kelopak bawah naik: mata jadi kubah (senang)
        yb = rnd(y0 + hi - F.lb * hi)
        fb.fill_rect(x0 - 1, yb, wi + 2, y0 + hi - yb + 2, 0)


def _ears(fb, F):
    """Telinga di pojok atas layar: tegak (0) -> menyamping "pesawat" (0,5) -> rata ke belakang (1)."""
    for side in (-1, 1):
        e = max(-0.2, min(1.0, F.earL if side < 0 else F.earR))
        if e <= 0.5:
            u = max(0.0, e) * 2
            tx, ty = 15 - 13 * u, -3 + 10 * u
        else:
            u = (e - 0.5) * 2
            tx, ty = 2 + 4 * u, 7 + 6 * u
        pts = [(7, 11), (24, 11), (tx, ty)]
        inner = [(12, 10), (19, 10), (15 + (tx - 15) * 0.4, 10 + (ty - 10) * 0.3)]  # bagian dalam telinga: kecil
        if side > 0:
            pts = [(128 - x, y) for x, y in pts]
            inner = [(128 - x, y) for x, y in inner]
        fb.fill_triangle(*[rnd(v) for p in pts for v in p])
        if e < 0.4:
            fb.fill_triangle(*[rnd(v) for p in inner for v in p], 0)


def _mouth(fb, F, mx):
    my = 44
    mo = max(F.mo, 0.45 if F.jaw else 0.0)  # rahang mengoceh: langsung buka-tutup, tanpa pegas
    if mo > 0.25:  # terbuka (menguap, tertawa, mengunyah): cincin + lidah
        mw, mh = max(5, rnd(F.mw)), max(5, rnd(mo * 14))
        x0, y0 = mx - mw // 2, my - 1
        fb.vline(mx, 43, y0 - 43)
        fb.fill_round_rect(x0, y0, mw, mh, min(mw, mh) // 2)
        fb.fill_round_rect(x0 + 2, y0 + 2, mw - 4, mh - 4, max(0, min(mw - 4, mh - 4) // 2), 0)
        if mo > 0.5 and mw >= 8:
            fb.fill_round_rect(x0 + 3, y0 + mh - 5, mw - 6, 3, 1)
        return
    fb.vline(mx, 43, my - 43)
    for s in (-1, 1):  # "ω": dua busur kecil, melengkung sesuai mc
        fb.curve((mx, my), (mx + s * F.mw / 2, my + 2 * F.mc), (mx + s * F.mw, my), 1)
    if F.tongue > 0.3:
        fb.fill_round_rect(mx - 2, rnd(my + F.mc + 1), 4, rnd(2 + F.tongue * 3), 2)


def _whiskers(fb, F, shift):
    for i, ((sx, sy), (ex, ey)) in enumerate(WHISKERS_L):
        ex, ey = sx + (ex - sx) * F.wlen, sy + (ey - sy) * F.wlen  # kumis maju (pendek di tampak depan) saat tertarik
        ey = ey + (sy - ey) * F.wflat - F.wh * 3 + math.sin(F.t / 85 + i * 1.7) * F.amp
        for side in (-1, 1):
            e = ey - (2 if F.twitch in ("L" if side < 0 else "R", "B") else 0) + side * F.htilt * 1.5
            xs, xe = (sx, ex) if side < 0 else (128 - sx, 128 - ex)
            mx, my = (xs + xe) / 2, (sy + e) / 2 + 1.2  # melengkung halus
            fb.curve((rnd(xs) + shift, sy), (rnd(mx) + shift, rnd(my)), (rnd(xe) + shift, rnd(e)), 1)


def _paw(fb, cx, cy, w=12, h=14):
    """Telapak kaki depan: halo hitam supaya terpisah dari wajah, tiga jari."""
    fb.fill_round_rect(cx - w // 2 - 1, cy - h // 2 - 1, w + 2, h + 2, w // 2 + 1, 0)
    fb.fill_round_rect(cx - w // 2, cy - h // 2, w, h, w // 2)
    for dx in (-w // 6, w // 6):
        fb.vline(cx + dx, cy + h // 8, max(2, h // 3), 0)


def _bird(fb, x, y, flap, d):
    """Burung kecil 8-bit; d = arah terbang (+1 ke kanan)."""
    x, y = rnd(x), rnd(y)

    def px(v):
        return x + (v if d > 0 else 8 - v)

    fb.fill_round_rect(min(px(0), px(7)), y + 1, 7, 4, 2)
    fb.pixel(px(8), y + 2)
    fb.line(px(-3), y + 2, px(0), y + 2)
    wing = -3 if flap else 3
    fb.line(px(2), y + 1, px(4), y + 1 + wing)
    fb.line(px(3), y + 1, px(5), y + 1 + wing)


def _draw_face(fb, F):
    fb.clear()
    fb.ox, fb.oy = rnd(F.shake[0]), rnd(F.shake[1] + F.fy + F.breath)
    _ears(fb, F)
    _eye(fb, F, -1, F.bL)
    _eye(fb, F, 1, F.bR)
    ny, nx = F.nose, rnd(F.htilt * 1.5)
    fb.fill_triangle(61 + nx, 39 + ny, 67 + nx, 39 + ny, 64 + nx, 43 + ny)   # hidung kecil
    _mouth(fb, F, 64 + nx)
    _whiskers(fb, F, nx)
    if F.blush > 0.5:  # pipi merona (pola bintik) saat senang
        for yy in range(33, 36):
            for xx in range(28, 37):
                if (xx + yy) % 2 == 0:
                    fb.pixel(xx, yy)
                    fb.pixel(128 - xx, yy)
    fb.ox = fb.oy = 0  # hiasan di bawah tidak ikut bergetar
    for hx, hy in F.hearts:
        fb.bitmap(rnd(hx), rnd(hy), HEART)
    if F.fish:  # ikan jatuh: hilang masuk ke mulut (dipotong di atas mulut)
        fx, fy = rnd(F.fish[0]) - 7, rnd(F.fish[1])
        for sy in range(_FISH.h):
            for sx in range(_FISH.w):
                if _FISH.px[sy][sx] and fy + sy <= 41:
                    fb.pixel(fx + sx, fy + sy)
    for cx, cy in F.crumbs:  # remah makanan
        fb.fill_rect(rnd(cx), rnd(cy), 2, 1)
    if F.paw:  # mencuci muka
        _paw(fb, rnd(F.paw[0]), rnd(F.paw[1]))
    if F.knead:  # kneading: kaki depan menekan bergantian
        _paw(fb, 46, rnd(F.knead[0]), 10, 10)
        _paw(fb, 82, rnd(F.knead[1]), 10, 10)
    if F.bird:
        _bird(fb, *F.bird)
    if F.exclaim:
        fb.char(59, 0, "!", 2)
    for zx, zy, big in F.zs:
        fb.char(rnd(zx), rnd(zy), "Z" if big else "z", 2 if big else 1)


# ---------------------------------------------------------------- adegan kucing 8-bit (badan penuh, beberapa pose)

SPR_W, SPR_H, GROUND, SPR_Y0 = 42, 26, 23, 30  # kaki menapak di y layar = SPR_Y0 + GROUND = 53
_CAT = Fb(SPR_W, SPR_H)
_MOUSE = Fb(12, 6)
SIT_HEAD = (21.0, 6.5)  # pusat kepala pose duduk di dalam sprite: titik tujuan wajah mengecil

POSES = {
    "sit":    dict(sitting=True, head_dy=-1, ear=0.0, tail=-0.5),
    "alert":  dict(crouch=0.15, head_dy=-1, ear=0.0, tail=0.3, pupil=2),
    "walk":   dict(crouch=0.0, ear=0.0, tail=0.9, gait=(3, 2)),
    "run":    dict(crouch=0.0, ear=0.4, tail=0.2, gait=(4, 3)),
    "stalk":  dict(crouch=1.0, head_dy=2, ear=0.35, tail=-0.8, gait=(1.5, 1), pupil=2),
    "freeze": dict(crouch=1.0, head_dy=2, ear=0.3, tail=-0.8, pupil=2),
    "wiggle": dict(crouch=1.0, head_dy=2, ear=0.3, tail=-0.8, pupil=2, wiggle=True),
    "pounce": dict(crouch=0.1, head_dy=-1, ear=0.4, tail=0.5, pupil=2, mouth=1,
                   legs=[(-6, 0), (-7, 0), (6, 6), (7, 7)]),
    "catch":  dict(crouch=0.9, head_dy=3, ear=0.6, tail=-0.3, mouth=1, prey=True),
    "spin":   dict(sitting=True, head_dy=1, ear=0.2, tail=0.9, wagamp=3, wagspd=6.0, pupil=2),     # mengejar ekor
    "dizzy":  dict(sitting=True, head_dy=2, ear=0.5, tail=-0.2, eyes_closed=True),
    "stretch1": dict(crouch=0.5, head_dy=3, ear=0.2, tail=0.9, tilt=4, legs=[(0, 0), (1, 0), (7, 0), (8, 0)]),
    "arch":   dict(crouch=0.0, head_dy=1, ear=0.5, tail=1.0, arch=5, mouth=1, eyes_closed=True),   # punggung melengkung
    "scratch": dict(sitting=True, head_dy=0, ear=0.3, tail=-0.4, eyes_closed=True),                 # + scratch=osilasi
    "loaf":   dict(crouch=1.4, head_dy=3, ear=0.5, tail=-0.5, eyes_closed=True, nolegs=True),        # tidur melingkar
}


DEFAULTS = dict(crouch=0.0, head_dy=0.0, ear=0.0, tail=0.5, tilt=0.0, arch=0.0, bob=0.0)  # parameter pose yang diperhalus


def _cat_sitting(ph, P):
    """Kucing DUDUK tegak (tampak samping, menghadap kanan): paha di lantai, dada tegak, kaki depan lurus, ekor melengkung."""
    s = _CAT
    s.clear()
    ear = P.get("ear", 0.0)
    top = 3 + rnd(P.get("head_dy", 0.0)) + rnd(P.get("bob", 0.0))
    tail = P.get("tail", -0.5)
    flick = rnd(math.sin(ph * P.get("wagspd", 1.2)) * P.get("wagamp", 1.5 if tail > 0 else 0.7))
    s.curve((7, 20), (-2, 25), (3, 14 + flick), 2)                  # ekor melengkung di belakang, ujungnya berkedut
    s.fill_round_rect(5, 12, 13, 12, 5)                             # paha belakang
    s.fill_round_rect(13, top + 7, 9, 24 - (top + 7), 3)            # dada & badan tegak
    s.fill_round_rect(15, top, 13, 10, 4)                           # kepala
    s.fill_triangle(16, top + 1, rnd(17 - ear * 2), rnd(top - 4 + ear * 3), 21, top + 1)   # telinga
    s.fill_triangle(23, top + 1, rnd(25 - ear * 2), rnd(top - 4 + ear * 3), 27, top + 2)
    if P.get("eyes_closed"):
        s.hline(23, top + 4, 3, 0)
    else:
        big = P.get("pupil", 1) > 1
        s.fill_rect(23, top + 3, 2, 3 if big else 2, 0)             # mata hitam besar
        s.pixel(23, top + 3)                                        # kilau cahaya
    s.pixel(27, top + 5, 0)                                         # hidung
    if P.get("mouth"):
        s.hline(25, top + 7, 3, 0)
    s.fill_round_rect(18, 21, 7, 3, 1)                              # kaki depan menghadap ke kanan
    s.fill_round_rect(3, 21, 9, 3, 1)                               # kaki belakang
    s.vline(17, top + 13, 8, 0)                                     # celah antara paha dan kaki depan
    scratch = P.get("scratch")
    if scratch is not None:  # kaki belakang terangkat menggaruk telinga (diberi tepi hitam 1 px supaya terlihat)
        fx, fy = 22 + rnd(scratch * 2), top + 2
        for ox in (-1, 0, 1, 2):
            s.line(12 + ox, 18, fx + ox, fy, 0)
        for d in (0, 1):
            s.line(12 + d, 18, fx + d, fy)
        s.fill_rect(fx - 1, fy - 1, 3, 3)


def _cat_sprite(ph, P):
    """Kucing menghadap kanan (di-flip saat menghadap kiri). Kaki: garis tebal 2 px. P = dict pose (POSES)."""
    if P.get("sitting"):
        _cat_sitting(ph, P)
        return
    s = _CAT
    s.clear()

    def g(k):
        return P.get(k, DEFAULTS[k])

    cr, ear, tail, tilt, arch = g("crouch"), g("ear"), g("tail"), g("tilt"), g("arch")
    dy = rnd(5 * cr)
    wob = (1 if math.sin(ph) > 0 else 0) if P.get("wiggle") else 0  # goyang pantat: pinggul naik-turun, kepala diam
    base = 8 + dy + rnd(g("bob"))
    by_r, by_f = base - rnd(tilt) + wob, base      # atas punggung belakang / depan (tilt > 0: pantat naik)
    hy = base - 4 + rnd(g("head_dy"))
    hip_r, hip_f = by_r + 7, by_f + 7
    wag = math.sin(ph * 0.5 * P.get("wagspd", 1.0)) * P.get("wagamp", 2 if tail > 0.6 else 1)
    wag += math.sin(ph * 3) * 1.5 if P.get("wiggle") else 0
    s.curve((7, by_r + 3), (0, by_r + 8), (2, rnd(by_r + 3 - tail * 9 + wag)), 2)    # ekor
    for x in range(6, 23):                                                           # badan: tegak / miring / melengkung
        u = (x - 6) / 16
        top = rnd(by_r + (by_f - by_r) * u - arch * 4 * u * (1 - u))
        trim = 1 if x in (6, 22) else 0
        s.fill_rect(x, top + trim, 1, 8 - 2 * trim)
    s.fill_round_rect(21, hy, 12, 9, 3)                                              # kepala
    s.fill_triangle(22, hy + 1, rnd(23 - ear * 2), rnd(hy - 4 + ear * 3), 26, hy + 1)   # telinga
    s.fill_triangle(28, hy + 1, rnd(30 - ear * 2), rnd(hy - 4 + ear * 3), 32, hy + 2)
    if P.get("eyes_closed"):
        s.hline(29, hy + 4, 3, 0)
    else:
        big = P.get("pupil", 1) > 1
        s.fill_rect(29, hy + 3, 2, 3 if big else 2, 0)                               # mata hitam besar
        s.pixel(29, hy + 3)                                                          # kilau cahaya
    s.pixel(32, hy + 5, 0)                                                           # hidung
    if P.get("mouth"):
        s.hline(30, hy + 7, 3, 0)
    if P.get("prey"):  # tikus di mulut
        s.fill_rect(33, hy + 6, 4, 3)
        s.line(37, hy + 7, 40, hy + 9)
    legs, scratch = P.get("legs"), P.get("scratch")
    if not P.get("nolegs"):
        for i, (hx, off) in enumerate(((8, 0.0), (11, 2.1), (18, 3.14), (21, 5.2))):  # belakang x2, depan x2
            if scratch is not None and i == 1:
                continue
            if legs:
                dx, lift = legs[i]
            elif P.get("gait"):
                amp, la = P["gait"]
                a = ph + off
                dx, lift = rnd(math.sin(a) * amp), max(0.0, math.cos(a)) * la
            else:
                dx, lift = 0, 0
            hip = hip_r if i < 2 else hip_f
            for d in (0, 1):
                s.line(hx + d, hip, hx + d + dx, rnd(GROUND - lift))
    if scratch is not None:  # kaki belakang terangkat di atas punggung menggaruk telinga
        fx, fy = 22 + rnd(scratch * 2), hy - 3
        for d in (0, 1):
            s.line(13 + d, by_r + 1, fx + d, fy)
            s.line(13 + d, by_r + 1, fx + d, fy + 1)


def _mouse_sprite(ph):
    m = _MOUSE
    m.clear()
    m.fill_round_rect(3, 1, 7, 4, 2)
    m.pixel(10, 3)
    m.pixel(8, 0)
    m.pixel(9, 2, 0)
    m.line(3, 4, 0, 2)
    for x in ((4, 7) if int(ph * 6) % 2 else (5, 8)):  # kaki berlari kecil
        m.pixel(x, 5)


def _build_fish():
    f = Fb(14, 8)
    f.fill_round_rect(0, 1, 10, 6, 3)
    f.fill_triangle(9, 4, 13, 0, 13, 7)
    f.pixel(2, 3, 0)
    return f


_FISH = _build_fish()
_FACE = Fb()
_BAYER = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))  # dithering 4x4: peralihan ala 8-bit


def _smooth(a, b, x):
    x = max(0.0, min(1.0, (x - a) / (b - a)))
    return x * x * (3 - 2 * x)


def _vis(alpha, x, y):
    return alpha > (_BAYER[y % 4][x % 4] + 0.5) / 16


def _blit_scaled(dst, src, cx, cy, sc, alpha):
    """Tempel `src` (128x64) diperkecil `sc` kali di sekitar (cx, cy); alpha < 1 = pudar dengan dithering."""
    inv = 1 / sc
    blk = max(1, int(math.ceil(inv)))
    hw, hh = 64 * sc, 32 * sc
    for dy in range(int(cy - hh) - 1, int(cy + hh) + 2):
        for dx in range(int(cx - hw) - 1, int(cx + hw) + 2):
            if not (0 <= dx < W and 0 <= dy < H) or not _vis(alpha, dx, dy):
                continue
            sx0, sy0 = rnd((dx - cx) * inv + 64 - blk / 2), rnd((dy - cy) * inv + 32 - blk / 2)
            hit = 0
            for yy in range(max(0, sy0), min(H, sy0 + blk)):
                row = src.px[yy]
                for xx in range(max(0, sx0), min(W, sx0 + blk)):
                    hit += row[xx]
            if hit >= max(1, blk * blk * 0.3):
                dst.pixel(dx, dy)


def _head_xy(x, flip, y=0.0):
    """Pusat kepala kucing (duduk) di layar: ke sinilah wajah mengecil / dari sini wajah membesar."""
    hx = (SPR_W - 1 - SIT_HEAD[0]) if flip else SIT_HEAD[0]
    return rnd(x) - SPR_W // 2 + hx, SPR_Y0 + y + SIT_HEAD[1]


def _draw_dust(fb, F):
    for dx, dy, age in F.dust:  # debu mengecil
        sz = 3 if age < 150 else 2 if age < 300 else 1
        fb.fill_rect(rnd(dx) - sz // 2, rnd(dy) - sz // 2, sz, sz)


def _draw_morph(fb, F):
    """Peralihan wajah <-> kucing: wajah mengecil menuju kepala sementara badan tumbuh dari kepala ke ekor."""
    c = F.cat
    u = c["u"]
    e = _smooth(0, 1, u)
    _draw_face(_FACE, F)
    fb.clear()
    fb.ox = fb.oy = 0
    _draw_dust(fb, F)
    hx, hy = _head_xy(c["x"], c["flip"], c.get("y", 0.0))
    _cat_sprite(0.0, POSES["sit"])
    x0, y0, reach, a_body = rnd(c["x"]) - SPR_W // 2, SPR_Y0 + rnd(c.get("y", 0.0)), e * 44, _smooth(0.1, 0.7, u)
    for sy in range(_CAT.h):
        for sx in range(_CAT.w):
            if _CAT.px[sy][sx] and abs(sx - SIT_HEAD[0]) <= reach:
                dx = x0 + (SPR_W - 1 - sx if c["flip"] else sx)
                if _vis(a_body, dx, y0 + sy):
                    fb.pixel(dx, y0 + sy)
    a_face = 1 - _smooth(0.55, 1.0, u)
    if a_face > 0:
        _blit_scaled(fb, _FACE, 64 + (hx - 64) * e, 32 + (hy - 32) * e, 1 - 0.83 * e, a_face)


_PEEK = Fb(34, 30)


def _peek_sprite(look):
    """Kepala kucing menghadap keluar dari tepi (telinga di atas kanvas, mata di tengah). Diputar sesuai sisi layar."""
    k = _PEEK
    k.clear()
    k.fill_round_rect(2, 7, 30, 23, 9)
    k.fill_triangle(3, 12, 5, 1, 12, 8)
    k.fill_triangle(31, 12, 29, 1, 22, 8)
    lk = rnd(look * 2)
    for ex in (9, 21):
        k.fill_round_rect(ex + lk, 13, 5, 8, 2, 0)
        k.pixel(ex + lk + 1, 14)  # kilau
    k.fill_rect(16, 23, 2, 1, 0)


def _draw_peek(fb, edge, vis, pos, look):
    """vis = berapa piksel kepala yang sudah menonjol dari tepi; pos = posisi di sepanjang tepi."""
    _peek_sprite(look)
    for r in range(_PEEK.h):
        for c in range(_PEEK.w):
            if _PEEK.px[r][c]:
                if edge == "bawah":
                    x, y = pos + c - 17, H - vis + r
                elif edge == "atas":
                    x, y = pos + c - 17, vis - 1 - r
                elif edge == "kiri":
                    x, y = vis - 1 - r, pos + c - 17
                else:
                    x, y = W - vis + r, pos + c - 17
                fb.pixel(x, y)


def _draw_fade(fb, F):
    """Menghilang mendadak: wajah memudar dengan pola titik sambil mengecil sedikit; u 0 = penuh, 1 = hilang."""
    c = F.cat
    _draw_face(_FACE, F)
    fb.clear()
    fb.ox = fb.oy = 0
    _draw_dust(fb, F)
    a = 1 - _smooth(0, 1, c["u"])
    if a > 0:
        _blit_scaled(fb, _FACE, 64, 32, 1.0 - 0.12 * c["u"], a)


def _draw_scene_cat(fb, F):
    c = F.cat
    if c["mode"] == "morph":
        _draw_morph(fb, F)
        return
    if c["mode"] == "fade":
        _draw_fade(fb, F)
        return
    fb.clear()
    fb.ox = fb.oy = 0
    if c["ground"]:  # tanah bergulir mundur: kesan berlari
        for x in range(W):
            if (x + c["scroll"]) % 10 < 5:
                fb.pixel(x, 55)
    _draw_dust(fb, F)
    if c["mode"] in ("sprite", "play"):
        _cat_sprite(c["ph"], c["pose"])
        fb.blit(_CAT, rnd(c["x"]) - SPR_W // 2, SPR_Y0 + rnd(c.get("y", 0.0)), c["flip"])
        toy = c.get("toy")
        if toy and not toy["hidden"]:  # tikus mainan
            _mouse_sprite(toy["ph"])
            fb.blit(_MOUSE, rnd(toy["x"]) - 6, 48, toy["dir"] < 0)
        for zx, zy, big in c.get("zs", []):
            fb.char(rnd(zx), rnd(zy), "Z" if big else "z", 2 if big else 1)
    elif c["mode"] == "dots" and c["rise"] > 0.6:  # "..." menunggu
        for i in range(int(c["rise"] * 1.5) % 4):
            fb.fill_rect(54 + i * 9, 30, 3, 3)
    elif c["mode"] == "peekedge":  # mengintip dari atas / kiri / kanan / bawah layar
        _draw_peek(fb, c["edge"], c["vis"], c["pos"], c["look"])
    elif c["mode"] == "peek":  # kepala muncul dari tepi bawah
        px = rnd(c["x"])
        base = 64 - rnd(15 * c["rise"])
        fb.fill_round_rect(px - 15, base, 30, 24, 9)
        fb.fill_triangle(px - 14, base + 4, px - 12, base - 5, px - 6, base)
        fb.fill_triangle(px + 14, base + 4, px + 12, base - 5, px + 6, base)
        lk = rnd(c["look"] * 2)
        fb.fill_round_rect(px - 8 + lk, base + 6, 4, 8, 1, 0)
        fb.fill_round_rect(px + 4 + lk, base + 6, 4, 8, 1, 0)


def draw_face(fb, F):
    """Satu gambar penuh: wajah, atau adegan kucing 8-bit saat kabur / main."""
    if F.cat is None:
        _draw_face(fb, F)
    else:
        _draw_scene_cat(fb, F)


# ---------------------------------------------------------------- logika Pet: pegas + jadwal kegiatan (waktu dalam ms)


class Sp:
    """Pegas: nilai mengejar target dengan sedikit pantulan (lebih hidup daripada ganti nilai langsung)."""
    __slots__ = ("x", "t", "v", "k", "c")

    def __init__(self, x, k, c):
        self.x = self.t = x
        self.v, self.k, self.c = 0.0, k, c

    def step(self, dt):
        self.v += (self.k * (self.t - self.x) - self.c * self.v) * dt
        self.x += self.v * dt


BASE = dict(w=22, h=22, r=11, lt=0, tilt=0, lb=0, pw=8, ph=12, curve=0, wflat=0, wlen=1.0,
            mc=2.2, mo=0, mw=5, tongue=0, wh=0, amp=0.35, earL=0.0, earR=0.0, htilt=0.0, blush=0.0)
PRESETS = {
    "normal": {},
    "bosan": dict(h=21, lt=0.42, tilt=0.6, pw=5, ph=9, mc=1.2, mw=5, wh=-1, amp=0.15, earL=0.35, earR=0.35, wlen=1.05),
    "tidur": dict(h=3, r=1, curve=1, pw=0, ph=0, mc=2.2, wh=-1, amp=0.1, earL=0.45, earR=0.45),
    "senang": dict(w=24, r=9, lb=0.6, pw=0, ph=0, mc=3, mo=0.55, mw=9, wh=1, amp=0.8, blush=1.0),
    "purr": dict(w=24, r=9, lb=0.68, pw=0, ph=0, mc=3, mo=0.15, mw=6, wh=1, amp=1.0, earL=0.3, earR=0.3, blush=1.0),
    "terkejut": dict(w=26, h=28, r=13, pw=4, ph=4, mc=0, mo=0.5, mw=7, wflat=1, amp=0.1, wlen=0.85),
    "geli": dict(h=22, pw=0, ph=0, mc=1, mo=0.9, mw=12, wh=1, amp=1.6, earL=0.2, earR=0.2, blush=1.0),
    "yawn": dict(h=3, r=1, pw=0, ph=0, mc=0, mo=1.0, mw=10, wh=-1, amp=0.1, earL=0.3, earR=0.3),
    "kesal": dict(h=21, lt=0.38, tilt=-0.8, pw=3, ph=10, mc=-2.2, mw=5, amp=1.5, earL=1.0, earR=1.0, wlen=0.9),
    "main": dict(w=24, h=25, r=12, pw=10, ph=14, wh=1, amp=1.2, wlen=0.85),
    "makan_tunggu": dict(w=24, h=26, r=12, pw=10, ph=14, mc=2, mw=6, wh=1, amp=0.8, wlen=0.85),
    "makan": dict(w=24, r=9, lb=0.5, pw=0, ph=0, mc=3, mo=0.1, mw=8, wh=1, amp=1.0, earL=0.15, earR=0.15, blush=0.6),
    "kenyang": dict(w=24, r=9, lb=0.6, pw=0, ph=0, mc=3, mo=0, mw=8, wh=1, amp=0.6, earL=0.3, earR=0.3, blush=1.0),
    "groom": dict(h=3, r=1, pw=0, ph=0, mc=2, mo=0, mw=6, wh=0, amp=0.3, earL=0.3, earR=0.3),
    "bird": dict(w=24, h=25, r=12, pw=10, ph=14, mc=2, mw=6, wh=0.5, amp=1.0, wlen=0.8),
}
SPRING = {  # (kekakuan, redaman): redaman < 2*sqrt(k) = sedikit memantul
    **{n: (120, 14) for n in ("w", "h", "r", "lt", "tilt", "lb", "pw", "ph", "curve", "wflat", "wlen")},
    **{n: (110, 13) for n in ("mc", "mo", "mw", "tongue")},
    "wh": (90, 12), "amp": (40, 10), "gx": (260, 22), "gy": (260, 22), "fy": (180, 9),
    "earL": (220, 16), "earR": (220, 16), "htilt": (150, 14), "blush": (100, 14),
}
HOLD_MS, PURR_SLEEP_MS = 500, 3500
HAPPY_MS, GELI_MS, SURPRISE_MS, LOOK_MS = 1800, 2000, 2000, 900
GAZES = [(-1, 0), (1, 0), (0, -0.8), (-0.8, -0.6), (0.8, -0.6), (0.6, 0.7), (-0.6, 0.7), (0, 0), (0, 0)]
EVENT_MS = {"wink": 380, "blep": 1000, "sniff": 800, "kepo": 1100, "yawn": 1800, "slowblink": 1200,
            "tilt": 1600, "groom": 3800, "bird": 3600}
EVENTS = ["wink", "blep", "sniff", "kepo", "slowblink", "slowblink", "tilt", "tilt", "groom", "bird", "bird"]
MORPH_MS = 800
FEED_FALL, FEED_CHOMP, FEED_GAP, FEED_MAX = 700, 1000, 300, 3   # kasih makan: ikan jatuh, kunyah, jeda; 3 ikan = kenyang
ESC_SIT = 300
ESC_KINDS = {
    "kabur": dict(name="kabur", pre=700, pre_preset="kesal", l1="kesal", morph=800, run=1100, away=(3500, 5500),
                  peek=2600, back=2300, back_pose="walk"),
    "zoomies": dict(name="zoomies", pre=450, pre_preset="main", l1="siap", morph=500, run=900, away=(300, 300),
                    peek=0, back=1000, back_pose="run"),
    # kegiatan badan penuh yang muncul sendiri saat tidak ada tombol ditekan (script = fungsi _sc_*)
    "jalan": dict(name="jalan", script="jalan", dur=7500, morph=700, happy=False),
    "ekor": dict(name="ekor", script="ekor", dur=4600, morph=700, happy=False),
    "stretch": dict(name="stretch", script="stretch", dur=4300, morph=700, happy=False),
    "garuk": dict(name="garuk", script="garuk", dur=3400, morph=700, happy=False),
    "loaf": dict(name="loaf", script="loaf", dur=10 ** 9, morph=900, happy=False),   # sampai ada tombol ditekan
    # tiba-tiba menghilang (pergi main), lalu mengintip dari tepi layar, lalu muncul lagi
    "ngumpet": dict(name="ngumpet", script="ngumpet", fade=True, morph=350, happy=True),
}
BODY_KINDS = ["jalan", "ekor", "stretch", "garuk", "main_sendiri", "ngumpet", "ngumpet"]
STEP_MS = 1000 / 60


def _blink_value(dt, hold=0):
    """0..1 menurut waktu sejak kedip dimulai: menutup 70 ms, tertutup `hold`, membuka 110 ms."""
    if dt < 0:
        return 0.0
    if dt < 70:
        return dt / 70
    if dt < 70 + hold:
        return 1.0
    dt -= 70 + hold
    return max(0.0, 1 - dt / 110)


def _slow_blink(e):
    """Kedip lambat (tanda percaya pada kucing): menutup 500 ms, tahan 200 ms, membuka 500 ms."""
    if e < 0 or e >= 1200:
        return 0.0
    if e < 500:
        return _smooth(0, 1, e / 500)
    if e < 700:
        return 1.0
    return _smooth(0, 1, 1 - (e - 700) / 500)


def _groom_paw(e):
    """Mencuci muka: kaki naik ke mulut, menjilat, mengusap pipi/mata berputar, lalu turun. -> (x, y, menjilat) / None"""
    if e < 0 or e > 3800:
        return None
    if e < 600:
        u = _smooth(0, 1, e / 600)
        return 96 + (76 - 96) * u, 72 + (50 - 72) * u, False
    if e < 1500:
        j = e - 600
        return 76 + math.sin(j / 60) * 1.5, 50 + math.sin(j / 45) * 1.5, True
    if e < 3200:
        a = (e - 1500) / 1000 * 2 * math.pi * 1.4 + math.pi / 2
        wx, wy = 90 + 7 * math.cos(a), 28 + 9 * math.sin(a)
        u = _smooth(0, 1, (e - 1500) / 300)
        return 76 + (wx - 76) * u, 50 + (wy - 50) * u, False
    u = _smooth(0, 1, (e - 3200) / 600)
    a = (3200 - 1500) / 1000 * 2 * math.pi * 1.4 + math.pi / 2
    wx, wy = 90 + 7 * math.cos(a), 28 + 9 * math.sin(a)
    return wx + (98 - wx) * u, wy + (72 - wy) * u, False


# permainan (tahan DOWN): tikus mainan berlari-lari, kucing mengendap -> goyang pantat -> terkam -> menangkap -> mengejar
PLAY_LABELS = {"watch": "main: mengamati", "stalk": "main: mengendap", "freeze": "main: membeku",
               "wiggle": "main: goyang pantat", "pounce": "main: terkam!", "catch": "main: menangkap",
               "miss": "main: meleset", "chase": "main: mengejar", "rest": "main: istirahat"}


class Pet:
    def __init__(self, now, bored_after=15000, sleep_after=30000, rng=random, lively=True, kabur_prob=0.35,
                 zoom_prob=0.04, body_prob=0.3, loaf_prob=0.5, sleep_min=30000, sleep_max=180000):
        self.rng, self.bored_after, self.sleep_after, self.lively = rng, bored_after, sleep_after, lively
        self.kabur_prob, self.zoom_prob = kabur_prob, zoom_prob  # peluang pergi main (ngumpet/zoomies, TIDAK kesal) saat mulai bosan / zoomies per kegiatan
        self.sp = {n: Sp(BASE.get(n, 0.0), *SPRING[n]) for n in SPRING}
        self.t = float(now)  # waktu simulasi (ms), maju per 1/60 dtk
        self.last_input = now
        self.down = {}  # tombol -> waktu ditekan
        self.happy_until = self.geli_until = self.surprise_until = self.look_until = 0
        self.look_gx, self.ok_times = 0, []
        self.blink_at = now + rng.randint(2500, 5000)
        self.blink2 = False
        self.ev_kind, self.ev_start, self.ev_until, self.ev_side, self.ev_dir = None, 0, 0, "L", 1
        self.next_ev = now + rng.randint(3000, 6000)
        self.gz_next, self.gz = now + 1500, (0, 0)
        self.drift_at, self.drift = now, (0.0, 0.0)
        self.twitch_at, self.twitch_until, self.twitch_side = now + rng.randint(4000, 8000), 0, "L"
        self.ear_at, self.dream_at, self.dream_until = now + rng.randint(2000, 5000), 0, 0
        self.hearts, self.heart_at = [], 0
        self.esc, self.dust, self.dust_at, self.cat_ph, self.prev_mood = None, [], 0, 0.0, "normal"
        self.morph, self.morph_target, self.scene, self.pl = 0.0, 0.0, None, None  # wajah (0) <-> kucing 8-bit (1)
        self.cat_x, self.cat_dir, self.cat_y = 64.0, 1, 0.0
        self.feed_t0, self.feed_gy, self.chew_k, self.crumbs, self.hearty = None, 0.0, -1, [], False
        self.yawn_rolled = -1  # menguap saat bosan: kadang langsung mengantuk dan tidur
        self.sleep_min, self.sleep_max = sleep_min, sleep_max  # lama tidur acak; lalu bangun sendiri
        self.sleep_t0, self.sleep_dur = 0.0, 0.0
        self.pending_groom = self.pending_zoom = 0  # setelah makan: mencuci muka; setelah main: kadang zoomies
        self.mood_name, self.label = "normal", "normal"
        self.stats = defaultdict(int)
        self.body_prob, self.loaf_prob, self.auto_play_until = body_prob, loaf_prob, 0  # kegiatan badan penuh sendiri
        self.pose_cur = {}
        self._reset_pose()

    def _reset_pose(self):
        self.pose_cur = {k: POSES["sit"].get(k, v) for k, v in DEFAULTS.items()}

    def _ease_pose(self, target, dt, tau=0.12):
        """Parameter pose (merunduk, kepala, telinga, ekor, ...) berubah halus, bukan loncat antar pose."""
        k = 1 - math.exp(-dt / tau)
        for key, dflt in DEFAULTS.items():
            self.pose_cur[key] += (target.get(key, dflt) - self.pose_cur[key]) * k
        P = dict(target)
        P.update(self.pose_cur)
        return P

    # --- masukan
    def press(self, now, btn):
        """btn: 0 = UP (kiri), 1 = DOWN (kanan), 2 = OK."""
        self.advance(now)
        self.last_input = now
        self.auto_play_until = 0  # tombol ditekan: main sendiri berhenti
        self.down[btn] = now
        if btn in (0, 1):  # ketukan: melirik ke arah tombol sambil memiringkan kepala (penasaran)
            self.look_until, self.look_gx = now + LOOK_MS, (-1 if btn == 0 else 1)

    def release(self, now, btn):
        self.advance(now)
        t0 = self.down.pop(btn, None)
        if t0 is None:
            return
        held = now - t0
        self.last_input = now
        if btn != 2:
            return
        if held >= PURR_SLEEP_MS:  # dielus sampai ketiduran: bangun sambil menguap
            self._start_event("yawn", now)
        elif held >= HOLD_MS:      # elus panjang selesai: masih senang sebentar
            self.happy_until = now + 1000
        else:                      # ketukan
            if self.esc:           # dipanggil pulang saat kabur
                self._call(now)
                return
            self.ok_times = [t for t in self.ok_times if now - t < 2500] + [now]
            if len(self.ok_times) >= 6:  # digelitik terus-menerus: kabur
                self.start_escape(now)
            elif sum(1 for t in self.ok_times if now - t < 1500) >= 3:  # dielus cepat berkali-kali = geli
                self.geli_until = now + GELI_MS
            else:
                self.happy_until = now + HAPPY_MS
                self.sp["fy"].v -= 70    # melompat kecil
                self.heart_at = now

    def notify(self, now):
        self.advance(now)
        self.surprise_until = now + SURPRISE_MS
        self.last_input = now
        self.sp["fy"].v -= 110
        self.ev_until = 0

    def sleep_now(self, now):
        self.advance(now)
        self.last_input = now - self.sleep_after

    def trigger(self, kind, now):
        """Paksa satu kegiatan (untuk tes & gambar contoh)."""
        self.advance(now)
        if kind in ESC_KINDS:
            self.start_escape(now, kind)
        else:
            self._start_event(kind, now)

    # --- keadaan
    def _held(self, btn, t):
        t0 = self.down.get(btn)
        return t - t0 if t0 is not None else -1

    def mood(self, t):
        if self.esc:
            return "kabur"
        if self.scene == "main":
            return "main"
        if self._held(0, t) >= HOLD_MS:
            return "makan"
        ok = self._held(2, t)
        if t < self.geli_until:
            return "geli"
        if ok >= PURR_SLEEP_MS:
            return "tidur"
        if ok >= HOLD_MS:
            return "purr"
        if t < self.happy_until:
            return "senang"
        if t < self.surprise_until:
            return "terkejut"
        idle = t - self.last_input
        return "tidur" if idle >= self.sleep_after else "bosan" if idle >= self.bored_after else "normal"

    def _start_event(self, kind, t):
        rng = self.rng
        self.ev_kind, self.ev_start, self.ev_until = kind, t, t + EVENT_MS[kind]
        self.ev_side, self.ev_dir = rng.choice("LR"), rng.choice((-1, 1))
        self.stats[kind] += 1

    # --- kabur / zoomies: pet berubah mulus jadi kucing 8-bit, lari keluar layar, (mengintip), pulang, jadi wajah lagi
    def start_escape(self, t, kind="kabur"):
        if self.esc:
            return
        rng, k = self.rng, ESC_KINDS[kind]
        peeks, total = [], 0
        if kind == "ngumpet":  # 3-4 intipan dari sisi layar yang berbeda-beda
            total = 900
            for edge in rng.sample(["atas", "kiri", "kanan", "bawah"], rng.randint(3, 4)):
                hold = rng.randint(900, 1700)
                pos = rng.randint(30, 98) if edge in ("atas", "bawah") else rng.randint(18, 46)
                peeks.append(dict(edge=edge, t0=total, dur=450 + hold + 450, hold=hold, pos=pos))
                total += 450 + hold + 450 + rng.randint(700, 1500)
        b = dict(poof=k.get("pre", 0))
        b["run"] = b["poof"] + k["morph"]
        if k.get("script"):  # kegiatan berskrip: berubah jadi kucing -> skrip -> berubah balik jadi wajah
            b["gone"] = b["run"] + (total if kind == "ngumpet" else k["dur"])
            b["end"] = b["gone"] + k["morph"]
        else:
            b["gone"] = b["run"] + k["run"]
            b["peek"] = b["gone"] + rng.randint(*k["away"])
            b["ret"] = b["peek"] + k["peek"]
            b["sit"] = b["ret"] + k["back"]
            b["unm"] = b["sit"] + ESC_SIT
            b["end"] = b["unm"] + k["morph"]
        self.esc = dict(t0=t, dir=rng.choice((-1, 1)), back=rng.choice((-1, 1)), peek_x=rng.randint(30, 98), b=b, k=k, peeks=peeks,
                        sleep_dur=rng.uniform(self.sleep_min, self.sleep_max) if kind == "loaf" else 0.0)
        self.cat_dir = self.esc["dir"]
        self._reset_pose()
        self.dust, self.dust_at, self.ok_times = [], 0, []
        if kind == "kabur":
            self.last_input = t  # hanya kabur (karena diganggu) yang dianggap interaksi; kegiatan sendiri tidak
        self.stats[kind] += 1

    def _call(self, t):
        """OK diketuk saat kabur: kalau sudah di luar layar / mengintip, pet langsung pulang."""
        esc = self.esc
        b, e = esc["b"], t - esc["t0"]
        if esc["k"]["name"] == "kabur" and b["gone"] + 600 <= e < b["ret"]:
            b["peek"], b["ret"] = min(b["peek"], e), e
            b["sit"] = e + esc["k"]["back"]
            b["unm"] = b["sit"] + ESC_SIT
            b["end"] = b["unm"] + esc["k"]["morph"]

    def _escape(self, t):
        """-> (adegan kucing atau None, preset wajah, label)"""
        esc, rng = self.esc, self.rng
        b, k, e, d, dt = esc["b"], esc["k"], t - esc["t0"], esc["dir"], STEP_MS / 1000
        name = k["name"]
        if e >= b["end"]:  # sudah jadi wajah lagi
            happy = k.get("happy", True)
            self.esc, self.dust, self.morph = None, [], 0.0
            if k["name"] in ("kabur", "loaf"):
                self.last_input = t
            if k["name"] == "loaf":
                self._start_event("yawn", t)  # bangun tidur: menguap
            if happy:
                self.happy_until, self.heart_at = t + HAPPY_MS, t
                self.sp["fy"].v -= 90
            return None, ("senang" if happy else "normal"), "pulang"
        if k.get("script"):
            return self._activity(t, e, k, b, d)
        self.dust = [q for q in self.dust if t - q[2] < 450]

        def puff(x):
            self.dust.append([x + rng.uniform(-2, 2), 52 - rng.randint(0, 4), t])

        cat = dict(mode="none", x=64.0, ph=0.0, flip=d < 0, ground=False, scroll=0, rise=0.0, look=0.0, y=0.0,
                   pose=POSES["sit"])
        if e < b["poof"]:
            self.morph = 0.0
            return None, k["pre_preset"], f"{name}: {k['l1']}"
        if e < b["run"]:  # wajah mengecil jadi kepala, badan tumbuh
            u = (e - b["poof"]) / k["morph"]
            self.morph = u
            if u > 0.85 and not self.dust:
                for j in range(4):
                    puff(55 + j * 7)
            cat.update(mode="morph", u=u)
            return cat, k["pre_preset"], f"{name}: berubah jadi kucing"
        self.morph = 1.0
        if e < b["gone"]:  # lari makin cepat
            tau = (e - b["run"]) / 1000
            x = 64 + d * 70 * tau * tau
            if t - self.dust_at >= 90:
                self.dust_at = t
                puff(x - d * 16)
            self.cat_ph += min(31.0, 10 + 21 * tau) * dt
            cat.update(mode="sprite", x=x, ph=self.cat_ph, ground=True, scroll=-int(x), pose=POSES["run"])
            return cat, "normal", f"{name}: lari"
        if e < b["peek"]:
            cat.update(mode="dots", rise=(e - b["gone"]) / 1000)
            return cat, "normal", f"{name}: sepi"
        if e < b["ret"]:
            cat.update(mode="peek", rise=min(1.0, (e - b["peek"]) / 500), x=esc["peek_x"],
                       look=math.sin((e - b["peek"]) / 450))
            return cat, "normal", f"{name}: mengintip"
        bd, run = esc["back"], k["back_pose"] == "run"
        if e < b["sit"]:  # kembali, melambat sampai berhenti di tengah
            u = (e - b["ret"]) / k["back"]
            x0 = -20 if bd > 0 else 148
            x = x0 + (64 - x0) * (1 - (1 - u) ** 2)
            vel = abs(64 - x0) * 2 * (1 - u) / (k["back"] / 1000)
            if t - self.dust_at >= 90 and run:
                self.dust_at = t
                puff(x - bd * 16)
            self.cat_ph += min(31.0 if run else 14.0, 4 + vel * (0.12 if run else 0.08)) * dt
            cat.update(mode="sprite", x=x, ph=self.cat_ph, flip=bd < 0, ground=True, scroll=-int(x),
                       pose=POSES[k["back_pose"]])
            return cat, "normal", f"{name}: pulang"
        self.cat_ph += 3 * dt
        if e < b["unm"]:  # duduk, ekor bergoyang
            cat.update(mode="sprite", x=64.0, ph=self.cat_ph, flip=bd < 0, pose=POSES["sit"])
            return cat, "senang", f"{name}: duduk"
        u = 1 - (e - b["unm"]) / k["morph"]  # kucing mengecil jadi kepala -> wajah membesar
        self.morph = u
        cat.update(mode="morph", u=u, flip=bd < 0)
        return cat, "senang", f"{name}: jadi wajah lagi"

    # --- kegiatan sendiri (tanpa tombol): jalan-jalan, kejar ekor, peregangan, garuk, tidur melingkar
    def _activity(self, t, e, k, b, d):
        name, dt, esc, fade = k["name"], STEP_MS / 1000, self.esc, k.get("fade")
        self.dust = [q for q in self.dust if t - q[2] < 450]
        cat = dict(mode="none", x=64.0, ph=self.cat_ph, flip=self.cat_dir < 0, ground=False, scroll=0, rise=0.0,
                   look=0.0, y=0.0, pose=POSES["sit"])
        if (name == "loaf" and not esc.get("woke") and e >= b["run"]
                and (self.last_input > esc["t0"] + 300 or e - b["run"] >= esc["sleep_dur"])):
            esc["woke"] = True  # ada tombol ditekan, atau cukup tidur (paling lama sleep_max): bangun
            b["gone"], b["end"] = e, e + k["morph"]
        if e < b["run"]:  # wajah mengecil jadi kepala + badan tumbuh / atau menghilang mendadak (fade)
            self.morph = e / k["morph"]
            if fade:
                if self.morph > 0.4 and not self.dust:  # kepulan debu saat menghilang
                    for j in range(5):
                        self.dust.append([50 + j * 7 + self.rng.uniform(-2, 2), 32 + self.rng.randint(-8, 8), t])
                cat.update(mode="fade", u=self.morph)
                return cat, "normal", f"{name}: menghilang"
            cat.update(mode="morph", u=self.morph)
            return cat, "normal", f"{name}: berubah jadi kucing"
        if e >= b["gone"]:  # kembali jadi wajah
            self.morph = 1 - (e - b["gone"]) / k["morph"]
            if fade:
                cat.update(mode="fade", u=self.morph)
                return cat, "senang", f"{name}: muncul lagi"
            cat.update(mode="morph", u=self.morph)
            return cat, "normal", f"{name}: jadi wajah lagi"
        self.morph = 1.0
        if k["script"] == "ngumpet":
            return self._peek_scene(e - b["run"], cat, esc)
        label, pose, upd = getattr(self, "_sc_" + k["script"])(e - b["run"], d, dt)
        cat.update(mode="sprite", ground=True, pose=self._ease_pose(pose, dt), **upd)
        return cat, "normal", label

    def _peek_scene(self, s, cat, esc):
        """Layar kosong; kepala kucing muncul dari satu tepi layar, melihat-lihat, lalu bersembunyi lagi."""
        for pk in esc["peeks"]:
            u = s - pk["t0"]
            if 0 <= u < pk["dur"]:
                if u < 450:
                    rise = _smooth(0, 1, u / 450)
                elif u < 450 + pk["hold"]:
                    rise = 1.0
                else:
                    rise = _smooth(0, 1, 1 - (u - 450 - pk["hold"]) / 450)
                cat.update(mode="peekedge", edge=pk["edge"], pos=pk["pos"], vis=rnd(rise * 22), look=math.sin(u / 260))
                return cat, "normal", f"ngumpet: mengintip {pk['edge']}"
        return cat, "normal", "ngumpet: sepi"

    def _walk_rate(self, u):
        return 6 + 6 * math.sin(math.pi * max(0.0, min(1.0, u)))   # langkah melambat di awal & akhir

    def _sc_jalan(self, s, d, dt):
        """Patroli: duduk sebentar, jalan ke satu sisi, duduk menoleh kiri-kanan, jalan kembali ke tengah, duduk."""
        side = 64 + d * 44
        if s < 500:
            x, pose, lab = 64.0, "sit", "jalan: duduk"
            self.cat_ph += 3 * dt
        elif s < 2500:
            u = (s - 500) / 2000
            x, pose, lab, self.cat_dir = 64 + (side - 64) * _smooth(0, 1, u), "walk", "jalan: berjalan", d
            self.cat_ph += self._walk_rate(u) * dt
        elif s < 4700:
            x, pose, lab = side, "sit", "jalan: duduk melihat sekeliling"
            self.cat_dir = d if ((s - 2500) // 800) % 2 == 0 else -d
            self.cat_ph += 3 * dt
        elif s < 6700:
            u = (s - 4700) / 2000
            x, pose, lab, self.cat_dir = side + (64 - side) * _smooth(0, 1, u), "walk", "jalan: kembali", -d
            self.cat_ph += self._walk_rate(u) * dt
        else:
            x, pose, lab = 64.0, "sit", "jalan: duduk"
            self.cat_ph += 3 * dt
        return lab, POSES[pose], dict(x=x, flip=self.cat_dir < 0, ph=self.cat_ph)

    def _sc_ekor(self, s, d, dt):
        """Melihat ekornya lalu berputar mengejarnya, akhirnya pusing."""
        self.cat_ph += 3 * dt
        x, y, P = 64.0, 0.0, POSES["sit"]
        lab = "ekor: melihat ekor"
        if 600 <= s < 3400:
            P, lab = POSES["spin"], "ekor: berputar"
            self.cat_dir = d if int((s - 600) / 230) % 2 == 0 else -d
            self.cat_ph += 14 * dt
            x = 64 + math.sin((s - 600) / 115) * 2
            y = -abs(math.sin((s - 600) / 230 * math.pi)) * 2
        elif s >= 3400:
            P, lab = dict(POSES["dizzy"]), "ekor: pusing"
            P["head_dy"] = 3 + math.sin(s / 90) * 1.5
        return lab, P, dict(x=x, y=y, flip=self.cat_dir < 0, ph=self.cat_ph)

    def _sc_stretch(self, s, d, dt):
        """Peregangan: menunduk dengan pantat naik, lalu punggung melengkung sambil menguap."""
        self.cat_ph += 3 * dt
        self.cat_dir = d
        pose, lab = (("sit", "stretch: duduk") if s < 500 else ("stretch1", "stretch: menunduk") if s < 2000
                     else ("arch", "stretch: punggung melengkung") if s < 3200 else ("sit", "stretch: duduk"))
        return lab, POSES[pose], dict(x=64.0, flip=d < 0, ph=self.cat_ph)

    def _sc_garuk(self, s, d, dt):
        """Duduk, kaki belakang menggaruk telinga dengan mata terpejam."""
        self.cat_ph += 3 * dt
        self.cat_dir = d
        if 400 <= s < 2900:
            P, lab = dict(POSES["scratch"]), "garuk: menggaruk telinga"
            P["scratch"] = math.sin(s / 55)
        else:
            P, lab = POSES["sit"], "garuk: duduk"
        return lab, P, dict(x=64.0, flip=d < 0, ph=self.cat_ph)

    def _sc_loaf(self, s, d, dt):
        """Tidur melingkar (loaf): bernapas pelan, z melayang, sampai ada tombol ditekan."""
        self.cat_ph += dt
        self.cat_dir = d
        if s < 600:  # mengantuk sambil duduk, lalu merebahkan diri
            P = dict(POSES["sit"])
            P["eyes_closed"] = True
        else:
            P = dict(POSES["loaf"])
            P["bob"] = math.sin(s / 480) * 0.9
        zs = []
        for i, off in enumerate((0, 1300)):
            ph = ((s + off) % 2600) / 2600
            zs.append((64 + 8 + ph * 10 + i * 3, 32 - ph * 22, i == 1))
        return "loaf: tidur melingkar", P, dict(x=64.0, flip=d < 0, ph=self.cat_ph, zs=zs)

    # --- ajak main: tahan DOWN -> wajah berubah jadi kucing 8-bit; tikus mainan, mengendap, goyang pantat, terkam
    def _play(self, t, dt):
        rng = self.rng
        u = self.morph
        if u < 1.0:
            going = self.morph_target >= 1.0
            if not going:
                self.cat_y = 0.0
            cat = dict(mode="morph", u=u, x=self.cat_x, y=self.cat_y, flip=self.cat_dir < 0)
            return cat, ("main" if going else "senang"), ("main: berubah jadi kucing" if going else "main: kembali jadi wajah")
        if self.pl is None:  # baru selesai berubah: tikus masuk dari sisi layar
            self._reset_pose()
            side = rng.choice((-1, 1))
            self.pl = dict(st="watch", t0=t, mx=64 + side * 52.0, mv=0.0, mdir=-side, mtarget=64 + side * 30.0,
                           mpause=t + 700, mspeed=75.0, pinned=False, hidden=False, ph=0.0, z=None, wig=0)
        pl = self.pl
        # --- tikus: berlari pendek-pendek lalu membeku (seperti mangsa), kecuali sedang ditangkap
        if not pl["pinned"]:
            if t < pl["mpause"]:
                pl["mv"] = 0.0
            else:
                dist = pl["mtarget"] - pl["mx"]
                if abs(dist) < 2:
                    pl["mpause"] = t + rng.randint(600, 1700)
                    far = [x for x in (rng.uniform(14, 114) for _ in range(4)) if abs(x - pl["mx"]) > 24]
                    pl["mtarget"] = far[0] if far else min(114.0, max(14.0, 128 - pl["mx"]))
                    pl["mspeed"] = 75.0
                    pl["mv"] = 0.0
                else:
                    pl["mdir"] = 1 if dist > 0 else -1
                    pl["mv"] = pl["mspeed"] * pl["mdir"]
                    pl["mx"] = max(10.0, min(118.0, pl["mx"] + pl["mv"] * dt))
            pl["ph"] += abs(pl["mv"]) * dt * 0.06
        mx, cx, dist = pl["mx"], self.cat_x, pl["mx"] - self.cat_x
        sdir = 1 if dist > 0 else -1
        st, e = pl["st"], t - pl["t0"]

        def go(new):
            pl["st"], pl["t0"] = new, t

        pose, label = "alert", None
        if st == "watch":  # mengamati dulu dengan telinga tegak, pupil membesar
            self.cat_dir = sdir
            if e > 700 and abs(dist) > 30:
                go("stalk")
        elif st in ("stalk", "freeze"):  # mengendap rendah; membeku bila tikus bergerak
            self.cat_dir = sdir
            if abs(pl["mv"]) > 5:
                pose, st2 = "freeze", "freeze"
            else:
                pose, st2 = "stalk", "stalk"
                self.cat_x = max(20.0, min(108.0, cx + sdir * 20 * dt))
                self.cat_ph += 2.2 * dt
            pl["st"] = st2
            if abs(mx - self.cat_x) <= 30:
                go("wiggle")
        elif st == "wiggle":  # goyang pantat sebelum menerkam
            pose = "wiggle"
            self.cat_dir = sdir
            self.cat_ph += 45 * dt
            if abs(dist) > 44:
                go("stalk")
            elif e > 900:
                pl["z"] = (self.cat_x, mx - sdir * 5)
                go("pounce")
        elif st == "pounce":  # melompat: lintasan parabola ke arah tikus
            pose = "pounce"
            f = min(1.0, e / 450)
            x0, x1 = pl["z"]
            self.cat_x = x0 + (x1 - x0) * f
            self.cat_y = -15 * 4 * f * (1 - f)
            if f >= 1.0:
                self.cat_y = 0.0
                if abs(pl["mx"] - self.cat_x) < 14 and rng.random() < 0.8:
                    pl["pinned"] = True
                    go("catch")
                else:
                    pl["mspeed"] = 130.0   # meleset: tikus kabur, kucing menyesal sebentar
                    pl["mpause"] = 0
                    go("miss")
        elif st == "miss":
            pose = "sit"
            if e > 600:
                go("stalk")
        elif st == "catch":  # menggigit tikus, kepala menunduk
            pose = "catch"
            pl["hidden"] = e > 350
            if e > 1300:
                pl["pinned"], pl["hidden"] = False, False
                pl["mx"], pl["mdir"] = self.cat_x + sdir * 6, sdir
                pl["mtarget"] = 114.0 if sdir > 0 else 14.0   # tikus dilepas lalu kabur
                pl["mspeed"], pl["mpause"] = 140.0, 0
                go("chase")
        elif st == "chase":  # mengejar sebentar (kucing tidak mengejar lama) lalu berhenti
            pose = "run"
            self.cat_dir = sdir
            self.cat_x = max(20.0, min(108.0, cx + sdir * 80 * dt))
            self.cat_ph += 24 * dt
            if e > 650 or abs(dist) < 8:
                go("rest")
        elif st == "rest":
            pose = "sit"
            pl["mspeed"] = 75.0
            if e > 800:
                go("watch")
        self.cat_y = self.cat_y if pl["st"] == "pounce" else 0.0
        label = PLAY_LABELS[pl["st"]]
        toy = dict(x=pl["mx"], dir=pl["mdir"], ph=pl["ph"], hidden=pl["hidden"])
        cat = dict(mode="play", x=self.cat_x, y=self.cat_y, flip=self.cat_dir < 0, ph=self.cat_ph, ground=True,
                   scroll=0, pose=self._ease_pose(POSES[pose], dt), toy=toy)
        return cat, "main", label

    # --- kasih makan: tahan UP -> ikan jatuh ke mulut, dikunyah; 3 ikan = kenyang
    def _feed(self, t):
        """-> (preset wajah, posisi ikan atau None, label, ganti-ganti parameter)"""
        if self.feed_t0 is None:
            self.feed_t0, self.feed_gy, self.chew_k, self.crumbs, self.hearty = t, 0.0, -1, [], False
        e = t - self.feed_t0
        cyc = FEED_FALL + FEED_CHOMP + FEED_GAP
        n, c = int(e // cyc), e % cyc
        if n >= FEED_MAX:
            self.feed_gy, self.hearty = 0.0, True
            return "kenyang", None, "makan: kenyang", {}
        if c < FEED_FALL:
            u = c / FEED_FALL
            self.feed_gy, self.chew_k = -0.9 + 1.3 * u, -1
            return "makan_tunggu", (64.0, -9 + 50 * u * u), "makan: ikan jatuh", {"mo": 0.45 * u}
        self.feed_gy = 0.4
        c -= FEED_FALL
        if c < FEED_CHOMP:
            k = int(c // 160)
            if k != self.chew_k:
                self.chew_k = k
                if k % 2 == 0:
                    self.crumbs += [[64 + self.rng.uniform(-6, 6), 46.0, self.rng.uniform(-35, 35),
                                     -self.rng.uniform(30, 60)] for _ in range(3)]
            return "makan", None, "makan: nyam nyam", {"mo": 0.7 if k % 2 == 0 else 0.1}
        return "makan", None, "makan", {}

    def _pick_kind(self, mood):
        """Pilih kegiatan acak berikutnya: variasi wajah (paling sering) atau kegiatan badan penuh."""
        rng = self.rng
        if mood == "bosan":  # bosan: lebih sering bergerak sendiri
            if rng.random() < self.body_prob * 1.6:
                return rng.choice(["jalan", "ekor", "stretch", "garuk", "ngumpet"])
            return rng.choice(["yawn", "yawn", "groom", "slowblink"])
        if self.zoom_prob and rng.random() < self.zoom_prob:
            return "zoomies"
        if rng.random() < self.body_prob:
            return rng.choice(BODY_KINDS)
        return rng.choice(EVENTS)

    def advance(self, now):
        while self.t + STEP_MS <= now:
            self.t += STEP_MS
            self._step(self.t)

    def _step(self, t):
        rng = self.rng
        dt = STEP_MS / 1000
        if not self.esc:  # tahan DOWN = diajak main (bisa dilepas kapan saja: berubah balik dengan mulus)
            dn = self._held(1, t) >= HOLD_MS or t < self.auto_play_until  # tahan DOWN, atau main sendiri
            if dn and self.scene is None:
                self.scene, self.cat_x, self.cat_y, self.morph_target, self.pl = "main", 64.0, 0.0, 1.0, None
            elif self.scene == "main" and not dn:
                self.morph_target = 0.0
            if self.scene == "main":
                step = dt * 1000 / MORPH_MS
                self.morph += max(-step, min(step, self.morph_target - self.morph))
                if self.morph_target == 0.0 and self.morph <= 0.0:
                    self.scene, self.morph, self.pl = None, 0.0, None
                    self.happy_until, self.heart_at = t + 1200, t
                    if self.zoom_prob and rng.random() < 0.4:  # habis main: kadang zoomies
                        self.pending_zoom = t + 1600
        mood = self.mood(t)
        if mood == "bosan" and self.prev_mood != "bosan" and self.lively and rng.random() < self.kabur_prob:
            self.start_escape(t, rng.choice(["ngumpet", "zoomies"]))  # bosan: pergi main sendiri (bukan kabur kesal)
            mood = "kabur"
        if (mood == "tidur" and self.prev_mood != "tidur" and self.lively and not self.down
                and rng.random() < self.loaf_prob):
            self.start_escape(t, "loaf")  # mengantuk: tidur melingkar
            mood = "kabur"
        if mood == "tidur" and self.prev_mood != "tidur":  # mulai tidur (wajah saja): lamanya acak
            self.sleep_t0, self.sleep_dur = t, rng.uniform(self.sleep_min, self.sleep_max)
        if mood == "tidur" and self.lively and not self.down and t - self.sleep_t0 >= self.sleep_dur:
            self.last_input = t  # cukup tidur: bangun sendiri sambil menguap
            self._start_event("yawn", t)
            mood = "normal"
        if mood == "normal" and t >= self.ev_until:  # kebiasaan setelah makan / main
            if self.pending_groom and t >= self.pending_groom:
                self.pending_groom = 0
                self._start_event("groom", t)
            elif self.pending_zoom and t >= self.pending_zoom:
                self.pending_zoom = 0
                self.start_escape(t, "zoomies")
                mood = "kabur"
        if self.prev_mood == "makan" and mood != "makan":  # selesai makan: puas, lalu mencuci muka
            self.happy_until, self.heart_at, self.feed_t0, self.hearty = t + 1000, t, None, False
            self.pending_groom = t + 1500
        self.prev_mood = mood
        if self.down:
            self.last_input = t  # selama ada tombol ditahan, pet tidak bosan
        tg = dict(BASE)
        fp, cat, fish, elabel, over = mood, None, None, "", {}
        if mood == "kabur":
            cat, fp, elabel = self._escape(t)
        elif mood == "main":
            cat, fp, elabel = self._play(t, dt)
        elif mood == "makan":
            fp, fish, elabel, over = self._feed(t)
        tg.update(PRESETS.get(fp, {}))
        tg.update(over)
        gx = gy = 0.0
        label = elabel or mood

        # kegiatan acak (hanya saat santai, tanpa tombol apa pun)
        if self.lively and mood in ("normal", "bosan") and t >= self.next_ev and t >= self.ev_until:
            kind = self._pick_kind(mood)
            if kind in ESC_KINDS:          # kegiatan badan penuh: berubah jadi kucing 8-bit
                self.start_escape(t, kind)
                mood = self.prev_mood = "kabur"
                cat, fp, elabel = self._escape(t)
                tg.update(PRESETS.get(fp, {}))
                label = elabel
                self.next_ev = t + rng.randint(9000, 16000)
            elif kind == "main_sendiri":   # main sendiri dengan tikus mainan
                self.auto_play_until = t + rng.randint(9000, 14000)
                self.stats["main_sendiri"] += 1
                self.next_ev = self.auto_play_until + rng.randint(8000, 14000)
            else:
                self._start_event(kind, t)
                self.next_ev = self.ev_until + (rng.randint(4000, 8000) if mood == "bosan" else rng.randint(2500, 6000))
        if self.lively and mood in ("normal", "bosan") and t >= self.gz_next:  # melirik ke sekeliling
            self.gz = rng.choice(GAZES)
            self.gz_next = t + rng.randint(700, 2200)
            self.stats["gaze"] += 1
        if self.lively and t >= self.drift_at:  # mata tidak pernah benar-benar diam: bergeser sedikit-sedikit
            self.drift = (rng.uniform(-0.25, 0.25), rng.uniform(-0.2, 0.2))
            self.drift_at = t + rng.randint(300, 900)
        if self.lively and mood not in ("kabur", "main") and t >= self.ear_at:  # telinga berkedut sendiri
            self.sp["earL" if rng.random() < 0.5 else "earR"].v += rng.uniform(5, 8)
            self.ear_at = t + rng.randint(2500, 8000)

        ev = self.ev_kind if t < self.ev_until else None
        if (mood == "bosan" and ev is None and self.ev_kind == "yawn" and self.yawn_rolled != self.ev_start
                and t >= self.ev_until):  # habis menguap: kadang tertidur
            self.yawn_rolled = self.ev_start
            if self.lively and rng.random() < 0.3:
                self.last_input = t - self.sleep_after
        e_t = t - self.ev_start
        sgn = 1 if self.ev_side == "R" else -1
        paw = bird = None
        jaw = False
        bL = bR = 0.0
        allowed = ("yawn", "groom", "slowblink") if mood == "bosan" else EVENTS + ["yawn"]
        if ev and mood in ("normal", "bosan") and ev in allowed:
            if ev == "yawn":
                tg.update(PRESETS["yawn"])
                label = "menguap"
            elif ev == "blep":
                tg["tongue"] = 1
                label = "melet"
            elif ev == "sniff":
                tg.update(amp=1.8, wh=0.4)
                label = "mengendus"
            elif ev == "kepo":
                tg.update(h=25, w=24, r=12, pw=10, ph=14, wlen=0.85, htilt=0.5 * sgn)
                label = "kepo"
            elif ev == "tilt":  # kepala miring: penasaran, satu telinga mengarah ke bunyi
                tg.update(htilt=float(sgn), h=26, earL=0.0 if sgn < 0 else 0.3, earR=0.0 if sgn > 0 else 0.3)
                label = "kepala miring"
            elif ev == "slowblink":
                tg.update(mc=4.5, earL=0.25, earR=0.25)
                bL = bR = _slow_blink(e_t)
                label = "kedip lambat"
            elif ev == "groom":
                tg.update(PRESETS["groom"])
                paw = _groom_paw(e_t)
                if paw and paw[2]:
                    tg["tongue"] = 1
                label = "mencuci muka"
            elif ev == "bird":  # melihat burung: mata mengikuti, mulut mengoceh
                f = e_t / EVENT_MS["bird"]
                bx = -10 + 148 * (f if self.ev_dir > 0 else 1 - f)
                by_ = 5 + math.sin(e_t / 220) * 2.5
                bird = (bx, by_, int(e_t / 90) % 2, self.ev_dir)
                tg.update(PRESETS["bird"])
                jaw = 500 < e_t < 3000 and int(e_t / 90) % 2 == 0
                label = "melihat burung"
        if mood in ("normal", "bosan") and ev not in ("yawn", "groom"):
            gx, gy = (self.gz[0] + self.drift[0], self.gz[1] + self.drift[1]) if self.lively else (0, 0)
            if mood == "bosan":
                gy = max(gy, 0) * 0.5
            if ev == "kepo":
                gx, gy = self.gz[0] or 1, -0.6
            if ev == "bird" and bird:
                gx, gy = max(-1.0, min(1.0, (bird[0] - 64) / 48)), -0.85
        if t < self.look_until and mood in ("normal", "bosan"):  # UP/DOWN diketuk: lirik ke arahnya + miring penasaran
            gx, gy = self.look_gx, 0
            tg["htilt"] = 0.7 * self.look_gx
            label = "lirik kiri" if self.look_gx < 0 else "lirik kanan"
        if mood == "makan":  # mata mengikuti ikan
            gx, gy = 0.0, self.feed_gy

        # kedip: normal dan bosan (lebih lambat)
        if mood in ("normal", "bosan") and ev not in ("yawn", "groom", "slowblink", "bird") and self.lively:
            dtb = t - self.blink_at
            hold = 0 if mood == "normal" else 90
            if dtb >= 0:
                bL = bR = _blink_value(dtb, hold)
                if dtb > 180 + hold:
                    self.stats["blink"] += 1
                    if not self.blink2 and rng.random() < 0.25:  # kedip dua kali
                        self.blink2 = True
                        self.blink_at = t + 60
                    else:
                        self.blink2 = False
                        self.blink_at = t + rng.randint(2500, 5500)
        if ev == "wink" and mood == "normal":
            v = _blink_value(t - self.ev_start, 140)
            if self.ev_side == "L":
                bL = max(bL, v)
            else:
                bR = max(bR, v)
            label = "kedip sebelah"

        # kumis bergerak sendiri; saat tidur: berkedut seperti bermimpi
        if t >= self.twitch_at:
            self.twitch_side, self.twitch_until = rng.choice("LR"), t + 150
            self.twitch_at = t + rng.randint(4000, 8000)
        twitch = self.twitch_side if t < self.twitch_until else None
        if ev == "sniff":
            twitch = "B" if int(t / 110) % 2 else None
        if mood == "tidur" and self.lively:
            if t >= self.dream_at:
                self.dream_until, self.dream_at = t + 350, t + rng.randint(3000, 8000)
                self.sp["earL" if rng.random() < 0.5 else "earR"].v += 6
            if t < self.dream_until:
                twitch = "B" if int(t / 60) % 2 else None

        # hati melayang saat senang / dielus / kenyang; remah makanan jatuh
        shake = (0.0, 0.0)
        if (mood in ("senang", "purr") or self.hearty) and t >= self.heart_at:
            self.heart_at = t + (260 if mood == "purr" else 340)
            self.hearts.append([rng.choice((10, 20, 30, 92, 102, 112)) + rng.random() * 6, 36.0, t])
        self.hearts = [h for h in self.hearts if h[1] > -6]
        for h in self.hearts:
            h[1] -= 20 * dt
            h[0] += math.sin((t - h[2]) / 260) * 0.35
        for q in self.crumbs:
            q[3] += 160 * dt
            q[0] += q[2] * dt
            q[1] += q[3] * dt
        self.crumbs = [q for q in self.crumbs if q[1] < 64]
        knead = None
        if mood == "kabur" and fp == "kesal":
            shake = (math.sin(t / 20) * 1.3, 0.0)
        elif mood == "geli":
            shake = (math.sin(t / 22) * 1.6, 0.0)
        elif mood == "purr":  # mendengkur: getar halus + menyodorkan kepala; kaki depan menguleni
            shake = (math.sin(t / 36) * 0.6 + math.sin(t / 520) * 1.0, 0.0)
            a = math.sin(t / 130)
            knead = (59 - 4 * max(0.0, a), 59 - 4 * max(0.0, -a))

        for n, s in self.sp.items():
            if n == "gx":
                s.t = gx
            elif n == "gy":
                s.t = gy
            elif n == "fy":
                s.t = 0.0
            else:
                s.t = tg[n]
            s.step(dt)

        # napas halus, lebih dalam saat tidur
        breath = math.sin(t / (3000 / (2 * math.pi))) * (1.0 if mood == "tidur" else 0.8)
        zs = []
        if mood == "tidur":  # z melayang naik
            for i, off in enumerate((0, 1200)):
                ph = ((t + off) % 2400) / 2400
                zs.append((102 + ph * 8 + i * 4, 20 - ph * 18, i == 1))
        self.mood_name, self.label = mood, label
        self.F = SimpleNamespace(
            t=t, bL=bL, bR=bR, twitch=twitch, shake=shake, breath=breath, hearts=[(h[0], h[1]) for h in self.hearts],
            nose=(1 if int(t / 80) % 2 else 0) if ev == "sniff" else 0, exclaim=mood == "terkejut",
            geli=mood == "geli", zs=zs, cat=cat, dust=[(q[0], q[1], t - q[2]) for q in self.dust],
            fish=fish, crumbs=[(q[0], q[1]) for q in self.crumbs], paw=paw, knead=knead, bird=bird, jaw=jaw,
            **{n: s.x for n, s in self.sp.items()})

    def frame(self, now):
        self.advance(now)
        if not hasattr(self, "F"):
            self._step(self.t)
        return self.F


# ---------------------------------------------------------------- tampilan (pygame)

SCALE = 6
LIT, DARK, PANEL = (190, 225, 255), (6, 9, 16), (24, 26, 33)


def render(surface, fb, ox, oy, scale=SCALE):
    surface.fill(DARK, (ox, oy, W * scale, H * scale))
    gap = 1 if scale >= 4 else 0
    for y in range(H):
        row = fb.px[y]
        for x in range(W):
            if row[x]:
                surface.fill(LIT, (ox + x * scale, oy + y * scale, scale - gap, scale - gap))


def _settle(pet, now, seconds):
    t = now
    for _ in range(int(seconds * 60)):
        t += STEP_MS
        pet.advance(t)
    return t


def _until(p, t, label, limit=30000):
    """Maju sampai label tertentu muncul. -> (waktu, frame)"""
    end = t + limit
    while t < end:
        t += 33.3
        F = p.frame(t)
        if p.label == label:
            return t, F
    raise AssertionError(f"label tidak muncul: {label}")


def scenario_frames():
    """Gambar contoh: tiap ekspresi/adegan diambil dari Pet sungguhan setelah dibiarkan bergerak sampai mantap."""
    out = []

    def pet(**kw):
        return Pet(0, rng=random.Random(7), lively=False, zoom_prob=0, **kw)

    p = pet()
    out.append(("normal (telinga tegak)", p.frame(_settle(p, 0, 2))))
    p = pet(); p.press(0, 2); p.release(80, 2)
    out.append(("senang (tap OK)", p.frame(_settle(p, 80, 0.7))))
    p = pet(); p.press(0, 2)
    out.append(("dielus: mendengkur + kneading", p.frame(_settle(p, 0, 1.8))))
    out.append(("ketiduran saat dielus", p.frame(_settle(p, 1.8, 2.7))))
    p = pet()
    for i in range(3):
        p.press(i * 200, 2); p.release(i * 200 + 60, 2)
    out.append(("geli (OK 3x)", p.frame(_settle(p, 560, 0.3))))
    p = pet(); p.notify(0)
    out.append(("terkejut", p.frame(_settle(p, 0, 0.7))))
    p = pet(bored_after=500); out.append(("bosan (telinga turun)", p.frame(_settle(p, 0, 3))))
    p = pet(bored_after=500); p.trigger("yawn", 2000)
    out.append(("menguap", p.frame(_settle(p, 2000, 0.7))))
    p = pet(); p.sleep_now(0)
    out.append(("tidur", p.frame(_settle(p, 0, 3.4))))
    for kind, wait, name in (("slowblink", 0.6, "kedip lambat (percaya)"), ("tilt", 0.7, "kepala miring"),
                             ("blep", 0.6, "melet"), ("sniff", 0.3, "mengendus"), ("wink", 0.17, "kedip sebelah")):
        p = pet(); p.trigger(kind, 0)
        out.append((name, p.frame(_settle(p, 0, wait))))
    p = pet(); p.trigger("groom", 0)
    t = _settle(p, 0, 1.0)
    out.append(("mencuci muka: menjilat kaki", p.frame(t)))
    out.append(("mencuci muka: mengusap", p.frame(_settle(p, t, 1.0))))
    p = pet(); p.trigger("bird", 0)
    out.append(("melihat burung (mengoceh)", p.frame(_settle(p, 0, 1.4))))
    p = pet(); p.press(0, 0)
    t = _settle(p, 0, 0.95)
    out.append(("tahan UP: ikan jatuh", p.frame(t)))
    t = _settle(p, t, 0.55)
    out.append(("tahan UP: nyam nyam", p.frame(t)))
    out.append(("tahan UP: kenyang", p.frame(_settle(p, t, 6.5))))
    p = pet(); p.press(0, 1)
    t = 0
    for name, dt_ in (("tahan DOWN: berubah 1/3", 0.75), ("berubah 2/3", 0.2), ("berubah 3/3", 0.2)):
        t = _settle(p, t, dt_)
        out.append((name, p.frame(t)))
    for lab in ("main: mengamati", "main: mengendap", "main: goyang pantat", "main: terkam!", "main: menangkap",
                "main: mengejar"):
        t, F = _until(p, t, lab)
        out.append((lab, F))
    for kind, labs in (("jalan", ("jalan: berjalan", "jalan: duduk melihat sekeliling")),
                       ("ekor", ("ekor: berputar", "ekor: pusing")),
                       ("stretch", ("stretch: menunduk", "stretch: punggung melengkung")),
                       ("garuk", ("garuk: menggaruk telinga",)), ("loaf", ("loaf: tidur melingkar",))):
        p = pet()
        p.start_escape(0, kind)
        t = 0.0
        for lab in labs:
            t, _ = _until(p, t, lab)
            t += 400
            out.append((lab, p.frame(t)))
    p = pet()
    p.start_escape(0, "ngumpet")
    out.append(("ngumpet: menghilang mendadak", p.frame(250)))
    b, pks = p.esc["b"], p.esc["peeks"]
    for pk in pks:
        out.append((f"ngumpet: mengintip {pk['edge']}", p.frame(b["run"] + pk["t0"] + 450 + pk["hold"] // 2)))
    p = pet()
    p.start_escape(0)
    b = p.esc["b"]
    for name, e in (("kabur 1: kesal", 500), ("kabur 2: berubah jadi kucing", b["poof"] + 420),
                    ("kabur 3: lari", b["run"] + 800), ("kabur 4: layar sepi", b["gone"] + 2200),
                    ("kabur 5: mengintip", b["peek"] + 900), ("kabur 6: pulang", b["ret"] + 1100),
                    ("kabur 7: duduk", b["sit"] + 150), ("kabur 8: jadi wajah lagi", b["unm"] + 400)):
        out.append((name, p.frame(e)))
    p = pet()
    p.start_escape(0, "zoomies")
    b = p.esc["b"]
    out.append(("zoomies: lari kencang", p.frame(b["run"] + 700)))
    out.append(("zoomies: kembali", p.frame(b["ret"] + 350)))
    return out


def _contact_sheet(items, path, scale, cols):
    os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
    import pygame
    pygame.init()
    pad = 14
    rows = (len(items) + cols - 1) // cols
    cw, ch = W * scale + pad, H * scale + pad + 14
    surf = pygame.Surface((cols * cw + pad, rows * ch + pad))
    surf.fill(PANEL)
    font = pygame.font.Font(None, 20)
    fb = Fb()
    for i, (name, F) in enumerate(items):
        draw_face(fb, F)
        ox, oy = pad + (i % cols) * cw, pad + (i // cols) * ch + 14
        surf.blit(font.render(name, True, (230, 230, 230)), (ox, oy - 14))
        render(surf, fb, ox, oy, scale)
    pygame.image.save(surf, path)
    print("tersimpan:", path)


def save_sheet(path):
    _contact_sheet(scenario_frames(), path, 3, 4)


def save_morph_strip(path):
    """Frame beruntun (tiap 100 ms) saat wajah berubah jadi kucing 8-bit: untuk menilai mulusnya peralihan."""
    p = Pet(0, rng=random.Random(7), lively=False, kabur_prob=0, zoom_prob=0)
    p.start_escape(0)
    t0 = p.esc["b"]["poof"] - 100
    items = [(f"+{k * 100} ms", p.frame(t0 + k * 100)) for k in range(0, 11)]
    _contact_sheet(items, path, 2, 4)


def _lit(fb):
    return sum(sum(r) for r in fb.px)


def selftest():
    fb = Fb()
    for name, F in scenario_frames():
        draw_face(fb, F)
        assert "sepi" in name or _lit(fb) > 100, f"{name}: gambar hampir kosong"

    # wajah diam simetris terhadap x = 64 (pakai kolom 128 - x), termasuk telinga
    p = Pet(0, lively=False, zoom_prob=0)
    draw_face(fb, p.frame(_settle(p, 0, 3)))
    bad = sum(1 for row in fb.px for x in range(1, 128) if row[x] != row[128 - x])
    assert bad <= 30, f"normal tidak simetris ({bad} piksel)"
    assert sum(fb.px[y][x] for y in range(0, 10) for x in range(4, 26)) > 20, "telinga tidak tergambar"

    # hidup: dibiarkan 4 menit tanpa bosan -> semua kegiatan muncul, gambar terus berubah
    p, hashes = Pet(0, bored_after=10 ** 9, sleep_after=10 ** 9, rng=random.Random(3)), set()
    mins = 99
    for i in range(30 * 20):  # 20 dtk pada 30 fps
        F = p.frame(i * 33.3)
        draw_face(fb, F)
        hashes.add(bytes(b for r in fb.px for b in r))
        mins = min(mins, F.h + (2.5 - F.h) * max(F.bL, F.bR))
    assert len(hashes) > 150, f"gambar jarang berubah ({len(hashes)} dari 600)"
    assert mins < 5, "tidak pernah berkedip"
    _settle(p, 20000, 900)
    s = p.stats
    need = ("blink", "wink", "blep", "sniff", "kepo", "slowblink", "tilt", "groom", "bird", "gaze", "zoomies",
            "jalan", "ekor", "stretch", "garuk", "main_sendiri")
    assert all(s[k] > 0 for k in need), {k: s[k] for k in need}

    # bosan -> menguap / mencuci muka -> tidur
    p = Pet(0, bored_after=1000, sleep_after=25000, rng=random.Random(5), kabur_prob=0, zoom_prob=0, body_prob=0,
            loaf_prob=0)
    seen = set()
    for i in range(30 * 40):
        p.frame(i * 33.3)
        seen.add(p.label)
    assert {"bosan", "menguap", "tidur"} <= seen, seen

    # interaksi: ketukan UP = lirik + kepala miring; OK = senang; OK cepat 3x = geli; notifikasi = terkejut
    p = Pet(0, lively=False, zoom_prob=0)
    p.press(0, 0); p.release(80, 0)
    F = p.frame(_settle(p, 80, 0.5))
    assert F.gx < -0.5 and F.htilt < -0.3 and p.label == "lirik kiri", (F.gx, F.htilt, p.label)
    t = _settle(p, 80, 2)
    p.press(t + 2000, 2); p.release(t + 2060, 2)
    p.frame(t + 2100)
    assert p.mood_name == "senang"
    p = Pet(0, lively=False, zoom_prob=0)
    p.press(0, 2)
    F = p.frame(1000)
    assert p.mood_name == "purr" and F.knead is not None                   # tahan OK = dielus + kneading
    p.frame(4000)
    assert p.mood_name == "tidur"                                          # ... lalu ketiduran
    p.release(4100, 2)
    p.frame(4200)
    assert p.mood_name != "tidur" and p.label == "menguap"                 # dilepas = bangun menguap
    p = Pet(0, lively=False, zoom_prob=0)
    for i in range(3):
        p.press(i * 200, 2); p.release(i * 200 + 60, 2)
    p.frame(600)
    assert p.mood_name == "geli"
    p.notify(5000)
    p.frame(5100)
    assert p.mood_name == "terkejut"
    p.sleep_now(9000)
    p.frame(9001)
    assert p.mood_name == "tidur"
    p.press(9100, 0)
    p.frame(9200)
    assert p.mood_name == "normal"

    # telinga mengikuti suasana: tegak (normal) < turun (bosan/tidur) < rata (kesal)
    def ear_after(**kw):
        q = Pet(0, lively=False, zoom_prob=0, **kw)
        return q.frame(_settle(q, 0, 3)).earL
    assert ear_after() < 0.05 < ear_after(bored_after=500) < 0.5
    p = Pet(0, rng=random.Random(2), lively=False, zoom_prob=0)
    p.start_escape(0)
    assert p.frame(600).earL > 0.4, "telinga tidak rata saat kesal"

    # mencuci muka: kaki muncul, menjilat (lidah), mengusap
    p = Pet(0, lively=False, zoom_prob=0)
    p.trigger("groom", 0)
    paws, tongue, t = 0, False, 0.0
    while t < 4200:
        F = p.frame(t)
        draw_face(fb, F)
        paws += F.paw is not None
        tongue |= F.tongue > 0.3
        t += 33.3
    assert paws > 80 and tongue, (paws, tongue)
    # burung: melintas, mata mengikuti, mulut mengoceh
    p = Pet(0, lively=False, zoom_prob=0)
    p.trigger("bird", 0)
    gxs, chat, t = [], False, 0.0
    while t < 3700:
        F = p.frame(t)
        draw_face(fb, F)
        if F.bird:
            gxs.append(F.gx)
        chat |= F.jaw
        t += 33.3
    assert len(gxs) > 60 and (max(gxs) - min(gxs)) > 0.8 and chat, (len(gxs), chat)

    # kasih makan (tahan UP): ikan jatuh -> dikunyah -> kenyang; dilepas = senang lalu mencuci muka
    p = Pet(0, lively=False, zoom_prob=0)
    p.press(0, 0)
    labels, fish_seen, crumbs_seen, t = set(), False, False, 0.0
    while t < 8000:
        F = p.frame(t)
        draw_face(fb, F)
        labels.add(p.label)
        fish_seen |= F.fish is not None
        crumbs_seen |= bool(F.crumbs)
        t += 33.3
    assert {"makan: ikan jatuh", "makan: nyam nyam", "makan: kenyang"} <= labels and fish_seen and crumbs_seen, labels
    p.release(8100, 0)
    p.frame(8300)
    assert p.mood_name == "senang"
    _until(p, 8300, "mencuci muka", 6000)

    # ajak main (tahan DOWN): wajah mengecil bertahap -> kucing mengendap, goyang pantat, terkam, menangkap
    p = Pet(0, lively=False, zoom_prob=0, rng=random.Random(11))
    p.press(0, 1)
    seen_u, t, prev_u = [], 0.0, 0.0
    while t < 1500:
        F = p.frame(t)
        draw_face(fb, F)
        if F.cat is not None and F.cat["mode"] == "morph":
            assert F.cat["u"] >= prev_u and F.cat["u"] - prev_u < 0.1, "morph melompat"
            prev_u = F.cat["u"]
            seen_u.append(F.cat["u"])
        t += 33.3
    assert len(seen_u) > 15 and min(seen_u) < 0.1 and max(seen_u) > 0.95, (len(seen_u), min(seen_u), max(seen_u))
    got, xs, ys = set(), set(), set()
    need = {"main: mengamati", "main: mengendap", "main: goyang pantat", "main: terkam!", "main: menangkap",
            "main: mengejar"}
    while t < 90000 and not need <= got:
        t += 33.3
        F = p.frame(t)
        draw_face(fb, F)
        if F.cat and F.cat["mode"] == "play":
            got.add(p.label)
            xs.add(rnd(F.cat["x"]))
            ys.add(rnd(F.cat["y"]))
            assert 10 <= F.cat["toy"]["x"] <= 118 and 18 <= F.cat["x"] <= 110, (F.cat["x"], F.cat["toy"]["x"])
    assert need <= got, got
    assert len(xs) > 12 and min(ys) < -8, "kucing tidak bergerak / tidak melompat"
    p.release(t, 1)
    t2, back = t, []
    while t2 < t + 1400:
        t2 += 33.3
        F = p.frame(t2)
        draw_face(fb, F)
        if F.cat is not None and F.cat["mode"] == "morph":
            back.append(F.cat["u"])
    assert back and back[0] > 0.9 and back[-1] < 0.2 and p.scene is None, (back[:1], back[-1:], p.scene)

    # kabur: kesal -> kucing -> lari -> sepi -> mengintip -> pulang -> duduk -> wajah lagi -> senang
    p = Pet(0, rng=random.Random(2), lively=False, zoom_prob=0)
    p.start_escape(0)
    end0, labels, t, after = p.esc["b"]["end"], set(), 0.0, set()
    while t < end0 + 3000:
        F = p.frame(t)
        draw_face(fb, F)
        labels.add(p.label)
        if p.label == "kabur: lari":
            assert _lit(fb) > 80, "kucing tidak tergambar"
        if p.esc is None:
            after.add(p.mood_name)
        t += 33.3
    need = {"kabur: kesal", "kabur: berubah jadi kucing", "kabur: lari", "kabur: sepi", "kabur: mengintip",
            "kabur: pulang", "kabur: duduk", "kabur: jadi wajah lagi"}
    assert need <= labels and "senang" in after, (labels, after)
    p = Pet(0, rng=random.Random(2), lively=False, zoom_prob=0)
    p.start_escape(0)
    b = p.esc["b"]
    p.frame(b["gone"] + 1500)
    p.press(b["gone"] + 1500, 2)
    p.release(b["gone"] + 1560, 2)
    assert p.esc["b"]["end"] < end0, "dipanggil tapi tidak lebih cepat pulang"
    p = Pet(0, lively=False, zoom_prob=0)  # digelitik 6x cepat = kabur
    for i in range(6):
        p.press(i * 150, 2)
        p.release(i * 150 + 50, 2)
    p.frame(1000)
    assert p.mood_name == "kabur", p.mood_name
    p = Pet(0, bored_after=1000, sleep_after=10 ** 9, rng=random.Random(1), kabur_prob=1.0, zoom_prob=0)  # bosan -> pergi main (bukan kabur)
    for i in range(30 * 3):
        p.frame(i * 33.3)
    assert p.stats["ngumpet"] + p.stats["zoomies"] == 1 and p.stats["kabur"] == 0, dict(p.stats)
    # zoomies: lari kencang ke luar, kembali kencang, jadi wajah lagi
    p = Pet(0, rng=random.Random(4), lively=False, zoom_prob=0)
    p.start_escape(0, "zoomies")
    zend, labels, t, after = p.esc["b"]["end"], set(), 0.0, set()
    while t < zend + 2500:
        p.frame(t)
        labels.add(p.label)
        if p.esc is None:
            after.add(p.mood_name)
        t += 33.3
    assert {"zoomies: siap", "zoomies: lari", "zoomies: pulang", "zoomies: duduk"} <= labels and "senang" in after, labels

    # kumis tumbuh dari sisi moncong (dekat hidung), bukan melayang di samping mata
    p = Pet(0, lively=False, zoom_prob=0)
    draw_face(fb, p.frame(_settle(p, 0, 2)))
    roots = sum(fb.px[y][x] for y in range(40, 53) for x in range(47, 56))
    assert roots >= 5, f"pangkal kumis tidak dekat moncong ({roots})"

    # kegiatan badan penuh berskrip: tiap tahap muncul, selalu tergambar, lalu kembali jadi wajah
    for kind, need_l in (("jalan", {"jalan: berjalan", "jalan: duduk melihat sekeliling", "jalan: kembali"}),
                         ("ekor", {"ekor: melihat ekor", "ekor: berputar", "ekor: pusing"}),
                         ("stretch", {"stretch: menunduk", "stretch: punggung melengkung", "stretch: duduk"}),
                         ("garuk", {"garuk: menggaruk telinga"})):
        p = Pet(0, rng=random.Random(6), lively=False, zoom_prob=0)
        p.start_escape(0, kind)
        zend, labs, t, xs = p.esc["b"]["end"], set(), 0.0, set()
        while t < zend + 500:
            F = p.frame(t)
            draw_face(fb, F)
            labs.add(p.label)
            if F.cat and F.cat["mode"] == "sprite":
                assert _lit(fb) > 80, f"{kind}: kucing tidak tergambar"
                xs.add(rnd(F.cat["x"]))
            t += 33.3
        assert need_l <= labs and p.esc is None and p.mood_name == "normal", (kind, labs)
        if kind == "jalan":
            assert len(xs) > 30, "kucing tidak berjalan melintasi layar"
    # tidur melingkar: bernapas + z, bangun saat tombol ditekan
    p = Pet(0, rng=random.Random(6), lively=False, zoom_prob=0)
    p.start_escape(0, "loaf")
    zs_seen, t = False, 0.0
    while t < 5000:
        F = p.frame(t)
        draw_face(fb, F)
        zs_seen |= bool(F.cat and F.cat.get("zs"))
        t += 33.3
    assert p.label == "loaf: tidur melingkar" and zs_seen, p.label
    p.press(5100, 0)
    t = 5100.0
    while p.esc is not None and t < 9000:
        t += 33.3
        p.frame(t)
    assert p.esc is None and p.mood_name != "kabur", "tidak bangun saat tombol ditekan"
    # mengantuk sendiri -> tidur melingkar
    p = Pet(0, bored_after=100, sleep_after=500, rng=random.Random(1), kabur_prob=0, zoom_prob=0, body_prob=0,
            loaf_prob=1.0)
    for i in range(30 * 4):
        p.frame(i * 33.3)
    assert p.label == "loaf: tidur melingkar", p.label
    # main sendiri tanpa tombol: tikus + kucing 8-bit; tombol ditekan = berhenti dan kembali jadi wajah
    p = Pet(0, rng=random.Random(2), lively=False, zoom_prob=0)
    p.auto_play_until = 12000
    got, t = set(), 0.0
    while t < 9000:
        t += 33.3
        F = p.frame(t)
        if F.cat and F.cat["mode"] == "play":
            got.add(p.label)
    assert len(got) >= 3, got
    p.press(9000, 0)
    p.release(9060, 0)
    _until(p, 9060, "main: kembali jadi wajah", 3000)

    # ngumpet: tiba-tiba menghilang, mengintip dari 3-4 sisi layar yang berbeda, lalu muncul lagi dengan senang
    p = Pet(0, rng=random.Random(8), lively=False, zoom_prob=0)
    p.start_escape(0, "ngumpet")
    zend, labs, t, after, lit_peek = p.esc["b"]["end"], set(), 0.0, set(), 0
    while t < zend + 500:
        F = p.frame(t)
        draw_face(fb, F)
        labs.add(p.label)
        if F.cat and F.cat["mode"] == "peekedge" and F.cat["vis"] >= 15:
            lit_peek += _lit(fb) > 150
        if p.esc is None:
            after.add(p.mood_name)
        t += 33.3
    edges = {x.split()[-1] for x in labs if x.startswith("ngumpet: mengintip")}
    assert {"ngumpet: menghilang", "ngumpet: muncul lagi", "ngumpet: sepi"} <= labs, labs
    assert len(edges) >= 3 and edges <= {"atas", "kiri", "kanan", "bawah"} and lit_peek > 10, (edges, lit_peek)
    assert "senang" in after, after
    # acak sendiri: dibiarkan lama, pet ngumpet beberapa kali
    p = Pet(0, bored_after=10 ** 9, sleep_after=10 ** 9, rng=random.Random(3), zoom_prob=0, body_prob=1.0)
    _settle(p, 0, 400)
    assert p.stats["ngumpet"] >= 2, dict(p.stats)

    # bosan berulang -> akhirnya tidur: kegiatan sendiri tidak mengulang waktu bosan
    p = Pet(0, bored_after=1000, sleep_after=8000, rng=random.Random(9), kabur_prob=0, zoom_prob=0, body_prob=1.0,
            loaf_prob=0)
    seen, t = set(), 0.0
    while t < 90000 and "tidur" not in seen:
        t += 33.3
        p.frame(t)
        seen.add(p.mood_name)
    assert "tidur" in seen and sum(p.stats[k] for k in BODY_KINDS if k in p.stats) >= 1, (seen, dict(p.stats))
    # menguap saat bosan: kadang langsung tertidur (bukan hanya menunggu waktu)
    p = Pet(0, bored_after=1000, sleep_after=600000, rng=random.Random(4), kabur_prob=0, zoom_prob=0, body_prob=0,
            loaf_prob=0)
    seen, t = set(), 0.0
    while t < 400000 and "tidur" not in seen:
        t += 33.3
        p.frame(t)
        seen.add(p.mood_name)
    assert "tidur" in seen and p.stats["yawn"] >= 1, (seen, dict(p.stats))

    # tidur ada batasnya (acak 0,5-3 menit): wajah saja maupun tidur melingkar bangun sendiri sambil menguap
    for loaf_prob in (0.0, 1.0):
        p = Pet(0, bored_after=100, sleep_after=500, rng=random.Random(5), kabur_prob=0, zoom_prob=0, body_prob=0,
                loaf_prob=loaf_prob, sleep_min=4000, sleep_max=4000)
        t, state, woke_at, yawned = 0.0, "awal", None, False
        while t < 20000 and woke_at is None:
            t += 33.3
            p.frame(t)
            if p.mood_name == "tidur" or p.label.startswith("loaf: tidur"):
                state = "tidur"
            if state == "tidur" and p.mood_name not in ("tidur", "kabur"):
                woke_at = t
            yawned |= p.label == "menguap"
        assert woke_at is not None and 4000 <= woke_at <= 8000, (loaf_prob, state, woke_at)
        for _ in range(60):
            t += 33.3
            p.frame(t)
            yawned |= p.label == "menguap"
        assert yawned, f"tidak menguap saat bangun (loaf_prob={loaf_prob})"
    ds = set()
    for sd in range(20):
        q = Pet(0, rng=random.Random(sd), loaf_prob=1.0)
        q.start_escape(0, "loaf")
        ds.add(round(q.esc["sleep_dur"]))
    assert 30000 <= min(ds) and max(ds) <= 180000 and len(ds) > 10, (min(ds), max(ds))

    # kehalusan: ganti ekspresi / peralihan wajah <-> kucing: selisih piksel antar frame 30 fps terbatas
    def worst_diff(p, t0, n, act=None):
        prev, worst, t = None, 0, t0
        for i in range(n):
            if act and i == act[0]:
                act[1]()
            t += 33.3
            draw_face(fb, p.frame(t))
            cur = [bytearray(r) for r in fb.px]
            if prev:
                worst = max(worst, sum(1 for y in range(H) for x in range(W) if cur[y][x] != prev[y][x]))
            prev = cur
        return worst
    p = Pet(0, lively=False, zoom_prob=0)
    t = _settle(p, 0, 1)
    w1 = worst_diff(p, t, 60, (1, lambda: (p.press(t + 40, 2), p.release(t + 100, 2))))
    assert w1 < 1100, f"transisi ekspresi patah-patah ({w1})"
    p = Pet(0, rng=random.Random(7), lively=False, kabur_prob=0, zoom_prob=0)
    p.start_escape(0)
    w2 = worst_diff(p, p.esc["b"]["poof"] - 60, 32)
    assert w2 < 1600, f"peralihan wajah -> kucing patah-patah ({w2})"
    print(f"selftest OK (perubahan terbesar antar frame: ekspresi {w1} piksel, peralihan wajah->kucing {w2} piksel)")


def run(debug=False):
    import pygame
    pygame.init()
    ox, oy = 40, 40
    win = pygame.display.set_mode((W * SCALE + 80, H * SCALE + 150))
    pygame.display.set_caption("Pet kucing - simulasi OLED 128x64, 4 tombol" + (" [DEBUG]" if debug else ""))
    font, small = pygame.font.Font(None, 28), pygame.font.Font(None, 22)
    clock = pygame.time.Clock()
    fb = Fb()
    pet = Pet(pygame.time.get_ticks())
    # Perangkat asli: HANYA 4 tombol. Hanya ini yang tampil dan bisa diklik.
    labels = ["1  UP (tahan: makan)", "2  DOWN (tahan: main)", "3  OK (elus)", "4  BACK (keluar)"]
    keymap = {pygame.K_1: 0, pygame.K_KP1: 0, pygame.K_2: 1, pygame.K_KP2: 1, pygame.K_3: 2, pygame.K_KP3: 2,
              pygame.K_4: 3, pygame.K_KP4: 3}
    # Tombol uji tersembunyi (hanya dengan --debug): bukan bagian dari perangkat.
    hidden = {pygame.K_n: lambda t: pet.notify(t), pygame.K_s: lambda t: pet.sleep_now(t),
              pygame.K_k: lambda t: pet.start_escape(t), pygame.K_z: lambda t: pet.start_escape(t, "zoomies"),
              pygame.K_r: lambda t: pet.start_escape(t, random.choice(["jalan", "ekor", "stretch", "garuk"])),
              pygame.K_l: lambda t: pet.start_escape(t, "loaf"), pygame.K_g: lambda t: pet.start_escape(t, "ngumpet")}
    bw = (W * SCALE) // 4
    rects = [pygame.Rect(ox + i * bw + 4, oy + H * SCALE + 24, bw - 8, 40) for i in range(4)]
    pressed = set()

    def down(b, now):
        """-> True bila harus keluar"""
        if b == 3:  # BACK = keluar dari layar Pet
            return True
        pressed.add(b)
        pet.press(now, b)
        return False

    def up(b, now):
        if b < 3:
            pet.release(now, b)
        pressed.discard(b)

    mouse_b = None
    while True:
        now = pygame.time.get_ticks()
        for ev in pygame.event.get():
            if ev.type == pygame.QUIT:
                return
            if ev.type == pygame.KEYDOWN:
                if ev.key == pygame.K_ESCAPE:
                    return
                if ev.key in keymap and down(keymap[ev.key], now):
                    return
                if debug and ev.key in hidden:
                    hidden[ev.key](now)
            elif ev.type == pygame.KEYUP and ev.key in keymap:
                up(keymap[ev.key], now)
            elif ev.type == pygame.MOUSEBUTTONDOWN and ev.button == 1:
                for i, r in enumerate(rects):
                    if r.collidepoint(ev.pos):
                        if down(i, now):
                            return
                        mouse_b = i
            elif ev.type == pygame.MOUSEBUTTONUP and ev.button == 1 and mouse_b is not None:
                up(mouse_b, now)
                mouse_b = None
        F = pet.frame(now)
        draw_face(fb, F)
        win.fill(PANEL)
        pygame.draw.rect(win, (0, 0, 0), (ox - 10, oy - 10, W * SCALE + 20, H * SCALE + 20), border_radius=8)
        render(win, fb, ox, oy)
        for i, r in enumerate(rects):
            held = i in pressed and i < 3 and now - pet.down.get(i, now) >= HOLD_MS
            col = (230, 160, 70) if held else (90, 140, 200) if i in pressed else (50, 54, 66)
            pygame.draw.rect(win, col, r, border_radius=6)
            img = small.render(labels[i], True, (235, 235, 235))
            win.blit(img, img.get_rect(center=r.center))
        idle = (now - pet.last_input) / 1000
        win.blit(font.render(f"{pet.label}   (diam {idle:0.0f} dtk)", True, (230, 230, 230)),
                 (ox, oy + H * SCALE + 78))
        win.blit(small.render("Ketuk: UP/DOWN lirik, OK elus (3x = geli, 6x = kabur).  TAHAN >0,5 dtk: UP = kasih makan, "
                              "DOWN = ajak main, OK = dielus terus", True, (150, 155, 170)),
                 (ox, oy + H * SCALE + 108))
        pygame.display.flip()
        clock.tick(30)


if __name__ == "__main__":
    if "--selftest" in sys.argv:
        selftest()
    elif "--sheet" in sys.argv:
        save_sheet(sys.argv[sys.argv.index("--sheet") + 1])
    elif "--morph" in sys.argv:
        save_morph_strip(sys.argv[sys.argv.index("--morph") + 1])
    else:
        run(debug="--debug" in sys.argv)
