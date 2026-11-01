// Üretici tutarlılık testi: her stil/varyasyon/tohum için MIDI baytlarının özetini yazar.
// tools/gen-reference.js aynı listeyi web motoruyla üretir; iki çıktı birebir aynı olmalı.
#include <juce_core/juce_core.h>
#include "../Source/Data.h"
#include "../Source/Generator.h"
#include <iostream>

static juce::uint32 fnv (const juce::MemoryBlock& m)
{
    juce::uint32 h = 2166136261u;
    auto* p = static_cast<const juce::uint8*> (m.getData());
    for (size_t i = 0; i < m.getSize(); ++i) { h ^= p[i]; h *= 16777619u; }
    return h;
}

int main (int argc, char** argv)
{
    juce::ignoreUnused (argc, argv);
    const auto& D = hg::Data::get();
    if (! D.ok()) { std::cerr << "data failed to load\n"; return 1; }
    const std::vector<int> all { hg::kChords, hg::kBass, hg::kDrums, hg::kMelody };
    int n = 0;
    for (int si = 0; si < D.numStyles(); ++si)
    {
        const auto& S = D.style (si);
        for (int vi = 0; vi < 4; ++vi)
            for (int seed : { 1, 7, 42, 999, 54321 })
                for (int sev = 0; sev < 2; ++sev)
                    for (int thick : { 0, 70, 100 })
                    {
                        hg::GenParams p;
                        p.style = S["id"].toString(); p.vi = vi; p.seed = seed; p.bars = 8; p.key = (int) S["key"];
                        p.orig = 30; p.energy = 60; p.human = 20; p.sevenths = sev == 1; p.thick = thick;
                        p.bpm = (int) S["bpmDef"]; p.swing = (double) S["swing"];
                        auto r = hg::generate (D, p);
                        auto a = hg::assemble (D, r);
                        std::cout << p.style << ' ' << vi << ' ' << seed << ' ' << sev << ' ' << thick << ' '
                                  << fnv (hg::midiFile (D, r, all, false)) << ' ' << fnv (hg::midiFile (D, r, all, true)) << ' '
                                  << fnv (hg::midiFile (D, a, all, false)) << '\n';
                        ++n;
                    }
    }
    // "başka öner" değişkenleri
    const int alts[5][4] = { { 3, 0, 0, 0 }, { 0, 5, 0, 0 }, { 0, 0, 11, 0 }, { 0, 0, 0, 17 }, { 3, 5, 11, 17 } };
    for (int si = 0; si < D.numStyles(); ++si)
    {
        const auto& S = D.style (si);
        for (int vi = 0; vi < 4; ++vi)
            for (int ai = 0; ai < 5; ++ai)
            {
                hg::GenParams p;
                p.style = S["id"].toString(); p.vi = vi; p.seed = 7; p.bars = 8; p.key = (int) S["key"];
                p.orig = 30; p.energy = 60; p.human = 20; p.sevenths = false; p.thick = 70;
                p.bpm = (int) S["bpmDef"]; p.swing = (double) S["swing"];
                for (int k = 0; k < 4; ++k) p.alt[k] = alts[ai][k];
                auto r = hg::generate (D, p);
                auto a = hg::assemble (D, r);
                std::cout << p.style << " alt" << ai << ' ' << vi << ' ' << fnv (hg::midiFile (D, r, all, false)) << ' ' << fnv (hg::midiFile (D, a, all, false)) << '\n';
                ++n;
            }
    }
    std::cerr << n << " cases\n";
    return 0;
}
