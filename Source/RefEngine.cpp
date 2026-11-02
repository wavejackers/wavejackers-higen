#include "RefEngine.h"
#include <juce_events/juce_events.h>

namespace hg
{
RefEngine::RefEngine() : alive (std::make_shared<std::atomic<bool>> (true)) {}
RefEngine::~RefEngine() { *alive = false; stopFlag = true; playing = false; pool.removeAllJobs (true, 5000); }

void RefEngine::post (std::function<void()> f)
{
    auto a = alive;
    juce::MessageManager::callAsync ([a, f = std::move (f)] { if (a->load()) f(); });
}

void RefEngine::setClip (std::shared_ptr<RefClip> c)
{
    std::shared_ptr<const RefClip> old;
    { const juce::SpinLock::ScopedLockType sl (lock); old = std::move (clipAudio); clipAudio = c; rawAudio = c.get(); }
    // eski klip burada (mesaj iş parçacığında) yok edilir, ses iş parçacığında değil
    old.reset();
}

void RefEngine::cancel() { stopFlag = true; pool.removeAllJobs (true, 5000); stopFlag = false; if (state == Loading || state == Analysing) state = clipMsg ? Loaded : Idle; progress = 0; }

void RefEngine::clear()
{
    cancel(); playing = false; setClip (nullptr); clipMsg.reset(); analysisMsg.reset(); error.clear(); state = Idle; regionStart = regionEnd = 0; posSec = 0; seekTo = 0; setPath ({}, {}); missingPath.clear();
    if (onChange) onChange();
}

void RefEngine::load (const juce::File& f)
{
    cancel(); playing = false; error.clear(); state = Loading; progress = 0; analysisMsg.reset();
    if (onChange) onChange();
    pool.addJob ([this, f]
    {
        juce::String err; auto c = loadRefAudio (f, err, &stopFlag);
        if (stopFlag) return;
        post ([this, c, err]
        {
            if (c == nullptr) { error = err.isEmpty() ? "fileBad" : err; state = Failed; clipMsg.reset(); setClip (nullptr); setPath ({}, {}); }
            else { setClip (std::const_pointer_cast<RefClip> (c)); clipMsg = c; setPath (c->path, c->name); state = Loaded; regionStart = regionEnd = 0; posSec = 0; seekTo = 0; }
            progress = 1; if (onChange) onChange();
        });
    });
}

void RefEngine::analyse()
{
    auto c = clipMsg; if (c == nullptr) return;
    if (state == Loading || state == Analysing) return;
    state = Analysing; progress = 0; error.clear(); if (onChange) onChange();
    pool.addJob ([this, c]
    {
        auto res = std::make_shared<RefAnalysis>();
        const bool done = analyzeRef (*c, *res, &stopFlag, [this] (float p) { progress = p; });
        if (stopFlag || ! done) return;
        post ([this, res] { analysisMsg = res; state = Ready; progress = 1; if (onChange) onChange(); });
    });
}

void RefEngine::extractDna (double t0, double t1, int role, std::function<void (bool, const TrackDna&)> done)
{
    auto c = clipMsg; if (c == nullptr) { done (false, {}); return; }
    pool.addJob ([this, c, t0, t1, role, done]
    {
        TrackDna d; const bool ok = ::hg::extractDna (*c, t0, t1, role, d, &stopFlag);
        if (stopFlag) return;
        post ([done, ok, d] { done (ok, d); });
    });
}

void RefEngine::render (juce::AudioBuffer<float>& out, double hostSr) noexcept
{
    if (! playing.load (std::memory_order_relaxed)) return;
    const juce::SpinLock::ScopedTryLockType tl (lock);
    if (! tl.isLocked() || rawAudio == nullptr) return;     // kilit yazarda: bu blokta sessizlik (parça atlamaz, kilit çok kısa tutulur)
    const auto& a = rawAudio->audio; const int n = a.getNumSamples(), nc = a.getNumChannels(); if (n < 2 || hostSr <= 0) return;
    const double sr = rawAudio->sampleRate, ratio = sr / hostSr;
    double s0 = 0, s1 = (double) n; const double rs = regionStart.load(), re = regionEnd.load();
    if (re > rs + 0.01) { s0 = std::max (0.0, rs * sr); s1 = std::min ((double) n - 1, re * sr); }
    const double sk = seekTo.exchange (-1.0); if (sk >= 0) rpos = std::clamp (sk * sr, 0.0, (double) n - 1);
    if (rpos < s0 || rpos >= s1) rpos = s0;
    const float g = gain.load(), *L = a.getReadPointer (0), *R = a.getReadPointer (nc > 1 ? 1 : 0);
    const bool lp = loop.load(); const int nOut = out.getNumSamples(), oc = out.getNumChannels();
    float* oL = out.getWritePointer (0); float* oR = oc > 1 ? out.getWritePointer (1) : nullptr;
    auto lim = [] (float x) { const float ax = std::abs (x); return ax <= 0.5f ? x : std::copysign (0.5f + 0.5f * std::tanh ((ax - 0.5f) * 2.0f), x); };   // CPU'da ucuz yumuşak sınır
    for (int i = 0; i < nOut; ++i)
    {
        if (rpos >= s1) { if (lp) rpos = s0; else { playing.store (false); break; } }
        const int k = (int) rpos; const float f = (float) (rpos - k); const int k1 = std::min (k + 1, n - 1);
        const float l = (L[k] + (L[k1] - L[k]) * f) * g, r = (R[k] + (R[k1] - R[k]) * f) * g;
        oL[i] = std::clamp (oL[i] + lim (l), -0.891f, 0.891f); if (oR) oR[i] = std::clamp (oR[i] + lim (r), -0.891f, 0.891f);
        rpos += ratio;
    }
    posSec.store (rpos / sr, std::memory_order_relaxed);
}

void RefEngine::samplerNoteOn (int note, int vel, int delay) noexcept
{
    const int mv = std::clamp (sampler.maxVoices.load(), 1, kMaxVoices);
    int idx = -1, active = 0; unsigned long long oldest = ~0ull; int oi = 0;
    for (int i = 0; i < kMaxVoices; ++i) { if (voices[(size_t) i].on) { ++active; if (voices[(size_t) i].age < oldest) { oldest = voices[(size_t) i].age; oi = i; } } else if (idx < 0) idx = i; }
    if (idx < 0 || active >= mv) idx = oi;                        // en eski sesi çal (voice stealing)
    auto& v = voices[(size_t) idx]; v = Voice(); v.on = true; v.held = true; v.note = note; v.delay = std::max (0, delay); v.vel = std::clamp (vel, 1, 127) / 127.0f; v.age = ++voiceClock; v.pos = -1;
}
void RefEngine::samplerNoteOff (int note) noexcept { for (auto& v : voices) if (v.on && v.held && v.note == note) { v.held = false; if (v.stage < 3) v.stage = 3; } }
void RefEngine::samplerAllOff() noexcept { for (auto& v : voices) v = Voice(); }

void RefEngine::renderSampler (juce::AudioBuffer<float>& out, double hostSr) noexcept
{
    bool any = false; for (auto& v : voices) any = any || v.on; if (! any) return;
    const juce::SpinLock::ScopedTryLockType tl (lock);
    if (! tl.isLocked() || rawAudio == nullptr || hostSr <= 0) return;
    if (voiceClip != rawAudio) { if (voiceClip != nullptr) { for (auto& v : voices) if (v.pos >= 0) v = Voice(); } voiceClip = rawAudio; }   // klip değişti: eski sesler kesilir
    const auto& a = rawAudio->audio; const int n = a.getNumSamples(), nc = a.getNumChannels(); if (n < 2) return;
    const double sr = rawAudio->sampleRate;
    double t0 = sampler.trimStart.load() * sr, t1 = sampler.trimEnd.load() * sr; if (t1 <= t0 + 16) { t0 = 0; t1 = n; }
    t0 = std::clamp (t0, 0.0, (double) n - 2); t1 = std::clamp (t1, t0 + 2, (double) n - 1);
    const bool lp = sampler.loop.load(); double ls = sampler.loopStart.load() * sr, le = sampler.loopEnd.load() * sr;
    if (le <= ls + 64) { ls = t0; le = t1; } ls = std::clamp (ls, t0, t1 - 2); le = std::clamp (le, ls + 2, t1);
    const float A = std::max (0.001f, sampler.a.load()), D = std::max (0.001f, sampler.d.load()), S = sampler.s.load(), R = std::max (0.003f, sampler.r.load());
    const float g = sampler.gain.load(), vs = sampler.velSens.load();
    const int root = sampler.root.load(), tr = sampler.transpose.load(); const double fine = sampler.fineCents.load();
    const float aInc = 1.0f / (A * (float) hostSr), dInc = (1.0f - S) / (D * (float) hostSr), rInc = 1.0f / (R * (float) hostSr);
    const float *L = a.getReadPointer (0), *Rr = a.getReadPointer (nc > 1 ? 1 : 0);
    const int nOut = out.getNumSamples(), oc = out.getNumChannels(); float* oL = out.getWritePointer (0); float* oR = oc > 1 ? out.getWritePointer (1) : nullptr;
    for (auto& v : voices)
    {
        if (! v.on) continue;
        if (v.pos < 0) { v.pos = t0; v.inc = std::pow (2.0, (v.note + tr - root) / 12.0 + fine / 1200.0) * sr / hostSr; }
        const float vg = g * (1.0f - vs * (1.0f - v.vel));
        for (int i = v.delay; i < nOut; ++i)
        {
            if (v.pos >= (lp ? le : t1) - 1)
            {
                if (lp && v.stage < 3) v.pos = ls + std::fmod (v.pos - ls, std::max (2.0, le - ls)); else { v.on = false; break; }
            }
            switch (v.stage)
            {
                case 0: v.env += aInc; if (v.env >= 1.0f) { v.env = 1.0f; v.stage = 1; } break;
                case 1: v.env -= dInc; if (v.env <= S) { v.env = S; v.stage = 2; } break;
                case 2: break;
                default: v.env -= rInc * std::max (0.05f, v.env) * 2.0f; if (v.env <= 0.0005f) { v.on = false; } break;
            }
            if (! v.on) break;
            const int k = (int) v.pos; const float f = (float) (v.pos - k); const int k1 = std::min (k + 1, n - 1);
            const float l = L[k] + (L[k1] - L[k]) * f, r = Rr[k] + (Rr[k1] - Rr[k]) * f; const float e = v.env * vg;
            oL[i] = std::clamp (oL[i] + l * e, -0.891f, 0.891f); if (oR) oR[i] = std::clamp (oR[i] + r * e, -0.891f, 0.891f);
            v.pos += v.inc;
        }
        v.delay = 0;
    }
}

juce::var RefEngine::samplerToVar() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("format", "wavejackers-sampler"); o->setProperty ("version", 1);
    { const juce::ScopedLock sl (pathLock); o->setProperty ("source", curName); o->setProperty ("sourcePath", curPath); }
    o->setProperty ("root", sampler.root.load()); o->setProperty ("transpose", sampler.transpose.load()); o->setProperty ("fineCents", sampler.fineCents.load());
    o->setProperty ("voices", sampler.maxVoices.load()); o->setProperty ("loop", sampler.loop.load());
    o->setProperty ("a", sampler.a.load()); o->setProperty ("d", sampler.d.load()); o->setProperty ("s", sampler.s.load()); o->setProperty ("r", sampler.r.load());
    o->setProperty ("gain", sampler.gain.load()); o->setProperty ("velSens", sampler.velSens.load());
    o->setProperty ("trimStart", sampler.trimStart.load()); o->setProperty ("trimEnd", sampler.trimEnd.load());
    o->setProperty ("loopStart", sampler.loopStart.load()); o->setProperty ("loopEnd", sampler.loopEnd.load());
    return juce::var (o);
}

void RefEngine::samplerFromVar (const juce::var& v)
{
    if (! v.isObject()) return;
    auto I = [&] (const char* k, int lo, int hi, int def) { return std::clamp ((int) v.getProperty (k, def), lo, hi); };
    auto F = [&] (const char* k, float lo, float hi, float def) { const double x = (double) v.getProperty (k, def); return std::isfinite (x) ? std::clamp ((float) x, lo, hi) : def; };
    auto Dd = [&] (const char* k) { const double x = (double) v.getProperty (k, 0.0); return std::isfinite (x) ? std::clamp (x, 0.0, 36000.0) : 0.0; };
    sampler.root = I ("root", 0, 127, 60); sampler.transpose = I ("transpose", -48, 48, 0); sampler.fineCents = I ("fineCents", -100, 100, 0); sampler.maxVoices = I ("voices", 1, kMaxVoices, 8);
    sampler.loop = (bool) v.getProperty ("loop", false);
    sampler.a = F ("a", 0.001f, 5.0f, 0.005f); sampler.d = F ("d", 0.001f, 5.0f, 0.25f); sampler.s = F ("s", 0.0f, 1.0f, 0.8f); sampler.r = F ("r", 0.003f, 10.0f, 0.3f);
    sampler.gain = F ("gain", 0.0f, 1.5f, 0.8f); sampler.velSens = F ("velSens", 0.0f, 1.0f, 0.7f);
    sampler.trimStart = Dd ("trimStart"); sampler.trimEnd = Dd ("trimEnd"); sampler.loopStart = Dd ("loopStart"); sampler.loopEnd = Dd ("loopEnd");
}

juce::var RefEngine::stateToVar() const
{
    auto* o = new juce::DynamicObject();
    { const juce::ScopedLock sl (pathLock); o->setProperty ("path", curPath.isNotEmpty() ? curPath : missingPath); }
    o->setProperty ("sampler", samplerToVar());
    o->setProperty ("sel0", regionStart.load()); o->setProperty ("sel1", regionEnd.load());
    return juce::var (o);
}

void RefEngine::restoreFromVar (const juce::var& v)
{
    if (! v.isObject()) return;
    const auto path = v.getProperty ("path", {}).toString(); auto samp = v.getProperty ("sampler", {});
    const double s0 = (double) v.getProperty ("sel0", 0.0), s1 = (double) v.getProperty ("sel1", 0.0);
    post ([this, path, samp, s0, s1]
    {
        samplerFromVar (samp);
        if (path.isEmpty()) return;
        const juce::File f (path);
        if (f.existsAsFile())
        {
            missingPath.clear(); load (f);
            regionStart = std::isfinite (s0) ? std::max (0.0, s0) : 0.0; regionEnd = std::isfinite (s1) ? std::max (0.0, s1) : 0.0;
        }
        else { missingPath = path; setPath (path, {}); error = "fileMissing"; state = Failed; if (onChange) onChange(); }
    });
}
}
