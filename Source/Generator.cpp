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
    Rng rc ((double) P.seed * 1297 + 3 + vi + P.alt[kChords] * 50021);
    const juce::String cmode = orStr (V["cm"], S["chord"]["mode"].toString());
    std::vector<juce::String> cpKeys;
    if (has (V, "cp")) cpKeys.push_back (V["cp"].toString());
    else if (auto* a = S["chord"]["pats"].getArray()) for (auto& x : *a) cpKeys.push_back (x.toString());
    std::vector<std::vector<std::vector<double>>> cpats;
    for (auto& k : cpKeys) cpats.push_back (rows (data.root()["cp"][juce::Identifier (k)]));
    auto arpSeq = intArray (data.root()["arp"][juce::Identifier (orStr (S["chord"]["arp"], "trance"))]);
    for (int b = 0; b < bars; ++b)
    {
        const int base = b * 16; auto ch = R.chords[(size_t) b].midi;
        if (P.alt[kChords] != 0)   // "başka öner": her ölçüde farklı ters çevirme (alt = 0 iken hiçbir şey değişmez)
        {
            Rng ra ((double) P.seed * 17 + (double) P.alt[kChords] * 9973 + (double) b * 131);
            const int inv = (int) std::floor (ra() * 3);
            for (int k = 0; k < inv && ! ch.empty(); ++k) { ch.push_back (ch.front() + 12); ch.erase (ch.begin()); }
        }
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
    Rng rb ((double) P.seed * 3571 + 5 + vi + P.alt[kBass] * 50021);
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
    Rng rd ((double) P.seed * 6151 + 9 + vi + P.alt[kDrums] * 50021);
    std::map<juce::String, juce::String> D;
    auto mergeDrums = [&] (const juce::var& o) { if (auto* obj = o.getDynamicObject()) for (auto& p : obj->getProperties()) D[p.name.toString()] = p.value.toString(); };
    mergeDrums (S["drums"]); mergeDrums (V["dr"]);
    struct Lane { const char* id; int note, bv; };
    const Lane lanes[] = { { "kick", kGM_kick, 118 }, { "clap", kGM_clap, 100 }, { "snare", kGM_snare, 106 }, { "chat", kGM_chat, 80 }, { "ohat", kGM_ohat, 90 }, { "rim", kGM_rim, 70 }, { "perc", kGM_perc, 80 } };
    auto drumAt = [&] (double s, int n) { for (auto& x : T[kDrums]) if (x.s == s && (n < 0 || x.n == n)) return true; return false; };
    const juce::String fill = D.count ("fill") ? D["fill"] : juce::String();
    const juce::String hacc = D.count ("hacc") ? D["hacc"] : juce::String();
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
                if (lane == "chat") v = hacc == "off" ? (s % 4 == 2 ? 92 : s % 2 == 0 ? 68 : 56) : s % 4 == 0 ? 92 : s % 2 == 0 ? 78 : 58;
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
        if (fill == "perc" && b % 4 == 3 && b > 0 && bars > 4)
        {
            const bool big = b % 8 == 7; const int from = big ? 8 : 12; const double pr = big ? .75 : .6;
            for (int s = from; s < 16; ++s)
                if (rd() < pr)
                {
                    const int nn = rd() < .3 ? kGM_rim : kGM_perc;
                    if (! drumAt (base + s, nn)) add (kDrums, base + s, .9, nn, 52 + std::floor (rd() * 44));
                }
        }
        else if (fill != "none" && fill != "perc" && b % 4 == 3 && b > 0 && bars > 4)
        {
            int note = fill == "snare" ? kGM_snare : kGM_clap;
            int i = 0;
            for (int s : { 12, 13, 14, 15 }) { if (rd() < .8 && ! drumAt (base + s, note)) add (kDrums, base + s, .9, note, 70 + i * 12); ++i; }
        }
    }

    // ---- melodi izi
    Rng rm ((double) P.seed * 104729 + 7 + vi * 5 + P.alt[kMelody] * 50021);
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
        // Tek akılda kalan motif ve cevap (A B A B'): web h-gen.js ile birebir aynı (RNG çağrı sırası dahil)
        std::vector<juce::String> hrk;
        auto hv2 = has (V, "hr") ? V["hr"] : mel["hr"];
        if (auto* a = hv2.getArray()) for (auto& x : *a) hrk.push_back (x.toString());
        const auto tp = rows (data.root()["hr"][juce::Identifier (hrk[(size_t) std::floor (rm() * (double) hrk.size())])]);
        const double rep = has (mel, "rep") ? num (mel["rep"]) : .4;   // özgünlük melodiye eklenmez
        const double keep = clampT (.5 + dens, .55, 1.0);
        std::vector<std::vector<double>> hits;
        for (size_t j = 0; j < tp.size(); ++j) if (j == 0 || rm() <= keep) hits.push_back (tp[j]);
        const int n = (int) hits.size();
        std::vector<int> strong; for (int j = 1; j < n; ++j) if (((int) hits[(size_t) j][0]) % 4 == 0) strong.push_back (j);
        int peak;
        if (n < 2 || rm() < .25) peak = 0;
        else if (! strong.empty()) peak = strong[(size_t) std::floor (rm() * (double) strong.size())];
        else peak = 1 + (int) std::floor (rm() * (n - 1));
        std::vector<int> rel { 0 }; int prevSt = 1;
        for (int j = 1; j < n; ++j)
        {
            const double r = rm(); int st;
            if (j < peak) st = r < .6 ? 1 : r < .85 ? 0 : 2;
            else if (j == peak) st = r < .5 ? 2 : r < .8 ? 3 : 4;
            else st = r < rep * .4 ? 0 : r < .75 ? -1 : -2;
            if (st == 0 && prevSt == 0) st = j <= peak ? 1 : -1;   // aynı nota en çok iki kez üst üste
            prevSt = st; rel.push_back (peak == 0 ? clampT (rel[(size_t) j - 1] + st, -5, 0) : clampT (rel[(size_t) j - 1] + st, -2, 5));
        }
        const int A0 = (peak == 0 ? 6 : 3) + (int) std::floor (rm() * 3);   // inen hook daha yüksekten başlar
        auto snapFix = [] (int m, const std::vector<int>& tones)
        {
            auto in = [&] (int x) { return std::find (tones.begin(), tones.end(), mod7 (x)) != tones.end(); };
            for (int d = 0; d <= 3; ++d) { if (in (m - d)) return m - d; if (in (m + d)) return m + d; }
            return m;
        };
        const bool dropLast = n > 2 && peak != n - 1;   // cevap son notayı düşürür (tepe son notaysa düşmez)
        for (int b = 0; b < bars; ++b)
        {
            const auto& c = R.chords[(size_t) b]; const int role = b % 4; const bool cad = b == bars - 1;
            const int anchor = snapFix (A0, cTones (c)), seqShift = role == 3 ? -1 : 0;
            const int cnt = ((role == 1 || role == 3) && dropLast && ! cad) ? n - 1 : n;
            for (int j = 0; j < cnt; ++j)
            {
                int dg = clampT (anchor + rel[(size_t) j] + seqShift, 0, 10);
                if (j == cnt - 1)
                {
                    const auto tones = cad ? std::vector<int> { c.d % 7 } : role == 1 ? std::vector<int> { (c.d + 2) % 7, (c.d + 4) % 7 } : cTones (c);
                    dg = snapFix (dg, tones);
                    const int pk = clampT (anchor + rel[(size_t) peak] + seqShift, 0, 10);
                    if (j != peak && dg > pk) { dg = pk - 1; while (dg > pk - 7 && std::find (tones.begin(), tones.end(), mod7 (dg)) == tones.end()) --dg; }   // tepeyi aşmaz
                }
                const double s = hits[(size_t) j][0], nx = j + 1 < cnt ? hits[(size_t) j + 1][0] : 16;
                const double l = cad && j == cnt - 1 ? 16 - s : std::min (hits[(size_t) j][1], nx - s);
                addM (b, s, l, mbase + deg (dg), j == peak ? 108 : ((int) s) % 4 == 0 ? 100 : 84);
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
            const int shift = (b % 8 >= 4) ? 1 : 0;
            for (int s = 0; s < 16; ++s)
            {
                int ix = seq[(size_t) ((s + shift) % (int) seq.size())]; if (ix < 0) continue;
                if (rm() < (1 - clampT (dens, 0.0, 1.0)) * .55) continue;
                int m = e2[(size_t) (ix % (int) e2.size())];
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
            if (b % 4 == 3 && riff.size() > 2 && rm() < .4) { size_t i = (size_t) std::floor (rm() * (double) riff.size()); riff[i] = { riff[i].first, pickFrom (rm, offs) }; }
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
    else if (mmode == "motif")
    {
        // kulüp hook'u (JS ile aynı): 2-5 notalık tek ölçülük motif, çift ölçüde cevap (son nota boş), 4. ölçüde dönüş
        const std::vector<int> cand { 0, 3, 6, 8, 10, 11, 14 };
        const int cnt = clampT ((int) jsRound (2 + dens * 4), 2, 5);
        auto pool = cand; std::vector<int> hits;
        while ((int) hits.size() < cnt && ! pool.empty()) { size_t i = (size_t) std::floor (rm() * (double) pool.size()); hits.push_back (pool[i]); pool.erase (pool.begin() + (long) i); }
        std::stable_sort (hits.begin(), hits.end());
        std::vector<int> shape, lens;
        for (size_t j = 0; j < hits.size(); ++j) shape.push_back (pickFrom (rm, std::vector<int> { 0, 0, 2, 4, 7, -1 }));
        for (size_t j = 0; j < hits.size(); ++j) lens.push_back (rm() < .7 ? 1 : 2);
        const int resp = pickFrom (rm, std::vector<int> { 2, -1, 4 });
        for (int b = 0; b < bars; ++b)
        {
            const auto& c = R.chords[(size_t) b];
            const int n = (b % 2 == 1 && hits.size() > 2) ? (int) hits.size() - 1 : (int) hits.size();
            for (int j = 0; j < n; ++j)
            {
                const int s = hits[(size_t) j]; const double nx = j + 1 < n ? hits[(size_t) j + 1] : 16;
                const int dg = c.d + shape[(size_t) j] + ((b % 4 == 3 && j == n - 1) ? resp : 0);
                addM (b, s, std::min ((double) lens[(size_t) j], nx - s), mbase + deg (dg), s % 4 == 0 ? 104 : 90);
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
struct Mask { int drums = 2; bool bass = true, chords = true, melody = true; int roll = -1; };   // drums: 0 yok, 1 hafif, 2 tam, 3 build, 4 yalnızca kapalı hihat
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
    std::vector<ArrSec> arr;
    if (auto* a = data.styleById (res.P.style)["arr"].getArray())
        for (auto& sec : *a) arr.push_back ({ sec[0].toString(), (int) sec[1], {} });
    return assembleArr (data, res, arr, {});
}

Result assembleArr (const Data& data, const Result& res, const std::vector<ArrSec>& arr, const juce::String& profile)
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
    if (L > 0)
        for (size_t si = 0; si < arr.size(); ++si)
        {
            const auto& sec = arr[si];
            const juce::String& name = sec.name; const int B = sec.bars; const juce::String& kind = sec.kind;
            const juce::String nk = si + 1 < arr.size() ? arr[si + 1].kind : juce::String();
            A.sections.push_back ({ name, off, B });
            for (int bi = 0; bi < B; ++bi)
            {
                int src = bi % L; auto m = maskFor (name, bi, B); double shift = (off + bi - src) * 16.0;
                double vel = 1; bool clapIn = false;
                if (profile == "organic" && kind == "break") { m.drums = 1; vel = .6; }                 // Black Coffee: kick kesilmez, inceltilir
                if (profile == "techno" && kind == "intro" && B >= 16 && bi < 8) m.drums = 4;           // FRERE: ilk 8 ölçü kicksiz
                if (profile == "techno" && (kind == "drop" || kind == "groove") && B >= 16 && (nk == "break" || nk == "build") && bi >= B - 8) clapIn = true;
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
                        if (m.drums == 4 && n.n != kGM_chat) continue;
                        n.s += shift;
                        if (vel != 1) n.v = (int) clampT (jsRound (n.v * vel), 1.0, 127.0);
                        A.tracks[kDrums].push_back (n);
                    }
                if (clapIn)
                    for (int st : { 4, 12 })
                    {
                        bool has = false;
                        for (auto& n : perBar[kDrums][(size_t) src]) if (n.n == kGM_clap && n.s - src * 16 == st) has = true;
                        if (! has) { NoteEv e; e.s = (off + bi) * 16 + st; e.l = 1; e.n = kGM_clap; e.v = 96; A.tracks[kDrums].push_back (e); }
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

// ---- Proje iskeleti: Black Coffee (Ableton) ve FRERE (FL Studio) projelerinden çıkarılan aranjman kuralları
const char* const kSkKinds[kNumSkKinds] = { "intro", "build", "drop", "break", "groove", "outro", "verse" };
static int skWeight (const juce::String& k)
{
    if (k == "build") return 2;
    if (k == "break" || k == "verse") return 3;
    if (k == "intro" || k == "drop" || k == "groove" || k == "outro") return 4;
    return 3;
}
juce::String skName (const juce::String& k)
{
    if (k == "intro") return "Intro";
    if (k == "build") return "Buildup";
    if (k == "drop") return "Drop";
    if (k == "break") return "Breakdown";
    if (k == "groove") return "Groove";
    if (k == "outro") return "Outro";
    if (k == "verse") return "Verse";
    return k;
}
juce::String skProfile (const juce::var& style)
{
    const auto g = style["grp"].toString();
    if (g == "afro" || g == "house") return "organic";
    if (g == "techno" || g == "trance" || g == "hard") return "techno";
    return "edm";
}
std::vector<int> skBars (const juce::StringArray& kinds, int sec, int bpm)
{
    const int n = kinds.size(); if (n == 0) return {};
    const long long P = std::max<long long> (n, ((long long) sec * bpm + 960) / 1920), rem = P - n;
    std::vector<long long> w, ph, fr; long long W = 0;
    for (auto& k : kinds) { w.push_back (skWeight (k)); W += w.back(); }
    long long sum = 0;
    for (int i = 0; i < n; ++i) { ph.push_back (1 + rem * w[(size_t) i] / W); fr.push_back ((rem * w[(size_t) i]) % W); sum += ph.back(); }
    long long left = P - sum; std::vector<bool> used ((size_t) n, false);
    while (left > 0)
    {
        int bi = -1;
        for (int i = 0; i < n; ++i) if (! used[(size_t) i] && (bi < 0 || fr[(size_t) i] > fr[(size_t) bi])) bi = i;
        if (bi < 0) break;
        used[(size_t) bi] = true; ++ph[(size_t) bi]; --left;
    }
    for (int i = 0; i < n; ++i)
        if (kinds[i] == "build" && ph[(size_t) i] > 2)
        {
            int j = -1;
            for (int k = i + 1; k < n && j < 0; ++k) if (kinds[k] != "build") j = k;
            for (int k = i - 1; k >= 0 && j < 0; --k) if (kinds[k] != "build") j = k;
            if (j >= 0) { ph[(size_t) j] += ph[(size_t) i] - 2; ph[(size_t) i] = 2; }
        }
    std::vector<int> out; for (auto x : ph) out.push_back ((int) (x * 8));
    return out;
}
std::vector<SkSection> skSections (const juce::StringArray& kinds, const std::vector<int>& bars)
{
    std::map<juce::String, int> tot, c; for (auto& k : kinds) ++tot[k];
    std::vector<SkSection> out; int off = 0;
    for (int i = 0; i < kinds.size() && i < (int) bars.size(); ++i)
    {
        const auto& k = kinds[i]; ++c[k];
        out.push_back ({ k, skName (k) + (tot[k] > 1 ? " " + juce::String (c[k]) : juce::String()), off, bars[(size_t) i] });
        off += bars[(size_t) i];
    }
    return out;
}
std::vector<SkCue> skCues (const std::vector<SkSection>& secs, const juce::String& profile)
{
    std::vector<SkCue> C;
    const int total = secs.empty() ? 0 : secs.back().bar + secs.back().bars;
    auto add = [&] (int bar, int len, const char* type, int i) { if (bar >= 0 && bar < total && len > 0) C.push_back ({ bar, std::min (len, total - bar), type, i }); };
    int drops = 0;
    if (profile == "organic") add (0, total, "pad", 0);
    for (int i = 0; i < (int) secs.size(); ++i)
    {
        const auto& x = secs[(size_t) i];
        const int s = x.bar, B = x.bars; const auto& k = x.kind;
        const juce::String pk = i > 0 ? secs[(size_t) i - 1].kind : juce::String(), nk = i + 1 < (int) secs.size() ? secs[(size_t) i + 1].kind : juce::String();
        const bool main = k == "drop" || k == "groove";
        if (k == "drop") ++drops;
        if (profile == "organic")
        {
            if (s > 0) { if (s >= 4) add (s - 4, 4, "reverse", i); add (s, 4, "downlifter", i); }
            if (k == "intro") { add (s, B, "kickIn", i); if (B >= 16) add (s + B / 2, B / 2, "perc", i); }
            if (k == "build") { add (s, B, "noise", i); add (s, B, "filterOpen", i); }
            if (main && (pk == "build" || pk == "break")) add (s - 1, 1, "reverbThrow", i);
            if (k == "break") { add (s, B, "kickHpf", i); add (s, B, "bassOut", i); add (s, B, "chordsFilter", i); if (B >= 16) add (s + B / 2, B / 2, "topMelody", i); }
            if (main) { add (s, B, "percUp", i); if (B >= 16) add (s + B / 2, B / 2, "leadVar", i); }
            if (k == "outro") { add (s + B / 4, B - B / 4, "percOut", i); add (s + B / 2, B / 2, "filterClose", i); }
        }
        else if (profile == "techno")
        {
            if (k == "intro") { add (s, B, "atmos", i); if (B >= 16) { add (s, 8, "hatsOnly", i); add (s + 8, B - 8, "kickIn", i); } else add (s, B, "kickIn", i); }
            if (main)
            {
                if (s > 0) add (s, 1, "impact", i);
                for (int m = s + 4; m + 4 <= s + B; m += 8) add (m, 4, "uplifter", i);
                if (B >= 16 && (nk == "break" || nk == "build")) add (s + B - 8, 8, "clapIn", i);
                if (k == "drop" && drops >= 2 && pk == "build") add (s - 1, 1, "gap", i);
            }
            if (k == "break") { add (s, 1, "impact", i); add (s, 4, "subDrop", i); add (s, std::min (8, B), "downlifter", i); }
            if (k == "build") { add (s, B, "riser", i); add (s, B, "sweep", i); add (s, B, "drumBuild", i); add (s + B - 1, 1, "vocal", i); }
            if (k == "outro") { add (s, std::min (8, B), "downlifter", i); add (s + B / 2, B / 2, "percOut", i); }
        }
        else
        {
            if (k == "intro") add (s, B, "kickIn", i);
            if (k == "build") { add (s, B, "riser", i); add (s + B - 4, 4, "roll", i); add (s + B - 1, 1, "fill", i); }
            if (main && s > 0) { add (s, 1, "impact", i); if (k == "drop") add (s, 4, "subDrop", i); }
            if (k == "break") { add (s, 4, "downlifter", i); add (s, B, "atmos", i); }
            if (k == "outro") add (s + B / 2, B / 2, "percOut", i);
        }
    }
    std::stable_sort (C.begin(), C.end(), [] (const SkCue& a, const SkCue& b) { return a.bar < b.bar; });
    return C;
}
Skeleton skeleton (const Data& data, const Result& res, const juce::StringArray& kinds, int sec, const std::vector<int>* bars)
{
    Skeleton k;
    const auto b = bars != nullptr ? *bars : skBars (kinds, sec, res.P.bpm);
    k.secs = skSections (kinds, b); k.profile = skProfile (data.styleById (res.P.style));
    k.cues = skCues (k.secs, k.profile);
    std::vector<ArrSec> arr; for (auto& x : k.secs) arr.push_back ({ x.name, x.bars, x.kind });
    k.song = assembleArr (data, res, arr, k.profile);
    return k;
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
        for (const auto& sc : res.sections) ev.push_back (meta (sc.bar * 16 * STEP, 0x06, str (sc.name)));   // bölüm işaretçileri (tam parça)
        for (const auto& mk : res.marks) ev.push_back (meta (mk.bar * 16 * STEP, 0x06, str (mk.name)));       // iskelet ipuçları (yalnızca eklenti dışa aktarımı)
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
