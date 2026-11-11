#include "Arrangement.h"
#include "Data.h"
#include <algorithm>
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
    { "Bass", "bass", kBass, -1, 1 }, { "Chords", "chords", kChords, -1, 0 }, { "Lead", "lead", kMelody, -1, 2 } };

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

// ---- türetilmiş katmanlar: akor ilerleyişinden Pad (uzun, geniş) ve Arp (16'lık/8'lik akor tonu yürüyüşü).
// Üretim algoritmasına dokunmaz; yalnızca aranjman çıktılarında (MIDI, FL, Ableton, stem) ayrı izler olarak yer alır.
static bool padIn (const juce::String& profile, const SkSection& s, int bi)
{
    const auto& k = s.kind;
    if (profile == "edm") return k == "break" || k == "build";
    if (profile == "organic") return true;   // Black Coffee: pad bütün parça boyunca
    if (k == "intro") return bi >= juce::jmin (8, s.bars / 2);
    if (k == "outro") return bi < s.bars / 2;
    return true;
}
static bool arpIn (const juce::String& profile, const SkSection& s, int bi)
{
    const auto& k = s.kind; const bool main = k == "drop" || k == "groove";
    if (profile == "organic") return main && bi >= s.bars / 2;
    if (main) return true;
    return k == "build" && bi >= s.bars / 2;
}
// Afro/organik teknikleri (Blessings): yalnızca afro grubundaki stiller; tech house kendi perküsyonunu ve groove'unu korur
static bool isAfro (const Arrangement& a) { return a.profile == "organic" && Data::get().styleById (a.P.style)["grp"].toString() == "afro"; }
static bool mainSec (const SkSection& s) { return s.kind == "drop" || s.kind == "groove"; }

// ---- "Blessings" (Liva K) anlatımından: az eleman, tek tonal çapa, oktav yükselen stab, seyrek perküsyon, hızlanan buildup.
// Stab (yalnızca organic): ton kökü, tresillo benzeri seyrek vuruşlar; ilk drop alçak, sonrakiler bir oktav yukarıda;
// buildup'larda dört ölçüde bir ipucu verir (drop'ta duyulacak sesi önceden ezberletir).
static void deriveStab (Arrangement& a, const Skeleton& sk)
{
    if (! isAfro (a)) return;
    ArrLane st; st.name = "Stab"; st.role = "stab"; st.part = kBass; st.midiChannel = 5;
    const int root = ((a.P.key % 12) + 12) % 12;
    static const int steps[] = { 0, 3, 6, 10 }; static const int vel[] = { 100, 78, 88, 78 };
    int mains = 0;
    for (auto& s : sk.secs)
    {
        if (mainSec (s))
        {
            const int base = 36 + root + (mains > 0 ? 12 : 0);
            for (int b = 0; b < s.bars; ++b)
                for (int k = 0; k < 4; ++k) st.notes.push_back ({ (s.bar + b) * 16.0 + steps[k], 1.5, base, vel[k] });
            ++mains;
        }
        else if (s.kind == "build")
            for (int b = 0; b < s.bars; b += 4) st.notes.push_back ({ (s.bar + b) * 16.0, 1.5, 36 + root, 58 });
    }
    if (! st.notes.empty()) a.lanes.push_back (std::move (st));
}

// Stutter: her buildup'ın son 4 ölçüsünde tek nota hızlanarak tekrarlanır (dörtlük, sekizlik, 16'lık, 32'lik) ve
// pitch bend (+12, uzun buildup'ta +24 yarım ton) ile CC74 son 4-8 ölçüde yükselir; drop'ta sıfırlanır.
static void deriveBuildAuto (Arrangement& a, const Skeleton& sk)
{
    ArrLane st; st.name = "Stutter"; st.role = "stutter"; st.part = kMelody; st.midiChannel = 6;
    const int root = ((a.P.key % 12) + 12) % 12;
    for (auto& s : sk.secs)
    {
        if (s.kind != "build" || s.bars < 4) continue;
        const int hit = juce::jmin (4, s.bars), ramp = juce::jmin (8, s.bars);
        static const double rate[] = { 4, 2, 1, 0.5 };
        for (int k = 0; k < hit; ++k)
        {
            const double bar0 = (s.bar + s.bars - hit + k) * 16.0, r = rate[k];
            const int v = 60 + (k * 55) / juce::jmax (1, hit - 1);
            for (double t = 0; t < 16.0 - 1e-9; t += r) st.notes.push_back ({ bar0 + t, r * .8, 72 + root, juce::jlimit (1, 127, v + (int) (t * 2)) });
        }
        st.bendRange = s.bars >= 16 ? 24 : 12;
        const double a0 = (s.bar + s.bars - ramp) * 16.0, a1 = (s.bar + s.bars) * 16.0;
        for (double t = a0; t < a1 - 1e-9; t += 4.0)
        {
            const double x = (t - a0) / (a1 - a0), e = x * x;   // yavaş başlar, drop'a doğru hızlanır
            st.autos.push_back ({ t, -1, 8192 + juce::roundToInt (e * 8191) });
            st.autos.push_back ({ t, 74, juce::roundToInt (e * 127) });
        }
        st.autos.push_back ({ a1, -1, 8192 }); st.autos.push_back ({ a1, 74, 0 });
    }
    if (! st.notes.empty()) a.lanes.push_back (std::move (st));
}

// Perküsyon inceltme (organic): kalabalık loop yerine ölçü başına en fazla 2 vuruş (en güçlüler kalır)
static void thinPerc (Arrangement& a)
{
    if (! isAfro (a)) return;
    for (auto& L : a.lanes)
    {
        if (L.role != "perc") continue;
        std::vector<NoteEv> out;
        for (size_t i = 0; i < L.notes.size();)
        {
            const int bar = (int) (L.notes[i].s / 16.0); size_t j = i;
            while (j < L.notes.size() && (int) (L.notes[j].s / 16.0) == bar) ++j;
            std::vector<size_t> idx; for (size_t k = i; k < j; ++k) idx.push_back (k);
            std::stable_sort (idx.begin(), idx.end(), [&] (size_t x, size_t y) { return L.notes[x].v > L.notes[y].v; });
            if (idx.size() > 2) idx.resize (2);
            std::sort (idx.begin(), idx.end());
            for (auto k : idx) out.push_back (L.notes[k]);
            i = j;
        }
        L.notes = std::move (out);
    }
}

static std::vector<int> chordPcs (const ChordInfo& c)
{
    std::vector<int> pcs; for (int m : c.midi) { const int pc = ((m % 12) + 12) % 12; if (std::find (pcs.begin(), pcs.end(), pc) == pcs.end()) pcs.push_back (pc); }
    if (pcs.empty()) pcs = { ((c.pc % 12) + 12) % 12 };
    return pcs;
}
static void deriveLayers (Arrangement& a, const Skeleton& sk)
{
    const auto& ch = sk.song.chords;
    if (ch.empty()) return;
    ArrLane pad; pad.name = "Pad"; pad.role = "pad"; pad.part = kChords; pad.midiChannel = 3;
    ArrLane arp; arp.name = "Arp"; arp.role = "arp"; arp.part = kMelody; arp.midiChannel = 4;
    std::vector<int> prev; int prevBar = -2;
    for (auto& s : sk.secs)
        for (int bi = 0; bi < s.bars; ++bi)
        {
            const int bar = s.bar + bi;
            if (bar >= (int) ch.size()) break;
            auto pcs = chordPcs (ch[(size_t) bar]);
            const int root = ((ch[(size_t) bar].pc % 12) + 12) % 12;
            if (padIn (a.profile, s, bi))
            {   // geniş voicing: kök 48-59 arası, akor tonları 60-71 arası
                std::vector<int> v { 48 + root }; for (int pc : pcs) v.push_back (60 + pc);
                std::sort (v.begin(), v.end());
                if (v == prev && prevBar == bar - 1)
                {   // aynı akor sürüyor: notaları uzat
                    for (auto it = pad.notes.rbegin(); it != pad.notes.rend() && it->s + it->l >= bar * 16.0 - 1e-9; ++it) it->l += 16;
                }
                else for (int n : v) pad.notes.push_back ({ bar * 16.0, 16.0, n, 62 });
                prev = v; prevBar = bar;
            }
            else prevBar = -2;
            if (arpIn (a.profile, s, bi))
            {
                int base = 60 + root; if (base > 66) base -= 12;   // kök 54-66 arası, tonlar bir oktav içinde yukarı
                std::vector<int> tones; for (int pc : pcs) tones.push_back (base + ((pc - root + 12) % 12));
                std::sort (tones.begin(), tones.end()); tones.push_back (base + 12);
                const int step = a.profile == "organic" ? 2 : 1;
                for (int k = 0, i = 0; k < 16; k += step, ++i)
                    arp.notes.push_back ({ bar * 16.0 + k, step * .9, tones[(size_t) i % tones.size()], k % 4 == 0 ? 92 : 70 });
            }
        }
    if (! pad.notes.empty()) a.lanes.push_back (std::move (pad));
    if (! arp.notes.empty()) a.lanes.push_back (std::move (arp));
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
    thinPerc (a);
    deriveLayers (a, sk);
    deriveStab (a, sk);
    deriveBuildAuto (a, sk);
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
        if (! L.autos.empty())
        {
            const int ch = L.midiChannel + 1;
            if (L.bendRange > 0)
                for (auto cc : { std::pair<int, int> { 101, 0 }, { 100, 0 }, { 6, L.bendRange }, { 38, 0 } }) s.addEvent (juce::MidiMessage::controllerEvent (ch, cc.first, cc.second), 0);
            for (auto& e : L.autos)
            {
                const double t = (double) tickOf (e.s, P.swing);
                s.addEvent (e.kind < 0 ? juce::MidiMessage::pitchWheel (ch, juce::jlimit (0, 16383, e.value)) : juce::MidiMessage::controllerEvent (ch, e.kind, juce::jlimit (0, 127, e.value)), t);
            }
        }
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
