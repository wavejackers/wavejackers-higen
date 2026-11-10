// Ableton Live projesi (.als) yazıcı: FL Studio .flp ile aynı aranjmanı Live setine döker.
// Her şerit bir MIDI izi (Simpler + şeridin sesi, C3 kökte), her bölüm bir MIDI klibi; FX ipuçları ses izlerinde,
// bölümler Locator olarak, tempo ana izde. Biçim: gzip'li Live 10 XML (Live 11/12 açılışta dönüştürür).
// Şablon (Resources/als/HitGenTemplate.als) boş bir Live 10.1 setinden küçültüldü: bir MIDI izi (Simpler), bir ses izi.
#pragma once
#include <juce_core/juce_core.h>
#include "FlpWriter.h"

namespace hg
{
struct Arrangement;
struct AlsInfo { int midiTracks = 0, audioTracks = 0, midiClips = 0, notes = 0, audioClips = 0, locators = 0, nextPointee = 0; };

// setDir: .als dosyasının klasörü (sample yolları buna göre göreli yazılır)
juce::String buildAlsXml (const Arrangement& a, const FlpSources& src, const juce::File& setDir, AlsInfo* info = nullptr);
bool writeAls (const juce::File& out, const juce::String& xml);
juce::String readAlsXml (const juce::File& f);   // testler: gzip'i aç
}
