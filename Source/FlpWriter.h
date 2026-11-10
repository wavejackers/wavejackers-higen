// FL Studio projesi (.flp) yazıcı: iskeletten dolu bir playlist.
// Olay düzeni FL Studio 20.9 dosyalarından alınmıştır (FRERE "Powerful" / "Tiger" projeleri; PyFLP ile doğrulanır):
// başlık, pattern notaları, sampler kanalları, aranjman (playlist, işaretçiler, 500 iz), mikser.
// Tüm kanallar Sampler'dır: davul vuruşları, HitGen'in kendi sesleri (C5 kök) ve FX ses klipleri. Eklenti gerektirmez.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Generator.h"
#include <map>
#include <array>

namespace hg
{
struct FlpNote { int pos = 0, len = 24, key = 60, vel = 100; };
struct FlpChannel { juce::String name, samplePath; juce::uint32 colour = 0x5a5a5a; int insert = -1; };
struct FlpPattern { juce::String name; juce::uint32 colour = 0x5a5a5a; int lengthTicks = 0, channel = 0; std::vector<FlpNote> notes; };
struct FlpItem { bool pattern = true; int index = 0, pos = 0, len = 0, track = 0; };   // pattern: 1 tabanlı; değilse kanal (ses klibi)
struct FlpMarker { int pos = 0; juce::String name; };
struct FlpTrack { juce::String name; juce::uint32 colour = 0x8a6e45; };

struct FlpProject
{
    int ppq = 96; double bpm = 120;
    juce::String title, genre, comment;
    std::vector<FlpChannel> channels;
    std::vector<FlpPattern> patterns;
    std::vector<FlpItem> items;
    std::vector<FlpMarker> markers;
    std::vector<FlpTrack> tracks;
};

juce::MemoryBlock writeFlp (const FlpProject& p);

// Basit okuyucu (testler ve doğrulama): olay kimlikleri ve değerleri
struct FlpEvent { int id = 0; juce::uint32 value = 0; juce::MemoryBlock data; };
bool readFlpEvents (const juce::MemoryBlock& file, int& ppq, int& numChannels, std::vector<FlpEvent>& out);

// İskeletten FL projesi. sounds: davul pad'leri (GM notası -> wav), parçalar (akor/bas/melodi -> wav, C5 kök),
// fx: ipucu başına ses klibi (wav ve saniye cinsinden uzunluk). Yollar mutlak olmalı.
struct FlpSources
{
    std::map<juce::String, juce::String> lanes;        // şerit rolü (kick, bass, pad, arp...) -> wav yolu (melodik: C5 kök)
    struct Fx { SkCue cue; juce::String path; double seconds = 0; bool endAligned = false; };
    std::vector<Fx> fx;
};
struct Arrangement;
FlpProject buildFlp (const Arrangement& a, const FlpSources& src, const juce::String& title);
}
