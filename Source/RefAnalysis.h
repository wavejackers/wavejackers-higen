// Referans ses: içe aktarma, DSP analizi (tempo, ton, onset, bölge adayları) ve Track DNA.
// Tüm hesaplar arka plan iş parçacığında yapılır; iptal bayrağı desteklenir. Ses iş parçacığında çalışmaz.
// Dürüstlük notu: "bölge adayı" miksin o bölgede o sesin baskın olduğu yeri gösterir; sesi miksten ayırmaz (stem ayrıştırma yok).
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

namespace hg
{
constexpr int kRefVersion = 1;                   // analiz algoritması sürümü (DNA profiline yazılır)
constexpr double kRefMaxSeconds = 360.0;         // bellek sınırı: daha uzun dosyalar bu süreyle kırpılır

// Kaynağın özgün örnekleme hızında, en çok 2 kanallı ses
struct RefClip
{
    juce::AudioBuffer<float> audio;
    double sampleRate = 44100.0;
    juce::String name, path;
    bool truncated = false;
    int numSamples() const noexcept { return audio.getNumSamples(); }
    double seconds() const noexcept { return sampleRate > 0 ? audio.getNumSamples() / sampleRate : 0.0; }
};

// WAV, AIFF, FLAC, OGG ve (derlemede açıksa) MP3. Hata olursa nullptr ve error doldurulur.
std::shared_ptr<RefClip> loadRefAudio (const juce::File& f, juce::String& error, const std::atomic<bool>* cancel = nullptr);

enum RefCat { catBass, catLead, catPad, catPerc, kNumRefCats };
const char* refCatKey (int c);     // "bass", "lead", "pad", "perc"

struct RefRegion
{
    double t0 = 0, t1 = 0;     // saniye
    int cat = catBass;
    float score = 0;           // 0..1 sezgisel baskınlık puanı (olasılık değildir)
    float confidence = 0;      // 0..1: puan ile ikinci en iyi kategori arasındaki farktan türetilir
    float lowFrac = 0, midFrac = 0, highFrac = 0, harmonic = 0, onsetsPerSec = 0, centroidHz = 0, width = 0, rmsDb = -100;
};

struct RefAnalysis
{
    bool ok = false;
    double durationSec = 0, analysisSr = 0;
    double bpm = 0; float bpmConf = 0;
    int key = 0; bool minor = true; float keyConf = 0;    // key: 0=C ... 11=B
    int keyAlt = 0; bool keyAltMinor = false;             // ikinci en iyi aday (ör. bağıl majör/minör)
    std::vector<float> onsets;                            // saniye
    float harmonicRatio = 0, percussiveRatio = 0, centroidHz = 0, flatness = 0, widthAll = 0, rmsDb = -100;
    std::vector<RefRegion> regions;                       // kategoriye göre azalan güvenle sıralı
    juce::String notes;                                   // kullanıcıya gösterilecek uyarılar
    int version = kRefVersion;
    std::vector<RefRegion> top (int cat, int maxN = 4) const;
};

// progress: 0..1; false dönerse iptal
bool analyzeRef (const RefClip& clip, RefAnalysis& out, const std::atomic<bool>* cancel = nullptr, const std::function<void (float)>& progress = {});

// ---------------------------------------------------------------- Track DNA
struct TrackDna
{
    int version = kRefVersion;
    juce::String name, artist, sourceFile, role;          // role: bass, lead, pad, perc
    double regionStart = 0, regionEnd = 0, sourceSr = 0;
    juce::String created;
    // ölçülen özellikler (hepsi 0..1 ya da açıkça birimli)
    std::vector<float> envDb;          // 16 bantlık log aralıklı spektral zarf (dB, tepe = 0)
    std::vector<float> harmonics;      // f0 tahmini varsa 1..8. harmoniklerin göreli genliği (0..1)
    float f0Hz = 0; bool pitched = false; float pitchConf = 0;
    float centroidHz = 0, rolloffHz = 0, brightness = 0, warmth = 0, lowFrac = 0, subFrac = 0;
    float oddEven = 1;                 // tek/çift harmonik oranı (>1: kare/üçgen benzeri)
    float harmonicSlope = 0;           // harmonik başına dB düşüş (yaklaşık)
    float attackMs = 0, decayMs = 0, sustainLevel = 0, releaseMs = 0, crest = 0;
    float noise = 0;                   // 0..1 gürültü içeriği (spektral düzlük + harmonik olmayan enerji)
    float width = 0;                   // 0..1 stereo genişlik (yan/orta)
    float ampModHz = 0, ampModDepth = 0, brightModDepth = 0;
    float onsetsPerSec = 0, ioiRegularity = 0;
    float pitchRangeSemis = 0, pitchWobble = 0;           // perde hareketi
    float brightTrend = 0, energyTrend = 0;               // -1..1 zaman içindeki eğilim
    juce::var toVar() const;
    static bool fromVar (const juce::var& v, TrackDna& out);
};

bool extractDna (const RefClip& clip, double t0, double t1, int role, TrackDna& out, const std::atomic<bool>* cancel = nullptr);

// DNA -> HitGen ses tasarımı parametreleri (SoundParams'taki Sp sırası).
//  similarity 0..100: 0 = yalnızca role uygun rastgele ses, 100 = tamamen DNA eşlemesi (ölçülmüş bir benzerlik yüzdesi DEĞİLDİR).
//  variation  0..100: DNA hedefi etrafında sınırlı, tohumlu oynatma miktarı.
//  Aynı (dna, rol, similarity, variation, seed) her zaman aynı değerleri üretir.
void dnaToSoundParams (const TrackDna& dna, char role, float similarity, float variation, juce::uint32 seed, float* outVals);
}
