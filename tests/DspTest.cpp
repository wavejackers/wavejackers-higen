// DSP testleri: saturasyon harmonikleri, DC kaybı, sınırlılık; LPF kazancı/gecikmesi; NN yükleme güvenliği.
#include "../Source/AsymmetricSaturator.h"
#include "../Source/AntiAliasedLowPass.h"
#include "../Source/NeuralBassProcessor.h"
#include <cstdio>
#include <cmath>
#include <string>
#if HITGEN_HAS_RTNEURAL
  #include <RTNeural/RTNeural.h>   // nlohmann::json da burada gelir
#endif

using namespace hg;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf ("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

// Tek bir frekansın genliği (Goertzel-benzeri, tam periyot sayısı üzerinden)
static double toneAmp (const std::vector<float>& y, double fs, double f, size_t from)
{
    double re = 0, im = 0; size_t n = y.size() - from;
    for (size_t i = from; i < y.size(); ++i)
    {
        double ph = 2.0 * M_PI * f * (double) (i - from) / fs;
        re += y[i] * std::cos (ph); im += y[i] * std::sin (ph);
    }
    return 2.0 * std::sqrt (re * re + im * im) / (double) n;
}

static void testSaturator()
{
    const double fs = 48000.0; const int N = 48000 * 2; const size_t skip = 48000;
    std::vector<float> x (N), ySym (N), yAsym (N);
    for (int i = 0; i < N; ++i) x[i] = 0.5f * (float) std::sin (2.0 * M_PI * 50.0 * i / fs);

    AsymmetricSaturator sym; sym.bias = 0.0f; sym.drive = 2.0f;
    AsymmetricSaturator asy; asy.bias = 0.25f; asy.drive = 2.0f;
    for (int i = 0; i < N; ++i) { ySym[i] = sym.processSample (x[i]); yAsym[i] = asy.processSample (x[i]); }

    const double f1s = toneAmp (ySym, fs, 50, skip), h2s = toneAmp (ySym, fs, 100, skip);
    const double f1a = toneAmp (yAsym, fs, 50, skip), h2a = toneAmp (yAsym, fs, 100, skip);
    std::printf ("simetrik: 2.harm/temel = %.4f ; asimetrik: 2.harm/temel = %.4f\n", h2s / f1s, h2a / f1a);
    CHECK (h2s / f1s < 0.01, "simetrik eğri çift harmonik üretmemeli");
    CHECK (h2a / f1a > 0.05, "asimetrik eğri belirgin 2. harmonik üretmeli (%.4f)", h2a / f1a);

    double mean = 0, peak = 0;
    for (size_t i = skip; i < yAsym.size(); ++i) { mean += yAsym[i]; peak = std::max (peak, (double) std::fabs (yAsym[i])); }
    mean /= (double) (yAsym.size() - skip);
    CHECK (std::fabs (mean) < 0.005, "DC blocker çalışmalı (ortalama %.6f)", mean);
    CHECK (peak < 2.0, "çıkış sınırlı olmalı (tepe %.3f)", peak);
    for (float v : yAsym) if (! std::isfinite (v)) { ++fails; std::printf ("FAIL NaN/inf\n"); break; }
}

static void testLowPass()
{
    const double fs = 48000.0; const int block = 512;
    juce::dsp::ProcessSpec spec { fs, (juce::uint32) block, 1 };
    AntiAliasedLowPass lp; lp.prepare (spec); lp.setCutoff (2000.0f, 0.707f);
    const int lat = lp.getLatencySamples();
    CHECK (lat >= 0 && lat < 512, "gecikme makul olmalı (%d)", lat);

    auto run = [&] (double f) {
        lp.reset(); const int N = 48000;
        std::vector<float> y; y.reserve (N);
        juce::AudioBuffer<float> buf (1, block);
        for (int start = 0; start < N; start += block)
        {
            for (int i = 0; i < block; ++i) buf.setSample (0, i, 0.5f * (float) std::sin (2.0 * M_PI * f * (start + i) / fs));
            lp.process (buf);
            for (int i = 0; i < block; ++i) y.push_back (buf.getSample (0, i));
        }
        return toneAmp (y, fs, f, 16000);
    };
    const double pass = run (300.0), stop = run (10000.0);
    const double dbPass = 20.0 * std::log10 (pass / 0.5), dbStop = 20.0 * std::log10 (stop / 0.5);
    std::printf ("LPF 300 Hz: %.2f dB ; 10 kHz: %.2f dB\n", dbPass, dbStop);
    CHECK (std::fabs (dbPass) < 1.0, "geçiş bandı düz olmalı (%.2f dB)", dbPass);
    // 2 kutuplu (12 dB/okt) filtre: 2 kHz kesimde 10 kHz ~2,3 oktav üstünde, beklenen ~ -28 dB.
    CHECK (dbStop < -25.0, "kesim üstü 2 kutuplu eğimle en az -25 dB olmalı (%.2f dB)", dbStop);
}

#if HITGEN_HAS_RTNEURAL
// Sıfır ağırlıklı LSTM + bias 0.25'li Dense: çıkış her girişte tam 0.25 olmalı (kesin beklenen değer).
static nlohmann::json grid (int rows, int cols, double v)
{
    nlohmann::json m = nlohmann::json::array();
    for (int r = 0; r < rows; ++r)
    {
        nlohmann::json row = nlohmann::json::array();
        for (int c = 0; c < cols; ++c) row.push_back (v);
        m.push_back (row);
    }
    return m;
}

static std::string makeKnownModelJson (int denseInSize)
{
    using nlohmann::json;
    json dense1 = { { "type", "dense" }, { "activation", "" }, { "shape", json::array ({ 1, 8 }) }, { "weights", json::array() } };
    dense1["weights"].push_back (grid (denseInSize, 8, 0.5));  // W: in x out
    dense1["weights"].push_back (json (std::vector<double> (8, 0.0)));   // bias: düz 8 değer
    json lstm = { { "type", "lstm" }, { "shape", json::array ({ 1, 16 }) }, { "weights", json::array() } };
    lstm["weights"].push_back (grid (8, 64, 0.0));             // kernel: in x 4*out
    lstm["weights"].push_back (grid (16, 64, 0.0));            // recurrent: out x 4*out
    lstm["weights"].push_back (json (std::vector<double> (64, 0.0)));
    json dense2 = { { "type", "dense" }, { "activation", "" }, { "shape", json::array ({ 1, 1 }) }, { "weights", json::array() } };
    dense2["weights"].push_back (grid (16, 1, 0.0));
    dense2["weights"].push_back (json (std::vector<double> { 0.25 }));
    json root = { { "in_shape", json::array ({ 1, 1, 1 }) }, { "layers", json::array() } };
    root["layers"].push_back (dense1);
    root["layers"].push_back (lstm);
    root["layers"].push_back (dense2);
    return root.dump();
}
#endif

static void testNeuralSafety()
{
    NeuralBassProcessor nn; nn.prepare();
    CHECK (! nn.loadWeights ("{ bozuk json"), "bozuk json reddedilmeli");
    CHECK (! nn.isLoaded(), "yükleme başarısızsa bypass kalmalı");
    float buf[4] = { 0.1f, -0.2f, 0.3f, -0.4f };
    nn.process (buf, 4);
    CHECK (buf[0] == 0.1f && buf[3] == -0.4f, "yüklü değilken işleme bypass olmalı");
#if HITGEN_HAS_RTNEURAL
    CHECK (nn.loadWeights (juce::String (makeKnownModelJson (1))), "bilinen model yüklenmeli");
    CHECK (nn.isLoaded(), "yüklendikten sonra isLoaded true olmalı");
    float y[6] = { -1.0f, -0.3f, 0.0f, 0.4f, 0.9f, 1.0f };
    nn.prepare();
    nn.process (y, 6);
    for (float v : y) CHECK (std::fabs (v - 0.25f) < 1e-4f, "sıfır LSTM + bias 0.25 çıkışı 0.25 olmalı (%.6f)", v);
    NeuralBassProcessor bad;
    CHECK (! bad.loadWeights (juce::String (makeKnownModelJson (2))), "boyut uyuşmazlığı reddedilmeli");
#endif
}

int main()
{
    testSaturator();
    testLowPass();
    testNeuralSafety();
    std::printf (fails ? "DSP testleri BAŞARISIZ (%d)\n" : "DSP testleri geçti\n", fails);
    return fails ? 1 : 0;
}
