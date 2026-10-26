#include "Synth.h"
#include "Data.h"
#include <cmath>

namespace hg
{
namespace
{
constexpr double kTwoPi = 6.283185307179586;
inline float frand (uint32_t& s) noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 8388608.0f - 1.0f; }
inline float fastTanh (float x) noexcept { x = juce::jlimit (-4.5f, 4.5f, x); const float x2 = x * x; return x * (27.0f + x2) / (27.0f + 9.0f * x2); }
inline float sat (float x, float k) noexcept { const float a = 1.0f + k * 4.0f; return fastTanh (x * a) / fastTanh (a); }   // web satCurve(k)
inline void svfCoef (float* c, double fc, double q, double sr) noexcept
{
    const double g = std::tan (juce::MathConstants<double>::pi * juce::jlimit (20.0, sr * 0.49, fc) / sr), k = 1.0 / juce::jmax (0.05, q);
    const double a1 = 1.0 / (1.0 + g * (g + k));
    c[0] = (float) a1; c[1] = (float) (g * a1); c[2] = (float) (g * g * a1); c[3] = (float) k;
}
struct SvfOut { float lp, bp, hp; };
inline SvfOut svf (float x, const float* c, float* s) noexcept
{
    const float v3 = x - s[1], v1 = c[0] * s[0] + c[1] * v3, v2 = s[1] + c[1] * s[0] + c[2] * v3;
    s[0] = 2 * v1 - s[0]; s[1] = 2 * v2 - s[1];
    return { v2, v1, x - c[3] * v1 - v2 };
}
inline float lookup (const float* tb, double ph) noexcept
{
    const double p = ph * WaveTable::kSize; const int i = (int) p; const float f = (float) (p - i);
    const float a = tb[i & (WaveTable::kSize - 1)], b = tb[(i + 1) & (WaveTable::kSize - 1)];
    return a + (b - a) * f;
}
inline float lfoWave (int shape, double ph) noexcept
{
    switch (shape)
    {
        case 1: return (float) (ph < .5 ? 4 * ph - 1 : 3 - 4 * ph);
        case 2: return ph < .5 ? 1.0f : -1.0f;
        case 3: return (float) (2 * ph - 1);
        default: return (float) std::sin (kTwoPi * ph);
    }
}
const float kFormF[5][3] = { { 850, 1220, 2810 }, { 500, 900, 2830 }, { 370, 950, 2670 }, { 520, 1850, 2600 }, { 350, 2300, 3050 } };
const float kFormG[5][3] = { { 1, .55f, .25f }, { 1, .45f, .12f }, { 1, .35f, .10f }, { 1, .50f, .20f }, { 1, .45f, .20f } };
// Web Audio'da lowpass/highpass Q desibel cinsindendir
inline double qFromDb (double qdb) { return std::pow (10.0, qdb / 20.0); }
}

// ---------------------------------------------------------------- WaveTable
void WaveTable::build (const std::vector<float>& harm, double sampleRate)
{
    double norm = 0;
    for (int L = 0; L < kLevels; ++L)
    {
        auto& t = lv[(size_t) L]; t.assign (kSize, 0.0f);
        const double top = 20.0 * std::pow (2.0, L + 1);
        const int hmax = juce::jlimit (1, (int) harm.size(), (int) std::floor (sampleRate * 0.45 / top));
        std::vector<double> acc (kSize, 0.0);
        for (int n = 1; n <= hmax; ++n)
        {
            const double a = harm[(size_t) n - 1]; if (std::abs (a) < 1e-7) continue;
            const double w = kTwoPi * n / kSize, cw = std::cos (w), sw = std::sin (w);
            double s = 0, c = 1;   // sin(n·θ) özyinelemesi
            for (int i = 0; i < kSize; ++i) { acc[(size_t) i] += a * s; const double s2 = s * cw + c * sw; c = c * cw - s * sw; s = s2; }
        }
        if (L == 0) { for (auto x : acc) norm = std::max (norm, std::abs (x)); if (norm < 1e-9) norm = 1; }
        for (int i = 0; i < kSize; ++i) t[(size_t) i] = (float) (acc[(size_t) i] / norm);
    }
}

// ---------------------------------------------------------------- Patch
std::unique_ptr<Patch> Patch::fromVar (const juce::var& v, const Data& data, double sampleRate)
{
    auto p = std::make_unique<Patch>();
    p->id = v["id"].toString(); p->cat = v["cat"].toString(); p->trim = (float) data.trim (p->id);
    p->chordsHalf = v["r"].toString().contains ("m");
    auto waveOf = [] (const juce::String& w) { return w == "w" ? 0 : w == "q" ? 1 : w == "t" ? 2 : w == "s" ? 3 : w == "x" ? 4 : 0; };
    if (auto* o = v["o"].getArray())
        for (auto& L : *o)
            if (p->numOsc < 3) p->osc[(size_t) p->numOsc++] = { waveOf (at (L, 0).toString()), (float) num (at (L, 1)), (float) num (at (L, 2)), (float) num (at (L, 3)) };
    if (auto* u = v["u"].getArray()) { p->uniN = juce::jlimit (1, 7, (int) num ((*u)[0], 1)); p->uniSpread = (float) num ((*u)[1]); }
    if (auto* f = v["f"].getArray())
    {
        auto t = (*f)[0].toString(); p->ftype = t == "highpass" ? 1 : t == "bandpass" ? 2 : 0;
        p->cut = (float) num ((*f)[1], 8000); p->q = (float) num ((*f)[2], .7); p->envAmt = (float) num ((*f)[3]); p->envAtk = (float) num ((*f)[4]); p->envDec = (float) num ((*f)[5], .2);
        p->slope24 = f->size() > 6 && num ((*f)[6]) > 0;
    }
    else { p->ftype = 0; p->cut = 8000; p->q = .7f; }
    if (auto* a = v["a"].getArray()) { p->A = (float) num ((*a)[0], .01); p->D = (float) num ((*a)[1], .2); p->S = (float) num ((*a)[2], .7); p->R = (float) num ((*a)[3], .3); }
    if (auto* w = v["wt"].getArray()) { p->wtA = (float) num ((*w)[0]); p->wtB = (float) num ((*w)[1]); p->wtMorph = (float) num ((*w)[2]); p->wtBright = (float) num ((*w)[3], 1); }
    int li = 0;
    if (auto* l = v["l"].getArray())
        for (auto& L : *l)
        {
            if (li >= 2 || num (at (L, 1)) <= 0) continue;
            auto tg = at (L, 2).toString(), sh = at (L, 3).toString();
            p->lfo[(size_t) li++] = { (float) num (at (L, 0), 1), (float) num (at (L, 1)), tg == "pit" ? 1 : tg == "amp" ? 2 : 0, sh == "triangle" ? 1 : sh == "square" ? 2 : sh == "sawtooth" ? 3 : 0, num (at (L, 4)) > 0 };
        }
    if (auto* lf = v["lf"].getArray()) p->lfo[2] = { (float) num ((*lf)[0], 1), (float) num ((*lf)[1]), 0, 0, false };   // eski filtre LFO'su
    p->rv = (float) num (v["rv"]); p->dl = (float) num (v["dl"]); p->ch = (float) num (v["ch"]); p->dr = (float) num (v["dr"]); p->fd = (float) num (v["fd"]);
    p->vol = (float) num (v["vol"], .2); p->kt = has (v, "kt") ? (float) num (v["kt"]) : .5f; p->noi = (float) num (v["noi"]); p->sub = (float) num (v["sub"]);
    if (auto* fm = v["fm"].getArray()) { p->hasFm = true; p->fmRatio = (float) num ((*fm)[0], 1); p->fmIndex = (float) num ((*fm)[1], 1); p->fmDecay = (float) num ((*fm)[2], .2); p->fmSus = fm->size() > 3 ? (float) num ((*fm)[3], .25) : .25f; p->fmLevel = fm->size() > 4 ? (float) num ((*fm)[4], 1) : 1.0f; if (p->fmLevel <= 0) p->fmLevel = 1; }
    if (auto* pe = v["pe"].getArray()) { p->hasPe = true; p->peSemi = (float) num ((*pe)[0]); p->peTime = (float) num ((*pe)[1], .1); }
    if (auto* vb = v["vib"].getArray()) { p->hasVib = true; p->vibRate = (float) num ((*vb)[0], 5); p->vibCents = (float) num ((*vb)[1]); p->vibDelay = (float) num ((*vb)[2]); }
    auto vw = v["vow"].toString(); p->vowel = vw == "a" ? 0 : vw == "o" ? 1 : vw == "u" ? 2 : vw == "e" ? 3 : vw == "i" ? 4 : -1;
    p->male = (bool) v["male"];
    // wavetable (web ile aynı: 48 harmonik, geçiş ve parlaklık 1/16 adımlı)
    bool needWt = false; for (int i = 0; i < p->numOsc; ++i) if (p->osc[(size_t) i].wave == 4) needWt = true;
    if (needWt)
    {
        const auto& W = data.wavetables(); const int A = juce::jlimit (0, 11, (int) p->wtA), B = juce::jlimit (0, 11, (int) p->wtB);
        const double m = std::round (p->wtMorph * 16) / 16, br = std::round (p->wtBright * 16) / 16, cut = 2 + br * 46;
        std::vector<float> h (48);
        for (int n = 1; n <= 48; ++n) h[(size_t) n - 1] = (float) (((double) W[A][n - 1] * (1 - m) + (double) W[B][n - 1] * m) * (n <= cut ? 1.0 : std::exp (-(n - cut) / 3.0)));
        p->table.build (h, sampleRate);
    }
    return p;
}

// ---------------------------------------------------------------- Engine
Engine::Engine() { for (auto& p : pending) p.store (nullptr); }
Engine::~Engine()
{
    for (auto& p : pending) delete p.exchange (nullptr);
    for (auto*& p : cur) { delete p; p = nullptr; }
    for (auto*& p : retiring) { delete p; p = nullptr; }
    collectGarbage();
}

void Engine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate; maxBlockSize = juce::jmax (16, maxBlock);
    std::vector<float> saw (1024), sq (1024), tri (1024), sine (1, 1.0f);
    for (int n = 1; n <= 1024; ++n) { saw[(size_t) n - 1] = 1.0f / n; sq[(size_t) n - 1] = n % 2 ? 1.0f / n : 0; tri[(size_t) n - 1] = n % 2 ? (n % 4 == 1 ? 1.0f : -1.0f) / (float) (n * n) : 0; }
    basic[0].build (saw, sr); basic[1].build (sq, sr); basic[2].build (tri, sr); basic[3].build (sine, sr);
    for (int t = 0; t < 3; ++t) for (int c = 0; c < 2; ++c) chorusBuf[t][c].assign ((size_t) (sr * 0.06) + 4, 0.0f);
    for (int c = 0; c < 2; ++c) dlBuf[c].assign ((size_t) (sr * 2.0) + 4, 0.0f);
    partBuf.setSize (8, maxBlockSize); sendRv.setSize (2, maxBlockSize); sendDl.setSize (2, maxBlockSize); drumBuf.setSize (2, maxBlockSize);
    juce::Reverb::Parameters rp; rp.roomSize = .84f; rp.damping = .45f; rp.wetLevel = .9f; rp.dryLevel = 0; rp.width = 1; rp.freezeMode = 0;
    reverb.setParameters (rp); reverb.setSampleRate (sr);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlockSize, 2 };
    comp.prepare (spec); comp.setThreshold (-16); comp.setRatio (6); comp.setAttack (3); comp.setRelease (160);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices) v.active = false;
    for (auto& d : drums) d.active = false;
    for (int t = 0; t < 3; ++t) for (int c = 0; c < 2; ++c) std::fill (chorusBuf[t][c].begin(), chorusBuf[t][c].end(), 0.0f);
    for (int c = 0; c < 2; ++c) std::fill (dlBuf[c].begin(), dlBuf[c].end(), 0.0f);
    dlLp[0] = dlLp[1] = 0; drumLpS[0] = drumLpS[1] = 0;
    reverb.reset(); comp.reset();
    pumpG = { 1, 1, 1, 1 }; pumpWait = { 0, 0, 0, 0 };
}

void Engine::setPatch (int track, std::unique_ptr<Patch> p)
{
    auto* raw = p.release();
    if (auto* old = pending[(size_t) track].exchange (raw)) delete old;   // ses iş parçacığı henüz almadıysa
}

void Engine::setKit (const juce::var& k)
{
    Kit x;
    auto K = k["k"], S = k["s"], C = k["c"], H = k["h"], O = k["o"], Rr = k["r"], P = k["p"];
    x.kf0 = (float) num (K["f0"], 160); x.kf1 = (float) num (K["f1"], 50); x.kpd = (float) num (K["pd"], .04); x.kd = (float) num (K["d"], .32); x.kclick = (float) num (K["click"]); x.kdrv = (float) num (K["drv"]);
    x.sf = (float) num (S["f"], 190); x.std_ = (float) num (S["td"], .1); x.snf = (float) num (S["nf"], 2400); x.snd = (float) num (S["nd"], .16); x.smix = (float) num (S["mix"], .55);
    x.cnf = (float) num (C["nf"], 1400); x.cd = (float) num (C["d"], .22);
    x.hhp = (float) num (H["hp"], 7500); x.hd = (float) num (H["d"], .045); x.hm = (float) num (H["m"]);
    x.ohp = (float) num (O["hp"], 6500); x.od = (float) num (O["d"], .28); x.om = (float) num (O["m"]);
    x.rf = (float) num (Rr["f"], 1800); x.pf = (float) num (P["f"], 330); x.pd = (float) num (P["d"], .15);
    auto pt = P["type"].toString(); x.ptype = pt == "triangle" ? 1 : pt == "square" ? 2 : 0;
    x.lp = (float) num (k["lp"]); x.rv = (float) num (k["rv"], .08);
    pendingKit = x; kitDirty.store (true, std::memory_order_release);
}

void Engine::retireNow (Patch* p)
{
    int s1, n1, s2, n2; trashFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) trash[(size_t) s1] = p;
    trashFifo.finishedWrite (n1);   // kuyruk doluysa (pratikte olmaz) yama sızar ama ses iş parçacığında silinmez
}

void Engine::collectGarbage()
{
    int s1, n1, s2, n2; trashFifo.prepareToRead (trashFifo.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) delete trash[(size_t) (s1 + i)];
    for (int i = 0; i < n2; ++i) delete trash[(size_t) (s2 + i)];
    trashFifo.finishedRead (n1 + n2);
}

Voice* Engine::findVoice (int track)
{
    Voice* best = nullptr; double oldest = 1e300; int count = 0;
    const int limit = track == 0 ? 28 : track == 1 ? 8 : 20;
    for (auto& v : voices) if (v.active && v.track == track) ++count;
    for (auto& v : voices) if (! v.active) { if (count < limit) return &v; break; }
    // ses çalma: önce bırakılmış, sonra en eski
    for (auto& v : voices) if (v.active && v.track == track && v.released && v.startClock < oldest) { oldest = v.startClock; best = &v; }
    if (best == nullptr) for (auto& v : voices) if (v.active && v.track == track && v.startClock < oldest) { oldest = v.startClock; best = &v; }
    if (best == nullptr) for (auto& v : voices) if (! v.active) return &v;
    return best;
}

void Engine::noteOn (int track, int note, int vel, double durSec, int)
{
    if (track == 2) { drumHit (note, vel, 0); return; }
    const int ti = track == 3 ? 2 : track;   // patch dizisi: 0 akor, 1 bas, 2 melodi
    Patch* p = cur[(size_t) ti]; if (p == nullptr) return;
    if (auto* v = findVoice (ti)) { startVoice (*v, *p, ti, note, vel, durSec); }
}

void Engine::noteOff (int track, int note, int)
{
    const int ti = track == 3 ? 2 : track;
    const float R = cur[(size_t) ti] ? cur[(size_t) ti]->R * std::pow (2.0f, macros[(size_t) (ti == 2 ? 3 : ti)].release * 3) : .3f;
    for (auto& v : voices) if (v.active && v.track == ti && v.gate && v.note == note && ! v.released) { v.gate = false; v.relT = v.t; v.endT = v.t + juce::jmax (.02f, R) * 2.5 + .05; }
}

void Engine::allNotesOff()
{
    for (auto& v : voices) if (v.active && ! v.released) { v.released = true; v.relT = v.t; v.gate = false; v.endT = v.t + 3.0; }
}

void Engine::startVoice (Voice& v, const Patch& p, int track, int note, int vel, double durSec)
{
    const Macros& m = macros[(size_t) (track == 2 ? 3 : track)];
    v = Voice();
    v.active = true; v.track = track; v.note = note; v.vel = vel / 127.0f; v.f0 = 440.0 * std::pow (2.0, (note - 69) / 12.0);
    v.startClock = voiceClock++; v.patch = &p;
    v.rng = 0x9E3779B9u ^ (uint32_t) (note * 2654435761u) ^ (uint32_t) v.startClock;
    const float atkM = std::pow (2.0f, m.attack * 3), relM = std::pow (2.0f, m.release * 3);
    const double A = juce::jmax (.002, (double) p.A * atkM), D = juce::jmax (.01, (double) p.D), R = juce::jmax (.02, (double) p.R * relM);
    v.gate = durSec < 0;
    const double end = v.gate ? 1e9 : juce::jmax (durSec, .05);
    v.relT = v.gate ? 1e9 : juce::jmax (end, A + .01);
    v.endT = v.relT + R * 2 + .05;
    v.peak = p.vol * p.trim * (.4f + .6f * v.vel);
    v.cD = (float) std::exp (-1.0 / (D / 3.0 * sr)); v.cR = (float) std::exp (-1.0 / (R / 3.0 * sr)); v.cA = (float) (1.0 / (A * sr)); v.atk = A;
    // kaliteye göre unison sınırı; çok ses aynı anda çalıyorsa tek sese düş
    int act = 0; for (auto& o : voices) if (o.active) ++act;
    v.uniN = juce::jmin (p.uniN, quality.maxUnison); if (act > (quality.maxUnison <= 2 ? 28 : 48)) v.uniN = 1;
    for (int u = 0; u < v.uniN; ++u)
    {
        const float pos = v.uniN > 1 ? (u / (float) (v.uniN - 1)) * 2 - 1 : 0;
        v.uniDet[u] = pos * p.uniSpread;
        const float pan = pos * .7f, ang = (pan + 1) * juce::MathConstants<float>::pi / 4;
        v.panL[u] = std::cos (ang) * 1.41421356f; v.panR[u] = std::sin (ang) * 1.41421356f;   // ortada 0 dB (web ile aynı seviye)
        for (int j = 0; j < 3; ++j) v.ph[j][u] = (frand (v.rng) + 1) * .5;
    }
    v.ctl = 0;
}

void Engine::renderVoice (Voice& v, const Patch& p, const Macros& m, float* L, float* R, int n)
{
    const double dt = 1.0 / sr; const int CR = quality.controlRate;
    const float uniNorm = 1.0f / std::sqrt ((float) v.uniN);
    const bool bass = p.cat == "Bas", vox = p.cat == "Vox";
    const float thickG = vox ? 0 : (bass ? .3f : .2f) * thick;
    const float fd = juce::jlimit (0.0f, 1.5f, p.fd + m.drive);
    const double cutMul = std::pow (2.0, m.cutoff * 3);
    double qBase = p.ftype == 2 ? p.q * (1 + m.reso * 3) : qFromDb (p.q + m.reso * 12);
    if (p.slope24 && p.ftype != 2) qBase = qFromDb ((p.q + m.reso * 12) * .7);
    const double c0 = juce::jlimit (60.0, 16000.0, p.cut * cutMul * std::pow (v.f0 / 261.63, (double) p.kt));
    const double pk = juce::jlimit (60.0, 18000.0, c0 * (1 + p.envAmt));
    const double lfoHz[3] = { p.lfo[0].sync ? bpm / 60.0 * p.lfo[0].rate : p.lfo[0].rate, p.lfo[1].sync ? bpm / 60.0 * p.lfo[1].rate : p.lfo[1].rate, p.lfo[2].rate };
    const double formScale = p.male ? .85 : 1;
    for (int i = 0; i < n; ++i)
    {
        if (v.ctl <= 0)
        {
            v.ctl = CR;
            // filtre zarfı (web: lineer çıkış ve üstel dönüş)
            double fc = c0;
            if (p.envAmt > 0)
            {
                if (p.envAtk > 0) fc = v.t < p.envAtk ? c0 + (pk - c0) * (v.t / p.envAtk) : c0 + (pk - c0) * std::exp (-(v.t - p.envAtk) / (juce::jmax (.02f, p.envDec) / 3.0));
                else fc = c0 + (pk - c0) * std::exp (-v.t / (juce::jmax (.02f, p.envDec) / 3.0));
            }
            double cents = 0; float ampL = 1;
            for (int l = 0; l < 3; ++l)
            {
                const auto& lf = p.lfo[(size_t) l]; if (lf.depth <= 0) continue;
                const float w = lfoWave (lf.shape, v.lfoPh[l]); v.lfoPh[l] += lfoHz[l] * CR * dt; v.lfoPh[l] -= std::floor (v.lfoPh[l]);
                if (lf.target == 0) fc += c0 * lf.depth * w; else if (lf.target == 1) cents += lf.depth * 100 * w; else ampL *= 1 - lf.depth / 2 + lf.depth / 2 * w;
            }
            if (p.hasVib) { const double ramp = juce::jlimit (0.0, 1.0, v.t / (p.vibDelay + .25)); cents += std::sin (kTwoPi * v.vibPh) * p.vibCents * ramp; v.vibPh += p.vibRate * CR * dt; v.vibPh -= std::floor (v.vibPh); }
            v.ampLfo = ampL;
            double pm = 1; if (p.hasPe) pm = 1 + (std::pow (2.0, p.peSemi / 12.0) - 1) * std::exp (-v.t / (juce::jmax (.005f, p.peTime) / 3.0));
            const double cm = std::pow (2.0, cents / 1200.0);
            for (int j = 0; j < p.numOsc; ++j)
            {
                const auto& o = p.osc[(size_t) j]; if (o.gain <= .001f) continue;
                const double fr = v.f0 * std::pow (2.0, (double) o.octave) * pm * cm;
                v.lv[j] = o.wave == 4 ? p.table.levelFor (fr) : basic[o.wave].levelFor (fr);
                for (int u = 0; u < v.uniN; ++u) v.inc[j][u] = fr * std::pow (2.0, (o.detune + v.uniDet[u]) / 1200.0) * dt;
            }
            v.carMul = (float) cm;
            if (p.hasFm) v.fmDev = (float) (p.fmIndex * v.f0 * p.fmRatio * (p.fmSus + (1 - p.fmSus) * std::exp (-v.t / (juce::jmax (.02f, p.fmDecay) / 3.0))));
            svfCoef (v.cf, fc, qBase, sr);
            if (p.slope24) svfCoef (v.cf2, fc, qBase, sr);
            if (p.vowel >= 0 && v.cfF[0][0] == 0) for (int f = 0; f < 3; ++f) svfCoef (v.cfF[f], kFormF[p.vowel][f] * formScale, 7 + f * 2, sr);
            if (p.noi > 0 && v.cfN[0] == 0) svfCoef (v.cfN, p.vowel >= 0 ? 1200 : 2500, .707, sr);
        }
        --v.ctl;
        // zarf
        if (v.t < v.relT)
        {
            if (v.t < v.atk) v.env = v.peak * (float) (v.t / v.atk);
            else v.env = v.peak * p.S + (v.env - v.peak * p.S) * v.cD;
        }
        else { if (! v.released) v.released = true; v.env *= v.cR; }
        // osilatörler (stereo unison)
        float l = 0, r = 0;
        for (int j = 0; j < p.numOsc; ++j)
        {
            const auto& o = p.osc[(size_t) j]; if (o.gain <= .001f) continue;
            const float* tb = v.lv[j]; const float gg = o.gain * uniNorm;
            for (int u = 0; u < v.uniN; ++u)
            {
                double& ph = v.ph[j][u]; ph += v.inc[j][u]; if (ph >= 1) ph -= 1;
                const float s = lookup (tb, ph) * gg; l += s * v.panL[u]; r += s * v.panR[u];
            }
        }
        float mono = 0;
        if (thickG > 0) { const double tf = bass ? v.f0 : v.f0 * .5; v.thPh += tf * dt; if (v.thPh >= 1) v.thPh -= 1; mono += lookup (basic[bass ? 3 : 2].levelFor (tf), v.thPh) * thickG; }
        if (p.sub > 0) { v.subPh += v.f0 * .5 * dt; if (v.subPh >= 1) v.subPh -= 1; mono += (float) std::sin (kTwoPi * v.subPh) * p.sub; }
        if (p.hasFm)
        {
            v.fmMod += v.f0 * p.fmRatio * dt; if (v.fmMod >= 1) v.fmMod -= 1;
            const double mod = std::sin (kTwoPi * v.fmMod) * v.fmDev;
            v.fmCar += (v.f0 * v.carMul + mod) * dt; v.fmCar -= std::floor (v.fmCar);
            mono += (float) std::sin (kTwoPi * v.fmCar) * p.fmLevel;
        }
        if (p.noi > 0) { const float nz = frand (v.rng); float st[2] = { v.nz1, v.nz2 }; mono += svf (nz, v.cfN, st).hp * p.noi; v.nz1 = st[0]; v.nz2 = st[1]; }
        float ch[2] = { l + mono, r + mono };
        for (int c = 0; c < 2; ++c)
        {
            float x = ch[c];
            if (fd > 0) x = sat (x, fd);
            if (p.vowel >= 0)
            {
                float y = 0; for (int f = 0; f < 3; ++f) y += svf (x, v.cfF[f], v.fsv[c][f]).bp * kFormG[p.vowel][f];
                x = y;
            }
            auto o1 = svf (x, v.cf, v.sv[c][0]);
            float y = p.ftype == 0 ? o1.lp : p.ftype == 1 ? o1.hp : o1.bp;
            if (p.slope24) { auto o2 = svf (y, v.cf2, v.sv[c][1]); y = p.ftype == 0 ? o2.lp : p.ftype == 1 ? o2.hp : o2.bp; }
            ch[c] = y * v.env * v.ampLfo;
        }
        L[i] += ch[0]; R[i] += ch[1];
        v.t += dt;
        if (v.t > v.endT || (v.released && v.env < 1e-5f)) { v.active = false; break; }
    }
}

void Engine::drumHit (int note, int vel, int)
{
    int type = note == 36 ? 0 : note == 38 ? 1 : note == 39 ? 2 : note == 42 ? 3 : note == 46 ? 4 : note == 37 ? 5 : 6;
    DrumVoice* d = nullptr; double oldest = -1;
    for (auto& x : drums) if (! x.active) { d = &x; break; }
    if (d == nullptr) for (auto& x : drums) if (x.t > oldest) { oldest = x.t; d = &x; }
    if (type == 4) for (auto& x : drums) if (x.active && x.type == 4) x.len = juce::jmin (x.len, x.t + .01);   // açık hat'ı yeni hat kısar
    if (type == 3) for (auto& x : drums) if (x.active && x.type == 4) x.len = juce::jmin (x.len, x.t + .02);
    *d = DrumVoice(); d->active = true; d->type = type; d->vel = vel / 127.0f; d->rng = 0x1234567u + (uint32_t) note * 977u + (uint32_t) voiceClock++;
    const auto& K = kit;
    switch (type)
    {
        case 0: d->len = K.kd * 1.6 + .1; svfCoef (d->cfA, 1800, .7, sr); break;
        case 1: d->len = juce::jmax (K.std_, K.snd) * 1.6 + .05; svfCoef (d->cfA, K.snf, .8, sr); break;
        case 2: d->len = K.cd * 1.6 + .05; svfCoef (d->cfA, K.cnf, 1.3, sr); break;
        case 3: d->len = K.hd * 1.8 + .02; svfCoef (d->cfA, K.hhp, .707, sr); break;
        case 4: d->len = K.od * 1.8 + .02; svfCoef (d->cfA, K.ohp, .707, sr); break;
        case 5: d->len = .08; svfCoef (d->cfA, 3400, 1, sr); break;
        default: d->len = K.pd * 1.6 + .05; svfCoef (d->cfA, K.pf * 3, 1.2, sr); break;
    }
    if (type == 0) { for (int t = 0; t < 4; ++t) if (pumpDepth[(size_t) t] > 0) { pumpG[(size_t) t] = 1 - juce::jmin (.95f, pumpDepth[(size_t) t] * pumpAmt); pumpWait[(size_t) t] = (int) (sr * .012); } }
}

void Engine::renderDrum (DrumVoice& d, float* L, float* R, int n)
{
    const auto& K = kit; const double dt = 1.0 / sr; const float v = d.vel;
    auto expd = [] (double t, double tau) { return (float) std::exp (-t / juce::jmax (1e-4, tau)); };
    static const float metal[6] = { 263, 400, 421, 474, 587, 845 };
    for (int i = 0; i < n; ++i)
    {
        const double t = d.t; float s = 0;
        switch (d.type)
        {
            case 0:
            {
                const double f = K.kf1 + (K.kf0 - K.kf1) * std::exp (-t / K.kpd);
                d.ph += f * dt; d.ph -= std::floor (d.ph);
                const float amp = t < K.kd * .15 ? v * .95f : v * .95f * expd (t - K.kd * .15, K.kd / 4);
                s = (float) std::sin (kTwoPi * d.ph) * amp;
                if (K.kdrv > 0) s = sat (s, K.kdrv);
                if (K.kclick > 0 && t < .05) s += svf (frand (d.rng), d.cfA, d.s[0]).hp * v * K.kclick * .5f * expd (t, .005);
                break;
            }
            case 1:
            {
                const double f = K.sf + (K.sf * .4) * std::exp (-t / .02); d.ph += f * dt; d.ph -= std::floor (d.ph);
                const float tri = (float) (d.ph < .5 ? 4 * d.ph - 1 : 3 - 4 * d.ph);
                s = tri * v * (1 - K.smix) * .8f * expd (t, K.std_ / 4) + svf (frand (d.rng), d.cfA, d.s[0]).bp * v * K.smix * 1.1f * expd (t, K.snd / 4);
                break;
            }
            case 2:
            {
                const float nz = svf (frand (d.rng), d.cfA, d.s[0]).bp;
                float e = 0; const double offs[3] = { 0, .011, .022 };
                for (int b = 0; b < 3; ++b) if (t >= offs[b]) e += expd (t - offs[b], (b < 2 ? .02 : K.cd) / 4);
                s = nz * v * .55f * e;
                break;
            }
            case 3: case 4:
            {
                const float m = d.type == 3 ? K.hm : K.om, dd = d.type == 3 ? K.hd : K.od;
                float x = frand (d.rng) * (1 - m * .6f);
                if (m > 0) { float sq = 0; for (int k = 0; k < 6; ++k) { d.ph2[k] += metal[k] * 1.7 * dt; d.ph2[k] -= std::floor (d.ph2[k]); sq += d.ph2[k] < .5 ? 1.0f : -1.0f; } x += sq * m * .18f; }
                s = svf (x, d.cfA, d.s[0]).hp * v * .42f * expd (t, dd / 4);
                break;
            }
            case 5:
            {
                d.ph += K.rf * dt; d.ph -= std::floor (d.ph);
                s = (float) std::sin (kTwoPi * d.ph) * v * .5f * expd (t, .008) + svf (frand (d.rng), d.cfA, d.s[0]).bp * v * .3f * expd (t, .03 / 4);
                break;
            }
            default:
            {
                const double f = K.pf + K.pf * .25 * std::exp (-t / .02); d.ph += f * dt; d.ph -= std::floor (d.ph);
                float w = K.ptype == 1 ? (float) (d.ph < .5 ? 4 * d.ph - 1 : 3 - 4 * d.ph) : K.ptype == 2 ? (d.ph < .5 ? 1.0f : -1.0f) : (float) std::sin (kTwoPi * d.ph);
                s = w * v * .7f * expd (t, K.pd / 4) + svf (frand (d.rng), d.cfA, d.s[0]).bp * v * .25f * expd (t, .03 / 4);
                break;
            }
        }
        L[i] += s; R[i] += s;
        d.t += dt;
        if (d.t > d.len) { d.active = false; break; }
    }
}

void Engine::applyChorus (int track, float amt, float* L, float* R, int n)
{
    auto& bL = chorusBuf[track][0]; auto& bR = chorusBuf[track][1]; const int size = (int) bL.size();
    int pos = chorusPos[track];
    static const double d0[2] = { .014, .021 }, rate[2] = { .7, .53 };
    for (int i = 0; i < n; ++i)
    {
        bL[(size_t) pos] = L[i]; bR[(size_t) pos] = R[i];
        float outL = L[i] * .85f, outR = R[i] * .85f;
        for (int k = 0; k < 2; ++k)
        {
            const double ph = chorusPh[track][k] + rate[k] * i / sr;
            const double dly = (d0[k] + .004 * std::sin (kTwoPi * ph)) * sr;
            double rp = pos - dly; while (rp < 0) rp += size;
            const int i0 = (int) rp; const float f = (float) (rp - i0);
            const float m = ((bL[(size_t) (i0 % size)] + bR[(size_t) (i0 % size)]) * (1 - f) + (bL[(size_t) ((i0 + 1) % size)] + bR[(size_t) ((i0 + 1) % size)]) * f) * .5f * amt;
            const float pan = k == 0 ? -.9f : .9f;
            outL += m * (1 - pan) * .5f * 1.2f; outR += m * (1 + pan) * .5f * 1.2f;
        }
        L[i] = outL; R[i] = outR;
        if (++pos >= size) pos = 0;
    }
    chorusPos[track] = pos; for (int k = 0; k < 2; ++k) { chorusPh[track][k] += rate[k] * n / sr; chorusPh[track][k] -= std::floor (chorusPh[track][k]); }
}

void Engine::render (juce::AudioBuffer<float>& out, int start, int num)
{
    juce::ScopedNoDenormals nd;
    const auto t0 = juce::Time::getHighResolutionTicks();
    // bekleyen yamaları ve kiti al
    for (size_t t = 0; t < 4; ++t)
        if (auto* p = pending[t].exchange (nullptr))
        {
            if (cur[t] != nullptr)
            {
                bool placed = false;
                for (auto& r : retiring) if (r == nullptr) { r = cur[t]; placed = true; break; }
                if (! placed) { for (auto& v : voices) if (v.active && v.patch == cur[t]) v.active = false; retireNow (cur[t]); }
            }
            cur[t] = p;
        }
    if (kitDirty.exchange (false, std::memory_order_acquire)) kit = pendingKit;
    for (auto& r : retiring)   // artık hiçbir sesin kullanmadığı eski yamaları çöpe gönder
        if (r != nullptr) { bool used = false; for (auto& v : voices) if (v.active && v.patch == r) { used = true; break; } if (! used) { retireNow (r); r = nullptr; } }

    int done = 0;
    while (done < num)
    {
        const int n = juce::jmin (maxBlockSize, num - done);
        partBuf.clear (0, n); sendRv.clear (0, n); sendDl.clear (0, n); drumBuf.clear (0, n);
        int active = 0;
        for (auto& v : voices)
            if (v.active && v.patch != nullptr)
            {
                ++active;
                const int mi = v.track == 2 ? 3 : v.track;
                renderVoice (v, *v.patch, macros[(size_t) mi], partBuf.getWritePointer (v.track * 2, 0), partBuf.getWritePointer (v.track * 2 + 1, 0), n);
            }
        for (auto& d : drums) if (d.active) { ++active; renderDrum (d, drumBuf.getWritePointer (0), drumBuf.getWritePointer (1), n); }
        lastActive = active;

        float* oL = out.getWritePointer (0, start + done); float* oR = out.getNumChannels() > 1 ? out.getWritePointer (1, start + done) : nullptr;
        float* rvL = sendRv.getWritePointer (0); float* rvR = sendRv.getWritePointer (1); float* dlL = sendDl.getWritePointer (0); float* dlR = sendDl.getWritePointer (1);
        juce::FloatVectorOperations::clear (oL, n); if (oR) juce::FloatVectorOperations::clear (oR, n);
        static const int trackMacro[3] = { 0, 1, 3 };
        for (int t = 0; t < 3; ++t)
        {
            const Patch* p = cur[(size_t) t]; if (p == nullptr) continue;
            const Macros& m = macros[(size_t) trackMacro[t]];
            float* L = partBuf.getWritePointer (t * 2); float* R = partBuf.getWritePointer (t * 2 + 1);
            const float inG = (t == 0 && p->chordsHalf) ? .6f : 1.0f;
            if (inG != 1.0f) { juce::FloatVectorOperations::multiply (L, inG, n); juce::FloatVectorOperations::multiply (R, inG, n); }
            if (p->ch > 0 && quality.chorus) applyChorus (t, p->ch, L, R, n);
            if (p->dr > 0) for (int i = 0; i < n; ++i) { L[i] = sat (L[i], p->dr); R[i] = sat (R[i], p->dr); }
            const float bus = m.mute ? 0.0f : m.volume;
            const float rv = juce::jlimit (0.0f, 1.0f, p->rv + m.reverb), dl = juce::jlimit (0.0f, 1.0f, p->dl + m.delay);
            const int pt = trackMacro[t]; float g = pumpG[(size_t) pt]; int wait = pumpWait[(size_t) pt];
            const float coef = (float) std::exp (-1.0 / (juce::jmax (.04f, pumpRel[(size_t) pt]) / 3.0 * sr));
            const bool pumping = pumpDepth[(size_t) pt] > 0 && pumpAmt > 0;
            for (int i = 0; i < n; ++i)
            {
                if (pumping) { if (wait > 0) --wait; else g = 1 - (1 - g) * coef; }
                const float gl = (pumping ? g : 1.0f) * bus, a = L[i] * gl, b = R[i] * gl;
                oL[i] += a; if (oR) oR[i] += b; else oL[i] += b;
                rvL[i] += a * rv; rvR[i] += b * rv; dlL[i] += a * dl; dlR[i] += b * dl;
            }
            pumpG[(size_t) pt] = g; pumpWait[(size_t) pt] = wait;
        }
        // davullar
        {
            const Macros& m = macros[2]; const float bus = m.mute ? 0.0f : m.volume;
            float* L = drumBuf.getWritePointer (0); float* R = drumBuf.getWritePointer (1);
            float lpC[4] {}; const bool lp = kit.lp > 0; if (lp) svfCoef (lpC, kit.lp, .707, sr);
            for (int i = 0; i < n; ++i)
            {
                float a = L[i], b = R[i];
                if (lp) { float st[2] = { drumLpS[0], drumLpS[1] }; a = b = svf (a, lpC, st).lp; drumLpS[0] = st[0]; drumLpS[1] = st[1]; }
                a *= bus; b *= bus;
                oL[i] += a; if (oR) oR[i] += b; rvL[i] += a * kit.rv; rvR[i] += b * kit.rv;
            }
        }
        // delay (noktalı sekizlik, geri besleme LP)
        {
            const int size = (int) dlBuf[0].size(); const double dsec = juce::jmin (1.9, 60.0 / juce::jmax (40.0f, bpm) * .75);
            const int dsmp = juce::jlimit (1, size - 1, (int) (dsec * sr)); float lpc = (float) std::exp (-kTwoPi * 2800 / sr);
            for (int i = 0; i < n; ++i)
            {
                int rp = dlPos - dsmp; if (rp < 0) rp += size;
                for (int c = 0; c < 2; ++c)
                {
                    const float y = dlBuf[c][(size_t) rp]; dlLp[c] = y + (dlLp[c] - y) * lpc;
                    dlBuf[c][(size_t) dlPos] = (c == 0 ? dlL[i] : dlR[i]) + dlLp[c] * .38f;
                }
                oL[i] += dlLp[0] * .6f; if (oR) oR[i] += dlLp[1] * .6f;
                rvL[i] += dlLp[0] * .3f; rvR[i] += dlLp[1] * .3f;
                if (++dlPos >= size) dlPos = 0;
            }
        }
        reverb.processStereo (rvL, rvR, n);
        for (int i = 0; i < n; ++i) { oL[i] += rvL[i] * .55f; if (oR) oR[i] += rvR[i] * .55f; }
        // master: kazanç, kompresör, yumuşak limiter, güvenlik sınırı
        juce::FloatVectorOperations::multiply (oL, .42f, n); if (oR) juce::FloatVectorOperations::multiply (oR, .42f, n);
        {
            float* chs[2] = { oL, oR ? oR : oL };
            juce::dsp::AudioBlock<float> blk (chs, oR ? 2u : 1u, (size_t) n);
            juce::dsp::ProcessContextReplacing<float> ctx (blk); comp.process (ctx);
        }
        float pk = 0; bool bad = false;
        for (int c = 0; c < (oR ? 2 : 1); ++c)
        {
            float* x = c == 0 ? oL : oR;
            for (int i = 0; i < n; ++i)
            {
                // web DynamicsCompressor'daki otomatik makeup kazancına (~+6 dB) denk; sonra web'deki gibi yumuşak eğri
                float y = sat (juce::jlimit (-1.0f, 1.0f, x[i] * 2.0f), 0) * .891f * master;   // tavan -1 dBFS
                if (! std::isfinite (y)) { bad = true; y = 0; }
                x[i] = juce::jlimit (-1.0f, 1.0f, y); pk = juce::jmax (pk, std::abs (x[i]));
            }
        }
        if (bad) reset();   // sayısal hata: tüm durumları sıfırla, sessizlikle devam et
        peakOut.store (juce::jmax (pk, peakOut.load (std::memory_order_relaxed) * .96f), std::memory_order_relaxed);
        done += n;
    }
    const double el = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
    cpuLoad.store ((float) (el / (num / sr)) * .2f + cpuLoad.load (std::memory_order_relaxed) * .8f, std::memory_order_relaxed);
}
}
