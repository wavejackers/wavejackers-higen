// Ses ön dinlemesi: seçili sesin kategorisine uygun iki ölçülük kısa cümle (La minör).
// Tablolar sabittir; ses iş parçacığı yalnızca okur (bellek ayırmaz, kilit yok).
#pragma once
#include <juce_core/juce_core.h>
#include <array>

namespace hg
{
struct DemoNote { float beat, len; int note, vel; };
enum DemoKind { dmSub, dmRoll, dmAcid, dmBass, dmLead, dmPluck, dmStab, dmSeq, dmPad, dmTex, dmFx, dmHit, kNumDemo };
constexpr float kDemoBeats = 8.0f;

struct DemoTable
{
    std::array<std::array<DemoNote, 64>, kNumDemo> notes {};
    std::array<int, kNumDemo> count {};
    DemoTable()
    {
        auto add = [this] (int k, float b, float l, int n, int v) { if (count[(size_t) k] < 64) notes[(size_t) k][(size_t) count[(size_t) k]++] = { b, l, n, v }; };
        // sub: uzun notalar, A1 bölgesi
        for (auto [b, l, n] : { std::tuple<float, float, int> { 0, 1.75f, 33 }, { 2, .75f, 33 }, { 3, .75f, 36 }, { 4, 1.75f, 29 }, { 6, .75f, 29 }, { 7, .75f, 31 } }) add (dmSub, b, l, n, 100);
        // rolling: kick arası 1/16'lar
        for (int b = 0; b < 8; ++b) for (int s = 1; s < 4; ++s) add (dmRoll, b + s * .25f, .2f, b >= 6 ? 36 : 33, s == 2 ? 112 : 88);
        // acid: 1/16 hat, kaymalar (üst üste binen notalar) ve aksanlar
        const int ac[16] = { 45, 45, 57, 45, 48, 45, 52, 45, 45, 57, 45, 55, 45, 48, 45, 43 };
        const bool slide[16] = { 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0 }, acc[16] = { 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0 };
        for (int r = 0; r < 2; ++r) for (int i = 0; i < 16; ++i) add (dmAcid, r * 4 + i * .25f, slide[i] ? .3f : .16f, ac[i], acc[i] ? 122 : 84);
        // genel bas: off-beat
        for (int b = 0; b < 8; ++b) add (dmBass, b + .5f, .4f, b >= 4 ? 29 : 33, 100);
        // lead: La minör cümle
        for (auto [b, l, n] : { std::tuple<float, float, int> { 0, 1.5f, 64 }, { 1.5f, .5f, 62 }, { 2, 1, 60 }, { 3, 1, 57 }, { 4, 1.5f, 60 }, { 5.5f, .5f, 62 }, { 6, 1.75f, 64 } }) add (dmLead, b, l, n, 100);
        // pluck: 1/8 arpej
        const int ar[8] = { 57, 60, 64, 67, 72, 67, 64, 60 };
        for (int i = 0; i < 16; ++i) add (dmPluck, i * .5f, .4f, ar[i % 8] - (i >= 8 ? 4 : 0), 96);
        // stab: off-beat akorlar (Am, F)
        for (int b = 0; b < 8; ++b) for (int n : b < 4 ? std::array<int, 3> { 57, 60, 64 } : std::array<int, 3> { 53, 57, 60 }) add (dmStab, b + .5f, .3f, n, 100);
        // sekans: 1/16
        const int sq[16] = { 57, 57, 69, 57, 64, 57, 67, 57, 57, 60, 69, 57, 64, 57, 72, 57 };
        for (int r = 0; r < 2; ++r) for (int i = 0; i < 16; ++i) add (dmSeq, r * 4 + i * .25f, .2f, sq[i], i % 4 == 0 ? 110 : 84);
        // pad: iki akor
        for (int n : { 57, 60, 64, 67 }) add (dmPad, 0, 3.9f, n, 90);
        for (int n : { 53, 57, 60, 64 }) add (dmPad, 4, 3.9f, n, 90);
        // doku: tek uzun akor
        for (int n : { 45, 52, 57, 60 }) add (dmTex, 0, 7.8f, n, 90);
        // fx: tek uzun nota; vuruş: iki darbe
        add (dmFx, 0, 7.8f, 57, 100);
        add (dmHit, 0, .5f, 45, 115); add (dmHit, 4, .5f, 45, 115);
    }
};

// Kategori (ve sesin "demo" alanı) -> cümle türü
inline int demoKindFor (const juce::String& cat, const juce::String& demoField)
{
    if (demoField == "fx") return dmFx;
    if (demoField == "hit") return dmHit;
    if (cat == "Sub") return dmSub;
    if (cat == "Rolling") return dmRoll;
    if (cat == "Acid") return dmAcid;
    if (cat == "Bas") return dmBass;
    if (cat == "Lead" || cat == "Vox") return dmLead;
    if (cat == "Pluck") return dmPluck;
    if (cat == "Stab" || cat == "Keys") return dmStab;
    if (cat == "Seq" || cat == "Metal") return dmSeq;
    if (cat == "Pad") return dmPad;
    if (cat == "Texture") return dmTex;
    if (cat == "FX") return dmFx;
    return dmLead;
}
}
