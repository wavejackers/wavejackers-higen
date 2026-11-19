// Synth X: HitGen'in genişletilmiş ses motoru (akor, bas, melodi). Bir ses JSON'u "xs" nesnesi taşıyorsa o ses bu motorla çalar;
// taşımıyorsa klasik motor (Synth.cpp) değişmeden çalar. Böylece kalibre edilmiş eski sesler aynı kalır.
//  - 3 osilatör, her biri: Mod (6 temel dalga), ROM (216 dalga ya da SoundFont örneği) ya da Multiwave (4 yığın x 8 ses)
//  - Sentez kipleri: çıkarımlı, FM, faz bozma (PD); dalga geçişi ve dalga yüzeyi (tabloyu tara); 4 darbe genişliği kipi
//  - Unison 1-12 ses, katman x2 (toplam 24); Multiwave 32 ses; 4 faz kipi
//  - Osilatör başına filtre (9 tür, ses hızında FM filtresi) ve 5 doygunluk kipi; anahtar ve hız aralıklı katman (gürültü/FX) osilatörü
//  - 4 ADSR (1. ses düzeyi), 2 LFO (24 dalga), 16 adımlı adım kontrolcüsü, 2 kaynaklı matris; her kaynak 5 hedefe
// Ses iş parçacığında bellek ayırma ve kilit yoktur: program ve tablolar mesaj iş parçacığında hazırlanır.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <memory>
#include <vector>

namespace xs
{
constexpr int kOsc = 3, kMaxPh = 32, kMaxUni = 12, kSlots = 5, kSteps = 16, kEnvs = 4, kLfos = 2, kMtx = 2;
constexpr int kBasic = 6, kLfoShapes = 24, kFilterTypes = 9, kSatModes = 6;

// ---- bant sınırlı tablo (oktav başına bir seviye)
struct Table
{
    static constexpr int kSize = 2048, kLevels = 11;
    std::array<std::vector<float>, kLevels> lv;
    inline const float* levelFor (double hz) const noexcept { int L = 0; double f = 40.0; while (L < kLevels - 1 && hz > f) { f *= 2.0; ++L; } return lv[(size_t) L].data(); }
};
int romCount();                                   // 216
juce::String romName (int i);
juce::String basicName (int i);                   // Saw, Square, Triangle, Sine, Pulse, Organ
const Table& basicTable (int i, double sr);       // mesaj iş parçacığında çağrılır; tablolar kalıcıdır (adres değişmez)
const Table& romTable (int i, double sr);
void waveShape (int bank, int i, std::vector<float>& out, int n);   // önizleme: bank 0 temel, 1 ROM

// ---- SoundFont (SF2): ilk preset'in örnek bölgeleri
struct SoundFont
{
    struct Zone { int keyLo = 0, keyHi = 127, velLo = 0, velHi = 127, root = 60; double cents = 0; int start = 0, end = 0, loopS = 0, loopE = 0; bool loop = false; double rate = 44100; };
    std::vector<Zone> zones; std::vector<float> data; juce::String name, path;
    const Zone* find (int key, int vel) const noexcept;
};
std::shared_ptr<const SoundFont> loadSoundFont (const juce::File&, juce::String& error);

enum Dest
{
    dOscBase = 0, kOscDests = 16,   // osilatör k: k*16 + (perde, ince, düzey, pan, geçiş, biçim, PW, detune, yayılım, FM, FM filtre, kesim, rezonans, sürüş, katman düzeyi, faz)
    dLayerLevel = 48, dLayerPitch, dLayerCut, dLayerRes,
    dPitch = 52, dAmp, dPan, dLfo1Rate, dLfo2Rate, dLfo1Amt, dLfo2Amt, dStepRate, dMultiMix, dNoise,
    kNumDests
};
juce::String destName (int d);
enum Source { sOff, sVel, sKey, sWheel, sAfter, sRand, sEnv1, sEnv2, sEnv3, sEnv4, sLfo1, sLfo2, sStep, sAudio, sAlpha, sMacro1, sMacro2, sMacro3, sMacro4, sMorphX, sMorphY, kNumSources };
juce::String sourceName (int s);
juce::String lfoShapeName (int s);
juce::String filterName (int t);
juce::String satName (int s);

struct Route { int dest = -1; float amt = 0; };
using Routes = std::array<Route, kSlots>;

constexpr int kFxSlots = 5, kFxTypes = 15;   // 0 boş
juce::String fxName (int t);
juce::String fxParamName (int t, int p);    // p 0-3
struct FxSlot { int type = 0; std::array<float, 4> p { { .5f, .5f, .5f, .5f } }; float mix = .5f; };
using FxChain = std::array<FxSlot, kFxSlots>;
FxChain fxFromVar (const juce::var&);
juce::var fxToVar (const FxChain&);

struct Program
{
    struct Filter { int type = 0; float cut = 18000, res = .1f, env = 0, kt = .3f, drive = 0, offset = 1; int sat = 0; };
    struct Osc
    {
        int type = 0;                 // 0 kapalı, 1 Mod, 2 ROM, 3 Multiwave
        int a = 0, b = 1;             // dalgalar (Mod/Multi: 0-5, ROM: 0-215, ROM a = -1: SoundFont)
        float morph = 0; int morphMode = 0;   // 0 geçiş, 1 dalga yüzeyi (a..b arasındaki tabloları tarar)
        float shape = 0; int synth = 0;       // 0 çıkarımlı (biçim = dalga katlama), 1 FM, 2 PD (biçim = faz bozma)
        float fm = 0, fmRatio = 1, fmFilter = 0;
        int pwMode = 0; float pw = .5f;       // 0 kapalı, 1 genişlik, 2 çift testere (gerçek PWM), 3 senkron
        int uni = 1; float detune = .2f, spread = .7f;
        int layer = 1; float layerSemi = 12, layerLevel = .5f;
        int phaseMode = 0; float phase = 0;   // 0 rastgele, 1 sıfırla, 2 eşit yay, 3 sabit
        int oct = 0, semi = 0; float fine = 0, level = .8f, pan = 0;
        int stacks = 4; std::array<int, 4> mw { { 0, 0, 0, 0 } }; std::array<float, 4> ml { { 1, .8f, .6f, .5f } }; float mix = .6f;   // Multiwave
        Filter f;
        std::array<const Table*, 2> tA {}, tB {};   // hazırlanmış tablolar (a, b); yüzey kipinde ara tablolar surf'te
        std::vector<const Table*> surf;
    };
    struct Layer { bool on = false; int wave = -1; int ftype = 0; float cut = 8000, res = .1f, level = .3f, pitch = 0; int keyLo = 0, keyHi = 127, velLo = 0, velHi = 127; const Table* t = nullptr; };
    struct Env { float a = .005f, d = .3f, s = .8f, r = .3f, curve = 0; };
    struct Lfo { int shape = 0; float rate = 2; bool sync = false, retrig = true; float fade = 0; Routes to {}; };
    struct Step { int n = 16; std::array<float, kSteps> v {}; int div = 4; float glide = 0; Routes to {}; };
    struct Mtx { int src = sOff; Routes to {}; };

    std::array<Osc, kOsc> osc {};
    Layer layer; std::array<Env, kEnvs> env {}; std::array<Routes, kEnvs> envTo {};   // envTo[0] kullanılmaz (1. zarf ses düzeyi)
    std::array<Lfo, kLfos> lfo {}; Step step; std::array<Mtx, kMtx> mtx {};
    float vol = .7f, glide = 0, bend = 2, keyPan = 0;
    FxChain fx {};
    struct Arp { bool on = false; int mode = 0, div = 2, oct = 1; float gate = .8f; } arp;   // mode: 0 yukarı 1 aşağı 2 yukarı-aşağı 3 rastgele 4 çalındığı sıra; div: 0 1/4, 1 1/8, 2 1/16, 3 1/8T, 4 1/16T, 5 1/32
    struct Chord { int mode = 0; float strum = 0; bool scaleLock = false; int split = 60, side = 0; int h1 = 0, h2 = 0; bool hKey = true; } chord;   // h1/h2: harmonizer (0 kapalı; hKey: gam adımı, değilse yarım ton)
    juce::String cat;   // program kategorisi (tarayıcı)   // side: 0 tüm klavye, 1 split altı, 2 split üstü (akor/arpej/sekans yalnız orada)
    int glideMode = 0, glideSync = 0;   // glideMode: 0 her nota, 1 yalnızca legato (otomatik); glideSync: 0 kapalı, 1-6 = 1/4..1/32
    struct SeqNote { float beat = 0, len = .25f; int rel = 0, vel = 100; };
    std::vector<SeqNote> seq; float seqBeats = 4; juce::String seqName;   // MIDI dosyasından sekans: tutulan tuşa göre aktarılır   // mode: 0 kapalı 1 üçlü 2 yedili 3 power 4 oktav 5 sus2 6 sus4; strum: 0..1 = 0..120 ms
    std::shared_ptr<const SoundFont> sf; juce::String sfPath; const Table* sine = nullptr;
    double sr = 48000;

    static std::shared_ptr<const Program> fromVar (const juce::var& v, double sampleRate);
    static juce::var toVar (const juce::var& v);   // normalleştirilmiş kopya (eksik alanlar varsayılanla)
    static juce::var init();
    float measureDb (int note, double secs) const;   // tek notanın en yüksek kısa süreli (50 ms) RMS düzeyi (dBFS, vol dahil)                        // başlangıç programı: tek testere, açık filtre
    float releaseTime() const noexcept { return env[0].r; }
};

// ---- ses (Engine'in her ses yuvası için bir tane; Synth.cpp yönetir)
struct Voice
{
    struct O
    {
        double ph[kMaxPh] {}, inc[kMaxPh] {}, pos[kMaxPh] {}; float rat[kMaxPh] {}, gl[kMaxPh] {}, gr[kMaxPh] {}; int st[kMaxPh] {};
        int n = 0; double mph = 0, minc = 0; bool dead[kMaxPh] {};
        float svf[2][2][2] {}, lad[2][4] {}, ladY[2] {};
        const SoundFont::Zone* zone = nullptr;
        // kontrol hızında hesaplanan değerler
        float level = 0, morph = 0, shape = 0, pw = .5f, fm = 0, fmFilter = 0, drive = 0, cut = 18000, res = 0, cut2 = 18000; double pitch = 1;
        const float* lvA = nullptr; const float* lvB = nullptr; const float* lvS[4] {}; int surfI = 0; float surfF = 0;
        float g1 = 0, k1 = 0, g2 = 0;
    };
    std::array<O, kOsc> o {};
    double t = 0, f0 = 440, glideFrom = 1; float vel = 0, key = 0, rnd = 0;
    int note = 60; bool released = false, done = false;
    std::array<float, kEnvs> ev {}; std::array<int, kEnvs> stage {};     // 0 atak 1 düşüş 2 sürdür 3 bırak
    std::array<double, kLfos> lph {}; std::array<float, kLfos> lHold {}, lNext {}, lOut {};
    double sph = 0; float sOut = 0;
    float lyPh = 0; float lySvf[2][2] {}; float lyPink[3] {}; uint32_t rng = 1;
    std::array<float, kNumDests> mod {};
    int ctl = 0;
};

struct Ctx { double sr = 48000, bpm = 120; float cutMul = 1, resAdd = 0, atkMul = 1, relMul = 1, drive = 0, lfoMul = 1, wheel = 0, after = 0, audioIn = 0, alpha = 0; std::array<float, 6> macro {}; int maxPh = 32, controlRate = 16; };
void start (Voice&, const Program&, int note, int vel, double glideFrom, uint32_t seed, const Ctx&);
void migrate (Voice&, const Program&, const Ctx&);   // canlı düzenleme: sesler yeni programla sürer
void release (Voice&) noexcept;
// L/R'ye ekler; ses bittiğinde v.done olur
void render (Voice&, const Program&, const Ctx&, float* L, float* R, int n) noexcept;
// Çevrilen seslerin seviyesi: en yüksek kısa süreli düzey parti hedefine (akor nota başına -24, bas ve melodi -18 dBFS) gelecek şekilde "vol"
float calibratedVolume (const juce::var& program, int part, double sampleRate);
}

// ---- 2. aşama: parti başına efekt rafı (5 yuva) ve master bölümü
namespace xs
{

class FxRack
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (const FxChain&, float* L, float* R, int n, double bpm) noexcept;
private:
    struct S
    {
        std::vector<float> buf[2]; int w = 0; double ph = 0, ph2 = 0; float z[2][8] {}, lp[2] {}, hold[2] {}; int cnt = 0;
        float ap[2][8] {}; float fb[2] {}; float svf[2][3][2] {}; float eq[2][3][2] {};
        juce::Reverb rev; int type = -1;
    };
    std::array<S, kFxSlots> s; double sr = 48000; int maxD = 0;
};

struct Master { float hp = 0, lp = 1, width = .5f, color = .5f, ceiling = 1, keyPan = 0; };
class MasterFx
{
public:
    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { for (auto& x : st) x = 0; }
    void process (const Master&, float* L, float* R, int n) noexcept;
private:
    double sr = 48000; float st[8] {};
};
}
