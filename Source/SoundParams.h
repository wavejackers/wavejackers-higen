// Ses tasarımı parametreleri: akor, bas ve melodi izleri için ayrı ayrı otomasyonlanabilir ayarlar.
// Seçilen ses (yama JSON'u) bu parametrelere açılır; parametreler değişince yama yeniden kurulur.
// Açılmayan alanlar (FM, perde zarfı, vibrato, formant) seçilen sesten aynen korunur.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace hg
{
enum SpKind { spFloat, spInt, spChoice };

enum Sp
{
    o1w, o1oct, o1semi, o1fine, o1lvl,
    o2w, o2oct, o2semi, o2fine, o2lvl,
    o3w, o3oct, o3semi, o3fine, o3lvl,
    wtA, wtB, wtMorph, wtBright,
    uni, uniDet, sub, noise,
    ftype, fslope, cut, reso, fenv, fatk, fdec, ktrack, fdrive,
    envA, envD, envS, envR,
    l1rate, l1depth, l1target, l1shape, l1sync,
    l2rate, l2depth, l2target, l2shape, l2sync,
    fxDrive, fxChorus, fxDelay, fxReverb, level,
    kNumSp
};

struct SpDef
{
    const char* key;        // parametre kimliği son eki
    const char* name;       // DAW'da görünen ad
    SpKind kind;
    float lo, hi, def, centre;   // centre > 0 ise logaritmik his (skew)
    const char* label;      // arayüz çeviri anahtarı
};

const SpDef& spDef (int i);
// Parça kodları: 0 akor, 1 bas, 2 melodi
juce::String spId (int part, int i);
const char* spPartCode (int part);
juce::StringArray spChoices (int i);   // yalnızca seçim türü için (İngilizce; arayüz kendi çevirisini kullanır)
int spPartOfId (const juce::String& paramId);   // "sd_" ile başlamıyorsa -1

// Ses JSON'u -> parametre değerleri (düz değerler)
void spFromSound (const juce::var& sound, float* out);
// Parametre değerleri + temel ses -> yeni ses JSON'u
juce::var spToSound (const juce::var& base, const float* vals);
// Rol için müzikal sınırlar içinde rastgele ses: role 'c' akor, 'b' bas, 'm' melodi
void spRandom (char role, juce::Random& rng, float* out);

void addSoundParams (juce::AudioProcessorValueTreeState::ParameterLayout& layout);
}
