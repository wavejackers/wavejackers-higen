#include "AutoMix.h"
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <cmath>

namespace hg
{
namespace
{
constexpr int kBands = 28, kFftOrder = 12, kFft = 1 << kFftOrder;

double erbRate (double f) { return 21.4 * std::log10 (1.0 + 0.00437 * f); }
double erbHz (double e) { return (std::pow (10.0, e / 21.4) - 1.0) / 0.00437; }
double toDb (double x) { return 20.0 * std::log10 (std::max (1e-9, x)); }

struct BandMap
{
    std::array<int, kBands + 1> edge {};   // FFT bin sınırları
    explicit BandMap (double sr)
    {
        const double f0 = 40.0, f1 = std::min (16000.0, 0.45 * sr), e0 = erbRate (f0), e1 = erbRate (f1);
        int prev = 0;
        for (int i = 0; i <= kBands; ++i)
        {
            const double f = erbHz (e0 + (e1 - e0) * i / kBands);
            int b = (int) std::lround (f / sr * kFft);
            b = std::max (b, prev + 1); edge[(size_t) i] = b; prev = b;
        }
    }
};

// Kritik bant gücü: etkin çerçevelerde (kanal başına) ortalama güç. Aynı fonksiyon gürültüye ve kanallara uygulanır, bu yüzden
// mutlak ölçek önemsizdir; yalnızca oran kullanılır.
struct Analysis { std::array<double, kBands> p {}; int frames = 0; };
Analysis analyse (const juce::AudioBuffer<float>& b, double sr, double activeThresholdDb)
{
    Analysis A; const BandMap map (sr);
    juce::dsp::FFT fft (kFftOrder);
    std::vector<float> win ((size_t) kFft), buf ((size_t) kFft * 2);
    for (int i = 0; i < kFft; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (2.0 * juce::MathConstants<double>::pi * i / kFft);
    const int n = b.getNumSamples(), nch = std::min (2, b.getNumChannels());
    if (n <= 0 || nch == 0) return A;
    const double thr = std::pow (10.0, activeThresholdDb / 10.0);
    std::array<double, kBands> acc {};
    for (int pos = 0; pos < std::max (1, n - kFft / 2); pos += kFft / 2)
    {
        double e = 0; const int len = std::min (kFft, n - pos);
        for (int c = 0; c < nch; ++c) { const float* s = b.getReadPointer (c) + pos; for (int i = 0; i < len; ++i) e += (double) s[i] * s[i]; }
        e /= (double) (nch * kFft);
        if (e < thr) continue;
        ++A.frames;
        for (int c = 0; c < nch; ++c)
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            const float* s = b.getReadPointer (c) + pos;
            for (int i = 0; i < len; ++i) buf[(size_t) i] = s[i] * win[(size_t) i];
            fft.performFrequencyOnlyForwardTransform (buf.data(), true);
            for (int k = 0; k < kBands; ++k)
            {
                double sum = 0;
                for (int j = map.edge[(size_t) k]; j < map.edge[(size_t) k + 1] && j <= kFft / 2; ++j) sum += (double) buf[(size_t) j] * buf[(size_t) j];
                acc[(size_t) k] += sum / nch;
            }
        }
    }
    if (A.frames > 0) for (int k = 0; k < kBands; ++k) A.p[(size_t) k] = acc[(size_t) k] / A.frames;
    return A;
}

// Gürültüye göre duyulabilirlik: kanalın bant gücü / gürültünün bant gücü; en yüksek üç oranın ortalaması (dB)
double audibilityDb (const Analysis& lane, const Analysis& noise)
{
    if (lane.frames == 0) return -999;
    std::array<double, kBands> r {};
    for (int k = 0; k < kBands; ++k) r[(size_t) k] = lane.p[(size_t) k] / std::max (1e-30, noise.p[(size_t) k]);
    std::sort (r.begin(), r.end(), std::greater<double>());
    return 10.0 * std::log10 (std::max (1e-30, (r[0] + r[1] + r[2]) / 3.0));
}

const Analysis& noiseAnalysis (double sr)
{
    static std::map<int, Analysis> cache; static juce::CriticalSection lock;
    const juce::ScopedLock sl (lock);
    const int key = (int) sr;
    auto it = cache.find (key);
    if (it == cache.end()) it = cache.emplace (key, analyse (pinkNoise (sr, 20.0, kRefNoiseRmsDb, 1), sr, -80.0)).first;
    return it->second;
}

// Örnekleyici modeli (FL Sampler / Ableton Simpler gibi): davul vuruşu tek örnek; melodik örnek C5 (nota 60) kökte, perde için
// yeniden örnekleme, nota süresi + kısa bırakma kadar çalar. Hız doğrusal (vel/127).
juce::AudioBuffer<float> simulateLane (const ArrLane& L, const juce::AudioBuffer<float>& shot, double sr, double step0, double step1, double stepSec)
{
    const int total = (int) std::ceil ((step1 - step0) * stepSec * sr);
    juce::AudioBuffer<float> out (2, std::max (1, total)); out.clear();
    const int sl = shot.getNumSamples(), sch = shot.getNumChannels();
    if (sl <= 1 || sch == 0) return out;
    for (auto& n : L.notes)
    {
        if (n.s < step0 || n.s >= step1) continue;
        const int t0 = (int) std::lround ((n.s - step0) * stepSec * sr);
        const double amp = juce::jlimit (0.0, 1.0, n.v / 127.0);
        const bool melodic = L.gm < 0;
        const double ratio = melodic ? std::pow (2.0, (n.n - 60) / 12.0) : 1.0;
        const int gate = melodic ? (int) ((n.l * stepSec + 0.06) * sr) : sl;
        const int len = std::min ({ (int) ((sl - 1) / ratio), gate, out.getNumSamples() - t0 });
        const int fade = std::min (len, (int) (0.02 * sr));
        for (int c = 0; c < 2; ++c)
        {
            const float* s = shot.getReadPointer (std::min (c, sch - 1)); float* d = out.getWritePointer (c) + t0;
            for (int i = 0; i < len; ++i)
            {
                const double p = i * ratio; const int i0 = (int) p; const float fr = (float) (p - i0);
                float v = s[i0] + (s[std::min (sl - 1, i0 + 1)] - s[i0]) * fr;
                if (melodic && i > len - fade) v *= (float) (len - i) / (float) std::max (1, fade);
                d[i] += (float) (v * amp);
            }
        }
    }
    return out;
}
}

juce::AudioBuffer<float> pinkNoise (double sr, double seconds, double rmsDb, unsigned seed)
{
    const int n = std::max (1, (int) (sr * seconds));
    juce::AudioBuffer<float> b (2, n);
    for (int c = 0; c < 2; ++c)
    {
        juce::Random rnd ((juce::int64) seed * 7919 + c * 104729);
        double b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0; float* d = b.getWritePointer (c);
        for (int i = 0; i < n; ++i)   // Paul Kellet, rafine: -3 dB/oktav
        {
            const double w = rnd.nextDouble() * 2.0 - 1.0;
            b0 = 0.99886 * b0 + w * 0.0555179; b1 = 0.99332 * b1 + w * 0.0750759; b2 = 0.96900 * b2 + w * 0.1538520;
            b3 = 0.86650 * b3 + w * 0.3104856; b4 = 0.55000 * b4 + w * 0.5329522; b5 = -0.7616 * b5 - w * 0.0168980;
            d[i] = (float) (b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362); b6 = w * 0.115926;
        }
    }
    double e = 0; for (int c = 0; c < 2; ++c) { const float* d = b.getReadPointer (c); for (int i = 0; i < n; ++i) e += (double) d[i] * d[i]; }
    const double rms = std::sqrt (e / (2.0 * n));
    b.applyGain ((float) (std::pow (10.0, rmsDb / 20.0) / std::max (1e-9, rms)));
    return b;
}

double roleOffsetDb (const juce::String& r)
{
    // Eşit duyulabilirlikten tür dengesine: ritim temeli önde, süs perküsyonu ve pad geride
    static const std::map<juce::String, double> m {
        { "kick", 2.0 }, { "clap", 0.0 }, { "snare", 0.0 }, { "hat", -1.0 }, { "ohat", -1.5 }, { "rim", -3.0 }, { "perc", -2.0 },
        { "bass", 1.0 }, { "chords", -1.0 }, { "pad", -3.0 }, { "lead", 1.5 }, { "arp", -1.0 }, { "stab", 0.0 }, { "stutter", -2.0 } };
    auto it = m.find (r); return it != m.end() ? it->second : -1.0;
}
double fxOffsetDb (const juce::String& t)
{
    static const std::map<juce::String, double> m {
        { "riser", -4.0 }, { "sweep", -5.0 }, { "uplifter", -3.0 }, { "impact", 1.0 }, { "subDrop", 0.0 }, { "downlifter", -4.0 },
        { "vocal", -2.0 }, { "noise", -6.0 }, { "roll", -3.0 }, { "fill", -3.0 } };
    auto it = m.find (t); return it != m.end() ? it->second : -3.0;
}

MixResult autoMix (const Arrangement& a, const std::map<juce::String, juce::AudioBuffer<float>>& shots, double sr, juce::AudioBuffer<float>* excerpt)
{
    MixResult R;
    if (a.P.bpm <= 0 || sr <= 0) return R;
    const double stepSec = 15.0 / a.P.bpm;
    // referans bölüm: en çok nota içeren drop/groove (yoksa ilk bölüm, o da yoksa ilk 8 ölçü)
    int bestCount = -1; R.fromBar = 0; R.bars = std::min (8, std::max (1, a.bars));
    for (auto& s : a.secs)
    {
        const bool main = s.kind == "drop" || s.kind == "groove";
        int cnt = 0; for (auto& L : a.lanes) for (auto& n : L.notes) if (n.s >= s.bar * 16.0 && n.s < (s.bar + s.bars) * 16.0) ++cnt;
        const int score = main ? cnt + 1000000 : cnt;
        if (score > bestCount) { bestCount = score; R.fromBar = s.bar; R.bars = std::min (8, std::max (1, s.bars)); }
    }
    const double s0 = R.fromBar * 16.0, s1 = (R.fromBar + R.bars) * 16.0;
    const auto& noise = noiseAnalysis (sr);

    std::vector<juce::AudioBuffer<float>> sims;
    for (auto& L : a.lanes)
    {
        MixLane M; M.role = L.role; M.name = L.name; M.offsetDb = roleOffsetDb (L.role);
        juce::AudioBuffer<float> sim (2, 1);
        if (auto it = shots.find (L.role); it != shots.end() && it->second.getNumSamples() > 1)
        {
            sim = simulateLane (L, it->second, sr, s0, s1, stepSec);
            const auto an = analyse (sim, sr, -50.0);
            M.audDb = audibilityDb (an, noise);
            M.active = an.frames > 0 && M.audDb > -200;
        }
        if (M.active) M.gainDb = kTargetAudDb - M.audDb + M.offsetDb;
        R.lanes.push_back (M); sims.push_back (std::move (sim));
    }
    // ortak kazanç: referans bölüm toplamı -6 dBFS tepe
    const int len = std::max (1, (int) std::ceil ((s1 - s0) * stepSec * sr));
    juce::AudioBuffer<float> mix (2, len); mix.clear();
    auto addAll = [&] (double common)
    {
        mix.clear();
        for (size_t i = 0; i < R.lanes.size(); ++i)
            if (R.lanes[i].active)
                for (int c = 0; c < 2; ++c)
                    mix.addFrom (c, 0, sims[i], c, 0, std::min (len, sims[i].getNumSamples()), (float) std::pow (10.0, (R.lanes[i].gainDb + common) / 20.0));
    };
    addAll (0.0);
    double peak = 0; for (int c = 0; c < 2; ++c) peak = std::max (peak, (double) mix.getMagnitude (c, 0, len));
    R.commonDb = peak > 1e-9 ? juce::jlimit (-60.0, 60.0, kMixPeakDb - toDb (peak)) : 0.0;
    addAll (R.commonDb);
    for (size_t i = 0; i < R.lanes.size(); ++i)
    {
        auto& M = R.lanes[i]; if (! M.active) continue;
        M.gainDb += R.commonDb;
        const float g = (float) std::pow (10.0, M.gainDb / 20.0);
        double pk = 0, e = 0; long cnt = 0;
        for (int c = 0; c < 2; ++c)
        {
            const float* d = sims[i].getReadPointer (c); const int n = std::min (len, sims[i].getNumSamples());
            for (int k = 0; k < n; ++k) { pk = std::max (pk, (double) std::abs (d[k] * g)); e += (double) d[k] * g * d[k] * g; ++cnt; }
        }
        M.peakDb = toDb (pk); M.rmsDb = toDb (std::sqrt (e / std::max (1L, cnt)));
    }
    double mp = 0, me = 0; for (int c = 0; c < 2; ++c) { mp = std::max (mp, (double) mix.getMagnitude (c, 0, len)); const float* d = mix.getReadPointer (c); for (int k = 0; k < len; ++k) me += (double) d[k] * d[k]; }
    R.mixPeakDb = toDb (mp); R.mixRmsDb = toDb (std::sqrt (me / (2.0 * len)));
    if (excerpt != nullptr) *excerpt = std::move (mix);
    return R;
}

double fxGainDb (const juce::String& cueType, const juce::AudioBuffer<float>& b, double sr, const MixResult& m)
{
    const auto an = analyse (b, sr, -60.0);
    const double aud = audibilityDb (an, noiseAnalysis (sr));
    if (aud < -200) return m.commonDb;
    double g = kTargetAudDb - aud + fxOffsetDb (cueType) + m.commonDb;
    double pk = 0; for (int c = 0; c < b.getNumChannels(); ++c) pk = std::max (pk, (double) b.getMagnitude (c, 0, b.getNumSamples()));
    if (pk > 1e-9) g = std::min (g, -3.0 - toDb (pk));
    return g;
}

juce::String mixReport (const MixResult& m, const Arrangement& a)
{
    juce::String s;
    s << "Wavejackers HitGen - Mix Report\n\n"
      << "Method: pink noise reference. Pink noise (-3 dB/octave) at RMS " << juce::String (kRefNoiseRmsDb, 1) << " dBFS is the fixed reference.\n"
      << "Each sound is compared with the noise in 28 critical (ERB) bands and set so its strongest bands sit "
      << juce::String (kTargetAudDb, 1) << " dB above the noise (just audible). A small genre balance offset is added per role.\n"
      << "Gain staging: one common gain puts the loudest reference section (bars " << m.fromBar + 1 << "-" << m.fromBar + m.bars
      << ") at a " << juce::String (kMixPeakDb, 1) << " dBFS peak. Mixer faders stay at 0 dB; the gain is written into the samples.\n\n"
      << "Common gain: " << juce::String (m.commonDb, 1) << " dB   Mix peak: " << juce::String (m.mixPeakDb, 1) << " dBFS   Mix RMS: " << juce::String (m.mixRmsDb, 1) << " dBFS\n\n"
      << "Lane                 audibility  offset    gain     peak     rms   (dB)\n";
    for (auto& l : m.lanes)
    {
        if (! l.active) { s << l.name.paddedRight (' ', 20) << "         -       -       -       -       -   (no sample)\n"; continue; }
        s << l.name.paddedRight (' ', 20) << juce::String (l.audDb, 1).paddedLeft (' ', 10) << juce::String (l.offsetDb, 1).paddedLeft (' ', 8)
          << juce::String (l.gainDb, 1).paddedLeft (' ', 8) << juce::String (l.peakDb, 1).paddedLeft (' ', 8) << juce::String (l.rmsDb, 1).paddedLeft (' ', 8) << "\n";
    }
    s << "\nAudio/Pink Noise Reference.wav is the same reference noise. To check the balance yourself: solo it, bring one channel in at a time and "
         "each should sit just above the noise.\nThe offsets are starting points; trust your ears for the final balance. Tempo " << a.P.bpm << " BPM.\n";
    return s;
}
}
