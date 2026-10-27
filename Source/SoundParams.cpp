#include "SoundParams.h"
#include <cmath>

namespace hg
{
namespace
{
#define OSC(n, wdef, ldef) \
    { "o" #n "w", "Osc " #n " wave", spChoice, 0, 5, wdef, 0, "wave" }, \
    { "o" #n "oct", "Osc " #n " octave", spInt, -3, 3, 0, 0, "octave" }, \
    { "o" #n "semi", "Osc " #n " semitone", spInt, -12, 12, 0, 0, "semi" }, \
    { "o" #n "fine", "Osc " #n " fine", spFloat, -50, 50, 0, 0, "fine" }, \
    { "o" #n "lvl", "Osc " #n " level", spFloat, 0, 1, ldef, 0, "level" }
#define LFO(n) \
    { "l" #n "rate", "LFO " #n " rate", spFloat, .02f, 20, 1, 2, "rate" }, \
    { "l" #n "depth", "LFO " #n " depth", spFloat, 0, 1, 0, 0, "depth" }, \
    { "l" #n "target", "LFO " #n " target", spChoice, 0, 2, 0, 0, "target" }, \
    { "l" #n "shape", "LFO " #n " shape", spChoice, 0, 3, 0, 0, "shape" }, \
    { "l" #n "sync", "LFO " #n " tempo sync", spChoice, 0, 1, 0, 0, "sync" }

const SpDef kDefs[kNumSp] = {
    OSC (1, 1, .6f), OSC (2, 0, .4f), OSC (3, 0, .3f),
    { "wtA", "Wavetable A", spChoice, 0, 11, 0, 0, "tableA" },
    { "wtB", "Wavetable B", spChoice, 0, 11, 3, 0, "tableB" },
    { "wtMorph", "Wavetable A-B", spFloat, 0, 1, .3f, 0, "morph" },
    { "wtBright", "Wavetable brightness", spFloat, 0, 1, 1, 0, "bright" },
    { "uni", "Unison voices", spInt, 1, 7, 1, 0, "unison" },
    { "uniDet", "Unison spread", spFloat, 0, 60, 0, 0, "spread" },
    { "sub", "Sub", spFloat, 0, 1, 0, 0, "sub" },
    { "noise", "Noise", spFloat, 0, .5f, 0, 0, "noise" },
    { "ftype", "Filter type", spChoice, 0, 2, 0, 0, "filterType" },
    { "fslope", "Filter slope", spChoice, 0, 1, 0, 0, "slope" },
    { "cut", "Filter cutoff", spFloat, 30, 18000, 8000, 1000, "cutoff" },
    { "reso", "Filter resonance", spFloat, 0, 20, .7f, 3, "reso" },
    { "fenv", "Filter env amount", spFloat, 0, 6, 0, 1.5f, "envAmt" },
    { "fatk", "Filter env attack", spFloat, 0, 2, 0, .2f, "envAtk" },
    { "fdec", "Filter env decay", spFloat, .01f, 4, .2f, .4f, "envDec" },
    { "kt", "Filter key tracking", spFloat, 0, 1, .5f, 0, "keytrack" },
    { "fdrive", "Filter drive", spFloat, 0, 1.5f, 0, 0, "drive" },
    { "A", "Attack", spFloat, .001f, 5, .01f, .3f, "attack" },
    { "D", "Decay", spFloat, .01f, 5, .2f, .5f, "decay" },
    { "S", "Sustain", spFloat, 0, 1, .7f, 0, "sustain" },
    { "R", "Release", spFloat, .01f, 8, .3f, .6f, "release" },
    LFO (1), LFO (2),
    { "drive", "Saturation", spFloat, 0, 1, 0, 0, "sat" },
    { "chorus", "Chorus", spFloat, 0, 1, 0, 0, "chorus" },
    { "delay", "Delay", spFloat, 0, 1, 0, 0, "delay" },
    { "reverb", "Reverb", spFloat, 0, 1, 0, 0, "reverb" },
    { "level", "Level", spFloat, 0, .8f, .2f, 0, "volume" },
};
#undef OSC
#undef LFO

const char* const kParts[3] = { "c", "b", "m" };
const char* const kPartNames[3] = { "Chords", "Bass", "Melody" };
const char kWaveCodes[6] = { 0, 'w', 'q', 't', 's', 'x' };

double num (const juce::var& v, double d = 0) { return v.isDouble() || v.isInt() || v.isInt64() || v.isBool() ? (double) v : d; }
const juce::var& at (const juce::var& a, int i) { static const juce::var none; auto* arr = a.getArray(); return arr != nullptr && i >= 0 && i < arr->size() ? arr->getReference (i) : none; }
float clampTo (int i, double x) { const auto& d = kDefs[i]; return (float) juce::jlimit ((double) d.lo, (double) d.hi, x); }
juce::var arr (std::initializer_list<juce::var> l) { juce::Array<juce::var> a; for (auto& x : l) a.add (x); return a; }
float r6 (double x) { return (float) (std::round (x * 1e6) / 1e6); }
}

const SpDef& spDef (int i) { return kDefs[juce::jlimit (0, kNumSp - 1, i)]; }
const char* spPartCode (int part) { return kParts[juce::jlimit (0, 2, part)]; }
juce::String spId (int part, int i) { return juce::String ("sd_") + spPartCode (part) + "_" + kDefs[i].key; }

int spPartOfId (const juce::String& id)
{
    if (! id.startsWith ("sd_") || id.length() < 6) return -1;
    const auto c = id[3];
    return c == 'c' ? 0 : c == 'b' ? 1 : c == 'm' ? 2 : -1;
}

juce::StringArray spChoices (int i)
{
    const juce::String k (kDefs[i].key);
    if (k.endsWith ("w") && k.startsWith ("o")) return { "Off", "Saw", "Square", "Triangle", "Sine", "Wavetable" };
    if (k == "wtA" || k == "wtB") return { "Saw", "Square", "Triangle", "Glass", "Organ", "Bell", "Vowel A", "Vowel O", "Digital", "Narrow pulse", "Hollow", "Wall of sound" };
    if (k == "ftype") return { "Low-pass", "High-pass", "Band-pass" };
    if (k == "fslope") return { "12 dB", "24 dB" };
    if (k.endsWith ("target")) return { "Cutoff", "Pitch", "Volume" };
    if (k.endsWith ("shape")) return { "Sine", "Triangle", "Square", "Saw" };
    if (k.endsWith ("sync")) return { "Off", "On" };
    return {};
}

void addSoundParams (juce::AudioProcessorValueTreeState::ParameterLayout& L)
{
    using namespace juce;
    for (int part = 0; part < 3; ++part)
        for (int i = 0; i < kNumSp; ++i)
        {
            const auto& d = kDefs[i];
            const ParameterID id { spId (part, i), 2 };
            const String name = String (kPartNames[part]) + " " + d.name;
            if (d.kind == spChoice) { L.add (std::make_unique<AudioParameterChoice> (id, name, spChoices (i), (int) d.def)); continue; }
            if (d.kind == spInt) { L.add (std::make_unique<AudioParameterInt> (id, name, (int) d.lo, (int) d.hi, (int) d.def)); continue; }
            NormalisableRange<float> r (d.lo, d.hi);
            if (d.centre > 0) r.setSkewForCentre (d.centre);
            const String key (d.key);
            auto fmt = [key] (float v, int) -> String
            {
                if (key == "cut") return v >= 1000 ? String (v / 1000.0f, 2) + " kHz" : String (roundToInt (v)) + " Hz";
                if (key == "A" || key == "D" || key == "R" || key == "fatk" || key == "fdec") return v < 1 ? String (roundToInt (v * 1000)) + " ms" : String (v, 2) + " s";
                if (key.endsWith ("rate")) return String (v, v < 1 ? 2 : 1) + " Hz";
                if (key.endsWith ("fine") || key == "uniDet") return String (v, 1) + " ct";
                if (key == "reso") return String (v, 1);
                if (key == "fenv") return String (v, 2) + "x";
                return String (roundToInt (v * 100)) + "%";
            };
            L.add (std::make_unique<AudioParameterFloat> (id, name, r, d.def, AudioParameterFloatAttributes().withStringFromValueFunction (fmt)));
        }
}

void spFromSound (const juce::var& v, float* x)
{
    for (int i = 0; i < kNumSp; ++i) x[i] = kDefs[i].def;
    // osilatörler: toplam perde = oktav*1200 + detune; oktav / yarım ses / ince ayar olarak ayrıştır
    const int oscBase[3] = { o1w, o2w, o3w };
    for (int j = 0; j < 3; ++j)
    {
        const auto& L = at (v["o"], j); const int b = oscBase[j];
        if (L.isVoid()) { x[b] = 0; continue; }
        const auto w = at (L, 0).toString();
        int wi = 1; for (int k = 1; k < 6; ++k) if (w.length() == 1 && w[0] == (juce::juce_wchar) kWaveCodes[k]) wi = k;
        const double total = num (at (L, 2)) * 1200.0 + num (at (L, 1));
        const int oct = (int) std::lround (total / 1200.0); const double rem = total - oct * 1200.0;
        const int semi = (int) std::lround (rem / 100.0); const double fine = rem - semi * 100.0;
        x[b] = (float) wi; x[b + 1] = clampTo (b + 1, oct); x[b + 2] = clampTo (b + 2, semi); x[b + 3] = clampTo (b + 3, fine); x[b + 4] = clampTo (b + 4, num (at (L, 3), .6));
    }
    if (auto* w = v["wt"].getArray()) { x[wtA] = clampTo (wtA, num (at (v["wt"], 0))); x[wtB] = clampTo (wtB, num (at (v["wt"], 1), 3)); x[wtMorph] = clampTo (wtMorph, num (at (v["wt"], 2), .3)); x[wtBright] = clampTo (wtBright, num (at (v["wt"], 3), 1)); juce::ignoreUnused (w); }
    if (v["u"].isArray()) { x[uni] = clampTo (uni, num (at (v["u"], 0), 1)); x[uniDet] = clampTo (uniDet, num (at (v["u"], 1))); }
    x[sub] = clampTo (sub, num (v["sub"])); x[noise] = clampTo (noise, num (v["noi"]));
    if (v["f"].isArray())
    {
        const auto& f = v["f"]; const auto t = at (f, 0).toString();
        x[ftype] = t == "highpass" ? 1.0f : t == "bandpass" ? 2.0f : 0.0f;
        x[cut] = clampTo (cut, num (at (f, 1), 8000)); x[reso] = clampTo (reso, num (at (f, 2), .7)); x[fenv] = clampTo (fenv, num (at (f, 3)));
        x[fatk] = clampTo (fatk, num (at (f, 4))); x[fdec] = clampTo (fdec, num (at (f, 5), .2)); x[fslope] = num (at (f, 6)) > 0 ? 1.0f : 0.0f;
    }
    else { x[ftype] = 0; x[cut] = 8000; x[reso] = .7f; x[fenv] = 0; x[fatk] = 0; x[fdec] = .2f; x[fslope] = 0; }
    x[ktrack] = v.hasProperty ("kt") ? clampTo (ktrack, num (v["kt"])) : .5f;
    x[fdrive] = clampTo (fdrive, num (v["fd"]));
    if (v["a"].isArray()) { x[envA] = clampTo (envA, num (at (v["a"], 0), .01)); x[envD] = clampTo (envD, num (at (v["a"], 1), .2)); x[envS] = clampTo (envS, num (at (v["a"], 2), .7)); x[envR] = clampTo (envR, num (at (v["a"], 3), .3)); }
    const int lb[2] = { l1rate, l2rate };
    for (int k = 0; k < 2; ++k)
    {
        const auto& L = at (v["l"], k); const int b = lb[k];
        if (L.isVoid()) { x[b + 1] = 0; continue; }
        const auto tg = at (L, 2).toString(), sh = at (L, 3).toString();
        x[b] = clampTo (b, num (at (L, 0), 1)); x[b + 1] = clampTo (b + 1, num (at (L, 1)));
        x[b + 2] = tg == "pit" ? 1.0f : tg == "amp" ? 2.0f : 0.0f;
        x[b + 3] = sh == "triangle" ? 1.0f : sh == "square" ? 2.0f : sh == "sawtooth" ? 3.0f : 0.0f;
        x[b + 4] = num (at (L, 4)) > 0 ? 1.0f : 0.0f;
    }
    x[fxDrive] = clampTo (fxDrive, num (v["dr"])); x[fxChorus] = clampTo (fxChorus, num (v["ch"])); x[fxDelay] = clampTo (fxDelay, num (v["dl"]));
    x[fxReverb] = clampTo (fxReverb, num (v["rv"])); x[level] = clampTo (level, num (v["vol"], .2));
}

juce::var spToSound (const juce::var& base, const float* x)
{
    std::unique_ptr<juce::DynamicObject> o;
    if (auto* b = base.getDynamicObject()) o = b->clone(); else o = std::make_unique<juce::DynamicObject>();
    juce::Array<juce::var> osc;
    const int oscBase[3] = { o1w, o2w, o3w };
    for (int j = 0; j < 3; ++j)
    {
        const int b = oscBase[j], wi = juce::jlimit (0, 5, (int) std::lround (x[b]));
        if (wi == 0) continue;
        const double octave = std::lround (x[b + 1]) + std::lround (x[b + 2]) / 12.0;
        osc.add (arr ({ juce::String::charToString ((juce::juce_wchar) kWaveCodes[wi]), r6 (x[b + 3]), r6 (octave), r6 (x[b + 4]) }));
    }
    o->setProperty ("o", osc);
    o->setProperty ("wt", arr ({ (int) std::lround (x[wtA]), (int) std::lround (x[wtB]), r6 (x[wtMorph]), r6 (x[wtBright]) }));
    o->setProperty ("u", arr ({ (int) std::lround (x[uni]), r6 (x[uniDet]) }));
    o->setProperty ("sub", r6 (x[sub])); o->setProperty ("noi", r6 (x[noise]));
    const int ft = (int) std::lround (x[ftype]);
    o->setProperty ("f", arr ({ ft == 1 ? "highpass" : ft == 2 ? "bandpass" : "lowpass", r6 (x[cut]), r6 (x[reso]), r6 (x[fenv]), r6 (x[fatk]), r6 (x[fdec]), x[fslope] > .5f ? 1 : 0 }));
    o->setProperty ("kt", r6 (x[ktrack])); o->setProperty ("fd", r6 (x[fdrive]));
    o->setProperty ("a", arr ({ r6 (x[envA]), r6 (x[envD]), r6 (x[envS]), r6 (x[envR]) }));
    juce::Array<juce::var> lfos;
    const int lb[2] = { l1rate, l2rate };
    const char* const tg[3] = { "cut", "pit", "amp" }; const char* const sh[4] = { "sine", "triangle", "square", "sawtooth" };
    for (int k = 0; k < 2; ++k)
    {
        const int b = lb[k];
        lfos.add (arr ({ r6 (x[b]), r6 (x[b + 1]), tg[juce::jlimit (0, 2, (int) std::lround (x[b + 2]))], sh[juce::jlimit (0, 3, (int) std::lround (x[b + 3]))], x[b + 4] > .5f ? 1 : 0 }));
    }
    o->setProperty ("l", lfos);
    o->setProperty ("dr", r6 (x[fxDrive])); o->setProperty ("ch", r6 (x[fxChorus])); o->setProperty ("dl", r6 (x[fxDelay]));
    o->setProperty ("rv", r6 (x[fxReverb])); o->setProperty ("vol", r6 (x[level]));
    return juce::var (o.release());
}

void spRandom (char role, juce::Random& rng, float* x)
{
    for (int i = 0; i < kNumSp; ++i) x[i] = kDefs[i].def;
    auto u = [&] (float a, float b) { return a + (b - a) * rng.nextFloat(); };
    auto lg = [&] (float a, float b) { return a * std::pow (b / a, rng.nextFloat()); };
    auto chance = [&] (float p) { return rng.nextFloat() < p; };
    auto pickOf = [&] (std::initializer_list<int> l) { auto it = l.begin(); std::advance (it, rng.nextInt ((int) l.size())); return *it; };
    x[wtA] = (float) rng.nextInt (12); x[wtB] = (float) rng.nextInt (12); x[wtMorph] = u (.15f, .75f); x[wtBright] = u (.5f, 1);
    x[o2w] = x[o3w] = 0; x[l1depth] = x[l2depth] = 0;
    if (role == 'b')
    {
        x[o1w] = (float) pickOf ({ 1, 1, 2, 4, 5 }); x[o1lvl] = u (.55f, .85f);
        x[o2w] = 4; x[o2oct] = -1; x[o2lvl] = u (.7f, 1);
        if (chance (.35f)) { x[o3w] = 2; x[o3fine] = u (-9, 9); x[o3lvl] = u (.25f, .4f); }
        x[uni] = 1;
        x[cut] = lg (200, 750); x[reso] = u (1, 6); x[fenv] = u (1.5f, 4); x[fdec] = lg (.06f, .22f); x[fslope] = chance (.4f) ? 1.0f : 0.0f; x[fdrive] = u (.15f, .5f);
        x[envA] = lg (.002f, .006f); x[envD] = lg (.08f, .25f); x[envS] = u (.3f, .7f); x[envR] = lg (.04f, .12f);
        x[fxDrive] = u (.15f, .4f); x[level] = .2f;
        return;
    }
    if (role == 'c')
    {
        const bool pad = chance (.5f);
        x[o1w] = (float) pickOf ({ 1, 1, 5, 5, 3 }); x[o1lvl] = u (.45f, .6f);
        if (chance (.6f)) { x[o2w] = (float) pickOf ({ 1, 3, 5 }); x[o2oct] = (float) pickOf ({ -1, 0, 0 }); x[o2fine] = u (-12, 12); x[o2lvl] = u (.25f, .4f); }
        if (chance (.25f)) { x[o3w] = 4; x[o3oct] = -1; x[o3lvl] = u (.15f, .25f); }
        x[uni] = (float) (3 + rng.nextInt (5)); x[uniDet] = u (8, 25); x[sub] = u (0, .15f);
        x[ftype] = chance (.85f) ? 0.0f : 2.0f; x[fslope] = chance (.3f) ? 1.0f : 0.0f;
        x[cut] = lg (700, 4000); x[reso] = x[ftype] > 0 ? u (1, 3) : u (.5f, 4); x[fenv] = u (.5f, 3); x[fdec] = lg (.2f, 2);
        if (pad) { x[fatk] = u (.1f, .8f); x[envA] = lg (.3f, 1); x[envD] = u (.5f, 1.5f); x[envS] = u (.6f, .9f); x[envR] = u (.8f, 2); }
        else { x[envA] = lg (.002f, .01f); x[envD] = u (.2f, .6f); x[envS] = u (0, .3f); x[envR] = u (.1f, .4f); }
        if (chance (.5f)) { x[l1rate] = lg (.08f, .5f); x[l1depth] = u (.1f, .3f); x[l1target] = 0; }
        x[fxDrive] = u (0, .2f); x[fxChorus] = u (.2f, .6f); x[fxDelay] = u (0, .4f); x[fxReverb] = u (.3f, .6f); x[level] = .21f;
        return;
    }
    const bool pluck = chance (.5f);
    x[o1w] = (float) pickOf ({ 1, 2, 5, 5 }); x[o1lvl] = u (.5f, .6f);
    if (chance (.4f)) { x[o2w] = (float) pickOf ({ 1, 3 }); x[o2oct] = 1; x[o2fine] = u (-7, 7); x[o2lvl] = u (.2f, .35f); }
    x[uni] = (float) (1 + rng.nextInt (5)); x[uniDet] = u (6, 20);
    x[cut] = lg (1500, 5000); x[reso] = u (.5f, 3); x[fenv] = u (.5f, 3); x[fdec] = lg (.15f, .4f);
    if (pluck) { x[envA] = .002f; x[envD] = u (.2f, .4f); x[envS] = u (0, .15f); x[envR] = u (.2f, .3f); }
    else { x[envA] = lg (.005f, .03f); x[envD] = u (.15f, .3f); x[envS] = u (.7f, .9f); x[envR] = u (.2f, .5f); }
    if (chance (.35f)) { x[l1rate] = u (5, 6); x[l1depth] = u (.05f, .12f); x[l1target] = 1; }
    x[fxChorus] = u (0, .4f); x[fxDelay] = u (.3f, .55f); x[fxReverb] = u (.3f, .55f); x[level] = .2f;
}
}
