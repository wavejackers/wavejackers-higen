#include "Data.h"
#include "BinaryData.h"

namespace hg
{
Data::Data (const juce::String& text)
{
    auto r = juce::JSON::parse (text, json);
    if (r.failed() || ! json.isObject()) return;
    if (auto* a = json["styles"].getArray()) styles = *a;
    if (auto* a = json["sounds"].getArray()) sounds = *a;
    if (auto* a = json["kits"].getArray()) kits = *a;
    for (int i = 0; i < styles.size(); ++i) styleIdx.set (styles[i]["id"].toString(), i);
    for (int i = 0; i < sounds.size(); ++i) soundIdx.set (sounds[i]["id"].toString(), i);
    for (int i = 0; i < kits.size(); ++i) kitIdx.set (kits[i]["id"].toString(), i);
    if (auto* a = json["notes"].getArray()) for (auto& v : *a) noteNames.add (v.toString());
    if (auto* a = json["scaleOrder"].getArray()) for (auto& v : *a) scaleOrder.add (v.toString());
    if (auto* a = json["langs"].getArray()) for (auto& v : *a) { langCodes.add (v[0].toString()); langNames.add (v[1].toString()); }
    // "^(wj|gt|...)-" biçimindeki imza önekleri
    auto sig = json["sig"].toString().fromFirstOccurrenceOf ("(", false, false).upToFirstOccurrenceOf (")", false, false);
    sigPrefixes.addTokens (sig, "|", {});
    valid = styles.size() > 0 && sounds.size() > 0 && kits.size() > 0 && noteNames.size() == 12;
}

const Data& Data::get()
{
    static const Data d (juce::String::fromUTF8 (BinaryData::hitgendata_json, BinaryData::hitgendata_jsonSize));
    return d;
}

int Data::styleIndex (const juce::String& id) const { return styleIdx.contains (id) ? styleIdx[id] : -1; }
const juce::var& Data::styleById (const juce::String& id) const { auto i = styleIndex (id); return i >= 0 ? styles.getReference (i) : styles.getReference (0); }
int Data::soundIndex (const juce::String& id) const { return soundIdx.contains (id) ? soundIdx[id] : -1; }
const juce::var& Data::soundById (const juce::String& id) const { auto i = soundIndex (id); return i >= 0 ? sounds.getReference (i) : nullVar; }
int Data::kitIndex (const juce::String& id) const { return kitIdx.contains (id) ? kitIdx[id] : -1; }
double Data::trim (const juce::String& id) const { auto t = json["trim"][juce::Identifier (id)]; return t.isVoid() ? 1.0 : (double) t; }
const juce::var& Data::scale (const juce::String& id) const { return json["scales"][juce::Identifier (id)]; }
bool Data::isRtl (const juce::String& lang) const { return (bool) json["rtl"][juce::Identifier (lang)]; }

bool Data::isSignature (const juce::String& id) const
{
    for (auto& p : sigPrefixes) if (id.startsWith (p + "-")) return true;
    return false;
}

juce::String Data::soundName (const juce::String& id, const juce::String& lang) const
{
    auto i = soundIndex (id);
    juce::var o = i >= 0 ? sounds[i] : (kitIndex (id) >= 0 ? kits[kitIndex (id)] : juce::var());
    if (o.isVoid()) return id;
    if (lang == "tr") return has (o, "name") ? o["name"].toString() : o["n"].toString();
    auto en = json["en"][juce::Identifier (id)];
    if (! en.isVoid()) return en.toString();
    return has (o, "name") ? o["name"].toString() : o["n"].toString();
}

juce::String Data::tr (const juce::String& lang, const juce::String& key) const
{
    auto v = json["i18n"][juce::Identifier (lang)][juce::Identifier (key)];
    if (v.isVoid()) v = json["i18n"]["en"][juce::Identifier (key)];
    return v.isVoid() ? key : v.toString();
}

juce::var Data::styleText (const juce::String& lang, const juce::String& styleId) const
{
    auto v = json["st"][juce::Identifier (lang)][juce::Identifier (styleId)];
    if (v.isVoid()) v = json["st"]["en"][juce::Identifier (styleId)];
    return v;
}
}
