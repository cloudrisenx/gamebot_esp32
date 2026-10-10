#pragma once
#include <Arduino.h>

// Jam tersimpan: ESP32-C3 tidak punya baterai RTC. Jam sistem bertahan saat deep sleep, tapi hilang
// saat mati total / reset hardware. Epoch terakhir disimpan di NVS ("clock") dan dipakai sebagai
// PERKIRAAN saat boot tanpa jam valid. Sinkron HP (Chronos/CTS) selalu menimpanya.
enum ClockSource : uint8_t { CLK_NONE, CLK_SYNC, CLK_MANUAL, CLK_ESTIMATE };

void clock_begin();                 // setup(): pulihkan jam bila belum valid
void clock_loop();                  // loop(): tangani sinkron HP & simpan berkala (15 mnt)
void clock_save(const char *why);   // sebelum deep sleep / restart
ClockSource clock_source();
const char *clock_sourceName();     // "Sinkron" / "Manual" / "Perkiraan" / "Belum"
uint32_t clock_lastSync();          // epoch lokal sinkron terakhir (0 = belum pernah)
// Setel jam manual (month 1..12). false bila tanggal tidak valid.
bool clock_setManual(int hour, int minute, int day, int month, int year);
