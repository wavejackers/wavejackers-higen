// Açılış sesi (efekt + seslendirme): ileti iş parçacığında bir kez çözülür, ses iş parçacığında yalnızca okunur.
// Güvenlik: DAW çalarken, çevrimdışı dışa aktarımda ya da kullanıcı atladığında hemen (20 ms) susar; hiçbir zaman kayda girmez.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>

namespace hg
{
class IntroSound
{
public:
    // ileti iş parçacığı: veriyi çöz ve çalmayı iste (ilk çağrıda bellek ayrılır; sonra yeniden kullanılır)
    bool start (const void* data, size_t size)
    {
        if (! ready.load())
        {
            juce::OggVorbisAudioFormat fmt;
            std::unique_ptr<juce::AudioFormatReader> r (fmt.createReaderFor (new juce::MemoryInputStream (data, size, false), true));
            if (r == nullptr || r->lengthInSamples <= 0) return false;
            buf.setSize (2, (int) r->lengthInSamples);
            r->read (&buf, 0, (int) r->lengthInSamples, 0, true, true);
            if (r->numChannels == 1) buf.copyFrom (1, 0, buf, 0, 0, buf.getNumSamples());
            srcRate = r->sampleRate;
            ready.store (true, std::memory_order_release);
        }
        stopReq.store (false); startReq.store (true, std::memory_order_release);
        return true;
    }
    void stop() { stopReq.store (true); }
    bool playing() const { return active.load(); }

    // ses iş parçacığı: çıkışa ekle (bellek ayırma ve kilit yok)
    void process (juce::AudioBuffer<float>& out, double hostRate, bool mustBeSilent) noexcept
    {
        if (startReq.exchange (false, std::memory_order_acquire) && ready.load (std::memory_order_acquire)) { pos = 0.0; fade = 1.0f; active.store (true); }
        if (! active.load()) return;
        const int n = out.getNumSamples(), len = buf.getNumSamples();
        const double step = srcRate / juce::jmax (1.0, hostRate);
        const float fadeStep = 1.0f / (float) juce::jmax (1.0, hostRate * 0.02);
        const bool quiet = mustBeSilent || stopReq.load();
        for (int i = 0; i < n; ++i)
        {
            if (quiet) { fade -= fadeStep; if (fade <= 0.0f) { active.store (false); return; } }
            const int i0 = (int) pos; if (i0 + 1 >= len) { active.store (false); return; }
            const float fr = (float) (pos - i0), g = gain * fade;
            for (int c = 0; c < juce::jmin (2, out.getNumChannels()); ++c)
            {
                const float* s = buf.getReadPointer (c);
                out.addSample (c, i, g * (s[i0] + fr * (s[i0 + 1] - s[i0])));
            }
            pos += step;
        }
    }

private:
    juce::AudioBuffer<float> buf; double srcRate = 44100.0, pos = 0.0;
    std::atomic<bool> ready { false }, startReq { false }, stopReq { false }, active { false };
    float fade = 1.0f; const float gain = 0.6f;   // kaynak tepe -3 dBFS -> çıkışta yaklaşık -7.5 dBFS
};
}
