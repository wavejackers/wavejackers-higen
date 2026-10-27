#include "Presets.h"

namespace hg
{
juce::File PresetStore::dir() const
{
    auto d = overrideDir != juce::File() ? overrideDir
                               : juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::File PresetStore::fileFor (const juce::String& artist, const juce::String& name) const
{
    auto base = juce::File::createLegalFileName (artist.trim() + " - " + name.trim()).trim();
    if (base.isEmpty()) base = "preset";
    return dir().getChildFile (base.substring (0, 120) + kExt);
}

bool PresetStore::readInfo (const juce::File& f, PresetInfo& out)
{
    auto xml = juce::parseXML (f);
    if (xml == nullptr || ! xml->hasTagName ("HitGenPreset")) return false;
    out.file = f; out.artist = xml->getStringAttribute ("artist").substring (0, 80); out.name = xml->getStringAttribute ("name").substring (0, 80);
    out.style = xml->getStringAttribute ("style");
    out.created = juce::Time::fromISO8601 (xml->getStringAttribute ("created"));
    return out.artist.isNotEmpty() && xml->getChildByName ("HitGen") != nullptr;
}

juce::ValueTree PresetStore::readState (const juce::File& f, PresetInfo* info)
{
    PresetInfo tmp; if (! readInfo (f, tmp)) return {};
    if (info != nullptr) *info = tmp;
    auto xml = juce::parseXML (f);
    if (auto* st = xml->getChildByName ("HitGen")) return juce::ValueTree::fromXml (*st);
    return {};
}

void PresetStore::rescan()
{
    juce::Array<PresetInfo> fresh;
    for (const auto& e : juce::RangedDirectoryIterator (dir(), false, juce::String ("*") + kExt, juce::File::findFiles))
    {
        PresetInfo i; if (readInfo (e.getFile(), i)) fresh.add (i);
    }
    std::sort (fresh.begin(), fresh.end(), [] (const PresetInfo& a, const PresetInfo& b)
    {
        const int c = a.artist.compareIgnoreCase (b.artist);
        return c != 0 ? c < 0 : a.name.compareIgnoreCase (b.name) < 0;
    });
    list = fresh; ++rev;
}

int PresetStore::indexOf (const juce::String& artist, const juce::String& name) const
{
    for (int i = 0; i < list.size(); ++i) if (list[i].artist == artist && list[i].name == name) return i;
    return -1;
}

bool PresetStore::write (const juce::File& f, const juce::String& artist, const juce::String& name, const juce::String& style, const juce::ValueTree& state)
{
    juce::XmlElement root ("HitGenPreset");
    root.setAttribute ("format", 1); root.setAttribute ("artist", artist.trim()); root.setAttribute ("name", name.trim());
    root.setAttribute ("style", style); root.setAttribute ("created", juce::Time::getCurrentTime().toISO8601 (true));
    if (auto x = state.createXml()) root.addChildElement (x.release());
    const bool ok = root.writeTo (f);
    rescan();
    return ok;
}

bool PresetStore::remove (const juce::File& f)
{
    const bool ok = f.isAChildOf (dir()) && f.deleteFile();
    rescan();
    return ok;
}

bool PresetStore::importFile (const juce::File& src, PresetInfo& out)
{
    PresetInfo i; if (! readInfo (src, i)) return false;
    auto dst = fileFor (i.artist, i.name);
    if (src != dst && ! src.copyFileTo (dst)) return false;
    rescan();
    const int k = indexOf (i.artist, i.name);
    out = k >= 0 ? list[k] : i;
    return true;
}
}
