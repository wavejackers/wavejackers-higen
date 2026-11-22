// Ses tasarımı ön dinlemesi: çevrimdışı işlenmiş bir klibi (ör. metinden üretilen ses) eklentinin çıkışına karıştırır.
// Mesaj iş parçacığı yeni tamponu verir; ses iş parçacığı kilitsiz alır ve eskisini çöp kutusuna koyar (bellek ayırmaz).
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

class ClipPlayer
{
public:
    ~ClipPlayer() { delete pending.exchange (nullptr); delete live; delete pendingFree; for (auto& t : trash) delete t.exchange (nullptr); }
    // ---- mesaj iş parçacığı
    void play (std::unique_ptr<juce::AudioBuffer<float>> clip, bool loop)   // yeni klip baştan çalar
    {
        looping.store (loop); restart.store (true);
        delete pending.exchange (clip.release());
        playing.store (true);
    }
    void stop() noexcept { playing.store (false); }
    bool isPlaying() const noexcept { return playing.load(); }
    float position() const noexcept { return pos01.load(); }              // 0..1 (çalma imleci)
    void collect() { for (auto& t : trash) delete t.exchange (nullptr); }
    // ---- ses iş parçacığı
    void process (juce::AudioBuffer<float>& out, int n) noexcept
    {
        if (auto* p = pending.exchange (nullptr))
        {
            if (live != nullptr) { bool put = false; for (auto& t : trash) { Buf* e = nullptr; if (t.compare_exchange_strong (e, live)) { put = true; break; } } if (! put) pendingFree = live; }
            live = p; at = 0;
        }
        if (restart.exchange (false)) at = 0;
        if (! playing.load() || live == nullptr || live->getNumSamples() == 0) return;
        const int len = live->getNumSamples(), chs = live->getNumChannels();
        for (int i = 0; i < n; ++i)
        {
            if (at >= len) { if (looping.load()) at = 0; else { playing.store (false); break; } }
            for (int c = 0; c < out.getNumChannels(); ++c) out.addSample (c, i, live->getSample (juce::jmin (c, chs - 1), at));
            ++at;
        }
        pos01.store ((float) at / (float) len);
    }
private:
    using Buf = juce::AudioBuffer<float>;
    std::atomic<Buf*> pending { nullptr }; Buf* live = nullptr; Buf* pendingFree = nullptr; std::array<std::atomic<Buf*>, 8> trash {};
    std::atomic<bool> playing { false }, looping { true }, restart { false }; std::atomic<float> pos01 { 0 };
    int at = 0;
};
