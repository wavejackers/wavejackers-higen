// Wavejackers HitGen ses motoru: bant sınırlı wavetable osilatörler, SVF filtre, davul sentezi,
// sidechain pump, chorus/delay/reverb, master kompresör + yumuşak limiter.
// Ses iş parçacığında bellek ayırma yoktur; yamalar ve desenler hazır nesneler olarak değiştirilir.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace hg
{
class Data;

// ---- Bant sınırlı tablo (oktav başına bir seviye)
struct WaveTable
{
    static constexpr int kSize = 2048, kLevels = 11;
    std::array<std::vector<float>, kLevels> lv;
    // harmonik genlikleri (sinüs serisi, 1. eleman temel) ile tablo seviyelerini hazırlar
    void build (const std::vector<float>& harm, double sampleRate);
    inline const float* levelFor (double hz) const noexcept
    {
        int L = 0; double f = 20.0 * 2.0;
        while (L < kLevels - 1 && hz > f) { f *= 2.0; ++L; }
        return lv[(size_t) L].data();
    }
};

struct Patch
{
    juce::String id, cat;
    struct Osc { int wave = 0; float detune = 0, octave = 0, gain = 0; };   // wave: 0 testere 1 kare 2 üçgen 3 sinüs 4 wavetable
    std::array<Osc, 3> osc {}; int numOsc = 0;
    int uniN = 1; float uniSpread = 0;
    int ftype = 0; float cut = 8000, q = .7f, envAmt = 0, envAtk = 0, envDec = .2f; bool slope24 = false;
    float A = .01f, D = .2f, S = .7f, R = .3f;
    float wtA = 0, wtB = 3, wtMorph = .3f, wtBright = 1;
    struct Lfo { float rate = 1, depth = 0; int target = 0, shape = 0; bool sync = false; };   // target: 0 kesim 1 perde 2 ses
    std::array<Lfo, 3> lfo {};
    float rv = 0, dl = 0, ch = 0, dr = 0, fd = 0, vol = .2f, kt = .5f, trim = 1, noi = 0, sub = 0;
    bool hasFm = false; float fmRatio = 1, fmIndex = 1, fmDecay = .2f, fmSus = .25f, fmLevel = 1;
    bool hasPe = false; float peSemi = 0, peTime = .1f;
    bool hasVib = false; float vibRate = 5, vibCents = 0, vibDelay = 0;
    int vowel = -1; bool male = false; bool chordsHalf = false;
    WaveTable table;   // wavetable osilatörleri için (A/B/geçiş/parlaklık)
    static std::unique_ptr<Patch> fromVar (const juce::var& v, const Data& data, double sampleRate);
};

// Kullanıcının parça başına makro ayarları (otomasyonlanabilir parametrelerden okunur)
struct Macros { float cutoff = 0, reso = 0, attack = 0, release = 0, drive = 0, reverb = 0, delay = 0, volume = 1; bool mute = false; };

struct Quality { int maxUnison = 4, controlRate = 16; bool chorus = true; };

class Engine;

// ---- Ezgisel ses (akor, bas, melodi)
struct Voice
{
    bool active = false, released = false, gate = false;
    int note = 0, track = 0, uniN = 1, ctl = 0;
    float vel = 0, peak = 0, env = 0;
    double f0 = 440, t = 0, relT = 1e9, endT = 1e9, startClock = 0;
    double ph[3][7] {}, inc[3][7] {};
    float panL[7] {}, panR[7] {}, uniDet[7] {};
    double fmCar = 0, fmMod = 0, subPh = 0, thPh = 0, vibPh = 0, lfoPh[3] {};
    float sv[2][2][2] {};       // [kanal][aşama][durum] SVF
    float fsv[2][3][2] {};      // formant bant geçirenleri
    float nz1 = 0, nz2 = 0;     // gürültü HP
    uint32_t rng = 1;
    float cf[4] {}, cf2[4] {}, cfF[3][4] {}, cfN[4] {};   // SVF katsayıları (a1, a2, a3, k)
    float ampLfo = 1, fmDev = 0, carMul = 1;
    float cA = 0, cD = 0, cR = 0; double atk = .01;   // zarf katsayıları
    const float* lv[3] {}; const float* lvTh = nullptr;
    const Patch* patch = nullptr;   // ses değişince eski sesler eski yamayla doğal biter
};

struct DrumVoice
{
    bool active = false; int type = 0; float vel = 0; double t = 0, len = 0, ph = 0, ph2[6] {};
    uint32_t rng = 7; float s[4][2] {}; float cfA[4] {}, cfB[4] {};
};

class Engine
{
public:
    Engine();
    ~Engine();
    void prepare (double sampleRate, int maxBlock);
    void reset();

    // Mesaj iş parçacığından: yeni yama / kit (ses iş parçacığı bir sonraki blokta alır)
    void setPatch (int track, std::unique_ptr<Patch> p);
    void setKit (const juce::var& kit);
    void setPump (int track, float depth, float release) { pumpDepth[(size_t) track] = depth; pumpRel[(size_t) track] = release; }
    void collectGarbage();   // eski yamaları mesaj iş parçacığında siler

    // Ses iş parçacığı
    void noteOn (int track, int note, int vel, double durSec, int offset);
    void noteOff (int track, int note, int offset);
    void drumHit (int note, int vel, int offset);
    void allNotesOff();
    void render (juce::AudioBuffer<float>& out, int start, int num);

    // Blok başında ayarlanır
    std::array<Macros, 4> macros;
    float bpm = 126, thick = .7f, pumpAmt = 1, master = 1;
    Quality quality;
    std::atomic<float> peakOut { 0 }, cpuLoad { 0 };
    int activeVoices() const noexcept { return lastActive; }

private:
    double sr = 44100; int maxBlockSize = 512;
    WaveTable basic[4];    // testere, kare, üçgen, sinüs
    std::array<std::atomic<Patch*>, 4> pending {};
    std::array<Patch*, 4> cur {};
    std::array<Patch*, 16> retiring {};   // hâlâ çalan seslerin kullandığı eski yamalar
    juce::AbstractFifo trashFifo { 64 }; std::array<Patch*, 64> trash {};
    std::unique_ptr<Patch> owned[4];   // yalnızca mesaj iş parçacığı dokunur
    std::array<Voice, 64> voices;
    std::array<DrumVoice, 24> drums;
    struct Kit { float kf0 = 160, kf1 = 50, kpd = .04f, kd = .32f, kclick = .3f, kdrv = .5f, sf = 190, std_ = .1f, snf = 2400, snd = .16f, smix = .55f, cnf = 1400, cd = .22f,
                 hhp = 7500, hd = .045f, hm = .25f, ohp = 6500, od = .28f, om = .3f, rf = 1800, pf = 330, pd = .15f, lp = 0, rv = .08f; int ptype = 0; };
    Kit kit, pendingKit; std::atomic<bool> kitDirty { false };
    std::array<float, 4> pumpDepth {}, pumpRel {}, pumpG { 1, 1, 1, 1 };
    std::array<int, 4> pumpWait {};
    float pumpTarget = 1;
    // efekt durumları
    std::vector<float> chorusBuf[3][2]; int chorusPos[3] {}; double chorusPh[3][2] {};
    std::vector<float> dlBuf[2]; int dlPos = 0; float dlLp[2] {};
    juce::Reverb reverb; juce::dsp::Compressor<float> comp;
    float drumLpS[2] {};
    juce::AudioBuffer<float> partBuf, sendRv, sendDl, drumBuf;
    int lastActive = 0; double voiceClock = 0;

    void startVoice (Voice& v, const Patch& p, int track, int note, int vel, double durSec);
    void renderVoice (Voice& v, const Patch& p, const Macros& m, float* L, float* R, int n);
    void renderDrum (DrumVoice& d, float* L, float* R, int n);
    void applyChorus (int track, float amt, float* L, float* R, int n);
    Voice* findVoice (int track);
    void retireNow (Patch* p);
};
}
