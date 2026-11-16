#include "SynthX.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <map>
#include <tuple>

namespace xs
{
namespace
{
constexpr double kTau = juce::MathConstants<double>::twoPi, kPi = juce::MathConstants<double>::pi;
inline double frac (double x) noexcept { return x - std::floor (x); }
inline double sawW (double p) { return 1 - 2 * frac (p); }

// ---------------------------------------------------------------- dalga kütüphanesi
// 27 aile x 8 değişke = 216 ROM dalgası. Her biri zaman alanında tanımlanır, FFT ile harmoniklere ayrılır ve oktav başına
// bant sınırlı tabloya dönüştürülür (en yüksek harmonik Nyquist'in %90'ının altında kalır).
const char* const kFam[27] = { "Saw-Square", "Pulse", "Tri-Saw", "Sine Fold", "Drive", "Sync Saw", "Sync Square", "Sync Sine", "Reso Saw", "Reso Tri", "Reso Pulse",
                               "Chebyshev", "Tilt", "Odd-Even", "Organ", "Vowel", "Formant", "Bit Sine", "Stair Saw", "Digital", "Noise Cycle", "Comb Saw",
                               "Double Saw", "Exp Ramp", "Partial", "Rectified", "Tri Fold" };
const float kOrgan[8][9] = { { 1, .8f, 0, .6f, 0, .4f, 0, 0, 0 }, { 1, 1, 1, 0, 0, 0, 0, 0, 0 }, { .8f, 0, .8f, 0, .8f, 0, .6f, 0, .4f }, { 1, .5f, .3f, .5f, .2f, .3f, .1f, .2f, .1f },
                             { 0, 1, 0, 1, 0, 1, 0, 1, 0 }, { 1, 0, 0, 0, 0, 0, 0, 0, 1 }, { 1, .6f, .6f, .6f, .6f, .6f, .6f, .6f, .6f }, { .6f, 1, .4f, 0, .3f, 0, 0, .2f, 0 } };
const int kDraw[9] = { 1, 2, 3, 4, 6, 8, 10, 12, 16 };
const float kVowelF[8][3] = { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 }, { 570, 840, 2410 }, { 300, 870, 2240 }, { 660, 1720, 2410 }, { 400, 1600, 2300 }, { 350, 1700, 2100 } };

double rnd (uint32_t& s) { s = s * 1664525u + 1013904223u; return (double) (s >> 8) / 16777216.0; }

double romSample (int fam, int v, double p)
{
    const double t = v / 7.0, s = std::sin (kTau * p);
    switch (fam)
    {
        case 0: return (1 - t) * sawW (p) + t * (p < .5 ? 1 : -1);
        case 1: return p < .5 - .45 * t ? 1 : -1;
        case 2: { const double k = .5 + .49 * t; return p < k ? -1 + 2 * p / k : 1 - 2 * (p - k) / (1 - k); }
        case 3: return std::sin (s * (1 + t * 6) * kPi * .5);
        case 4: { const double d = 1 + t * 9; return std::tanh (s * d) / std::tanh (d); }
        case 5: return sawW (p * (1 + t * 3.5));
        case 6: return frac (p * (1 + t * 3.5)) < .5 ? 1 : -1;
        case 7: return std::sin (kTau * frac (p * (1 + t * 3.5)));
        case 8: return (1 - p) * (1 - std::cos (kTau * p * (1 + t * 7))) - .5;
        case 9: return (1 - std::abs (2 * p - 1)) * (1 - std::cos (kTau * p * (1 + t * 7))) - .5;
        case 10: return (p < .5 ? 1.0 : 0.0) * (1 - std::cos (kTau * p * (2 + t * 7))) - .5;
        case 11: return std::cos ((2 + v) * std::acos (juce::jlimit (-1.0, 1.0, s * .98)));
        case 12: { double y = 0; for (int n = 1; n <= 64; ++n) y += std::pow (n, -(.4 + t * 2.2)) * std::sin (kTau * n * p); return y; }
        case 13: { double y = 0; for (int n = 1; n <= 64; ++n) y += (n % 2 ? 1.0 : t) / n * std::sin (kTau * n * p); return y; }
        case 14: { double y = 0; for (int k = 0; k < 9; ++k) y += kOrgan[v][k] * std::sin (kTau * kDraw[k] * p); return y; }
        case 15: { double y = 0; for (int n = 1; n <= 40; ++n) { double a = 0; for (int f = 0; f < 3; ++f) a += (f == 0 ? 1 : .6 / f) * std::exp (-std::pow ((n * 110.0 - kVowelF[v][f]) / 120.0, 2)); y += (a + .04 / n) * std::sin (kTau * n * p); } return y; }
        case 16: { double y = 0; const double h = 2 + v * 4; for (int n = 1; n <= 48; ++n) y += (std::exp (-std::pow ((n - h) / 1.6, 2)) + .08 / n) * std::sin (kTau * n * p); return y; }
        case 17: { const double L = 2 + v * 2; return std::round (s * L) / L; }
        case 18: { const double st = 3 + v * 2; return std::floor (p * st) / (st - 1) * 2 - 1; }
        case 19: { uint32_t r = 9001u + (uint32_t) v * 77u; double y = 0; for (int n = 1; n <= 48; ++n) { const double a = rnd (r), sg = rnd (r) < .5 ? -1 : 1; y += sg * a * a / std::sqrt ((double) n) * std::sin (kTau * n * p); } return y; }
        case 20: { uint32_t r = 4242u + (uint32_t) v * 131u; double pts[16]; for (auto& x : pts) x = rnd (r) * 2 - 1; const double q = p * 16; const int i = (int) q % 16; const double f = q - std::floor (q), a = pts[i], b = pts[(i + 1) % 16]; return a + (b - a) * (.5 - .5 * std::cos (kPi * f)); }
        case 21: { double y = 0; const int k = 2 + v; for (int n = 1; n <= 64; ++n) if (n % k) y += std::sin (kTau * n * p) / n; return y; }
        case 22: return .5 * (sawW (p) + sawW (p + .05 + t * .45));
        case 23: { const double k = 1 + t * 12; return (std::exp (-k * p) - std::exp (-k)) / (1 - std::exp (-k)) * 2 - 1; }
        case 24: return s + .7 * std::sin (kTau * (v + 2) * p);
        case 25: return std::pow (std::abs (s), .5 + t * 2) * 2 - 1;
        default: { const double tr = p < .5 ? 4 * p - 1 : 3 - 4 * p; return std::sin (tr * (1 + t * 5) * kPi * .5); }
    }
}
double basicSample (int i, double p)
{
    switch (i)
    {
        case 0: return sawW (p);
        case 1: return p < .5 ? 1 : -1;
        case 2: return p < .5 ? 4 * p - 1 : 3 - 4 * p;
        case 3: return std::sin (kTau * p);
        case 4: return p < .25 ? 1 : -1;
        default: { double y = 0; const int h[5] = { 1, 2, 3, 4, 6 }; const double a[5] = { 1, .7f, .5f, .35f, .25f }; for (int k = 0; k < 5; ++k) y += a[k] * std::sin (kTau * h[k] * p); return y; }
    }
}

std::unique_ptr<Table> buildTable (int bank, int idx, double sr)
{
    constexpr int NA = 4096, NS = Table::kSize;
    std::vector<float> an ((size_t) NA * 2, 0.0f);
    for (int i = 0; i < NA; ++i) { const double p = (i + .5) / NA; an[(size_t) i] = (float) (bank == 0 ? basicSample (idx, p) : romSample (idx / 8, idx % 8, p)); }
    juce::dsp::FFT fa (12); fa.performRealOnlyForwardTransform (an.data());
    auto T = std::make_unique<Table>(); juce::dsp::FFT fs (11); std::vector<float> sy ((size_t) NS * 2);
    double norm = 0;
    for (int L = 0; L < Table::kLevels; ++L)
    {
        const int hmax = juce::jlimit (1, NS / 2 - 1, (int) std::floor (sr * .45 / (40.0 * std::pow (2.0, L))));
        std::fill (sy.begin(), sy.end(), 0.0f);
        for (int k = 1; k <= hmax; ++k)
        {
            const float re = an[(size_t) k * 2], im = an[(size_t) k * 2 + 1];
            sy[(size_t) k * 2] = re; sy[(size_t) k * 2 + 1] = im;
            sy[(size_t) (NS - k) * 2] = re; sy[(size_t) (NS - k) * 2 + 1] = -im;
        }
        fs.performRealOnlyInverseTransform (sy.data());
        auto& t = T->lv[(size_t) L]; t.assign ((size_t) NS + 1, 0.0f);
        if (L == 0) { for (int i = 0; i < NS; ++i) norm = std::max (norm, (double) std::abs (sy[(size_t) i])); if (norm < 1e-12) norm = 1; }
        for (int i = 0; i < NS; ++i) t[(size_t) i] = (float) (sy[(size_t) i] / norm);
        t[(size_t) NS] = t[0];   // doğrusal aradeğerleme için koruma örneği
    }
    return T;
}

juce::CriticalSection tableLock;
std::map<std::tuple<int, int, int>, std::unique_ptr<Table>>& tables() { static std::map<std::tuple<int, int, int>, std::unique_ptr<Table>> m; return m; }
const Table& tableFor (int bank, int idx, double sr)
{
    const juce::ScopedLock sl (tableLock);
    auto& m = tables(); const auto key = std::make_tuple (bank, idx, (int) std::lround (sr));
    auto it = m.find (key); if (it != m.end()) return *it->second;
    auto& slot = m[key]; slot = buildTable (bank, idx, sr); return *slot;
}

inline float lookup (const float* t, double p) noexcept
{
    const double x = p * Table::kSize; const int i = (int) x; const float f = (float) (x - i);
    return t[i] + (t[i + 1] - t[i]) * f;
}

// ---------------------------------------------------------------- SoundFont okuyucu
struct Chunk { const char* id; const uint8_t* d; uint32_t n; };
inline uint16_t u16 (const uint8_t* p) { return (uint16_t) (p[0] | (p[1] << 8)); }
inline uint32_t u32 (const uint8_t* p) { return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24); }
std::map<juce::String, std::weak_ptr<const SoundFont>>& sfCache() { static std::map<juce::String, std::weak_ptr<const SoundFont>> m; return m; }

// LFO dalgaları (faz 0..1, döngü başına iki rastgele değer)
float lfoWave (int s, double p, float a, float b) noexcept
{
    const float x = (float) p, sn = std::sin ((float) kTau * x);
    switch (s)
    {
        case 0: return sn;
        case 1: return x < .5f ? 4 * x - 1 : 3 - 4 * x;
        case 2: return 2 * x - 1;
        case 3: return 1 - 2 * x;
        case 4: return x < .5f ? 1.0f : -1.0f;
        case 5: return x < .25f ? 1.0f : -1.0f;
        case 6: return x < .125f ? 1.0f : -1.0f;
        case 7: return a;
        case 8: return a + (b - a) * (.5f - .5f * std::cos ((float) kPi * x));
        case 9: return (std::exp (4 * x) - 1) / (std::exp (4.0f) - 1) * 2 - 1;
        case 10: return (std::exp (4 * (1 - x)) - 1) / (std::exp (4.0f) - 1) * 2 - 1;
        case 11: return std::abs (sn) * 2 - 1;
        case 12: return sn * sn * (sn < 0 ? -1.0f : 1.0f);
        case 13: return std::floor (x * 4) / 3 * 2 - 1;
        case 14: return std::floor (x * 8) / 7 * 2 - 1;
        case 15: return juce::jlimit (-1.0f, 1.0f, (x < .5f ? 4 * x - 1 : 3 - 4 * x) * 2);
        case 16: return std::abs (std::cos ((float) kPi * x * 3)) * (1 - x) * 2 - 1;
        case 17: { const float tr = x < .5f ? 4 * x - 1 : 3 - 4 * x; return tr * std::abs (tr); }
        case 18: return (sn + .5f * std::sin ((float) kTau * 2 * x)) / 1.3f;
        case 19: { float y = .5f + .4f * a; for (int i = 0; i < (int) (x * 12); ++i) y = 3.9f * y * (1 - y); return y * 2 - 1; }
        case 20: return std::log1p (x * 15) / std::log (16.0f) * 2 - 1;
        case 21: return 1 - std::log1p (x * 15) / std::log (16.0f) * 2;
        case 22: return (x < .1f || (x >= .25f && x < .35f)) ? 1.0f : -1.0f;
        default: return std::round (sn * 8) / 8;
    }
}

const double kSyncBeats[12] = { 32, 16, 8, 4, 2, 1, 2.0 / 3, .5, 1.0 / 3, .25, 1.0 / 6, .125 };   // döngü başına vuruş
inline double lfoHz (const Program::Lfo& l, double bpm) { return l.sync ? bpm / 60.0 / kSyncBeats[juce::jlimit (0, 11, (int) l.rate)] : (double) l.rate; }

float destRange (int d)
{
    if (d < 48) { const int j = d % 16; return j == 0 ? 24.0f : j == 1 ? 1.0f : j == 11 ? 5.0f : 1.0f; }
    if (d == dLayerPitch || d == dPitch) return 24.0f;
    if (d == dLayerCut) return 5.0f;
    return 1.0f;
}

inline float saturate (float x, int mode, float drive) noexcept
{
    if (mode == 0 && drive <= 0) return x;
    const float g = 1 + drive * 9; x *= g;
    switch (mode)
    {
        case 2: return juce::jlimit (-1.0f, 1.0f, x) / std::sqrt (g);
        case 3: { const float y = x >= 0 ? std::tanh (x) : std::tanh (x * .6f) / .6f * .8f; return y / std::sqrt (g); }
        case 4: return std::sin (x * 1.2f) / (1 + drive);
        case 5: return (std::tanh (x) * .6f + std::tanh (std::abs (x)) * .4f) / std::sqrt (g);
        default: return std::tanh (x) / std::tanh (std::sqrt (g));
    }
}

struct Svf { float lp, bp, hp; };
inline Svf svfTick (float x, float g, float k, float* s) noexcept
{
    const float a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const float v3 = x - s[1], v1 = a1 * s[0] + a2 * v3, v2 = s[1] + a2 * s[0] + a3 * v3;
    s[0] = 2 * v1 - s[0]; s[1] = 2 * v2 - s[1];
    return { v2, v1, x - k * v1 - v2 };
}
inline float gOf (double hz, double sr) { return (float) std::tan (kPi * juce::jlimit (16.0, sr * .45, hz) / sr); }
}

// ---------------------------------------------------------------- adlar
int romCount() { return 216; }
juce::String romName (int i) { i = juce::jlimit (0, 215, i); return juce::String (kFam[i / 8]) + " " + juce::String (i % 8 + 1); }
juce::String basicName (int i) { static const char* n[6] = { "Saw", "Square", "Triangle", "Sine", "Pulse", "Organ" }; return n[juce::jlimit (0, 5, i)]; }
const Table& basicTable (int i, double sr) { return tableFor (0, juce::jlimit (0, kBasic - 1, i), sr); }
const Table& romTable (int i, double sr) { return tableFor (1, juce::jlimit (0, 215, i), sr); }
void waveShape (int bank, int i, std::vector<float>& out, int n)
{
    out.resize ((size_t) juce::jmax (1, n)); const auto& t = bank == 0 ? basicTable (i, 48000) : romTable (i, 48000);
    for (int k = 0; k < n; ++k) out[(size_t) k] = lookup (t.lv[0].data(), (double) k / n);
}
juce::String destName (int d)
{
    static const char* o[16] = { "Pitch", "Fine", "Level", "Pan", "Morph", "Shape", "PW", "Detune", "Spread", "FM", "FM filter", "Cutoff", "Resonance", "Drive", "Layer level", "Phase" };
    static const char* g[13] = { "Layer level", "Layer pitch", "Layer cutoff", "Layer resonance", "Pitch", "Volume", "Pan", "LFO 1 rate", "LFO 2 rate", "LFO 1 amount", "LFO 2 amount", "Step rate", "Multi mix" };
    if (d < 0) return "-";
    if (d < 48) return "Osc " + juce::String (d / 16 + 1) + " " + o[d % 16];
    return d - 48 < 13 ? g[d - 48] : "Noise";
}
juce::String sourceName (int s) { static const char* n[kNumSources] = { "Off", "Velocity", "Key", "Mod wheel", "Aftertouch", "Random", "Env 1", "Env 2", "Env 3", "Env 4", "LFO 1", "LFO 2", "Step" }; return n[juce::jlimit (0, kNumSources - 1, s)]; }
juce::String lfoShapeName (int s)
{
    static const char* n[kLfoShapes] = { "Sine", "Triangle", "Saw up", "Saw down", "Square", "Pulse 25", "Pulse 12", "Sample & hold", "Smooth random", "Exp rise", "Exp fall", "Bounce sine",
                                         "Sharp sine", "Stairs 4", "Stairs 8", "Trapezoid", "Bounce", "Soft triangle", "Double sine", "Chaos", "Log rise", "Log fall", "Double pulse", "Stepped sine" };
    return n[juce::jlimit (0, kLfoShapes - 1, s)];
}
juce::String filterName (int t) { static const char* n[kFilterTypes] = { "Off", "Multi 12 dB", "Multi 24 dB", "Analog", "High pass", "Band pass", "Band reject", "Duo parallel", "Duo serial" }; return n[juce::jlimit (0, kFilterTypes - 1, t)]; }
juce::String satName (int s) { static const char* n[kSatModes] = { "Off", "Soft", "Hard", "Tube", "Fold", "Fuzz" }; return n[juce::jlimit (0, kSatModes - 1, s)]; }

// ---------------------------------------------------------------- SoundFont
const SoundFont::Zone* SoundFont::find (int key, int vel) const noexcept
{
    const Zone* best = nullptr; int bd = 999;
    for (const auto& z : zones)
    {
        if (vel < z.velLo || vel > z.velHi) continue;
        if (key >= z.keyLo && key <= z.keyHi) return &z;
        const int d = key < z.keyLo ? z.keyLo - key : key - z.keyHi; if (d < bd) { bd = d; best = &z; }   // aralık dışı: en yakın bölge
    }
    return best != nullptr ? best : (zones.empty() ? nullptr : &zones.front());
}

std::shared_ptr<const SoundFont> loadSoundFont (const juce::File& f, juce::String& err)
{
    const auto key = f.getFullPathName();
    if (auto it = sfCache().find (key); it != sfCache().end()) if (auto sp = it->second.lock()) return sp;
    juce::MemoryBlock mb;
    if (! f.existsAsFile() || f.getSize() > 400 * 1024 * 1024 || ! f.loadFileAsData (mb) || mb.getSize() < 12) { err = "file"; return nullptr; }
    const auto* d = (const uint8_t*) mb.getData(); const size_t N = mb.getSize();
    if (memcmp (d, "RIFF", 4) != 0 || memcmp (d + 8, "sfbk", 4) != 0) { err = "format"; return nullptr; }
    std::map<juce::String, Chunk> ch;
    std::function<void (size_t, size_t)> walk = [&] (size_t a, size_t z)
    {
        while (a + 8 <= z)
        {
            const uint32_t n = u32 (d + a + 4); if (a + 8 + n > z) break;
            const juce::String id ((const char*) d + a, 4);
            if (id == "LIST") walk (a + 12, a + 8 + n); else ch[id] = { nullptr, d + a + 8, n };
            a += 8 + n + (n & 1);
        }
    };
    walk (12, N);
    for (auto* need : { "smpl", "phdr", "pbag", "pgen", "inst", "ibag", "igen", "shdr" }) if (ch.find (need) == ch.end()) { err = "format"; return nullptr; }
    const auto& ph = ch["phdr"]; const auto& pb = ch["pbag"]; const auto& pg = ch["pgen"]; const auto& in = ch["inst"]; const auto& ib = ch["ibag"]; const auto& ig = ch["igen"]; const auto& sh = ch["shdr"]; const auto& sm = ch["smpl"];
    const int nPh = (int) (ph.n / 38), nPb = (int) (pb.n / 4), nPg = (int) (pg.n / 4), nIn = (int) (in.n / 22), nIb = (int) (ib.n / 4), nIg = (int) (ig.n / 4), nSh = (int) (sh.n / 46), nSm = (int) (sm.n / 2);
    if (nPh < 2 || nIn < 2 || nSh < 1) { err = "empty"; return nullptr; }
    auto sf = std::make_shared<SoundFont>(); sf->path = key; sf->name = juce::String ((const char*) ph.d, 20).trim();
    sf->data.resize ((size_t) nSm); for (int i = 0; i < nSm; ++i) sf->data[(size_t) i] = (int16_t) u16 (sm.d + i * 2) / 32768.0f;
    struct G { int v[61]; bool has[61]; };
    auto readGens = [] (const Chunk& c, int a, int z, G& g) { for (int i = a; i < z; ++i) { const int op = u16 (c.d + i * 4); if (op < 61) { g.v[op] = (int16_t) u16 (c.d + i * 4 + 2); g.has[op] = true; } } };
    // ilk preset'in bölgeleri -> çalgılar
    const int b0 = u16 (ph.d + 24), b1 = juce::jmin (nPb - 1, (int) u16 (ph.d + 38 + 24));
    for (int bi = b0; bi < b1; ++bi)
    {
        G pgz {}; readGens (pg, u16 (pb.d + bi * 4), juce::jmin (nPg, (int) u16 (pb.d + (bi + 1) * 4)), pgz);
        if (! pgz.has[41]) continue;
        const int inst = juce::jlimit (0, nIn - 2, pgz.v[41]);
        const int pkLo = pgz.has[43] ? (pgz.v[43] & 255) : 0, pkHi = pgz.has[43] ? ((pgz.v[43] >> 8) & 255) : 127;
        const int ib0 = u16 (in.d + inst * 22 + 20), ib1 = juce::jmin (nIb - 1, (int) u16 (in.d + (inst + 1) * 22 + 20));
        G glob {};
        for (int z = ib0; z < ib1; ++z)
        {
            G g = glob; readGens (ig, u16 (ib.d + z * 4), juce::jmin (nIg, (int) u16 (ib.d + (z + 1) * 4)), g);
            if (! g.has[53]) { if (z == ib0) glob = g; continue; }   // örneksiz ilk bölge: genel ayarlar
            const int si = juce::jlimit (0, nSh - 1, g.v[53]); const uint8_t* s = sh.d + si * 46;
            if ((u16 (s + 44) & 0x8000) != 0) continue;   // ROM örneği
            SoundFont::Zone Z;
            Z.keyLo = juce::jmax (pkLo, g.has[43] ? (g.v[43] & 255) : 0); Z.keyHi = juce::jmin (pkHi, g.has[43] ? ((g.v[43] >> 8) & 255) : 127);
            Z.velLo = g.has[44] ? (g.v[44] & 255) : 0; Z.velHi = g.has[44] ? ((g.v[44] >> 8) & 255) : 127;
            Z.root = g.has[58] && g.v[58] >= 0 ? g.v[58] : s[40];
            Z.cents = (g.has[51] ? g.v[51] * 100 : 0) + (g.has[52] ? g.v[52] : 0) + (int8_t) s[41] + (pgz.has[51] ? pgz.v[51] * 100 : 0) + (pgz.has[52] ? pgz.v[52] : 0);
            Z.start = (int) u32 (s + 20) + (g.has[0] ? g.v[0] : 0) + (g.has[4] ? g.v[4] * 32768 : 0);
            Z.end = (int) u32 (s + 24) + (g.has[1] ? g.v[1] : 0) + (g.has[12] ? g.v[12] * 32768 : 0);
            Z.loopS = (int) u32 (s + 28) + (g.has[2] ? g.v[2] : 0) + (g.has[45] ? g.v[45] * 32768 : 0);
            Z.loopE = (int) u32 (s + 32) + (g.has[3] ? g.v[3] : 0) + (g.has[50] ? g.v[50] * 32768 : 0);
            Z.loop = g.has[54] && (g.v[54] & 1) != 0 && Z.loopE > Z.loopS + 8;
            Z.rate = juce::jmax (1000u, u32 (s + 36));
            Z.start = juce::jlimit (0, nSm - 2, Z.start); Z.end = juce::jlimit (Z.start + 1, nSm - 1, Z.end);
            Z.loopS = juce::jlimit (Z.start, Z.end - 1, Z.loopS); Z.loopE = juce::jlimit (Z.loopS + 1, Z.end, Z.loopE);
            if (Z.end - Z.start > 16) sf->zones.push_back (Z);
        }
    }
    if (sf->zones.empty()) { err = "empty"; return nullptr; }
    sfCache()[key] = sf;
    return sf;
}

// ---------------------------------------------------------------- program (JSON)
namespace
{
const juce::var& at (const juce::var& a, int i) { static const juce::var none; auto* arr = a.getArray(); return arr != nullptr && i >= 0 && i < arr->size() ? arr->getReference (i) : none; }
double num (const juce::var& o, const char* k, double d) { const auto v = o[k]; return v.isDouble() || v.isInt() || v.isInt64() || v.isBool() ? (double) v : d; }
Routes routesOf (const juce::var& a)
{
    Routes r {};
    if (auto* arr = a.getArray()) for (int i = 0; i < kSlots && i < arr->size(); ++i) if (auto* x = (*arr)[i].getArray(); x != nullptr && x->size() >= 2) r[(size_t) i] = { juce::jlimit (-1, kNumDests - 1, (int) (*x)[0]), juce::jlimit (-1.0f, 1.0f, (float) (double) (*x)[1]) };
    return r;
}
juce::var routesVar (const Routes& r) { juce::Array<juce::var> a; for (auto& x : r) a.add (juce::Array<juce::var> { x.dest, x.amt }); return a; }
}

std::shared_ptr<const Program> Program::fromVar (const juce::var& v, double sampleRate)
{
    auto P = std::make_shared<Program>(); P->sr = sampleRate;
    auto I = [] (const juce::var& o, const char* k, int d, int lo, int hi) { return juce::jlimit (lo, hi, (int) std::lround (num (o, k, d))); };
    auto F = [] (const juce::var& o, const char* k, double d, double lo, double hi) { return (float) juce::jlimit (lo, hi, num (o, k, d)); };
    P->sfPath = v["sf"].toString();
    if (P->sfPath.isNotEmpty()) { juce::String e; P->sf = loadSoundFont (juce::File (P->sfPath), e); }
    for (int k = 0; k < kOsc; ++k)
    {
        const auto o = at (v["o"], k); auto& O = P->osc[(size_t) k];
        if (! o.isObject()) continue;
        O.type = I (o, "t", 0, 0, 3);
        O.a = I (o, "a", 0, -1, 215); O.b = I (o, "b", 1, 0, 215);
        O.morph = F (o, "m", 0, 0, 1); O.morphMode = I (o, "mm", 0, 0, 1); O.shape = F (o, "sh", 0, 0, 1); O.synth = I (o, "sm", 0, 0, 2);
        O.fm = F (o, "fm", 0, 0, 1); O.fmRatio = F (o, "fr", 1, .25, 16); O.fmFilter = F (o, "ff", 0, 0, 1);
        O.pwMode = I (o, "pm", 0, 0, 3); O.pw = F (o, "pw", .5, .02, .98);
        O.uni = I (o, "u", 1, 1, kMaxUni); O.detune = F (o, "d", .2, 0, 1); O.spread = F (o, "sp", .7, 0, 1);
        O.layer = I (o, "ly", 1, 1, 2); O.layerSemi = F (o, "ls", 12, -24, 24); O.layerLevel = F (o, "ll", .5, 0, 1);
        O.phaseMode = I (o, "ph", 0, 0, 3); O.phase = F (o, "pv", 0, 0, 1);
        O.oct = I (o, "oc", 0, -3, 3); O.semi = I (o, "se", 0, -12, 12); O.fine = F (o, "fi", 0, -1, 1); O.level = F (o, "lv", .8, 0, 1); O.pan = F (o, "pa", 0, -1, 1);
        O.stacks = I (o, "n", 4, 1, 4); O.mix = F (o, "mx", .6, 0, 1);
        for (int s = 0; s < 4; ++s) { O.mw[(size_t) s] = juce::jlimit (0, kBasic - 1, (int) (at (o["mw"], s).isVoid() ? 0 : (int) at (o["mw"], s))); O.ml[(size_t) s] = at (o["ml"], s).isVoid() ? (s == 0 ? 1.0f : .6f) : juce::jlimit (0.0f, 1.0f, (float) (double) at (o["ml"], s)); }
        const auto f = o["f"];
        O.f.type = I (f, "t", 0, 0, kFilterTypes - 1); O.f.cut = F (f, "c", 18000, 20, 20000); O.f.res = F (f, "r", .1, 0, 1); O.f.env = F (f, "e", 0, -1, 1);
        O.f.kt = F (f, "k", .3, 0, 1); O.f.drive = F (f, "dr", 0, 0, 1); O.f.sat = I (f, "sa", 0, 0, kSatModes - 1); O.f.offset = F (f, "o", 1, -4, 4);
        // tablolar
        if (O.type == 1) { O.a = juce::jlimit (0, kBasic - 1, O.a); O.b = juce::jlimit (0, kBasic - 1, O.b); }
        if (O.type == 2 && O.a < 0 && P->sf == nullptr) O.a = 0;   // SoundFont yüklenemedi: ilk ROM dalgası
        auto tab = [&] (int i) { return O.type == 2 ? &romTable (i, sampleRate) : &basicTable (i, sampleRate); };
        if (O.type == 1 || (O.type == 2 && O.a >= 0))
        {
            O.tA[0] = tab (O.a); O.tB[0] = tab (O.b);
            if (O.morphMode == 1)
            {
                const int lo = juce::jmin (O.a, O.b), hi = juce::jmax (O.a, O.b), cnt = juce::jmin (16, hi - lo + 1);
                for (int s = 0; s < cnt; ++s) O.surf.push_back (tab (lo + (cnt > 1 ? s * (hi - lo) / (cnt - 1) : 0)));
                if (O.surf.size() < 2) O.surf.push_back (O.surf.front());
            }
        }
        if (O.type == 3) for (int s = 0; s < 4; ++s) O.surf.push_back (&basicTable (O.mw[(size_t) s], sampleRate));
    }
    const auto L = v["lx"]; auto& Y = P->layer;
    if (L.isObject())
    {
        Y.on = num (L, "on", 0) > 0; Y.wave = I (L, "w", -1, -3, 215); Y.ftype = I (L, "t", 0, 0, 3); Y.cut = F (L, "c", 8000, 20, 20000); Y.res = F (L, "r", .1, 0, 1);
        Y.level = F (L, "lv", .3, 0, 1); Y.pitch = F (L, "p", 0, -24, 24); Y.keyLo = I (L, "kl", 0, 0, 127); Y.keyHi = I (L, "kh", 127, 0, 127); Y.velLo = I (L, "vl", 0, 0, 127); Y.velHi = I (L, "vh", 127, 0, 127);
        if (Y.wave >= 0) Y.t = &romTable (Y.wave, sampleRate);
    }
    for (int e = 0; e < kEnvs; ++e)
    {
        const auto a = at (v["env"], e); auto& E = P->env[(size_t) e];
        if (a.isArray() && a.size() >= 4) { E.a = (float) juce::jlimit (0.0005, 20.0, (double) a[0]); E.d = (float) juce::jlimit (.001, 30.0, (double) a[1]); E.s = (float) juce::jlimit (0.0, 1.0, (double) a[2]); E.r = (float) juce::jlimit (.002, 30.0, (double) a[3]); E.curve = a.size() > 4 ? (float) juce::jlimit (-1.0, 1.0, (double) a[4]) : 0.0f; }
        if (e > 0) P->envTo[(size_t) e] = routesOf (at (v["ed"], e - 1));
    }
    for (int l = 0; l < kLfos; ++l)
    {
        const auto o = at (v["lfo"], l); auto& Lf = P->lfo[(size_t) l]; if (! o.isObject()) continue;
        Lf.shape = I (o, "w", 0, 0, kLfoShapes - 1); Lf.sync = num (o, "sy", 0) > 0; Lf.rate = Lf.sync ? (float) I (o, "r", 5, 0, 11) : F (o, "r", 2, .01, 40);
        Lf.retrig = num (o, "rt", 1) > 0; Lf.fade = F (o, "fa", 0, 0, 10); Lf.to = routesOf (o["d"]);
    }
    if (const auto s = v["stp"]; s.isObject())
    {
        P->step.n = I (s, "n", 16, 1, kSteps); P->step.div = I (s, "r", 9, 0, 11); P->step.glide = F (s, "gl", 0, 0, 1); P->step.to = routesOf (s["d"]);
        for (int i = 0; i < kSteps; ++i) P->step.v[(size_t) i] = at (s["v"], i).isVoid() ? 0.0f : juce::jlimit (-1.0f, 1.0f, (float) (double) at (s["v"], i));
    }
    for (int m = 0; m < kMtx; ++m) { const auto o = at (v["mtx"], m); if (o.isObject()) { P->mtx[(size_t) m].src = I (o, "s", 0, 0, kNumSources - 1); P->mtx[(size_t) m].to = routesOf (o["d"]); } }
    P->sine = &basicTable (3, sampleRate);   // FM taşıyıcısı (ses iş parçacığı kilitsiz okur)
    P->vol = F (v, "vol", .7, 0, 1); P->glide = F (v, "gl", 0, 0, 2); P->bend = F (v, "bd", 2, 0, 24);
    return P;
}

juce::var Program::init()
{
    auto* o = new juce::DynamicObject();
    auto osc = [] (int t, int a)
    {
        auto* x = new juce::DynamicObject(); x->setProperty ("t", t); x->setProperty ("a", a); x->setProperty ("b", 1); x->setProperty ("lv", .8);
        auto* f = new juce::DynamicObject(); f->setProperty ("t", 1); f->setProperty ("c", 6000); f->setProperty ("r", .15); f->setProperty ("k", .4); x->setProperty ("f", juce::var (f));
        return juce::var (x);
    };
    o->setProperty ("o", juce::Array<juce::var> { osc (1, 0), osc (0, 1), osc (0, 0) });
    o->setProperty ("env", juce::Array<juce::var> { juce::Array<juce::var> { .005, .3, .8, .3 }, juce::Array<juce::var> { .005, .4, .2, .3 }, juce::Array<juce::var> { .01, .5, .5, .5 }, juce::Array<juce::var> { .01, .5, .5, .5 } });
    o->setProperty ("vol", .7);
    return juce::var (o);
}

juce::var Program::toVar (const juce::var& v)
{
    const auto P = fromVar (v, 48000);
    auto* o = new juce::DynamicObject(); juce::Array<juce::var> os;
    for (const auto& O : P->osc)
    {
        auto* x = new juce::DynamicObject();
        x->setProperty ("t", O.type); x->setProperty ("a", O.a); x->setProperty ("b", O.b); x->setProperty ("m", O.morph); x->setProperty ("mm", O.morphMode); x->setProperty ("sh", O.shape); x->setProperty ("sm", O.synth);
        x->setProperty ("fm", O.fm); x->setProperty ("fr", O.fmRatio); x->setProperty ("ff", O.fmFilter); x->setProperty ("pm", O.pwMode); x->setProperty ("pw", O.pw);
        x->setProperty ("u", O.uni); x->setProperty ("d", O.detune); x->setProperty ("sp", O.spread); x->setProperty ("ly", O.layer); x->setProperty ("ls", O.layerSemi); x->setProperty ("ll", O.layerLevel);
        x->setProperty ("ph", O.phaseMode); x->setProperty ("pv", O.phase); x->setProperty ("oc", O.oct); x->setProperty ("se", O.semi); x->setProperty ("fi", O.fine); x->setProperty ("lv", O.level); x->setProperty ("pa", O.pan);
        x->setProperty ("n", O.stacks); x->setProperty ("mx", O.mix);
        juce::Array<juce::var> mw, ml; for (int s = 0; s < 4; ++s) { mw.add (O.mw[(size_t) s]); ml.add (O.ml[(size_t) s]); }
        x->setProperty ("mw", mw); x->setProperty ("ml", ml);
        auto* f = new juce::DynamicObject(); f->setProperty ("t", O.f.type); f->setProperty ("c", O.f.cut); f->setProperty ("r", O.f.res); f->setProperty ("e", O.f.env); f->setProperty ("k", O.f.kt);
        f->setProperty ("dr", O.f.drive); f->setProperty ("sa", O.f.sat); f->setProperty ("o", O.f.offset); x->setProperty ("f", juce::var (f));
        os.add (juce::var (x));
    }
    o->setProperty ("o", os);
    { const auto& Y = P->layer; auto* l = new juce::DynamicObject(); l->setProperty ("on", Y.on ? 1 : 0); l->setProperty ("w", Y.wave); l->setProperty ("t", Y.ftype); l->setProperty ("c", Y.cut); l->setProperty ("r", Y.res);
      l->setProperty ("lv", Y.level); l->setProperty ("p", Y.pitch); l->setProperty ("kl", Y.keyLo); l->setProperty ("kh", Y.keyHi); l->setProperty ("vl", Y.velLo); l->setProperty ("vh", Y.velHi); o->setProperty ("lx", juce::var (l)); }
    juce::Array<juce::var> env, ed; for (int e = 0; e < kEnvs; ++e) { const auto& E = P->env[(size_t) e]; env.add (juce::Array<juce::var> { E.a, E.d, E.s, E.r, E.curve }); if (e > 0) ed.add (routesVar (P->envTo[(size_t) e])); }
    o->setProperty ("env", env); o->setProperty ("ed", ed);
    juce::Array<juce::var> lf; for (const auto& L : P->lfo) { auto* l = new juce::DynamicObject(); l->setProperty ("w", L.shape); l->setProperty ("r", L.rate); l->setProperty ("sy", L.sync ? 1 : 0); l->setProperty ("rt", L.retrig ? 1 : 0); l->setProperty ("fa", L.fade); l->setProperty ("d", routesVar (L.to)); lf.add (juce::var (l)); }
    o->setProperty ("lfo", lf);
    { auto* s = new juce::DynamicObject(); juce::Array<juce::var> sv; for (auto x : P->step.v) sv.add (x); s->setProperty ("n", P->step.n); s->setProperty ("r", P->step.div); s->setProperty ("gl", P->step.glide); s->setProperty ("v", sv); s->setProperty ("d", routesVar (P->step.to)); o->setProperty ("stp", juce::var (s)); }
    juce::Array<juce::var> mx; for (const auto& M : P->mtx) { auto* m = new juce::DynamicObject(); m->setProperty ("s", M.src); m->setProperty ("d", routesVar (M.to)); mx.add (juce::var (m)); }
    o->setProperty ("mtx", mx); o->setProperty ("vol", P->vol); o->setProperty ("gl", P->glide); o->setProperty ("bd", P->bend);
    if (P->sfPath.isNotEmpty()) o->setProperty ("sf", P->sfPath);
    return juce::var (o);
}

// ---------------------------------------------------------------- ses
namespace
{
inline float nextRand (uint32_t& r) noexcept { r ^= r << 13; r ^= r >> 17; r ^= r << 5; return (float) (r & 0xffffff) / 8388608.0f - 1.0f; }

int phasesFor (const Program::Osc& O, const Ctx& c)
{
    if (O.type == 0) return 0;
    if (O.type == 3) return juce::jmin (c.maxPh, O.stacks * 8);
    return juce::jmin (c.maxPh, O.uni * O.layer, kMaxPh);
}

void setupOsc (Voice& v, const Program& P, int k, const Ctx& c, bool keepPhases)
{
    const auto& O = P.osc[(size_t) k]; auto& S = v.o[(size_t) k];
    const int old = keepPhases ? S.n : 0, n = phasesFor (O, c); S.n = n;
    if (n == 0) return;
    static const float kSuper[8] = { -1.0f, -.62f, -.32f, -.1f, .1f, .3f, .6f, .96f };   // süpertestere: kenarlara doğru açılan aralık
    const int layers = O.type == 3 ? 1 : O.layer, base = O.type == 3 ? 8 : juce::jmax (1, n / layers);
    float norm = 0;
    for (int u = 0; u < n; ++u)
    {
        const int L = O.type == 3 ? 0 : u / base, i = O.type == 3 ? u % 8 : u % base, cnt = O.type == 3 ? 8 : base;
        const float x = cnt > 1 ? (O.type == 3 ? kSuper[i] : -1.0f + 2.0f * i / (cnt - 1)) : 0.0f;
        float cents = O.type == 3 ? x * O.detune * 70 + (u / 8) * .37f * O.detune : x * O.detune * 50 * (.6f + .4f * std::abs (x));
        if (L == 1) cents += O.layerSemi * 100;
        S.rat[u] = std::pow (2.0f, cents / 1200.0f);
        S.st[u] = O.type == 3 ? u / 8 : L;
        float g = L == 1 ? O.layerLevel : 1.0f;
        if (O.type == 3) g = O.ml[(size_t) juce::jmin (3, u / 8)] * (std::abs (x) < .2f ? (1.2f - O.mix) : (.25f + O.mix));
        const float pan = juce::jlimit (-1.0f, 1.0f, O.pan + x * O.spread * (i % 2 ? -1.0f : 1.0f) * (cnt > 1 ? 1.0f : 0.0f)), a = (pan + 1) * (float) kPi * .25f;
        S.gl[u] = std::cos (a) * g; S.gr[u] = std::sin (a) * g; norm += g * g;
        if (u >= old)
        {
            S.dead[u] = false;
            S.ph[u] = O.phaseMode == 1 ? 0.0 : O.phaseMode == 2 ? (double) u / n : O.phaseMode == 3 ? O.phase : (nextRand (v.rng) + 1) * .5;
            if (O.type == 2 && O.a < 0 && S.zone != nullptr) S.pos[u] = S.zone->start + (u == 0 ? 0.0 : (nextRand (v.rng) + 1) * 40);
        }
    }
    const float nn = 1.0f / std::sqrt (juce::jmax (1e-6f, norm));
    for (int u = 0; u < n; ++u) { S.gl[u] *= nn * 1.4142f; S.gr[u] *= nn * 1.4142f; }
}

void advanceEnv (const Program::Env& E, float& ev, int& st, bool released, double sec, double sr, float atkMul, float relMul) noexcept
{
    const double n = sec * sr;
    if (released && st < 3) st = 3;
    if (st == 0) { ev += (float) (n / (juce::jmax (.0005, (double) E.a * atkMul) * sr)); if (ev >= 1) { ev = 1; st = 1; } }
    else if (st == 1 || st == 2) { const float c = (float) std::exp (-n / (E.d * sr / 5)); ev = E.s + (ev - E.s) * c; st = 2; }
    else { ev *= (float) std::exp (-n / (juce::jmax (.002, (double) E.r * relMul) * sr / 5)); }
}
inline float envOut (const Program::Env& E, float ev) noexcept { return E.curve == 0 ? ev : std::pow (juce::jmax (0.0f, ev), std::pow (3.0f, -E.curve)); }

void control (Voice& v, const Program& P, const Ctx& c, int n)
{
    const double sec = n / c.sr;
    for (int e = 1; e < kEnvs; ++e) advanceEnv (P.env[(size_t) e], v.ev[(size_t) e], v.stage[(size_t) e], v.released, sec, c.sr, 1, 1);
    for (int l = 0; l < kLfos; ++l)
    {
        const auto& L = P.lfo[(size_t) l];
        const double hz = lfoHz (L, c.bpm) * std::pow (2.0, v.mod[(size_t) (l == 0 ? dLfo1Rate : dLfo2Rate)] * 4);
        double ph = v.lph[(size_t) l] + hz * sec;
        if (ph >= 1) { ph -= std::floor (ph); v.lHold[(size_t) l] = v.lNext[(size_t) l]; v.lNext[(size_t) l] = nextRand (v.rng); }
        v.lph[(size_t) l] = ph;
        const float fade = L.fade > 0 ? (float) juce::jmin (1.0, v.t / L.fade) : 1.0f;
        v.lOut[(size_t) l] = lfoWave (L.shape, ph, v.lHold[(size_t) l], v.lNext[(size_t) l]) * fade * juce::jmax (0.0f, 1 + v.mod[(size_t) (l == 0 ? dLfo1Amt : dLfo2Amt)]);
    }
    {
        const auto& S = P.step; const double hz = c.bpm / 60.0 / kSyncBeats[S.div] * std::pow (2.0, v.mod[dStepRate] * 4);
        v.sph += hz * sec; const int i = (int) std::floor (v.sph) % S.n; const float target = S.v[(size_t) i];
        const float k = S.glide <= 0 ? 1.0f : (float) (1 - std::exp (-sec * hz * 8 / (S.glide * S.glide + 1e-3)));
        v.sOut += (target - v.sOut) * k;
    }
    std::array<float, kNumDests> m {};
    auto add = [&] (const Routes& r, float val, float scale)
    {
        for (const auto& x : r) if (x.dest >= 0 && x.amt != 0) m[(size_t) x.dest] += val * x.amt * destRange (x.dest) * scale;
    };
    for (int e = 1; e < kEnvs; ++e) add (P.envTo[(size_t) e], envOut (P.env[(size_t) e], v.ev[(size_t) e]), c.lfoMul);
    for (int l = 0; l < kLfos; ++l) add (P.lfo[(size_t) l].to, v.lOut[(size_t) l], c.lfoMul);
    add (P.step.to, v.sOut, 1);
    for (const auto& M : P.mtx)
    {
        float s = 0;
        switch (M.src)
        {
            case sVel: s = v.vel; break; case sKey: s = v.key; break; case sWheel: s = c.wheel; break; case sAfter: s = c.after; break; case sRand: s = v.rnd; break;
            case sEnv1: case sEnv2: case sEnv3: case sEnv4: s = envOut (P.env[(size_t) (M.src - sEnv1)], v.ev[(size_t) (M.src - sEnv1)]); break;
            case sLfo1: s = v.lOut[0]; break; case sLfo2: s = v.lOut[1]; break; case sStep: s = v.sOut; break; default: continue;
        }
        add (M.to, s, 1);
    }
    v.mod = m;
    // osilatör türevleri
    const double gl = v.glideFrom, f0 = v.f0 * gl;
    for (int k = 0; k < kOsc; ++k)
    {
        const auto& O = P.osc[(size_t) k]; auto& S = v.o[(size_t) k]; if (S.n == 0) continue;
        auto M = [&] (int j) { return m[(size_t) (k * 16 + j)]; };
        const double semis = O.oct * 12 + O.semi + O.fine + M (0) + M (1) + m[dPitch];
        S.pitch = std::pow (2.0, semis / 12.0);
        const double hz = f0 * S.pitch;
        S.level = juce::jlimit (0.0f, 1.5f, O.level + M (2));
        S.morph = juce::jlimit (0.0f, 1.0f, O.morph + M (4)); S.shape = juce::jlimit (0.0f, 1.0f, O.shape + M (5)); S.pw = juce::jlimit (.02f, .98f, O.pw + M (6) * .5f);
        S.fm = juce::jlimit (0.0f, 1.0f, O.fm + M (9)); S.fmFilter = juce::jlimit (0.0f, 1.0f, O.fmFilter + M (10));
        S.drive = juce::jlimit (0.0f, 1.0f, O.f.drive + c.drive + M (13));
        const float detMod = M (7), lyMod = M (14), mixMod = m[dMultiMix];
        if (detMod != 0 || lyMod != 0 || mixMod != 0 || M (8) != 0)
        {   // detune / yayılım / katman düzeyi modülasyonu: oranları ve kazançları kontrol hızında yeniden kur
            auto O2 = O; O2.detune = juce::jlimit (0.0f, 1.0f, O.detune + detMod); O2.layerLevel = juce::jlimit (0.0f, 1.0f, O.layerLevel + lyMod); O2.mix = juce::jlimit (0.0f, 1.0f, O.mix + mixMod); O2.spread = juce::jlimit (0.0f, 1.0f, O.spread + M (8));
            Program P2; P2.osc[(size_t) k] = O2; const int keep = S.n; setupOsc (v, P2, k, c, true); S.n = keep;
        }
        const bool sample = O.type == 2 && O.a < 0;
        double maxHz = hz;
        for (int u = 0; u < S.n; ++u)
        {
            const double f = hz * S.rat[u]; maxHz = juce::jmax (maxHz, f);
            S.inc[u] = sample && S.zone != nullptr ? f / (440.0 * std::pow (2.0, (S.zone->root - 69) / 12.0 + S.zone->cents / 1200.0)) * S.zone->rate / c.sr : f / c.sr;
        }
        S.minc = hz * O.fmRatio / c.sr;
        const double topHz = maxHz * (O.synth == 1 ? 1 + S.fm * 6 : 1) * (O.pwMode == 3 ? 1 + S.pw * 7 : 1);
        if (O.type == 1 || (O.type == 2 && ! sample))
        {
            if (O.morphMode == 1 && O.surf.size() >= 2) { const float x = S.morph * (float) (O.surf.size() - 1); S.surfI = juce::jmin ((int) O.surf.size() - 2, (int) x); S.surfF = x - S.surfI; S.lvA = O.surf[(size_t) S.surfI]->levelFor (topHz); S.lvB = O.surf[(size_t) S.surfI + 1]->levelFor (topHz); }
            else { S.lvA = O.tA[0]->levelFor (topHz); S.lvB = O.tB[0]->levelFor (topHz); }
        }
        if (O.type == 3) for (int s = 0; s < 4 && s < (int) O.surf.size(); ++s) S.lvS[s] = O.surf[(size_t) s]->levelFor (topHz);
        // filtre
        const auto& Fl = O.f;
        double cut = Fl.cut * c.cutMul * std::pow (2.0, (double) (M (11) + Fl.env * 5 * envOut (P.env[1], v.ev[1]) * c.lfoMul)) * std::pow (v.f0 / 261.63, (double) Fl.kt);
        cut = juce::jlimit (16.0, c.sr * .45, cut);
        S.cut = (float) cut; S.cut2 = (float) juce::jlimit (16.0, c.sr * .45, cut * std::pow (2.0, (double) Fl.offset));
        S.res = juce::jlimit (0.0f, .99f, Fl.res + c.resAdd + M (12));
        S.g1 = gOf (S.cut, c.sr); S.g2 = gOf (S.cut2, c.sr); S.k1 = 2 - 2 * S.res * .98f;
    }
}

inline float filterTick (Voice::O& S, int type, float x, int ch, float g, float g2) noexcept
{
    switch (type)
    {
        case 1: return svfTick (x, g, S.k1, S.svf[ch][0]).lp;
        case 2: return svfTick (svfTick (x, g, S.k1, S.svf[ch][0]).lp, g, juce::jmin (2.0f, S.k1 * 1.3f), S.svf[ch][1]).lp;
        case 3:
        {
            const float G = g / (1 + g); float u = std::tanh (x - S.res * 3.9f * S.ladY[ch]);
            for (int s = 0; s < 4; ++s) { const float vv = (u - S.lad[ch][s]) * G, y = vv + S.lad[ch][s]; S.lad[ch][s] = y + vv; u = y; }
            S.ladY[ch] = u; return u * (1 + S.res * .8f);
        }
        case 4: return svfTick (x, g, S.k1, S.svf[ch][0]).hp;
        case 5: return svfTick (x, g, S.k1, S.svf[ch][0]).bp * S.k1;
        case 6: { const auto o = svfTick (x, g, S.k1, S.svf[ch][0]); return o.lp + o.hp; }
        case 7: return .7f * (svfTick (x, g, S.k1, S.svf[ch][0]).lp + svfTick (x, g2, S.k1, S.svf[ch][1]).hp);
        case 8: return svfTick (svfTick (x, g, S.k1, S.svf[ch][0]).hp, g2, S.k1, S.svf[ch][1]).lp;
        default: return x;
    }
}
}

void start (Voice& v, const Program& P, int note, int vel, double glideFrom, uint32_t seed, const Ctx& c)
{
    v = Voice();
    v.rng = seed | 1u; v.note = note; v.f0 = 440.0 * std::pow (2.0, (note - 69) / 12.0); v.vel = vel / 127.0f; v.key = (note - 60) / 48.0f; v.rnd = nextRand (v.rng);
    v.glideFrom = glideFrom;
    for (int l = 0; l < kLfos; ++l) { v.lph[(size_t) l] = P.lfo[(size_t) l].retrig ? 0.0 : (nextRand (v.rng) + 1) * .5; v.lHold[(size_t) l] = nextRand (v.rng); v.lNext[(size_t) l] = nextRand (v.rng); }
    for (int k = 0; k < kOsc; ++k)
    {
        auto& S = v.o[(size_t) k]; const auto& O = P.osc[(size_t) k];
        if (O.type == 2 && O.a < 0 && P.sf != nullptr) S.zone = P.sf->find (note, vel);
        setupOsc (v, P, k, c, false);
    }
    v.lyPh = (nextRand (v.rng) + 1) * .5f;
    control (v, P, c, 0);
}

void migrate (Voice& v, const Program& P, const Ctx& c)
{
    for (int k = 0; k < kOsc; ++k)
    {
        auto& S = v.o[(size_t) k]; const auto& O = P.osc[(size_t) k];
        if (O.type == 2 && O.a < 0 && P.sf != nullptr && S.zone == nullptr) S.zone = P.sf->find (v.note, (int) (v.vel * 127));
        if (! (O.type == 2 && O.a < 0)) S.zone = nullptr;
        setupOsc (v, P, k, c, true);
    }
    v.ctl = 0;
}

void release (Voice& v) noexcept { v.released = true; }

void render (Voice& v, const Program& P, const Ctx& c, float* L, float* R, int n) noexcept
{
    if (v.done) return;
    const double dt = 1.0 / c.sr; const int CR = juce::jmax (1, c.controlRate);
    const auto& Y = P.layer;
    const bool layerOn = Y.on && v.note >= Y.keyLo && v.note <= Y.keyHi && (int) (v.vel * 127) >= Y.velLo && (int) (v.vel * 127) <= Y.velHi;
    for (int i = 0; i < n; ++i)
    {
        if (v.ctl <= 0) { control (v, P, c, CR); v.ctl = CR; if (P.glide > 0 && v.glideFrom != 1) v.glideFrom = 1 + (v.glideFrom - 1) * std::exp (-(double) CR / (P.glide * c.sr / 4)); }
        --v.ctl;
        advanceEnv (P.env[0], v.ev[0], v.stage[0], v.released, dt, c.sr, c.atkMul, c.relMul);
        const float amp = envOut (P.env[0], v.ev[0]);
        float outL = 0, outR = 0;
        for (int k = 0; k < kOsc; ++k)
        {
            const auto& O = P.osc[(size_t) k]; auto& S = v.o[(size_t) k]; if (S.n == 0 || S.level <= 0) continue;
            float modv = 0;
            if (S.fm > 0 || S.fmFilter > 0) { S.mph += S.minc; if (S.mph >= 1) S.mph -= 1; modv = lookup (P.sine->lv[0].data(), S.mph); }
            float sL = 0, sR = 0;
            const bool sample = O.type == 2 && O.a < 0;
            for (int u = 0; u < S.n; ++u)
            {
                float x;
                if (sample)
                {
                    if (S.zone == nullptr || S.dead[u]) continue;
                    const auto& Z = *S.zone; double p = S.pos[u];
                    const int i0 = (int) p; const float f = (float) (p - i0);
                    x = P.sf->data[(size_t) i0] + (P.sf->data[(size_t) juce::jmin (Z.end, i0 + 1)] - P.sf->data[(size_t) i0]) * f;
                    p += S.inc[u];
                    if (Z.loop && p >= Z.loopE) p -= (Z.loopE - Z.loopS); else if (p >= Z.end - 1) S.dead[u] = true;
                    S.pos[u] = p;
                }
                else
                {
                    double p = S.ph[u]; S.ph[u] += S.inc[u]; if (S.ph[u] >= 1) S.ph[u] -= 1;
                    if (O.synth == 1) p = frac (p + S.fm * 2.5 * modv);
                    else if (O.synth == 2) { const double d = .5 - S.shape * .49; p = p < d ? p * .5 / d : .5 + (p - d) * .5 / (1 - d); }
                    if (O.pwMode == 1) { const double w = S.pw; p = p < w ? p * .5 / w : .5 + (p - w) * .5 / (1 - w); }
                    else if (O.pwMode == 3) p = frac (p * (1 + S.pw * 7));
                    auto read = [&] (double q) -> float
                    {
                        if (O.type == 3) return lookup (S.lvS[S.st[u] & 3], q);
                        if (O.morphMode == 1) { const float a = lookup (S.lvA, q); return a + (lookup (S.lvB, q) - a) * S.surfF; }
                        const float a = lookup (S.lvA, q); return S.morph <= 0 ? a : a + (lookup (S.lvB, q) - a) * S.morph;
                    };
                    x = read (p);
                    if (O.pwMode == 2) x = .7f * (x - read (frac (p + S.pw)));
                    if (O.pwMode == 3) x *= (float) (1 - S.ph[u]);   // senkron penceresi: sıçramayı yumuşatır
                    if (O.synth == 0 && S.shape > 0) x = std::sin (x * (1 + S.shape * 5) * 1.5707963f) * (1 - S.shape * .3f);
                }
                sL += x * S.gl[u]; sR += x * S.gr[u];
            }
            float g = S.g1;
            if (S.fmFilter > 0 && O.f.type != 0) g = gOf (S.cut * std::pow (2.0f, S.fmFilter * 4 * modv), c.sr);
            if (O.f.sat != 0 || S.drive > 0) { sL = saturate (sL, O.f.sat, S.drive); sR = saturate (sR, O.f.sat, S.drive); }
            sL = filterTick (S, O.f.type, sL, 0, g, S.g2); sR = filterTick (S, O.f.type, sR, 1, g, S.g2);
            outL += sL * S.level; outR += sR * S.level;
        }
        if (layerOn)
        {
            float x;
            if (Y.wave >= 0 && Y.t != nullptr)
            {
                const double f = v.f0 * std::pow (2.0, (Y.pitch + v.mod[dLayerPitch]) / 12.0);
                v.lyPh += (float) (f / c.sr); if (v.lyPh >= 1) v.lyPh -= 1;
                x = lookup (Y.t->levelFor (f), v.lyPh);
            }
            else
            {
                const float w = nextRand (v.rng);
                if (Y.wave == -2) { auto& b = v.lyPink; b[0] = .99765f * b[0] + w * .099046f; b[1] = .963f * b[1] + w * .2965164f; b[2] = .57f * b[2] + w * 1.0526913f; x = (b[0] + b[1] + b[2] + w * .1848f) * .25f; }
                else if (Y.wave == -3) x = std::abs (w) > .985f ? w * 3 : w * .05f;   // çıtırtı
                else x = w;
            }
            const float cutv = juce::jlimit (20.0f, (float) (c.sr * .45), Y.cut * std::pow (2.0f, v.mod[dLayerCut]));
            const float res = juce::jlimit (0.0f, .99f, Y.res + v.mod[dLayerRes]), kq = 2 - 2 * res * .98f;
            const auto o = svfTick (x, gOf (cutv, c.sr), kq, v.lySvf[0]);
            const float y = Y.ftype == 0 ? o.lp : Y.ftype == 1 ? o.hp : Y.ftype == 2 ? o.bp * kq : o.lp + o.hp;
            const float lv = juce::jmax (0.0f, Y.level + v.mod[dLayerLevel] + v.mod[dNoise]);
            outL += y * lv; outR += y * lv;
        }
        const float vol = P.vol * (.4f + .6f * v.vel) * juce::jmax (0.0f, 1 + v.mod[dAmp]) * amp * .3f;
        const float pan = juce::jlimit (-1.0f, 1.0f, v.mod[dPan]), pl = pan > 0 ? 1 - pan : 1.0f, pr = pan < 0 ? 1 + pan : 1.0f;
        L[i] += outL * vol * pl; R[i] += outR * vol * pr;
        v.t += dt;
        if (v.released && v.stage[0] == 3 && v.ev[0] < 1e-5f) { v.done = true; break; }
    }
}
}
