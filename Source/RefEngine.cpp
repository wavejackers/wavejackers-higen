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
    cancel(); playing = false; setClip (nullptr); clipMsg.reset(); analysisMsg.reset(); error.clear(); state = Idle; regionStart = regionEnd = 0; posSec = 0; seekTo = 0;
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
            if (c == nullptr) { error = err.isEmpty() ? "fileBad" : err; state = Failed; clipMsg.reset(); setClip (nullptr); }
            else { setClip (std::const_pointer_cast<RefClip> (c)); clipMsg = c; state = Loaded; regionStart = regionEnd = 0; posSec = 0; seekTo = 0; }
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
}
