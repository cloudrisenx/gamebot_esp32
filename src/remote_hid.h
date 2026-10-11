#pragma once
#include <Arduino.h>

// HID Bluetooth untuk layar Kamera: jam tampil sebagai tombol media bernama DEVICE_NAME " Remote"
// (alamat sendiri, pairing disimpan terpisah dari mode biasa). Eksklusif: BLE Chronos/ANCS
// dihentikan selama aktif.
void remote_begin();
void remote_recoverBonds();   // panggil sekali saat boot: pulihkan bond mode biasa bila reset saat HID aktif
void remote_end();
void remote_update();         // panggil tiap loop: reset BLE otomatis bila pairing gagal berulang
bool remote_running();
bool remote_connected();      // terhubung & terenkripsi (laporan HID bisa dikirim)
bool remote_rejectedFull();   // HP baru ditolak karena memori pairing (bond) penuh
bool remote_bondError();      // penyimpanan pairing terpisah gagal disiapkan (HID tidak dinyalakan)
void remote_volumeUp();       // shutter kamera di iOS & Android
