#include "LoopForge.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace lf
{
const ScaleDef kScales[5] = { { "Minor", { 0, 2, 3, 5, 7, 8, 10 } }, { "Phrygian", { 0, 1, 3, 5, 7, 8, 10 } }, { "Dorian", { 0, 2, 3, 5, 7, 9, 10 } },
                              { "Harmonic minor", { 0, 2, 3, 5, 7, 8, 11 } }, { "Major", { 0, 2, 4, 5, 7, 9, 11 } } };

int degreeToSemis (int scale, int degree)
{
    const auto& iv = kScales[juce::jlimit (0, 4, scale)].iv;
    const int oct = (int) std::floor (degree / 7.0);
    return iv[degree - oct * 7] + 12 * oct;
}

std::array<bool, kMaxSteps> euclid (int steps, int pulses, int rotation)
{
    std::array<bool, kMaxSteps> out {};
    steps = juce::jlimit (1, kMaxSteps, steps); pulses = juce::jlimit (0, steps, pulses);
    for (int i = 0; i < steps; ++i) out[(size_t) (((i + rotation) % steps + steps) % steps)] = (i * pulses) % steps < pulses;
    return out;
}

void mutate (Settings& s, MutateMode mode, uint32_t seed)
{
    juce::Random r ((juce::int64) seed);
    const int n = s.steps;
    if (mode == mSubtle)   // en çok iki küçük değişiklik: bir tetik ve bir perde
    {
        const int a = r.nextInt (n), b = r.nextInt (n);
        if (a % 4 != 0) s.step[(size_t) a].on = ! s.step[(size_t) a].on;
        if (s.step[(size_t) b].on) s.step[(size_t) b].pitch = juce::jlimit (-7, 7, s.step[(size_t) b].pitch + (r.nextBool() ? 1 : -1) * (1 + r.nextInt (2)));
        return;
    }
    s.pulses = juce::jlimit (2, n, n / 4 + r.nextInt (juce::jmax (1, n / 2)));
    s.rotation = r.nextInt (n);
    const auto e = euclid (n, s.pulses, s.rotation);
    static const int arp[8] = { 0, 2, 4, 7, 4, 2, 0, -3 };
    static const int alt[4] = { 2, 4, 7, -3 };
    int k = r.nextInt (8);
    for (int i = 0; i < n; ++i)
    {
        auto& st = s.step[(size_t) i]; st = Step(); st.on = e[(size_t) i];
        if (! st.on) continue;
        st.vel = i % 4 == 0 ? .95f : .6f + r.nextFloat() * .3f;
        if (mode == mArp) { st.pitch = arp[k++ % 8]; st.decay = .35f; }
        else
        {
            if (r.nextFloat() < .2f) st.pitch = alt[r.nextInt (4)];
            if (r.nextFloat() < .12f) st.rev = true;
            if (r.nextFloat() < .15f) st.ratchet = 2 + r.nextInt (2);
            st.pan = r.nextFloat() < .3f ? r.nextFloat() - .5f : 0.0f;
        }
    }
    s.seed = (int) (seed & 0x7fffffff);
}

static int snapZero (const juce::AudioBuffer<float>& b, int at, int range)
{
    const int N = b.getNumSamples(); if (N < 2) return 0;
    auto v = [&] (int i) { float x = 0; for (int c = 0; c < b.getNumChannels(); ++c) x += b.getSample (c, i); return x; };
    for (int d = 0; d < range; ++d)
        for (int i : { at - d, at + d })
            if (i > 0 && i < N && (v (i - 1) <= 0) != (v (i) <= 0)) return i;
    return juce::jlimit (0, N - 1, at);
}

void autoTrim (const juce::AudioBuffer<float>& b, float& start, float& end)
{
    const int N = b.getNumSamples(); start = 0; end = 1; if (N < 16) return;
    const float thr = b.getMagnitude (0, N) * .003f;   // tepe değerin -50 dB altı
    int a = 0, z = N - 1;
    auto mag = [&] (int i) { float m = 0; for (int c = 0; c < b.getNumChannels(); ++c) m = juce::jmax (m, std::abs (b.getSample (c, i))); return m; };
    while (a < z && mag (a) < thr) ++a;
    while (z > a && mag (z) < thr) --z;
    a = snapZero (b, a, 96); z = snapZero (b, z, 96);
    if (z - a < 16) return;
    start = (float) a / (float) N; end = (float) (z + 1) / (float) N;
}

void findHit (const juce::AudioBuffer<float>& b, double sr, double maxSeconds, int& from, int& to)
{
    const int N = b.getNumSamples(); from = 0; to = N; if (N < 64) return;
    int pk = 0; float pm = 0;
    for (int c = 0; c < b.getNumChannels(); ++c) { auto* d = b.getReadPointer (c); for (int i = 0; i < N; ++i) if (std::abs (d[i]) > pm) { pm = std::abs (d[i]); pk = i; } }
    if (pm <= 1e-5f) return;
    const int win = juce::jmax (8, (int) (sr * .005)); const float thr = pm * .02f;
    auto quiet = [&] (int at) { for (int c = 0; c < b.getNumChannels(); ++c) if (b.getMagnitude (c, at, juce::jmin (win, N - at)) >= thr) return false; return true; };
    from = pk; while (from > win && pk - from < (int) (sr * .25) && ! quiet (from - win)) from -= win;
    to = pk; const int lim = juce::jmin (N, from + (int) (sr * maxSeconds));
    while (to + win < lim && ! quiet (to)) to += win;
    to = juce::jmin (lim, to + win);
}

juce::ValueTree Settings::toTree() const
{
    juce::ValueTree t ("loopforge");
    t.setProperty ("steps", steps, nullptr); t.setProperty ("pulses", pulses, nullptr); t.setProperty ("rotation", rotation, nullptr); t.setProperty ("bars", bars, nullptr);
    t.setProperty ("root", root, nullptr); t.setProperty ("scale", scale, nullptr); t.setProperty ("swing", swing, nullptr); t.setProperty ("human", human, nullptr);
    t.setProperty ("start", start, nullptr); t.setProperty ("end", end, nullptr); t.setProperty ("space", space, nullptr); t.setProperty ("drive", drive, nullptr);
    t.setProperty ("motion", motion, nullptr); t.setProperty ("seed", seed, nullptr);
    juce::String g;   // adım başına: açık, ters, perde, ratchet, hız, kesim, pan, sönüm
    for (int i = 0; i < kMaxSteps; ++i) { const auto& x = step[(size_t) i]; g << (x.on ? 1 : 0) << ' ' << (x.rev ? 1 : 0) << ' ' << x.pitch << ' ' << x.ratchet << ' ' << x.vel << ' ' << x.cut << ' ' << x.pan << ' ' << x.decay << ';'; }
    t.setProperty ("grid", g, nullptr);
    return t;
}

void Settings::fromTree (const juce::ValueTree& t)
{
    if (! t.hasType ("loopforge")) return;
    auto I = [&] (const char* k, int d, int lo, int hi) { return juce::jlimit (lo, hi, (int) t.getProperty (k, d)); };
    auto F = [&] (const char* k, float d, float lo = 0, float hi = 1) { return juce::jlimit (lo, hi, (float) t.getProperty (k, d)); };
    steps = I ("steps", 16, 16, 32) >= 32 ? 32 : 16; pulses = I ("pulses", 5, 0, 32); rotation = I ("rotation", 0, 0, 31);
    bars = I ("bars", 2, 1, 8); if (bars != 1 && bars != 2 && bars != 4 && bars != 8) bars = 2;
    root = I ("root", 9, 0, 11); scale = I ("scale", 1, 0, 4); seed = I ("seed", 1, 0, 0x7fffffff);
    swing = F ("swing", 0); human = F ("human", 0); start = F ("start", 0); end = F ("end", 1); space = F ("space", .25f); drive = F ("drive", .15f); motion = F ("motion", 0);
    if (end - start < .001f) { start = 0; end = 1; }
    auto rows = juce::StringArray::fromTokens (t.getProperty ("grid").toString(), ";", "");
    for (int i = 0; i < kMaxSteps && i < rows.size(); ++i)
    {
        auto v = juce::StringArray::fromTokens (rows[i], " ", ""); if (v.size() < 8) continue;
        auto& x = step[(size_t) i];
        x.on = v[0].getIntValue() != 0; x.rev = v[1].getIntValue() != 0; x.pitch = juce::jlimit (-14, 14, v[2].getIntValue()); x.ratchet = juce::jlimit (1, 4, v[3].getIntValue());
        x.vel = juce::jlimit (0.0f, 1.0f, v[4].getFloatValue()); x.cut = juce::jlimit (0.0f, 1.0f, v[5].getFloatValue());
        x.pan = juce::jlimit (-1.0f, 1.0f, v[6].getFloatValue()); x.decay = juce::jlimit (0.0f, 1.0f, v[7].getFloatValue());
    }
}

// ---- çevrimdışı işleme
namespace
{
struct Svf   // durum değişkenli alçak geçiren (TPT)
{
    float ic1 = 0, ic2 = 0;
    float lp (float x, float g, float k) { const float a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2, v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3; ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2; return v2; }
};
inline float gOf (double hz, double sr) { return (float) std::tan (juce::MathConstants<double>::pi * juce::jlimit (20.0, sr * .45, hz) / sr); }
}

std::unique_ptr<Loop> render (const Settings& s, const juce::AudioBuffer<float>& sample, double sampleSr, double bpm, double sr)
{
    bpm = juce::jlimit (40.0, 240.0, bpm);
    const double stepDur = 60.0 / bpm / 4 * sr;                       // 1/16 nota (örnek)
    const int N = juce::jmax (64, (int) std::llround (stepDur * 16 * s.bars));
    auto L = std::make_unique<Loop>(); L->bpm = bpm; L->sr = sr; L->bars = s.bars; L->buf.setSize (2, N); L->buf.clear();
    const int SN = sample.getNumSamples();
    if (SN < 16) return L;
    const int a = juce::jlimit (0, SN - 2, (int) (s.start * SN)), z = juce::jlimit (a + 2, SN, (int) (s.end * SN)), segLen = z - a;
    const float* sl = sample.getReadPointer (0); const float* sR = sample.getReadPointer (sample.getNumChannels() > 1 ? 1 : 0);
    juce::AudioBuffer<float> dry (2, N); dry.clear();
    float* dl = dry.getWritePointer (0); float* dr = dry.getWritePointer (1);
    juce::Random hr ((juce::int64) s.seed * 7919 + 3);
    const int total = 16 * s.bars;
    for (int k = 0; k < total; ++k)
    {
        const auto& st = s.step[(size_t) (k % s.steps)];
        const double jitter = (hr.nextDouble() * 2 - 1) * s.human * .02 * sr;   // en çok ±20 ms
        if (! st.on) continue;
        const double rate = std::pow (2.0, degreeToSemis (s.scale, st.pitch) / 12.0) * sampleSr / sr;
        const int rat = juce::jlimit (1, 4, st.ratchet);
        const double sub = stepDur / rat;
        const double maxLen = juce::jmin ((double) segLen / rate, stepDur * (.25 + st.decay * st.decay * 16));
        const float g = gOf (200.0 * std::pow (90.0, (double) st.cut), sr), kq = 1.2f;
        const float pl = std::cos ((st.pan + 1) * juce::MathConstants<float>::pi * .25f) * 1.4142f, pr = std::sin ((st.pan + 1) * juce::MathConstants<float>::pi * .25f) * 1.4142f;
        for (int q = 0; q < rat; ++q)
        {
            const double t0 = k * stepDur + (k % 2 == 1 ? s.swing * stepDur / 3 : 0.0) + jitter + q * sub;
            const int len = juce::jmax (8, (int) (rat > 1 ? juce::jmin (maxLen, sub) : maxLen)), fade = juce::jmin (len / 2, juce::jmax (8, (int) (sr * .008))), atk = juce::jmin (len / 4, (int) (sr * .002) + 1);
            const float vel = st.vel * (q == 0 ? 1.0f : .8f);
            Svf fl, fr;
            for (int i = 0; i < len; ++i)
            {
                const double p = st.rev ? (z - 2) - i * rate : a + i * rate;
                const int i0 = (int) p; if (i0 < a || i0 + 1 >= z) break;
                const float fr2 = (float) (p - i0);
                float xl = sl[i0] + (sl[i0 + 1] - sl[i0]) * fr2, xr = sR[i0] + (sR[i0 + 1] - sR[i0]) * fr2;
                if (st.cut < .98f) { xl = fl.lp (xl, g, kq); xr = fr.lp (xr, g, kq); }
                const float env = vel * juce::jmin (1.0f, (float) i / (float) atk) * juce::jmin (1.0f, (float) (len - i) / (float) fade);
                const int o = (int) (((long long) std::llround (t0) + i) % N + N) % N;   // döngü sonunu aşan ses başa sarar
                dl[o] += xl * env * pl; dr[o] += xr * env * pr;
            }
        }
    }
    // ---- makrolar: DRIVE (bant doygunluğu), MOTION (tempoya kilitli filtre ve genişlik), SPACE (ping-pong delay + reverb)
    const float drv = 1 + s.drive * 7, drvNorm = 1 / std::tanh (drv);
    const int dN = juce::jmax (1, (int) std::llround (stepDur * 3));   // noktalı sekizlik
    std::vector<float> dL ((size_t) dN, 0.0f), dR ((size_t) dN, 0.0f); int dp = 0;
    const float fb = .25f + .35f * s.space, dWet = .4f * s.space, rWet = .3f * s.space;
    juce::Reverb rev; juce::Reverb::Parameters rp; rp.roomSize = .45f + .45f * s.space; rp.damping = .45f; rp.wetLevel = 1; rp.dryLevel = 0; rp.width = 1; rev.setSampleRate (sr); rev.setParameters (rp);
    Svf ml, mr; const double bar = stepDur * 16;
    float* ol = L->buf.getWritePointer (0); float* orr = L->buf.getWritePointer (1);
    for (int pass = 0; pass < 2; ++pass)
        for (int i = 0; i < N; ++i)
        {
            float xl = dl[i], xr = dr[i];
            if (s.drive > .001f) { xl = std::tanh (xl * drv) * drvNorm; xr = std::tanh (xr * drv) * drvNorm; }
            if (s.motion > .001f)
            {
                const double ph = juce::MathConstants<double>::twoPi * i / bar;
                const float g = gOf (18000.0 * std::pow (2.0, -5.0 * s.motion * (.5 - .5 * std::cos (ph))), sr);
                xl = ml.lp (xl, g, 1.0f); xr = mr.lp (xr, g, 1.0f);
                const float mid = (xl + xr) * .5f, side = (xl - xr) * .5f * (1 + s.motion * .8f * (float) std::sin (ph * 2));
                xl = mid + side; xr = mid - side;
            }
            const float el = dL[(size_t) dp], er = dR[(size_t) dp];
            dL[(size_t) dp] = (xl + xr) * .5f + er * fb; dR[(size_t) dp] = el * fb;   // ping-pong: sol -> sağ -> sol
            dp = (dp + 1) % dN;
            float wl = xl + el * dWet, wr = xr + er * dWet;
            float rl = wl, rr = wr; rev.processStereo (&rl, &rr, 1);
            ol[i] = wl + rl * rWet; orr[i] = wr + rr * rWet;
        }
    const float pk = L->buf.getMagnitude (0, N);
    if (pk > .891f) L->buf.applyGain (.891f / pk);   // -1 dBFS tavan
    return L;
}

// ---- motor
Engine::Engine() : juce::Thread ("LoopForge render")
{
    formats.registerBasicFormats();
    for (auto& t : trash) t.store (nullptr);
    const auto e = euclid (16, 5, 0); for (int i = 0; i < 16; ++i) { s.step[(size_t) i] = Step(); s.step[(size_t) i].on = e[(size_t) i]; }
    startThread();
}

Engine::~Engine()
{
    signalThreadShouldExit(); notify(); stopThread (4000);
    delete pending.exchange (nullptr); delete live; collect();
}

void Engine::prepare (double sampleRate)
{
    sr = sampleRate; ringLen = (int) (kRingSeconds * sr); ring.setSize (2, ringLen); ring.clear(); ringPos.store (0);
    changed();
}

void Engine::capture (const juce::AudioBuffer<float>& b, int n) noexcept
{
    if (ringLen <= 0 || b.getNumChannels() < 1) return;
    int w = ringPos.load (std::memory_order_relaxed);
    const float* l = b.getReadPointer (0); const float* r = b.getReadPointer (b.getNumChannels() > 1 ? 1 : 0);
    float* rl = ring.getWritePointer (0); float* rr = ring.getWritePointer (1);
    for (int i = 0; i < n; ++i) { rl[w] = l[i]; rr[w] = r[i]; if (++w >= ringLen) w = 0; }
    ringPos.store (w, std::memory_order_release);
}

void Engine::process (juce::AudioBuffer<float>& out, int n, bool hostPlay, double ppq, double bpm) noexcept
{
    if (auto* np = pending.exchange (nullptr))
    {
        if (live != nullptr) { bool put = false; for (auto& t : trash) { Loop* e = nullptr; if (t.compare_exchange_strong (e, live)) { put = true; break; } } if (! put) delete live; }
        live = np;
    }
    if (! playing.load() || live == nullptr || out.getNumChannels() < 2) return;
    const int N = live->buf.getNumSamples(); if (N < 2) return;
    if (hostPlay)   // konumu ev sahibinin PPQ'sundan al: döngü ölçü çizgisine kilitli kalır
    {
        const double beats = live->bars * 4.0, f = std::fmod (std::fmod (ppq, beats) + beats, beats) / beats;
        freePos = f * N;
    }
    const double inc = hostPlay ? bpm / live->bpm : 1.0;   // tempo değişince yeni döngü gelene kadar hızla telafi
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
}

void Engine::collect() { for (auto& t : trash) delete t.exchange (nullptr); }
void Engine::changed() { wantVersion.fetch_add (1); notify(); }

void Engine::run()
{
    while (! threadShouldExit())
    {
        const int want = wantVersion.load();
        if (want == doneVersion.load()) { wait (200); continue; }
        Settings cs; juce::AudioBuffer<float> smp; double ssr, bpm, osr;
        { const juce::ScopedLock sl (lock); cs = s; smp.makeCopyOf (sample); ssr = sampleSr; bpm = tempo; osr = sr; }
        auto L = render (cs, smp, ssr, bpm, osr);
        { const juce::ScopedLock sl (lock); last = std::make_unique<Loop> (*L); }
        delete pending.exchange (L.release());
        doneVersion.store (want);
    }
}

void Engine::setSample (juce::AudioBuffer<float>&& b, double bsr, const juce::String& nm, const juce::File& f)
{
    float st = 0, en = 1; autoTrim (b, st, en);
    { const juce::ScopedLock sl (lock); sample = std::move (b); sampleSr = bsr; name = nm; path = f; s.start = st; s.end = en; }
    changed();
}

bool Engine::loadFile (const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (f));
    if (rd == nullptr || rd->lengthInSamples < 16) return false;
    const int len = (int) juce::jmin<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 20));   // en çok 20 sn
    juce::AudioBuffer<float> b (juce::jmin (2, (int) rd->numChannels), len);
    rd->read (&b, 0, len, 0, true, true);
    setSample (std::move (b), rd->sampleRate, f.getFileNameWithoutExtension(), f);
    return true;
}

bool Engine::captureNow()
{
    if (ringLen <= 0) return false;
    juce::AudioBuffer<float> lin (2, ringLen);
    const int w = ringPos.load();
    for (int c = 0; c < 2; ++c) { lin.copyFrom (c, 0, ring, c, w, ringLen - w); lin.copyFrom (c, ringLen - w, ring, c, 0, w); }
    if (lin.getMagnitude (0, ringLen) < 1e-4f) return false;
    int a = 0, z = ringLen; findHit (lin, sr, 2.0, a, z);
    if (z - a < 64) return false;
    juce::AudioBuffer<float> hit (2, z - a);
    for (int c = 0; c < 2; ++c) hit.copyFrom (c, 0, lin, c, a, z - a);
    // yakalanan ses kalıcı olsun: proje yeniden açıldığında dosyadan yüklenir
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Wavejackers HitGen").getChildFile ("LoopForge");
    dir.createDirectory();
    auto f = dir.getChildFile ("Capture " + juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M-%S") + ".wav");
    if (auto os = f.createOutputStream())
        if (std::unique_ptr<juce::AudioFormatWriter> wr { juce::WavAudioFormat().createWriterFor (os.get(), sr, 2, 24, {}, 0) }) { os.release(); wr->writeFromAudioSampleBuffer (hit, 0, hit.getNumSamples()); }
    setSample (std::move (hit), sr, "Capture", f.existsAsFile() ? f : juce::File());
    return true;
}

Settings Engine::get() const { const juce::ScopedLock sl (lock); return s; }
void Engine::set (const Settings& n) { { const juce::ScopedLock sl (lock); s = n; } changed(); }
void Engine::setTempo (double bpm) { { const juce::ScopedLock sl (lock); if (std::abs (bpm - tempo) < .01) return; tempo = bpm; } changed(); }
bool Engine::hasSample() const { const juce::ScopedLock sl (lock); return sample.getNumSamples() >= 16; }
juce::String Engine::sampleName() const { const juce::ScopedLock sl (lock); return name; }

void Engine::peaks (std::vector<float>& out, int n) const
{
    const juce::ScopedLock sl (lock);
    out.assign ((size_t) juce::jmax (0, n), 0.0f);
    const int N = sample.getNumSamples(); if (N < 1 || n < 1) return;
    for (int i = 0; i < n; ++i) { const int a = (int) ((juce::int64) i * N / n), len = juce::jmax (1, (int) ((juce::int64) (i + 1) * N / n) - a); out[(size_t) i] = sample.getMagnitude (a, juce::jmin (len, N - a)); }
}

juce::File Engine::exportWav()
{
    for (int i = 0; i < 100 && wantVersion.load() != doneVersion.load(); ++i) juce::Thread::sleep (20);   // süren işlemeyi bekle (en çok 2 sn)
    const juce::ScopedLock sl (lock);
    if (last == nullptr || sample.getNumSamples() < 16) return {};
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("LoopForge " + (name.isEmpty() ? juce::String ("Loop") : juce::File::createLegalFileName (name)) + " " + juce::String (last->bars) + "bar " + juce::String (juce::roundToInt (last->bpm)) + "bpm.wav");
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
    if (! t.hasType ("loopforge")) return;
    const juce::File f (t.getProperty ("file").toString());
    if (f.existsAsFile()) loadFile (f);          // otomatik kırpma çalışır; kayıtlı sınırlar aşağıda geri yazılır
    { const juce::ScopedLock sl (lock); s.fromTree (t); }
    playing.store ((bool) t.getProperty ("play", false) && f.existsAsFile());
    changed();
}
}
