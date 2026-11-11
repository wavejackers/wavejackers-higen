// HitGen 3.0 aranjman veri modeli: iskeletten (Skeleton) okunur, tüm dışa aktarımlar (çok izli MIDI, manifest,
// FL Piano Roll script'leri, stem'ler) bu tek modeli kullanır. Üretim algoritmasını değiştirmez.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Generator.h"

namespace hg
{
class Data;

// Şerit otomasyonu (yalnızca MIDI dosyasına yazılır): pitch bend (0..16383, 8192 = orta) ya da MIDI CC
struct ArrAuto { double s = 0; int kind = 0, value = 0; };   // kind: -1 = pitch bend, 0..127 = CC numarası; s: 16'lık adım
struct ArrLane
{
    juce::String name, role;      // role: kick, clap, snare, hat, ohat, rim, perc, bass, chords, melody
    int part = 0;                 // kaynak iz: kChords, kBass, kDrums, kMelody
    int gm = -1;                  // davul şeridi: GM nota; melodik şerit: -1
    int midiChannel = 0;          // 0 tabanlı (davullar 9)
    std::vector<NoteEv> notes;    // 16'lık adım cinsinden, tam şarkı zamanında
    std::vector<ArrAuto> autos;   // pitch bend / CC (Stutter şeridi: buildup yükselişi)
    int bendRange = 0;            // autos'ta pitch bend varsa: yarım ton aralığı (RPN 0'a yazılır)
};

struct Arrangement
{
    GenParams P;                  // stil, ton, makam, tempo, tohum, swing
    juce::String profile;         // organic / techno / edm
    int bars = 0;
    std::vector<SkSection> secs;
    std::vector<SkCue> cues;
    std::vector<ArrLane> lanes;
    double seconds() const { return P.bpm > 0 ? bars * 240.0 / P.bpm : 0.0; }
    double secondsAt (double step) const { return P.bpm > 0 ? step * 15.0 / P.bpm : 0.0; }
};

Arrangement buildArrangement (const Skeleton& sk);

// Type-1 çok izli MIDI (PPQ 480): iletken izinde tempo, ölçü, ton, bölüm ve ipucu işaretçileri; her şeritte iz adı ve notalar.
juce::MemoryBlock arrangementMidi (const Data& data, const Arrangement& a);

// Proje manifesti (JSON): DAW köprüleri ve kullanıcı için tek gerçek kaynak. files: göreli yollar (ör. "Stems/Bass.wav").
juce::String arrangementManifest (const Data& data, const Arrangement& a, const juce::StringPairArray& files);

// FL Studio Piano Roll script'i (.pyscript): notaları ve bölüm işaretçilerini açık piyano rulosuna yazar (Image-Line resmi API).
juce::String flPianoRollScript (const Arrangement& a, const ArrLane& lane, const juce::String& songName);
}
