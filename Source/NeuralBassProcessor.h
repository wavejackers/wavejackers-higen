// Eğitilmiş küçük bir sinir ağını (RTNeural, .json ağırlıkları) gerçek zamanlı işler.
//  - Ağırlıklar prepare() aşamasında (ses iş parçacığı dışında) yüklenir; processBlock yalnızca forward() çağırır.
//  - Model yapısı (aşağıdaki şablon) eğitimdeki modelle BİREBİR aynı olmalı.
//  - Bozuk ya da uyumsuz json yüklemeyi başarısız kılar; eklenti bypass'ta kalır, çökmez.
//  - RTNeural yoksa (HITGEN_HAS_RTNEURAL=0) sınıf derlenir ama her zaman bypass eder.
#pragma once
#include <juce_core/juce_core.h>

#if HITGEN_HAS_RTNEURAL
  #include <RTNeural/RTNeural.h>
#endif

namespace hg
{
class NeuralBassProcessor
{
public:
    // jsonText: juce_add_binary_data veya dosyadan okunmuş RTNeural json'u.
    bool loadWeights (const juce::String& jsonText)
    {
        loaded = false;
#if HITGEN_HAS_RTNEURAL
        try
        {
            // RTNeural, json nesnesini kabul eder (akış değil); metni burada çözüyoruz.
            const auto parent = nlohmann::json::parse (jsonText.toStdString());
            // RTNeural uyumsuz modelde hata atmadan sessizce döner; bu yüzden mimariyi önce doğruluyoruz.
            if (! matchesArchitecture (parent)) return false;
            model.parseJson (parent);
            model.reset();
            loaded = true;
        }
        catch (...) { loaded = false; }
#else
        juce::ignoreUnused (jsonText);
#endif
        return loaded;
    }

#if HITGEN_HAS_RTNEURAL
    // Beklenen mimari: Dense(1->8) -> LSTM(8->16) -> Dense(16->1). Değiştirirsen burayı da güncelle.
    static bool matchesArchitecture (const nlohmann::json& p)
    {
        if (! p.contains ("in_shape") || ! p.contains ("layers")) return false;
        const auto& shape = p["in_shape"];
        const auto& layers = p["layers"];
        if (! shape.is_array() || shape.empty() || shape.back().get<int>() != 1) return false;
        if (! layers.is_array() || layers.size() != 3) return false;

        const char* types[] = { "dense", "lstm", "dense" };
        const int outs[] = { 8, 16, 1 };
        for (size_t i = 0; i < 3; ++i)
        {
            const auto& l = layers[i];
            if (! l.contains ("type") || l["type"] != types[i]) return false;
            if (! l.contains ("shape") || ! l["shape"].is_array() || l["shape"].empty()
                || l["shape"].back().get<int>() != outs[i]) return false;
        }

        // Ağırlık boyutları: Dense [in][out] + bias; LSTM [in][4*out], [out][4*out], bias[4*out]
        const auto& w0 = layers[0]["weights"];
        if (! w0.is_array() || w0.size() != 2) return false;
        if (w0[0].size() != 1 || w0[0][0].size() != 8 || w0[1].size() != 8) return false;

        const auto& w1 = layers[1]["weights"];
        if (! w1.is_array() || w1.size() != 3) return false;
        if (w1[0].size() != 8 || w1[0][0].size() != 64) return false;
        if (w1[1].size() != 16 || w1[1][0].size() != 64) return false;
        if (w1[2].size() != 64) return false;

        const auto& w2 = layers[2]["weights"];
        if (! w2.is_array() || w2.size() != 2) return false;
        if (w2[0].size() != 16 || w2[0][0].size() != 1 || w2[1].size() != 1) return false;
        return true;
    }
#endif

    bool isLoaded() const noexcept { return loaded; }

    void prepare() noexcept
    {
#if HITGEN_HAS_RTNEURAL
        model.reset();
#endif
    }

    // Mono işleme. Stereo için kanal başına ayrı bir örnek gerekir (durum taşıdığı için).
    void process (float* data, int numSamples) noexcept
    {
#if HITGEN_HAS_RTNEURAL
        if (! loaded) return;
        for (int i = 0; i < numSamples; ++i)
        {
            const float in[1] = { data[i] };
            data[i] = model.forward (in);
        }
#else
        juce::ignoreUnused (data, numSamples);
#endif
    }

private:
#if HITGEN_HAS_RTNEURAL
    // Örnek yapı: Dense(1->8) -> LSTM(8->16) -> Dense(16->1). Eğitim modelinizle değiştirin.
    RTNeural::ModelT<float, 1, 1,
        RTNeural::DenseT<float, 1, 8>,
        RTNeural::LSTMLayerT<float, 8, 16>,
        RTNeural::DenseT<float, 16, 1>> model;
#endif
    bool loaded = false;
};
} // namespace hg
