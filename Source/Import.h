// Kullanıcının kendi sentezleyici presetlerini içe aktarma: Serum 2 (.SerumPreset) gerçekten okunur ve ses tasarımı
// parametrelerine çevrilir; VST2 FXP/FXB (Serum 1, Synth1 vb.) kapsayıcısı okunur, adı ve türü bulunur. İç ayar blokları
// yazarına özgü ve belgesiz olduğundan bunlar için türüne uyan dolgun bir Studio sesi atanır (kesin dönüşüm iddia edilmez).
#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include "SoundParams.h"

namespace hg
{
struct ImportedSound
{
    bool ok = false;
    bool exact = false;                    // true: ayarlar dosyadan okunup parametrelere işlendi
    juce::String name, author, format;     // ör. "Serum 2", "VST2 FXP (XfsX)"
    int part = 2;                          // önerilen iz: 0 akor, 1 bas, 2 melodi
    juce::String fallbackId;               // exact değilse atanacak yerleşik ses
    std::array<float, kNumSp> x {};        // exact ise parametre değerleri
    int programs = 1;                      // FXB için program sayısı
    juce::String error;
};

ImportedSound importSoundFile (const juce::File& f);
// Testler ve fuzz için: bellekten
ImportedSound importSoundBytes (const void* data, size_t size, const juce::String& fileName);
int guessPart (const juce::String& name);
juce::String fallbackFor (const juce::String& name, int part);
}
