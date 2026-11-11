#include "AlsWriter.h"
#include "Arrangement.h"
#include <BinaryData.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <map>

namespace hg
{
namespace
{
using X = juce::XmlElement;

X* at (X* e, const juce::String& path)
{
    for (auto& p : juce::StringArray::fromTokens (path, "/", ""))
        if (e == nullptr || (e = e->getChildByName (p)) == nullptr) return nullptr;
    return e;
}
void setV (X* e, const juce::String& path, const juce::String& v) { if (auto* c = at (e, path)) c->setAttribute ("Value", v); }
juce::String num (double v) { return juce::String (v, 6).trimCharactersAtEnd ("0").trimCharactersAtEnd ("."); }
X* add (X* parent, const X& tmpl) { auto* c = new X (tmpl); parent->addChildElement (c); return c; }
void clearChildren (X* e) { if (e != nullptr) e->deleteAllChildElements(); }

std::unique_ptr<X> loadTemplate()
{
    juce::MemoryInputStream raw (BinaryData::HitGenTemplate_als, (size_t) BinaryData::HitGenTemplate_alsSize, false);
    juce::GZIPDecompressorInputStream gz (&raw, false, juce::GZIPDecompressorInputStream::gzipFormat);
    return juce::parseXML (gz.readEntireStreamAsString());
}

// Live 10 dosya referansı: proje klasörüne göreli yol (RelativePathType 3) + arama ipucu; Windows'ta Data = UTF-16LE tam yol
void setFileRef (X* fr, const juce::File& f, const juce::File& setDir)
{
    if (fr == nullptr) return;
    const bool inside = f.isAChildOf (setDir);
    setV (fr, "HasRelativePath", inside ? "true" : "false");
    setV (fr, "RelativePathType", inside ? "3" : "0");
    auto* rp = at (fr, "RelativePath"); clearChildren (rp);
    int id = 0;
    if (inside && rp != nullptr)
        for (auto& d : juce::StringArray::fromTokens (f.getParentDirectory().getRelativePathFrom (setDir), "/\\", ""))
            if (d.isNotEmpty() && d != ".")
            { auto* e = rp->createNewChildElement ("RelativePathElement"); e->setAttribute ("Id", id++); e->setAttribute ("Dir", d); }
    setV (fr, "Name", f.getFileName());
    juce::String hex;
   #if JUCE_WINDOWS
    for (auto c : f.getFullPathName()) { const auto u = (juce::uint32) c; hex << juce::String::toHexString ((int) (u & 0xff)).paddedLeft ('0', 2) << juce::String::toHexString ((int) ((u >> 8) & 0xff)).paddedLeft ('0', 2); }
    hex = hex.toUpperCase() + "0000";
   #endif
    if (auto* d = at (fr, "Data")) { d->deleteAllTextElements(); d->addTextElement (hex); }
    if (auto* ph = at (fr, "SearchHint/PathHint"))
    {
        clearChildren (ph);
        for (auto& d : juce::StringArray::fromTokens (f.getParentDirectory().getFullPathName(), "/\\", ""))
            if (d.isNotEmpty() && ! d.endsWithChar (':'))
            { auto* e = ph->createNewChildElement ("RelativePathElement"); e->setAttribute ("Id", id++); e->setAttribute ("Dir", d); }
    }
    setV (fr, "SearchHint/FileSize", juce::String (f.getSize()));
}

struct AudioInfo { juce::int64 len = 0; double sr = 44100; };
AudioInfo audioInfo (juce::AudioFormatManager& fm, const juce::File& f)
{
    AudioInfo r;
    if (std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f)); rd != nullptr) { r.len = rd->lengthInSamples; r.sr = rd->sampleRate; }
    return r;
}

void setSampleRef (X* sr, const juce::File& f, const juce::File& setDir, const AudioInfo& ai)
{
    if (sr == nullptr) return;
    setFileRef (at (sr, "FileRef"), f, setDir);
    setV (sr, "LastModDate", juce::String (f.getLastModificationTime().toMilliseconds() / 1000));
    setV (sr, "DefaultDuration", juce::String (ai.len));
    setV (sr, "DefaultSampleRate", juce::String (juce::roundToInt (ai.sr)));
}

// Live renk dizini (0..69) şerit rolüne göre
int colourFor (const juce::String& role)
{
    static const std::map<juce::String, int> m { { "kick", 14 }, { "clap", 1 }, { "snare", 2 }, { "hat", 4 }, { "ohat", 5 }, { "rim", 3 },
        { "perc", 6 }, { "bass", 22 }, { "chords", 18 }, { "pad", 19 }, { "lead", 25 }, { "arp", 26 }, { "stab", 23 }, { "stutter", 28 }, { "melody", 25 } };
    if (auto it = m.find (role); it != m.end()) return it->second;
    return 13;
}

void setTrackName (X* t, const juce::String& name)
{
    setV (t, "Name/EffectiveName", name);
    setV (t, "Name/UserName", name);
}

void setClipSpan (X* c, double start, double len, const juce::String& name, int colour)
{
    c->setAttribute ("Time", num (start));
    setV (c, "CurrentStart", num (start));
    setV (c, "CurrentEnd", num (start + len));
    setV (c, "Name", name);
    setV (c, "ColorIndex", juce::String (colour));
}
}

juce::String buildAlsXml (const Arrangement& a, const FlpSources& src, const juce::File& setDir, AlsInfo* info)
{
    AlsInfo I;
    auto root = loadTemplate();
    if (root == nullptr) return {};
    auto* ls = root->getChildByName ("LiveSet");
    auto* tracks = at (ls, "Tracks");
    if (tracks == nullptr) return {};
    std::unique_ptr<X> midiT, audioT;
    for (auto* t = tracks->getFirstChildElement(); t != nullptr; t = t->getNextElement())
    {
        if (t->hasTagName ("MidiTrack") && midiT == nullptr) midiT = std::make_unique<X> (*t);
        if (t->hasTagName ("AudioTrack") && audioT == nullptr) audioT = std::make_unique<X> (*t);
    }
    clearChildren (tracks);
    if (midiT == nullptr || audioT == nullptr) return {};
    const juce::String mEv = "DeviceChain/MainSequencer/ClipTimeable/ArrangerAutomation/Events", aEv = "DeviceChain/MainSequencer/Sample/ArrangerAutomation/Events";
    std::unique_ptr<X> midiClip, audioClip;
    if (auto* e = at (midiT.get(), mEv); e != nullptr && e->getFirstChildElement() != nullptr) midiClip = std::make_unique<X> (*e->getFirstChildElement());
    if (auto* e = at (audioT.get(), aEv); e != nullptr && e->getFirstChildElement() != nullptr) audioClip = std::make_unique<X> (*e->getFirstChildElement());
    if (midiClip == nullptr || audioClip == nullptr) return {};
    clearChildren (at (midiT.get(), mEv)); clearChildren (at (audioT.get(), aEv));

    juce::AudioFormatManager fm; fm.registerBasicFormats();
    int trackId = 20;
    const double totalBeats = a.bars * 4.0;

    // ---- MIDI izleri: şerit başına Simpler + bölüm başına klip
    for (auto& L : a.lanes)
    {
        auto* t = add (tracks, *midiT);
        t->setAttribute ("Id", trackId++);
        setTrackName (t, L.name);
        const int col = colourFor (L.role);
        setV (t, "ColorIndex", juce::String (col));
        if (auto* simpler = at (t, "DeviceChain/DeviceChain/Devices/OriginalSimpler"))
        {
            auto* part = at (simpler, "Player/MultiSampleMap/SampleParts/MultiSamplePart");
            if (auto it = src.lanes.find (L.role); it != src.lanes.end() && part != nullptr)
            {
                const juce::File f (it->second);
                const auto ai = audioInfo (fm, f);
                const auto last = juce::String (std::max<juce::int64> (0, ai.len - 1));
                setV (part, "Name", f.getFileNameWithoutExtension());
                setV (part, "SampleEnd", last); setV (part, "SustainLoop/End", last); setV (part, "ReleaseLoop/End", last);
                setV (part, "RootKey", "60");
                setSampleRef (at (part, "SampleRef"), f, setDir, ai);
            }
        }
        auto* ev = at (t, mEv);
        int clipId = 0;
        for (auto& s : a.secs)
        {
            const double s0 = s.bar * 16.0, s1 = (s.bar + s.bars) * 16.0;
            std::map<int, std::vector<const NoteEv*>> keys;
            for (auto& n : L.notes) if (n.s >= s0 && n.s < s1) keys[L.gm < 0 ? n.n : 60].push_back (&n);
            if (keys.empty()) continue;
            auto* c = add (ev, *midiClip);
            c->setAttribute ("Id", clipId++);
            const double start = s0 / 4.0, len = s.bars * 4.0;
            setClipSpan (c, start, len, L.name + " - " + s.name, col);
            setV (c, "Loop/LoopStart", "0"); setV (c, "Loop/LoopEnd", num (len)); setV (c, "Loop/StartRelative", "0");
            setV (c, "Loop/LoopOn", "false"); setV (c, "Loop/OutMarker", num (len));
            setV (c, "Loop/HiddenLoopStart", "0"); setV (c, "Loop/HiddenLoopEnd", num (len));
            auto* kt = at (c, "Notes/KeyTracks");
            int kid = 0, noteId = 1;
            for (auto& [key, notes] : keys)
            {
                auto* k = kt->createNewChildElement ("KeyTrack"); k->setAttribute ("Id", kid++);
                auto* ns = k->createNewChildElement ("Notes");
                for (auto* n : notes)
                {
                    const double ns0 = (n->s - s0) / 4.0, nl = juce::jmax (1.0 / 16.0, juce::jmin (n->l, s1 - n->s) / 4.0);
                    auto* e = ns->createNewChildElement ("MidiNoteEvent");
                    e->setAttribute ("Time", num (ns0)); e->setAttribute ("Duration", num (nl));
                    e->setAttribute ("Velocity", juce::jlimit (1, 127, n->v)); e->setAttribute ("OffVelocity", 64);
                    e->setAttribute ("IsEnabled", "true"); e->setAttribute ("NoteId", noteId++);
                    ++I.notes;
                }
                k->createNewChildElement ("MidiKey")->setAttribute ("Value", juce::jlimit (0, 127, key));
            }
            setV (c, "Notes/NoteIdGenerator/NextId", juce::String (noteId));
            ++I.midiClips;
        }
        ++I.midiTracks;
    }

    // ---- FX: ipucu türü başına bir ses izi, sample doğal hızında (iki warp işaretçisi)
    std::map<juce::String, X*> fxTracks;
    std::map<juce::String, int> fxClipIds;
    const double bpm = juce::jmax (1, a.P.bpm);
    for (auto& f : src.fx)
    {
        const juce::File file (f.path);
        if (! file.existsAsFile()) continue;
        X*& t = fxTracks[f.cue.type];
        if (t == nullptr)
        {
            t = add (tracks, *audioT);
            t->setAttribute ("Id", trackId++);
            setTrackName (t, "FX " + f.cue.type);
            setV (t, "ColorIndex", "57");
            ++I.audioTracks;
        }
        const auto ai = audioInfo (fm, file);
        const double secs = ai.len > 0 ? (double) ai.len / ai.sr : f.seconds;
        const double clipBeats = juce::jmax (0.25, secs * bpm / 60.0);
        const double c0 = f.cue.bar * 4.0, c1 = (f.cue.bar + f.cue.len) * 4.0;
        double pos, len;
        if (f.endAligned) { pos = juce::jmax (0.0, juce::jmax (c0 - 32.0, c1 - clipBeats)); len = juce::jmin (clipBeats, c1 - pos); }
        else { pos = c0; len = juce::jmin (clipBeats, juce::jmax (c1 - c0, 16.0) + 16.0); }
        len = juce::jmin (len, totalBeats - pos);
        if (len <= 0) continue;
        auto* ev = at (t, aEv);
        // aynı izde çakışma olmasın: önceki klibi bu klibin başında kes
        if (auto* prev = ev->getChildElement (ev->getNumChildElements() - 1))
        {
            const double ps = at (prev, "CurrentStart")->getDoubleAttribute ("Value"), pe = at (prev, "CurrentEnd")->getDoubleAttribute ("Value");
            if (pe > pos) { if (pos - ps < 0.25) continue; setV (prev, "CurrentEnd", num (pos)); }
        }
        auto* c = add (ev, *audioClip);
        c->setAttribute ("Id", fxClipIds[f.cue.type]++);
        setClipSpan (c, pos, len, file.getFileNameWithoutExtension(), 57);
        setV (c, "Loop/LoopStart", "0"); setV (c, "Loop/LoopEnd", num (clipBeats)); setV (c, "Loop/StartRelative", "0");
        setV (c, "Loop/LoopOn", "false"); setV (c, "Loop/OutMarker", num (clipBeats));
        setV (c, "Loop/HiddenLoopStart", "0"); setV (c, "Loop/HiddenLoopEnd", num (clipBeats));
        setV (c, "IsWarped", "true"); setV (c, "WarpMode", "0");
        if (auto* wm = at (c, "WarpMarkers"))
        {
            clearChildren (wm);
            auto* w0 = wm->createNewChildElement ("WarpMarker"); w0->setAttribute ("Id", 0); w0->setAttribute ("SecTime", "0"); w0->setAttribute ("BeatTime", "0");
            auto* w1 = wm->createNewChildElement ("WarpMarker"); w1->setAttribute ("Id", 1); w1->setAttribute ("SecTime", num (secs)); w1->setAttribute ("BeatTime", num (clipBeats));
        }
        clearChildren (at (c, "SavedWarpMarkersForStretched"));
        setSampleRef (at (c, "SampleRef"), file, setDir, ai);
        ++I.audioClips;
    }

    // ---- tempo, bölüm işaretçileri, döngü bölgesi
    setV (ls, "MasterTrack/DeviceChain/Mixer/Tempo/Manual", juce::String (a.P.bpm));
    if (auto* loc = at (ls, "Locators/Locators"))
    {
        clearChildren (loc);
        int id = 0;
        for (auto& s : a.secs)
        {
            auto* e = loc->createNewChildElement ("Locator"); e->setAttribute ("Id", id++);
            e->createNewChildElement ("LomId")->setAttribute ("Value", 0);
            e->createNewChildElement ("Time")->setAttribute ("Value", num (s.bar * 4.0));
            e->createNewChildElement ("Name")->setAttribute ("Value", s.name);
            e->createNewChildElement ("Annotation")->setAttribute ("Value", "");
            e->createNewChildElement ("IsSongStart")->setAttribute ("Value", "false");
            ++I.locators;
        }
    }
    setV (ls, "Transport/LoopStart", "0"); setV (ls, "Transport/LoopLength", num (juce::jmax (4.0, totalBeats)));

    // ---- otomasyon/modülasyon hedef kimlikleri tüm sette benzersiz olmalı
    int next = 100;
    std::function<void (X*)> renumber = [&] (X* e)
    {
        const auto tag = e->getTagName();
        if (e->hasAttribute ("Id") && (tag.endsWith ("Target") || tag.startsWith ("ControllerTargets."))) e->setAttribute ("Id", next++);
        for (auto* c = e->getFirstChildElement(); c != nullptr; c = c->getNextElement()) renumber (c);
    };
    renumber (root.get());
    setV (ls, "NextPointeeId", juce::String (next));
    I.nextPointee = next;
    if (info) *info = I;
    juce::XmlElement::TextFormat fmt; fmt.dtd = {}; fmt.newLineChars = "\n";
    return root->toString (fmt);
}

bool writeAls (const juce::File& out, const juce::String& xml)
{
    if (xml.isEmpty()) return false;
    juce::MemoryOutputStream mo;
    {
        juce::GZIPCompressorOutputStream gz (mo, 6, juce::GZIPCompressorOutputStream::windowBitsGZIP);
        gz.writeText (xml, false, false, nullptr);
        gz.flush();
    }
    return out.replaceWithData (mo.getData(), mo.getDataSize());
}

juce::String readAlsXml (const juce::File& f)
{
    juce::MemoryBlock mb;
    if (! f.loadFileAsData (mb)) return {};
    juce::MemoryInputStream raw (mb, false);
    juce::GZIPDecompressorInputStream gz (&raw, false, juce::GZIPDecompressorInputStream::gzipFormat);
    return gz.readEntireStreamAsString();
}
}
