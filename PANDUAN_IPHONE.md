# Panduan Memakai Jam ESP32 dengan iPhone

Jam ini bisa dipakai dengan **Android** (lewat aplikasi Chronos) atau **iPhone**.
Untuk iPhone **tidak perlu aplikasi apa pun** — jam memakai fitur Bluetooth bawaan iOS.

---

## 1. Memasangkan (pairing) iPhone

1. Di jam: buka **Setting → Mode HP**, tekan **OK** sampai tertulis **iPhone**.
   Layar panduan singkat akan muncul; tekan **BACK** untuk menutupnya.
2. Di iPhone: buka **Pengaturan → Bluetooth**, pastikan Bluetooth menyala.
3. Di bagian **Perangkat Lain**, ketuk **cute_device**.
4. Muncul permintaan pairing → ketuk **Pasangkan**.
5. Muncul pertanyaan **"Bagikan Notifikasi Sistem?"** → ketuk **Izinkan**.
   (Tanpa izin ini notifikasi tidak akan sampai ke jam.)
6. Selesai. Di pojok kanan atas jam, tulisan **iOS** berubah menjadi **BT** dan jam di Beranda
   langsung menyesuaikan dengan jam iPhone.

Setelah dipasangkan sekali, jam akan tersambung lagi otomatis setiap kali berada di dekat iPhone.

### Agar isi pesan WhatsApp ikut terkirim

Secara bawaan iPhone menyembunyikan isi pesan saat HP terkunci. Ubah:

**Pengaturan → Notifikasi → WhatsApp → Tampilkan Pratinjau → Selalu**

Pastikan juga **Izinkan Notifikasi** untuk WhatsApp **aktif**, dan chat yang ingin diterima tidak di-*mute*.

---

## 2. Fitur: Android vs iPhone

| Fitur | Android (Chronos) | iPhone | Keterangan |
|---|:---:|:---:|---|
| Notifikasi WhatsApp & aplikasi lain | ✓ | ✓ | iPhone: lewat izin "Bagikan Notifikasi Sistem" |
| Jam & tanggal otomatis | ✓ | ✓ | |
| Panggilan masuk (pop-up) | ✓ | ✓ | iPhone bisa **angkat (OK)** / **tolak (BACK)** dari jam |
| Kontrol musik (play/jeda, lagu, volume) | ✓ | ✓ | |
| Posisi & durasi lagu | ✗ (perkiraan "~") | ✓ | iPhone mengirim posisi lagu asli → ada progress bar |
| Navigasi Google Maps | ✓ | ✗ | iPhone tidak mengirim data navigasi ke perangkat Bluetooth |
| Cuaca | ✓ | ✗ | Cuaca adalah fitur aplikasi Chronos (tidak ada di iOS) |
| Baterai HP di layar | ✓ | ? | Tampil hanya bila iPhone menyediakan datanya |

Di mode iPhone, menu **Navigasi** dan **HP → Cuaca** tetap ada tetapi
menampilkan pesan **"Hanya untuk Android"**.

---

## 3. Memutus / ganti HP

1. Di iPhone: **Pengaturan → Bluetooth → ⓘ di sebelah cute_device → Lupakan Perangkat Ini**.
2. Di jam: **Setting → Hapus pairing**, tekan **OK** dua kali (tertulis "OK?" lalu "Ok").
3. Pasangkan iPhone baru dengan langkah di bagian 1.

Untuk kembali ke Android: **Setting → Mode HP → Android**, lalu sambungkan dari aplikasi Chronos seperti biasa.

---

## 4. Masalah & solusi

**Jam tidak muncul di daftar Bluetooth iPhone**
- Pastikan di jam **Setting → Bluetooth = ON** dan **Mode HP = iPhone** (pojok kanan atas tertulis **iOS**).
- Jika tetap tidak muncul: pasang aplikasi gratis **nRF Connect** atau **LightBlue** dari App Store,
  cari **cute_device**, ketuk **Connect** sekali → iPhone akan meminta pairing. Setelah itu aplikasi boleh ditutup.

**Notifikasi tidak masuk**
- Saat pairing, pertanyaan **"Bagikan Notifikasi Sistem"** harus dijawab **Izinkan**. Jika terlewat:
  lupakan perangkat di iPhone, **Hapus pairing** di jam, lalu pasangkan ulang.
- Di jam: **Setting → Notif pop-up** jangan **OFF** (pilih **WhatsApp** atau **Semua**).
- Notifikasi harus benar-benar muncul di iPhone (chat tidak sedang dibuka, tidak di-*mute*, mode Fokus/Jangan Ganggu mati).

**Isi pesan kosong, hanya nama pengirim**
- Ubah **Tampilkan Pratinjau** WhatsApp menjadi **Selalu** (lihat bagian 1).

**Jam tidak sinkron (Beranda menampilkan "--:--")**
- Jam baru tersinkron setelah pairing berhasil dan tersambung (pojok kanan atas **BT**).
- Jika sudah **BT** tetapi jam tetap "--:--", sambungkan ulang: matikan-nyalakan Bluetooth iPhone.

**Musik tidak tampil**
- Putar lagu di aplikasi musik iPhone (Apple Music, Spotify, YouTube Music) lalu buka **HP → Musik** di jam.

---

## Kamera (jepret kamera bawaan dari jam) — iPhone & Android

**HP → Kamera** membuat jam tampil sebagai tombol media Bluetooth bernama **cute_device Remote**
(alamat Bluetooth sendiri, jadi muncul sebagai perangkat terpisah dari "cute_device"). Selama layar
Kamera terbuka, notifikasi/musik/jam dari HP berhenti; BACK keluar dan jam kembali ke Mode HP biasa.
Pairing Kamera disimpan terpisah, jadi pairing iPhone untuk notifikasi tidak terganggu.

**WAJIB setelah update ke v1.8.5:** isi HID berubah (fitur Scroll dihapus). Di HP pilih
**Lupakan perangkat** untuk "cute_device Remote", lalu pair ulang.

**Pair sekali:**
1. Buka **HP → Kamera** di jam.
2. Di HP: **Pengaturan → Bluetooth** → ketuk **cute_device Remote** → Pasangkan.

**Memotret:** buka aplikasi **Kamera bawaan** di HP, lalu di jam:

| Tombol | Aksi |
|---|---|
| Kiri / kanan | Timer turun / naik: OFF → 3 → 5 → 10 detik (tersimpan) |
| OK | Timer OFF = langsung jepret; selain itu hitung mundur lalu jepret |
| BACK | Saat hitung mundur = batal; selain itu = keluar |

Jepret = tombol **Volume Up**, yang di aplikasi Kamera iPhone & Android berfungsi sebagai tombol rana.
Jam menyimpan maks **3 pairing Kamera** (terpisah dari pairing mode biasa); bila penuh, HP baru ditolak.
