# Wavejackers HitGen 2.5

MIDI üretici ve synth eklentisi. VST3 (FL Studio, Ableton Live, Cubase, Reaper, Bitwig, Studio One), AU (Logic Pro, GarageBand) ve tek başına uygulama (Standalone) olarak çalışır.

*English below.*

## Neler yapar

- **23 sanatçı ilhamlı stil** (DJ Mag Top 100 2026 analizine göre): Wavejackers, David Guetta, Martin Garrix, Anyma, Alok, Dimitri Vegas & Like Mike, Charlotte de Witte, Armin van Buuren, Timmy Trumpet, Fred again.., Carl Cox, Indira Paganotto, Mau P, Sara Landry, FISHER, Tiësto, Dom Dolla, Ummet Ozcan, Solomun, Miss Monique, Mahmut Orhan, HUGEL, Black Coffee. Her stilin 4 varyasyonu var.
  - 2.2'de eklenen stiller: Solomun (melodik house), Miss Monique (progressive melodik house), Mahmut Orhan (Hicaz makamlı oryantal deep house), HUGEL (latin/afro house: montuno piyano, tumbao bas, clave), Black Coffee (7'li akorlu soulful afro house). Her birine özel 3 imza ses var (toplam 16 yeni ses) ve yeni "Afro ve organik" grubu.
- **2.5'te yeni: iş akışı yardımcıları**
  - **DNA Orb stüdyosu (ÜRET):** yüklenen sesin tamamı bir halka olarak çizilir (dalga formu, BPM'den vuruş çentikleri, Smart Detect bölgeleri, çalma ibresi, gerçek çıkış seviyesi). Halkaya tıklamak o noktaya atlar.
  - **6 VARYASYON:** aynı DNA'nın altı tohumu yan yana; her kutu gerçek ses parametrelerinden bir parmak izi gösterir. Tıkla dinle, Tut ile kullan (oklar ve 1-6 da çalışır).
  - **Geri al / yinele:** üretim, geçmiş ve DNA makroları için Ctrl/Cmd+Z ve Ctrl/Cmd+Shift+Z (ÜRET sekmesinde).
  - **Bara hizala:** dalga formunda sürüklenen seçim algılanan tempoda tam barlara oturur (ızgara ilk vuruştan başlar; ölçü başı tahmindir). Serbest seçim için Alt.
  - **Ritim MIDI:** seçimin (yoksa tüm dosyanın) davul MIDI taslağını DAW'a sürükle-bırak. Kick / snare / hi-hat tam miks üzerinde kaba bir bant ve sönüm tahminiyle ayrılır; kesin bir transkripsiyon değildir.
  - **Klasörü analiz et:** kütüphanede bir klasördeki tüm ses dosyalarını (en çok 60) arka planda analiz eder, her birinin en iyi bas / lead / pad / vurmalı bölgesini DNA profili olarak kaydeder; iptal edilebilir.
  - **Otomatik etiketler:** DNA profilleri ölçülen özelliklerden etiket alır (karanlık, parlak, sıcak, pluck, geniş, gürültülü, hareketli, sub...). Arama etiketlerde de çalışır.
- **AI Minimal arayüz:** koyu antrasit + lavanta paleti, ince başlık ve dört çalışma alanı: REFERENCE, SMART DETECT, DNA ENGINE, GENERATE (MIDI Studio, Sound design ve Presets ikincil sekmeler olarak duruyor). Texture / Movement / Brightness yalnızca üretilmiş sesin gerçek parametrelerini (gürültü+sürücü, LFO 1 derinliği, filtre kesimi) değiştirir ve ses üretildikten sonra açılır. Durum göstergesi (Ready / Processing / Completed / Error) gerçek işlemden beslenir. Ses motoru, MIDI, parametre kimlikleri ve preset biçimi değişmedi. Kısayollar: Ctrl/Cmd+1-7 sayfalar, Esc ayarları kapatır. Pencere: Compact / Default / Expanded (sabit en-boy oranı korunur; yaklaşık 900x578, 1100x708, 1400x902).
- **2.4'te yeni: Referans sekmesi (Smart Detect ve Track DNA):**
  - **Ses içe aktarma:** WAV, AIFF, MP3, FLAC, OGG dosyasını pencereye sürükle ya da "Ses aç" ile seç. Dalga formu, yakınlaştırma/kaydırma (fare tekerleği), bölge seçimi (sürükle) ve orijinali çalma (döngü, ses seviyesi) vardır. Ses yalnızca bu bilgisayarda kalır; internet gerekmez. 6 dakikadan uzun dosyalar bellek için kırpılır.
  - **SMART DETECT:** tempo (BPM), ton ve makam (güven değeriyle), vuruş başlangıçları ve bas / synth-lead / pad / vurmalı için aday bölgeler. **Dürüst not:** adaylar tam miksin içinde o ses türünün baskın olduğu yerlerdir; ses miksten ayrıştırılmaz (stem ayrıştırma yoktur) ve güven değeri olasılık değil sezgisel bir puandır. "ORİJİNALİ ÇAL" her zaman tam miksi çalar. Ton tespiti özellikle kısa ya da tek enstrümanlı kayıtlarda belirsiz olabilir; arayüz bunu "ton belirsiz" diye söyler.
  - **BENZERİNİ ÜRET:** seçili bölgeden Track DNA çıkarır (spektral zarf, harmonikler, parlaklık, atak/decay/sustain, gürültü, stereo genişlik, modülasyon, ritim, perde hareketi) ve HitGen sentezinin parametrelerine eşler. Örneği çalmaz; yeni bir synth sesi kurar. **DNA benzerliği** (0–100): 0 = role uyan herhangi bir ses, 100 = ölçülen özellikleri synth'in izin verdiği kadar izle; ölçülmüş bir benzerlik yüzdesi değildir. **Varyasyon** (0–100): her parametrenin hedef çevresinde ne kadar sapabileceği. Aynı DNA + ayar + tohum her zaman aynı sesi verir; her tıklamada tohum bir artar. Geçmiş (< >), A/B karşılaştırma ve otomatik MIDI cümlesiyle dinleme vardır. Sonuç Akor / Bas / Melodi izine yazılır, MIDI klavyeyle çalınır ve projeyle kaydedilir.
  - **Örnekleyici:** seçili bölgeyi MIDI ile çal: kök nota, transpoze, ince ayar, ADSR, velocity duyarlılığı, 16 sese kadar polifoni, kırpma, döngü, `.hgsamp` preset. Perde klasik yeniden örnekleme ile değişir (yüksek nota = kısa süre); süreyi koruyan perde kaydırma yoktur.
  - **Kütüphane:** DNA profillerini sanatçı etiketi ve adla kaydet, ara, favorile, yeniden adlandır, sil, içe/dışa aktar (`.hgdna`), son kullanılanlar. Telifli kayıt ya da hazır sanatçı profili paketlenmez; "sanatçı" senin girdiğin etikettir.
  - **HIZLI mod:** tek tık: ses aç → SMART DETECT → en güvenli bölge → BENZERİNİ ÜRET → dinle. Kısayollar (Referans sekmesinde): Boşluk orijinali çal, G üret, [ ] geçmiş, A A/B, Ctrl/Cmd+4 sekme.
  - Proje kaydı referans dosyanın yolunu ve örnekleyici ayarlarını saklar; dosya taşınmışsa arayüz "dosya bulunamadı" der ve yolu korur.
- **2.3'te yeni:**
  - **Animasyonlu, renkli arayüz** (eklenti ve web): akan renk bulutları, parlayan başlık, çalarken nabız atan Çal düğmesi, canlı çıkış seviyesi göstergesi. Kalite "Eco" ise ya da "Animasyon" kapalıysa hareket durur; web'de sistemin "hareketi azalt" ayarına uyar.
  - **Başka öner.** Her izin (akor, bas, davul, melodi) yanındaki ↻ düğmesi yalnızca o izi farklı bir fikirle değiştirir, diğer üçü aynı kalır.
  - **Yeni fikir** artık tohumla birlikte ton, makam, tempo, enerji, özgünlük, insan hissi, swing ve varyasyonu stile uygun sınırlarda birlikte değiştirir.
  - **Studio ses bankası:** 14 yeni dolgun ses (Studio Super Chord vb.) ve ince kalan eski varsayılanların yerine geçen yoğun katmanlı sesler. Sanatçı stillerine özel ayarlar (Solomun, Miss Monique, Mahmut Orhan, HUGEL, Black Coffee).
  - **Güvenli ses seviyesi:** çıkış her zaman -1 dBFS'in altında kalır (en yüksek ayarda ölçüldü: -1.002 dBFS). Web'de de aynı tavan var.
  - **FXP / FXB / Serum yükleme (eklenti, Ses tasarımı sekmesi):** Serum 2 `.SerumPreset` dosyasındaki osilatör, filtre, zarf ve efekt ayarları okunup yaklaşık olarak aktarılır. Serum 1, Synth1 ve diğer VST2 `.fxp/.fxb` dosyalarının iç yapısı kapalıdır; bunlarda yalnızca ad ve kategoriden en yakın yerleşik ses seçilir. Yalnızca sahip olduğun dosyaları yükle. Ticari ses paketleri bu depoya dahil değildir.
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
- New in 2.5: DNA Orb studio (the whole reference drawn as a ring from real data), 6 VARIATIONS grid (six seeds of one DNA with a parameter fingerprint each; click to listen, Keep to use), undo/redo for generation, history and DNA macros (Ctrl/Cmd+Z, Ctrl/Cmd+Shift+Z), snap-to-bars selection (grid starts at the first onset, so the downbeat is an estimate; hold Alt for a free selection), Rhythm MIDI drag-and-drop (a drum sketch from detected onsets; kick/snare/hat are a rough band-and-decay guess on the full mix, not a transcription), folder batch analysis into the DNA library (up to 60 files, background, cancellable) and automatic library tags derived from measured DNA features (searchable).
- AI Minimal UI: dark anthracite and lavender palette, slim header, four workspaces (Reference, Smart Detect, DNA Engine, Generate) with MIDI Studio, Sound design and Presets kept as secondary tabs. Texture / Movement / Brightness edit only the generated sound's real parameters (noise and drive, LFO 1 depth, filter cutoff) and unlock after a sound is generated. The status indicator follows real work. DSP, MIDI, parameter IDs and the preset format are unchanged. Shortcuts: Ctrl/Cmd+1-7 pages, Esc closes settings. Three window sizes keep a fixed aspect ratio (about 900x578, 1100x708, 1400x902).
- New in 2.4 (Reference tab): import WAV/AIFF/MP3/FLAC/OGG by drag-and-drop or file browser, waveform with zoom, region selection and looped original playback; SMART DETECT (tempo, key with confidence, onsets, candidate bass / lead / pad / percussion regions). Candidates are regions of the full mix where a sound type dominates, not separated stems, and the confidence is a heuristic score, not a probability. GENERATE SIMILAR extracts a Track DNA profile from the selected region and maps it onto HitGen's synth (it does not replay the sample); DNA Similarity (0 = any sound that fits the role, 100 = follow the measured traits as far as the synth allows) and Variation (how far parameters may wander around the target) are documented controls, not a measured similarity percentage; the same DNA + settings + seed always gives the same sound. Also: a MIDI sampler (root note, transpose, ADSR, velocity, up to 16 voices, trim, loop, presets; pitch changes by resampling, there is no duration-preserving pitch shift), a DNA library (search, favourites, rename, delete, import/export, recents; no copyrighted audio or artist profiles are bundled), a one-click QUICK mode, variation history and A/B compare. Analysis runs on a background thread and can be cancelled; everything stays on your computer.
- New in 2.3: animated, colourful UI (plugin and web, with an animations switch and reduced-motion support), a live output meter, a per-track "Another idea" button (↻) that replaces only that track, a richer "New idea", 14 full-bodied Studio sounds, a hard -1 dBFS output ceiling, and an FXP/FXB/Serum 2 importer in the plugin's sound design tab. Serum 2 presets are read approximately; closed VST2 formats (Serum 1, Synth1, ...) only pick the closest built-in sound. Load files you own; no commercial sound packs are included.
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
