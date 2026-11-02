// Track DNA: seçilen bölgeden ölçülen özellikler (yalnızca yerel DSP) ve bunların HitGen synth parametrelerine eşlenmesi.
#include "RefAnalysis.h"
#include "SoundParams.h"
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>

namespace hg
{
namespace
{
inline bool cancelled (const std::atomic<bool>* c) { return c != nullptr && c->load (std::memory_order_relaxed); }
inline float clamp01 (float x) { return juce::jlimit (0.0f, 1.0f, x); }
constexpr float kPi = 3.14159265f;

float medianV (std::vector<float> v) { if (v.empty()) return 0; std::nth_element (v.begin(), v.begin() + (long) v.size() / 2, v.end()); return v[v.size() / 2]; }
float meanV (const std::vector<float>& v) { if (v.empty()) return 0; double s = 0; for (float x : v) s += x; return (float) (s / v.size()); }
float stdV (const std::vector<float>& v) { if (v.size() < 2) return 0; const float m = meanV (v); double s = 0; for (float x : v) s += (x - m) * (x - m); return (float) std::sqrt (s / v.size()); }

// YIN: tek kare için f0 (0 = belirsiz)
float yin (const float* x, int n, double sr, float& conf)
{
    const int tauMin = std::max (2, (int) (sr / 2000.0)), tauMax = std::min (n / 2 - 1, (int) (sr / 40.0));
    if (tauMax <= tauMin + 2) { conf = 0; return 0; }
    std::vector<float> d ((size_t) tauMax + 1, 0.0f), cm ((size_t) tauMax + 1, 1.0f);
    const int w = n - tauMax;
    for (int tau = 1; tau <= tauMax; ++tau) { double s = 0; for (int i = 0; i < w; ++i) { const float a = x[i] - x[i + tau]; s += (double) a * a; } d[(size_t) tau] = (float) s; }
    double run = 0; for (int tau = 1; tau <= tauMax; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? (float) (d[(size_t) tau] * tau / run) : 1.0f; }
    int tau = -1; for (int t = tauMin; t < tauMax; ++t) if (cm[(size_t) t] < 0.15f) { while (t + 1 < tauMax && cm[(size_t) t + 1] < cm[(size_t) t]) ++t; tau = t; break; }
    if (tau < 0) { conf = 0; return 0; }
    float t = (float) tau; if (tau > 1 && tau < tauMax) { const float a = cm[(size_t) tau - 1], b = cm[(size_t) tau], c = cm[(size_t) tau + 1]; const float den = a - 2 * b + c; if (std::abs (den) > 1e-9f) t += 0.5f * (a - c) / den; }
    conf = clamp01 (1.0f - cm[(size_t) tau]);
    return (float) (sr / t);
}
}

juce::var TrackDna::toVar() const
{
    auto* o = new juce::DynamicObject();
    auto arr = [] (const std::vector<float>& v) { juce::Array<juce::var> a; for (float x : v) a.add ((double) x); return juce::var (a); };
    o->setProperty ("format", "wavejackers-track-dna"); o->setProperty ("version", version);
    o->setProperty ("name", name); o->setProperty ("artist", artist); o->setProperty ("sourceFile", sourceFile); o->setProperty ("role", role);
    o->setProperty ("regionStart", regionStart); o->setProperty ("regionEnd", regionEnd); o->setProperty ("sourceSr", sourceSr); o->setProperty ("created", created);
    o->setProperty ("envDb", arr (envDb)); o->setProperty ("harmonics", arr (harmonics));
#define P(x) o->setProperty (#x, (double) x);
    P(f0Hz) P(pitchConf) P(centroidHz) P(rolloffHz) P(brightness) P(warmth) P(lowFrac) P(subFrac) P(oddEven) P(harmonicSlope)
    P(attackMs) P(decayMs) P(sustainLevel) P(releaseMs) P(crest) P(noise) P(width) P(ampModHz) P(ampModDepth) P(brightModDepth)
    P(onsetsPerSec) P(ioiRegularity) P(pitchRangeSemis) P(pitchWobble) P(brightTrend) P(energyTrend)
#undef P
    o->setProperty ("pitched", pitched);
    return juce::var (o);
}

bool TrackDna::fromVar (const juce::var& v, TrackDna& d)
{
    if (! v.isObject() || v["format"].toString() != "wavejackers-track-dna") return false;
    const int ver = (int) v["version"]; if (ver < 1 || ver > kRefVersion) return false;
    d = TrackDna(); d.version = ver;
    d.name = v["name"].toString(); d.artist = v["artist"].toString(); d.sourceFile = v["sourceFile"].toString(); d.role = v["role"].toString();
    d.regionStart = (double) v["regionStart"]; d.regionEnd = (double) v["regionEnd"]; d.sourceSr = (double) v["sourceSr"]; d.created = v["created"].toString();
    auto rd = [] (const juce::var& a, std::vector<float>& out) { if (auto* A = a.getArray()) for (auto& x : *A) out.push_back ((float) (double) x); };
    rd (v["envDb"], d.envDb); rd (v["harmonics"], d.harmonics);
    if (d.envDb.size() != 16) return false;
#define G(x) d.x = (float) (double) v[#x]; if (! std::isfinite (d.x)) return false;
    G(f0Hz) G(pitchConf) G(centroidHz) G(rolloffHz) G(brightness) G(warmth) G(lowFrac) G(subFrac) G(oddEven) G(harmonicSlope)
    G(attackMs) G(decayMs) G(sustainLevel) G(releaseMs) G(crest) G(noise) G(width) G(ampModHz) G(ampModDepth) G(brightModDepth)
    G(onsetsPerSec) G(ioiRegularity) G(pitchRangeSemis) G(pitchWobble) G(brightTrend) G(energyTrend)
#undef G
    d.pitched = (bool) v["pitched"];
    return true;
}

bool extractDna (const RefClip& clip, double t0, double t1, int role, TrackDna& d, const std::atomic<bool>* cancel)
{
    d = TrackDna(); const double sr = clip.sampleRate; const int total = clip.numSamples();
    t0 = std::max (0.0, t0); t1 = std::min (t1, clip.seconds());
    if (t1 - t0 > 20.0) t1 = t0 + 20.0;
    if (t1 - t0 < 0.15) return false;
    const int i0 = (int) (t0 * sr), n = std::min (total, (int) (t1 * sr)) - i0; if (n < 4096) return false;
    std::vector<float> x ((size_t) n);
    const float* L = clip.audio.getReadPointer (0) + i0; const float* R = clip.audio.getNumChannels() > 1 ? clip.audio.getReadPointer (1) + i0 : L;
    double me = 0, se = 0;
    for (int i = 0; i < n; ++i) { x[(size_t) i] = 0.5f * (L[i] + R[i]); const double a = 0.5 * (L[i] + R[i]), b = 0.5 * (L[i] - R[i]); me += a * a; se += b * b; }
    d.width = me + se > 1e-12 ? clamp01 ((float) (2 * se / (me + se))) : 0;
    double e = 0; for (float v : x) e += (double) v * v; if (e / n < 1e-10) return false;     // sessiz bölge
    d.role = refCatKey (role); d.regionStart = t0; d.regionEnd = t1; d.sourceSr = sr; d.version = kRefVersion;
    d.created = juce::Time::getCurrentTime().toISO8601 (true); d.sourceFile = clip.name;

    // ---- genlik zarfı (5 ms adım)
    const int eh = std::max (1, (int) (sr * 0.005)), ew = eh * 2; const int ne = (n - ew) / eh;
    if (ne < 8) return false;
    std::vector<float> env ((size_t) ne);
    for (int k = 0; k < ne; ++k) { double s = 0; const float* p = x.data() + (size_t) k * eh; for (int i = 0; i < ew; ++i) s += (double) p[i] * p[i]; env[(size_t) k] = (float) std::sqrt (s / ew); }
    float peakE = *std::max_element (env.begin(), env.end()); if (peakE <= 1e-7f) return false;
    { float pk = 0; for (float v : x) pk = std::max (pk, std::abs (v)); d.crest = 20.0f * std::log10 (pk / (float) std::sqrt (e / n) + 1e-9f); }
    std::vector<float> envS (env.size()); { // 20 ms hareketli ortalama
        const int w = 4; for (int k = 0; k < ne; ++k) { double s = 0; int c = 0; for (int j = std::max (0, k - w); j <= std::min (ne - 1, k + w); ++j) { s += env[(size_t) j]; ++c; } envS[(size_t) k] = (float) (s / c); } }
    if (cancelled (cancel)) return false;

    // ---- onset'ler (zarf yükselişi)
    std::vector<int> ons; { const int lag = 4; float prevPk = 0; int lastOn = -1000;
        for (int k = lag; k < ne - 2; ++k) { const float rise = envS[(size_t) k] - envS[(size_t) k - lag]; const float th = 0.25f * peakE;
            if (rise > th && envS[(size_t) k] >= envS[(size_t) k - 1] && k - lastOn > (int) (0.08 / 0.005)) { ons.push_back (k - lag); lastOn = k; prevPk = envS[(size_t) k]; } } (void) prevPk; }
    d.onsetsPerSec = (float) ons.size() / (float) (t1 - t0);
    if (ons.size() >= 3) { std::vector<float> ioi; for (size_t i = 1; i < ons.size(); ++i) ioi.push_back ((ons[i] - ons[i - 1]) * 0.005f); const float m = meanV (ioi); d.ioiRegularity = m > 0 ? clamp01 (1.0f - stdV (ioi) / m) : 0; }

    // ---- zarf şekli: atak, düşüş, sürdürme
    std::vector<float> atk, dec, sus;
    auto analyseNote = [&] (int s, int endK)
    {
        int pk = s; for (int k = s; k < endK; ++k) if (envS[(size_t) k] > envS[(size_t) pk]) pk = k;
        const float pv = envS[(size_t) pk]; if (pv < 0.05f * peakE) return;
        int a10 = s, a90 = pk; for (int k = s; k <= pk; ++k) { if (envS[(size_t) k] < 0.1f * pv) a10 = k; if (envS[(size_t) k] < 0.9f * pv) a90 = k; }
        atk.push_back ((a90 - a10 + 1) * 5.0f);
        int dk = endK - 1; for (int k = pk; k < endK; ++k) if (envS[(size_t) k] < 0.1f * pv) { dk = k; break; }
        dec.push_back ((dk - pk) * 5.0f);
        const int sk = pk + (int) ((endK - pk) * 0.6f); sus.push_back (envS[(size_t) std::min (ne - 1, sk)] / pv);
    };
    if (ons.empty()) analyseNote (0, std::min (ne, (int) (3.0 / 0.005)));
    else for (size_t i = 0; i < ons.size(); ++i) analyseNote (ons[i], i + 1 < ons.size() ? ons[i + 1] : std::min (ne, ons[i] + (int) (1.5 / 0.005)));
    d.attackMs = medianV (atk); d.decayMs = medianV (dec); d.sustainLevel = clamp01 (medianV (sus));
    if (ons.empty() && d.sustainLevel > 0.5f) d.decayMs = 0;
    { // son kalan kuyruk: son tepeden -6 dB'den -30 dB'ye süre
        int pk = 0; for (int k = 0; k < ne; ++k) if (envS[(size_t) k] > envS[(size_t) pk]) pk = k;
        int a = -1, b = -1; for (int k = ne - 1; k > pk; --k) { if (a < 0 && envS[(size_t) k] > 0.5f * peakE) a = k; if (b < 0 && envS[(size_t) k] > 0.03f * peakE) b = k; }
        d.releaseMs = (a > 0 && b > a) ? (b - a) * 5.0f : 0.0f;
    }

    // ---- ortalama güç spektrumu (4096'lık Hann, yarım adım)
    const int fo = 12, fn = 1 << fo; juce::dsp::FFT fft (fo);
    std::vector<float> win ((size_t) fn), buf (2 * (size_t) fn), pw ((size_t) fn / 2 + 1, 0.0f); for (int i = 0; i < fn; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2 * kPi * i / fn);
    std::vector<float> frC, frE; int nf = 0;
    for (int s = 0; s + fn <= n; s += fn / 2)
    {
        if (cancelled (cancel)) return false;
        std::fill (buf.begin(), buf.end(), 0.0f); for (int i = 0; i < fn; ++i) buf[(size_t) i] = x[(size_t) s + (size_t) i] * win[(size_t) i];
        fft.performFrequencyOnlyForwardTransform (buf.data());
        double num = 0, den = 0; for (int k = 1; k <= fn / 2; ++k) { const float p = buf[(size_t) k] * buf[(size_t) k]; pw[(size_t) k] += p; num += (double) k * p; den += p; }
        frC.push_back (den > 1e-12 ? (float) (num / den * sr / fn) : 0.0f); frE.push_back ((float) den); ++nf;
    }
    if (nf == 0) return false;
    const float bw = (float) (sr / fn); double tot = 0; for (float p : pw) tot += p; if (tot <= 1e-12) return false;
    { double num = 0; for (size_t k = 1; k < pw.size(); ++k) num += (double) k * bw * pw[k]; d.centroidHz = (float) (num / tot); }
    { double acc = 0; d.rolloffHz = 0; for (size_t k = 1; k < pw.size(); ++k) { acc += pw[k]; if (acc >= 0.85 * tot) { d.rolloffHz = (float) k * bw; break; } } }
    auto bandPow = [&] (float a, float b) { double s = 0; for (size_t k = 1; k < pw.size(); ++k) { const float f = k * bw; if (f >= a && f < b) s += pw[k]; } return (float) (s / tot); };
    d.subFrac = bandPow (0, 100); d.lowFrac = bandPow (0, 250); d.warmth = bandPow (150, 500);
    d.brightness = clamp01 (std::log2 (std::max (d.centroidHz, 50.0f) / 200.0f) / std::log2 (8000.0f / 200.0f));
    d.envDb.assign (16, -90.0f); { const float lo = 40.0f, hi = std::min (16000.0f, (float) (0.45 * sr)); float mx = -200;
        for (int b = 0; b < 16; ++b) { const float a = lo * std::pow (hi / lo, b / 16.0f), c = lo * std::pow (hi / lo, (b + 1) / 16.0f); double s = 0; int cnt = 0;
            for (size_t k = 1; k < pw.size(); ++k) { const float f = k * bw; if (f >= a && f < c) { s += pw[k]; ++cnt; } }
            d.envDb[(size_t) b] = cnt ? 10.0f * std::log10 ((float) (s / cnt) + 1e-12f) : -120.0f; mx = std::max (mx, d.envDb[(size_t) b]); }
        for (auto& v : d.envDb) v = std::max (-90.0f, v - mx); }
    { // parlaklık ve enerji eğilimi, parlaklık modülasyonu
        const int th = std::max (1, nf / 3); float c0 = 0, c1 = 0, e0 = 0, e1 = 0; for (int i = 0; i < th; ++i) { c0 += frC[(size_t) i]; e0 += frE[(size_t) i]; c1 += frC[(size_t) (nf - 1 - i)]; e1 += frE[(size_t) (nf - 1 - i)]; }
        c0 /= th; c1 /= th; d.brightTrend = clamp01 (0.5f + 0.5f * (c1 - c0) / std::max (50.0f, c0 + c1)) * 2.0f - 1.0f;
        d.energyTrend = clamp01 (0.5f + 0.5f * (e1 - e0) / std::max (1e-9f, e0 + e1)) * 2.0f - 1.0f;
        std::vector<float> cs; for (int i = 0; i < nf; ++i) if (frE[(size_t) i] > 0.05f * *std::max_element (frE.begin(), frE.end())) cs.push_back (frC[(size_t) i]);
        d.brightModDepth = cs.size() > 2 ? clamp01 (stdV (cs) / std::max (50.0f, meanV (cs)) * 2.0f) : 0.0f; }

    // ---- perde: YIN, sesli karelerin ortancası
    std::vector<float> f0s, semis; int voiced = 0, tried = 0;
    { const int yn = 2048; const int stepY = std::max (yn / 2, (n - yn) / 48);
      for (int s = 0; s + yn <= n && tried < 60; s += stepY)
      { if (cancelled (cancel)) return false; double ee = 0; for (int i = 0; i < yn; ++i) ee += (double) x[(size_t) s + (size_t) i] * x[(size_t) s + (size_t) i]; if (std::sqrt (ee / yn) < 0.1f * peakE) continue;
        ++tried; float c = 0; const float f = yin (x.data() + s, yn, sr, c); if (f > 0 && c > 0.6f) { f0s.push_back (f); semis.push_back (12.0f * std::log2 (f / 440.0f)); ++voiced; } } }
    d.pitchConf = tried ? (float) voiced / tried : 0.0f; d.pitched = d.pitchConf >= 0.4f && ! f0s.empty();
    if (d.pitched)
    {
        d.f0Hz = medianV (f0s); std::vector<float> sm = semis; std::sort (sm.begin(), sm.end());
        d.pitchRangeSemis = sm.size() > 4 ? sm[(size_t) (sm.size() * 0.95)] - sm[(size_t) (sm.size() * 0.05)] : 0.0f;
        std::vector<float> dd; for (size_t i = 1; i < semis.size(); ++i) dd.push_back (std::abs (semis[i] - semis[i - 1])); d.pitchWobble = std::min (4.0f, meanV (dd));
        // harmonik dağılım
        std::vector<float> amp (8, 0.0f); float mx = 0; double harmE = 0;
        for (int h = 1; h <= 8; ++h) { const float f = d.f0Hz * h; if (f > 0.45f * sr) break; float best = 0; const int lo = std::max (1, (int) std::floor (f * 0.97f / bw)), hi = std::min ((int) pw.size() - 1, (int) std::ceil (f * 1.03f / bw));
            for (int k = lo; k <= hi; ++k) best = std::max (best, pw[(size_t) k]); amp[(size_t) h - 1] = std::sqrt (best); mx = std::max (mx, amp[(size_t) h - 1]); }
        if (mx > 0) { for (auto& a : amp) a /= mx; d.harmonics = amp; }
        float od = 0, ev = 0; int co = 0, ce = 0; for (int h = 1; h <= 8; ++h) { if (h & 1) { od += amp[(size_t) h - 1]; ++co; } else { ev += amp[(size_t) h - 1]; ++ce; } }
        d.oddEven = (ce && ev > 1e-6f) ? std::min (8.0f, (od / co) / (ev / ce)) : 4.0f;
        { double sx = 0, sy = 0, sxx = 0, sxy = 0; int c = 0; for (int h = 1; h <= 8; ++h) { const float a = amp[(size_t) h - 1]; if (a < 1e-4f) continue; const double xx = std::log2 ((double) h), yy = 20.0 * std::log10 ((double) a); sx += xx; sy += yy; sxx += xx * xx; sxy += xx * yy; ++c; }
          d.harmonicSlope = c > 2 && c * sxx - sx * sx > 1e-9 ? (float) ((c * sxy - sx * sy) / (c * sxx - sx * sx)) : -6.0f; }
        // harmonik olmayan enerji = gürültü
        for (int h = 1; h <= 20; ++h) { const float f = d.f0Hz * h; if (f > 0.45f * sr) break; const int lo = std::max (1, (int) std::floor (f * 0.97f / bw)), hi = std::min ((int) pw.size() - 1, (int) std::ceil (f * 1.03f / bw)); for (int k = lo; k <= hi; ++k) harmE += pw[(size_t) k]; }
        d.noise = clamp01 ((float) (1.0 - harmE / tot));
    }
    else
    {
        double lg = 0, am = 0; int c = 0; for (size_t k = 1; k < pw.size(); ++k) { const float f = k * bw; if (f > 100 && f < 10000) { lg += std::log (pw[k] + 1e-12); am += pw[k]; ++c; } }
        d.noise = c ? clamp01 ((float) (std::exp (lg / c) / (am / c + 1e-12)) * 6.0f) : 0.0f; d.oddEven = 1.0f; d.harmonicSlope = -3.0f; d.pitchRangeSemis = 0; d.pitchWobble = 0;
    }

    // ---- genlik modülasyonu: zarfın 0.2-12 Hz bandındaki en güçlü bileşeni
    { const int steps = ne; const double fs = 200.0; double mean = 0; for (float v : envS) mean += v; mean /= steps; float bestMag = 0, bestHz = 0; const int Kmax = std::min (steps / 2, 2400);
      for (float hz = 0.2f; hz <= 12.0f; hz += 0.1f) { double re = 0, im = 0; for (int k = 0; k < steps; ++k) { const double ph = 2 * kPi * hz * k / fs; const double v = envS[(size_t) k] - mean; re += v * std::cos (ph); im += v * std::sin (ph); }
        const float mag = (float) (std::sqrt (re * re + im * im) * 2.0 / steps); if (mag > bestMag) { bestMag = mag; bestHz = hz; } } (void) Kmax;
      d.ampModHz = bestHz; d.ampModDepth = mean > 1e-9 ? clamp01 ((float) (bestMag / mean)) : 0.0f; }
    return true;
}

// ---------------------------------------------------------------- DNA -> synth
void dnaToSoundParams (const TrackDna& dna, char role, float similarity, float variation, juce::uint32 seed, float* x)
{
    const float sim = juce::jlimit (0.0f, 100.0f, similarity) / 100.0f, var = juce::jlimit (0.0f, 100.0f, variation) / 100.0f;
    juce::Random baseRng ((juce::int64) seed * 2654435761LL + 17);
    float base[kNumSp]; spRandom (role, baseRng, base);
    float tgt[kNumSp]; std::copy (base, base + kNumSp, tgt);
    bool mapped[kNumSp] = {};
    auto setT = [&] (int i, float v) { tgt[i] = v; mapped[i] = true; };
    const bool bass = role == 'b', pad = role == 'c' && dna.role == "pad";
    // dalga biçimi: harmonik yapıdan
    int w = 1;
    if (dna.pitched)
    {
        const float h2 = dna.harmonics.size() > 1 ? dna.harmonics[1] : 0, h3 = dna.harmonics.size() > 2 ? dna.harmonics[2] : 0;
        float upper = 0; for (size_t i = 2; i < dna.harmonics.size(); ++i) upper = std::max (upper, dna.harmonics[i]);
        if (h2 < 0.12f && upper < 0.12f) w = 4;                                  // neredeyse sinüs
        else if (dna.oddEven > 1.8f) w = dna.harmonicSlope < -10.0f ? 3 : 2;     // tek harmonik baskın: üçgen / kare
        else w = 1; (void) h3;                                                   // tam harmonik dizi: testere
    }
    setT (o1w, (float) w); setT (o1oct, 0); setT (o1semi, 0); setT (o1fine, 0); setT (o1lvl, bass ? 0.6f : 0.55f);
    if (bass || dna.lowFrac > 0.6f) { setT (o2w, 4); setT (o2oct, -1); setT (o2semi, 0); setT (o2fine, 0); setT (o2lvl, 0.55f + 0.3f * dna.subFrac); }
    else if (dna.width > 0.25f) { setT (o2w, (float) (w == 4 ? 3 : w)); setT (o2oct, 0); setT (o2semi, 0); setT (o2fine, 6.0f + 20.0f * dna.width); setT (o2lvl, 0.4f); }
    else { setT (o2w, 0); setT (o2lvl, 0); }
    setT (o3w, 0); setT (o3lvl, 0);
    setT (uni, bass ? 1.0f : std::round (1.0f + 6.0f * clamp01 (dna.width * 1.6f))); setT (uniDet, 6.0f + 34.0f * dna.width);
    setT (sub, bass ? juce::jlimit (0.0f, 1.0f, 0.2f + 0.6f * dna.subFrac) : 0.12f * dna.subFrac);
    setT (noise, std::min (0.5f, dna.noise * 0.5f));
    // filtre: çıkış parlaklığından ve zarf hareketinden
    setT (ftype, 0); setT (fslope, dna.rolloffHz < 2000.0f ? 1.0f : 0.0f);
    float peaky = 0; for (int i = 1; i < 15; ++i) peaky = std::max (peaky, dna.envDb[(size_t) i] - 0.5f * (dna.envDb[(size_t) i - 1] + dna.envDb[(size_t) i + 1]));
    setT (cut, juce::jlimit (120.0f, 16000.0f, dna.rolloffHz * (dna.pitched ? 0.85f : 1.0f)));
    setT (reso, 0.7f + 7.0f * clamp01 (peaky / 14.0f));
    setT (fenv, juce::jlimit (0.0f, 5.0f, 0.4f + 4.0f * dna.brightModDepth + (dna.decayMs > 0 && dna.decayMs < 400 ? 1.0f : 0.0f)));
    setT (fdec, juce::jlimit (0.05f, 2.0f, std::max (0.1f, dna.decayMs / 1000.0f * 0.8f)));
    setT (fatk, dna.brightTrend > 0.25f ? 0.3f : 0.0f);
    setT (fdrive, juce::jlimit (0.0f, 0.8f, (9.0f - dna.crest) / 15.0f));
    // genlik zarfı
    const float relDef = bass ? 0.12f : pad ? 1.2f : 0.35f;
    setT (envA, juce::jlimit (0.002f, 3.0f, std::max (0.002f, dna.attackMs / 1000.0f)));
    setT (envD, juce::jlimit (0.03f, 4.0f, dna.decayMs > 0 ? dna.decayMs / 1000.0f : 0.5f));
    setT (envS, dna.sustainLevel); setT (envR, juce::jlimit (0.03f, 6.0f, dna.releaseMs > 0 ? dna.releaseMs / 1000.0f : relDef));
    // LFO: gerçekten ölçülen modülasyon varsa
    setT (l1depth, 0); setT (l2depth, 0);
    if (dna.ampModDepth > 0.25f && dna.onsetsPerSec < 0.6f && dna.ampModHz >= 0.2f) { setT (l1rate, dna.ampModHz); setT (l1depth, std::min (0.5f, dna.ampModDepth * 0.5f)); setT (l1target, 2); setT (l1shape, 0); setT (l1sync, 0); }
    else if (dna.brightModDepth > 0.15f) { setT (l1rate, std::max (0.1f, dna.ampModHz)); setT (l1depth, std::min (0.5f, dna.brightModDepth * 1.5f)); setT (l1target, 0); setT (l1shape, 0); setT (l1sync, 0); }
    if (dna.pitched && dna.pitchWobble > 0.08f && dna.pitchWobble < 1.5f) { setT (l2rate, 5.5f); setT (l2depth, juce::jlimit (0.03f, 0.25f, dna.pitchWobble * 0.2f)); setT (l2target, 1); setT (l2shape, 0); setT (l2sync, 0); }
    // efektler
    setT (fxChorus, bass ? 0.0f : 0.1f + 0.6f * dna.width); setT (fxReverb, bass ? 0.05f : juce::jlimit (0.05f, 0.7f, dna.releaseMs / 3000.0f + 0.2f * dna.width + 0.1f));
    setT (fxDrive, juce::jlimit (0.0f, 0.5f, (9.0f - dna.crest) / 18.0f));

    // benzerlik: sürekli parametrelerde doğrusal karışım, ayrık olanlarda eşik (0.5)
    float out[kNumSp];
    for (int i = 0; i < kNumSp; ++i)
    {
        if (! mapped[i]) { out[i] = base[i]; continue; }
        const auto& dfn = spDef (i);
        if (dfn.kind == spFloat) { if (dfn.centre > 0 && dfn.lo > 0 && base[i] > 0 && tgt[i] > 0) out[i] = std::exp (std::log (base[i]) * (1 - sim) + std::log (tgt[i]) * sim); else out[i] = base[i] + (tgt[i] - base[i]) * sim; }
        else out[i] = sim >= 0.5f ? tgt[i] : base[i];
    }
    // sınırlı varyasyon: her parametre grubu için belgelenmiş en büyük sapma (var = 1'de)
    juce::Random rng ((juce::int64) seed * 40503LL + 911);
    auto g = [&] { return (rng.nextFloat() * 2.0f - 1.0f); };
    auto logJit = [&] (int i, float octaves) { if (mapped[i]) out[i] *= std::pow (2.0f, g() * octaves * var); };
    auto linJit = [&] (int i, float amt) { if (mapped[i]) out[i] += g() * amt * var * (spDef (i).hi - spDef (i).lo); };
    logJit (cut, 1.0f); logJit (reso, 0.8f); logJit (fdec, 0.8f); logJit (envA, 1.0f); logJit (envD, 0.7f); logJit (envR, 0.7f); logJit (uniDet, 0.6f); logJit (l1rate, 0.5f);
    linJit (envS, 0.25f); linJit (fenv, 0.2f); linJit (fxChorus, 0.3f); linJit (fxReverb, 0.3f); linJit (fxDrive, 0.15f); linJit (sub, 0.2f); linJit (noise, 0.3f); linJit (o2lvl, 0.2f);
    linJit (o2fine, 0.15f); linJit (l1depth, 0.2f); linJit (fxDelay, 0.4f);
    if (rng.nextFloat() < var * 0.5f && out[uni] > 1) out[uni] += rng.nextBool() ? 1.0f : -1.0f;
    if (rng.nextFloat() < var * 0.35f)
    {
        const int cur = (int) std::round (out[o1w]);
        if (cur == 1 || cur == 2) out[o1w] = rng.nextBool() ? 1.0f : 2.0f; else if (cur == 4 || cur == 3) out[o1w] = rng.nextBool() ? 4.0f : 3.0f;
    }
    if (var > 0.15f) { out[wtA] = (float) rng.nextInt (12); out[wtB] = (float) rng.nextInt (12); }
    for (int i = 0; i < kNumSp; ++i)
    {
        const auto& dfn = spDef (i); float v = juce::jlimit (dfn.lo, dfn.hi, out[i]);
        if (dfn.kind != spFloat) v = std::round (v);
        x[i] = std::isfinite (v) ? v : dfn.def;
    }
}
}
