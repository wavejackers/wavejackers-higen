// Wavejackers HitGen: stil tabanlı MIDI üretici (web sürümüyle birebir aynı algoritma)
#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <cstdint>

namespace hg
{
enum Track { kChords = 0, kBass = 1, kDrums = 2, kMelody = 3, kNumTracks = 4 };
extern const char* const kTrackIds[kNumTracks];

struct NoteEv { double s = 0, l = 0; int n = 0, v = 0; };

struct ChordInfo
{
    int d = 0, pc = 0, third = 0, fifth = 0;
    std::vector<int> midi;
    juce::String name, roman;
};

struct GenParams
{
    juce::String style = "wavejackers";
    int vi = 0, seed = 7, bars = 8, key = 9, bpm = 126;
    juce::String scale;            // boş = stil varsayılanı
    double orig = 30, energy = 60, human = 20, swing = 0, thick = 70;
    bool sevenths = false;
};

struct Result
{
    GenParams P;                   // P.scale çözülmüş haliyle
    std::vector<ChordInfo> chords;
    std::array<std::vector<NoteEv>, kNumTracks> tracks;
    int bars = 0, steps = 0;
    struct Section { juce::String name; int bar = 0, bars = 0; };
    std::vector<Section> sections;   // yalnızca tam parça aranjmanında dolu
};

// Mulberry32: JS sürümündeki MB.rng ile aynı diziyi üretir
struct Rng
{
    uint32_t a;
    explicit Rng (double seed) : a ((uint32_t) (uint64_t) std::llround (seed)) {}
    double operator()() noexcept
    {
        a += 0x6D2B79F5u;
        uint32_t t = a;
        t = (t ^ (t >> 15)) * (t | 1u);
        t ^= t + (t ^ (t >> 7)) * (t | 61u);
        return (double) (t ^ (t >> 14)) / 4294967296.0;
    }
};

class Data;   // JSON veri kümesi (Data.h)

Result generate (const Data& data, const GenParams& p);
Result assemble (const Data& data, const Result& loop);

struct KeyInfo { juce::String cam; juce::StringArray compat; int sf = 0, mi = 0; juce::StringArray notes; };
KeyInfo keyInfo (const Data& data, int key, const juce::String& scale);

int tickOf (double s, double swing);   // PPQ 480
juce::MemoryBlock midiFile (const Data& data, const Result& res, const std::vector<int>& tracks, bool format0);
}
