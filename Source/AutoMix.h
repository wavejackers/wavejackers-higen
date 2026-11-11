// Otomatik mix: pembe gürültü (pink noise) referanslı denge + gain staging.
// Yöntem (Sound On Sound "pink noise reference", Produce Like A Pro): tüm kanallar kapalıyken referans pembe gürültü sabit bir
// düzeyde çalar; her kanal gürültüyle birlikte dinlenir ve "gürültünün üstünde tam duyulur" düzeye çekilir. Burada kulak yerine
// ölçüm kullanılır: gürültünün ve kanalın gücü kritik bantlarda (ERB ölçeği) karşılaştırılır, kanal en güçlü bantlarında
// gürültüden kTargetAudDb kadar yukarıda olacak şekilde ayarlanır (maskeleme: duyulabilirlik bant içi sinyal/gürültü oranına bağlıdır).
// Sonra tür odaklı küçük bir denge tablosu eklenir ve tüm kanallara ortak bir kazanç uygulanarak mix tepe değeri -6 dBFS'e oturtulur
// (master girişinde 3-6 dB boşluk). Fader'lar 0 dB'de kalır; kazanç örneklerin içine yazılır (clip gain / trim).
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <map>
#include "Arrangement.h"

namespace hg
{
constexpr double kRefNoiseRmsDb = -23.0;   // referans gürültü: RMS -23 dBFS (SOS: mdaTestTone -11 dB ~ RMS -23 dBFS)
constexpr double kTargetAudDb   = 3.0;     // "tam duyulur": en güçlü bantlarda gürültünün +3 dB üstü
constexpr double kMixPeakDb     = -6.0;    // mix tepe hedefi (gain staging: master girişinde -6 dBFS)

// Pembe gürültü (-3 dB/oktav), belirlenimci; stereo, iki kanal ilişkisiz
juce::AudioBuffer<float> pinkNoise (double sr, double seconds, double rmsDb, unsigned seed = 1);

struct MixLane
{
    juce::String role, name;
    double offsetDb = 0;     // tür dengesi (kick biraz önde, pad geride...)
    double audDb = -999;     // gürültüye göre duyulabilirlik, kazançsız (en güçlü 3 bantta ortalama)
    double gainDb = 0;       // örneğe uygulanacak toplam kazanç
    double peakDb = -999, rmsDb = -999;   // kazançlı, referans bölümünde
    bool active = false;
};
struct MixResult
{
    std::vector<MixLane> lanes;
    double commonDb = 0, mixPeakDb = -999, mixRmsDb = -999;
    int fromBar = 0, bars = 0;   // referans bölüm (en yüksek yoğunluklu ilk drop/groove)
    double gainFor (const juce::String& role) const { for (auto& l : lanes) if (l.role == role) return l.gainDb; return 0; }
};

// oneShots: şerit rolü -> örnek (melodik: C5 kökte tek nota; davul: tek vuruş). Referans bölüm örnekleyici modeliyle çalınır.
// excerpt verilirse dengelenmiş referans bölüm mix'i (stereo) oraya yazılır.
MixResult autoMix (const Arrangement& a, const std::map<juce::String, juce::AudioBuffer<float>>& oneShots, double sr,
                   juce::AudioBuffer<float>* excerpt = nullptr);

// FX örneği (riser, impact...) için kazanç: gürültüye göre aynı ölçüt + tür ofseti; tepe -3 dBFS'i aşmaz
double fxGainDb (const juce::String& cueType, const juce::AudioBuffer<float>& b, double sr, const MixResult& m);

// Okunur rapor (Mix Report.txt)
juce::String mixReport (const MixResult& m, const Arrangement& a);

// Tür dengesi tablosu (dB); testler de kullanır
double roleOffsetDb (const juce::String& role);
double fxOffsetDb (const juce::String& cueType);
}
