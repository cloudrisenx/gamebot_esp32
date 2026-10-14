#include "frames_store.h"
#include <esp_partition.h>
#include <esp_idf_version.h>
#include <Adafruit_SSD1306.h>
#include "power.h"
#include "screen.h"

extern Adafruit_SSD1306 display;

static const uint32_t ERASE_BLOCK = 0x10000;

// ESP-IDF 5 memindahkan tipe memory-map partisi ke esp_partition_* (spi_flash_mmap_* sudah tidak di esp_partition.h)
#if ESP_IDF_VERSION_MAJOR >= 5
typedef esp_partition_mmap_handle_t frames_mmap_handle_t;
#define FRAMES_MMAP_DATA ESP_PARTITION_MMAP_DATA
#define frames_munmap esp_partition_munmap
#else
typedef spi_flash_mmap_handle_t frames_mmap_handle_t;
#define FRAMES_MMAP_DATA SPI_FLASH_MMAP_DATA
#define frames_munmap spi_flash_munmap
#endif

static const esp_partition_t *part = nullptr;
static frames_mmap_handle_t mapHandle;
static bool mapped = false;
static const uint8_t *mapPtr = nullptr;
static FramesHeader header;
static bool valid = false;
static bool compressed = false;  // FRM2

// FRM2: frame hasil decode terakhir (buffer statis, 1 KB)
static uint8_t frameBuf[FRAME_BYTES];
static int32_t bufIndex = -1;

// Status penulisan OTA
static FramesHeader pendingHdr;
static uint32_t writeOffset = 0;
static uint32_t erasedUpTo = 0;
static uint32_t writeCrc = 0;
static bool writing = false;

// CRC32 standar (sama dengan zlib.crc32 di Python)
static uint32_t crc32Update(uint32_t crc, const uint8_t *data, size_t len) {
  crc = ~crc;
  while (len--) {
    crc ^= *data++;
    for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
  }
  return ~crc;
}

static const esp_partition_t *findPartition() {
  if (!part) part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                             (esp_partition_subtype_t)FRAMES_PART_SUBTYPE, "frames");
  return part;
}

static void unmap() {
  if (mapped) frames_munmap(mapHandle);
  mapped = false;
  mapPtr = nullptr;
  valid = false;
  bufIndex = -1;
}

static bool isFrm2(const FramesHeader &h) { return memcmp(h.magic, "FRM2", 4) == 0; }

// Panjang data setelah header: FRM1 = count x 1024, FRM2 = 24-bit di reserved[0..2]
static uint32_t dataLen(const FramesHeader &h) {
  if (isFrm2(h)) return h.reserved[0] | (h.reserved[1] << 8) | ((uint32_t)h.reserved[2] << 16);
  return (uint32_t)h.count * FRAME_BYTES;
}

static bool headerSane(const FramesHeader &h, const esp_partition_t *p) {
  bool dimsOk = (h.width == 128 && h.height == 64) || (h.width == 64 && h.height == 128);
  bool magicOk = memcmp(h.magic, "FRM1", 4) == 0 || isFrm2(h);
  uint32_t len = dataLen(h);
  // FRM2: minimal tabel offset (4 B) + 1 byte tipe per frame
  bool lenOk = isFrm2(h) ? len >= (uint32_t)h.count * 5 : true;
  return magicOk && dimsOk && h.count > 0 && h.fps > 0 && lenOk && sizeof(FramesHeader) + len <= p->size;
}

// ---------------- FRM2 decode ----------------
// RLE: kontrol c < 128 -> c+1 byte literal; c >= 128 -> byte berikut diulang c-125 kali.
// Tidak pernah menulis lewat FRAME_BYTES; true bila tepat 1024 byte & data habis tepat di end.
static bool rleDecode(const uint8_t *p, const uint8_t *end, uint8_t *dst, bool xorMode) {
  uint32_t o = 0;
  while (p < end) {
    uint8_t c = *p++;
    if (c < 128) {
      uint32_t k = c + 1;
      if ((uint32_t)(end - p) < k || o + k > FRAME_BYTES) return false;
      for (uint32_t j = 0; j < k; j++, o++) dst[o] = xorMode ? dst[o] ^ p[j] : p[j];
      p += k;
    } else {
      uint32_t r = c - 125;
      if (p >= end || o + r > FRAME_BYTES) return false;
      uint8_t b = *p++;
      for (uint32_t j = 0; j < r; j++, o++) dst[o] = xorMode ? dst[o] ^ b : b;
    }
  }
  return o == FRAME_BYTES;
}

static const uint8_t *dataPtr() { return mapPtr + sizeof(FramesHeader); }

static uint32_t frameOffset(uint16_t i) {
  const uint8_t *t = dataPtr() + (uint32_t)i * 4;
  return t[0] | (t[1] << 8) | (t[2] << 16) | ((uint32_t)t[3] << 24);
}

// Decode frame i ke frameBuf (tipe 2 = XOR terhadap isi frameBuf, yaitu frame i-1)
static bool decodeFrame(uint16_t i) {
  uint32_t len = dataLen(header);
  uint32_t start = frameOffset(i), end = i + 1 < header.count ? frameOffset(i + 1) : len;
  if (start < (uint32_t)header.count * 4 || end <= start || end > len) return false;
  const uint8_t *p = dataPtr() + start, *e = dataPtr() + end;
  switch (*p) {
    case 0:
      if (end - start != FRAME_BYTES + 1) return false;
      memcpy(frameBuf, p + 1, FRAME_BYTES);
      return true;
    case 1: return rleDecode(p + 1, e, frameBuf, false);
    case 2: return i % FRAMES_KEY_EVERY != 0 && rleDecode(p + 1, e, frameBuf, true);
    default: return false;
  }
}

bool frames_begin() {
  unmap();
  const esp_partition_t *p = findPartition();
  if (!p) return false;
  if (esp_partition_read(p, 0, &header, sizeof(header)) != ESP_OK || !headerSane(header, p)) return false;

  uint32_t len = sizeof(FramesHeader) + dataLen(header);
  const void *ptr;
  if (esp_partition_mmap(p, 0, len, FRAMES_MMAP_DATA, &ptr, &mapHandle) != ESP_OK) return false;
  mapped = true;
  mapPtr = (const uint8_t *)ptr;

  // CRC per 64KB agar watchdog tetap diberi makan
  uint32_t crc = 0;
  const uint8_t *data = mapPtr + sizeof(FramesHeader);
  uint32_t remaining = len - sizeof(FramesHeader);
  while (remaining) {
    uint32_t n = min(remaining, ERASE_BLOCK);
    crc = crc32Update(crc, data, n);
    data += n;
    remaining -= n;
    power_feed();
  }
  if (crc != header.crc32) {
    unmap();
    return false;
  }
  compressed = isFrm2(header);
  if (compressed) {
    // Decode seluruh animasi sekali: tabel offset & setiap frame terbukti utuh sebelum dipakai
    for (uint16_t i = 0; i < header.count; i++) {
      if (!decodeFrame(i)) {
        Serial.printf("[ANIM] frames.bin FRM2 rusak di frame %u\n", i);
        unmap();
        return false;
      }
      if (i % 64 == 0) power_feed();
    }
    bufIndex = header.count - 1;
    Serial.printf("[ANIM] FRM2 %u frame, %u B (rasio %.1fx)\n", header.count, len,
                  (float)(sizeof(FramesHeader) + (uint32_t)header.count * FRAME_BYTES) / len);
  }
  valid = true;
  return true;
}

bool frames_valid() { return valid; }
uint16_t frames_count() { return valid ? header.count : 0; }
uint8_t frames_fps() { return header.fps; }
uint8_t frames_width() { return header.width; }
uint8_t frames_height() { return header.height; }
bool frames_portrait() { return header.width < header.height; }

const uint8_t *frames_get(uint16_t index) {
  if (!compressed) return mapPtr + sizeof(FramesHeader) + (uint32_t)index * FRAME_BYTES;
  if (!valid || index >= header.count) return frameBuf;
  if ((int32_t)index == bufIndex) return frameBuf;
  // Lanjut dari frame di buffer bila sesudah keyframe terdekat (frame berikutnya = satu decode);
  // selain itu (mundur / loop / lompat jauh) mulai dari keyframe.
  uint16_t key = index - index % FRAMES_KEY_EVERY;
  uint16_t from = bufIndex >= key && bufIndex < (int32_t)index ? bufIndex + 1 : key;
  for (uint16_t i = from; i <= index; i++) decodeFrame(i);  // sudah divalidasi di frames_begin
  bufIndex = index;
  return frameBuf;
}

void frames_drawFrame(uint16_t index) {
  if (frames_portrait()) screen_portrait();
  else screen_landscape();
  display.clearDisplay();
  display.drawBitmap(0, 0, frames_get(index), header.width, header.height, SSD1306_WHITE);
}

bool frames_writeBegin(const FramesHeader &hdr, String &err) {
  const esp_partition_t *p = findPartition();
  if (!p) { err = "Partisi frames tidak ada"; return false; }
  if (!headerSane(hdr, p)) { err = "Bukan frames.bin valid / terlalu besar"; return false; }
  unmap();  // mapping lama tidak boleh dipakai saat flash ditulis
  pendingHdr = hdr;
  writeOffset = sizeof(FramesHeader);
  erasedUpTo = 0;
  writeCrc = 0;
  writing = true;
  return true;
}

bool frames_writeData(const uint8_t *data, size_t len, String &err) {
  if (!writing) return false;
  uint32_t end = sizeof(FramesHeader) + dataLen(pendingHdr);
  if (writeOffset + len > end) { err = "Ukuran file melebihi header"; writing = false; return false; }

  while (writeOffset + len > erasedUpTo) {
    if (esp_partition_erase_range(part, erasedUpTo, ERASE_BLOCK) != ESP_OK) {
      err = "Gagal erase flash"; writing = false; return false;
    }
    erasedUpTo += ERASE_BLOCK;
    power_feed();
  }
  if (esp_partition_write(part, writeOffset, data, len) != ESP_OK) {
    err = "Gagal tulis flash"; writing = false; return false;
  }
  writeCrc = crc32Update(writeCrc, data, len);
  writeOffset += len;
  return true;
}

bool frames_writeEnd(String &err) {
  if (!writing) return false;
  writing = false;
  uint32_t end = sizeof(FramesHeader) + dataLen(pendingHdr);
  if (writeOffset != end) { err = "File terpotong"; return false; }
  if (writeCrc != pendingHdr.crc32) { err = "CRC tidak cocok"; return false; }
  if (esp_partition_write(part, 0, &pendingHdr, sizeof(pendingHdr)) != ESP_OK) {
    err = "Gagal tulis header"; return false;
  }
  if (!frames_begin()) { err = "Verifikasi gagal"; return false; }
  return true;
}
