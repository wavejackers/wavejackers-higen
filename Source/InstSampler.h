// Kullanıcının kendi sesi (akor, bas ya da melodi için): bir ses dosyası, notaya göre perdesi kaydırılarak çalınır.
// Örnek değişimi kilitsizdir: ileti iş parçacığı yeni örneği bekleyen işaretçiye koyar, ses iş parçacığı blok başında alır,
// eskisini çöp kuyruğuna bırakır; ileti iş parçacığı collectGarbage() ile siler. Ses iş parçacığında bellek ayrılmaz.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

namespace hg
{
struct InstSample
{
    juce::AudioBuffer<float> buf;   // 1 ya da 2 kanal, tepe 0.9
    double rate = 44100; int root = 60; juce::String path, name;
};

class InstSampler
{
public:
    ~InstSampler() { delete pending.exchange (nullptr); collectGarbage(); if (cur != &clearSentinel) delete cur; }
    void prepare (double sampleRate) noexcept { sr = sampleRate; }
    void setSample (std::unique_ptr<InstSample> s)                 // ileti iş parçacığı; nullptr: sentezleyiciye dön
    {
        InstSample* raw = s != nullptr ? s.release() : &clearSentinel;
        auto* old = pending.exchange (raw);
        if (old != nullptr && old != &clearSentinel) delete old;
        loaded.store (raw != &clearSentinel);
    }
    void collectGarbage()                                           // ileti iş parçacığı
    {
        if (auto* t = trash.exchange (nullptr)) if (t != &clearSentinel) delete t;
    }
    std::atomic<bool> loaded { false };                             // arayüz için

    // ---- ses iş parçacığı
    bool active() const noexcept { return cur != nullptr && cur != &clearSentinel; }
    void swapIfPending() noexcept
    {
        if (pending.load (std::memory_order_acquire) == nullptr || trash.load() != nullptr) return;   // çöp boşalmadan değiştirme
        auto* nw = pending.exchange (nullptr, std::memory_order_acq_rel);
        for (auto& v : voices) v.on = false;
        if (cur != nullptr) trash.store (cur);
        cur = nw == &clearSentinel ? nullptr : nw;
    }
    void noteOn (int note, int vel, double durSec) noexcept
    {
        if (! active()) return;
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.on) { v = &x; break; }
        if (v == nullptr) { v = &voices[0]; for (auto& x : voices) if (x.age > v->age) v = &x; }   // en eskiyi çal
        v->on = true; v->note = note; v->pos = 0.0; v->age = 0;
        v->step = cur->rate / sr * std::pow (2.0, (note - cur->root) / 12.0);
        v->gain = 0.75f * std::pow (juce::jlimit (1, 127, vel) / 127.0f, 1.3f);
        v->env = 0.0f; v->released = false; v->gate = durSec > 0 ? (int) (durSec * sr) : -1;
        for (auto& x : voices) ++x.age;
    }
    void noteOff (int note) noexcept { for (auto& v : voices) if (v.on && v.note == note && ! v.released) v.released = true; }
    void allOff() noexcept { for (auto& v : voices) if (v.on) v.released = true; }
    void reset() noexcept { for (auto& v : voices) v.on = false; }
    void render (float* L, float* R, int n) noexcept               // tampona ekler
    {
        if (! active()) return;
        const auto& b = cur->buf; const int len = b.getNumSamples(), nc = b.getNumChannels();
        const float* s0 = b.getReadPointer (0); const float* s1 = b.getReadPointer (nc > 1 ? 1 : 0);
        const float atk = 1.0f / (float) (0.003 * sr), rel = 1.0f / (float) (0.12 * sr);
        for (auto& v : voices)
        {
            if (! v.on) continue;
            for (int i = 0; i < n; ++i)
            {
                if (v.gate >= 0 && --v.gate < 0) v.released = true;
                if (v.released) { v.env -= rel; if (v.env <= 0.0f) { v.on = false; break; } }
                else if (v.env < 1.0f) v.env = juce::jmin (1.0f, v.env + atk);
                const int i0 = (int) v.pos; if (i0 + 1 >= len) { v.on = false; break; }
                const float fr = (float) (v.pos - i0), g = v.gain * v.env;
                L[i] += g * (s0[i0] + fr * (s0[i0 + 1] - s0[i0]));
                R[i] += g * (s1[i0] + fr * (s1[i0 + 1] - s1[i0]));
                v.pos += v.step;
            }
        }
    }

private:
    struct Voice { bool on = false, released = false; int note = 0, gate = -1, age = 0; double pos = 0, step = 1; float gain = 0, env = 0; };
    std::array<Voice, 24> voices {};
    double sr = 44100;
    InstSample* cur = nullptr;
    InstSample clearSentinel;
    std::atomic<InstSample*> pending { nullptr }, trash { nullptr };
};

// Basit perde bulucu (normalize özilinti, 50-1500 Hz): bulunamazsa -1. Kök notayı tahmin etmek için.
inline int detectRootNote (const juce::AudioBuffer<float>& b, double rate)
{
    const int start = juce::jmin (b.getNumSamples() - 1, (int) (rate * 0.03)), len = juce::jmin (b.getNumSamples() - start, (int) (rate * 0.25));
    if (len < 512) return -1;
    const float* x = b.getReadPointer (0) + start;
    const int minLag = (int) (rate / 1500.0), maxLag = juce::jmin (len / 2, (int) (rate / 50.0));
    std::vector<double> c ((size_t) (maxLag + 2), 0.0);
    double best = 0;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double num = 0, e1 = 0, e2 = 0;
        for (int i = 0; i + lag < len; ++i) { num += x[i] * x[i + lag]; e1 += x[i] * x[i]; e2 += x[i + lag] * x[i + lag]; }
        c[(size_t) lag] = num / (std::sqrt (e1 * e2) + 1e-12); best = std::max (best, c[(size_t) lag]);
    }
    if (best < 0.6) return -1;
    // oktav hatasına karşı: en iyiye yakın (%92) ilk yerel tepe = temel periyot
    int bestLag = -1;
    for (int lag = minLag + 1; lag < maxLag; ++lag)
        if (c[(size_t) lag] >= best * 0.92 && c[(size_t) lag] >= c[(size_t) lag - 1] && c[(size_t) lag] >= c[(size_t) lag + 1]) { bestLag = lag; break; }
    if (bestLag < 0) return -1;
    // parabolik ara değer: daha hassas periyot
    const double y0 = c[(size_t) bestLag - 1], y1 = c[(size_t) bestLag], y2 = c[(size_t) bestLag + 1], den = y0 - 2 * y1 + y2;
    const double lagF = bestLag + (std::abs (den) > 1e-12 ? 0.5 * (y0 - y2) / den : 0.0);
    const double f = rate / lagF;
    return juce::jlimit (12, 108, (int) std::lround (69.0 + 12.0 * std::log2 (f / 440.0)));
}
}
