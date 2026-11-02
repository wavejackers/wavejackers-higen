#include "RefAnalysis.h"
#include "SoundParams.h"
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace hg
{
namespace
{
constexpr int kFftOrder = 11, kFft = 1 << kFftOrder, kHop = 512;
constexpr double kAnaSr = 22050.0;
constexpr int kBins = 557;    // ~6 kHz'e kadar (22050/2048 = 10.77 Hz/bin)
constexpr float kPi = 3.14159265f;

inline bool cancelled (const std::atomic<bool>* c) { return c != nullptr && c->load (std::memory_order_relaxed); }
inline float ss (float a, float b, float x) { const float t = juce::jlimit (0.0f, 1.0f, (x - a) / (b - a)); return t * t * (3 - 2 * t); }
inline float clamp01 (float x) { return juce::jlimit (0.0f, 1.0f, x); }
inline float toDb (float p) { return 10.0f * std::log10 (std::max (p, 1e-12f)); }

struct Mono { std::vector<float> x; double sr = kAnaSr; };

// özgün ses -> mono, 22050 Hz (kutu süzgeç ile)
Mono toMono22k (const RefClip& c)
{
    Mono m; const int n = c.numSamples(); if (n <= 0) return m;
    std::vector<double> pre ((size_t) n + 1, 0.0);
    const float* L = c.audio.getReadPointer (0); const float* R = c.audio.getNumChannels() > 1 ? c.audio.getReadPointer (1) : L;
    for (int i = 0; i < n; ++i) pre[(size_t) i + 1] = pre[(size_t) i] + 0.5 * ((double) L[i] + (double) R[i]);
    const double r = c.sampleRate / kAnaSr; const int outN = (int) std::floor (n / r);
    m.x.resize ((size_t) std::max (0, outN));
    for (int i = 0; i < outN; ++i)
    {
        const double a = i * r, b = (i + 1) * r; const int ia = (int) a, ib = std::min (n, (int) std::ceil (b));
        const int cnt = std::max (1, ib - ia);
        m.x[(size_t) i] = (float) ((pre[(size_t) std::min (n, ib)] - pre[(size_t) ia]) / cnt);
    }
    return m;
}

float medianOf (float* v, int n) { std::nth_element (v, v + n / 2, v + n); return v[n / 2]; }

// Krumhansl-Kessler profilleri
const float kMaj[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
const float kMin[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

float corr12 (const float* a, const float* b)
{
    float ma = 0, mb = 0; for (int i = 0; i < 12; ++i) { ma += a[i]; mb += b[i]; } ma /= 12; mb /= 12;
    float n = 0, da = 0, db = 0;
    for (int i = 0; i < 12; ++i) { const float x = a[i] - ma, y = b[i] - mb; n += x * y; da += x * x; db += y * y; }
    return n / (std::sqrt (da * db) + 1e-9f);
}
}

const char* refCatKey (int c) { static const char* k[] = { "bass", "lead", "pad", "perc" }; return k[juce::jlimit (0, 3, c)]; }

std::vector<RefRegion> RefAnalysis::top (int cat, int maxN) const
{
    std::vector<RefRegion> r; for (auto& g : regions) if (g.cat == cat) r.push_back (g);
    std::sort (r.begin(), r.end(), [] (const RefRegion& a, const RefRegion& b) { return a.confidence > b.confidence; });
    if ((int) r.size() > maxN) r.resize ((size_t) maxN);
    return r;
}

// ---------------------------------------------------------------- yükleme
std::shared_ptr<RefClip> loadRefAudio (const juce::File& f, juce::String& error, const std::atomic<bool>* cancel)
{
    error.clear();
    if (! f.existsAsFile()) { error = "fileMissing"; return nullptr; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
    if (rd == nullptr) { error = "fileBad"; return nullptr; }
    if (rd->lengthInSamples <= 0 || rd->numChannels < 1 || rd->sampleRate < 8000.0 || rd->sampleRate > 384000.0) { error = "fileBad"; return nullptr; }
    auto c = std::make_shared<RefClip>();
    c->sampleRate = rd->sampleRate; c->name = f.getFileNameWithoutExtension(); c->path = f.getFullPathName();
    juce::int64 total = rd->lengthInSamples; const juce::int64 cap = (juce::int64) (kRefMaxSeconds * rd->sampleRate);
    if (total > cap) { total = cap; c->truncated = true; }
    if (rd->getFormatName().containsIgnoreCase ("WAV") || rd->getFormatName().containsIgnoreCase ("AIFF"))
    {   // başlık uzunluğu dosya boyutundan büyükse (kesik dosya) yalnızca gerçekten bulunan örnekleri yükle
        const juce::int64 bytesPer = std::max<juce::int64> (1, (juce::int64) rd->numChannels * (juce::int64) ((rd->bitsPerSample + 7) / 8));
        const juce::int64 avail = std::max<juce::int64> (0, (f.getSize() - 64) / bytesPer);
        if (avail < total) { total = avail; c->truncated = true; }
        if (total < 1024) { error = "fileBad"; return nullptr; }
    }
    const int outCh = (int) std::min<unsigned> (2, rd->numChannels);
    c->audio.setSize (outCh, (int) total);
    const int chunk = 1 << 16; juce::AudioBuffer<float> tmp ((int) rd->numChannels, chunk);
    for (juce::int64 pos = 0; pos < total; pos += chunk)
    {
        if (cancelled (cancel)) { error = "cancelled"; return nullptr; }
        const int n = (int) std::min<juce::int64> (chunk, total - pos);
        tmp.clear();
        if (! rd->read (&tmp, 0, n, pos, true, true)) { error = "fileBad"; return nullptr; }
        for (int ch = 0; ch < outCh; ++ch) c->audio.copyFrom (ch, (int) pos, tmp, ch, 0, n);
    }
    // bozuk dosyada NaN/Inf ya da tamamen sessizlik
    double energy = 0; bool bad = false;
    for (int ch = 0; ch < outCh && ! bad; ++ch)
    {
        const float* p = c->audio.getReadPointer (ch);
        for (int i = 0; i < (int) total; ++i) { if (! std::isfinite (p[i])) { bad = true; break; } energy += (double) p[i] * p[i]; }
    }
    if (bad) { error = "fileBad"; return nullptr; }
    if (energy <= 1e-12) { error = "fileSilent"; return nullptr; }
    return c;
}

// ---------------------------------------------------------------- analiz
bool analyzeRef (const RefClip& clip, RefAnalysis& out, const std::atomic<bool>* cancel, const std::function<void (float)>& progress)
{
    out = RefAnalysis(); out.durationSec = clip.seconds(); out.analysisSr = kAnaSr;
    auto prog = [&] (float p) { if (progress) progress (p); };
    Mono m = toMono22k (clip);
    const int N = (int) m.x.size();
    if (N < kFft * 3) { out.notes = "tooShort"; return true; }
    const int T = (N - kFft) / kHop + 1;
    prog (0.02f);

    // STFT büyüklükleri
    juce::dsp::FFT fft (kFftOrder);
    std::vector<float> win (kFft); for (int i = 0; i < kFft; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2 * kPi * i / kFft);
    std::vector<float> M ((size_t) T * kBins), buf (2 * kFft);
    std::vector<float> frameRmsDb ((size_t) T);
    for (int t = 0; t < T; ++t)
    {
        if ((t & 255) == 0 && cancelled (cancel)) return false;
        std::fill (buf.begin(), buf.end(), 0.0f);
        double e = 0; const float* s = m.x.data() + (size_t) t * kHop;
        for (int i = 0; i < kFft; ++i) { buf[(size_t) i] = s[i] * win[(size_t) i]; e += (double) s[i] * s[i]; }
        fft.performFrequencyOnlyForwardTransform (buf.data());
        std::copy_n (buf.begin(), kBins, M.begin() + (size_t) t * kBins);
        frameRmsDb[(size_t) t] = toDb ((float) (e / kFft));
    }
    prog (0.15f);

    // HPSS: zaman ortancası (harmonik), frekans ortancası (vurmalı)
    const int hw = 17, half = hw / 2;
    std::vector<float> Hm ((size_t) T * kBins), tmp (hw), col ((size_t) T);
    {
        std::vector<float> Hmed ((size_t) T * kBins);
        for (int k = 0; k < kBins; ++k)
        {
            if ((k & 15) == 0 && cancelled (cancel)) return false;
            for (int t = 0; t < T; ++t) col[(size_t) t] = M[(size_t) t * kBins + (size_t) k];
            for (int t = 0; t < T; ++t)
            {
                int n = 0; for (int d = -half; d <= half; ++d) { const int u = t + d; if (u >= 0 && u < T) tmp[(size_t) n++] = col[(size_t) u]; }
                Hmed[(size_t) t * kBins + (size_t) k] = medianOf (tmp.data(), n);
            }
        }
        prog (0.40f);
        for (int t = 0; t < T; ++t)
        {
            if ((t & 127) == 0 && cancelled (cancel)) return false;
            const float* row = M.data() + (size_t) t * kBins;
            for (int k = 0; k < kBins; ++k)
            {
                int n = 0; for (int d = -half; d <= half; ++d) { const int u = k + d; if (u >= 0 && u < kBins) tmp[(size_t) n++] = row[u]; }
                const float p = medianOf (tmp.data(), n), h = Hmed[(size_t) t * kBins + (size_t) k];
                Hm[(size_t) t * kBins + (size_t) k] = h * h / (h * h + p * p + 1e-12f);
            }
        }
    }
    prog (0.55f);

    // kare başına özellikler
    struct FF { float sub, low, mid, high, harmE, totE, flux, centroid, flat; };
    std::vector<FF> F ((size_t) T);
    const float binHz = (float) (kAnaSr / kFft);
    std::vector<float> prevC (kBins, 0.0f), env ((size_t) T, 0.0f);
    for (int t = 0; t < T; ++t)
    {
        const float* r = M.data() + (size_t) t * kBins; const float* hm = Hm.data() + (size_t) t * kBins; FF f {};
        double num = 0, den = 0, lg = 0; int lgN = 0;
        for (int k = 2; k < kBins; ++k)
        {
            const float p = r[k] * r[k], fr = k * binHz; f.totE += p; f.harmE += p * hm[k];
            if (fr < 150) f.sub += p; else if (fr < 300) f.low += p; else if (fr < 2000) f.mid += p; else f.high += p;
            num += (double) fr * p; den += p;
            if (fr > 150 && fr < 5500) { lg += std::log (p + 1e-9f); ++lgN; }
            const float c = std::log1p (10.0f * r[k] * (1.0f - hm[k] * 0.5f)); const float d = c - prevC[(size_t) k]; if (d > 0) f.flux += d; prevC[(size_t) k] = c;
        }
        f.centroid = den > 1e-12 ? (float) (num / den) : 0.0f;
        double am = 0; int cnt = 0; for (int k = 14; k < 510; ++k) { am += r[k] * r[k]; ++cnt; }
        f.flat = lgN > 0 && am > 1e-12 ? (float) std::exp (lg / lgN) / (float) (am / cnt + 1e-12) : 0.0f;
        f.flat = clamp01 (f.flat);
        F[(size_t) t] = f; env[(size_t) t] = f.flux;
    }

    // onset: uyarlanır eşik + yerel tepe
    const float fr = (float) (kAnaSr / kHop);  // kare/sn (~43.07)
    std::vector<float> ma ((size_t) T, 0.0f);
    {
        const int w = 21; double acc = 0; std::vector<double> pre ((size_t) T + 1, 0.0);
        for (int t = 0; t < T; ++t) pre[(size_t) t + 1] = pre[(size_t) t] + env[(size_t) t];
        for (int t = 0; t < T; ++t) { const int a = std::max (0, t - w / 2), b = std::min (T, t + w / 2 + 1); acc = pre[(size_t) b] - pre[(size_t) a]; ma[(size_t) t] = (float) (acc / (b - a)); }
    }
    float gm = 0; for (float v : env) gm += v; gm /= (float) T;
    int last = -100;
    for (int t = 2; t < T - 2; ++t)
    {
        const float v = env[(size_t) t];
        if (v > ma[(size_t) t] * 1.45f + 0.15f * gm && v >= env[(size_t) t - 1] && v >= env[(size_t) t + 1] && v >= env[(size_t) t - 2] && v >= env[(size_t) t + 2] && t - last >= 4)
        { out.onsets.push_back ((t * kHop + kFft / 2) / (float) kAnaSr); last = t; if (out.onsets.size() > 30000) break; }
    }
    prog (0.62f);

    // tempo: onset zarfı otokorelasyonu + armonik tarak, 126 BPM çevresinde geniş bir önsel
    {
        std::vector<float> e ((size_t) T);
        for (int t = 0; t < T; ++t) e[(size_t) t] = std::max (0.0f, env[(size_t) t] - ma[(size_t) t]);
        const int maxLag = (int) (fr * 60.0 / 55.0 * 4.5) + 4; std::vector<double> ac ((size_t) maxLag + 2, 0.0);
        for (int l = 0; l <= maxLag; ++l) { double s = 0; for (int t = 0; t + l < T; ++t) s += (double) e[(size_t) t] * e[(size_t) t + l]; ac[(size_t) l] = s / std::max (1, T - l); }
        auto acAt = [&] (double lag) { if (lag < 0 || lag >= maxLag) return 0.0; const int i = (int) lag; const double f = lag - i; return ac[(size_t) i] * (1 - f) + ac[(size_t) i + 1] * f; };
        double bestS = -1, sum = 0; int cntS = 0; double bestB = 0; std::vector<double> sc;
        for (double b = 70.0; b <= 180.0; b += 0.25)
        {
            const double lag = fr * 60.0 / b; double s = 0, w = 1.0;
            for (int mlt = 1; mlt <= 4; ++mlt, w *= 0.7) s += w * acAt (lag * mlt);
            const double o = std::log2 (b / 126.0); const double prior = std::exp (-o * o / (2 * 0.6 * 0.6));
            s = s / (ac[0] + 1e-12) * (0.6 + 0.4 * prior);
            sc.push_back (s); sum += s; ++cntS; if (s > bestS) { bestS = s; bestB = b; }
        }
        const double mean = sum / std::max (1, cntS);
        if (ac[0] > 1e-9 && bestS > 0) { out.bpm = std::round (bestB * 10.0) / 10.0; out.bpmConf = clamp01 ((float) ((bestS / (mean + 1e-12) - 1.0) / 2.5)); }
    }
    prog (0.70f);

    // ton: harmonik maskeli kroma, 130-2000 Hz, KK profilleri ile
    {
        float chroma[12] = {}; float maxDb = -200; for (float d : frameRmsDb) maxDb = std::max (maxDb, d);
        for (int t = 0; t < T; ++t)
        {
            if (frameRmsDb[(size_t) t] < maxDb - 45) continue;
            float c[12] = {}; float sum = 0; const float* r = M.data() + (size_t) t * kBins; const float* hm = Hm.data() + (size_t) t * kBins;
            for (int k = 12; k < 186; ++k)
            {
                const float f0 = k * binHz; const float midi = 69.0f + 12.0f * std::log2 (f0 / 440.0f); const float w = std::sqrt (r[k] * hm[k]);
                // harmonik ağırlıklandırma: bir tepe, f/n perdelerine de azalan ağırlıkla katkı verir (üçüncü harmonik beşliyi öne çıkarmasın)
                float sw = 1.0f;
                for (int nn = 1; nn <= 4; ++nn, sw *= 0.6f)
                {
                    const float mm = midi - 12.0f * std::log2 ((float) nn); if (mm < 24.0f) break;
                    const int a = (int) std::floor (mm); const float fr2 = mm - a;
                    c[((a % 12) + 12) % 12] += w * sw * (1 - fr2); c[(((a + 1) % 12) + 12) % 12] += w * sw * fr2;
                }
                sum += w;
            }
            if (sum <= 1e-9f) continue;
            const float inv = 1.0f / std::sqrt (sum);   // kısmi normalizasyon: gürültülü karelerin ağırlığı azalır
            for (int i = 0; i < 12; ++i) chroma[i] += c[i] * inv;
        }
        float sc[24]; float prof[12];
        for (int k = 0; k < 12; ++k)
        {
            for (int i = 0; i < 12; ++i) prof[i] = kMaj[(i - k + 12) % 12]; sc[k] = corr12 (chroma, prof);
            for (int i = 0; i < 12; ++i) prof[i] = kMin[(i - k + 12) % 12]; sc[12 + k] = corr12 (chroma, prof);
        }
        int b1 = 0; for (int i = 1; i < 24; ++i) if (sc[i] > sc[b1]) b1 = i;
        int b2 = b1 == 0 ? 1 : 0; for (int i = 0; i < 24; ++i) if (i != b1 && sc[i] > sc[b2]) b2 = i;
        float mean = 0; for (float s : sc) mean += s; mean /= 24;
        out.key = b1 % 12; out.minor = b1 >= 12; out.keyAlt = b2 % 12; out.keyAltMinor = b2 >= 12;
        out.keyConf = clamp01 ((sc[b1] - sc[b2]) / std::max (0.02f, sc[b1] - mean) * 1.2f) * clamp01 ((sc[b1] - 0.2f) / 0.5f);
    }
    prog (0.78f);

    // genel
    {
        double hs = 0, ts = 0, cs = 0, cw = 0, fl = 0; int fc = 0;
        for (int t = 0; t < T; ++t) { hs += F[(size_t) t].harmE; ts += F[(size_t) t].totE; if (F[(size_t) t].totE > 1e-6f) { cs += (double) F[(size_t) t].centroid * F[(size_t) t].totE; cw += F[(size_t) t].totE; fl += F[(size_t) t].flat; ++fc; } }
        out.harmonicRatio = ts > 0 ? (float) (hs / ts) : 0; out.percussiveRatio = 1 - out.harmonicRatio;
        out.centroidHz = cw > 0 ? (float) (cs / cw) : 0; out.flatness = fc ? (float) (fl / fc) : 0;
        double e = 0; for (float v : m.x) e += (double) v * v; out.rmsDb = toDb ((float) (e / m.x.size()));
        double me = 0, se = 0; const int n = clip.numSamples();
        if (clip.audio.getNumChannels() > 1) { const float* L = clip.audio.getReadPointer (0); const float* R = clip.audio.getReadPointer (1); for (int i = 0; i < n; i += 3) { const double a = 0.5 * (L[i] + R[i]), b = 0.5 * (L[i] - R[i]); me += a * a; se += b * b; } }
        out.widthAll = me + se > 1e-12 ? (float) (2 * se / (me + se)) : 0;
    }

    // bölge adayları
    {
        const double secPerBar = out.bpm > 0 && out.bpmConf > 0.15f ? 4.0 * 60.0 / out.bpm : 2.0;
        double W = std::clamp (secPerBar * 2.0, 2.0, 8.0); const double H = W / 2;
        const double dur = N / kAnaSr;
        if (dur < W) W = dur;
        const bool haveSt = clip.audio.getNumChannels() > 1;
        for (double t0 = 0; t0 + W <= dur + 1e-6; t0 += H)
        {
            if (cancelled (cancel)) return false;
            const int a = std::max (0, (int) std::floor ((t0 * kAnaSr) / kHop)), b = std::min (T, (int) std::ceil (((t0 + W) * kAnaSr - kFft) / kHop) + 1);
            if (b - a < 4) continue;
            double sub = 0, low = 0, mid = 0, high = 0, harm = 0, tot = 0, cen = 0, cenW = 0, flt = 0;
            for (int t = a; t < b; ++t) { const auto& f = F[(size_t) t]; sub += f.sub; low += f.low; mid += f.mid; high += f.high; harm += f.harmE; tot += f.totE; cen += (double) f.centroid * f.totE; cenW += f.totE; flt += f.flat; }
            if (tot <= 1e-9) continue;
            RefRegion g; g.t0 = t0; g.t1 = t0 + W;
            g.lowFrac = (float) (sub / tot); g.midFrac = (float) ((low + mid) / tot); g.highFrac = (float) (high / tot); g.harmonic = (float) (harm / tot) * (1.0f - ss (0.10f, 0.35f, (float) (flt / (b - a))));   // gürültü benzeri (düz spektrum) bölgeler harmonik sayılmaz g.centroidHz = (float) (cen / cenW);
            int on = 0; for (float o : out.onsets) if (o >= t0 && o < t0 + W) ++on;
            g.onsetsPerSec = (float) (on / W);
            double e = 0; const int i0 = (int) (t0 * kAnaSr), i1 = std::min (N, (int) ((t0 + W) * kAnaSr)); for (int i = i0; i < i1; ++i) e += (double) m.x[(size_t) i] * m.x[(size_t) i];
            g.rmsDb = toDb ((float) (e / std::max (1, i1 - i0)));
            if (g.rmsDb < -52.0f) continue;     // sessiz bölgeler aday olmaz
            if (haveSt)
            {
                const double sr = clip.sampleRate; const int j0 = (int) (t0 * sr), j1 = std::min (clip.numSamples(), (int) ((t0 + W) * sr)); double me = 0, se = 0;
                const float* L = clip.audio.getReadPointer (0); const float* R = clip.audio.getReadPointer (1);
                for (int j = j0; j < j1; j += 2) { const double p = 0.5 * (L[j] + R[j]), q = 0.5 * (L[j] - R[j]); me += p * p; se += q * q; }
                g.width = me + se > 1e-12 ? (float) (2 * se / (me + se)) : 0;
            }
            const float lowF = g.lowFrac, tonalMid = g.midFrac + 0.4f * g.highFrac, hr = g.harmonic, od = g.onsetsPerSec;
            float s[kNumRefCats];
            s[catBass] = ss (0.45f, 0.85f, lowF) * ss (0.30f, 0.75f, hr);
            s[catPerc] = ss (0.35f, 0.80f, 1 - hr) * ss (1.0f, 4.0f, od);
            s[catLead] = ss (0.35f, 0.75f, tonalMid) * ss (0.45f, 0.80f, hr) * ss (0.6f, 2.5f, od) * (1 - ss (0.55f, 0.90f, lowF));
            s[catPad]  = ss (0.30f, 0.70f, tonalMid) * ss (0.50f, 0.85f, hr) * (1 - ss (0.8f, 3.0f, od)) * (1 - ss (0.55f, 0.90f, lowF));
            int best = 0; for (int c = 1; c < kNumRefCats; ++c) if (s[c] > s[best]) best = c;
            float second = 0; for (int c = 0; c < kNumRefCats; ++c) if (c != best) second = std::max (second, s[c]);
            if (s[best] < 0.2f) continue;      // hiçbir kategoriye yeterince uymayan pencere uydurulmaz
            g.cat = best; g.score = s[best];
            g.confidence = s[best] * (0.5f + 0.5f * clamp01 ((s[best] - second) / std::max (s[best], 1e-3f)));
            out.regions.push_back (g);
        }
        // kategori başına çakışmayan en iyiler
        std::vector<RefRegion> kept;
        for (int c = 0; c < kNumRefCats; ++c)
        {
            auto r = out.top (c, 1000); std::vector<RefRegion> sel;
            for (auto& g : r) { bool ok = true; for (auto& h : sel) if (std::min (g.t1, h.t1) - std::max (g.t0, h.t0) > 0.4 * (g.t1 - g.t0)) { ok = false; break; } if (ok) sel.push_back (g); if (sel.size() >= 6) break; }
            kept.insert (kept.end(), sel.begin(), sel.end());
        }
        out.regions = kept;
    }
    if (out.bpmConf < 0.2f) out.notes += "bpmLow ";
    if (out.keyConf < 0.25f) out.notes += "keyLow ";
    if (clip.truncated) out.notes += "truncated ";
    prog (1.0f);
    out.ok = true;
    return true;
}
}
