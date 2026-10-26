// Gömülü JSON veri kümesi: stiller, sesler, kitler, wavetable'lar, çeviriler
#pragma once
#include <juce_core/juce_core.h>

namespace hg
{
class Data
{
public:
    explicit Data (const juce::String& json);
    static const Data& get();                  // BinaryData'dan bir kez yüklenir

    bool ok() const noexcept { return valid; }
    const juce::var& root() const noexcept { return json; }

    // Stiller
    int numStyles() const { return styles.size(); }
    const juce::var& style (int i) const { return styles.getReference (i); }
    int styleIndex (const juce::String& id) const;
    const juce::var& styleById (const juce::String& id) const;
    juce::String styleId (int i) const { return style (i)["id"].toString(); }

    // Sesler ve kitler
    int numSounds() const { return sounds.size(); }
    const juce::var& sound (int i) const { return sounds.getReference (i); }
    int soundIndex (const juce::String& id) const;
    const juce::var& soundById (const juce::String& id) const;
    int numKits() const { return kits.size(); }
    const juce::var& kit (int i) const { return kits.getReference (i); }
    int kitIndex (const juce::String& id) const;
    double trim (const juce::String& id) const;
    juce::String soundName (const juce::String& id, const juce::String& lang) const;
    bool isSignature (const juce::String& id) const;

    // Ölçek ve notalar
    juce::StringArray noteNames, scaleOrder;
    const juce::var& scale (const juce::String& id) const;

    // Çeviri
    juce::String tr (const juce::String& lang, const juce::String& key) const;
    juce::StringArray langCodes, langNames;
    bool isRtl (const juce::String& lang) const;
    juce::var styleText (const juce::String& lang, const juce::String& styleId) const;   // [tag, blurb, [n,note] x4]

    const juce::var& wavetables() const { return json["wt"]; }

private:
    juce::var json, nullVar;
    juce::Array<juce::var> styles, sounds, kits;
    juce::HashMap<juce::String, int> styleIdx, soundIdx, kitIdx;
    juce::StringArray sigPrefixes;
    bool valid = false;
};

// var yardımcıları
inline double num (const juce::var& v, double def = 0.0) { return v.isVoid() || v.isUndefined() ? def : (double) v; }
inline int inum (const juce::var& v, int def = 0) { return v.isVoid() || v.isUndefined() ? def : (int) v; }
// güvenli dizi erişimi: juce::var[int] sınır dışında tanımsız davranır
inline juce::var at (const juce::var& a, int i) { auto* arr = a.getArray(); return arr != nullptr && i >= 0 && i < arr->size() ? (*arr)[i] : juce::var(); }
inline bool has (const juce::var& o, const char* k) { auto* d = o.getDynamicObject(); return d != nullptr && d->hasProperty (k); }
}
