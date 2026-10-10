# Riwayat Pengembangan & Ceklist Uji

Versi firmware saat ini: **v1.9.19** (`src/version.h`). Update ke perangkat hanya lewat **OTA**.

---

## 1. Teks panjang berjalan ke kanan (marquee), bukan turun ke bawah / terpotong

Tujuan: semua teks satu baris yang lebih panjang dari layar **berjalan (scroll) ke samping**,
seperti judul lagu di layar Musik — bukan dipindah ke baris bawah atau dipotong.

- [x] Buat **satu helper marquee bersama** (pindahkan logika dari `drawTitle()` di `src/phone.cpp`
      ke helper di `src/menu.cpp`/`menu.h`), dipakai semua layar:
      area x/y/lebar, font, jeda awal 1 dtk, geser 1 px / 50 ms, dua salinan teks agar mulus,
      hanya beranimasi saat layar menyala (hemat daya).
- [ ] Cek dulu di perangkat: **judul lagu** sudah berjalan sejak v1.5.2. Jika di perangkat masih
      turun ke bawah, berarti firmware di perangkat masih versi lama → update dulu.

Tempat yang masih membungkus (wrap) atau memotong teks — ganti ke marquee:

| Layar | Lokasi | Sekarang |
|---|---|---|
| [x] Musik — artis | `src/phone.cpp` (`artist.substring(0, 18) + "..."`) | dipotong "..." |
| [x] Musik — nama aplikasi | `src/phone.cpp` (`app.substring(0, 12)`) | dipotong |
| [x] Notifikasi — daftar (judul) | `src/phone.cpp` (`e.title ... substring(0, 15)`) | dipotong |
| [x] Notifikasi — detail (judul) | `src/phone.cpp` (`e.title ... substring(0, 21)`) | dipotong |
| [x] Cuaca — nama kota | `src/phone.cpp` (`ble_weatherCity() ... substring(0, 21)`) | dipotong |
| [x] Cuaca — 3 hari ke depan | `src/phone.cpp` (`next.substring(0, 21)`) | dipotong |
| [x] Navigasi — jarak/judul | `src/menu.cpp` (`title.substring(0, cols)`) | dipotong |
| [x] Navigasi — nama jalan | `src/menu.cpp` (`drawWrapped(... nav.directions ...)`) | turun ke bawah (2 baris) |
| [x] Pop-up WhatsApp — pengirim | `src/menu.cpp` `popup_draw()` (`sender ... substring(0, 20)`) | dipotong |
| [x] Pop-up panggilan — nama | `src/menu.cpp` `call_draw()` (`drawWrapped(22, 20, 17, 2, name)`) | turun ke bawah |
| [x] Beranda — baris info (cuaca / navigasi) | `src/home.cpp` `drawInfoText()` | dipotong |
| [x] Info — notif terakhir | `src/menu.cpp` (`ble_lastNotifInfo().substring(0, 21)`) | dipotong |

**Tetap dibungkus (wrap) ke bawah — sengaja, karena isinya paragraf:**
- Isi pesan di **pop-up WhatsApp** dan **detail notifikasi** (sudah bisa di-scroll UP/DOWN di detail).
- Teks petunjuk tetap seperti "Mulai rute di Google Maps".

> Keputusan: kalau isi pesan WhatsApp juga mau berjalan ke samping (bukan paragraf), tandai di sini: [ ]

---

## 2. Hapus notifikasi dengan **tahan OK** di daftar Notifikasi

Lokasi: `src/phone.cpp` (layar `PS_NOTIF_LIST`), riwayat di `src/ble_link.cpp` (`history[]`).

- [x] **Ketuk OK** = buka detail (seperti sekarang).
- [x] **Tahan OK ±0,8 dtk** = hapus notifikasi yang dipilih dari daftar.
  - [x] Tampilkan umpan balik saat menahan: bar kecil yang terisi di baris terpilih (agar tidak terhapus tak sengaja).
  - [x] Setelah terhapus: tampil sebentar "Dihapus", pilihan pindah ke item berikutnya.
- [x] Tambah fungsi `ble_historyDelete(index)` di `ble_link` (geser isi ring buffer, kurangi jumlah, sesuaikan penghitung belum dibaca).
- [x] Daftar kosong setelah semua dihapus → tampil "Belum ada notifikasi".
- [x] Ubah legenda/petunjuk di layar: "OK buka  tahan=hapus" (versi panjang 23 karakter tidak muat 128 px; daftar jadi 3 baris).

> Keputusan (opsional): perlu juga **"Hapus semua"** (mis. tahan OK 2 dtk)? [ ]

---

## 3. Ganti semua nama perangkat menjadi `cute_device`

Saat ini nama perangkat berbeda-beda di tiap tempat. Satukan semuanya menjadi **`cute_device`**.

- [x] Buat **satu konstanta** `DEVICE_NAME "cute_device"` (mis. di `src/version.h` atau `src/device.h` baru)
      lalu pakai di semua tempat di bawah, supaya ke depan cukup ganti di satu tempat.

| Tempat | Lokasi | Sekarang | Menjadi |
|---|---|---|---|
| [x] Bluetooth mode Android (dicari di aplikasi Chronos) | `src/ble_link.h` `BLE_NAME` | `ESP32C3 Nav` | `cute_device` |
| [x] Bluetooth mode iPhone (Pengaturan → Bluetooth) | `src/ios_link.h` `IOS_BLE_NAME` | `ESP32C3 Watch` | `cute_device` |
| [x] Nama WiFi saat Update OTA (+ QR WiFi) | `src/ota.h` `OTA_SSID` | `ESP32C3-OTA` | `cute_device_OTA` |
| [x] Teks layar OTA langkah 1 di OLED | `src/ota.cpp` `draw()` (baris "ESP32C3-" + "OTA") | 2 baris | 2 baris `cute_device` + `_OTA` (nama WiFi 15 karakter tidak muat 1 baris) |
| [x] Layar Navigasi "Nama: …" | `src/menu.cpp` (`"Nama: " BLE_NAME`) | `ESP32C3 Nav` | otomatis ikut konstanta |
| [x] Panduan iPhone di perangkat | `src/menu.cpp` `GUIDE_LINES` (`"   ESP32C3 Watch"`) | `ESP32C3 Watch` | `   cute_device` |
| [x] Halaman web OTA (judul tab & judul halaman) | `web/ota.html` `<title>` dan `<h1>` | `ESP32-C3 Update` | `cute_device Update` |
| [x] Pesan di halaman web OTA (2 tempat) | `web/ota.html` (teks "WiFi ESP32C3-OTA") | `ESP32C3-OTA` | `cute_device_OTA` |
| [x] Dokumen panduan iPhone (3 tempat) | `PANDUAN_IPHONE.md` | `ESP32C3 Watch` | `cute_device` |

> Keputusan: nama WiFi OTA dibuat sama persis `cute_device`, atau dibedakan mis. `cute_device_OTA`
> agar tidak tertukar dengan nama Bluetooth? Pilih: [ ] `cute_device`  [x] `cute_device_OTA`

Setelah ganti nama (catat di tes):
- [ ] **Android:** di aplikasi Chronos, hapus perangkat lama lalu tambahkan lagi **cute_device**.
- [ ] **iPhone:** "Lupakan Perangkat" lama di Pengaturan → Bluetooth + **Setting → Hapus pairing** di perangkat,
      lalu pair ulang **cute_device** (iPhone sering menyimpan nama lama di cache).
- [ ] **WiFi OTA:** HP yang pernah tersambung ke `ESP32C3-OTA` perlu sambung ke nama WiFi baru (scan ulang QR).

---

## 4. Penutup pekerjaan

- [x] Naikkan versi di `src/version.h` (mis. **1.6.1**).
- [x] `pio run` sukses → `firmware_ota.bin` siap di root proyek.
- [x] `node tools/test_web_converter.mjs` tetap LULUS.
- [x] Render PNG layar yang berubah (tanpa hardware) untuk cek tidak ada teks bertabrakan/terpotong.
- [x] Fitur Cari HP dihapus (tidak berfungsi di perangkat).
- [ ] Update ke perangkat lewat **Setting → Update OTA → tab Firmware**.

---

## 5. Tes di perangkat yang masih tertunda (dari pekerjaan sebelumnya)

**Mode iPhone (v1.6.0) — perlu iPhone + `pio device monitor`:**
- [ ] Setting → Mode HP → iPhone; panduan muncul; pojok kanan atas "iOS".
- [ ] Pair dari Pengaturan → Bluetooth → **cute_device** (setelah ganti nama, poin 3) → Pasangkan → **Izinkan Bagikan Notifikasi Sistem**.
- [ ] Serial menampilkan: `terenkripsi` → `jam disetel` → `ANCS aktif` → `AMS aktif`; jam Beranda ikut iPhone.
- [ ] WhatsApp (Tampilkan Pratinjau = Selalu): pop-up + riwayat + amplop.
- [ ] Panggilan: pop-up, **OK angkat / BACK tolak**.
- [ ] Musik: judul, artis, `posisi/durasi` + progress bar, semua tombol kontrol.
- [ ] Navigasi / Cuaca → "Hanya untuk Android".
- [ ] Hapus pairing → iPhone diminta pair ulang; kembali ke Mode Android → Chronos normal.
- [ ] Jika jam tidak muncul di daftar Bluetooth iPhone → coba lewat aplikasi nRF Connect / LightBlue (catat hasilnya).

**Hemat daya (v1.5.1):**
- [ ] Serial: `CPU 80 MHz`, `BLE: daya 0 dBm`; `[HEALTH]` putaran maks < 25 ms saat main game.
- [ ] Layar meredup di setengah waktu "Layar mati", lalu mati.
- [ ] (Opsional) ukur arus dengan USB power meter.

**Lain-lain:**
- [ ] OLED v1.9.19: Setting > OLED I2C (800/400/200 kHz) langsung berlaku tanpa restart; OLED tidak menjawab -> turun otomatis, kartu tampil "800>400k". Serial `[OLED] I2C 400 kHz`.
- [ ] OLED v1.9.19: Setting > Uji OLED: kiri/kanan pola (bingkai+silang / papan catur / kotak animasi), OK = overlay (kHz, waktu kirim terakhir/maks, frame, error, re-init), tahan OK 1 dtk = re-init OLED, BACK keluar.
- [ ] OLED v1.9.19: bandingkan 800 / 400 / 200 kHz di pola animasi 1-2 menit: catat mana yang tidak pernah bergeser & error tetap 0 -> pakai itu. Lalu cek game & Beranda tetap mulus. Info: baris "H.../...k I2C800 e0".
- [ ] Pong v1.9.18 2P: saat main tidak ada cara keluar (tahan kanan + OK lama TIDAK mengeluarkan, tidak ada bar "Keluar..."); keluar hanya dari layar menang: OK ulang, BACK keluar setelah 1,5 dtk. READY 2P: "Keluar: saat menang". Duel/XO tetap punya tahan kanan + OK.
- [ ] Pong v1.9.17: smash = tekan KIRI + KANAN bersamaan (selisih <= 0,12 dtk); P2 di 2P = OK + BACK. Smash SIAP sampai bola kena paddle (maks 3 dtk), lalu cooldown 3 dtk. Selama kedua tombol ditahan paddle diam; lepas satu = gerak normal. READY: "Kiri+kanan: smash".
- [ ] Pong v1.9.17: tahan kiri lalu tekan kanan belakangan (sedang bergerak) TIDAK memicu smash; chord P1 & P2 bersamaan di 2P keduanya SIAP; keluar 2P (tahan kanan + OK 5 dtk) tetap jalan [dihapus di v1.9.18: keluar hanya dari layar menang].
- [ ] Pong v1.9.16 2P: skor kecil di samping net; skor P2 terbalik (terbaca dari sisi P2).
- [ ] Pong v1.9.16 2P: saat ada pemenang layar berubah LANDSCAPE "P1/P2 MENANG", skor "7 - 4", "OK: ulang"; "BACK: keluar" baru muncul & aktif 1,5 dtk kemudian; tahan kanan+OK tidak lagi mengeluarkan dari layar menang.
- [ ] Pong v1.9.16 (diganti v1.9.17, lihat atas): menahan/tekan-tekan tombol untuk bergerak TIDAK memicu smash; 2 tap singkat cepat tetap memicu. Duel & menu tetap normal.
- [ ] Pong v1.9.15 (gesture smash diganti di v1.9.17: kiri+kanan bersamaan; sebelumnya v1.9.16: 2 TAP singkat <= 0,15 dtk di tombol yang sama, jarak <= 0,2 dtk): tekan 2x cepat tombol gerak -> smash SIAP 1,5 dtk (paddle berkedip tebal, panel kiri "SIAP"); pukulan berikutnya bola x1,7 + ekor, menjauhi paddle lawan; cooldown 3 dtk (bar "SMSH"). Serial `[PONG] smash P1`.
- [ ] Pong v1.9.15 2P: P1 2x kiri/kanan, P2 2x OK/BACK; double-tap bersamaan keduanya terdeteksi (interrupt). Tekan 3x cepat = 1 smash. Bola kembali normal setelah lawan memantulkan.
- [ ] Pong v1.9.15 1P: CPU kini kadang meleset (galat bidik ±7,5 px) -> bisa dimenangkan tanpa smash (simulasi ±35% poin), dengan smash jauh lebih mudah. Kalau terlalu gampang/sulit, kabari.
- [ ] Tombol v1.9.15: menu, Duel, Hitung, game lain tetap normal setelah perubahan interrupt.
- [ ] Duel v1.9.14: pemenang ditentukan stempel waktu interrupt (µs), bukan urutan loop; waktu reaksi tampil 1 desimal ("241,3 ms"). Serial: `[DUEL] P1 x ms, P2 y ms, selisih z ms -> MENANG/CURANG/SERI`.
- [ ] Duel v1.9.14 2P: tekan hampir bersamaan -> yang benar-benar duluan menang; selisih < 1 ms -> "SERI! ulang" (tanpa poin). Tidak ada lagi undian acak.
- [ ] Duel v1.9.14: curang -> yang LEBIH DULU menekan sebelum DOR yang kalah ronde (dulu: keduanya curang = ulang). Tombol menu & game lain tetap normal (cek!).
- [ ] Hitung v1.9.13: tahan OK di READY -> ATUR HITUNG: tiap baris "Lv3 1,6s  2x2-3" (waktu + pengecoh jenis x jumlah). Isian 1 "< Waktu 1,2s >" (2,0/1,5/1,2/1,0/0,8s), OK -> isian 2 "< Pengecoh Sedang >" (Tanpa/Sedikit/Sedang/Banyak), OK simpan; BACK isian sebelumnya/batal. Serial: `[HITUNG] atur: Lv5 1,2s, pengecoh Sedang`.
- [ ] Hitung v1.9.13: pengecoh Tanpa -> tidak ada emot lain sama sekali; Banyak = sama seperti v1.9.11. Mode 1P & 2P sama.
- [ ] Hitung v1.9.13: setelah update, waktu Lv5 lama terbawa (mis. 1,0s tetap 1,0s; Serial `[HITUNG] migrasi waktu Lv5 -> 1000 ms` sekali). Belum pernah diatur -> default 1,2s & pengecoh Sedang.
- [ ] Hitung v1.9.12 (sejak v1.9.13 menjadi layar ATUR HITUNG, lihat item di atas): di READY tahan OK 1 dtk (bar di bawah) -> layar WAKTU TAMPIL (Lv1-Lv5 + detik, Lv5 dibalik warna); kiri/kanan geser -> kelima baris berubah; OK simpan, BACK batal; OK singkat tetap mulai; kartu "Hitung Lv5" sudah tidak ada di Setting. Serial: `[HITUNG] waktu Lv5 = 1,0s`.
- [ ] Hitung v1.9.11 (sejak v1.9.12 pindah ke layar WAKTU di game Hitung, bukan Setting): waktu Lv5 1,5s/1,2s/1,0s/0,8s, default 1,0s; READY Hitung menampilkan nilainya di baris mode. Waktu Lv k = Lv5 + (5-k) x 0,2 dtk (mis. 1,0s -> 1,8/1,6/1,4/1,2/1,0).
- [ ] Hitung v1.9.11: pengecoh (emot lain) sudah muncul sejak Lv1 (Lv1 1 jenis 2-3, Lv5 3 jenis 3-4); mode 1P & 2P sama-sama mengikuti setting.
- [ ] Sync HP v1.9.10: Setting > Jam > OK saat Chronos tersambung -> "Tersinkron" (bukan "Gagal"). Serial: `[JAM] sinkron HP selesai, paket jam ada/tidak dikirim`. Bila sumber Manual/Perkiraan dan HP tidak kirim jam -> "Tanpa jam" (pakai Restart BT agar Chronos kirim jam saat sambung ulang).
- [ ] Hemat v1.9.9: layar mati (Jaga notif) -> WA tetap pop-up seketika (loop dibangunkan BLE), panggilan tetap tampil.
- [ ] Hemat v1.9.9: layar mati -> tekan tombol cepat/singkat tetap menyalakan layar (tidak perlu tekan 2x).
- [ ] Hemat v1.9.9: bandingkan log `[POWER] ... layar mati: kerja loop rata-rata X ms + idle Y ms` dengan versi lama (idle 20 -> 60 ms).
- [ ] Hemat v1.9.9: ±5 dtk setelah layar mati muncul `[BLE] conn param: hemat (itv 60-120 ms, lat 4)`; layar nyala -> `cepat (itv 15-30 ms, lat 0)`. Navigasi & Musik tetap lancar.
- [ ] Hemat v1.9.9: biarkan 1 jam layar mati -> Chronos/iPhone tidak sering putus; notif tetap masuk.
- [ ] Beranda v1.9.8: legenda bawah urut sesuai tombol fisik kiri->kanan: "<Notif" (UP), "Musik>" (DOWN), "Menu" (OK), ikon power (BACK); tiap label tepat di atas tombolnya.
- [ ] Menu v1.9.7: urutan carousel Animasi, Game, HP, Navigasi, Setting; OK membuka layar yang sesuai.
- [ ] Jam v1.9.7: Setting > Jam > OK -> baris "Sync HP" terpilih; OK -> "Meminta..." lalu "Tersinkron" (Android tersambung) / "Tdk sambung" (BT putus) / "Gagal" (tak ada balasan 5 dtk). Kanan = lanjut ke atur manual. Serial: `[JAM] minta sinkron HP: ok`.
- [ ] Jam v1.9.7 iPhone: Sync HP membaca ulang jam iPhone (CTS) -> "Tersinkron".
- [ ] Malam v1.9.7: setting lama (mis. 23-06) termigrasi jadi "23:00-06:00" (Serial `[POWER] migrasi jadwal malam lama`). Setting > Malam > OK: Aktif ON/OFF, Mulai, Selesai (menit per 5); OFF + OK langsung simpan; mulai = selesai ditolak.
- [ ] Malam v1.9.7: uji jendela pendek siang (mis. 13:00-13:10): layar mati 1 mnt -> tidur, bangun sendiri 13:10 (`[POWER] bangun: timer`). Lewat tengah malam juga (22:00-06:00).
- [ ] Malam v1.9.7: setelah cabut daya (jam "~" Perkiraan) jadwal malam TIDAK menidurkan jam sampai tersinkron/diatur manual.
- [ ] Jam v1.9.6: cabut daya/reset tombol EN lalu nyalakan -> Beranda tampil jam terakhir dengan tanda "~" + "Jam perkiraan, sambungkan Chronos" (bukan --:--). Serial: `[JAM] boot: jam hilang, pakai tersimpan`.
- [ ] Jam v1.9.6: sambungkan Chronos -> tanda "~" hilang, Setting > Jam = "Sinkron". Serial: `[JAM] simpan (sinkron)`.
- [ ] Jam v1.9.6: deep sleep lalu bangun -> jam tetap benar tanpa "~" (Serial: `[JAM] boot: jam sistem masih valid`). Juga cek setelah update OTA.
- [ ] Jam v1.9.6: Setting > Jam > OK: kiri/kanan ubah (tahan = cepat), OK lanjut, BACK isian sebelumnya/batal; simpan -> sumber "Manual"; 31/02 ditolak "Tanggal tdk valid".
- [ ] Cuaca v1.9.6: setelah cuaca masuk, restart/mati -> Beranda & HP > Cuaca tetap tampil dengan "(Nj lalu)" walau Chronos belum tersambung; data baru menggantikan.
- [ ] Restart BT v1.9.6: Setting > Restart BT > OK, OK lagi -> "Restart.." lalu "Ok"; Chronos tersambung lagi sendiri, pairing iPhone tetap ada. Serial: `[BT] restart modul Bluetooth`.
- [ ] Pong v1.9.4: P2 (atas, berhadapan) kini BACK = kiri P2, OK = kanan P2 menurut P2. Kalau masih terbalik, tukar di games_p2MinusBtn/PlusBtn (src/games.h).
- [ ] Hitung v1.9.4 2P serentak: P1 kiri/kanan, P2 BACK(-)/OK(+); sisi P2 tampil terbalik 180 derajat; tahan 2 tombol sendiri 0,5 dtk = KUNCI (angka tidak berubah saat mengunci); 6 dtk; keluar tahan BACK hanya di luar fase jawab.
- [ ] Hitung v1.9.5 waktu tampil (v1.9.4 terlalu cepat): Lv1 2,0 / Lv2 1,6 / Lv3 1,3 / Lv4 1,0 / Lv5 0,8 dtk (pas?).
- [ ] Hitung v1.9.3 (pengganti Racer): benda tampil sekilas lalu hilang; kanan +1, kiri -1 (tahan = cepat), OK kunci, bar 8 dtk; Lv1-5 makin banyak/cepat + pengecoh mulai Lv3; 1P vs CPU (Hi = total poin) & 2P (sejak v1.9.4 serentak, lihat item di atas); keluar tahan BACK 1 dtk; hi-score game lain tetap.
- [ ] Duel v1.9.2 (pengganti Tron): "Siap..." lalu DOR! di waktu acak; 1P vs CPU (kiri = tembak, BACK keluar, Hi = ronde menang), 2P (P1 kiri, P2 BACK, keluar tahan kanan+OK 5 dtk); tekan sebelum DOR = CURANG! (poin lawan); 5 poin menang; hi-score game lain tetap.
- [ ] Teks kontrol v1.9.1: READY Breakout/Dino/Flappy/Pong memakai kiri/kanan (Dino: OK/kiri lompat, tahan kanan merunduk) dan sesuai tombol fisik.
- [ ] XO v1.9.1: 1P vs CPU (kamu X, BACK keluar, CPU bisa dikalahkan, Hi = menang beruntun) & 2P bergantian (keluar tahan kanan+OK 5 dtk); kiri/kanan pindah kotak kosong, OK taruh, garis menang tersorot, yang mulai bergantian.
- [ ] Game v1.9.0: WA tidak pop-up saat di menu Game tapi masuk riwayat (HP > Notifikasi) & amplop Beranda; panggilan tetap tampil. Serial: `[NOTIF] pop-up ditahan (game)`.
- [ ] Pong v1.8.9: 2P potret (P1 UP/DOWN, P2 OK/BACK), menang 7 poin, keluar tahan kanan+OK 5 dtk [sejak v1.9.18: keluar hanya dari layar menang]; 1P vs CPU tetap (hi-score). Cek juga pop-up WA saat main 2P lalu layar kembali potret.
- [ ] Game v1.8.8: Flappy celah tidak nempel tanah/atap & pipa atas terlihat di balik skor; Breakout/Shooter area tengah + panel retro; Breakout lebih cepat; game ke-6 Pong vs CPU (hi-score tersimpan, game lain tidak hilang).
- [ ] Malam v1.8.7 (sejak v1.9.7 jam bebas, lihat item di atas): Setting > Malam = 23:00-06:00; deep sleep terjadwal (layar mati 1 mnt) & bangun otomatis jam 06:00, siang tetap Jaga notif. Catatan: selama deep sleep malam, WA/panggilan TIDAK masuk (Android hilang, iPhone dikirim ulang saat tersambung). Serial: `[POWER] malam: tidur sampai 06:00 (n mnt)`, `[POWER] bangun: timer`.
- [ ] Tidur v1.8.6: Setting > Auto sleep = Jaga notif; layar mati sendiri, kirim WA dari HP lain -> pop-up muncul & layar menyala; deep sleep lama (2/5/10 mnt) tetap bisa dipilih. Serial: `[POWER] mode tidur=Jaga notif`.
- [ ] v1.8.5: fitur Scroll dihapus. Lupakan & pair ulang "cute_device Remote" sekali, lalu Kamera tetap jepret (timer kiri/kanan, OK).
- [ ] Beranda v1.8.4: BACK mematikan layar dan tetap mati (kecuali tombol/panggilan/WA). Kalau menyala sendiri: kirim baris Serial `[POWER] layar ON alasan=..`.
- [ ] Animasi v1.8.2: update firmware DULU, lalu upload MP4 30 fps (FRM2) lewat tab Animasi; durasi lebih panjang (lihat "rasio kompresi" di halaman), log Serial `[ANIM] fps target=30.0 aktual=~30`, frames.bin lama (FRM1) tetap jalan.
- [ ] Kamera v1.8.0: Lupakan & pair ulang "cute_device Remote", timer kiri/kanan (OFF/3/5/10), OK jepret di Kamera bawaan iPhone/Android, BACK batal hitung mundur.
- [ ] Setelah Kamera, notifikasi iPhone (ANCS) masih jalan tanpa pair ulang (Serial saat keluar: `[BOND] bond_hid -> NVS ..., NVS bond_main -> store`).
- [ ] Beranda: legenda kiri/kanan cocok tombol (<Notif kiri = UP, Musik> kanan = DOWN; jika terbalik ubah `MENU_SWAP_LR`).
- [ ] Flappy: kepak (OK/UP), tabrakan pipa/tanah/atap, hi-score tersimpan setelah restart & hi-score game lain tidak hilang.
- [ ] Pesan WA panjang: cek log [NOTIF] len=... (`pio device monitor`), pop-up bergulir (UP/DOWN manual), detail sampai akhir. Jika len ±52 -> dipotong di HP/Chronos, bukan firmware.
- [ ] Kartu Notifikasi & Setting: geser, tahan OK hapus, ubah nilai (ketuk OK di kartu Notifikasi = detail; konfirmasi "OK lagi?" di Setting tetap jalan).
- [ ] Breakout & Shooter landscape: kontrol & tingkat kesulitan (arah UP/DOWN sama dengan menu; bola/musuh tidak terasa terlalu lambat atau cepat).
- [ ] Carousel menu: arah tombol & animasi (Menu, HP, Game). Kiri = item sebelumnya; jika terbalik ubah `MENU_SWAP_LR` ke `1` di `src/screen.h`. Geser terasa halus, tombol cepat tidak tertahan animasi.
- [ ] Breakout (Racer sudah diganti Hitung v1.9.3): masih ada restart/hang? Jika ya, kirim log Serial (dekoder crash sudah aktif).
- [ ] Dino sampai skor > 1500: rintangan tetap muncul.
- [ ] OLED 800 kHz: tidak ada garis acak/berkedip (jika ada, ubah `OLED_I2C_HZ` ke `400000` di `src/main.cpp`).

---

Catatan: panduan lengkap pengguna iPhone ada di [`PANDUAN_IPHONE.md`](PANDUAN_IPHONE.md).
