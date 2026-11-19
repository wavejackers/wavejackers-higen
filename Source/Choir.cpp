#include "Choir.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

namespace ch
{
namespace
{
constexpr double kPi = juce::MathConstants<double>::pi, kTau = juce::MathConstants<double>::twoPi;
constexpr int kActPerBar = 32;
const float kPartFormant[kParts] = { .9f, .96f, 1.08f, 1.16f };   // bas daha koyu, soprano daha parlak ses yolu
const float kPartPan[kParts] = { -.12f, .38f, -.42f, .2f };

struct Vowel { float f[4], g[4], bw[4], noise; };
const Vowel kVowels[4] = { { { 730, 1090, 2440, 3400 }, { 1, .55f, .3f, .12f }, { 90, 100, 130, 150 }, .035f },     // Aah
                           { { 320, 800, 2300, 3200 }, { 1, .3f, .08f, .04f }, { 70, 80, 110, 140 }, .02f },        // Ooh
                           { { 420, 1900, 2600, 3350 }, { 1, .35f, .3f, .12f }, { 70, 100, 130, 150 }, .03f },      // Eeh
                           { { 260, 1050, 2200, 3000 }, { 1, .1f, .04f, .02f }, { 60, 90, 120, 140 }, .008f } };    // Mmm

struct Singer { double cents = 0, delay = 0, vibRate = 5.5, vibPhase = 0, vibDepth = 0, formant = 1, startJit = 0, loopJit = 0; float pan = 0; juce::int64 seed = 1; };

inline double vibratoCents (const Singer& sg, double sec)   // vibrato nota başladıktan sonra yavaşça açılır
{
    const double on = juce::jlimit (0.0, 1.0, (sec - .18) / .5);
    return sg.cents + std::sin (kTau * sg.vibRate * sec + sg.vibPhase) * sg.vibDepth * on * on + std::sin (kTau * .31 * sec + sg.vibPhase * 1.7) * 3.0;
}

// Dahili ünlü: gırtlak darbesi + nefes gürültüsü, dört paralel formant süzgecinden geçer
void synthNote (float* dst, int len, double f0, const Singer& sg, int vowel, double sr)
{
    const auto& V = kVowels[juce::jlimit (0, 3, vowel)];
    struct Bp { double b0, a1, a2, z1 = 0, z2 = 0; } bp[4];
    for (int k = 0; k < 4; ++k)
    {
        double f = V.f[k] * sg.formant; if (k == 0) f = juce::jmax (f, f0 * 1.02);   // soprano: ilk formant perdenin altında kalmaz
        f = juce::jmin (f, sr * .45);
        const double w = kTau * f / sr, al = std::sin (w) / (2 * f / V.bw[k]), a0 = 1 + al;
        bp[k].b0 = al / a0; bp[k].a1 = -2 * std::cos (w) / a0; bp[k].a2 = (1 - al) / a0;
    }
    juce::Random r (sg.seed); double ph = r.nextDouble(), lp = 0;
    for (int i = 0; i < len; ++i)
    {
        const double sec = i / sr, f = f0 * std::pow (2.0, vibratoCents (sg, sec) / 1200.0);
        ph += f / sr; if (ph >= 1) ph -= 1;
        const double open = ph < .6 ? .5 * (1 - std::cos (kPi * ph / .6)) : ph < .8 ? std::cos (kPi * .5 * (ph - .6) / .2) : 0.0;
        const double src = (open - .36) + (r.nextDouble() * 2 - 1) * V.noise * (.4 + open) * 6;
        const double x = src - lp; lp += (src - lp) * .002;     // DC'yi at
        double y = 0;
        for (int k = 0; k < 4; ++k) { auto& b = bp[k]; const double o = b.b0 * x + b.z1; b.z1 = -b.a1 * o + b.z2; b.z2 = -b.b0 * x - b.a2 * o; y += o * V.g[k]; }
        dst[i] += (float) (y * 2.2);
    }
}

// Kullanıcının sesi: TD-PSOLA. Perde hedef notaya sabitlenir, tanecikler formantı korur; uzatma bölgesinde ileri geri gezilir.
void psolaNote (float* dst, int len, double f0, const Singer& sg, const Voice& V, bool hold, double sr)
{
    const int M = (int) V.mark.size(); if (M < 4 || V.x.size() < 64) return;
    const double ratio = V.sr / sr, rho = sg.formant * ratio, first = V.mark[0], end = V.mark[(size_t) M - 1];
    const int la = juce::jlimit (0, M - 2, V.loopA), lb = juce::jlimit (la + 1, M - 1, V.loopB - (int) (sg.loopJit * (V.loopB - V.loopA) * .3));
    const double A = V.mark[(size_t) la], B = V.mark[(size_t) lb];
    double pos = hold ? first + sg.startJit * (A - first) * .6 : first, t = 0; int dir = 1;
    const int X = (int) V.x.size();
    while (t < len)
    {
        const double f = f0 * std::pow (2.0, vibratoCents (sg, t / sr) / 1200.0), T = sr / f;
        const auto it = std::lower_bound (V.mark.begin(), V.mark.end(), (int) pos);
        int m = juce::jlimit (0, M - 1, (int) (it - V.mark.begin()));
        if (m > 0 && std::abs (V.mark[(size_t) m - 1] - pos) < std::abs (V.mark[(size_t) m] - pos)) --m;
        const double Pa = V.period[(size_t) m], c = V.mark[(size_t) m], h = Pa / rho;
        const int half = juce::jmax (2, (int) h);
        const float g = (float) std::pow (juce::jlimit (.3, 3.0, T / h), .7);
        for (int k = -half; k <= half; ++k)
        {
            const int o = (int) t + k; if (o < 0 || o >= len) continue;
            const double sp = c + k * rho; const int i0 = (int) std::floor (sp); if (i0 < 0 || i0 + 1 >= X) continue;
            const float fr = (float) (sp - i0), w = .5f + .5f * (float) std::cos (kPi * k / half);
            dst[o] += (V.x[(size_t) i0] + (V.x[(size_t) i0 + 1] - V.x[(size_t) i0]) * fr) * w * g;
        }
        t += T; pos += dir * T * ratio;
        if (hold) { if (pos >= B) { pos = juce::jmax (A, 2 * B - pos); dir = -1; } else if (dir < 0 && pos <= A) { pos = juce::jmin (B, 2 * A - pos); dir = 1; } }
        else if (pos >= end) break;
    }
}
}

std::vector<std::array<int, kParts>> voicing (const std::vector<Bar>& bars)
{
    static const int lo[kParts] = { 40, 48, 55, 60 }, hi[kParts] = { 52, 60, 67, 76 };
    std::vector<std::array<int, kParts>> out; int prev[kParts] = { 45, 55, 62, 69 };
    auto near = [&] (int part, int pc)
    {
        int best = lo[part], bd = 999;
        for (int n = lo[part]; n <= hi[part]; ++n) if (((n - pc) % 12 + 12) % 12 == 0) { const int d = std::abs (n - prev[part]); if (d <= bd) { bd = d; best = n; } }
        return best;
    };
    for (const auto& b : bars)
    {
        const int t[3] = { ((b.pc % 12) + 12) % 12, ((b.pc + b.third) % 12 + 12) % 12, ((b.pc + b.fifth) % 12 + 12) % 12 };
        std::array<int, kParts> n {}; n[0] = near (0, t[0]);
        bool used[3] = { false, false, false };
        for (int part : { 3, 2, 1 })   // soprano en yakın sese; alto ve tenor kalan akor seslerini alır (akor eksiksiz duyulur)
        {
            int bi = -1, bd = 999;
            for (int k = 0; k < 3; ++k) { if (used[k]) continue; const int d = std::abs (near (part, t[k]) - prev[part]); if (d < bd) { bd = d; bi = k; } }
            used[bi] = true; n[(size_t) part] = near (part, t[bi]);
        }
        for (int p = 0; p < kParts; ++p) prev[p] = n[(size_t) p];
        out.push_back (n);
    }
    return out;
}

double detectPitch (const float* x, int n, double sr)
{
    const int minLag = juce::jmax (2, (int) (sr / 900)), maxLag = juce::jmin (n / 2, (int) (sr / 65));
    if (maxLag <= minLag + 2) return 0;
    const int W = n - maxLag; double en = 0;
    for (int j = 0; j < W; ++j) en += x[j] * x[j];
    if (en / W < 1e-6) return 0;
    std::vector<double> d ((size_t) maxLag + 1, 1.0); double run = 0;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        double s = 0; for (int j = 0; j < W; ++j) { const double e = x[j] - x[j + tau]; s += e * e; }
        run += s; d[(size_t) tau] = run > 0 ? s * tau / run : 1.0;      // YIN: kümülatif ortalamaya göre normalleştirilmiş fark
    }
    int best = -1;
    for (int tau = minLag; tau < maxLag; ++tau) if (d[(size_t) tau] < .15) { while (tau + 1 < maxLag && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau; best = tau; break; }
    if (best < 0) { best = minLag; for (int tau = minLag; tau < maxLag; ++tau) if (d[(size_t) tau] < d[(size_t) best]) best = tau; if (d[(size_t) best] > .35) return 0; }
    double tau = best;
    if (best > 1 && best < maxLag) { const double a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1], den = a - 2 * b + c; if (std::abs (den) > 1e-12) tau += .5 * (a - c) / den; }
    return sr / tau;
}

Voice analyse (const juce::AudioBuffer<float>& b, double sr)
{
    Voice v; v.sr = sr;
    int N = juce::jmin (b.getNumSamples(), (int) (sr * kMaxVoiceSeconds)); if (N < (int) (sr * .05) || b.getNumChannels() < 1) return v;
    std::vector<float> x ((size_t) N, 0.0f); double dc = 0;
    for (int c = 0; c < b.getNumChannels(); ++c) { auto* p = b.getReadPointer (c); for (int i = 0; i < N; ++i) x[(size_t) i] += p[i] / (float) b.getNumChannels(); }
    for (auto s : x) dc += s; dc /= N; float pk = 0; for (auto& s : x) { s -= (float) dc; pk = juce::jmax (pk, std::abs (s)); }
    if (pk < 1e-4f) return v;
    int a = 0, z = N - 1; const float thr = pk * .03f; const int pad = (int) (sr * .01);
    while (a < z && std::abs (x[(size_t) a]) < thr) ++a;
    while (z > a && std::abs (x[(size_t) z]) < thr) --z;
    a = juce::jmax (0, a - pad); z = juce::jmin (N - 1, z + pad); N = z - a + 1; if (N < (int) (sr * .05)) return v;
    v.x.assign (x.begin() + a, x.begin() + a + N); for (auto& s : v.x) s *= .9f / pk;
    const int fadeN = juce::jmin (N / 2, (int) (sr * .004)); for (int i = 0; i < fadeN; ++i) { const float g = (float) i / fadeN; v.x[(size_t) i] *= g; v.x[(size_t) (N - 1 - i)] *= g; }
    // ---- perde izi: ~12 kHz'e indirilmiş işaret üzerinde YIN, 10 ms adımla
    const int D = juce::jmax (1, (int) std::lround (sr / 12000.0)); const double dsr = sr / D; const int DN = N / D;
    std::vector<float> ds ((size_t) juce::jmax (1, DN));
    for (int i = 0; i < DN; ++i) { float s = 0; for (int k = 0; k < D; ++k) s += v.x[(size_t) (i * D + k)]; ds[(size_t) i] = s / D; }
    const int F = (int) (dsr * .045), H = juce::jmax (1, (int) (dsr * .01)); std::vector<double> f0;
    for (int at = 0; at + F <= DN; at += H) f0.push_back (detectPitch (ds.data() + at, F, dsr));
    std::vector<double> vf; for (auto f : f0) if (f > 0) vf.push_back (f);
    v.voiced = f0.empty() ? 0.0f : (float) vf.size() / (float) f0.size();
    if (v.voiced >= .15f)
    {
        std::nth_element (vf.begin(), vf.begin() + (long) vf.size() / 2, vf.end()); v.f0 = vf[vf.size() / 2];
        for (auto& f : f0) { if (f > v.f0 * 1.8) f *= .5; else if (f > 0 && f < v.f0 * .55) f *= 2; }   // oktav hataları
    }
    else for (auto& f : f0) f = 0;
    const double defP = sr / (v.f0 > 0 ? v.f0 : 160.0);
    auto frameAt = [&] (double t) { const int i = (int) ((t / D - F * .5) / H); return f0.empty() ? 0.0 : f0[(size_t) juce::jlimit (0, (int) f0.size() - 1, i)]; };
    // ---- perde işaretleri: her periyotta bir, dalganın tepesine oturtulur
    auto peakIn = [&] (int from, int to) { from = juce::jmax (0, from); to = juce::jmin (N - 1, to); int bi = from; for (int i = from; i <= to; ++i) if (v.x[(size_t) i] > v.x[(size_t) bi]) bi = i; return bi; };
    std::vector<char> vd; double t = peakIn (0, (int) defP);
    while (t < N)
    {
        const double f = frameAt (t), P = f > 0 ? sr / f : defP;
        v.mark.push_back ((int) t); v.period.push_back ((float) P); vd.push_back (f > 0);
        const double nx = t + P; t = f > 0 ? juce::jmax (t + P * .5, (double) peakIn ((int) (nx - P * .25), (int) (nx + P * .25))) : nx;
    }
    // ---- uzatma bölgesi: en uzun perdeli kesitin ortası (ünlünün kararlı kısmı)
    const int M = (int) v.mark.size(); int ba = 0, bl = 0;
    for (int i = 0; i < M;) { if (! vd[(size_t) i]) { ++i; continue; } int j = i; while (j < M && vd[(size_t) j]) ++j; if (j - i > bl) { bl = j - i; ba = i; } i = j; }
    if (bl >= 12) { v.loopA = ba + (int) (bl * .3); v.loopB = ba + bl - 1 - (int) (bl * .15); }
    else { v.loopA = M / 4; v.loopB = juce::jmax (v.loopA + 1, M * 4 / 5); }
    v.loopB = juce::jlimit (0, juce::jmax (0, M - 1), v.loopB); v.loopA = juce::jlimit (0, v.loopB, v.loopA);
    return v;
}

juce::ValueTree Settings::toTree() const
{
    juce::ValueTree t ("choir"); juce::String p; for (bool b : part) p << (b ? '1' : '0');
    t.setProperty ("source", source, nullptr); t.setProperty ("rhythm", rhythm, nullptr); t.setProperty ("size", size, nullptr); t.setProperty ("seed", seed, nullptr); t.setProperty ("parts", p, nullptr);
    t.setProperty ("level", level, nullptr); t.setProperty ("space", space, nullptr); t.setProperty ("detune", detune, nullptr); t.setProperty ("air", air, nullptr);
    t.setProperty ("vibrato", vibrato, nullptr); t.setProperty ("attack", attack, nullptr); t.setProperty ("character", character, nullptr);
    return t;
}

void Settings::fromTree (const juce::ValueTree& t)
{
    if (! t.hasType ("choir")) return;
    auto I = [&] (const char* k, int d, int lo, int hi) { return juce::jlimit (lo, hi, (int) t.getProperty (k, d)); };
    auto F = [&] (const char* k, float d, float lo = 0) { return juce::jlimit (lo, 1.0f, (float) t.getProperty (k, d)); };
    source = I ("source", sAah, 0, kNumSources - 1); rhythm = I ("rhythm", rHold, 0, kNumRhythms - 1); size = I ("size", 3, 1, 4); seed = I ("seed", 1, 0, 0x7fffffff);
    const auto p = t.getProperty ("parts", "1111").toString(); for (int i = 0; i < kParts; ++i) part[(size_t) i] = i >= p.length() || p[i] != '0';
    level = F ("level", .7f); space = F ("space", .6f); detune = F ("detune", .35f); air = F ("air", .4f); vibrato = F ("vibrato", .3f); attack = F ("attack", .4f); character = F ("character", 0, -1.0f);
}

std::unique_ptr<Loop> render (const Settings& s, const std::vector<Bar>& barsIn, const Voice& voice, double bpm, double sr, const std::function<bool()>& cancelled)
{
    bpm = juce::jlimit (40.0, 240.0, bpm);
    std::vector<Bar> bars = barsIn; if (bars.empty()) bars.push_back ({});
    const int NB = (int) bars.size(); const double beat = 60.0 / bpm * sr, barLen = beat * 4;
    const int N = juce::jmax (64, (int) std::llround (barLen * NB));
    auto L = std::make_unique<Loop>(); L->bpm = bpm; L->sr = sr; L->bars = NB; L->buf.setSize (2, N); L->buf.clear();
    for (auto& a : L->act) a.assign ((size_t) (NB * kActPerBar), 0.0f);
    const bool own = s.source >= sOwnHold && voice.mark.size() >= 4, words = own && s.source == sOwnWords;
    const auto notes = voicing (bars);
    juce::AudioBuffer<float> dry (2, N); dry.clear(); float* dl = dry.getWritePointer (0); float* dr = dry.getWritePointer (1);
    const bool stab = s.rhythm == rStab;
    const double atk = stab ? .012 : .012 + s.attack * s.attack * .6, rel = stab ? .09 : .14 + s.attack * .4;
    const double charF = std::pow (2.0, s.character * .25);
    std::vector<float> tmp;
    for (int p = 0; p < kParts; ++p)
    {
        if (! s.part[(size_t) p]) continue;
        struct Ev { double at, len; int midi; }; std::vector<Ev> evs;
        for (int b = 0; b < NB; ++b)
        {
            const int m = notes[(size_t) b][(size_t) p];
            if (s.rhythm == rHold) { if (! words && ! evs.empty() && evs.back().midi == m && evs.back().at + evs.back().len >= b * barLen - 1) evs.back().len += barLen; else evs.push_back ({ b * barLen, barLen, m }); }
            else if (s.rhythm == rHalf) for (int k = 0; k < 2; ++k) evs.push_back ({ b * barLen + k * beat * 2, beat * 2 * .96, m });
            else if (s.rhythm == rQuarter) for (int k = 0; k < 4; ++k) evs.push_back ({ b * barLen + k * beat, beat * .8, m });
            else for (int k = 0; k < 4; ++k) evs.push_back ({ b * barLen + (k + .5) * beat, beat * .4, m });
        }
        const int singers = juce::jlimit (1, 4, s.size); const float gain = .5f / std::sqrt ((float) singers);
        for (int q = 0; q < singers; ++q)
        {
            if (cancelled && cancelled()) return L;
            juce::Random r ((juce::int64) s.seed * 7919 + p * 131 + q * 17 + 5);
            Singer sg; const double sp = singers > 1 ? (double) q / (singers - 1) * 2 - 1 : 0.0;   // -1..1: şarkıcılar sahneye yayılır
            sg.cents = sp * s.detune * 16 + (r.nextDouble() - .5) * s.detune * 10; sg.delay = q == 0 ? 0.0 : (r.nextDouble() - .3) * s.detune * (stab ? .012 : .04);
            sg.vibRate = 4.9 + r.nextDouble() * 1.3; sg.vibPhase = r.nextDouble() * kTau; sg.vibDepth = s.vibrato * (22 + r.nextDouble() * 16);
            sg.formant = kPartFormant[p] * charF * (1 + (r.nextDouble() - .5) * .06); sg.startJit = q == 0 ? 0.0 : r.nextDouble(); sg.loopJit = r.nextDouble();
            sg.pan = juce::jlimit (-1.0f, 1.0f, kPartPan[p] + (float) sp * .22f);
            const float pl = std::cos ((sg.pan + 1) * (float) kPi * .25f), pr = std::sin ((sg.pan + 1) * (float) kPi * .25f);
            int ei = 0;
            for (const auto& e : evs)
            {
                const int len = (int) (e.len + rel * sr), aN = juce::jmax (8, (int) (atk * sr)), rN = juce::jmax (8, (int) (rel * sr)), gate = (int) e.len;
                tmp.assign ((size_t) len, 0.0f); sg.seed = (juce::int64) s.seed * 31 + p * 1009 + q * 101 + (ei++) * 7;
                const double f0 = 440.0 * std::pow (2.0, (e.midi - 69) / 12.0);
                if (own) psolaNote (tmp.data(), len, f0, sg, voice, ! words, sr); else synthNote (tmp.data(), len, f0, sg, s.source, sr);
                const long long at = std::llround (e.at + sg.delay * sr);
                for (int i = 0; i < len; ++i)
                {
                    const float env = juce::jmin (1.0f, (float) i / aN) * (i < gate ? 1.0f : juce::jmax (0.0f, 1.0f - (float) (i - gate) / rN));
                    const float x = tmp[(size_t) i] * env * gain; const int o = (int) (((at + i) % N + N) % N);
                    dl[o] += x * pl; dr[o] += x * pr;
                    if (q == 0 && (i & 255) == 0) { auto& a = L->act[(size_t) p]; const size_t ai = (size_t) ((long long) o * (long long) a.size() / N); a[ai] = juce::jmax (a[ai], env); }
                }
            }
        }
    }
    // ---- katedral: alçak kesim, hava (tiz parlaklık), ön gecikmeli geniş reverb; iki tur (kuyruk döngünün başına sarar)
    juce::Reverb rev; juce::Reverb::Parameters rp; rp.roomSize = .6f + .38f * s.space; rp.damping = .35f; rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1; rev.setSampleRate (sr); rev.setParameters (rp);
    const int pdN = juce::jmax (1, (int) (sr * .022)); std::vector<float> pdL ((size_t) pdN, 0.0f), pdR ((size_t) pdN, 0.0f); int pd = 0;
    const float hpC = (float) (1 - std::exp (-kTau * 90 / sr)), airC = (float) (1 - std::exp (-kTau * 5000 / sr)), wet = .95f * s.space, dryG = 1 - .35f * s.space;
    float hl = 0, hr = 0, al = 0, ar = 0; float* ol = L->buf.getWritePointer (0); float* orr = L->buf.getWritePointer (1);
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < N; ++i)
        {
            float xl = dl[i], xr = dr[i];
            hl += (xl - hl) * hpC; hr += (xr - hr) * hpC; xl -= hl; xr -= hr;
            al += (xl - al) * airC; ar += (xr - ar) * airC; xl += (xl - al) * s.air * 1.2f; xr += (xr - ar) * s.air * 1.2f;
            float rl = pdL[(size_t) pd], rr = pdR[(size_t) pd]; pdL[(size_t) pd] = xl; pdR[(size_t) pd] = xr; pd = (pd + 1) % pdN;
            rev.processStereo (&rl, &rr, 1);
            ol[i] = xl * dryG + rl * wet; orr[i] = xr * dryG + rr * wet;
        }
    const float pk = L->buf.getMagnitude (0, N);
    if (pk > 1e-6f) L->buf.applyGain (.6f * s.level / pk);
    return L;
}

// ---- motor
Engine::Engine() : juce::Thread ("Choir render")
{
    formats.registerBasicFormats();
    for (auto& t : trash) t.store (nullptr);
    for (auto& a : act) a.store (0);
    bars = { { 9, 3, 7 }, { 5, 4, 7 }, { 0, 4, 7 }, { 7, 4, 7 } };   // Am F C G: akor gelene kadar
    startThread();
}

Engine::~Engine()
{
    signalThreadShouldExit(); notify(); stopThread (6000);
    delete pending.exchange (nullptr); delete live; collect();
}

void Engine::prepare (double sampleRate) { sr = sampleRate; changed(); }

void Engine::process (juce::AudioBuffer<float>& out, int n, bool hostPlay, double ppq, double bpm) noexcept
{
    if (auto* np = pending.exchange (nullptr))
    {
        if (live != nullptr) { bool put = false; for (auto& t : trash) { Loop* e = nullptr; if (t.compare_exchange_strong (e, live)) { put = true; break; } } if (! put) delete live; }
        live = np;
    }
    if (! playing.load() || live == nullptr || out.getNumChannels() < 2) { for (auto& a : act) a.store (0, std::memory_order_relaxed); return; }
    const int N = live->buf.getNumSamples(); if (N < 2) return;
    if (hostPlay) { const double beats = live->bars * 4.0; freePos = std::fmod (std::fmod (ppq, beats) + beats, beats) / beats * N; }
    const double inc = hostPlay ? bpm / live->bpm : 1.0;
    const float* l = live->buf.getReadPointer (0); const float* r = live->buf.getReadPointer (1);
    float* ol = out.getWritePointer (0); float* orr = out.getWritePointer (1);
    double p = freePos;
    for (int i = 0; i < n; ++i)
    {
        if (p >= N) p -= N;
        const int i0 = (int) p, i1 = i0 + 1 < N ? i0 + 1 : 0; const float f = (float) (p - i0);
        ol[i] += l[i0] + (l[i1] - l[i0]) * f; orr[i] += r[i0] + (r[i1] - r[i0]) * f;
        p += inc;
    }
    if (p >= N) p -= N;
    freePos = p; headPos.store ((float) (p / N));
    for (int k = 0; k < kParts; ++k) { const auto& a = live->act[(size_t) k]; act[(size_t) k].store (a.empty() ? 0.0f : a[(size_t) juce::jmin ((int) a.size() - 1, (int) (p / N * a.size()))], std::memory_order_relaxed); }
}

void Engine::feed (const float* l, const float* r, int n, double rate) noexcept
{
    if (recState.load() != 1 || l == nullptr || rate <= 0) return;
    feeding.store (1);
    if (recState.load() == 1)
    {
        if (recRate <= 0) recRate = rate;
        int w = recPos.load (std::memory_order_relaxed); const int cap = juce::jmin ((int) rec.size(), (int) (recRate * kMaxVoiceSeconds)); float pk = recPeak.load (std::memory_order_relaxed) * .9f;
        for (int i = 0; i < n && w < cap; ++i) { const float x = r != nullptr ? (l[i] + r[i]) * .5f : l[i]; rec[(size_t) w++] = x; pk = juce::jmax (pk, std::abs (x)); }
        recPos.store (w, std::memory_order_release); recPeak.store (pk, std::memory_order_relaxed);
    }
    feeding.store (0);
}

void Engine::recordStart()
{
    recState.store (0); for (int i = 0; i < 50 && feeding.load() != 0; ++i) juce::Thread::sleep (2);
    rec.assign ((size_t) (192000 * kMaxVoiceSeconds), 0.0f); recPos.store (0); recPeak.store (0); recRate = 0;
    recState.store (1);
}

bool Engine::recordStop()
{
    if (recState.exchange (0) != 1) return false;
    for (int i = 0; i < 50 && feeding.load() != 0; ++i) juce::Thread::sleep (2);
    const int n = recPos.load(); const double rate = recRate;
    juce::AudioBuffer<float> b (1, juce::jmax (1, n)); if (n > 0) b.copyFrom (0, 0, rec.data(), n);
    rec.clear(); rec.shrink_to_fit();
    if (rate <= 0 || n < (int) (rate * .15) || b.getMagnitude (0, n) < 2e-3f) return false;
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Wavejackers HitGen").getChildFile ("Choir");
    dir.createDirectory();   // kayıt kalıcı olsun: proje yeniden açıldığında dosyadan yüklenir
    auto f = dir.getChildFile ("Voice " + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M-%S") + ".wav");
    if (auto os = f.createOutputStream())
        if (std::unique_ptr<juce::AudioFormatWriter> wr { juce::WavAudioFormat().createWriterFor (os.get(), rate, 1, 24, {}, 0) }) { os.release(); wr->writeFromAudioSampleBuffer (b, 0, n); }
    return setVoice (std::move (b), rate, "Voice", f.existsAsFile() ? f : juce::File());
}

void Engine::collect() { for (auto& t : trash) delete t.exchange (nullptr); }
void Engine::changed() { wantVersion.fetch_add (1); notify(); }

void Engine::run()
{
    while (! threadShouldExit())
    {
        const int want = wantVersion.load();
        if (want == doneVersion.load()) { wait (200); continue; }
        Settings cs; std::vector<Bar> cb; Voice cv; double bpm, osr;
        { const juce::ScopedLock sl (lock); cs = s; cb = bars; if (cs.source >= sOwnHold) cv = voice; bpm = tempo; osr = sr; }
        auto L = render (cs, cb, cv, bpm, osr, [this] { return threadShouldExit(); });
        if (threadShouldExit()) break;
        { const juce::ScopedLock sl (lock); last = std::make_unique<Loop> (*L); }
        delete pending.exchange (L.release());
        doneVersion.store (want);
    }
}

bool Engine::setVoice (juce::AudioBuffer<float>&& b, double bsr, const juce::String& nm, const juce::File& f)
{
    auto v = analyse (b, bsr); if (v.mark.size() < 4) return false;
    { const juce::ScopedLock sl (lock); voice = std::move (v); name = nm; path = f; if (s.source < sOwnHold) s.source = sOwnHold; }   // ses yüklenince koro onu söyler
    changed(); return true;
}

bool Engine::loadFile (const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (f));
    if (rd == nullptr || rd->lengthInSamples < 16) return false;
    const int len = (int) juce::jmin<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * kMaxVoiceSeconds));
    juce::AudioBuffer<float> b (juce::jmin (2, (int) rd->numChannels), len);
    rd->read (&b, 0, len, 0, true, true);
    return setVoice (std::move (b), rd->sampleRate, f.getFileNameWithoutExtension(), f);
}

Settings Engine::get() const { const juce::ScopedLock sl (lock); return s; }
void Engine::set (const Settings& n) { { const juce::ScopedLock sl (lock); s = n; } changed(); }
void Engine::setChords (const std::vector<Bar>& b)
{
    { const juce::ScopedLock sl (lock); if (b.size() == bars.size() && std::equal (b.begin(), b.end(), bars.begin(), [] (const Bar& x, const Bar& y) { return x.pc == y.pc && x.third == y.third && x.fifth == y.fifth; })) return; bars = b; }
    changed();
}
void Engine::setTempo (double bpm) { { const juce::ScopedLock sl (lock); if (std::abs (bpm - tempo) < .01) return; tempo = bpm; } changed(); }
bool Engine::hasVoice() const { const juce::ScopedLock sl (lock); return voice.mark.size() >= 4; }
double Engine::voicePitch() const { const juce::ScopedLock sl (lock); return voice.f0; }
juce::String Engine::voiceName() const { const juce::ScopedLock sl (lock); return name; }

void Engine::peaks (std::vector<float>& out, int n) const
{
    const juce::ScopedLock sl (lock);
    out.assign ((size_t) juce::jmax (0, n), 0.0f);
    const int N = (int) voice.x.size(); if (N < 1 || n < 1) return;
    for (int i = 0; i < n; ++i) { const int a = (int) ((juce::int64) i * N / n), z = juce::jmax (a + 1, (int) ((juce::int64) (i + 1) * N / n)); float m = 0; for (int k = a; k < z && k < N; ++k) m = juce::jmax (m, std::abs (voice.x[(size_t) k])); out[(size_t) i] = m; }
}

juce::File Engine::exportWav()
{
    for (int i = 0; i < 400 && busy(); ++i) juce::Thread::sleep (20);   // süren işlemeyi bekle (en çok 8 sn)
    const juce::ScopedLock sl (lock);
    if (last == nullptr) return {};
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("HitGen Choir " + juce::String (last->bars) + "bar " + juce::String (juce::roundToInt (last->bpm)) + "bpm.wav");
    f.deleteFile();
    auto os = f.createOutputStream(); if (os == nullptr) return {};
    std::unique_ptr<juce::AudioFormatWriter> wr { juce::WavAudioFormat().createWriterFor (os.get(), last->sr, 2, 24, {}, 0) };
    if (wr == nullptr) return {};
    os.release(); wr->writeFromAudioSampleBuffer (last->buf, 0, last->buf.getNumSamples()); wr.reset();
    return f;
}

juce::ValueTree Engine::toTree() const
{
    const juce::ScopedLock sl (lock);
    auto t = s.toTree(); t.setProperty ("file", path.getFullPathName(), nullptr); t.setProperty ("play", playing.load(), nullptr);
    return t;
}

void Engine::fromTree (const juce::ValueTree& t)
{
    if (! t.hasType ("choir")) return;
    const juce::File f (t.getProperty ("file").toString());
    if (f.existsAsFile()) loadFile (f);
    { const juce::ScopedLock sl (lock); s.fromTree (t); }
    playing.store ((bool) t.getProperty ("play", false));
    changed();
}
}
