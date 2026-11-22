// Metinden ses: kullanıcının yazdığı tarifi (Türkçe ya da İngilizce) Synth X programına çevirir. Harici servis yoktur.
//  1) Tarif çözümlenir: tür (bas, pad, akor, lead, pluck, çan), müzik tarzı, BPM, ton, süre ve sıfatlar
//     ("sıcak", "derin", "vurucu", "hafif analog distortion", "ritmik LFO", "mono alt", "kick'e yer"...).
//     Şiddet ("hafif", "çok") ve olumsuzluk ("reverb olmasın", "without distortion") anlaşılır.
//  2) Yazılmayanlar bağlama göre tamamlanır: mevcut BPM ve ton, seçili parti, tarzın tipik ayarları.
//  3) Tasarım (Design) osilatör, filtre, zarf, efekt ve modülasyon ayarlarına dönüştürülür; ses düzeyi otomatik ayarlanır.
//  4) Ön dinleme için tarza ve türe uygun kısa bir cümle (ton ve tempoda) çevrimdışı işlenir; WAV olarak kaydedilebilir.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <vector>

namespace hg { class Data; }

namespace sp
{
enum Kind { kBass, kPad, kChord, kLead, kPluck, kBell, kNumKinds };
constexpr int kNumRates = 8;
juce::String rateName (int i);                 // 1/1 1/2 1/4 1/8 1/8T 1/16 1/16T 1/32
int rateLfoIndex (int i);                      // Synth X LFO senkron dizini
int rateArpDiv (int i);                        // Synth X arpej bölümü

struct Context { double bpm = 124; int key = 9; bool minor = true; int part = 1; int bars = 4; juce::String lang = "en"; };

struct Design
{
    Kind kind = kBass; int part = 1; int arch = 0; juce::String genre;           // arch: türün içindeki ses mimarisi
    double bpm = 124; int key = 9; bool minor = true; int bars = 4; uint32_t seed = 1;
    // ses karakteri (0..1; bright -1..1)
    float bright = 0, warm = 0, deep = 0, punch = 0, shortness = 0, soft = 0, wide = 0, fat = 0, metal = 0;
    float dirt = 0, space = 0, delay = 0, side = 0, glide = 0, mono = 0;   // mono: Hz (0 kapalı)
    int dirtMode = 0;                                                       // 0 yumuşak, 2 analog, 9 tüp, 5 sert
    // basit denetimler
    bool lfo = false, arp = false; int rate = 3; bool wobble = false;
    // gelişmiş (kullanıcı düğmeleri; -1 = tasarımın kendi değeri)
    float advCut = -1, advRes = -1, advAtk = -1, advRel = -1;
    // açıklama: anlaşılanlar ve otomatik tamamlananlar (arayüz dilinde)
    juce::StringArray heard, filled;
    juce::String title;                                                     // kısa ad (dosya adı)
};

struct Note { double beat, len; int note, vel; };

Design design (const juce::String& text, const Context&, uint32_t seed);
juce::var build (const Design&, double sampleRate);                         // Synth X programı (JSON)
std::vector<Note> phrase (const Design&);                                   // ön dinleme cümlesi (vuruş)
double phraseSeconds (const Design&);
// çevrimdışı işleme (mesaj iş parçacığı); tepe -1 dBFS'e getirilir
std::unique_ptr<juce::AudioBuffer<float>> render (const Design&, const juce::var& program, const hg::Data&, double sampleRate);
bool writeWav (const juce::AudioBuffer<float>&, double sampleRate, const juce::File&);
juce::String kindName (Kind, const juce::String& lang);
juce::String keyName (int key, bool minor, const juce::String& lang);
int defaultRate (Kind);
}
