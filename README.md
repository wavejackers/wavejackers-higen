# Wavejackers HitGen 3.0

MIDI üretici ve synth eklentisi. VST3 (FL Studio, Ableton Live, Cubase, Reaper, Bitwig, Studio One), AU (Logic Pro, GarageBand) ve tek başına uygulama (Standalone) olarak çalışır.

*English below.*

## Neler yapar

- **24 sanatçı ilhamlı stil** (DJ Mag Top 100 2026 analizine göre; MoBlack 2.7'de eklendi): Wavejackers, David Guetta, Martin Garrix, Anyma, Alok, Dimitri Vegas & Like Mike, Charlotte de Witte, Armin van Buuren, Timmy Trumpet, Fred again.., Carl Cox, Indira Paganotto, Mau P, Sara Landry, FISHER, Tiësto, Dom Dolla, Ummet Ozcan, Solomun, Miss Monique, Mahmut Orhan, HUGEL, Black Coffee, MoBlack. Her stilin 4 varyasyonu var.
  - 2.2'de eklenen stiller: Solomun (melodik house), Miss Monique (progressive melodik house), Mahmut Orhan (Hicaz makamlı oryantal deep house), HUGEL (latin/afro house: montuno piyano, tumbao bas, clave), Black Coffee (7'li akorlu soulful afro house). Her birine özel 3 imza ses var (toplam 16 yeni ses) ve yeni "Afro ve organik" grubu.
- **Proje sekmesi (yeni):** 24 stilden birini seçip **Şablondan başla** ile tek tıkla yeni bir proje kur (stile uygun ton, makam, tempo, sesler ve yeni fikir). Seçili stilin **parça iskeleti** (Intro, Build, Drop, Breakdown, Outro; ölçü sayılarıyla) görünür. Projeni adla kaydet, sonra istediğinde aynı haliyle aç. **Tam parça** MIDI dosyası bölüm işaretçileri (Intro, Build, Drop...), tempo, ölçü ve ton bilgisiyle gelir ve her DAW'a sürüklenebilir: FL Studio, Ableton Live, Logic Pro, Cubase, Reaper, Bitwig, Studio One. Çok izli (Format 1) ve tek izli (Format 0) seçenekleri vardır; Format 0, çok izli dosyayı tek klibe indiren DAW'lar içindir.
- **Kendi sesin:** MIDI Studio'da her izin ses listesinin sonundaki "+ Kendi sesini yükle…" ile akor, bas ya da melodi için bir ses dosyası (WAV, AIFF, FLAC, MP3) yüklenir; nota perdesine göre çalınır (kök nota dosya adından, yoksa perde bulucudan). Synth preseti (Serum, FXP) seçeneği Ses Tasarımı'na, davul seçeneği Davul Makinesi'ne (yaratıcı mod) yönlendirir. Projeyle kaydedilir.
- **MIDI düzenleme:** "MIDI'yi düzenle" ile piyano rulosunda seçili iz tam boy açılır: tıkla ekle, sürükle taşı, sağ kenarı sürükle uzat, sağ tık/çift tık sil, tekerlek şiddet, oklar kaydır, Ctrl+Z/Ctrl+Y. Düzenleme çalmaya, .mid sürükle-bırak dışa aktarıma ve projeye girer; yalnızca tempo değişirse korunur, yeni fikir üretince "Geri al" ile döner.
- **Açılış ekranı:** HitGen açılınca gece zemininde "Welcome to Wavejackers Hit Generation" belirir, göz arayüzün üzerinde gezinip yerine oturur, sonra arayüz açılır. Ses DAW oturumu başına bir kez çalar; tıklama ya da tuş atlar; DAW çalarken ya da dışa aktarırken 20 ms içinde susar (kayda girmez). Ayarlar > Açılış sesi ve animasyonu ile kapatılabilir; hareket kapalıysa göz gezmez.
- **2.8'de yeni: göksel karakter teması (bilge ve melekler), Proje sekmesi (stil şablonları, kayıtlı projeler, DAW'a sürüklenen tam parça MIDI ve bölüm işaretleri), Özgünlük artık melodiye nota eklemiyor.**
- **2.7'de yeni: kulüp sesleri, davul makinesi, -14 LUFS, kılavuz ve asistan**
  - **Kulüp odaklı stiller:** Wavejackers artık indie dance / kulüp (126 BPM, sub ağırlıklı bas, kilitli groove); HUGEL Afro Latin tech house (montuno, tumbao, clave ve konga, 16'lık shaker). Yeni stil: **MoBlack** (tribal afro house). Solomun, Miss Monique, Mahmut Orhan ve Black Coffee kulübe göre yeniden ayarlandı.
  - **Kısa hook (motif) üretimi:** uzun melodiler yerine 2-5 notalık, tek ölçülük, tekrar eden ve her ikinci ölçüde "cevap veren" motifler. Web sürümüyle birebir aynı MIDI (3360 durumlu karşılaştırma testi).
  - **Elektronik melodi sesleri (Club Bank):** FM marimba, elektro çan pluck, analog pluck, lastik hook, kulüp org hook, acid hook, indie dance lead, afro mallet, oryantal pluck; deep kulüp stab, kulüp Rhodes, tribal akor ve iki bas. Özgün sentez; ticari preset ya da örnek kullanılmaz.
  - **Davul makinesi (DAVUL MAKİNESİ sekmesi):** Kick, Snare, Clap, Hi-hat, Açık hat, Rim ve Perküsyon için analog modellenmiş sentez (perde zarflı kick, iki tonlu snare, çoklu darbe clap, 808 tarzı metalik hat'ler, konga/tom). **Yaratıcı mod** açıkken her pede kendi örneğini yükle (sürükle-bırak ya da düğme; WAV, AIFF, FLAC, MP3, OGG; en çok 12 sn) ve **Punch, Soft, Mid, High**, Ton, Sönüm, Seviye ile şekillendir. Kitler `.hgkit` olarak kaydedilir (örnekler dosya yolu olarak). 49 ped ayarı DAW otomasyonuna ve MIDI Learn'e açıktır. Davul sayfası açıkken MIDI klavye pedleri çalar (GM: 36 kick, 38 snare, 39 clap, 42 hat, 46 açık hat, 37 rim).
  - **-14 LUFS çıkış:** ITU-R BS.1770 K-ağırlıklı ölçüm (son ~12 sn, kapılı), yavaş otomatik kazanç ve 4x aşırı örneklemeli tepe algılamalı, ileri bakışlı sınırlayıcı (gerçek tepe hedefi -1 dBTP). Başlıkta canlı LUFS göstergesi. Ayarlar'dan kapatılabilir. Bağımsız ölçüm (ffmpeg EBU R128, dokuz stil, 20 sn): -13.9 ile -14.0 LUFS, gerçek tepe en çok -1.0 dBTP. Sessizlikte kazanç artırılmaz. Ölçüm kayan bir pencere olduğu için bölüm geçişlerinde birkaç saniyelik yumuşak ayar duyulabilir.
  - **Kılavuz:** ilk açılışta 12 adımlı, alanları vurgulayan bir kılavuz (Yardım > Kılavuzu başlat ya da F1). 6 dilde.
  - **Asistan:** başlıktaki tek gözlü karakter, mevcut ayarlarına bakan kurallarla öneri verir (örneğin uzun melodi yerine kısa hook, afro stilde swing, -14 LUFS'u aç) ve "Uygula" ile ayarı değiştirir. İnternet ya da bulut kullanılmaz; bir dil modeli değildir.
  - Kurulum talimatları artık pakette `README.txt` dosyasında.
- **2.6'da yeni: MIDI Learn**
  - Herhangi bir düğmeye (MIDI Studio, Ses tasarımı) sağ tıkla, **MIDI Learn**'ü seç, MIDI kontrolcünde bir düğme ya da fader oynat. Eşlenen düğmenin üstünde "CC21" rozeti görünür.
  - Eşlemeyi değiştirmek için yeniden öğret; kaldırmak için sağ tık > Kaldır. Tüm liste: Ayarlar > MIDI eşlemeleri (temizle, yeni örnekler için varsayılan olarak kaydet / yükle).
  - Eşlemeler DAW projesiyle birlikte kaydedilir; preset dosyalarına girmez.
  - **Yakalama (soft takeover):** Bir parametre otomasyon, fare ya da preset ile değiştiyse kontrolcü, düğmeyi o anki değerin üzerinden geçirene kadar onu değiştirmez; böylece değer zıplamaz ve otomasyonla çakışmaz. Kontrolcü hareketi sunucuya otomasyon olarak bildirilir (yazma/dokunma modunda kaydedilebilir).
  - CC'ler ses iş parçacığında kilitsiz ve bellek ayırmadan işlenir; kanal modu mesajları (CC 120-127) eşlenmez. DNA makroları (Doku/Hareket/Parlaklık) DAW parametresi olmadığı için eşlenemez.
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
- **Proje iskeleti (Proje sekmesi, "Proje üret"):**
  - Yapı seçilir ya da elle kurulur: Intro, Buildup, Drop, Breakdown, Buildup, Drop, Outro varsayılandır. Bölüme tıklayınca türü değişir, 8 ölçü uzar ya da kısalır, taşınır, çoğaltılır ya da silinir.
  - Hedef süre seçilir (4:00 ile 10:00 arası ya da stil uzunluğu). Bölümler 8 ölçülük cümlelere dağıtılır; buildup en çok 16 ölçüdür. 8:00 ve 120 BPM tam 240 ölçü verir (Black Coffee projesinin uzunluğu).
  - Kurallar iki gerçek projeden çıkarıldı: Black Coffee "The Rapture Pt.III" Ableton remake (organik house: kick hiç kesilmez, breakdown'da high-pass ile incelir, bas çıkar, her bölüm sınırında ters ve düz downlifter, drop'tan önce reverb atışı, outro'da perküsyon katman katman çıkar) ve FRERE "Powerful" ile "Tiger" FL Studio projeleri (melodik techno: ilk 8 ölçü kicksiz, drop'ta her 8 ölçüde 4 ölçülük uplifter, breakdown başında impact ve sub drop, 16 ölçülük riser + sweep + davul buildup'ı, drop'tan önceki ölçüde vokal "go", son drop öncesi bir ölçü boşluk). Stilin türü (afro/house, techno/trance/hard, festival) hangi kuralların kullanılacağını seçer.
  - Her bölüm için MIDI ve her an için sample önerisi: sample klasörleri eklenince dosya adları okunur (kick, clap, hat, riser, uplifter, sweep, impact, sub drop, downlifter, noise, fill, vocal, ambience ve "124 BPM" gibi tempo bilgisi) ve tempoya yakın olanlar tercih edilir. Satırdaki çip sample'ı ya da bölümün MIDI'sini DAW'a sürükler. "Önerilen davulları yükle" Davul Makinesi pad'lerini doldurur.
  - "Proje klasörü oluştur": tam parça MIDI (bölüm ve ipucu işaretçileriyle), iz başına tam boy MIDI, bütün riser/impact/downlifter'ların yerleştiği FX Guide.wav (sample yoksa HitGen kendi sesleriyle üretir), önerilen sample'ların ölçü numarasıyla adlandırılmış kopyaları ve Plan.txt. Arka planda yazılır.
  - **FL Studio projesi (.flp):** "FL Studio projesi oluştur" aranjmanı otomatik doldurur. Açınca playlist hazırdır: her davul vuruşu (kick, clap, snare, hihat, open hat, rim, perc), bas, akor ve melodi kendi izinde, bölüm başına pattern olarak; riser, sweep, uplifter, impact, sub drop, downlifter ve vokal klipleri ipucu ölçülerinde; bölümler zaman işaretçisi olarak. Tüm kanallar FL'nin Sampler'ıdır (eklenti gerekmez): davullar sample klasörlerinden önerilen one-shot'larla ya da HitGen davul kitinin render'larıyla, bas/akor/melodi HitGen seslerinin C5'te render'ıyla çalar. Her kanal adını taşıyan kendi mikser kanalına gider. Biçim FL Studio 20.9 dosyalarından alındı ve PyFLP ile doğrulandı; FL Studio'nun kendisiyle açma testi yapılmadı.
  - İskelet projeyle saklanır. Algoritma web sürümüyle birebir aynıdır (eşlik testinde 576 iskelet durumu).
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

## HitGen 3.0: AI Producer

### Auto Producer (Proje sekmesi > "Auto Producer…")
Tür (Afro House, Melodic Techno, Progressive House, Indie Dance, Deep House), BPM, ton, makam, süre (4:00–10:00) ve enerji seçilir; "Parçayı üret" tek tıkla iskeleti, fikri, sesleri ve FX ipuçlarını kurar. Afro House → Black Coffee kuralları, Melodic Techno → FRERE kuralları; Progressive House ve Indie Dance için yeni tür stilleri eklendi (web sürümüyle birebir).

### Co-Producer (Proje sekmesi > "Co-Producer…")
Kendi MIDI'ni yükle ya da panele sürükle (melodi, akor veya bas; davul kanalı alınmaz). HitGen:
- rolünü tahmin eder (aynı anda çalan nota sayısı ve perde aralığı),
- ton adaylarını bulur: önce notaların makam içi oranı, eşitlikte Krumhansl-Kessler profili. Kısa döngülerde göreli majör/minör ayırt edilemeyebilir; bu yüzden en çok 4 aday tutulur,
- her aday tonda 48 fikir dener ve akorları notalarına en çok uyanı seçer (melodi: akor tonu oranı, akor: perde sınıfı örtüşmesi, bas: kök/akor tonu),
- eksik izleri tamamlar. Senin notaların kilitlidir: stil, tohum, uzunluk değişse ve proje yeniden açılsa da aynı kalır; tam parçada drop ve breakdown'larda çalar. "Başka uyum" sıradaki adayı, "Kilidi aç" HitGen'in kendi izini geri getirir.

### DAW çıktıları ("FL Studio projesi oluştur")
Proje klasörüne yazılanlar:
- `<ad>.flp`: dolu FL Studio projesi (Sampler kanalları, bölüm başına pattern'ler, FX klipleri, işaretçiler, adlı mikser kanalları). Dışa aktarma bitince FL Studio'da kendiliğinden açılır (.flp dosyası FL'ye bağlıysa); "FL Studio'da aç" düğmesi aynı projeyi yeniden açar.
- `<ad>.als`: dolu Ableton Live seti. Her şerit (Kick, Clap, Hat, Bass, Chords, Pad, Lead, Arp…) Simpler'lı bir MIDI izi ve FL projesindeki aynı sesle çalar; her bölüm ayrı MIDI klibi; riser/impact/downlifter gibi FX'ler ses izlerinde ipucu ölçülerinde; bölümler Locator; tempo ayarlı. Live 10 biçiminde yazılır, Live 11/12 açılışta dönüştürür. Max for Live gerekmez.
- `HitGen Arrangement.mid`: Type-1 çok izli MIDI, iz başına ad (Kick, Clap, Hat, Bass, Chords, Melody…), bölüm ve ipucu işaretçileri, tempo/ölçü/ton
- `Stems/`: tam şarkı WAV stem'leri (davullar ve her melodik şerit ayrı: Bass, Chords, Pad, Lead, Arp; 24 bit, diğer izler susturularak render; pump ve efektler karışımdakiyle aynı)
- `FL Piano Roll Scripts/*.pyscript`: Image-Line'ın resmi Piano Roll Scripting API'siyle iz başına notaları ve bölüm işaretçilerini açık piyano rulosuna yazar (`Belgeler\Image-Line\FL Studio\Settings\Piano roll scripts` klasörüne kopyala, Tools > Scripting)
- `manifest.json`: tempo, ton, bölümler (ölçü ve saniye), ipuçları, izler ve dosya eşlemesi (DAW köprüleri için tek kaynak)
- `MIDI/`, `Audio/FX Guide.wav`, `Samples/`, `Plan.txt`

Ableton Live için `<ad>.als` dosyasını aç. İstersen `HitGen Arrangement.mid` dosyasını aranjmana sürükleyip (her iz ayrı MIDI izi olur) `Stems/` klasöründeki WAV'ları 1. ölçüye de bırakabilirsin.

**Pad / Lead / Arp şeritleri:** aranjmanda akordan türetilen Pad (uzun akor sesleri, kök + üst ses) ve Arp (16'lık akor arpeji, drop ve groove bölümlerinde) şeritleri ile Lead (eski Melody) ayrı izlerdir; MIDI, stem, .flp ve .als hepsinde ayrı gelir.

**"Blessings" teknikleri (Liva K anlatımından, yalnızca afro stilleri):** aranjman çıktılarına üç kural eklendi. (1) *Stab* şeridi (yalnızca organic profil): ton kökünde seyrek, tresillo benzeri vuruşlar; ilk drop alçak oktavda, sonraki drop'lar bir oktav yukarıda, buildup'larda dört ölçüde bir ipucu. (2) *Seyrek perküsyon* (organic): Perc şeridinde ölçü başına en fazla 2 vuruş (en güçlüler) kalır. (3) *Stutter* şeridi: her buildup'ın son 4 ölçüsünde tek nota dörtlük, sekizlik, 16'lık, 32'lik diye hızlanır; aynı şeritte son 4-8 ölçüde pitch bend (+12, 16 ölçü ve üstü buildup'ta +24 yarım ton, RPN ile aralık ayarlı) ve CC74 yükselir, drop'ta sıfırlanır. Pitch bend ve CC yalnızca `HitGen Arrangement.mid` dosyasına yazılır; HitGen'in kendi motoru, stem'ler, FL Sampler ve Ableton Simpler bunları işlemez. Kendi synth'inde bend aralığını okuyup uygular.

**Tech House stili (yeni, Logic Pro tech house anlatımından):** Stil listesinde "Tech House" ve Auto Producer'da "Tech House" türü var. Kurallar: four-on-the-floor kick; clap ve hayalet rim (7. ve 15. adım); off-beat açık hat; swing'li 16'lık kapalı hat'ler, off-beat vurgulu; 1. 16'lığı kick'e bırakan, çoğu nota kök olan ve ölçü sonunda oktav/üçlü sıçraması yapan bas; kısa minör stab'lar; vokal chop hook'u; her dört ölçüde (8. ölçüde daha uzun) farklı çıkan bongo/konga tarzı perküsyon fill'i. Afro teknikleri (Stab, perküsyon inceltme) tech house'a uygulanmaz. Bu kurallar videodaki anlatımdan çıkarıldı; BPM ve ton videonun görsel analizinden tahmin edildi.

**Çok çıkış (multi-out):** eklentinin 4 ek stereo çıkışı var (Chords, Bass, Drums, Melody), varsayılan kapalı. DAW'da açınca o iz ana karışımdan çıkıp kendi çıkışına gider (ör. FL Studio: eklenti ayarları > Outputs; Ableton: izin girişini "HitGen > Bass" seç). Çıkışlarda -1 dBFS yumuşak sınırlayıcı var.

**FL Studio MIDI scripting neden yok:** FL'nin MIDI controller script API'si playlist'e pattern/klip yerleştiremez, sample yükleyemez; aranjmanı otomatik doldurmak için işe yaramaz. Bu yüzden aranjman doğrudan .flp dosyası olarak yazılıyor ve FL'de kendiliğinden açılıyor; piyano rulosu için ayrıca Piano Roll script'leri var.

### Doğrulama durumu (dürüst liste)
| Özellik | Otomatik test | Gerçek DAW'da denendi mi |
|---|---|---|
| Çok izli MIDI (iz adları, nota sayıları, işaretçiler) | Evet (JUCE ile geri okunur) | Hayır |
| Stem'ler (uzunluk, 24 bit, breakdown'da bas sessiz / drop'ta sesli) | Evet | Hayır |
| manifest.json (alanlar, işaret ettiği dosyalar var) | Evet | — |
| FL Piano Roll script'leri | Python sözdizimi + sahte flpianoroll modülüyle çalıştırma | Hayır (FL Studio'da çalıştırılmadı) |
| .flp projesi | Kendi okuyucumuz + PyFLP (FL 20.9 biçimi) | Hayır (FL Studio 2024/2025/2026'da açılmadı) |
| Co-Producer ton bulma | 98 durum: perde kümesi 98/98, gerçek ton adaylarda 98/98, tam ton (majör/minör) 62/98 | — |
| Co-Producer kilit (stil/tohum/uzunluk/proje yükleme) | Evet | — |
| Auto Producer (5 tür) | Evet (stil, tempo, ton, süre) | Dinlenerek değerlendirilmedi |
| .als Ableton seti | Kendi testimiz: gzip XML ayrıştırma, iz/klip/nota/Locator sayıları, tempo, benzersiz otomasyon kimlikleri, çakışan klip yok, her sample yolu diskte | Hayır (Ableton Live'da açılmadı) |
| Tech House stili (bas kick'e yer bırakır, off-beat açık hat, vurgu, hayalet rim, farklı fill'ler) + web/C++ eşliği (4428 durum) | Evet | Dinlenerek değerlendirilmedi |
| Pad / Arp / Lead şeritleri ve şerit başına stem'ler | Evet | Dinlenerek değerlendirilmedi |
| Stab, seyrek perküsyon, Stutter + pitch bend/CC74 | Evet (oktav kuralı, ölçü başına ≤2 perc, hızlanma, bend/RPN olayları MIDI'de) | Hayır, dinlenerek değerlendirilmedi |
| Çok çıkış (4 ek stereo bus) | Evet (bus düzeni, çıkışlarda ses, ana karışımda düşüş, tepe < -1 dBFS) + pluginval 10 | Hayır |
| Dışa aktarma sonrası FL'de otomatik açılma | Kod yolu var; Linux'ta açılacak FL olmadığından test edilmedi | Hayır |
| Ableton Max for Live cihazı, FL MIDI scripting | Yapılmadı: .als ve .flp dosyaları aynı işi eklentisiz yapıyor | — |
| Mix: pembe gürültü dengesi, gain staging (-6 dBFS mix tepe), rol bazlı pan | Evet (gürültü eğimi ve RMS, ölçüm doğrusallığı, tepe, kırpma yok, pan yasası) | Hayır, dinlenerek değerlendirilmedi |
| Factory 26 bankası (62 ses): ses yüksekliği, tepe, DC, nota başı tıklama, bas mono, mono toplam, kategori içi çeşitlilik | Evet (her ses demo cümlesiyle ölçülür) | Hayır, dinlenerek değerlendirilmedi |
| Ses çalma tıklaması, preset değiştirme, Movement makrosu, eski durum uyumu, Demo | Evet | Hayır |
| Yeni ses tasarımı sayfası (küre, makro yayları, tarayıcı, Demo) | Ekran görüntüleri + davranış testi (rol filtresi, sıra, parametre bağları) | Hayır (DAW içinde açılmadı) |

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
- New in 2.7: club-focused styles (Wavejackers as indie dance / club, HUGEL as Afro Latin tech house, new MoBlack tribal afro house style), a short-hook "motif" melody mode (2-5 notes, one bar, call and response; identical MIDI in the web version), an electronic Club Bank of melody sounds, a drum machine tab (seven analog-modelled pads, Creative mode with your own samples, Punch / Soft / Mid / High, Tune, Decay, Level, .hgkit kits), output normalised to -14 LUFS (BS.1770 gated measurement, slow auto gain, look-ahead limiter with 4x oversampled peak detection; ffmpeg EBU R128 cross-check over nine styles: -13.9 to -14.0 LUFS, true peak at most -1.0 dBTP), a 12-step first-run guide (F1) and a one-eyed assistant that gives rule-based suggestions from your current settings (no internet, not a language model). Installation instructions ship as README.txt.
- New in 2.6: MIDI Learn. Right-click a knob (MIDI Studio, Sound design), choose MIDI Learn and move a control on your MIDI controller; a "CC21" badge marks mapped knobs. Remove or re-learn from the same menu; Settings > MIDI mappings lists them all (clear, save/load as default for new instances). Mappings are stored with the DAW project, not in preset files. Soft takeover: after automation, the mouse or a preset changes a parameter, the controller only takes over once it passes the current value, so nothing jumps. CC handling on the audio thread is lock-free and allocation-free; channel mode messages (CC 120-127) are not mappable; the DNA macros are not DAW parameters and cannot be mapped.
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

### Celestial character theme
The whole editor uses the Wavejackers character art (sage with staff, two angels): night-blue background, ivory text, one gold accent, slightly translucent panels with the figures faintly behind them, a gold "staff orb" for the main sphere and the assistant, and a sage portrait in the header whose halo follows the output level (static when motion is off). High-contrast mode stays fully opaque and shows no figures. Art is owned by Wavejackers; transparent PNG cut-outs (~750 KB total) live in `Resources/characters/` and are scaled once when the editor opens, so painting does no resampling. Audio code is unchanged.

### Project skeleton
"Generate project" on the Project tab builds a full track from an editable section list (default Intro, Buildup, Drop, Breakdown, Buildup, Drop, Outro) and a target length (4:00 to 10:00, or the style's length). Sections are 8-bar phrases; 8:00 at 120 BPM is exactly 240 bars. Arrangement rules come from two real projects: Black Coffee "The Rapture Pt.III" (Ableton remake, organic house) and FRERE "Powerful" / "Tiger" (FL Studio, melodic techno). Every moment gets MIDI and, when sample folders are added, a sample suggestion picked from file names and tempo. "Create FL Studio project" writes a filled FL Studio project (.flp: Sampler channels with HitGen sounds or your samples, a pattern per part and section, FX audio clips at their cue bars, section markers, named mixer inserts), plus full-song and per-part MIDI, an FX guide WAV with every riser, impact and downlifter in place, copies of the suggested samples and a plan.

### HitGen 3.0 (English summary)
Auto Producer (genre, tempo, key, length, energy → a full track in one click; Afro House, Melodic Techno, Progressive House, Indie Dance, Deep House) and Co-Producer (load your MIDI; HitGen finds candidate keys, searches its ideas for the chords that fit your notes best, completes the other parts and keeps your notes locked). "Create FL Studio project" now also writes a filled Ableton Live set (.als: a Simpler MIDI track per lane, a clip per section, FX audio clips, locators; Live 10 format, untested in Live), separate Pad/Lead/Arp lanes, optional multi-out (4 extra stereo buses, off by default), opens the .flp in FL Studio automatically, and writes a Type-1 multi-track MIDI with named tracks and markers, full-song WAV stems, FL Piano Roll scripts and manifest.json. The .flp and the scripts follow FL Studio's documented/observed formats but have not been opened in FL Studio yet; see the verification table above.

## Mix: pembe gürültü referansı ve gain staging / Mix: pink-noise reference and gain staging

Dışa aktarılan proje artık dengelenmiş çıkar. Yöntem (Sound On Sound "pink noise reference", Produce Like A Pro): pembe gürültü (-3 dB/oktav, her oktavda eşit enerji) -23 dBFS RMS'de sabit referanstır; her ses gürültüyle birlikte dinlenip "gürültünün hemen üstünde duyulur" düzeye çekilir. HitGen bunu kulak yerine ölçerek yapar: her şeridin referans bölümdeki (ilk drop/groove, 8 ölçü) sesi 28 kritik (ERB) bantta gürültüyle karşılaştırılır, en güçlü bantları gürültünün +3 dB üstüne oturtulur, üstüne küçük bir tür dengesi (kick/bas önde, pad/perküsyon geride) eklenir. Sonra ortak bir kazançla mix tepe değeri -6 dBFS'e (master girişinde 3-6 dB boşluk) getirilir. Fader'lar 0 dB'de kalır; kazanç örneklere yazılır (clip gain). FX ses klipleri de aynı ölçütle ayarlanır (tepe en çok -3 dBFS).

Çıktılar: `Audio/Pink Noise Reference.wav` (aynı referans, kendiniz de kontrol edebilesiniz), `Mix Report.txt` (şerit başına duyulabilirlik, kazanç, tepe, RMS), `mixReport` ve `pinkNoiseReference` manifest anahtarları. Mikser fader/pan olayları FL'ye yazılmaz (gerçek FL'de açılamadığı için doğrulanamıyor); pan şerit rolüne göre örneğe gömülür (hat sağ 25, açık hat sol 30, perküsyon sağ 40, rim sol 35, arp sol 25, stab sağ 20; kick, bas, clap, akor, pad, lead ortada); her şeridin kendi mikser kanalı zaten var.

Doğrulama: pembe gürültü RMS ±0.2 dB ve oktav başına eşit enerji ±1 dB, duyulabilirlik ölçüsü doğrusallığı (+6 dB gürültü = -6 dB kazanç), dışa aktarımda mix tepe -6 dBFS ±0.3, örneklerde kırpma yok, rapor ve referans dosyası var. **Henüz kulakla dinlenmedi ve gerçek bir DAW'da açılmadı**; kayıtlı denge bir başlangıç noktasıdır, son kararı kulak verir.

## Wavejackers Factory 26: yeni fabrika ses bankası ve ses tasarımı sayfası / New factory bank and sound design page

**Banka.** Techno, Melodic Techno ve Progressive House için 62 yeni, tamamen özgün sentez ses (sample yok, üçüncü taraf preset kopyası yok). 11 kategori: Sub (5), Rolling bas (6), Acid (5), Lead (7), Pluck (6), Stab (6), Sekans (6), Pad (6), Doku (5), Metalik (5), FX (5: riser, impact, downlifter, sweep, zap). Her sesin özgün adı, karakter açıklaması ve kullanım bilgisi var (İngilizce ve Türkçe; diğer diller İngilizceye düşer). Eski 186 ses ve kimlikleri aynen duruyor; kayıtlı projeler ve kullanıcı presetleri aynı sesle açılır. Progressive House stili artık bu bankadan çalıyor (Horizon Pad, Velvet Drive, Prog Pluck).

**Motor (3.1).** Yeni yama alanları: glide (`gl`), dalga katlayıcı (`fold`), halka modülasyonu (`ring`), velocity → filtre zarfı (`vf`, acid aksanları), mono bas (`mono`: bu frekansın altında yan kanal 12 dB/oktav atılır), faz sıfırlama (`pr`, her bas notası aynı atakla). İz başına 5 Hz DC engelleyici. Ses çalma (voice steal) artık sesi kesmiyor: çalınan ses 5 ms'de söner, yeni nota boş yuvada başlar. Yeni `Movement` makrosu (`mov_chords`, `mov_bass`, `mov_melody`; sona eklendi, eski projelerde nötr açılır) LFO derinliğini ve filtre zarfını ölçekler. Parametre aralıklarının dışındaki preset değerleri (ör. 6 sn'lik riser filtresi) ilgili düğmeye dokunulana kadar korunur. Web sürümü glide, katlayıcı, halka modülasyonu ve velocity → filtreyi destekler; mono bas ve faz sıfırlama yalnızca eklentide.

**Ses tasarımı sayfası.** Ortada sesin dalga biçimini gösteren küre; çevresinde dört makro yayı: Parlaklık (`cut_`), Sürüş (`drv_`), Hareket (`mov_`), Alan (`rev_`). Üstte iz seçimi, kategori, önceki/sonraki, rastgele, başa dön, ADSR ve ses seviyesi; altta görünüm çubuğu (Makro / Tüm parametreler / Ses tarayıcısı), temel nota (oktav), ekran klavyesi, **Demo** ve İçe al. Demo, sesin kategorisine uygun iki ölçülük cümle çalar (sub, rolling 1/16, acid aksan ve kaymalı, lead, arpej, off-beat stab, sekans, pad akorları, doku, FX); MIDI çıkışına gitmez, ses iş parçacığında bellek ayırmaz.

**Ölçülen (otomatik testler).** Her ses demo cümlesiyle çalınır ve ölçülür: ses yüksekliği -18 LUFS ±1,5 (doku/FX -22), tepe < 0,98, DC < %2, nota başında tıklama yok, bas rolündeki sub/rolling/acid seslerinde 120 Hz altı yan/orta oranı < -30 dB, mono toplamda kayıp < 3 dB, aynı kategoride iki ses ölçülebilir biçimde farklı (1/3 oktav tını profili, atak, sürdürme, hareket, genişlik). Ayrıca: ses çalma tıklaması (düzeltme olmadan 0,23 sıçrama, şimdi < 0,05), notalar çalarken 124 preset değişikliği (NaN/patlama yok), Movement makrosu hareketi artırıyor, durum kaydet/yükle ve Movement'sız eski durumun nötr açılması, demonun durması ve MIDI'ye sızmaması, tarayıcı sırası ve rol filtresi. **Dinlenmedi:** seslerin müzikal kalitesi ve "profesyonel" duyulup duymadığı kulakla kontrol edilmedi; seviyeler ölçümle eşitlendi, karakter kararları kulak gerektirir.

*English:* 62 new original synthesized presets for Techno / Melodic Techno / Progressive House across 11 categories with descriptions and usage notes; engine additions (glide, wavefolder, ring mod, velocity→filter, mono-bass, phase reset, DC blocker, click-free voice stealing, Movement macro); a redesigned sound page (orb with four macro arcs, ADSR, browser, per-category demo phrases). Levels, DC, clicks, mono compatibility and variety are measured by tests; the sounds have not been listened to by a human yet.

## Hook melodisi, sürpriz sesler ve MIDI düzenleyicide boyama / Hook melody, surprise sounds, paint and erase

**Hook melodisi (web ve eklentide birebir aynı).** "Hook" kipindeki stiller (19 stil-varyasyon) artık ölçü ölçü rastgele yürüyüş yerine tek bir motif kurar: ritim ve çalan notalar bir kez seçilir, A-B-A-B' düzeninde tekrar eder. Tepe nota güçlü vuruşta ve tektir (%25 olasılıkla inen hook: tepe ilk notada). Sıçrama yalnızca tepeye çıkarken olur, sonra adımla iner. B (cevap) son notayı düşürür ve açık bir akor tonunda (üçlü/beşli) biter, B' bir derece aşağı dizilir, son ölçü kökte uzun biter. Motif her akorda ölçek içinde kaydırılır; aynı akor her seferinde aynı perdeyi verir. Testler: 192 durumda ölçü 1-2'nin 5-6'da birebir tekrarı, ölçü başına tek tepe, ölçü aralığı ≤ 14 yarım ses, her ölçü akor tonunda biter, son ölçü kökte biter. Özgünlük (originality) nota sayısını ya da aralığı artırmaz.

**Sürpriz sesler.** "Yeni fikir" artık elle seçilmemiş akor, bas ve melodi izlerine stilin türüne ve izin çalış biçimine uygun Factory 26 seslerinden yenilerini seçer (stab akor kipinde Stab/Pluck, techno basta Rolling/Acid/Sub, arpejde Seq/Pluck...). Elle seçtiğin ses korunur. Ses kutularında Factory 26 en üstte; "Beni şaşırt" seçeneği o ize tek seferde yeni bir kütüphane sesi verir. Stil değiştirmek stilin kendi seslerine döner.

**MIDI düzenleyici.** Boş yere sol tuşla basılı sürüklemek sağa/sola boyar (FL Studio'daki paint gibi: aynı uzunlukta, imlecin satırındaki perdeden, dolu hücreyi atlar). Sağ tık notayı siler; sağ tuş basılı sürüklemek yol boyunca tüm notaları siler (hızlı sürüklemede aradakiler de). Her boyama ya da silme hareketi tek adımda geri alınır. Not: bu yalnızca eklentide; web sürümünün düzenleyicisi değişmedi.

## Tür stilleri, riff/chop melodisi ve aday seçimi / Genre styles, riff and chop melodies, best-of-eight ideas

**Türkçe.** 30 yeni tür stili eklendi (toplam 57 stil, her biri 4 varyasyon). Üç yeni grup var: **Bass**, **Rap ve R&B**, **Chill**.

| Grup | Yeni stiller |
|---|---|
| Techno | Minimal, Acid Techno, Dub Techno |
| House | Deep House, Future House, Bass House, Nu-Disco, Piano House, Slap House |
| Afro | Amapiano |
| Ana sahne, Trance, Hard | Electro House, Psytrance, Uplifting Trance, Hardstyle, Hardcore |
| Bass | Liquid Drum & Bass, Neurofunk, Dubstep, Future Bass, UK Garage, Breakbeat |
| Chill | Lo-fi Hip-Hop, Downtempo, Synthwave |
| Rap ve R&B | Boom Bap, Trap, Drill, R&B, Trap Soul, Phonk |

- Yeni stiller listenin sonuna eklendi; eski projelerdeki stil seçimi değişmez.
- **Riff** melodileri artık her akorda gamın içinde kalır ve her ölçü aynen tekrar eder; yalnızca 4. ölçülerde tek nota değişir.
- **Chop** melodileri her vuruşa sabit bir akor sesi verir; aynı şekil her akorda tekrar eder (eskiden her ölçü rastgeleydi).
- **Yeni fikir** sekiz aday üretir ve en yüksek puanlıyı seçer (melodi aralığı, sıçramalar, tekrar, bas ile çakışma, akor sesinde bitiş). Yalnızca eklentide.
- Bass, Chill ve Rap/R&B stillerinde sürpriz sesler türün kendi paletinden gelir (808, Rhodes, çan, reese gibi).
- Sınırlar: trap hat ruloları (1/32) ve 808 kaymaları yok; davul kalıbı her ölçüde aynıdır. Kulakla ve DAW'da doğrulanmadı.

**English.** 30 new genre styles (57 total), three new groups (Bass, Rap & R&B, Chill). Riff melodies stay in key and repeat every bar; chop melodies keep one shape across chords. "New idea" generates eight candidates and keeps the best-scoring one (plugin only). Not yet verified by ear or in a DAW.

## Loop sekmesi (LoopForge) ve 24 yeni fabrika sesi / Loop tab and 24 new factory sounds

**Türkçe.**

**Loop sekmesi:** Tek vuruşluk bir sesten tempoya kilitli, dikişsiz döngü üretir.
- **Ses kaynağı:** WAV, AIFF ya da FLAC dosyasını sayfaya bırak ya da "Ses yükle"ye bas. "Yakala", HitGen'in son 30 saniyede çaldığı en güçlü vuruşu alır. Baş ve sondaki sessizlik otomatik kırpılır, sınırlar sıfır geçişine çekilir; dalga üstündeki tutamaklarla değiştirilebilir.
- **Ritim:** 16 ya da 32 adım. Vuruş ve Döndürme düğmeleri Öklid ritmi kurar; Swing ve İnsan dokunuşu zamanlamayı kaydırır.
- **Adım başına:** tetik, perde (seçili gamın içinde, ±1 oktav), ters çalma, ratchet (1-4), hız, filtre, pan, sönüm.
- **Üret / değiştir:** Küçük değişiklik, Tam Öklid, Melodik arpej.
- **Makrolar:** Alan (ping-pong delay ve reverb), Sürüş (doygunluk), Hareket (tempoya kilitli filtre ve genişlik).
- **Dikişsiz kuyruk:** Döngü iki turda işlenir; reverb ve delay kuyruğu döngü sonundan başa sarar.
- **Çalma ve dışa aktarım:** "Döngüyü çal" DAW konumuna kilitli çalar. "Döngüyü DAW'a sürükle" 24 bit WAV'ı (1, 2, 4 ya da 8 ölçü) doğrudan kanala bırakır. Duyulan döngü ile dosya aynı tampondur.
- **Proje:** Ayarlar ve ses dosyasının yolu projeyle kaydedilir.

- **Adım başına zaman kaydırma ve atak:** değer şeridinde "Zaman kaydırma" (±50 ms) ve "Atak" (2-80 ms) seçilebilir.
- **DAW otomasyonu:** Döngüyü çal, Vuruş, Döndürme, Swing, İnsan dokunuşu, Alan, Sürüş ve Hareket birer eklenti parametresidir (Loop ...).
- **DAW sesini yakalama:** Eklentide kapalı başlayan "Capture In" stereo girişi var. DAW'da bu girişe bir kanal yönlendirilirse Yakala o sesi kaydeder; bağlı değilse HitGen'in kendi çıkışını kaydeder. Girişin nasıl bağlanacağı DAW'a göre değişir (FL Studio'da mikser yönlendirmesi, Ableton'da sidechain).

Belgedeki tariften kalan farklar: arayüz WebView yerine HitGen'in kendi arayüzüyle yazıldı (ayrı eklenti yerine sekme istendi). Döngü başa sardığı için 5 ms'lik fade uygulanmıyor (fade döngü noktasında boşluk bırakırdı).

**Yeni sesler (Factory 26, toplam 86):** 808'ler (Clean, Dirt, Slide), baslar (Night Reese, Wobble, Log Drum, Slap Pluck, Felt Upright), tuşlular (Tine Keys, Dusty Keys, House Piano, Garage Organ), Tape Pad, Festival Saw, Future Chord, Tape Chord, lead'ler (Night Whistle, Hard Screech, Anthem Saw), Trap Bell, Cold Pizz, Future Pluck, Phonk Bell, Vocal Chop. Seviyeleri ölçülerek ayarlandı; yeni tür stillerinin varsayılan sesleri ve sürpriz paletleri bunları kullanıyor.

Doğrulanmayan: kulakla dinleme, DAW'a sürükleme (FL Studio, Ableton), macOS ve Windows'ta elle deneme.

**English.** New Loop tab: drop or capture a one-shot, build a Euclidean step pattern with per-step pitch (scale locked), reverse, ratchet, velocity, filter, pan and decay, shape it with Space / Drive / Motion, play it locked to the host and drag a seamless 24-bit WAV to the DAW (effect tails wrap around). Per-step timing shift (±50 ms) and attack; Loop macros and Euclid settings are automatable; an optional "Capture In" input (off by default) lets Capture record DAW audio. 24 new factory sounds for rap, R&B, bass music, house and hard styles. Not verified by ear or in a DAW.
