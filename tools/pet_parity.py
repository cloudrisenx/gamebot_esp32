"""Tes paritas: gambar mesin C++ (src/pet_core.cpp) harus SAMA piksel demi piksel dengan simulasi Python (pet_sim.py).

  1. bangun tes C++:  python -m ziglang c++ -std=c++17 -O2 -ffp-contract=off -Isrc tools/pet_host_test.cpp src/pet_core.cpp -o tools/pet_host_test.exe
  2. python tools/pet_parity.py

Python membuat banyak "keadaan wajah" (adegan contoh + putaran acak + semua kegiatan), menyimpannya sebagai angka
(dibulatkan ke float32 seperti di firmware), C++ menggambar keadaan yang sama, lalu hasilnya dibandingkan.
"""
import math
import os
import random
import struct
import subprocess
import sys
from types import SimpleNamespace

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pet_sim as m  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(HERE, "pet_host_test.exe")
PARAMS = ["w", "h", "r", "lt", "tilt", "lb", "pw", "ph", "curve", "wflat", "wlen", "mc", "mo", "mw", "tongue", "wh",
          "amp", "earL", "earR", "htilt", "blush", "gx", "gy", "fy"]
MODES = {"none": 0, "morph": 1, "fade": 2, "sprite": 3, "play": 4, "dots": 5, "peek": 6, "peekedge": 7}
EDGES = {"atas": 0, "kiri": 1, "kanan": 2, "bawah": 3}
TW = {None: 0, "L": 1, "R": 2, "B": 3}


def f32(x):
    return struct.unpack("f", struct.pack("f", x))[0]


def r32(o):
    """Salin dalam dengan semua float dibulatkan ke float32 (seperti penyimpanan di firmware)."""
    if isinstance(o, bool) or isinstance(o, int) or o is None or isinstance(o, str):
        return o
    if isinstance(o, float):
        return f32(o)
    if isinstance(o, (tuple, list)):
        return type(o)(r32(v) for v in o)
    if isinstance(o, dict):
        return {k: r32(v) for k, v in o.items()}
    if isinstance(o, SimpleNamespace):
        return SimpleNamespace(**{k: r32(v) for k, v in vars(o).items()})
    return o


def pose_toks(P):
    legs = P.get("legs") or [(0, 0)] * 4
    gait = P.get("gait") or (0, 0)
    sc = P.get("scratch")
    return [int(bool(P.get("sitting"))), int(bool(P.get("gait"))), int(bool(P.get("legs"))), int(bool(P.get("wiggle"))),
            int(bool(P.get("eyes_closed"))), int(bool(P.get("mouth"))), int(bool(P.get("prey"))), int(bool(P.get("nolegs"))),
            int(P.get("pupil", 1) > 1), int(sc is not None),
            P.get("crouch", 0.0), P.get("head_dy", 0.0), P.get("ear", 0.0), P.get("tail", 0.5), P.get("tilt", 0.0),
            P.get("arch", 0.0), P.get("bob", 0.0), gait[0], gait[1], *[v for lg in legs for v in lg],
            P.get("wagamp", -1), P.get("wagspd", -1), sc or 0.0]


def toks(F):
    t = [F.t] + [getattr(F, n) for n in PARAMS]
    t += [F.bL, F.bR, TW[F.twitch], F.shake[0], F.shake[1], F.breath, F.nose, int(F.geli), int(F.exclaim), int(F.jaw)]
    t += [len(F.hearts)] + [v for h in F.hearts for v in h]
    t += [1, *F.fish] if F.fish else [0]
    t += [len(F.crumbs)] + [v for c in F.crumbs for v in c]
    t += [1, *F.paw[:2]] if F.paw else [0]
    t += [1, *F.knead] if F.knead else [0]
    t += [1, F.bird[0], F.bird[1], int(F.bird[2]), int(F.bird[3])] if F.bird else [0]
    t += [len(F.zs)] + [v for z in F.zs for v in (z[0], z[1], 1 if z[2] else 0)]
    t += [len(F.dust)] + [v for d in F.dust for v in d]
    c = F.cat
    t += [1 if c else 0]
    if c:
        t += [MODES[c["mode"]], int(c.get("flip", False)), int(c.get("ground", False)), c.get("x", 64.0), c.get("y", 0.0),
              c.get("ph", 0.0), c.get("u", 0.0),
              c.get("rise", 0.0), c.get("look", 0.0), c.get("scroll", 0)]
        zs = c.get("zs", [])
        t += [len(zs)] + [v for z in zs for v in (z[0], z[1], 1 if z[2] else 0)]
        toy = c.get("toy")
        t += [1, toy["x"], toy["ph"], toy["dir"], int(toy["hidden"])] if toy else [0]
        t += [EDGES.get(c.get("edge"), 0), c.get("vis", 0), c.get("pos", 0)]
        t += pose_toks(c.get("pose", {}))
    return " ".join(repr(float(v)) if isinstance(v, float) else str(int(v)) for v in t)


def hexrow(fb):
    out = []
    for y in range(m.H):
        row = fb.px[y]
        for x in range(0, m.W, 4):
            out.append("0123456789abcdef"[row[x] * 8 + row[x + 1] * 4 + row[x + 2] * 2 + row[x + 3]])
    return "".join(out)


def collect():
    """-> daftar (nama, F) dari berbagai keadaan"""
    items = [(n, F) for n, F in m.scenario_frames()]
    for seed in (1, 2, 3):  # putaran acak: semua kegiatan sendiri
        p = m.Pet(0, rng=random.Random(seed), zoom_prob=0.05, body_prob=0.5, kabur_prob=0.3, loaf_prob=0.5,
                  bored_after=8000, sleep_after=20000, sleep_min=6000, sleep_max=12000)
        t = 0.0
        for i in range(1100):
            t += 300 + (i % 5) * 20
            items.append((f"acak{seed}:{p.label}", p.frame(t)))
    for kind in m.ESC_KINDS:  # tiap kegiatan rapat
        p = m.Pet(0, rng=random.Random(5), lively=False, zoom_prob=0)
        p.start_escape(0, kind)
        end = p.esc["b"]["end"] if kind != "loaf" else 9000
        t = 0.0
        while t < end + 200:
            t += 90
            if kind == "loaf" and 5000 < t < 5100:
                p.press(t, 0)
            items.append((f"{kind}:{p.label}", p.frame(t)))
    p = m.Pet(0, rng=random.Random(9), lively=False, zoom_prob=0)  # main & makan & dielus
    p.press(0, 1)
    t = 0.0
    while t < 22000:
        t += 120
        items.append((f"main:{p.label}", p.frame(t)))
        if abs(t - 20000) < 60:
            p.release(t, 1)
    p = m.Pet(0, rng=random.Random(9), lively=False, zoom_prob=0)
    p.press(0, 0)
    t = 0.0
    while t < 9000:
        t += 110
        items.append((f"makan:{p.label}", p.frame(t)))
    p = m.Pet(0, rng=random.Random(9), lively=False, zoom_prob=0)
    p.press(0, 2)
    t = 0.0
    while t < 5000:
        t += 130
        items.append((f"elus:{p.label}", p.frame(t)))
    return items


def main():
    if not os.path.exists(EXE):
        sys.exit("bangun dulu tes C++ (lihat docstring)")
    # tabel pose C++ harus sama dengan POSES Python (urutan sama)
    got = subprocess.run([EXE, "poses"], check=True, capture_output=True, text=True).stdout.split("\n")
    for line, (name, P) in zip(got, m.POSES.items()):
        t = pose_toks(P)
        want = t[:9] + t[10:19] + t[19:27] + t[27:29]
        have = [float(v) for v in line.split()]
        if [float(v) for v in want] != have:
            sys.exit(f"pose C++ beda dengan Python: {name}\n  py  {want}\n  c++ {have}")
    print(f"tabel pose sama ({len(m.POSES)} pose)")
    items = collect()
    rounded = [(n, r32(F)) for n, F in items]
    inp, outp = os.path.join(HERE, "_parity_in.txt"), os.path.join(HERE, "_parity_out.txt")
    with open(inp, "w") as f:
        for _, F in rounded:
            f.write(toks(F) + "\n")
    subprocess.run([EXE, "parity", inp, outp], check=True)
    cpp = open(outp).read().split()
    assert len(cpp) == len(rounded), (len(cpp), len(rounded))
    fb = m.Fb()
    bad, worst, total_diff = 0, (0, None), 0
    for i, ((name, F), line) in enumerate(zip(rounded, cpp)):
        m.draw_face(fb, F)
        py = hexrow(fb)
        if py != line:
            bad += 1
            diff = sum(bin(int(a, 16) ^ int(b, 16)).count("1") for a, b in zip(py, line))
            total_diff += diff
            if diff > worst[0]:
                worst = (diff, (i, name))
    print(f"{len(rounded)} frame dibandingkan, {bad} berbeda, selisih total {total_diff} piksel; terburuk: {worst}")
    for fnm in (inp, outp):
        os.remove(fnm)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
