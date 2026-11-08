#include "CoProducer.h"
#include "Data.h"
#include <cmath>
#include <set>

namespace hg
{
namespace
{
// Krumhansl & Kessler (1982) ton profilleri
const double kMaj[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
const double kMin[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };

double corr (const double* a, const double* b)
{
    double ma = 0, mb = 0; for (int i = 0; i < 12; ++i) { ma += a[i]; mb += b[i]; }
    ma /= 12; mb /= 12;
    double s = 0, sa = 0, sb = 0;
    for (int i = 0; i < 12; ++i) { s += (a[i] - ma) * (b[i] - mb); sa += (a[i] - ma) * (a[i] - ma); sb += (b[i] - mb) * (b[i] - mb); }
    return sa > 0 && sb > 0 ? s / std::sqrt (sa * sb) : 0;
}
int mod12 (int x) { return ((x % 12) + 12) % 12; }
}

bool userMidiFromFile (const juce::MidiFile& src, UserMidi& out, juce::String& errKey)
{
    const int tpq = src.getTimeFormat();
    if (tpq <= 0) { errKey = "coSmpte"; return false; }   // SMPTE zaman biçimi desteklenmez
    juce::MidiFile mf (src);
    out.notes.clear(); out.bpm = 0;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        juce::MidiMessageSequence seq (*mf.getTrack (t));
        seq.updateMatchedPairs();
        for (auto* e : seq)
        {
            const auto& m = e->message;
            if (m.isTempoMetaEvent() && out.bpm <= 0) out.bpm = 60.0 / juce::jmax (1e-6, m.getTempoSecondsPerQuarterNote());
            if (! m.isNoteOn() || m.getChannel() == 10) continue;   // davul kanalı alınmaz
            const double s = m.getTimeStamp() / tpq * 4.0;
            const double endT = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() : m.getTimeStamp() + tpq / 4.0;
            NoteEv n; n.s = s; n.l = juce::jmax (0.25, (endT - m.getTimeStamp()) / tpq * 4.0); n.n = m.getNoteNumber(); n.v = juce::jlimit (1, 127, (int) m.getVelocity());
            out.notes.push_back (n);
        }
    }
    if (out.notes.empty()) { errKey = "coEmpty"; return false; }
    std::stable_sort (out.notes.begin(), out.notes.end(), [] (const NoteEv& a, const NoteEv& b) { return a.s < b.s || (a.s == b.s && a.n < b.n); });
    // ilk notanın ölçüsünden başlat (boş girişleri at), 16'lığa yuvarla
    const double first = std::floor (out.notes.front().s / 16.0) * 16.0;
    double end = 0;
    for (auto& n : out.notes) { n.s = std::round ((n.s - first) * 4.0) / 4.0; n.l = juce::jmax (0.25, std::round (n.l * 4.0) / 4.0); end = juce::jmax (end, n.s + n.l); }
    const int need = (int) std::ceil (end / 16.0 - 1e-6);
    out.bars = need <= 4 ? 4 : need <= 8 ? 8 : 16;
    std::vector<NoteEv> keep;
    for (auto& n : out.notes) if (n.s < out.bars * 16.0) { n.l = juce::jmin (n.l, out.bars * 16.0 - n.s); keep.push_back (n); }
    out.notes.swap (keep);
    out.role = guessRole (out.notes);
    return true;
}

bool readUserMidi (const juce::File& f, UserMidi& out, juce::String& errKey)
{
    juce::FileInputStream in (f);
    juce::MidiFile mf;
    if (! in.openedOk() || ! mf.readFrom (in)) { errKey = "coBadFile"; return false; }
    out.name = f.getFileNameWithoutExtension();
    return userMidiFromFile (mf, out, errKey);
}

int guessRole (const std::vector<NoteEv>& notes)
{
    if (notes.empty()) return kMelody;
    // aynı anda çalan nota sayısı (süre ağırlıklı) ve ortalama perde
    double poly = 0, dur = 0, pitch = 0;
    for (auto& a : notes)
    {
        int over = 0; for (auto& b : notes) if (b.s < a.s + a.l && a.s < b.s + b.l) ++over;
        poly += over * a.l; dur += a.l; pitch += a.n * a.l;
    }
    poly /= juce::jmax (1e-9, dur); pitch /= juce::jmax (1e-9, dur);
    if (poly >= 2.4) return kChords;
    if (pitch < 50) return kBass;
    return kMelody;
}

std::vector<KeyGuess> detectKeys (const std::vector<NoteEv>& notes)
{
    static const int kIvMaj[7] = { 0, 2, 4, 5, 7, 9, 11 }, kIvMin[7] = { 0, 2, 3, 5, 7, 8, 10 };
    double h[12] = {}, tot = 0;
    for (auto& n : notes) { const double w = n.l * (std::fmod (n.s, 4.0) == 0 ? 1.25 : 1.0); h[mod12 (n.n)] += w; tot += w; }
    std::vector<KeyGuess> out;
    for (int k = 0; k < 12; ++k)
        for (int m = 0; m < 2; ++m)
        {
            double prof[12], cov = 0;
            for (int i = 0; i < 12; ++i) prof[i] = (m ? kMin : kMaj)[mod12 (i - k)];
            for (int i = 0; i < 7; ++i) cov += h[mod12 (k + (m ? kIvMin : kIvMaj)[i])];
            KeyGuess g; g.key = k; g.minor = m == 1; g.score = corr (h, prof); g.coverage = tot > 0 ? cov / tot : 0;
            out.push_back (g);
        }
    std::stable_sort (out.begin(), out.end(), [] (const KeyGuess& a, const KeyGuess& b)
                      { return std::abs (a.coverage - b.coverage) > 1e-9 ? a.coverage > b.coverage : a.score > b.score; });
    if (out.size() > 1) out[0].margin = out[0].score - out[1].score;
    return out;
}

KeyGuess detectKey (const std::vector<NoteEv>& notes) { return detectKeys (notes).front(); }

std::vector<KeyGuess> keyCandidates (const std::vector<NoteEv>& notes, int maxCount)
{
    auto all = detectKeys (notes); std::vector<KeyGuess> out;
    for (auto& g : all) if (g.coverage >= all.front().coverage - .03 && (int) out.size() < maxCount) out.push_back (g);
    return out;
}

double chordFit (const Result& r, const std::vector<NoteEv>& user, int role)
{
    if (r.chords.empty() || user.empty() || r.bars <= 0) return 0;
    auto pcsOf = [&] (int bar) { std::set<int> s; const auto& c = r.chords[(size_t) (bar % (int) r.chords.size())]; for (int m : c.midi) s.insert (mod12 (m)); return s; };
    std::set<int> scale;
    {
        const auto& iv = Data::get().scale (r.P.scale)["iv"];
        if (auto* a = iv.getArray()) for (auto& x : *a) scale.insert (mod12 (r.P.key + (int) x));
    }
    if (role == kChords)
    {   // ölçü başına perde sınıfı örtüşmesi (Jaccard)
        double tot = 0; int bars = 0;
        for (int b = 0; b < r.bars; ++b)
        {
            std::set<int> u; for (auto& n : user) if ((int) std::floor (n.s / 16.0) % r.bars == b) u.insert (mod12 (n.n));
            if (u.empty()) continue;
            const auto g = pcsOf (b); int inter = 0; for (int x : u) if (g.count (x)) ++inter;
            std::set<int> uni (u); uni.insert (g.begin(), g.end());
            tot += (double) inter / (double) uni.size(); ++bars;
        }
        return bars > 0 ? tot / bars : 0;
    }
    double num = 0, den = 0;
    for (auto& n : user)
    {
        const int bar = (int) std::floor (n.s / 16.0) % r.bars;
        const auto g = pcsOf (bar); const auto& c = r.chords[(size_t) (bar % (int) r.chords.size())];
        const double w = n.l * (std::fmod (n.s, 4.0) == 0 ? 1.5 : 1.0);
        double v = 0;
        if (role == kBass) v = mod12 (n.n) == mod12 (c.pc) ? 1.0 : g.count (mod12 (n.n)) ? .6 : scale.count (mod12 (n.n)) ? .2 : 0;
        else v = g.count (mod12 (n.n)) ? 1.0 : scale.count (mod12 (n.n)) ? .35 : 0;
        num += v * w; den += w;
    }
    return den > 0 ? num / den : 0;
}

std::vector<NoteEv> fitLocked (const std::vector<NoteEv>& notes, int srcBars, int dstBars)
{
    std::vector<NoteEv> out;
    if (srcBars <= 0 || dstBars <= 0) return out;
    for (int rep = 0; rep * srcBars < dstBars; ++rep)
        for (auto n : notes)
        {
            n.s += rep * srcBars * 16.0;
            if (n.s >= dstBars * 16.0) continue;
            n.l = juce::jmin (n.l, dstBars * 16.0 - n.s);
            out.push_back (n);
        }
    return out;
}

std::vector<CoCandidate> coSearch (const Data& data, const GenParams& base, const UserMidi& um, int seeds, int top, const std::vector<KeyGuess>& keys)
{
    std::vector<CoCandidate> all;
    std::vector<std::pair<int, juce::String>> ks;
    for (auto& k : keys) ks.push_back ({ k.key, k.minor ? "minor" : "major" });
    if (ks.empty()) ks.push_back ({ base.key, base.scale });
    for (auto& [key, scale] : ks)
    for (int i = 0; i < seeds; ++i)
        for (int vi = 0; vi < 4; ++vi)
        {
            CoCandidate c; c.P = base; c.P.key = key; c.P.scale = scale; c.P.bars = um.bars; c.P.vi = vi;
            c.P.seed = 1 + (int) (((long long) base.seed + (long long) i * 7919) % 99999);
            for (auto& a : c.P.alt) a = 0;
            const auto r = generate (data, c.P);
            c.fit = chordFit (r, um.notes, um.role);
            c.P.scale = r.P.scale;
            all.push_back (c);
        }
    std::stable_sort (all.begin(), all.end(), [] (const CoCandidate& a, const CoCandidate& b) { return a.fit > b.fit; });
    if ((int) all.size() > top) all.resize ((size_t) top);
    return all;
}

Result coApply (const Data& data, const CoCandidate& c, const UserMidi& um)
{
    auto r = generate (data, c.P);
    r.tracks[(size_t) um.role] = fitLocked (um.notes, um.bars, r.bars);
    return r;
}
}
