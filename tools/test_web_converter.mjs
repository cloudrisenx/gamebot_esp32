// Uji fungsi konverter di web/ota.html terhadap Pillow & tools/make_frames.py.
// Jalankan dari root proyek:  node tools/test_web_converter.mjs
// Butuh: Node 18+, Python 3 + Pillow.
import { readFileSync, writeFileSync, mkdtempSync, mkdirSync, rmSync } from 'node:fs';
import { execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { tmpdir } from 'node:os';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const html = readFileSync(join(root, 'web', 'ota.html'), 'utf8');
const firstScript = html.match(/<script>([\s\S]*?)<\/script>/)[1];
const mod = { exports: {} };
new Function('module', firstScript)(mod);
const { crc32, packFrames, parseFramesBin, toMono, decodeGif, parseFirmwareInfo, clipRange,
  packFrames2, unpackFrames2, rleEncode, rleDecode, fitGray, rgbaToGray } = mod.exports;

const tmp = mkdtempSync(join(tmpdir(), 'ota-web-test-'));
const python = process.platform === 'win32' ? 'python' : 'python3';
const py = (code) => execFileSync(python, ['-c', code], { cwd: tmp, encoding: 'utf8' });
const sha = (b) => createHash('sha256').update(b).digest('hex');

let failed = 0;
function check(name, ok, detail = '') {
  console.log(`${ok ? 'LULUS' : 'GAGAL'}  ${name}${detail ? '  — ' + detail : ''}`);
  if (!ok) failed++;
}

try {
  // 1. CRC32 == zlib.crc32
  const rnd = new Uint8Array(5000).map(() => Math.random() * 256);
  writeFileSync(join(tmp, 'rnd.bin'), rnd);
  const zcrc = +py("import zlib;print(zlib.crc32(open('rnd.bin','rb').read()))");
  check('crc32 == zlib.crc32', crc32(rnd) === zcrc);

  // 2. toMono (threshold) == Pillow point(>=t) + convert('1')
  py(`
import random
from PIL import Image
random.seed(7)
g = Image.new('L', (128, 64)); g.putdata([random.randrange(256) for _ in range(128*64)])
g.save('gray.png')
open('gray.raw','wb').write(g.tobytes())
for t in (1, 100, 128, 200):
    open(f'mono_{t}.bin','wb').write(g.point(lambda p: 255 if p >= t else 0).convert('1', dither=Image.NONE).tobytes())
open('mono_inv.bin','wb').write(Image.eval(g, lambda p: 255-p).point(lambda p: 255 if p >= 128 else 0).convert('1', dither=Image.NONE).tobytes())
`);
  const gray = new Uint8Array(readFileSync(join(tmp, 'gray.raw')));
  for (const t of [1, 100, 128, 200]) {
    const ref = readFileSync(join(tmp, `mono_${t}.bin`));
    check(`toMono threshold ${t} == Pillow`, Buffer.compare(Buffer.from(toMono(gray, t, false, false)), ref) === 0);
  }
  check('toMono invert == Pillow', Buffer.compare(Buffer.from(toMono(gray, 128, false, true)),
    readFileSync(join(tmp, 'mono_inv.bin'))) === 0);

  // 3. packFrames == make_frames.py (bitmap acak sebagai PNG 1-bit)
  mkdirSync(join(tmp, 'png'));
  py(`
import random
from PIL import Image
random.seed(11)
raw = b''
for i in range(37):
    im = Image.new('1', (128, 64)); im.putdata([random.choice((0, 255)) for _ in range(128*64)])
    im.save(f'png/f_{i}.png'); raw += im.tobytes()
open('bitmaps.raw','wb').write(raw)
`);
  execFileSync(python, [join(root, 'tools', 'make_frames.py'), join(tmp, 'png'), '-o', join(tmp, 'py.bin'), '--fps', '12'],
    { encoding: 'utf8' });
  const raw = new Uint8Array(readFileSync(join(tmp, 'bitmaps.raw')));
  const bitmaps = [];
  for (let i = 0; i < raw.length / 1024; i++) bitmaps.push(raw.subarray(i * 1024, (i + 1) * 1024));
  const jsBin = packFrames(bitmaps, 12);
  const pyBin = readFileSync(join(tmp, 'py.bin'));
  check('packFrames == make_frames.py (sha256)', sha(jsBin) === sha(pyBin), `${jsBin.length} byte`);
  const parsed = parseFramesBin(new Uint8Array(pyBin));
  check('parseFramesBin membaca frames.bin Python', typeof parsed === 'object' && parsed.count === 37 && parsed.fps === 12);
  // 3b. Potret 64x128: PNG 1-bit 64x128 -> make_frames.py --portrait == packFrames(..., 64, 128)
  mkdirSync(join(tmp, 'png_p'));
  py(`
import random
from PIL import Image
random.seed(13)
raw = b''
for i in range(9):
    im = Image.new('1', (64, 128)); im.putdata([random.choice((0, 255)) for _ in range(64*128)])
    im.save(f'png_p/f_{i}.png'); raw += im.tobytes()
open('bitmaps_p.raw','wb').write(raw)
`);
  execFileSync(python, [join(root, 'tools', 'make_frames.py'), join(tmp, 'png_p'), '-o', join(tmp, 'py_p.bin'), '--fps', '9', '--portrait'],
    { encoding: 'utf8' });
  const rawP = new Uint8Array(readFileSync(join(tmp, 'bitmaps_p.raw')));
  const bitmapsP = [];
  for (let i = 0; i < rawP.length / 1024; i++) bitmapsP.push(rawP.subarray(i * 1024, (i + 1) * 1024));
  const pyBinP = readFileSync(join(tmp, 'py_p.bin'));
  check('packFrames potret == make_frames.py --portrait (sha256)', sha(packFrames(bitmapsP, 9, 64, 128)) === sha(pyBinP));
  const parsedP = parseFramesBin(new Uint8Array(pyBinP));
  check('parseFramesBin membaca frames.bin potret', typeof parsedP === 'object' && parsedP.w === 64 && parsedP.h === 128);
  // toMono potret: gray 64x128 di-threshold == Pillow
  py(`
import random
from PIL import Image
random.seed(17)
g = Image.new('L', (64, 128)); g.putdata([random.randrange(256) for _ in range(64*128)])
open('gray_p.raw','wb').write(g.tobytes())
open('mono_p.bin','wb').write(g.point(lambda p: 255 if p >= 128 else 0).convert('1', dither=Image.NONE).tobytes())
`);
  check('toMono potret 64x128 == Pillow', Buffer.compare(
    Buffer.from(toMono(new Uint8Array(readFileSync(join(tmp, 'gray_p.raw'))), 128, false, false, 64, 128)),
    readFileSync(join(tmp, 'mono_p.bin'))) === 0);

  const corrupt = new Uint8Array(pyBin); corrupt[500] ^= 0xFF;
  check('parseFramesBin menolak file rusak', typeof parseFramesBin(corrupt) === 'string');

  // 3c. parseFirmwareInfo pada firmware hasil build (jalankan `pio run` dulu)
  const fwPath = join(root, '.pio', 'build', 'esp32-c3-devkitm-1', 'firmware.bin');
  const fwVersion = readFileSync(join(root, 'src', 'version.h'), 'utf8').match(/FW_VERSION\s+"([^"]+)"/)[1];
  let fw = null;
  try { fw = new Uint8Array(readFileSync(fwPath)); } catch { /* belum di-build */ }
  if (fw) {
    const info = parseFirmwareInfo(fw);
    check('parseFirmwareInfo firmware.bin: ESP32-C3 + versi', info.ok && info.chipId === 5 && info.version === fwVersion,
      `chip ${info.chipId}, versi ${info.version} (version.h ${fwVersion})`);
    const notC3 = fw.slice(0, 4096); notC3[12] = 0; notC3[13] = 0;  // chip id 0 = ESP32 biasa
    const r = parseFirmwareInfo(notC3);
    check('parseFirmwareInfo menolak firmware non-C3', !r.ok && r.chipId === 0);
  } else {
    check('firmware.bin hasil build tersedia (jalankan `pio run`)', false, fwPath);
  }
  check('parseFirmwareInfo menolak data acak', !parseFirmwareInfo(new Uint8Array(4096).map(() => Math.random() * 233)).ok);
  check('parseFirmwareInfo menolak frames.bin', !parseFirmwareInfo(new Uint8Array(pyBin)).ok);

  // 3d. clipRange: rentang potongan video
  {
    let c = clipRange(10, 2, 6, 15, 895);
    check('clipRange 2-6 dtk @15fps = 60 frame', c.frames === 60 && !c.clamped && c.start === 2 && c.end === 6,
      JSON.stringify(c));
    c = clipRange(120, 0, 120, 15, 895);
    check('clipRange video panjang dipotong ke 895 frame', c.clamped && c.frames === 895 && Math.abs(c.end - 895 / 15) < 1e-9,
      `end ${c.end.toFixed(2)}`);
    c = clipRange(5, 4.95, 5, 30, 895);
    check('clipRange di ujung video: frame >= 1, end <= durasi', c.frames >= 1 && c.end <= 5 && c.start < c.end,
      JSON.stringify(c));
    c = clipRange(10, 6, 2, 15, 895);
    check('clipRange start > end dikoreksi', c.start === 6 && c.end > c.start && c.end <= 10, JSON.stringify(c));
    c = clipRange(10, 2.4, 6.8, 15, 895);
    check('clipRange tahan galat float (2,4-6,8 dtk = 66 frame)', c.frames === 66, JSON.stringify(c));
  }

  // 4. decodeGif == Pillow (jumlah, ukuran, piksel setelah komposit)
  py(`
from PIL import Image, ImageDraw
# a) animasi biasa, Pillow mengoptimasi jadi sub-rect antar frame
fr = []
for i in range(12):
    im = Image.new('RGB', (160, 90), (20, 20, 40)); d = ImageDraw.Draw(im)
    d.ellipse((i*10, 20, i*10+40, 60), fill=(255, 200, 0)); d.rectangle((0, 80, 160, 90), fill=(0, 120, 0))
    fr.append(im)
fr[0].save('a.gif', save_all=True, append_images=fr[1:], duration=70, loop=0)
# b) transparansi + disposal 2 (restore background)
fr = []
for i in range(8):
    im = Image.new('RGBA', (64, 64), (0, 0, 0, 0)); ImageDraw.Draw(im).rectangle((i*6, i*6, i*6+16, i*6+16), fill=(255, 0, 0, 255))
    fr.append(im)
fr[0].save('b.gif', save_all=True, append_images=fr[1:], duration=100, loop=0, disposal=2, transparency=0)
# c) interlaced, satu frame, palet lokal
Image.radial_gradient('L').resize((100, 50)).convert('RGB').save('c.gif', interlace=True)
import json
out = {}
for name in ('a', 'b', 'c'):
    im = Image.open(name + '.gif'); frames = []
    for k in range(getattr(im, 'n_frames', 1)):
        im.seek(k); frames.append(im.convert('RGBA').tobytes())
    open(name + '.ref', 'wb').write(b''.join(frames))
    out[name] = [im.size[0], im.size[1], len(frames)]
print(json.dumps(out))
`);
  const meta = JSON.parse(py(`
import json
from PIL import Image
print(json.dumps({n: [*Image.open(n + '.gif').size, getattr(Image.open(n + '.gif'), 'n_frames', 1)] for n in 'abc'}))
`));
  for (const name of ['a', 'b', 'c']) {
    const [w, h, n] = meta[name];
    const ref = readFileSync(join(tmp, name + '.ref'));
    const frames = [];
    const info = decodeGif(new Uint8Array(readFileSync(join(tmp, name + '.gif'))).buffer, (rgba) => frames.push(Uint8Array.from(rgba)));
    check(`decodeGif ${name}.gif ukuran & jumlah frame`, info.width === w && info.height === h && info.count === n,
      `JS ${info.width}x${info.height}x${info.count}, Pillow ${w}x${h}x${n}`);
    // Bandingkan warna yang terlihat (alpha 0 dianggap hitam, seperti di konverter)
    let diff = 0;
    for (let f = 0; f < Math.min(frames.length, n); f++) {
      const off = f * w * h * 4;
      for (let p = 0; p < w * h; p++) {
        for (let c = 0; c < 3; c++) {
          const a = frames[f][p * 4 + 3] ? frames[f][p * 4 + c] : 0;
          const b = ref[off + p * 4 + 3] ? ref[off + p * 4 + c] : 0;
          if (a !== b) { diff++; break; }
        }
      }
    }
    check(`decodeGif ${name}.gif piksel == Pillow`, diff === 0, diff ? `${diff} piksel beda` : '');
  }

  // 9. FRM2: RLE kasus tepi
  const eq = (a, b) => a.length === b.length && a.every((v, i) => v === b[i]);
  const rleCase = (name, src) => {
    const enc = rleEncode(src), dst = new Uint8Array(1024);
    check(`RLE ${name}`, rleDecode(enc, 0, enc.length, dst, false) && eq(dst, src), `${enc.length} B`);
  };
  rleCase('semua sama (run maks berulang)', new Uint8Array(1024).fill(0xAA));
  rleCase('literal maks (tanpa run)', Uint8Array.from({ length: 1024 }, (_, i) => i & 1 ? 0x55 : 0xAA));
  rleCase('run 1/2/3 berselang', Uint8Array.from({ length: 1024 }, (_, i) => [1, 2, 2, 3, 3, 3][i % 6]));
  rleCase('run tepat 130 & 131', Uint8Array.from({ length: 1024 }, (_, i) => (i < 130 ? 7 : i < 261 ? 9 : i & 0xFF)));
  {
    const dst = new Uint8Array(1024);
    check('RLE tolak data terlalu panjang', !rleDecode([0xFF, 1, 0xFF, 1, 0xFF, 1, 0xFF, 1, 0xFF, 1, 0xFF, 1, 0xFF, 1, 0xFF, 1, 0xFF, 1], 0, 18, dst, false));
    check('RLE tolak data terpotong', !rleDecode([5, 1, 2], 0, 3, dst, false));
  }

  // 10. FRM2 round-trip: 3 GIF uji (128x64, threshold 128) + frame acak + frame kosong; lalu FRM1 lama
  const ratios = [];
  const frm2Case = (name, bitmaps, fps = 30) => {
    const packed = packFrames2(bitmaps, fps, 128, 64), un = unpackFrames2(packed);
    const ok = typeof un !== 'string' && un.count === bitmaps.length && bitmaps.every((b, i) => eq(un.bitmaps[i], b));
    const ratio = (16 + bitmaps.length * 1024) / packed.length;
    ratios.push([name, ratio, packed.length, bitmaps.length]);
    check(`FRM2 round-trip ${name}`, ok, typeof un === 'string' ? un : `${bitmaps.length} frame, rasio ${ratio.toFixed(2)}x`);
    return packed;
  };
  for (const name of ['a', 'b', 'c']) {
    const bms = [];
    decodeGif(new Uint8Array(readFileSync(join(tmp, name + '.gif'))).buffer, (rgba, delay, w, h) => {
      const g = rgbaToGray(rgba, w * h);
      bms.push(toMono(fitGray(g, w, h, 'fit', 128, 64), 128, false, false, 128, 64));
    });
    frm2Case(`${name}.gif`, bms);
  }
  const randomBms = Array.from({ length: 40 }, () => Uint8Array.from({ length: 1024 }, () => Math.random() * 256));
  const packedRnd = frm2Case('frame acak', randomBms);
  const packedEmpty = frm2Case('frame kosong', Array.from({ length: 95 }, () => new Uint8Array(1024)));
  {
    const fb = parseFramesBin(packFrames(randomBms.slice(0, 5), 12));
    check('parse FRM1 lama tetap jalan', typeof fb !== 'string' && fb.count === 5 && eq(fb.bitmaps[4], randomBms[4]));
    const bad = Uint8Array.from(packedRnd); bad[bad.length - 1] ^= 1;
    check('FRM2 tolak CRC rusak', typeof unpackFrames2(bad) === 'string');
    check('FRM2 keyframe tiap 30 frame bukan tipe 2', [0, 30, 60, 90].every((i) => {
      const dv = new DataView(packedEmpty.buffer); return packedEmpty[16 + dv.getUint32(16 + i * 4, true)] !== 2;
    }));
  }

  // 11. Decoder C++ (src/frames_store.cpp) disalin ke Python: hasil harus identik dengan unpackFrames2
  writeFileSync(join(tmp, 'frm2.bin'), packFrames2(randomBms.concat(Array.from({ length: 35 }, (_, i) => {
    const b = Uint8Array.from(randomBms[0]); b[i * 7] ^= 0xFF; return b; })), 30, 128, 64));
  const pyOut = py(`
import zlib
d = open('frm2.bin','rb').read()
KEY = 30
assert d[:4] == b'FRM2'
count = int.from_bytes(d[4:6], 'little'); ln = d[9] | d[10] << 8 | d[11] << 16
assert zlib.crc32(d[16:16+ln]) == int.from_bytes(d[12:16], 'little')
data = d[16:16+ln]
def off(i): return int.from_bytes(data[i*4:i*4+4], 'little')
def rle(p, end, dst, xor):           # salinan rleDecode() C++
    o = 0
    while p < end:
        c = data[p]; p += 1
        if c < 128:
            k = c + 1
            if end - p < k or o + k > 1024: return False
            for j in range(k): dst[o] = dst[o] ^ data[p+j] if xor else data[p+j]; o += 1
            p += k
        else:
            r = c - 125
            if p >= end or o + r > 1024: return False
            b = data[p]; p += 1
            for j in range(r): dst[o] = dst[o] ^ b if xor else b; o += 1
    return o == 1024
buf = bytearray(1024); out = bytearray()
for i in range(count):                # salinan decodeFrame() C++
    s = off(i); e = off(i+1) if i + 1 < count else ln
    assert s >= count*4 and e > s and e <= ln
    t = data[s]
    if t == 0: assert e - s == 1025; buf[:] = data[s+1:e]
    elif t == 1: assert rle(s+1, e, buf, False)
    elif t == 2: assert i % KEY and rle(s+1, e, buf, True)
    else: raise SystemExit('tipe ' + str(t))
    out += buf
open('py.raw','wb').write(out)
print(count)
`);
  const jsUn = unpackFrames2(new Uint8Array(readFileSync(join(tmp, 'frm2.bin'))));
  const pyRaw = new Uint8Array(readFileSync(join(tmp, 'py.raw')));
  const jsRaw = new Uint8Array(jsUn.count * 1024); jsUn.bitmaps.forEach((b, i) => jsRaw.set(b, i * 1024));
  check('decoder C++ (salinan Python) == unpackFrames2', +pyOut === jsUn.count && eq(pyRaw, jsRaw), `${pyOut.trim()} frame`);

  console.log('\nRasio kompresi FRM2 (perkiraan maks di 30 fps dengan rasio yang sama):');
  for (const [name, ratio, bytes, n] of ratios) {
    const perFrame = (bytes - 16) / n, maxFrames = Math.min(3600, Math.floor((0xE0000 - 16) / perFrame));
    console.log(`  ${name.padEnd(13)} ${ratio.toFixed(2)}x  (${perFrame.toFixed(0)} B/frame) -> ±${(maxFrames / 30).toFixed(0)} dtk @30fps`);
  }
} finally {
  rmSync(tmp, { recursive: true, force: true });
}

console.log(failed ? `\n${failed} cek GAGAL` : '\nSemua cek LULUS');
process.exit(failed ? 1 : 0);
