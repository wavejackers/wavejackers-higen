// Çıkış ses yüksekliği: ITU-R BS.1770 K-ağırlıklı ölçüm, hedef (varsayılan -14 LUFS) yönünde yavaş otomatik kazanç
// ve -1 dBFS tavanlı ileri bakışlı (lookahead) tepe sınırlayıcı.
//  - Ölçüm, 400 ms'lik pencerelerin %75 örtüşmeyle (100 ms adım) son ~12 saniyesi üzerinden, BS.1770'teki gibi
//    -70 LUFS mutlak ve -10 LU göreli kapılarla yapılır ("kayan entegre" ses yüksekliği).
//  - Kazanç sessizlikte artırılmaz (kuyruklar ve duraklamalar yükseltilmez); değişim hızı saniyede birkaç dB ile sınırlıdır.
//  - Sınırlayıcı örnekler arası tepeyi de izler: 4x aşırı örnekleme (12 dokunuşlu pencereli sinc, BS.1770 gerçek tepe yöntemine benzer);
//    tavan -1.1 dB, gerçek tepe hedefi -1 dBTP. Testlerde ffmpeg EBU R128 ile bağımsız olarak doğrulanır.
// Ses iş parçacığında bellek ayırmaz; tüm tamponlar prepare() içinde hazırlanır.
#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <vector>

namespace hg
{
class LoudnessMeter
{
public:
    void prepare (double sampleRate);
    void reset();
    // Bir bloğu ölçer (L/R; R nullptr ise mono). gain: ölçülen sinyale uygulanacak doğrusal çarpan.
    void add (const float* L, const float* R, int n, float gain = 1.0f) noexcept;
    // Kayan entegre ses yüksekliği (LUFS); yeterli veri yoksa -70.
    float integrated() const noexcept { return integ; }
    float momentary() const noexcept { return mom; }
    bool hasData() const noexcept { return integ > -69.0f; }

private:
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1[2] {}, z2[2] {};
        inline double run (int c, double x) noexcept { const double y = b0 * x + z1[c]; z1[c] = b1 * x - a1 * y + z2[c]; z2[c] = b2 * x - a2 * y; return y; }
    };
    Biquad shelf, hp;
    double sr = 48000, acc = 0; int accN = 0, subLen = 4800;
    std::array<double, 4> sub {}; int subPos = 0, subCount = 0;
    static constexpr int kHist = 120;                      // 120 x 100 ms = 12 s
    std::array<double, kHist> hist {}; int histPos = 0, histCount = 0;
    float integ = -70.0f, mom = -70.0f;
    void blockDone() noexcept;
};

class LoudnessNormalizer
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    // Bloğu yerinde işler: kazanç -> sınırlayıcı -> masterPost. masterPre (>1 ise) sınırlayıcıya sürülür.
    // Dönen değer: blok tepe değeri (mutlak).
    float process (float* L, float* R, int n, float masterPre, float masterPost) noexcept;
    float target = -14.0f;
    static constexpr float kCeiling = 0.88f;               // -1.1 dB: 4x aşırı örneklenmiş tepe tahminine uygulanır (gerçek tepe hedefi -1 dBTP)
    std::atomic<float> outLufs { -70.0f }, gainDb { 0.0f };

private:
    LoudnessMeter inMeter, outMeter;
    double sr = 48000; int la = 64;                         // ileri bakış (örnek)
    float g = 1.0f, gtLin = 1.0f, gDbCur = 0.0f, trimDb = 0.0f; int sinceUpdate = 0, upd = 4800;
    std::vector<float> dlyL, dlyR, req; int dpos = 0;       // gecikme hattı ve gerekli kazanç geçmişi
    float limG = 1.0f, relCoef = 0.999f, atkCoef = 0.9f;
    float holdMin = 1.0f; int holdLeft = 0;
    static constexpr int kTp = 12;
    float hl[kTp] {}, hr[kTp] {}; float tpc[3][kTp] {};      // son 12 örnek ve 1/4, 1/2, 3/4 ara değer katsayıları
    void updateGain() noexcept;
};
}
