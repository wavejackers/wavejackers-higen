// Davul makinesi: yedi ped (kick, snare, clap, kapalı hat, açık hat, rim, perküsyon).
//  - Her ped analog modellenmiş sentezle çalar (808/909 çizgisinde: perde zarflı sinüs kick, iki tonlu snare,
//    çoklu darbe clap, altı kare osilatörlü metalik hat'ler, perde düşüşlü konga/tom) ya da kullanıcının yüklediği örneği çalar.
//  - Ped kanalı: Seviye, Ton (yarım ses), Sönüm, Punch (vuruş/geçici), Soft (yumuşaklık), Mid ve High (EQ).
//  - Ses iş parçacığı bellek ayırmaz, kilit almaz. Örnekler mesaj iş parçacığında çözülür ve atomik işaretçiyle devredilir;
//    eski örnekler çöp kuyruğundan mesaj iş parçacığında silinir.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace hg
{
enum DrumPad { kPadKick = 0, kPadSnare, kPadClap, kPadHat, kPadOpenHat, kPadRim, kPadPerc, kNumPads };
extern const char* const kPadIds[kNumPads];          // "kick", "snare", "clap", "hat", "ohat", "rim", "perc"
extern const int kPadNotes[kNumPads];                 // GM: 36, 38, 39, 42, 46, 37, 64
int padForNote (int note) noexcept;                   // -1: davul notası değil

struct PadControls
{
    float level = 1.0f;     // 0..1.5 doğrusal
    float tune = 0.0f;      // yarım ses -12..12
    float decay = 1.0f;     // 0.25..2 çarpan
    float punch = 0.5f;     // 0..1 (0.5 nötr)
    float soft = 0.0f;      // 0..1
    float mid = 0.0f;       // dB -12..12
    float high = 0.0f;      // dB -12..12
};

struct DrumSample
{
    juce::AudioBuffer<float> buf;     // 1 ya da 2 kanal, tepe 0.95'e normalize
    double rate = 44100; juce::String path, name;
};

// Stil kitinin analog parametreleri (veri dosyasındaki MB.KITS ile aynı alanlar)
struct AnalogKit
{
    float kf0 = 160, kf1 = 50, kpd = .04f, kd = .32f, kclick = .3f, kdrv = .5f;
    float sf = 190, std_ = .1f, snf = 2400, snd = .16f, smix = .55f;
    float cnf = 1400, cd = .22f, hhp = 7500, hd = .045f, hm = .25f, ohp = 6500, od = .28f, om = .3f;
    float rf = 1800, pf = 330, pd = .15f, lp = 0, rv = .08f; int ptype = 0;
};

class DrumMachine
{
public:
    DrumMachine();
    ~DrumMachine();
    void prepare (double sampleRate);
    void reset();

    // ---- mesaj iş parçacığı
    void setKit (const AnalogKit& kitIn) { pendingKit = kitIn; kitDirty.store (true, std::memory_order_release); }
    void setSample (int pad, std::unique_ptr<DrumSample> s);   // nullptr: analog sese dön
    void collectGarbage();
    void audition (int pad, int vel = 110) noexcept;            // arayüzden ped çalma (ses iş parçacığı bir sonraki blokta çalar)

    // ---- ses iş parçacığı
    std::array<PadControls, kNumPads> ctl;                      // blok başında parametrelerden doldurulur
    bool creative = false;                                      // kapalıyken kontroller nötr ve örnekler devre dışı
    int hit (int note, int vel) noexcept;                       // çalınan ped (-1: yok)
    void render (float* L, float* R, int n) noexcept;          // davul veriyolu tamponuna ekler
    std::atomic<int> lastHit { -1 };                            // arayüz ped ışığı için
    const AnalogKit& kit() const noexcept { return k; }
    bool hasSample (int pad) const noexcept { return cur[(size_t) pad] != nullptr; }   // yalnızca ses iş parçacığı

    static PadControls neutral() { return {}; }

private:
    struct Biq { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; float z1[2] {}, z2[2] {}; };
    struct Voice
    {
        bool active = false; int pad = 0; float vel = 0; double t = 0, len = 0, ph = 0, ph2[6] {}, sp = 0, srate = 1;
        uint32_t rng = 7; float s[3][2] {}; float cfA[4] {}, cfB[4] {};
        const DrumSample* smp = nullptr; float fade = 1.0f, fadeStep = 0.0f;
        Biq mid, high; float lpS[2] {}, lpC = 1.0f, dcX[2] {}, dcY[2] {};
        PadControls c; float punchAmt = 0, softAtk = 0;
    };
    double sr = 44100;
    AnalogKit k, pendingKit; std::atomic<bool> kitDirty { false };
    std::array<Voice, 32> voices;
    std::array<const DrumSample*, kNumPads> cur {};
    std::array<std::atomic<DrumSample*>, kNumPads> pending {};
    std::array<std::atomic<bool>, kNumPads> pendingSet {};
    juce::AbstractFifo trashFifo { 32 }; std::array<DrumSample*, 32> trash {};
    std::atomic<uint32_t> auditionMask { 0 }; std::array<std::atomic<int>, kNumPads> auditionVel {};
    uint32_t clock = 1;
    void retire (const DrumSample* s) noexcept;
    void startVoice (Voice& v, int pad, float vel) noexcept;
    float synth (Voice& v) noexcept;
};
}
