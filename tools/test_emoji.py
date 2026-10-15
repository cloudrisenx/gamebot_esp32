#!/usr/bin/env python3
"""Cek pemetaan emoji running text: src/emoji_glyphs.h sesuai tools/emoji_glyphs.py, ke-16 emoji tombol web
(web/ota.html) dapat kode 1..16 dengan aturan yang sama seperti text_set() di src/text_screen.cpp, dan
padanan ASCII jalan.  python tools/test_emoji.py"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import emoji_glyphs as g  # noqa: E402

root = Path(__file__).resolve().parent.parent
assert (root / "src" / "emoji_glyphs.h").read_text(encoding="utf-8") == g.emit_header(), \
    "src/emoji_glyphs.h basi: jalankan python tools/emoji_glyphs.py"

code_of = {}
for i, (_, cp, aliases, _) in enumerate(g.EMOJI, 1):
    for c in [cp] + aliases:
        assert c not in code_of, f"U+{c:X} dobel"
        code_of[c] = i


def text_set(s):  # salinan aturan text_set(): ASCII, emoji ber-ikon -> kode, emoji lain -> ASCII / buang
    out = []
    for ch in s:
        cp = ord(ch)
        if 0x20 <= cp < 0x7F:
            out.append(ch)
        elif cp in code_of:
            out.append(chr(code_of[cp]))
        elif cp in g.ASCII_FALLBACK:
            out.append(g.ASCII_FALLBACK[cp])
    return "".join(out)


html = (root / "web" / "ota.html").read_text(encoding="utf-8")
arr = re.search(r"\[([^\[\]]*)\]\.forEach\(function \(e\)", html).group(1)


def js_str(t):  # isi string JS: \uXXXX, \u{XXXXX} atau karakter langsung
    t = re.sub(r"\\u\{([0-9A-Fa-f]+)\}", lambda m: chr(int(m.group(1), 16)), t)
    return re.sub(r"\\u([0-9A-Fa-f]{4})", lambda m: chr(int(m.group(1), 16)), t)


web = [js_str(e) for e in re.findall(r"'([^']*)'", arr)]
assert len(web) == len(g.EMOJI), (len(web), len(g.EMOJI))
for i, e in enumerate(web, 1):
    assert text_set(e) == chr(i), (i, e, text_set(e))
assert text_set("Hai \U0001F642!") == "Hai :)!"
assert text_set("\U0001F61B\U0001F44D\U0001F3FD") == ":P" + chr(code_of[0x1F44D])  # warna kulit dibuang
assert text_set("\U0001F984") == ""  # tidak dikenal: dibuang
print(f"OK: {len(web)} emoji web -> kode 1..{len(web)}, fallback ASCII & warna kulit benar")
