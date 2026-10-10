#pragma once
#include <Arduino.h>

// frames.bin di partisi "frames" (subtype 0x40), 128x64 (landscape) atau 64x128 (potret):
//  FRM1: header 16 byte + count x 1024 byte bitmap mentah (format drawBitmap)
//  FRM2: header 16 byte (reserved[0..2] = panjang data 24-bit LE) + data berkompresi:
//        tabel offset u32 LE per frame (relatif awal data), lalu tiap frame = 1 byte tipe + isi
//        (0 = 1024 B mentah, 1 = RLE bitmap, 2 = RLE dari XOR frame sebelumnya).
//        Frame kelipatan FRAMES_KEY_EVERY selalu tipe 0/1 (keyframe untuk lompat/loop).
//        RLE: kontrol c < 128 -> c+1 byte literal; c >= 128 -> byte berikut diulang c-125 kali.
//  crc32 = CRC32 seluruh data setelah header (kedua format).
#define FRAME_BYTES 1024
#define FRAMES_PART_SUBTYPE 0x40
#define FRAMES_KEY_EVERY 30

struct __attribute__((packed)) FramesHeader {
  char magic[4];  // "FRM1" atau "FRM2"
  uint16_t count;
  uint8_t width;
  uint8_t height;
  uint8_t fps;
  uint8_t reserved[3];  // FRM1: 0; FRM2: panjang data (24-bit LE)
  uint32_t crc32;  // CRC32 (zlib) dari seluruh data frame
};
static_assert(sizeof(FramesHeader) == 16, "header frames.bin harus 16 byte");

bool frames_begin();  // validasi header + CRC lalu mmap; panggil saat boot & setelah upload
bool frames_valid();
uint16_t frames_count();
uint8_t frames_fps();
uint8_t frames_width();
uint8_t frames_height();
bool frames_portrait();
const uint8_t *frames_get(uint16_t index);
// Bersihkan layar, atur orientasi sesuai animasi, lalu gambar frame (tanpa display())
void frames_drawFrame(uint16_t index);

// Penulisan via OTA. Header ditulis paling akhir, jadi upload yang terputus
// meninggalkan partisi tanpa magic (dianggap kosong), bukan animasi rusak.
bool frames_writeBegin(const FramesHeader &hdr, String &err);
bool frames_writeData(const uint8_t *data, size_t len, String &err);
bool frames_writeEnd(String &err);
