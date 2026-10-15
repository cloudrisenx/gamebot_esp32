#!/usr/bin/env python3
"""Buat frames.bin (animasi OLED 128x64) untuk partisi "frames" ESP32-C3.

Sumber yang didukung (dideteksi otomatis dari INPUT):
  - folder berisi frame_XXXXX.h lama (array 0x.. format drawBitmap)
  - satu file .h berisi banyak frame (video_frames[][1024], TOTAL_FRAMES, FRAME_DELAY ms)
  - file .gif (animasi)
  - folder berisi gambar .png/.jpg/.bmp (diurutkan berdasarkan nama)

Contoh:
  python tools/make_frames.py tools/legacy_frames/frames -o frames.bin --fps 30
  python tools/make_frames.py tools/legacy_frames/VideoFrame_R4.h -o frames.bin
  python tools/make_frames.py kucing.gif -o frames.bin --dither
  python tools/make_frames.py folder_png/ -o frames.bin --fps 12 --threshold 100
  python tools/make_frames.py video_tegak.gif -o frames.bin --portrait

Format: header 16 byte <4s H B B B 3x I> = "FRM1", count, lebar, tinggi, fps, crc32(data)
        lebar x tinggi = 128x64 (landscape, default) atau 64x128 (--portrait)
        + count x 1024 byte (baris demi baris, MSB = piksel kiri, 1 = nyala).
"""
import argparse
import re
import struct
import sys
import zlib
from pathlib import Path

W, H = 128, 64  # diganti 64x128 oleh --portrait
FRAME_BYTES = W * H // 8  # 1024 byte untuk kedua orientasi
PARTITION_SIZE = 0x60000  # partisi frames 384 KB (partitions.csv)
MAX_FRAMES = (PARTITION_SIZE - 16) // FRAME_BYTES
IMAGE_EXT = {".png", ".jpg", ".jpeg", ".bmp"}


def natural_key(path):
    return [int(t) if t.isdigit() else t for t in re.split(r"(\d+)", path.name)]


def load_legacy(folder):
    files = sorted(folder.glob("*.h"), key=natural_key)
    frames = []
    for f in files:
        text = f.read_text(encoding="utf-8", errors="ignore")
        body = text[text.index("{") + 1:text.rindex("}")]
        data = bytes(int(b, 16) for b in re.findall(r"0x([0-9A-Fa-f]{2})", body))
        if len(data) != FRAME_BYTES:
            sys.exit(f"{f.name}: {len(data)} byte, harus {FRAME_BYTES}")
        frames.append(data)
    return frames


def load_legacy_file(path):
    """Satu .h berisi banyak frame 1024 byte; fps dari FRAME_DELAY (ms per frame) bila ada."""
    text = path.read_text(encoding="utf-8", errors="ignore")
    data = bytes(int(b, 16) for b in re.findall(r"0x([0-9A-Fa-f]{2})", text[text.index("{"):]))
    if len(data) % FRAME_BYTES:
        sys.exit(f"{path.name}: {len(data)} byte, bukan kelipatan {FRAME_BYTES}")
    frames = [data[i:i + FRAME_BYTES] for i in range(0, len(data), FRAME_BYTES)]
    total = re.search(r"TOTAL_FRAMES\s*=\s*(\d+)", text)
    if total and int(total.group(1)) != len(frames):
        sys.exit(f"{path.name}: TOTAL_FRAMES {total.group(1)}, terbaca {len(frames)} frame")
    delay = re.search(r"FRAME_DELAY\s*=\s*(\d+)", text)
    fps = max(1, min(255, round(1000 / int(delay.group(1))))) if delay else 30
    return frames, fps


def to_bitmap(img, args):
    from PIL import Image, ImageOps

    img = img.convert("L")
    if args.fit == "stretch":
        img = img.resize((W, H), Image.LANCZOS)
    else:
        img = ImageOps.pad(img, (W, H), method=Image.LANCZOS, color=0)
    if args.invert:
        img = ImageOps.invert(img)
    if args.dither:
        bw = img.convert("1", dither=Image.FLOYDSTEINBERG)
    else:
        bw = img.point(lambda p: 255 if p >= args.threshold else 0).convert("1", dither=Image.NONE)
    # Mode "1" Pillow: baris demi baris, MSB = piksel kiri, 1 = putih -> sama dengan drawBitmap
    return bw.tobytes()


def load_gif(path, args):
    from PIL import Image, ImageSequence

    gif = Image.open(path)
    frames, durations = [], []
    for frame in ImageSequence.Iterator(gif):
        frames.append(to_bitmap(frame.convert("RGBA").convert("RGB"), args))
        durations.append(frame.info.get("duration", 100) or 100)
    gif_fps = round(1000 / (sum(durations) / len(durations)))
    return frames, max(1, min(60, gif_fps))


def load_images(folder, args):
    from PIL import Image

    files = sorted((f for f in folder.iterdir() if f.suffix.lower() in IMAGE_EXT), key=natural_key)
    return [to_bitmap(Image.open(f), args) for f in files]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input", type=Path, help="folder frame .h lama, file .gif, atau folder gambar")
    ap.add_argument("-o", "--output", type=Path, default=Path("frames.bin"))
    ap.add_argument("--fps", type=int, help="frame per detik (default: 30, atau dari GIF)")
    ap.add_argument("--threshold", type=int, default=128, help="ambang hitam/putih 0-255 (default 128)")
    ap.add_argument("--dither", action="store_true", help="pakai dithering Floyd-Steinberg")
    ap.add_argument("--invert", action="store_true", help="balik warna")
    ap.add_argument("--portrait", action="store_true", help="animasi potret 64x128 (default landscape 128x64)")
    ap.add_argument("--fit", choices=["contain", "stretch"], default="contain",
                    help="contain = jaga rasio + bingkai hitam (default), stretch = tarik penuh")
    args = ap.parse_args()
    global W, H
    if args.portrait:
        W, H = 64, 128

    src = args.input
    default_fps = 30
    if src.is_dir() and any(src.glob("*.h")):
        frames = load_legacy(src)
    elif src.is_file() and src.suffix.lower() == ".h":
        frames, default_fps = load_legacy_file(src)
        if args.invert:
            frames = [bytes(b ^ 0xFF for b in f) for f in frames]
    elif src.is_file() and src.suffix.lower() == ".gif":
        frames, default_fps = load_gif(src, args)
    elif src.is_dir():
        frames = load_images(src, args)
    else:
        sys.exit(f"Input tidak dikenali: {src}")

    if not frames:
        sys.exit("Tidak ada frame ditemukan")
    if len(frames) > MAX_FRAMES:
        print(f"Peringatan: {len(frames)} frame dipotong jadi {MAX_FRAMES} (batas partisi)")
        frames = frames[:MAX_FRAMES]

    fps = args.fps or default_fps
    if not 1 <= fps <= 255:
        sys.exit("--fps harus 1..255")

    data = b"".join(frames)
    header = struct.pack("<4sHBBB3xI", b"FRM1", len(frames), W, H, fps, zlib.crc32(data))
    args.output.write_bytes(header + data)
    total = len(header) + len(data)
    print(f"{args.output}: {len(frames)} frame @ {fps} fps, {total} byte "
          f"({total * 100 // PARTITION_SIZE}% partisi)")


if __name__ == "__main__":
    main()
