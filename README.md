# LR 266XL Comp-Gate (VST3 + Standalone)

Plugin kompresor + expander/gate 2 channel dengan GUI dari foto dbx 266XL (desain tidak diubah).
Dibuat dengan JUCE 8.0.6 (diunduh otomatis saat build). Latensi **0 sample** (tanpa lookahead / oversampling).

## Cara build lewat GitHub
1. Buat repo baru, upload **semua isi folder ini** (termasuk folder tersembunyi `.github`).
2. Buka tab **Actions** -> workflow **Build VST3 + Standalone** (jalan otomatis saat push, atau klik *Run workflow*).
3. Setelah hijau, unduh artifact `LR-266XL-windows` (dan `LR-266XL-macos`). Isinya folder `VST3` dan `Standalone`.
4. Untuk rilis: beri tag `v1.0.0` -> file ZIP otomatis masuk ke halaman *Releases*.

Pasang VST3 (Windows): salin `LR 266XL Comp-Gate.vst3` ke `C:\Program Files\Common Files\VST3\`.
Versi Windows memakai runtime statis, jadi tidak perlu install VC++ Redistributable.

## Waves SuperRack
SuperRack Performer dapat memuat plugin VST3 pihak ketiga (SuperRack SoundGrid tidak).
1. Pasang file `.vst3` ke folder VST3 di atas.
2. Di SuperRack Performer, scan plugin VST3. Kalau plugin belum muncul, coba pasang input **stereo** pada rack, lalu cek lagi daftarnya.
3. Kalau masih belum muncul: hapus cache VST3 SuperRack lalu scan ulang (lihat panduan Waves "How to Find Third Party VST3 Plugins in SuperRack Performer").
Plugin mendukung layout **mono** dan **stereo** (input = output).

## Latensi / live
- Latensi plugin 0 sample; seluruh proses per-sample tanpa alokasi memori di thread audio.
- **ASIO aktif di Standalone (Windows).** Workflow otomatis mengunduh ASIO SDK saat build. Di aplikasi Standalone buka **Options**, ubah *Audio device type* ke **ASIO**, lalu pilih driver ASIO interface Anda dan atur buffer sekecil yang stabil.
- Kalau unduhan SDK gagal, workflow berhenti dengan pesan jelas. Solusinya: unduh ASIO SDK dari Steinberg dan taruh folder `ASIOSDK` (berisi `common/` dan `host/`) di repo.
- Lisensi: Steinberg menyediakan ASIO SDK dengan opsi GPLv3. Untuk pemakaian pribadi tidak ada masalah; jika Anda membagikan hasil build (Standalone/VST3) ke orang lain, sumber proyek harus ikut dibagikan di bawah GPLv3.
- Standalone: matikan *Mute audio input* di Options agar suara masuk terdengar.

## Fitur sesuai tulisan di panel
Channel One = input kiri, Channel Two = input kanan (mono: hanya Channel One).

| Kontrol | Fungsi |
|---|---|
| Expander/Gate **Threshold** | OFF .. +15 dBu (skala -60 -40 -20 0). OFF = gate mati |
| Expander/Gate **Ratio** | 1:1 .. 4:1 (1.3, 2, 3). Di bawah threshold level turun sesuai rasio |
| Compressor **Threshold** | -40 .. +20 dBu |
| Compressor **Ratio** | 1:1 .. inf:1 (1.3, 2, 4) |
| **Attack** | FAST .. SLOW: ditampilkan dalam **ms** (waktu untuk 15 dB gain reduction), rentang 3 .. 340 ms. Posisi jam 12 = karakter dbx klasik |
| **Release** | FAST .. SLOW: ditampilkan dalam **ms** (waktu untuk 15 dB gain reduction), rentang 75 .. 5000 ms. Posisi jam 12 = karakter dbx klasik (125 ms untuk 15 dB) |
| **OverEasy** | knee lembut (lebar 14 dB); mati = hard knee. Default: nyala |
| **Auto** | meniru karakter dbx klasik: attack 15 ms per 10 dB, release 120 dB/s (setara 125 ms per 15 dB; mengabaikan knob Attack/Release) |
| **Output Gain** | -20 .. +20 dB |
| **Bypass** | bypass channel tanpa klik (lampu merah menyala = bypass) |
| **Stereo Couple** | detektor kompresor memakai True RMS power summing dari kedua channel; threshold, ratio, attack, release, OverEasy, Auto, dan gate memakai kontrol Channel One; Output Gain dan Bypass tetap per channel |

Karakter yang ditiru dari 266XL: detektor True RMS feed-forward, OverEasy soft-knee, attack/release laju tetap dalam dB. Nilai knob Attack dan Release ditampilkan sebagai waktu untuk perubahan 15 dB gain reduction dalam ms; perhitungan DSP internal tetap memakai laju dB/waktu. Gate expander 1:1..4:1 dengan attack < 100 us dan release bergantung program (kedalaman maks 80 dB), meter GR menampilkan yang lebih besar antara kompresor dan gate.

LED: kiri-atas tiap channel = indikator gate (merah: menutup, hijau: terbuka); tiga LED kompresor (hijau: di bawah threshold, kuning: tepat di threshold, merah: di atas); deretan 30 .. 1 = meter Gain Reduction (dB).
Geser knob untuk mengatur, klik dua kali = kembali ke default, scroll mouse = halus. Window bisa di-resize (rasio tetap).

## Tampilan
- Panel digambar ulang tajam (resolusi 3x) dengan tata letak, teks, dan model aslinya: skala knob, tulisan, LED, tombol, dan logo dbx. Semua knob memakai satu filmstrip 64 frame (`Assets/knob.png`).
- Ikon aplikasi dari logo dbx merah transparan (folder `Icon/`: `icon.png`, `icon_small.png`, `icon.ico`).

## Catatan penting
- Acuan level: **0 dBu = -20 dBFS** (+20 dBu = 0 dBFS), supaya seluruh rentang knob terpakai di plugin digital.
- Ini pemodelan perilaku berdasarkan spesifikasi dan manual dbx 266XL, bukan salinan sirkuit asli. Rentang attack/release memakai angka dari lini dbx 166/266 dan lebar knee OverEasy (14 dB) adalah perkiraan; semuanya bisa disetel di `Source/ParamMap.h` dan `Source/DbxEngine.h`.
- Posisi tiap elemen GUI ada di `Source/Layout.h` (koordinat 1x), gambar di folder `Assets/`. Referensi: folder `docs/`.
