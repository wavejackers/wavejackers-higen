#include "Arrangement.h"
#include "Data.h"
#ifndef JucePlugin_VersionString
 #define JucePlugin_VersionString "3.0.0"
#endif

namespace hg
{
namespace
{
struct LaneDef { const char* name; const char* role; int part, gm, ch; };
const LaneDef kLanes[] = {
    { "Kick", "kick", kDrums, 36, 9 }, { "Clap", "clap", kDrums, 39, 9 }, { "Snare", "snare", kDrums, 38, 9 }, { "Hat", "hat", kDrums, 42, 9 },
    { "Open Hat", "ohat", kDrums, 46, 9 }, { "Rim", "rim", kDrums, 37, 9 }, { "Perc", "perc", kDrums, 64, 9 },
    { "Bass", "bass", kBass, -1, 1 }, { "Chords", "chords", kChords, -1, 0 }, { "Melody", "melody", kMelody, -1, 2 } };

juce::String pyStr (const juce::String& s)   // Python dizgesi: tırnak ve ters bölü kaçışlı, ASCII dışı \u
{
    juce::String o ("\"");
    for (auto c : s)
    {
        if (c == '"' || c == '\\') o << "\\" << juce::String::charToString (c);
        else if (c >= 32 && c < 127) o << juce::String::charToString (c);
        else o << "\\u" << juce::String::toHexString ((int) c).paddedLeft ('0', 4);
    }
    return o + "\"";
}
}

Arrangement buildArrangement (const Skeleton& sk)
{
    Arrangement a;
    a.P = sk.song.P; a.profile = sk.profile; a.bars = sk.song.bars; a.secs = sk.secs; a.cues = sk.cues;
    for (auto& d : kLanes)
    {
        ArrLane L; L.name = d.name; L.role = d.role; L.part = d.part; L.gm = d.gm; L.midiChannel = d.ch;
        for (auto& n : sk.song.tracks[(size_t) d.part]) if (d.gm < 0 || n.n == d.gm) L.notes.push_back (n);
        if (! L.notes.empty()) a.lanes.push_back (std::move (L));
    }
    return a;
}

juce::MemoryBlock arrangementMidi (const Data& data, const Arrangement& a)
{
    const auto& P = a.P;
    juce::MidiFile mf; mf.setTicksPerQuarterNote (480);
    {
        juce::MidiMessageSequence c;
        c.addEvent (juce::MidiMessage::textMetaEvent (3, "Wavejackers HitGen"), 0);
        c.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / juce::jmax (1, P.bpm))), 0);
        c.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
        const auto ki = keyInfo (data, P.key, P.scale);
        c.addEvent (juce::MidiMessage::keySignatureMetaEvent (juce::jlimit (-7, 7, ki.sf), ki.mi != 0), 0);
        for (auto& s : a.secs) c.addEvent (juce::MidiMessage::textMetaEvent (6, s.name), s.bar * 16 * 120.0);
        for (auto& q : a.cues) if (q.type != "pad") c.addEvent (juce::MidiMessage::textMetaEvent (6, "> " + q.type), q.bar * 16 * 120.0);
        auto end = juce::MidiMessage::endOfTrack(); c.addEvent (end, a.bars * 16 * 120.0);
        c.sort();
        mf.addTrack (c);
    }
    for (auto& L : a.lanes)
    {
        juce::MidiMessageSequence s;
        s.addEvent (juce::MidiMessage::textMetaEvent (3, L.name), 0);
        for (auto& n : L.notes)   // önce bitişler: aynı tick'te kapanış açılıştan önce gelsin
            s.addEvent (juce::MidiMessage::noteOff (L.midiChannel + 1, juce::jlimit (0, 127, n.n)), (double) juce::jmax (tickOf (n.s, P.swing) + 1, tickOf (n.s + n.l, P.swing) - 1));
        for (auto& n : L.notes)
            s.addEvent (juce::MidiMessage::noteOn (L.midiChannel + 1, juce::jlimit (0, 127, n.n), (juce::uint8) juce::jlimit (1, 127, n.v)), (double) tickOf (n.s, P.swing));
        s.sort();
        mf.addTrack (s);
    }
    juce::MemoryOutputStream out;
    mf.writeTo (out, 1);
    return out.getMemoryBlock();
}

juce::String arrangementManifest (const Data& data, const Arrangement& a, const juce::StringPairArray& files)
{
    const auto& P = a.P;
    auto* o = new juce::DynamicObject();
    o->setProperty ("app", "Wavejackers HitGen"); o->setProperty ("version", JucePlugin_VersionString); o->setProperty ("schema", 1);
    o->setProperty ("style", P.style); o->setProperty ("artist", data.styleById (P.style)["artist"]); o->setProperty ("profile", a.profile);
    o->setProperty ("bpm", P.bpm); o->setProperty ("key", data.noteNames[juce::jlimit (0, 11, P.key)]); o->setProperty ("scale", P.scale);
    o->setProperty ("seed", P.seed); o->setProperty ("variation", P.vi); o->setProperty ("bars", a.bars);
    o->setProperty ("seconds", std::round (a.seconds() * 100.0) / 100.0); o->setProperty ("timeSignature", "4/4");
    juce::Array<juce::var> secs;
    for (auto& s : a.secs)
    {
        auto* x = new juce::DynamicObject();
        x->setProperty ("name", s.name); x->setProperty ("kind", s.kind); x->setProperty ("bar", s.bar + 1); x->setProperty ("bars", s.bars);
        x->setProperty ("startSeconds", std::round (a.secondsAt (s.bar * 16.0) * 1000.0) / 1000.0);
        secs.add (juce::var (x));
    }
    o->setProperty ("sections", secs);
    juce::Array<juce::var> cues;
    for (auto& q : a.cues)
    {
        auto* x = new juce::DynamicObject();
        x->setProperty ("type", q.type); x->setProperty ("bar", q.bar + 1); x->setProperty ("bars", q.len);
        x->setProperty ("section", q.sec >= 0 && q.sec < (int) a.secs.size() ? juce::var (a.secs[(size_t) q.sec].name) : juce::var());
        cues.add (juce::var (x));
    }
    o->setProperty ("cues", cues);
    juce::Array<juce::var> lanes;
    for (size_t i = 0; i < a.lanes.size(); ++i)
    {
        const auto& L = a.lanes[i];
        auto* x = new juce::DynamicObject();
        x->setProperty ("name", L.name); x->setProperty ("role", L.role); x->setProperty ("midiTrack", (int) i + 1);
        x->setProperty ("midiChannel", L.midiChannel + 1); if (L.gm >= 0) x->setProperty ("gmNote", L.gm);
        x->setProperty ("notes", (int) L.notes.size());
        lanes.add (juce::var (x));
    }
    o->setProperty ("tracks", lanes);
    auto* f = new juce::DynamicObject();
    for (auto& k : files.getAllKeys()) f->setProperty (k, files[k]);
    o->setProperty ("files", juce::var (f));
    return juce::JSON::toString (juce::var (o), false);
}

juce::String flPianoRollScript (const Arrangement& a, const ArrLane& L, const juce::String& songName)
{
    juce::String s;
    s << "# Wavejackers HitGen - " << L.name << " (" << songName << ")\n";
    s << "# FL Studio: copy this file to Documents\\Image-Line\\FL Studio\\Settings\\Piano roll scripts,\n";
    s << "# open the piano roll of the channel for " << L.name << " and run Tools > Scripting > this script.\n";
    s << "# It adds " << (int) L.notes.size() << " notes from bar 1 and the section markers. Tempo: " << a.P.bpm << " BPM.\n";
    s << "import flpianoroll as flp\n\nSRC_PPQ = 480\n";
    s << "NOTES = [\n";
    for (auto& n : L.notes)
    {
        const int t0 = tickOf (n.s, a.P.swing), t1 = juce::jmax (t0 + 1, tickOf (n.s + n.l, a.P.swing));
        s << "    (" << t0 << ", " << (t1 - t0) << ", " << juce::jlimit (0, 127, n.n) << ", " << juce::jlimit (1, 127, n.v) << "),\n";
    }
    s << "]\nMARKERS = [\n";
    for (auto& sec : a.secs) s << "    (" << sec.bar * 16 * 120 << ", " << pyStr (sec.name) << "),\n";
    s << "]\n\n";
    s << "def createDialog():\n";
    s << "    form = flp.ScriptDialog(" << pyStr ("HitGen - " + L.name) << ", " << pyStr ("Adds the HitGen " + L.name + " notes (" + juce::String ((int) L.notes.size()) + ") and section markers.") << ")\n";
    s << "    form.addInputCheckbox(\"Clear existing notes\", True)\n";
    s << "    form.addInputCheckbox(\"Add section markers\", True)\n";
    s << "    return form\n\n";
    s << "def apply(form):\n";
    s << "    k = flp.score.PPQ / SRC_PPQ\n";
    s << "    if form.getInputValue(\"Clear existing notes\"):\n";
    s << "        flp.score.clearNotes(True)\n";
    s << "    for t, l, n, v in NOTES:\n";
    s << "        note = flp.Note()\n";
    s << "        note.time = int(round(t * k))\n";
    s << "        note.length = max(1, int(round(l * k)))\n";
    s << "        note.number = n\n";
    s << "        note.velocity = v / 127.0\n";
    s << "        flp.score.addNote(note)\n";
    s << "    if form.getInputValue(\"Add section markers\"):\n";
    s << "        for t, name in MARKERS:\n";
    s << "            m = flp.Marker()\n";
    s << "            m.time = int(round(t * k))\n";
    s << "            m.name = name\n";
    s << "            flp.score.addMarker(m)\n";
    return s;
}
}
