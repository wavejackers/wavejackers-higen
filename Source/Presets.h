// Kullanıcının kendi sanatçı adıyla kaydettiği presetler (.hgpreset, XML)
// Bir preset; stil, tüm üretici ayarları, dört ses ve ses tasarımı parametrelerini taşır.
#pragma once
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace hg
{
struct PresetInfo
{
    juce::File file;
    juce::String artist, name, style;
    juce::Time created;
    juce::String display() const { return artist + " - " + name; }
};

class PresetStore
{
public:
    static constexpr const char* kExt = ".hgpreset";
    juce::File dir() const;                       // kullanıcı veri klasörü/Wavejackers/HitGen/Presets
    juce::File overrideDir;                       // testler için
    void rescan();
    const juce::Array<PresetInfo>& items() const noexcept { return list; }
    int revision() const noexcept { return rev; }   // liste değişince artar (arayüz yenilemesi için)
    juce::File fileFor (const juce::String& artist, const juce::String& name) const;
    bool write (const juce::File& f, const juce::String& artist, const juce::String& name, const juce::String& style, const juce::ValueTree& state);
    bool remove (const juce::File& f);
    bool importFile (const juce::File& src, PresetInfo& out);   // klasöre kopyalar
    static bool readInfo (const juce::File& f, PresetInfo& out);
    static juce::ValueTree readState (const juce::File& f, PresetInfo* info = nullptr);
    int indexOf (const juce::String& artist, const juce::String& name) const;
private:
    juce::Array<PresetInfo> list; int rev = 0;
};
}
