// HG.generate / HG.assemble / MB.midiFile'ın C++ karşılığı.
// Rastgele sayı çağrılarının sırası JS sürümüyle aynıdır; aynı ayarlar aynı MIDI'yi verir.
#include "Generator.h"
#include "Data.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace hg
{
const char* const kTrackIds[kNumTracks] = { "chords", "bass", "drums", "melody" };

namespace
{
const int kGM_kick = 36, kGM_rim = 37, kGM_snare = 38, kGM_clap = 39, kGM_chat = 42, kGM_ohat = 46, kGM_perc = 64;
const int MAJ[7] = { 0, 2, 4, 5, 7, 9, 11 };
const char* const ROMAN[7] = { "I", "II", "III", "IV", "V", "VI", "VII" };

inline int mod12 (int x) { return ((x % 12) + 12) % 12; }
inline int mod7 (int x) { return ((x % 7) + 7) % 7; }
inline double jsRound (double x) { return std::floor (x + 0.5); }
template <typename T> inline T clampT (T x, T a, T b) { return std::max (a, std::min (b, x)); }

std::vector<int> intArray (const juce::var& v)
{
    std::vector<int> out;
    if (auto* a = v.getArray()) for (auto& x : *a) out.push_back ((int) x);
    return out;
}
std::vector<std::vector<double>> rows (const juce::var& v)
{
    std::vector<std::vector<double>> out;
    if (auto* a = v.getArray())
        for (auto& r : *a)
        {
            std::vector<double> row;
            if (auto* b = r.getArray()) for (auto& x : *b) row.push_back ((double) x);
            out.push_back (row);
        }
    return out;
}
template <typename T> const T& pickFrom (Rng& r, const std::vector<T>& a) { return a[(size_t) std::floor (r() * (double) a.size())]; }

// JS'de undefined/0/'' için "||" varsayılanı
double orDef (const juce::var& v, double def) { if (v.isVoid() || v.isUndefined()) return def; double d = (double) v; return d == 0.0 ? def : d; }
juce::String orStr (const juce::var& v, const juce::String& def) { auto s = (v.isVoid() || v.isUndefined()) ? juce::String() : v.toString(); return s.isEmpty() ? def : s; }
}

Result generate (const Data& data, const GenParams& P)
{
    Result R;
    const juce::var& S = data.styleById (P.style);
    auto* varr = S["var"].getArray();
    const int vi = P.vi;
    const juce::var V = (varr != nullptr && vi >= 0 && vi < varr->size()) ? (*varr)[vi] : (*varr)[0];
    juce::String scale = P.scale.isNotEmpty() ? P.scale : orStr (V["scale"], S["scales"][0].toString());
    auto iv = intArray (data.scale (scale)["iv"]);
    if (iv.size() != 7) { scale = "minor"; iv = intArray (data.scale (scale)["iv"]); }
    const int bars = clampT (P.bars, 1, 64), key = P.key;
    const double orig = P.orig / 100.0, energy = P.energy / 100.0, hv = P.human / 100.0;
    auto deg = [&] (int i) { return iv[(size_t) mod7 (i)] + 12 * (int) std::floor (i / 7.0); };
    Rng rp ((double) P.seed * 7919 + 11 + vi * 13);

    // ---- akor ilerlemesi
    std::vector<int> prog = has (V, "prog") ? intArray (V["prog"]) : std::vector<int> { 0, 5, 2, 6 };
    for (int i = 1; i < 4; ++i)
        if (rp() < orig * .45)
            prog[(size_t) i] = pickFrom (rp, std::vector<int> { (prog[(size_t) i] + 2) % 7, (prog[(size_t) i] + 5) % 7, (prog[(size_t) i] + 3) % 7, (prog[(size_t) i] + 4) % 7 });
    auto turn = prog; turn[3] = pickFrom (rp, std::vector<int> { 4, 6, 3, prog[3] });
    auto degAt = [&] (int b) { int g = b / 4; return (g % 4 == 3 ? turn : prog)[(size_t) (b % 4)]; };

    auto info = [&] (int d, ChordInfo& c)
    {
        int root = deg (d), third = mod12 (deg (d + 2) - root), fifth = mod12 (deg (d + 4) - root), sev = mod12 (deg (d + 6) - root);
        juce::String q = "maj";
        if (third == 3 && fifth == 7) q = "min"; else if (third == 3 && fifth == 6) q = "dim"; else if (third == 4 && fifth == 8) q = "aug";
        juce::String suf = q == "min" ? "m" : q == "dim" ? "dim" : q == "aug" ? "aug" : "";
        juce::String rsuf = q == "dim" ? juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")) : q == "aug" ? "+" : "";
        if (P.sevenths)
        {
            if (q == "maj" && sev == 11) { suf = "maj7"; rsuf = "maj7"; }
            else if (q == "maj" && sev == 10) { suf = "7"; rsuf = "7"; }
            else if (q == "min" && sev == 10) { suf = "m7"; rsuf = "7"; }
            else if (q == "dim" && sev == 10) { suf = "m7b5"; rsuf = juce::String (juce::CharPointer_UTF8 ("\xc3\xb8" "7")); }
        }
        int acc = root - MAJ[d % 7];
        juce::String rn = ROMAN[d % 7];
        if (q == "min" || q == "dim") rn = rn.toLowerCase();
        c.name = data.noteNames[mod12 (key + root)] + suf;
        c.roman = juce::String (acc < 0 ? "b" : acc > 0 ? "#" : "") + rn + rsuf;
        c.pc = mod12 (key + root); c.fifth = fifth; c.third = third; c.d = d;
    };
    auto avg = [] (const std::vector<int>& a) { double s = 0; for (auto x : a) s += x; return s / (double) a.size(); };
    auto lead = [&] (const std::vector<int>* prev, std::vector<int> notes)
    {
        std::sort (notes.begin(), notes.end());
        std::vector<std::vector<int>> rots;
        auto c = notes;
        for (size_t k = 0; k < c.size(); ++k) { rots.push_back (c); std::vector<int> n2 (c.begin() + 1, c.end()); n2.push_back (c[0] + 12); c = n2; }
        std::vector<int> best; double bc = 1e9;
        for (auto& r : rots)
            for (int sh : { -24, -12, 0, 12 })
            {
                std::vector<int> cc; for (auto x : r) cc.push_back (x + sh);
                double av = avg (cc), cost;
                if (prev != nullptr)
                {
                    size_t m = std::min (cc.size(), prev->size()); cost = 0;
                    for (size_t i = 0; i < m; ++i) cost += std::abs (cc[i] - (*prev)[i]);
                    cost += std::abs (av - avg (*prev)) * 2;
                }
                else cost = std::abs (av - 61);
                if (av < 54 || av > 69) cost += 50;
                if (cc[0] < 45) cost += 50;
                if (cost < bc) { bc = cost; best = cc; }
            }
        return best;
    };
    std::vector<int> prevV; bool hasPrev = false;
    for (int b = 0; b < bars; ++b)
    {
        int d = degAt (b);
        std::vector<int> idx { d, d + 2, d + 4 }; if (P.sevenths) idx.push_back (d + 6);
        std::vector<int> notes; for (auto i : idx) notes.push_back (48 + key + deg (i));
        ChordInfo c; c.midi = lead (hasPrev ? &prevV : nullptr, notes); prevV = c.midi; hasPrev = true;
        info (d, c);
        R.chords.push_back (c);
    }

    auto& T = R.tracks;
    Rng rh ((double) P.seed * 31 + 17);
    auto add = [&] (int tr, double s, double l, double n, double v)
    {
        NoteEv e; e.s = s; e.l = l; e.n = (int) clampT (jsRound (n), 0.0, 127.0);
        e.v = (int) clampT (jsRound (v + (rh() - .5) * hv * 30), 1.0, 127.0);
        T[(size_t) tr].push_back (e);
    };
    const double eV = .8 + .2 * energy;

    // ---- akor izi
    Rng rc ((double) P.seed * 1297 + 3 + vi);
    const juce::String cmode = orStr (V["cm"], S["chord"]["mode"].toString());
    std::vector<juce::String> cpKeys;
    if (has (V, "cp")) cpKeys.push_back (V["cp"].toString());
    else if (auto* a = S["chord"]["pats"].getArray()) for (auto& x : *a) cpKeys.push_back (x.toString());
    std::vector<std::vector<std::vector<double>>> cpats;
    for (auto& k : cpKeys) cpats.push_back (rows (data.root()["cp"][juce::Identifier (k)]));
    auto arpSeq = intArray (data.root()["arp"][juce::Identifier (orStr (S["chord"]["arp"], "trance"))]);
    for (int b = 0; b < bars; ++b)
    {
        const int base = b * 16; const auto& ch = R.chords[(size_t) b].midi;
        if (cmode == "arp")
        {
            std::vector<int> ext = ch; for (auto n : ch) ext.push_back (n + 12);
            for (int s = 0; s < 16; ++s)
            {
                int ix = arpSeq[(size_t) (s % (int) arpSeq.size())]; if (ix < 0) continue;
                if (rc() < (1 - energy) * .12 + orig * .05) continue;
                add (kChords, base + s, 1, ext[(size_t) (ix % (int) ext.size())], (s % 4 == 0 ? 96 : 76) * eV);
            }
        }
        else
        {
            auto pat = cpats[(b % 4 == 3 && cpats.size() > 1) ? 1 : 0];
            if (cmode == "sustain" && ! has (V, "cp"))
            {
                auto p0 = S["chord"]["pats"][0].toString();
                pat = rows (data.root()["cp"][juce::Identifier (p0 == "whole" || p0 == "long" ? p0 : juce::String ("whole"))]);
            }
            for (auto& e : pat)
            {
                double s = e[0], l = e[1], v = e[2];
                if (orig > .2 && rc() < orig * .12 && s > 0) continue;
                for (auto n : ch) add (kChords, base + s, l, n, v * eV);
            }
        }
    }

    // ---- bas izi
    Rng rb ((double) P.seed * 3571 + 5 + vi);
    auto bpat = rows (data.root()["bp"][juce::Identifier (orStr (V["bp"], S["bass"]["pats"][0].toString()))]);
    const int boct = (int) orDef (S["bass"]["oct"], 36);
    for (int b = 0; b < bars; ++b)
    {
        const int base = b * 16; const auto& c = R.chords[(size_t) b];
        int root = boct + c.pc; if (c.pc > 6) root -= 12;
        auto note = [&] (int code) { return code == 1 ? root : code == 2 ? root + c.fifth : code == 3 ? root + 12 : code == 4 ? root + c.third : root + 10; };
        for (auto& e : bpat)
        {
            double s = e[0], l = e[1], v = e[3]; int code = (int) e[2];
            if (energy < .4 && v < 90 && ((int) s) % 2 == 1 && rb() < .5) continue;
            if (rb() < orig * .1) continue;
            int cd = code; if (rb() < orig * .12 && cd == 1) cd = 3;
            add (kBass, base + s, l, note (cd), v);
        }
    }

    // ---- davul izi
    Rng rd ((double) P.seed * 6151 + 9 + vi);
    std::map<juce::String, juce::String> D;
    auto mergeDrums = [&] (const juce::var& o) { if (auto* obj = o.getDynamicObject()) for (auto& p : obj->getProperties()) D[p.name.toString()] = p.value.toString(); };
    mergeDrums (S["drums"]); mergeDrums (V["dr"]);
    struct Lane { const char* id; int note, bv; };
    const Lane lanes[] = { { "kick", kGM_kick, 118 }, { "clap", kGM_clap, 100 }, { "snare", kGM_snare, 106 }, { "chat", kGM_chat, 80 }, { "ohat", kGM_ohat, 90 }, { "rim", kGM_rim, 70 }, { "perc", kGM_perc, 80 } };
    auto drumAt = [&] (double s, int n) { for (auto& x : T[kDrums]) if (x.s == s && (n < 0 || x.n == n)) return true; return false; };
    const juce::String fill = D.count ("fill") ? D["fill"] : juce::String();
    for (int b = 0; b < bars; ++b)
    {
        const int base = b * 16;
        for (auto& L : lanes)
        {
            auto it = D.find (L.id); if (it == D.end() || it->second.isEmpty()) continue;
            const juce::String& pat = it->second; const juce::String lane (L.id);
            for (int s = 0; s < 16 && s < pat.length(); ++s)
            {
                auto c = pat[s]; if (c != 'x' && c != 'X') continue;
                if (lane == "chat" && s % 2 == 1 && rd() > .55 + energy * .45) continue;
                if ((lane == "perc" || lane == "rim" || lane == "chat") && rd() < orig * .07) continue;
                int v = L.bv + (c == 'X' ? 8 : 0);
                if (lane == "chat") v = s % 4 == 0 ? 92 : s % 2 == 0 ? 78 : 58;
                add (kDrums, base + s, .9, L.note, v + (int) std::floor (rd() * 10) - 5);
            }
        }
        if (orig > .1)
            for (int s = 1; s < 16; s += 2)
                if (rd() < orig * .06 && ! drumAt (base + s, -1))
                {
                    int n = rd() < .5 ? kGM_rim : kGM_perc;
                    add (kDrums, base + s, .9, n, 48 + std::floor (rd() * 20));
                }
        if (fill != "none" && b % 4 == 3 && b > 0 && bars > 4)
        {
            int note = fill == "snare" ? kGM_snare : kGM_clap;
            int i = 0;
            for (int s : { 12, 13, 14, 15 }) { if (rd() < .8 && ! drumAt (base + s, note)) add (kDrums, base + s, .9, note, 70 + i * 12); ++i; }
        }
    }

    // ---- melodi izi
    Rng rm ((double) P.seed * 104729 + 7 + vi * 5);
    const juce::var mel = S["mel"];
    const juce::String mmode = orStr (V["mm"], mel["mode"].toString());
    const double dens = clampT (num (mel["dens"]) * (has (V, "md") ? num (V["md"]) : 1.0), .08, 1.4);
    const int mbase = (int) orDef (mel["oct"], 72) - 12 + key;
    auto cTones = [] (const ChordInfo& c) { return std::vector<int> { c.d % 7, (c.d + 2) % 7, (c.d + 4) % 7 }; };
    auto snapDeg = [&] (int m, const std::vector<int>& tones)
    {
        int best = m; double bd = 99;
        for (int k = m - 3; k <= m + 3; ++k)
        {
            if (std::find (tones.begin(), tones.end(), mod7 (k)) == tones.end()) continue;
            double d = std::abs (k - m) + rm() * .1;
            if (d < bd) { bd = d; best = k; }
        }
        return best;
    };
    auto addM = [&] (int b, double s, double l, int m, double v) { int x = m; while (x > 96) x -= 12; while (x < 36) x += 12; add (kMelody, b * 16 + s, std::max (1.0, l), x, v); };
    if (mmode == "hook")
    {
        std::vector<juce::String> hrk;
        auto hv2 = has (V, "hr") ? V["hr"] : mel["hr"];
        if (auto* a = hv2.getArray()) for (auto& x : *a) hrk.push_back (x.toString());
        std::vector<std::vector<std::vector<double>>> tpls;
        for (auto& k : hrk) tpls.push_back (rows (data.root()["hr"][juce::Identifier (k)]));
        const double rep = has (mel, "rep") ? num (mel["rep"]) : .4, leap = .12 + orig * .3;
        int cur = 3 + (int) std::floor (rm() * 3);
        for (int b = 0; b < bars; ++b)
        {
            const auto& c = R.chords[(size_t) b]; const auto& tp = tpls[(size_t) ((b >> 1) % (int) tpls.size())]; const bool odd = b % 2 == 1;
            std::vector<std::vector<double>> T2 = tp;
            if (odd) T2.resize ((size_t) std::max (2, (int) tp.size() - 1));
            const double keep = clampT (.5 + dens, .55, 1.0);
            for (size_t j = 0; j < T2.size(); ++j)
            {
                double s = T2[j][0], l = T2[j][1];
                if (j > 0 && rm() > keep) continue;
                if (j == 0) cur = snapDeg (cur, cTones (c));
                else if (rm() < rep) {}
                else
                {
                    double r = rm(); int mag = r < .6 ? 1 : r < .85 ? 2 : (rm() < leap * 4 ? 4 : 3);
                    bool up = rm() < .5;
                    if (cur > 8) up = rm() < .3; else if (cur < 2) up = rm() < .75;
                    cur += up ? mag : -mag;
                }
                cur = clampT (cur, 0, 10);
                const bool last = j == T2.size() - 1;
                if (last && odd) cur = snapDeg (cur, { c.d % 7, (c.d + 4) % 7 });
                double nx = j + 1 < T2.size() ? T2[j + 1][0] : 16;
                addM (b, s, std::min (l, nx - s), mbase + deg (cur), ((int) s) % 4 == 0 ? 100 : 84);
            }
        }
    }
    else if (mmode == "arp")
    {
        auto seq = intArray (data.root()["arp"][juce::Identifier (orStr (V["arp"], orStr (mel["arp"], "trance")))]);
        const int oc = (int) orDef (mel["oct"], 72);
        for (int b = 0; b < bars; ++b)
        {
            const auto& c = R.chords[(size_t) b]; std::vector<int> ext;
            for (auto n : c.midi) { int x = n; while (x < oc - 4) x += 12; while (x > oc + 8) x -= 12; ext.push_back (x); }
            auto e2 = ext; for (auto n : ext) e2.push_back (n + 12);
            std::stable_sort (e2.begin(), e2.end());
            const int lift = (b % 4 == 3 && orig > .3) ? 12 : 0, shift = (b % 8 >= 4) ? 1 : 0;
            for (int s = 0; s < 16; ++s)
            {
                int ix = seq[(size_t) ((s + shift) % (int) seq.size())]; if (ix < 0) continue;
                if (rm() < (1 - clampT (dens, 0.0, 1.0)) * .55) continue;
                int m = e2[(size_t) (ix % (int) e2.size())] + (s >= 12 ? lift : 0);
                addM (b, s, 1, m, s % 4 == 0 ? 98 : 78);
            }
        }
    }
    else if (mmode == "riff")
    {
        const std::vector<int> cand { 0, 2, 3, 4, 6, 7, 8, 10, 11, 12, 14, 15 }, offs { 0, 0, 0, 7, 10, 12, 3, 5 };
        const int cnt = (int) jsRound (3 + dens * 8);
        std::vector<std::pair<int, int>> riff; auto pool = cand;
        while ((int) riff.size() < cnt && ! pool.empty())
        {
            size_t i = (size_t) std::floor (rm() * (double) pool.size());
            int s = pool[i]; pool.erase (pool.begin() + (long) i);
            riff.push_back ({ s, pickFrom (rm, offs) });
        }
        std::stable_sort (riff.begin(), riff.end(), [] (auto& a, auto& z) { return a.first < z.first; });
        const int oc = (int) orDef (mel["oct"], 60);
        for (int b = 0; b < bars; ++b)
        {
            const auto& c = R.chords[(size_t) b];
            if (b % 4 == 3 && riff.size() > 2 && rm() < .4 + orig * .5) { size_t i = (size_t) std::floor (rm() * (double) riff.size()); riff[i] = { riff[i].first, pickFrom (rm, offs) }; }
            const int rootM = oc + ((c.pc - (oc % 12) + 12) % 12);
            for (size_t j = 0; j < riff.size(); ++j)
            {
                int s = riff[j].first, o = riff[j].second;
                double nx = j + 1 < riff.size() ? riff[j + 1].first : 16;
                double len = std::min (nx - s, rm() < .3 ? 2.0 : 1.0);
                addM (b, s, len, rootM + o, s % 4 == 0 ? 102 : 84);
            }
        }
    }
    else // chop
    {
        const std::vector<int> cand { 0, 2, 3, 5, 6, 7, 10, 11, 14 };
        const int cnt = (int) jsRound (3 + dens * 6);
        auto mk = [&]
        {
            auto p = cand; std::vector<int> out;
            while ((int) out.size() < cnt && ! p.empty()) { size_t i = (size_t) std::floor (rm() * (double) p.size()); out.push_back (p[i]); p.erase (p.begin() + (long) i); }
            std::stable_sort (out.begin(), out.end()); return out;
        };
        auto A = mk(); auto B = A;
        { size_t i = (size_t) std::floor (rm() * (double) B.size()); B[i] = pickFrom (rm, cand); }
        { std::vector<int> u; for (auto x : B) if (std::find (u.begin(), u.end(), x) == u.end()) u.push_back (x); std::stable_sort (u.begin(), u.end()); B = u; }
        const int oc = (int) orDef (mel["oct"], 72);
        for (int b = 0; b < bars; ++b)
        {
            const auto& c = R.chords[(size_t) b]; const auto& hits = (b % 2 == 0) ? A : B;
            const int rootM = oc + ((c.pc - (oc % 12) + 12) % 12) - 12;
            int last = 0;
            for (size_t j = 0; j < hits.size(); ++j)
            {
                int s = hits[j];
                int off = rm() < .45 ? last : pickFrom (rm, std::vector<int> { 0, c.third, c.fifth, 12, 12 + c.third, 12 + c.fifth });
                last = off;
                double nx = j + 1 < hits.size() ? hits[j + 1] : 16;
                double len = std::min (nx - s, rm() < .5 ? 1.0 : 2.0);
                addM (b, s, len, rootM + off, s % 4 == 0 ? 100 : 82);
            }
        }
    }

    // ---- kalınlık: akor kökü bir oktav alta, yüksek değerde melodi de
    const double thick = P.thick;
    auto byStartNote = [] (const NoteEv& x, const NoteEv& y) { return x.s < y.s || (x.s == y.s && x.n < y.n); };
    if (thick >= 40)
    {
        if (cmode == "arp")
        {
            for (int b = 0; b < bars; ++b)
            {
                const auto& c = R.chords[(size_t) b]; int low = *std::min_element (c.midi.begin(), c.midi.end());
                int r = low - mod12 (low - c.pc); if (r == low) r -= 12;
                if (r >= 38) { NoteEv e; e.s = b * 16; e.l = 16; e.n = r; e.v = (int) jsRound (70 + thick * .2); T[kChords].push_back (e); }
            }
        }
        else
        {
            std::map<double, std::vector<NoteEv>> groups;   // JS'de sayısal anahtarlar artan sırada gezilir
            for (auto& n : T[kChords]) groups[n.s].push_back (n);
            for (auto& g : groups)
            {
                int b = (int) std::floor (g.first / 16); const auto& c = R.chords[(size_t) b];
                int low = 999, vmax = 0; for (auto& n : g.second) { low = std::min (low, n.n); vmax = std::max (vmax, n.v); }
                int r = low - mod12 (low - c.pc); if (r == low) r -= 12;
                if (r >= 38) { NoteEv e; e.s = g.first; e.l = g.second[0].l; e.n = r; e.v = (int) clampT (jsRound (vmax * .85), 1.0, 127.0); T[kChords].push_back (e); }
            }
        }
        std::stable_sort (T[kChords].begin(), T[kChords].end(), byStartNote);
    }
    if (thick >= 60)
    {
        std::vector<NoteEv> dbl;
        for (auto& n : T[kMelody]) if (n.n - 12 >= 48) { NoteEv e = n; e.n = n.n - 12; e.v = (int) clampT (jsRound (n.v * .72), 1.0, 127.0); dbl.push_back (e); }
        T[kMelody].insert (T[kMelody].end(), dbl.begin(), dbl.end());
        std::stable_sort (T[kMelody].begin(), T[kMelody].end(), byStartNote);
    }

    R.P = P; R.P.scale = scale; R.P.vi = vi; R.bars = bars; R.steps = bars * 16;
    return R;
}

// ---- Loop'tan tam parça
namespace
{
struct Mask { int drums = 2; bool bass = true, chords = true, melody = true; int roll = -1; };   // drums: 0 yok, 1 hafif, 2 tam, 3 build
Mask maskFor (const juce::String& name, int bi, int B)
{
    auto n = name.toLowerCase(); Mask m;
    if (n.contains ("intro")) { m.drums = 1; m.bass = bi >= B / 2.0; m.chords = bi >= B * .75; m.melody = false; }
    else if (n.contains ("outro")) { m.drums = 1; m.bass = bi < B / 2.0; m.chords = bi < B / 4.0; m.melody = false; }
    else if (n.contains ("build")) { m.drums = 3; m.bass = bi < B - 2; m.chords = true; m.melody = false; m.roll = bi >= B - 4 ? bi - (B - 4) : -1; }
    else if (n.contains ("break")) { m.drums = 0; m.bass = false; m.chords = true; m.melody = true; }
    else if (n.contains ("verse")) { m.drums = 2; m.bass = true; m.chords = true; m.melody = false; }
    return m;
}
}

Result assemble (const Data& data, const Result& res)
{
    Result A; A.P = res.P;
    const juce::var& S = data.styleById (res.P.style);
    const int L = res.bars;
    std::array<std::vector<std::vector<NoteEv>>, kNumTracks> perBar;
    for (int k = 0; k < kNumTracks; ++k)
    {
        perBar[(size_t) k].assign ((size_t) L, {});
        for (auto& n : res.tracks[(size_t) k]) { int b = (int) std::floor (n.s / 16); if (b < L) perBar[(size_t) k][(size_t) b].push_back (n); }
    }
    const int rollNote = S["drums"]["fill"].toString() == "snare" ? kGM_snare : kGM_clap;
    int off = 0;
    if (auto* arr = S["arr"].getArray())
        for (auto& sec : *arr)
        {
            juce::String name = sec[0].toString(); int B = (int) sec[1];
            A.sections.push_back ({ name, off, B });
            for (int bi = 0; bi < B; ++bi)
            {
                int src = bi % L; auto m = maskFor (name, bi, B); double shift = (off + bi - src) * 16.0;
                A.chords.push_back (res.chords[(size_t) src]);
                const int melodic[3] = { kChords, kBass, kMelody };
                const bool on[3] = { m.chords, m.bass, m.melody };
                for (int q = 0; q < 3; ++q)
                    if (on[q]) for (auto n : perBar[(size_t) melodic[q]][(size_t) src]) { n.s += shift; A.tracks[(size_t) melodic[q]].push_back (n); }
                if (m.drums != 0)
                    for (auto n : perBar[kDrums][(size_t) src])
                    {
                        if (m.drums == 1 && n.n != kGM_kick && n.n != kGM_chat) continue;
                        if (m.drums == 3 && n.n != kGM_kick && n.n != kGM_chat && n.n != kGM_ohat) continue;
                        n.s += shift; A.tracks[kDrums].push_back (n);
                    }
                if (m.roll >= 0)
                {
                    std::vector<int> steps;
                    if (m.roll == 0) steps = { 0, 4, 8, 12 }; else if (m.roll == 1) steps = { 0, 2, 4, 6, 8, 10, 12, 14 }; else for (int s = 0; s < 16; ++s) steps.push_back (s);
                    for (int s : steps) { NoteEv e; e.s = (off + bi) * 16 + s; e.l = .9; e.n = rollNote; e.v = (int) clampT (jsRound (58 + ((m.roll * 16 + s) / 64.0) * 66), 1.0, 127.0); A.tracks[kDrums].push_back (e); }
                }
            }
            off += B;
        }
    A.bars = off; A.steps = off * 16;
    return A;
}

KeyInfo keyInfo (const Data& data, int key, const juce::String& scale)
{
    static const std::map<juce::String, int> offs { { "major", 0 }, { "minor", 3 }, { "dorian", 10 }, { "phrygian", 8 }, { "mixolydian", 5 }, { "harmonic", 3 }, { "hijaz", 8 } };
    KeyInfo k;
    int off = offs.count (scale) ? offs.at (scale) : 3;
    int rp = (key + off) % 12, n = ((7 + (rp * 7) % 12) % 12) + 1;
    juce::String letter = (scale == "major" || scale == "mixolydian") ? "B" : "A", other = letter == "A" ? "B" : "A";
    int up = n % 12 + 1, down = ((n + 10) % 12) + 1;
    k.cam = juce::String (n) + letter;
    k.compat = { juce::String (down) + letter, juce::String (up) + letter, juce::String (n) + other };
    k.sf = ((rp * 7 + 6) % 12) - 6;
    k.mi = (scale == "minor" || scale == "harmonic" || scale == "hijaz") ? 1 : 0;
    for (auto i : intArray (data.scale (scale)["iv"])) k.notes.add (data.noteNames[(key + i) % 12]);
    return k;
}

int tickOf (double s, double sw)
{
    const double STEP = 120;   // 480 / 4
    double sh = (std::floor (s) == s && ((long long) s) % 2 == 1) ? (sw / 100.0) * STEP : 0;
    return (int) jsRound (s * STEP + sh);
}

namespace
{
struct MEv { int t, o; std::vector<uint8_t> b; };
void vlq (std::vector<uint8_t>& out, uint32_t n)
{
    uint8_t buf[5]; int c = 0; buf[c++] = (uint8_t) (n & 0x7f); n >>= 7;
    while (n > 0) { buf[c++] = (uint8_t) ((n & 0x7f) | 0x80); n >>= 7; }
    while (c > 0) out.push_back (buf[--c]);
}
std::vector<uint8_t> str (const juce::String& s) { std::vector<uint8_t> v; auto u = s.toUTF8(); for (auto* p = u.getAddress(); *p; ++p) v.push_back ((uint8_t) *p); return v; }
MEv meta (int t, uint8_t type, const std::vector<uint8_t>& bytes) { MEv e { t, -1, { 0xFF, type } }; vlq (e.b, (uint32_t) bytes.size()); e.b.insert (e.b.end(), bytes.begin(), bytes.end()); return e; }
void u32 (std::vector<uint8_t>& o, uint32_t n) { o.push_back ((uint8_t) (n >> 24)); o.push_back ((uint8_t) (n >> 16)); o.push_back ((uint8_t) (n >> 8)); o.push_back ((uint8_t) n); }
void u16 (std::vector<uint8_t>& o, uint32_t n) { o.push_back ((uint8_t) (n >> 8)); o.push_back ((uint8_t) n); }
std::vector<uint8_t> chunk (const char* id, const std::vector<uint8_t>& body) { std::vector<uint8_t> o (id, id + 4); u32 (o, (uint32_t) body.size()); o.insert (o.end(), body.begin(), body.end()); return o; }
std::vector<uint8_t> pack (std::vector<MEv> ev, int total)
{
    std::stable_sort (ev.begin(), ev.end(), [] (const MEv& a, const MEv& b) { return a.t < b.t || (a.t == b.t && a.o < b.o); });
    std::vector<uint8_t> out; int last = 0;
    for (auto& e : ev) { vlq (out, (uint32_t) (e.t - last)); out.insert (out.end(), e.b.begin(), e.b.end()); last = e.t; }
    vlq (out, (uint32_t) std::max (0, total - last)); out.push_back (0xFF); out.push_back (0x2F); out.push_back (0x00);
    return out;
}
}

juce::MemoryBlock midiFile (const Data& data, const Result& res, const std::vector<int>& keys, bool fmt0)
{
    const auto& P = res.P; const int STEP = 120, total = res.bars * 16 * STEP;
    const juce::String title = "Wavejackers HitGen " + data.noteNames[P.key] + " " + data.scale (P.scale)["n"].toString() + " " + juce::String (P.bpm) + " BPM";
    auto head = [&]
    {
        int us = (int) jsRound (60000000.0 / P.bpm); auto ki = keyInfo (data, P.key, P.scale);
        std::vector<MEv> ev { meta (0, 0x03, str (title)), meta (0, 0x51, { (uint8_t) ((us >> 16) & 255), (uint8_t) ((us >> 8) & 255), (uint8_t) (us & 255) }),
                              meta (0, 0x58, { 4, 2, 24, 8 }), meta (0, 0x59, { (uint8_t) (ki.sf & 255), (uint8_t) ki.mi }) };
        for (size_t b = 0; b < res.chords.size(); ++b) ev.push_back (meta ((int) b * 16 * STEP, 0x06, str (res.chords[b].name)));
        return ev;
    };
    static const int CH[kNumTracks] = { 0, 1, 9, 2 };
    static const char* const TN[kNumTracks] = { "Chords", "Bass", "Drums", "Melody" };
    auto notes = [&] (int k)
    {
        const double H = P.human / 100.0;
        Rng rj ((double) P.seed * 977 + (double) juce::String (kTrackIds[k]).length() * 13);
        std::vector<MEv> ev;
        for (auto& n : res.tracks[(size_t) k])
        {
            int on = tickOf (n.s, P.swing), offT = tickOf (n.s + n.l, P.swing) - 2;
            int jit = (k == kDrums && n.n == kGM_kick) ? 0 : (int) jsRound (rj() * H * STEP * 0.45);
            on = std::min (on + jit, std::max (on, offT - 1));
            if (offT <= on) offT = on + 1;
            ev.push_back ({ on, 1, { (uint8_t) (0x90 | CH[k]), (uint8_t) n.n, (uint8_t) n.v } });
            ev.push_back ({ offT, 0, { (uint8_t) (0x80 | CH[k]), (uint8_t) n.n, 0 } });
        }
        return ev;
    };
    std::vector<std::vector<uint8_t>> trks;
    if (fmt0)
    {
        auto ev = head(); for (int k : keys) { auto e2 = notes (k); ev.insert (ev.end(), e2.begin(), e2.end()); }
        trks.push_back (chunk ("MTrk", pack (ev, total)));
    }
    else
    {
        trks.push_back (chunk ("MTrk", pack (head(), total)));
        for (int k : keys) { std::vector<MEv> ev { meta (0, 0x03, str (TN[k])) }; auto e2 = notes (k); ev.insert (ev.end(), e2.begin(), e2.end()); trks.push_back (chunk ("MTrk", pack (ev, total))); }
    }
    std::vector<uint8_t> hd; u16 (hd, fmt0 ? 0 : 1); u16 (hd, (uint32_t) trks.size()); u16 (hd, 480);
    auto out = chunk ("MThd", hd);
    for (auto& t : trks) out.insert (out.end(), t.begin(), t.end());
    return juce::MemoryBlock (out.data(), out.size());
}
}
