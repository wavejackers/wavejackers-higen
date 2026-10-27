# Wavejackers HitGen 2.2

MIDI üretici ve synth eklentisi. VST3 (FL Studio, Ableton Live, Cubase, Reaper, Bitwig, Studio One), AU (Logic Pro, GarageBand) ve tek başına uygulama (Standalone) olarak çalışır.

*English below.*

## Neler yapar

- **23 sanatçı ilhamlı stil** (DJ Mag Top 100 2026 analizine göre): Wavejackers, David Guetta, Martin Garrix, Anyma, Alok, Dimitri Vegas & Like Mike, Charlotte de Witte, Armin van Buuren, Timmy Trumpet, Fred again.., Carl Cox, Indira Paganotto, Mau P, Sara Landry, FISHER, Tiësto, Dom Dolla, Ummet Ozcan, Solomun, Miss Monique, Mahmut Orhan, HUGEL, Black Coffee. Her stilin 4 varyasyonu var.
  - 2.2'de eklenen stiller: Solomun (melodik house), Miss Monique (progressive melodik house), Mahmut Orhan (Hicaz makamlı oryantal deep house), HUGEL (latin/afro house: montuno piyano, tumbao bas, clave), Black Coffee (7'li akorlu soulful afro house). Her birine özel 3 imza ses var (toplam 16 yeni ses) ve yeni "Afro ve organik" grubu.
- **Ses tasarımı sekmesi.** Akor, bas ve melodi için ayrı ayrı:
  - 3 osilatör (testere, kare, üçgen, sinüs, wavetable; oktav, yarım ses, ince ayar, seviye), 12 wavetable ile A→B geçişi ve parlaklık, unison (7 sese kadar) ve açılım, sub ve gürültü.
  - Filtre (alçak/yüksek/bant geçiren, 12/24 dB), kesim, rezonans, zarf miktarı/atak/düşüş, tuş takibi, filtre sürücüsü.
  - Genlik zarfı (ADSR), 2 LFO (kesim, perde ya da ses; dört dalga biçimi, tempo kilidi).
  - Efektler: doygunluk, chorus, delay, reverb, seviye.
  - Dalga biçimi, filtre eğrisi ve zarf önizlemesi; ekran klavyesi (fare, bilgisayar klavyesi ya da MIDI klavye ile çalar).
  - "Rastgele ses" role uygun sınırlar içinde yeni bir ses kurar, "Başlangıca dön" seçili sesi geri getirir.
  - Toplam 153 ses tasarımı parametresi DAW'da otomasyona açıktır ve projeyle birlikte kaydedilir. Düğmeyi çevirdiğinde çalan notalar da anında değişir.
- **Sanatçı presetleri.** "Presetlerim" sekmesinde kendi sanatçı adın ve bir preset adıyla kaydet. Preset; stili, tüm üretici ayarlarını, dört sesi ve ses tasarımını saklar. Yükle, sil, dosyaya aktar, dosyadan al. Presetler stil listesindeki "Presetlerim" grubunda da görünür. Bir preset yüklüyken MIDI dosya adlarında senin sanatçı adın kullanılır.
  - Klasör: Windows `%APPDATA%\Wavejackers\HitGen\Presets`, macOS `~/Library/Wavejackers/HitGen/Presets`, Linux `~/.config/Wavejackers/HitGen/Presets`.
- Akor, bas, davul ve melodi üretir. Üretim ayarlarla belirlenir: aynı tohum ve ayar her zaman aynı MIDI'yi verir. Web sürümüyle birebir aynı motoru kullanır.
- **DAW tempo senkronu.** DAW çalınca eklenti desenini DAW'ın zamanıyla çalar. DAW durunca eklentideki Çal düğmesiyle önizleme yapılabilir.
- **MIDI'yi sürükle bırak.** İz kutularını (Akor .mid, Bas .mid…, Döngü, Tam parça) doğrudan DAW'a sürükle. Kutunun üzerinde Enter'a basarak ya da tıklayarak dosyaya da kaydedebilirsin.
- **MIDI çıkışı.** Üretilen notalar kanal 1 (akor), 2 (bas), 3 (melodi) ve 10'dan (davul) dışarı verilir. FL Studio'da MIDI çıkışı başka enstrümanlara yönlendirilebilir.
- **Klavye ile çalma.** DAW'dan gelen MIDI notaları seçili izin sesini çalar ("Klavye çalar" ayarı).
- **Ses motoru:**
  - 98 ses ve 10 davul kiti.
  - Bant sınırlı wavetable osilatörler, unison, 12/24 dB SVF filtre, formant ve LFO'lar.
  - Sidechain pump, chorus, delay ve reverb.
  - Master kompresör ve -1 dBFS tavanlı limiter.
- **Kalınlık.** Varsayılan değer 70: akorun kök sesini bir oktav altta katlar, synth'e alt oktav katmanı ekler. 60'ın üstünde melodi de katlanır.
- **Performans modları:**
  - **Eco:** unison en çok 2, chorus kapalı, kontrol hızı düşük.
  - **Dengeli:** varsayılan.
  - **Yüksek:** unison en çok 7.
  - Ses iş parçacığında bellek ayırma yapılmaz, NaN koruması vardır.
- **Diller:** English, Deutsch, Español, Türkçe, العربية (sağdan sola yerleşim), हिन्दी. Arapça ve Hintçe metinler için Noto yazı tipleri eklentinin içine gömülüdür; işletim sistemindeki yazı tiplerine bağlı değildir.
- **Erişilebilirlik:**
  - Tüm kontroller ekran okuyucu adı ve açıklaması taşır (Windows Narrator, NVDA, macOS VoiceOver).
  - Tam klavye gezinmesi ve görünür odak halkası vardır.
  - Yüksek kontrast teması, %75–200 arayüz boyutu ve piyano rulosunun metin özeti bulunur.
  - Kısayollar: Boşluk çal/durdur, N yeni fikir, Ctrl/Cmd+S döngüyü kaydet, Ctrl/Cmd+1/2/3 sekmeler.

## Kurulum

GitHub Actions bu depo her güncellendiğinde Windows, macOS ve Linux sürümlerini derler. **Actions → son çalıştırma → Artifacts** bölümünden indirebilirsin.

| Sistem | Dosya | Nereye kopyalanır |
|---|---|---|
| Windows | `VST3/Wavejackers HitGen.vst3` | `C:\Program Files\Common Files\VST3\` |
| macOS | `VST3/Wavejackers HitGen.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |
| macOS | `AU/Wavejackers HitGen.component` | `~/Library/Audio/Plug-Ins/Components/` |
| Linux | `VST3/Wavejackers HitGen.vst3` | `~/.vst3/` |

macOS'ta dosya imzasız olduğu için ilk açılışta uyarı çıkabilir. Terminalde şunu çalıştır:
`xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Wavejackers HitGen.vst3"`
Dağıtım yapacaksan Apple Developer ID ile imzalayıp noter onayı (notarization) almalısın.

### DAW'da kullanım

- **FL Studio:** Channel Rack → "+" → Wavejackers HitGen. İz kutusunu Playlist'e ya da Channel Rack'e sürükle. MIDI çıkışını başka bir enstrümana göndermek istersen eklenti penceresinin ayarlarında bir MIDI output port numarası seç, hedef enstrümanda aynı numarayı input port olarak ver.
- **Ableton Live:** Instruments → Plug-ins → Wavejackers HitGen. Kutuyu bir MIDI izine sürükle. Ableton enstrüman eklentilerinin MIDI çıkışını almaz; sürükle-bırak ya da kaydet kullan.
- **Logic Pro:** Software Instrument → AU Instruments → Wavejackers → HitGen.
- **Cubase, Reaper, Bitwig, Studio One:** Instrument izine VST3 olarak ekle. Kutuyu proje alanına sürükle.

## Kaynaktan derleme

Gerekenler: CMake 3.22+ ve bir C++17 derleyicisi (Visual Studio 2022, Xcode 15+ ya da GCC/Clang). JUCE 8.0.10 otomatik indirilir.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Testler:
- `HitGenGenTest` çıktısı `tests/reference.txt` ile birebir aynı olmalı. 2160 kombinasyonda web motoruyla aynı MIDI baytlarını doğrular.
- `HitGenGenTest` 23 stil için 2760 kombinasyonu denetler.
- `HitGenStressTest` şunları dener: 23 stil × 3 kalite, ses tasarımı dahil rastgele otomasyon, 44.1/48/96 kHz, 114 sesin parametrelere açılıp birebir geri kurulması, tutulan notalarda canlı düğme çevirme, preset kaydet/yükle, eski proje uyumu, durum kaydet/yükle ve 6 dilde üç sekmenin ekran görüntüsü.

`web/` klasörü tarayıcı sürümüdür (`web/build.sh` → `hitgen.html`). Stil, ses ve çeviri verisi buradan üretilir:
`node tools/export-data.js web Resources/hitgen-data.json`

## Lisanslar ve notlar

- JUCE 8: AGPLv3 ya da ticari JUCE lisansı. Kapalı kaynak satış için uygun JUCE lisansını al.
- Noto Sans yazı tipleri: SIL Open Font License (`Resources/fonts/OFL.txt`).
- VST, Steinberg Media Technologies GmbH'nin ticari markasıdır.
- **Sanatçı stilleri ilhamdır, kopya değildir.** Her stil o ismin genel üslubundan (tempo, armoni dili, ritim hissi) esinlenir. Hiçbir parçanın notası kullanılmaz ve adı geçen sanatçılarla bağlantı yoktur.

---

## English

Wavejackers HitGen is a MIDI generator and synth plugin: VST3 (FL Studio, Ableton Live, Cubase, Reaper, Bitwig, Studio One), AU (Logic, GarageBand) and Standalone.

- It has 23 artist-inspired styles with 4 variations each, including Solomun, Miss Monique, Mahmut Orhan (Hijaz scale), HUGEL and Black Coffee. Output is deterministic and matches the web version byte for byte.
- A full sound design tab for chords, bass and melody: 3 oscillators, 12 wavetables, unison, filter, envelope, 2 LFOs, effects, a preview and an on-screen keyboard. All 153 sound parameters can be automated.
- Artist presets: save the whole setup under your own artist name; load, delete, export and import `.hgpreset` files.
- It plays in sync with your DAW transport. You can also preview without the DAW.
- You can drag MIDI tiles straight into your DAW, or press Enter on a tile to save the file.
- The synth offers band-limited wavetables, SVF filters, sidechain pump, effects and a -1 dBFS limiter.
- Performance has three modes: Eco, Balanced and High. The audio thread never allocates.
- The interface is available in 6 languages, including right-to-left Arabic.
- Accessibility features:
  - screen-reader names for every control
  - full keyboard navigation
  - a high-contrast theme
  - UI scaling from 75 to 200%

Builds come from GitHub Actions (see Installation above). Build locally with CMake as shown.
Artist styles are inspiration only. No notes from any track are used, and there is no affiliation with the artists named.
