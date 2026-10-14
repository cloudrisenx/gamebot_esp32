#pragma once
#include <Arduino.h>

#define WDT_TIMEOUT_S 8

// Tombol yang bisa membangunkan dari deep sleep (ESP32-C3: hanya GPIO 0-5)
#define WAKE_PIN_UP 3
#define WAKE_PIN_DOWN 1

void power_begin();              // panggil awal setup(): WDT + catat alasan reset/bangun
void power_feed();               // panggil setiap loop()
bool power_wokeFromSleep();      // bangun dari deep sleep (tombol atau timer jadwal malam)
bool power_wokeByTimer();        // bangun karena timer akhir jadwal malam
const char *power_resetReason();

void power_applyContrast();
void oled_flush();               // display.display() hanya bila layar menyala (hemat I2C & daya)
bool power_takeScreenWoke();     // true sekali setelah layar menyala lagi -> gambar ulang layar aktif
bool power_dimmed();             // layar sedang diredupkan (idle >= setengah waktu "Layar mati")
uint32_t power_takeFlushMaxUs(); // durasi oled_flush terlama sejak panggilan terakhir (us), lalu reset
// Alasan layar menyala/mati (untuk log "[POWER]" dan aturan "dimatikan manual")
enum PowerReason : uint8_t {
  PR_NONE, PR_BUTTON, PR_POPUP, PR_CALL, PR_KEEP_NAV, PR_KEEP_ANIM, PR_KEEP_BOOT, PR_KEEP_OTA, PR_TIMEOUT, PR_MANUAL
};
// Reset timer idle & nyalakan layar. Setelah BACK di Beranda (dimatikan manual) layar hanya menyala
// lagi karena PR_BUTTON/PR_CALL/PR_POPUP; alasan keepAwake hanya menunda deep sleep.
void power_activity(PowerReason reason, const char *detail = nullptr);
bool power_screenOn();
void power_screenOffNow();       // BACK di Beranda: matikan layar sekarang (hanya tombol/panggilan/WA menyalakan)

// keepAwake != PR_NONE: layar tetap nyala & timer idle direset (boot, OTA, navigasi, animasi).
// sleepAllowed: boleh masuk deep sleep otomatis.
void power_update(PowerReason keepAwake, bool sleepAllowed);

void power_requestSleep(uint32_t delayMs);  // deep sleep setelah delayMs (non-blocking)
// Menit sampai akhir jendela malam [startH, endH) untuk jam nowMin (menit sejak 00:00); 0 = di luar
uint32_t power_nightMinutesLeft(int nowMin, int startMin, int endMin);  // menit sejak 00:00
bool power_sleepPending();

// Tunggu loop yang bisa dibangunkan: tombol (ISR) dan BLE (power_loopWake dari task lain) memutusnya seketika
void power_loopWaitBegin();                       // panggil sekali dari setup() (task loop)
void power_loopWait(uint32_t ms);
void power_loopWake();                            // aman dipanggil dari task NimBLE
bool power_buttonRecentlyChanged(uint32_t ms);    // ada perubahan pin tombol dalam ms terakhir

// Light sleep otomatis (esp_pm): hanya bila sdkconfig punya PM + tickless idle (env lightsleep)
#if defined(CONFIG_PM_ENABLE) && defined(CONFIG_FREERTOS_USE_TICKLESS_IDLE)
#define LIGHT_SLEEP_SUPPORTED 1
#else
#define LIGHT_SLEEP_SUPPORTED 0
#endif
// Panggil setiap loop. allowed = layar mati & tidak ada yang butuh CPU penuh / WiFi / interrupt tepi.
// Selama allowed: CPU boleh turun ke 40 MHz dan chip light sleep saat idle; tombol membangunkan lewat level LOW.
void power_lightSleep(bool allowed);
bool power_lightSleepActive();
