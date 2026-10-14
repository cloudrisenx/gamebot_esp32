#pragma once
#include <Arduino.h>

enum Btn { BTN_UP, BTN_DOWN, BTN_OK, BTN_BACK, BTN_COUNT };

void buttons_begin();
void buttons_update();          // panggil setiap loop()
bool btn_pressed(Btn b);        // true sekali per tekanan (event dikonsumsi)
bool btn_any_pressed();         // konsumsi semua event, true jika ada yang ditekan
bool btn_held(Btn b);           // sedang ditahan (status setelah debounce)
bool btn_any_pending();         // seperti btn_any_pressed tapi tidak mengonsumsi event
void buttons_attachWake(void (*isr)());  // ISR dipanggil di setiap perubahan pin tombol
// Stempel waktu interrupt (µs, esp_timer) untuk tekan PERTAMA tiap tombol sejak btn_armTiming()
void btn_armTiming();
bool btn_pressTimeUs(Btn b, uint64_t &us);  // false = belum ada tekan sejak di-arm
// Double-tap dari interrupt: true SEKALI bila dua TAP baru (tekan-lepas <= 150 ms) di tombol ini
// berselisih <= windowMs (dihitung dari saat ditekan)
bool btn_doubleTap(Btn b, uint32_t windowMs);
void btn_tapSync();  // anggap semua tekan sebelumnya sudah dibaca (tap & chord), mis. tekan menu sebelum game
// Chord dari interrupt: true SEKALI bila a dan b ditekan (tekan baru) dengan selisih <= windowMs
bool btn_chord(Btn a, Btn b, uint32_t windowMs);
void buttons_clear();           // buang semua event yang belum dibaca
// Light sleep: interrupt tepi tidak jalan saat tidur. on = pin yang sedang dilepas jadi wakeup level LOW
// (dikembalikan ke interrupt tepi oleh ISR saat ditekan); off = semua kembali ke interrupt tepi.
void buttons_armWake(bool on);
