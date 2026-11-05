// Asimetrik saturasyon (alt bas için). Eğri bias ile kaydırılır: tanh(k(x+b)) - tanh(k b).
// Simetrik bir eğri yalnızca tek harmonik üretir; bias 2. harmoniği (ve dolayısıyla "dolgun" alt
// bası) ekler. Bias DC kaydırır, bu yüzden çıkışta DC blocker vardır.
// Öneri: sadece düşük bantta uygulayın (ör. ~150 Hz LR4 crossover), bası mono tutun.
// Aliasing için bu sınıfı juce::dsp::Oversampling (2x) içinde çalıştırmak daha güvenlidir.
#pragma once
#include <cmath>

namespace hg
{
struct AsymmetricSaturator
{
    float drive = 2.0f;   // 1..8
    float bias  = 0.25f;  // 0 = simetrik, 0.2-0.35 = belirgin 2. harmonik
    float mix   = 1.0f;   // 0..1

    float processSample (float x) noexcept
    {
        const float y   = std::tanh (drive * (x + bias)) - std::tanh (drive * bias);
        const float out = y / std::tanh (drive * (1.0f + bias)); // kabaca seviye telafisi
        const float dc  = out - xPrev + 0.9990f * yPrev;         // ~10 Hz DC blocker
        xPrev = out;
        yPrev = dc;
        return x + mix * (dc - x);
    }

    void reset() noexcept { xPrev = yPrev = 0.0f; }

private:
    float xPrev = 0.0f, yPrev = 0.0f;
};
} // namespace hg
