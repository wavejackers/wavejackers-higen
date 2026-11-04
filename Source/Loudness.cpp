#include "Loudness.h"
#include <cmath>

namespace hg
{
// ---------------------------------------------------------------- K-ağırlıklı ölçer (BS.1770-4)
void LoudnessMeter::prepare (double sampleRate)
{
    sr = sampleRate;
    {   // 1. aşama: yüksek raf (kafa etkisi)
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / sr), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        shelf.b0 = (Vh + Vb * K / Q + K * K) / a0; shelf.b1 = 2.0 * (K * K - Vh) / a0; shelf.b2 = (Vh - Vb * K / Q + K * K) / a0;
        shelf.a1 = 2.0 * (K * K - 1.0) / a0; shelf.a2 = (1.0 - K / Q + K * K) / a0;
    }
    {   // 2. aşama: RLB yüksek geçiren
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / sr), a0 = 1.0 + K / Q + K * K;
        hp.b0 = 1.0; hp.b1 = -2.0; hp.b2 = 1.0; hp.a1 = 2.0 * (K * K - 1.0) / a0; hp.a2 = (1.0 - K / Q + K * K) / a0;
    }
    subLen = juce::jmax (1, (int) std::lround (sr * 0.1));
    reset();
}

void LoudnessMeter::reset()
{
    for (auto* b : { &shelf, &hp }) for (int c = 0; c < 2; ++c) b->z1[c] = b->z2[c] = 0;
    acc = 0; accN = 0; sub.fill (0); subPos = 0; subCount = 0; hist.fill (0); histPos = 0; histCount = 0; integ = mom = -70.0f;
}

void LoudnessMeter::add (const float* L, const float* R, int n, float gain) noexcept
{
    const double g2 = (double) gain * gain;
    for (int i = 0; i < n; ++i)
    {
        const double l = hp.run (0, shelf.run (0, (double) L[i]));
        double e = l * l;
        if (R != nullptr) { const double r = hp.run (1, shelf.run (1, (double) R[i])); e += r * r; }
        acc += e * g2;
        if (++accN >= subLen) { sub[(size_t) subPos] = acc / accN; subPos = (subPos + 1) & 3; ++subCount; acc = 0; accN = 0; blockDone(); }
    }
}

void LoudnessMeter::blockDone() noexcept
{
    if (subCount < 4) return;
    const double z = (sub[0] + sub[1] + sub[2] + sub[3]) * 0.25;
    mom = z > 1e-12 ? (float) (-0.691 + 10.0 * std::log10 (z)) : -70.0f;
    hist[(size_t) histPos] = z; histPos = (histPos + 1) % kHist; histCount = juce::jmin (kHist, histCount + 1);
    const double absGate = std::pow (10.0, (-70.0 + 0.691) / 10.0);
    double s = 0; int c = 0;
    for (int i = 0; i < histCount; ++i) if (hist[(size_t) i] > absGate) { s += hist[(size_t) i]; ++c; }
    if (c == 0) { integ = -70.0f; return; }
    const double relGate = (s / c) * std::pow (10.0, -10.0 / 10.0), gate = juce::jmax (absGate, relGate);
    double s2 = 0; int c2 = 0;
    for (int i = 0; i < histCount; ++i) if (hist[(size_t) i] > gate) { s2 += hist[(size_t) i]; ++c2; }
    integ = c2 > 0 ? (float) (-0.691 + 10.0 * std::log10 (s2 / c2)) : -70.0f;
}

// ---------------------------------------------------------------- normalleştirici + sınırlayıcı
void LoudnessNormalizer::prepare (double sampleRate, int)
{
    sr = sampleRate;
    inMeter.prepare (sr); outMeter.prepare (sr);
    la = juce::jmax (8, (int) std::lround (sr * 0.0015));          // 1.5 ms ileri bakış
    dlyL.assign ((size_t) la + 1, 0.0f); dlyR.assign ((size_t) la + 1, 0.0f); req.assign ((size_t) la + 1, 1.0f);
    upd = juce::jmax (1, (int) std::lround (sr * 0.1));
    // 4x ara değer: x[5] ile x[6] arasında t = 1/4, 1/2, 3/4 (Hann pencereli sinc, 12 dokunuş, birim DC kazancı)
    for (int ph = 0; ph < 3; ++ph)
    {
        const double t = (ph + 1) * 0.25; double sum = 0;
        for (int k = 0; k < kTp; ++k)
        {
            const double x = (k - 5) - t, sinc = std::abs (x) < 1e-9 ? 1.0 : std::sin (juce::MathConstants<double>::pi * x) / (juce::MathConstants<double>::pi * x);
            const double w = 0.5 + 0.5 * std::cos (juce::MathConstants<double>::pi * x / 6.5);
            tpc[ph][k] = (float) (sinc * w); sum += sinc * w;
        }
        for (int k = 0; k < kTp; ++k) tpc[ph][k] = (float) (tpc[ph][k] / sum);
    }
    atkCoef = 1.0f - (float) std::exp (-4.0 / la);
    relCoef = (float) std::exp (-1.0 / (sr * 0.08));               // 80 ms bırakma
    reset();
}

void LoudnessNormalizer::reset()
{
    inMeter.reset(); outMeter.reset();
    std::fill (dlyL.begin(), dlyL.end(), 0.0f); std::fill (dlyR.begin(), dlyR.end(), 0.0f); std::fill (req.begin(), req.end(), 1.0f);
    for (int k = 0; k < kTp; ++k) hl[k] = hr[k] = 0.0f;
    dpos = 0; limG = 1.0f; holdMin = 1.0f; holdLeft = 0; sinceUpdate = 0;
    // kazanç ve düzeltme korunur: yeniden başlatmada ses yüksekliği zıplamaz
}

void LoudnessNormalizer::updateGain() noexcept
{
    if (! inMeter.hasData() || inMeter.momentary() < -55.0f) return;          // sessizlik: kazancı tut
    const float desired = juce::jlimit (-24.0f, 18.0f, target - inMeter.integrated() + trimDb);
    const float d = desired - gDbCur;
    const float up = 0.3f, down = 0.6f;                                      // 100 ms başına dB (3 / 6 dB/s)
    gDbCur += juce::jlimit (-down, up, d);
    // sınırlayıcının ve gecikmenin kaybını düzelt: kazanç oturduysa çıkış ölçümüne göre yavaşça ayarla
    if (std::abs (d) < 0.5f && outMeter.hasData())
        trimDb = juce::jlimit (-4.0f, 4.0f, trimDb + juce::jlimit (-0.05f, 0.05f, (target - outMeter.integrated()) * 0.05f));
    gainDb.store (gDbCur, std::memory_order_relaxed);
    gtLin = std::pow (10.0f, gDbCur / 20.0f);
}

float LoudnessNormalizer::process (float* L, float* R, int n, float masterPre, float masterPost) noexcept
{
    inMeter.add (L, R, n);
    const float invPre = masterPre > 0.0f ? 1.0f / masterPre : 1.0f;
    float pk = 0.0f;
    const int size = (int) dlyL.size();
    for (int i = 0; i < n; ++i)
    {
        if (++sinceUpdate >= upd) { sinceUpdate = 0; updateGain(); }
        g += (gtLin - g) * 0.0005f;                                            // örnek düzeyinde yumuşatma (tıkırtı yok)
        const float gg = g * masterPre;
        const float l = L[i] * gg, r = R != nullptr ? R[i] * gg : l;
        // örnekler arası tepe: son 12 örnekten 4x ara değerler (pencereli sinc), orta bölge (6 örnek gecikmeli; ileri bakış kapsar)
        for (int k = 0; k < kTp - 1; ++k) { hl[k] = hl[k + 1]; hr[k] = hr[k + 1]; }
        hl[kTp - 1] = l; hr[kTp - 1] = r;
        float a = juce::jmax (std::abs (l), std::abs (r));
        for (int ph = 0; ph < 3; ++ph)
        {
            float yl = 0, yr = 0; for (int k = 0; k < kTp; ++k) { yl += tpc[ph][k] * hl[k]; yr += tpc[ph][k] * hr[k]; }
            a = juce::jmax (a, std::abs (yl), std::abs (yr));
        }
        const float need = a > kCeiling ? kCeiling / a : 1.0f;
        if (need <= holdMin) { holdMin = need; holdLeft = la; } else if (holdLeft > 0) --holdLeft; else holdMin = need;
        limG = holdMin < limG ? limG + (holdMin - limG) * atkCoef : holdMin + (limG - holdMin) * relCoef;
        dlyL[(size_t) dpos] = l; dlyR[(size_t) dpos] = r;
        int rp = dpos - la; if (rp < 0) rp += size;
        float yl = dlyL[(size_t) rp] * limG, yr = dlyR[(size_t) rp] * limG;
        if (++dpos >= size) dpos = 0;
        yl = juce::jlimit (-kCeiling, kCeiling, yl); yr = juce::jlimit (-kCeiling, kCeiling, yr);
        if (! std::isfinite (yl)) yl = 0.0f;
        if (! std::isfinite (yr)) yr = 0.0f;
        L[i] = yl; if (R != nullptr) R[i] = yr;
    }
    outMeter.add (L, R, n, invPre);
    for (int i = 0; i < n; ++i) { L[i] *= masterPost; pk = juce::jmax (pk, std::abs (L[i])); if (R != nullptr) { R[i] *= masterPost; pk = juce::jmax (pk, std::abs (R[i])); } }
    outLufs.store (outMeter.hasData() ? outMeter.integrated() + 20.0f * std::log10 (juce::jmax (1e-4f, masterPost)) : -70.0f, std::memory_order_relaxed);
    return pk;
}
}
