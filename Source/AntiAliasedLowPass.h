// Aliasing'i azaltılmış low-pass: 4x oversampling (polyphase IIR yarı-bant) içinde TPT/SVF filtre.
// Doğrusal bir low-pass tek başına aliasing üretmez; bozulma rezonans/drive gibi doğrusal olmayan
// adımlarda ve kesim frekansı Nyquist'e yaklaştığında ortaya çıkar. Filtreyi yüksek hızda çalıştırmak
// bu bozulmayı ses bandının dışına taşır. Gecikme getLatencySamples() ile eklentiye bildirilmeli.
// Ses iş parçacığında bellek ayırmaz: tamponlar prepare() içinde hazırlanır.
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <memory>

namespace hg
{
class AntiAliasedLowPass
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec, int factorLog2 = 2) // 2 => 4x
    {
        os = std::make_unique<juce::dsp::Oversampling<float>> (
            spec.numChannels, factorLog2,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
        os->initProcessing (spec.maximumBlockSize);

        auto osSpec = spec;
        osSpec.sampleRate       *= (double) os->getOversamplingFactor();
        osSpec.maximumBlockSize *= (juce::uint32) os->getOversamplingFactor();
        filter.prepare (osSpec);
        filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
        setCutoff (cutoffHz, q);
    }

    void setCutoff (float hz, float resonanceQ = 0.707f)
    {
        cutoffHz = hz;
        q = resonanceQ;
        filter.setCutoffFrequency (cutoffHz);
        filter.setResonance (q);
    }

    void reset()
    {
        if (os) os->reset();
        filter.reset();
    }

    int getLatencySamples() const { return os ? (int) std::ceil (os->getLatencyInSamples()) : 0; }

    // buffer: prepare()'de verilen kanal sayısıyla aynı olmalı.
    void process (juce::AudioBuffer<float>& buffer)
    {
        if (! os) return;
        juce::dsp::AudioBlock<float> block (buffer);
        auto up = os->processSamplesUp (block);
        juce::dsp::ProcessContextReplacing<float> ctx (up);
        filter.process (ctx);
        os->processSamplesDown (block);
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::dsp::StateVariableTPTFilter<float> filter;
    float cutoffHz = 8000.0f, q = 0.707f;
};
} // namespace hg
