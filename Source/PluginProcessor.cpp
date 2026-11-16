#include "PluginProcessor.h"
#include "Arrangement.h"
#include "PluginEditor.h"

namespace
{
const char* const kGenParams[] = { "altC", "altB", "altD", "altM", "style", "variation", "seed", "key", "scale", "bars", "orig", "energy", "human", "swing", "thick", "sevenths" };
const char* const kPartIds[4] = { "chords", "bass", "drums", "melody" };
const int kMidiCh[4] = { 1, 2, 10, 3 };
juce::ParameterID pid (const juce::String& s) { return { s, 1 }; }
}

juce::StringArray HitGenProcessor::styleNames (const hg::Data& d)
{
    juce::StringArray a; for (int i = 0; i < d.numStyles(); ++i) a.add (d.style (i)["artist"].toString()); return a;
}

juce::AudioProcessorValueTreeState::ParameterLayout HitGenProcessor::createLayout (const hg::Data& d)
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout L;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + "%"; });
    auto sgn = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { const int x = roundToInt (v * 100); return (x > 0 ? "+" : "") + String (x) + "%"; });
    auto gain = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100)) + "%"; });
    L.add (std::make_unique<AudioParameterChoice> (pid ("style"), "Style", styleNames (d), 0));
    L.add (std::make_unique<AudioParameterChoice> (pid ("variation"), "Variation", StringArray { "1", "2", "3", "4" }, 0));
    L.add (std::make_unique<AudioParameterInt> (pid ("seed"), "Seed", 1, 99999, 7));
    for (auto* an : { "altC", "altB", "altD", "altM" }) L.add (std::make_unique<AudioParameterInt> (pid (an), String ("Alternative ") + an, 0, 999, 0));
    L.add (std::make_unique<AudioParameterChoice> (pid ("key"), "Key", d.noteNames, 9));
    StringArray sc { "auto" }; sc.addArray (d.scaleOrder);
    L.add (std::make_unique<AudioParameterChoice> (pid ("scale"), "Scale", sc, 0));
    L.add (std::make_unique<AudioParameterChoice> (pid ("bars"), "Length", StringArray { "4", "8", "16" }, 1));
    L.add (std::make_unique<AudioParameterFloat> (pid ("orig"), "Originality", NormalisableRange<float> (0, 100, 1), 30, pct));
    L.add (std::make_unique<AudioParameterFloat> (pid ("energy"), "Energy", NormalisableRange<float> (10, 100, 1), 60, pct));
    L.add (std::make_unique<AudioParameterFloat> (pid ("human"), "Human feel", NormalisableRange<float> (0, 100, 1), 20, pct));
    L.add (std::make_unique<AudioParameterFloat> (pid ("swing"), "Swing", NormalisableRange<float> (0, 60, 1), 0, pct));
    L.add (std::make_unique<AudioParameterFloat> (pid ("thick"), "Thickness", NormalisableRange<float> (0, 100, 1), 70, pct));
    L.add (std::make_unique<AudioParameterChoice> (pid ("sevenths"), "7th chords", StringArray { "Off", "On" }, 0));
    L.add (std::make_unique<AudioParameterFloat> (pid ("pump"), "Sidechain pump", NormalisableRange<float> (0, 100, 1), 100, pct));
    L.add (std::make_unique<AudioParameterChoice> (pid ("quality"), "Performance", StringArray { "Eco", "Balanced", "High" }, 1));
    L.add (std::make_unique<AudioParameterChoice> (pid ("pattern"), "Play pattern", StringArray { "Off", "On" }, 1));
    L.add (std::make_unique<AudioParameterChoice> (pid ("midiOut"), "MIDI out", StringArray { "Off", "On" }, 1));
    L.add (std::make_unique<AudioParameterChoice> (pid ("keysTo"), "Keyboard plays", StringArray { "Chords", "Bass", "Melody" }, 2));
    L.add (std::make_unique<AudioParameterInt> (pid ("bpm"), "Preview tempo", 60, 190, 126));
    L.add (std::make_unique<AudioParameterFloat> (pid ("master"), "Master", NormalisableRange<float> (0, 1.5f, .01f), 1, gain));
    for (int t = 0; t < 4; ++t)
    {
        String n (kPartIds[t]);
        L.add (std::make_unique<AudioParameterFloat> (pid ("vol_" + n), "Volume " + n, NormalisableRange<float> (0, 1.5f, .01f), 1, gain));
        L.add (std::make_unique<AudioParameterChoice> (pid ("mute_" + n), "Mute " + n, StringArray { "Off", "On" }, 0));
        if (t == 2) continue;
        L.add (std::make_unique<AudioParameterFloat> (pid ("cut_" + n), "Cutoff " + n, NormalisableRange<float> (-1, 1, .01f), 0, sgn));
        L.add (std::make_unique<AudioParameterFloat> (pid ("res_" + n), "Resonance " + n, NormalisableRange<float> (-1, 1, .01f), 0, sgn));
        L.add (std::make_unique<AudioParameterFloat> (pid ("atk_" + n), "Attack " + n, NormalisableRange<float> (-1, 1, .01f), 0, sgn));
        L.add (std::make_unique<AudioParameterFloat> (pid ("rel_" + n), "Release " + n, NormalisableRange<float> (-1, 1, .01f), 0, sgn));
        L.add (std::make_unique<AudioParameterFloat> (pid ("drv_" + n), "Drive " + n, NormalisableRange<float> (0, 1, .01f), 0, gain));
        L.add (std::make_unique<AudioParameterFloat> (pid ("rev_" + n), "Reverb " + n, NormalisableRange<float> (-1, 1, .01f), 0, sgn));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dly_" + n), "Delay " + n, NormalisableRange<float> (-1, 1, .01f), 0, sgn));
    }
    // ---- 2.7: ses yüksekliği ve davul makinesi
    L.add (std::make_unique<AudioParameterChoice> (pid ("loudness"), "Loudness -14 LUFS", StringArray { "Off", "On" }, 1));
    L.add (std::make_unique<AudioParameterChoice> (pid ("drumCreative"), "Drum creative mode", StringArray { "Off", "On" }, 0));
    auto st = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { const int x = roundToInt (v * 10); return (x > 0 ? "+" : "") + String (x / 10.0, 1) + " st"; });
    auto db = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; });
    for (int pad = 0; pad < hg::kNumPads; ++pad)
    {
        const String n (hg::kPadIds[pad]), N = n.substring (0, 1).toUpperCase() + n.substring (1);
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_level"), "Drum " + N + " level", NormalisableRange<float> (0, 150, 1), 100, pct));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_tune"), "Drum " + N + " tune", NormalisableRange<float> (-12, 12, .1f), 0, st));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_decay"), "Drum " + N + " decay", NormalisableRange<float> (25, 200, 1), 100, pct));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_punch"), "Drum " + N + " punch", NormalisableRange<float> (0, 100, 1), 50, pct));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_soft"), "Drum " + N + " soft", NormalisableRange<float> (0, 100, 1), 0, pct));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_mid"), "Drum " + N + " mid", NormalisableRange<float> (-12, 12, .1f), 0, db));
        L.add (std::make_unique<AudioParameterFloat> (pid ("dm_" + n + "_high"), "Drum " + N + " high", NormalisableRange<float> (-12, 12, .1f), 0, db));
    }
    hg::addSoundParams (L);
    // 3.1: Movement makrosu (LFO derinliği ve filtre zarfı). Sona eklenir; eski projelerde yoksa 0 (nötr) kalır.
    for (int t : { 0, 1, 3 })
        L.add (std::make_unique<AudioParameterFloat> (juce::ParameterID { "mov_" + String (kPartIds[t]), 3 }, "Movement " + String (kPartIds[t]), NormalisableRange<float> (-1, 1, .01f), 0,
                                                      AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { const int x = roundToInt (v * 100); return (x > 0 ? "+" : "") + String (x) + "%"; })));
    // Loop (LoopForge) makroları ve Öklid ayarları: DAW otomasyonu için
    {
        const ParameterID V { "lf_play", 4 };
        L.add (std::make_unique<AudioParameterBool> (V, "Loop play", false));
        L.add (std::make_unique<AudioParameterInt> (ParameterID { "lf_pulses", 4 }, "Loop pulses", 0, 32, 5));
        L.add (std::make_unique<AudioParameterInt> (ParameterID { "lf_rot", 4 }, "Loop rotation", 0, 31, 0));
        const std::pair<const char*, float> fl[] = { { "swing", 0 }, { "human", 0 }, { "space", 25 }, { "drive", 15 }, { "motion", 0 } };
        for (auto& [id, def] : fl)
            L.add (std::make_unique<AudioParameterFloat> (ParameterID { "lf_" + String (id), 4 }, "Loop " + String (id), NormalisableRange<float> (0, 100, 1), def, pct));
    }
    // Synth X master bölümü ve Alpha düğmesi (matris kaynağı)
    {
        const std::pair<const char*, float> mp[] = { { "mst_hp", 0 }, { "mst_lp", 100 }, { "mst_width", 50 }, { "mst_color", 50 } };
        for (auto& [id, def] : mp) L.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 6 }, juce::String ("Master ") + (juce::String (id).substring (4)), NormalisableRange<float> (0, 100, 1), def, pct));
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { "mst_out", 6 }, "Master output", NormalisableRange<float> (-24, 6, .1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { "xs_alpha", 6 }, "Alpha knob", NormalisableRange<float> (0, 100, .1f), 0.0f, pct));
    }
    // Koro: çalma ve makrolar (DAW otomasyonu için)
    L.add (std::make_unique<AudioParameterBool> (ParameterID { "ch_play", 5 }, "Choir play", false));
    {
        const std::pair<const char*, float> fl[] = { { "level", 70 }, { "space", 60 }, { "detune", 35 }, { "air", 40 }, { "vibrato", 30 }, { "attack", 40 } };
        for (auto& [id, def] : fl)
            L.add (std::make_unique<AudioParameterFloat> (ParameterID { "ch_" + String (id), 5 }, "Choir " + String (id), NormalisableRange<float> (0, 100, 1), def, pct));
        L.add (std::make_unique<AudioParameterFloat> (ParameterID { "ch_char", 5 }, "Choir character", NormalisableRange<float> (-100, 100, 1), 0.0f, pct));
    }
    return L;
}

HitGenProcessor::HitGenProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                         .withOutput ("Chords", juce::AudioChannelSet::stereo(), false)
                                         .withOutput ("Bass", juce::AudioChannelSet::stereo(), false)
                                         .withOutput ("Drums", juce::AudioChannelSet::stereo(), false)
                                         .withOutput ("Melody", juce::AudioChannelSet::stereo(), false)
                                         .withInput ("Capture In", juce::AudioChannelSet::stereo(), false)),   // Loop: DAW sesini yakalamak için (kapalı başlar)
      data (hg::Data::get()),
      apvts (*this, nullptr, "HitGen", createLayout (data))
{
    for (auto* id : kGenParams) apvts.addParameterListener (id, this);
    for (int part = 0; part < 3; ++part)
        for (int i = 0; i < hg::kNumSp; ++i)
        {
            const auto id = hg::spId (part, i);
            sdPtr[(size_t) part][(size_t) i] = apvts.getRawParameterValue (id);
            apvts.addParameterListener (id, this);
        }
    for (int t = 0; t < 4; ++t)
    {
        const juce::String n (kPartIds[t]); auto& q = pp[(size_t) t];
        q.vol = apvts.getRawParameterValue ("vol_" + n); q.mute = apvts.getRawParameterValue ("mute_" + n);
        if (t == 2) continue;
        q.cut = apvts.getRawParameterValue ("cut_" + n); q.res = apvts.getRawParameterValue ("res_" + n); q.atk = apvts.getRawParameterValue ("atk_" + n);
        q.rel = apvts.getRawParameterValue ("rel_" + n); q.drv = apvts.getRawParameterValue ("drv_" + n); q.rev = apvts.getRawParameterValue ("rev_" + n); q.dly = apvts.getRawParameterValue ("dly_" + n);
        q.mov = apvts.getRawParameterValue ("mov_" + n);
    }
    for (int pad = 0; pad < hg::kNumPads; ++pad)
        for (int k = 0; k < kDmCtl; ++k) dmPtr[(size_t) pad][(size_t) k] = apvts.getRawParameterValue (dmParamId (pad, k));
    pLoud = p ("loudness"); pCreative = p ("drumCreative");
    pQuality = p ("quality"); pThick = p ("thick"); pPump = p ("pump"); pMaster = p ("master"); pBpm = p ("bpm"); pPattern = p ("pattern"); pMidiOut = p ("midiOut"); pKeysTo = p ("keysTo");
    auto ul = juce::SystemStats::getUserLanguage().substring (0, 2).toLowerCase();
    lang = data.langCodes.contains (ul) ? ul : "en";
    const auto& S = data.style (0);
    for (int t = 0; t < 4; ++t) sounds[(size_t) t] = S["sound"][kPartIds[t]].toString();
    lastStyle = 0;
    engine.prepare (44100, 512);
    for (int part = 0; part < 3; ++part) applySoundToParams (part);
    for (int t = 0; t < 4; ++t) loadPatch (t);
    applyStyleDefaults();
    presets.rescan();
    skel.kinds = skStructKinds (0);
    regenerateNow();
    midiLearn.init (*this); midiLearn.loadDefault();      // proje durumu gelirse kendi eşlemesi geçerli olur
    startTimerHz (30);
}

HitGenProcessor::~HitGenProcessor()
{
    stopTimer();
    for (auto* id : kGenParams) apvts.removeParameterListener (id, this);
    for (int part = 0; part < 3; ++part) for (int i = 0; i < hg::kNumSp; ++i) apvts.removeParameterListener (hg::spId (part, i), this);
    delete pendingPattern.exchange (nullptr);
    delete livePattern;
    int s1, n1, s2, n2; patTrash.prepareToRead (patTrash.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) delete patTrashSlots[(size_t) (s1 + i)];
    for (int i = 0; i < n2; ++i) delete patTrashSlots[(size_t) (s2 + i)];
    engine.collectGarbage();
}

bool HitGenProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    // ek çıkışlar (akor, bas, davul, melodi): kapalı ya da stereo; ana çıkış monoysa hepsi kapalı olmalı
    for (int b = 1; b < layouts.outputBuses.size(); ++b)
    {
        const auto& set = layouts.outputBuses.getReference (b);
        if (set.isDisabled()) continue;
        if (set != juce::AudioChannelSet::stereo() || out != juce::AudioChannelSet::stereo()) return false;
    }
    for (auto& in : layouts.inputBuses)   // yakalama girişi: kapalı, mono ya da stereo
        if (! in.isDisabled() && in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono()) return false;
    return true;
}

void HitGenProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; blockSize = samplesPerBlock;
    kbBuf.ensureSize (4096);
    for (auto& c : tableCache) c.reset();
    engine.prepare (sampleRate, juce::jmax (512, samplesPerBlock));
    loop.prepare (sampleRate); choir.prepare (sampleRate);
    capIn.setSize (2, juce::jmax (4096, samplesPerBlock));
    for (int t = 0; t < 4; ++t) loadPatch (t);   // wavetable seviyeleri örnekleme hızına bağlı
    numOffs = 0; wasRunning = false;
}

namespace { int partOfTrack (int t) { return t == hg::kMelody ? 2 : t; } int trackOfPart (int p) { return p == 2 ? hg::kMelody : p; } }

void HitGenProcessor::loadPatch (int track, bool live)
{
    if (track == hg::kDrums)
    {
        auto ki = data.kitIndex (sounds[2]); engine.setKit (data.kit (ki >= 0 ? ki : 0));
        return;
    }
    const auto& base = data.soundById (sounds[(size_t) track]);
    if (base.isVoid()) return;
    const int part = partOfTrack (track);
    if (! xsEdit[(size_t) part].isObject() || xsAuto[(size_t) part])   // eski motor kullanılmaz: program yoksa (ya da otomatik çevrildiyse) sesten yeniden çevir
    { xsEdit[(size_t) part] = juce::var(); xsEdit[(size_t) part] = xsFromClassic (part); xsAuto[(size_t) part] = true; ++xsVersion; }
    auto np = hg::Patch::fromVar (currentSoundVar (part), data, sr, tableCache[(size_t) part].get());
    if (np->xs != nullptr)
    {
        auto& pf = perf[(size_t) part]; const auto& X = *np->xs;
        pf.chord.store (X.chord.mode); pf.strum.store (X.chord.strum); pf.arpOn.store (X.arp.on ? 1 : 0); pf.arpMode.store (X.arp.mode); pf.arpDiv.store (X.arp.div); pf.arpOct.store (X.arp.oct); pf.gate.store (X.arp.gate);
    }
    if (np->usesTable()) tableCache[(size_t) part] = std::make_unique<hg::Patch> (*np);
    // eski yamaları hemen sil: zamanlayıcı işlemeden peş peşe çok yama gelirse (ör. mesaj döngüsü olmayan test ya da sunucu)
    // 64'lük çöp kuyruğu dolup yama sızdırmasın. Yalnızca mesaj iş parçacığında: setStateInformation başka iş parçacığından gelebilir.
    if (juce::MessageManager::existsAndIsCurrentThread()) engine.collectGarbage();
    engine.setPatch (track == hg::kMelody ? 2 : track, std::move (np), live);
}

void HitGenProcessor::readSd (int part, float* out) const
{
    for (int i = 0; i < hg::kNumSp; ++i) out[i] = sdPtr[(size_t) part][(size_t) i]->load();
}

void HitGenProcessor::writeSd (int part, const float* vals)
{
    const juce::ScopedValueSetter<bool> sv (suppressSd, true);
    for (int i = 0; i < hg::kNumSp; ++i)
        if (auto* pr = apvts.getParameter (hg::spId (part, i)))
        {
            const float nv = pr->convertTo0to1 (vals[i]);
            if (std::abs (pr->getValue() - nv) > 1e-7f) pr->setValueNotifyingHost (nv);
        }
    sdDirty[(size_t) part].store (false);
}

void HitGenProcessor::applySoundToParams (int part)
{
    float vals[hg::kNumSp];
    hg::spFromSound (data.soundById (sounds[(size_t) trackOfPart (part)]), vals);
    writeSd (part, vals);
}

juce::var HitGenProcessor::currentSoundVar (int part) const
{
    float vals[hg::kNumSp]; readSd (part, vals);
    auto s = hg::spToSound (data.soundById (sounds[(size_t) trackOfPart (part)]), vals);
    if (auto* o = s.getDynamicObject()) { if (xsEdit[(size_t) part].isObject()) o->setProperty ("xs", xsEdit[(size_t) part]); else o->removeProperty ("xs"); }
    return s;
}

void HitGenProcessor::setXsProgram (int part, const juce::var& prog)
{
    part = juce::jlimit (0, 2, part); xsEdit[(size_t) part] = prog.isObject() ? prog : juce::var(); xsAuto[(size_t) part] = ! prog.isObject(); ++xsVersion;
    loadPatch (trackOfPart (part), true);
}

juce::var HitGenProcessor::xsFromClassic (int part) const
{
    // klasik sesin yaklaşık karşılığı: osilatörler Mod osilatörüne, filtre her osilatöre, ADSR 1. zarfa, filtre zarfı 2. zarfa, LFO'lar LFO'lara
    const auto s = currentSoundVar (juce::jlimit (0, 2, part)); auto P = xs::Program::toVar (xs::Program::init());
    auto set = [] (juce::var o, const char* k, juce::var v) { if (auto* d = o.getDynamicObject()) d->setProperty (k, v); };
    const auto* oa = s["o"].getArray(); const auto* f = s["f"].getArray();
    const juce::String ft = f != nullptr ? (*f)[0].toString() : "lowpass";
    const int ftype = ft == "highpass" ? 4 : ft == "bandpass" ? 5 : (f != nullptr && f->size() > 6 && (double) (*f)[6] > 0) ? 2 : 1;
    for (int k = 0; k < 3; ++k)
    {
        auto O = P["o"][k];
        if (oa == nullptr || k >= oa->size()) { set (O, "t", 0); continue; }
        const auto L = (*oa)[k]; const auto w = L[0].toString();
        set (O, "t", 1); set (O, "a", w == "q" ? 1 : w == "t" ? 2 : w == "s" ? 3 : 0);
        if (w == "x") { set (O, "t", 2); const auto* wt = s["wt"].getArray(); set (O, "a", 12 * 8 + juce::jlimit (0, 7, (int) std::lround ((wt != nullptr && wt->size() > 3 ? (double) (*wt)[3] : 1.0) * 7))); }
        const double det = (double) L[1], oct = (double) L[2], gain = (double) L[3];
        set (O, "oc", juce::jlimit (-3, 3, (int) std::lround (oct))); set (O, "fi", juce::jlimit (-1.0, 1.0, det / 100.0)); set (O, "lv", juce::jlimit (0.0, 1.0, gain > 0 ? gain : .8));
        if (const auto* u = s["u"].getArray()) { set (O, "u", juce::jlimit (1, 12, (int) (*u)[0])); set (O, "d", juce::jlimit (0.0, 1.0, (double) (*u)[1] / 40.0)); }
        auto F = O["f"]; set (F, "t", ftype);
        if (f != nullptr) { set (F, "c", (double) (*f)[1]); set (F, "r", juce::jlimit (0.0, 1.0, (double) (*f)[2] / 24.0)); set (F, "e", juce::jlimit (-1.0, 1.0, (double) (*f)[3] / 5.0)); }
        set (F, "k", s["kt"].isVoid() ? .5 : (double) s["kt"]);
    }
    juce::Array<juce::var> env;
    if (const auto* a = s["a"].getArray()) env.add (juce::Array<juce::var> { (*a)[0], (*a)[1], (*a)[2], (*a)[3], 0 }); else env.add (P["env"][0]);
    env.add (juce::Array<juce::var> { f != nullptr && f->size() > 4 ? (*f)[4] : juce::var (.005), f != nullptr && f->size() > 5 ? (*f)[5] : juce::var (.3), 0, .3, 0 });
    env.add (P["env"][2]); env.add (P["env"][3]);
    set (P, "env", env);
    if (const auto* l = s["l"].getArray())
        for (int i = 0; i < 2 && i < l->size(); ++i)
        {
            const auto L = (*l)[i]; auto Lf = P["lfo"][i]; const auto tg = L[2].toString(), sh = L[3].toString();
            set (Lf, "w", sh == "triangle" ? 1 : sh == "square" ? 4 : sh == "sawtooth" ? 2 : 0); set (Lf, "sy", 0); set (Lf, "r", (double) L[0]);
            const double depth = juce::jlimit (0.0, 1.0, (double) L[1]);
            juce::Array<juce::var> d;
            if (tg == "pit") d.add (juce::Array<juce::var> { (int) xs::dPitch, depth * .05 });
            else if (tg == "amp") d.add (juce::Array<juce::var> { (int) xs::dAmp, depth * .5 });
            else for (int k = 0; k < 3; ++k) d.add (juce::Array<juce::var> { k * 16 + 11, depth * .3 });
            set (Lf, "d", d);
        }
    set (P, "gl", s["gl"].isVoid() ? 0.0 : (double) s["gl"]);
    // klasik sesin diğer özellikleri: alt osilatör, gürültü, FM, perde zarfı, vibrato, katlama, ünlü, seviye
    const int used = oa != nullptr ? juce::jmin (3, oa->size()) : 0;
    auto num = [&s] (const char* k) { return s[k].isVoid() ? 0.0 : (double) s[k]; };
    if (num ("sub") > 0 && used < 3) { auto O = P["o"][used]; set (O, "t", 1); set (O, "a", 3); set (O, "oc", -1); set (O, "lv", juce::jlimit (0.0, 1.0, num ("sub"))); set (O["f"], "t", 0); }
    if (num ("noi") > 0) { auto L = P["lx"]; set (L, "on", 1); set (L, "w", -1); set (L, "t", 1); set (L, "c", 2000); set (L, "lv", juce::jlimit (0.0, 1.0, num ("noi") * 2)); }
    if (const auto* fm = s["fm"].getArray(); fm != nullptr && fm->size() >= 2)
    {
        auto O = P["o"][0]; set (O, "sm", 1); set (O, "fr", juce::jlimit (.25, 16.0, (double) (*fm)[0])); set (O, "fm", juce::jlimit (0.0, 1.0, (double) (*fm)[1] * .08 * (fm->size() > 3 ? (double) (*fm)[3] : .25)));
        auto E = P["env"]; E.getArray()->set (2, juce::Array<juce::var> { .001, fm->size() > 2 ? (double) (*fm)[2] : .2, 0, .2, 0 });
        juce::Array<juce::var> r; r.add (juce::Array<juce::var> { 9, juce::jlimit (0.0, 1.0, (double) (*fm)[1] * .08) }); for (int i = 1; i < 5; ++i) r.add (juce::Array<juce::var> { -1, 0 });
        P["ed"].getArray()->set (1, r);
    }
    if (const auto* pe = s["pe"].getArray(); pe != nullptr && pe->size() >= 2 && (double) (*pe)[0] != 0)
    {
        P["env"].getArray()->set (3, juce::Array<juce::var> { .001, juce::jmax (.01, (double) (*pe)[1]), 0, .1, 0 });
        juce::Array<juce::var> r; r.add (juce::Array<juce::var> { (int) xs::dPitch, juce::jlimit (-1.0, 1.0, (double) (*pe)[0] / 24.0) }); for (int i = 1; i < 5; ++i) r.add (juce::Array<juce::var> { -1, 0 });
        P["ed"].getArray()->set (2, r);
    }
    if (const auto* vb = s["vib"].getArray(); vb != nullptr && vb->size() >= 2 && (double) (*vb)[1] > 0)
    {
        auto L = P["lfo"][1]; set (L, "w", 0); set (L, "sy", 0); set (L, "r", (double) (*vb)[0]); set (L, "fa", vb->size() > 2 ? (double) (*vb)[2] + .3 : .3);
        juce::Array<juce::var> r; r.add (juce::Array<juce::var> { (int) xs::dPitch, juce::jlimit (0.0, 1.0, (double) (*vb)[1] / 100.0 / 24.0) }); for (int i = 1; i < 5; ++i) r.add (juce::Array<juce::var> { -1, 0 });
        set (L, "d", r);
    }
    if (num ("fold") > 0) for (int k = 0; k < used; ++k) { auto O = P["o"][k]; if ((int) O["sm"] == 0) set (O, "sh", juce::jlimit (0.0, 1.0, num ("fold"))); }
    if (s["vow"].toString().isNotEmpty() && used > 0)
    {   // ünlü sesler: ROM "Vowel" tabloları (a e i o u)
        const auto vw = s["vow"].toString(); const int vi = vw == "e" ? 1 : vw == "i" ? 2 : vw == "o" ? 3 : vw == "u" ? 4 : 0;
        auto O = P["o"][0]; set (O, "t", 2); set (O, "a", 15 * 8 + vi);
    }
    const auto id = sounds[(size_t) trackOfPart (juce::jlimit (0, 2, part))];
    set (P, "vol", juce::jlimit (.05, 1.0, (s["vol"].isVoid() ? .2 : (double) s["vol"]) * data.trim (id) / .24));
    return xs::Program::toVar (P);
}

void HitGenProcessor::resetSoundDesign (int part)
{
    applySoundToParams (part);
    loadPatch (trackOfPart (part), true);
}

void HitGenProcessor::randomizeSound (int part)
{
    float vals[hg::kNumSp];
    hg::spRandom (part == 0 ? 'c' : part == 1 ? 'b' : 'm', juce::Random::getSystemRandom(), vals);
    writeSd (part, vals);
    loadPatch (trackOfPart (part), true);
}

void HitGenProcessor::applySoundParams (int part, const float* vals)
{
    writeSd (part, vals); loadPatch (trackOfPart (part), true);
}

void HitGenProcessor::readSoundParams (int part, float* vals) const
{
    for (int i = 0; i < hg::kNumSp; ++i) vals[i] = apvts.getRawParameterValue (hg::spId (part, i))->load();
}

void HitGenProcessor::playDemo (int part)
{
    const int t = trackOfPart (juce::jlimit (0, 2, part));
    const auto& v = data.soundById (sounds[(size_t) t]);
    const int kind = hg::demoKindFor (v["cat"].toString(), v["demo"].toString());
    demoOn.store (true); demoReq.store ((t << 8) | kind);
}

void HitGenProcessor::applyRefToProject (const hg::RefAnalysis& a)
{
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); pr->endChangeGesture(); } };
    if (a.bpm >= 60 && a.bpm <= 190) setP ("bpm", (float) juce::roundToInt (a.bpm));
    setP ("key", (float) a.key);
    const int idx = data.scaleOrder.indexOf (a.minor ? "minor" : "major");
    if (idx >= 0) setP ("scale", (float) (idx + 1));
}

void HitGenProcessor::setSound (int track, const juce::String& id, bool isManual)
{
    sounds[(size_t) track] = id; if (isManual) manual[(size_t) track] = true;
    if (track != hg::kDrums) { const auto& xs = data.soundById (id)["xs"]; xsEdit[(size_t) partOfTrack (track)] = xs.isObject() ? xs::Program::toVar (xs) : juce::var(); xsAuto[(size_t) partOfTrack (track)] = ! xs.isObject(); ++xsVersion; applySoundToParams (partOfTrack (track)); }
    loadPatch (track);
}

void HitGenProcessor::applyStyleDefaults()
{
    const int si = (int) p ("style")->load();
    const auto& S = data.style (juce::jlimit (0, data.numStyles() - 1, si));
    for (int t = 0; t < 4; ++t) if (! manual[(size_t) t]) setSound (t, S["sound"][kPartIds[t]].toString(), false);
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); };
    setP ("key", (float) (int) S["key"]); setP ("scale", 0); setP ("swing", (float) (double) S["swing"]); setP ("bpm", (float) (int) S["bpmDef"]); setP ("variation", 0);
    setP ("sevenths", (bool) S["sev"] ? 1.0f : 0.0f);   // stil varsayılanı (ör. Black Coffee 7'li akorlarla gelir)
    for (int t : { 0, 1, 3 })
    {
        auto pm = S["pump"][kPartIds[t]];
        engine.setPump (t, (float) (double) pm[0], (float) (double) pm[1]);
    }
}

void HitGenProcessor::parameterChanged (const juce::String& id, float)
{
    const int part = hg::spPartOfId (id);
    if (part >= 0) { if (! suppressSd) sdDirty[(size_t) part].store (true); return; }   // ses iş parçacığından da gelebilir: yalnızca bayrak
    if (id == "style") styleChanged.store (true);
    regenPending.store (true);
}

hg::GenParams HitGenProcessor::currentParams() const
{
    hg::GenParams g;
    const int si = juce::jlimit (0, data.numStyles() - 1, (int) p ("style")->load());
    g.style = data.styleId (si);
    g.vi = (int) p ("variation")->load(); g.seed = (int) p ("seed")->load(); g.key = (int) p ("key")->load();
    const int sc = (int) p ("scale")->load(); g.scale = sc <= 0 ? juce::String() : data.scaleOrder[sc - 1];
    const int bi = (int) p ("bars")->load(); g.bars = bi == 0 ? 4 : bi == 2 ? 16 : 8;
    g.orig = p ("orig")->load(); g.energy = p ("energy")->load(); g.human = p ("human")->load(); g.swing = p ("swing")->load();
    g.thick = p ("thick")->load(); g.sevenths = p ("sevenths")->load() > .5f;
    g.alt[0] = (int) p ("altC")->load(); g.alt[1] = (int) p ("altB")->load(); g.alt[2] = (int) p ("altD")->load(); g.alt[3] = (int) p ("altM")->load();
    const double hb = hostBpm.load(); g.bpm = hb > 0 ? (int) std::lround (hb) : (int) p ("bpm")->load();
    return g;
}

void HitGenProcessor::regenerateNow()
{
    if (editActive && current != nullptr)
    {
        if (paramsKey() == editKey)   // yalnızca tempo değişti: elle düzenlenen notaları koru
        {
            auto e = std::make_shared<hg::Result> (*current); e->P.bpm = currentParams().bpm;
            current = e; if (onResultChanged) onResultChanged(); return;
        }
        editActive = false; editKey.clear();   // müzikal bir ayar değişti: yeni fikir (geri al ile düzenlemeye dönülebilir)
    }
    auto r = std::make_shared<hg::Result> (hg::generate (data, currentParams()));
    if (r->tracks[hg::kDrums].empty()) return;   // savunma: boş sonuç gelirse eskisini koru
    if (coRole >= 0 && coRole < hg::kNumTracks && coRole != hg::kDrums)   // Co-Producer: kullanıcının notaları kilitli
        r->tracks[(size_t) coRole] = hg::fitLocked (coNotes, coBars, r->bars);
    if (savedEdit != nullptr)   // projeyle kaydedilmiş elle düzenleme: aynı ayarlarsa geri yükle
    {
        if (savedEditKey == paramsKey())
        {
            r->tracks = *savedEdit; savedEdit.reset();
            applyEditedResult (r); return;
        }
        savedEdit.reset();
    }
    pushPattern (*r);
    current = r;
    if (onResultChanged) onResultChanged();
}

juce::String HitGenProcessor::importSynthFile (const juce::File& f, int part, hg::ImportedSound* info)
{
    auto r = hg::importSoundFile (f);
    if (info != nullptr) *info = r;
    if (! r.ok) return "impFail";
    const int pt = juce::jlimit (0, 2, part >= 0 ? part : r.part);
    if (r.exact)
    {
        float vals[hg::kNumSp]; for (int i = 0; i < hg::kNumSp; ++i) vals[i] = r.x[(size_t) i];
        writeSd (pt, vals);
        sdDirty[(size_t) pt].store (true);       // yama yeni değerlerle kurulsun
        return "impOk";
    }
    static const int trackOf[3] = { hg::kChords, hg::kBass, hg::kMelody };
    if (data.soundIndex (r.fallbackId) >= 0) { setSound (trackOf[pt], r.fallbackId, true); return "impApprox"; }
    return "impFail";
}

void HitGenProcessor::newIdea()
{
    // Fikir üret: tohum + varyasyon + (stile uygun) ton, makam, enerji, özgünlük, swing ve tempo birlikte değişir.
    auto& rnd = juce::Random::getSystemRandom();
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); };
    const int si = juce::jlimit (0, data.numStyles() - 1, (int) p ("style")->load());
    const auto& S = data.style (si);
    for (auto* an : { "altC", "altB", "altD", "altM" }) setP (an, 0);
    setP ("variation", (float) rnd.nextInt (4));
    // ton: %45 stilin kendi tonu; aksi halde Camelot'ta komşu (±5/±7 yarım ses ya da paralel ton)
    const int home = (int) S["key"], cur = (int) p ("key")->load();
    static const int hop[] = { 0, 7, 5, 3, -3, 2, -2 };
    const int base = rnd.nextFloat() < .45f ? home : cur;
    setP ("key", (float) (((base + hop[rnd.nextInt (7)]) % 12 + 12) % 12));
    if (auto* sc = S["scales"].getArray()) if (sc->size() > 0)
    {
        const int idx = data.scaleOrder.indexOf ((*sc)[rnd.nextInt (sc->size())].toString());
        setP ("scale", idx >= 0 ? (float) (idx + 1) : 0.0f);
    }
    setP ("energy", 48.0f + (float) rnd.nextInt (42));
    setP ("orig", 18.0f + (float) rnd.nextInt (46));
    setP ("human", 10.0f + (float) rnd.nextInt (26));
    setP ("swing", juce::jlimit (0.0f, 60.0f, (float) (double) S["swing"] + (float) rnd.nextInt (9) - 3.0f));
    if (auto* bp = S["bpm"].getArray()) if (bp->size() == 2)
        setP ("bpm", (float) ((int) (*bp)[0] + rnd.nextInt (juce::jmax (1, (int) (*bp)[1] - (int) (*bp)[0] + 1))));
    // sekiz aday tohum üret, kurallara göre en yüksek puanlıyı seç (diğer ayarlar yukarıda belirlendi)
    auto gp = currentParams(); int best = 1; double bestScore = -1e9;
    for (int i = 0; i < 8; ++i)
    {
        gp.seed = rnd.nextInt ({ 1, 99999 });
        const double sc = hg::ideaScore (hg::generate (data, gp));
        if (sc > bestScore) { bestScore = sc; best = gp.seed; }
    }
    setP ("seed", (float) best);
    // sesler de şaşırtsın: elle seçilmemiş izlere kütüphaneden stile uygun yeni sesler
    for (int t : { hg::kChords, hg::kBass, hg::kMelody })
        if (! manual[(size_t) t]) { const auto id = librarySoundFor (t, rnd); if (id.isNotEmpty()) setSound (t, id, false); }
}

juce::String HitGenProcessor::librarySoundFor (int track, juce::Random& rnd) const
{
    if (track == hg::kDrums) return {};
    const int si = juce::jlimit (0, data.numStyles() - 1, (int) p ("style")->load());
    const auto& S = data.style (si); const int vi = juce::jlimit (0, 3, (int) p ("variation")->load());
    const auto& V = S["var"][vi];
    const auto grp = S["grp"].toString();
    // stil grubu -> ses etiketleri
    juce::StringArray tags;
    if (grp == "techno") tags = { "techno", "mtechno" }; else if (grp == "house") tags = { "house", "deep" }; else if (grp == "trance") tags = { "trance", "mtechno" };
    else if (grp == "afro") tags = { "afro", "deep", "mtechno" }; else if (grp == "hard") tags = { "techno", "trance" }; else tags = { "mtechno", "house", "trance" };
    // iz rolü ve çalış biçimi -> kategoriler
    juce::StringArray cats; char role = 'c';
    if (track == hg::kChords)
    {
        const auto cm = V.hasProperty ("cm") ? V["cm"].toString() : S["chord"]["mode"].toString();
        cats = cm == "stab" ? juce::StringArray { "Stab", "Pluck" } : cm == "arp" ? juce::StringArray { "Pluck", "Seq", "Stab" } : juce::StringArray { "Pad", "Stab", "Texture" };
    }
    else if (track == hg::kBass) { role = 'b'; cats = grp == "techno" || grp == "hard" ? juce::StringArray { "Rolling", "Acid", "Sub" } : juce::StringArray { "Rolling", "Sub" }; }
    else
    {
        role = 'm';
        const auto mm = V.hasProperty ("mm") ? V["mm"].toString() : S["mel"]["mode"].toString();
        cats = mm == "arp" ? juce::StringArray { "Seq", "Pluck" } : mm == "riff" ? juce::StringArray { "Seq", "Acid", "Metal", "Lead" } : mm == "chop" ? juce::StringArray { "Pluck", "Seq" } : juce::StringArray { "Lead", "Pluck", "Seq" };
    }
    const auto cur = sounds[(size_t) track];
    if (auto* pal = S["pal"][juce::Identifier (juce::String::charToString ((juce::juce_wchar) role))].getArray())   // türün kendi paleti (rap, R&B, bass, chill)
    {
        juce::StringArray pool; for (auto& x : *pal) if (x.toString() != cur) pool.add (x.toString());
        if (! pool.isEmpty()) return pool[rnd.nextInt (pool.size())];
    }
    for (int pass = 0; pass < 2; ++pass)   // önce etiket eşleşmesi, yoksa yalnızca rol ve kategori
    {
        juce::StringArray pool;
        for (int i = 0; i < data.numSounds(); ++i)
        {
            const auto& v = data.sound (i); const auto id = v["id"].toString();
            if (! data.isFactory (id) || id == cur || ! v["r"].toString().containsChar ((juce::juce_wchar) role) || ! cats.contains (v["cat"].toString())) continue;
            bool tag = pass == 1; if (! tag) if (auto* g = v["g"].getArray()) for (auto& x : *g) if (tags.contains (x.toString())) tag = true;
            if (tag) pool.add (id);
        }
        if (! pool.isEmpty()) return pool[rnd.nextInt (pool.size())];
    }
    return {};
}

void HitGenProcessor::surpriseSound (int track)
{
    const auto id = librarySoundFor (track, juce::Random::getSystemRandom());
    if (id.isNotEmpty()) setSound (track, id, true);
}

void HitGenProcessor::suggestAlternative (int track)
{
    static const char* ids[] = { "altC", "altB", "altD", "altM" };
    if (track < 0 || track > 3) return;
    if (auto* pr = apvts.getParameter (ids[track]))
    {
        const int cur = (int) p (ids[track])->load();
        int nxt; do nxt = juce::Random::getSystemRandom().nextInt ({ 1, 999 }); while (nxt == cur);
        pr->setValueNotifyingHost (pr->convertTo0to1 ((float) nxt));
    }
}

void HitGenProcessor::pushPattern (const hg::Result& r)
{
    {   // koro aynı akorları izler (ölçü başına bir akor)
        std::vector<ch::Bar> cb; for (int b = 0; b < r.bars && b < (int) r.chords.size(); ++b) cb.push_back ({ r.chords[(size_t) b].pc, r.chords[(size_t) b].third, r.chords[(size_t) b].fifth });
        if (! cb.empty()) choir.setChords (cb);
    }
    auto* pat = new Pattern();
    pat->lenPpq = r.bars * 4.0;
    for (int t = 0; t < hg::kNumTracks; ++t)
        for (auto& n : r.tracks[(size_t) t])
        {
            const int on = hg::tickOf (n.s, r.P.swing), off = hg::tickOf (n.s + n.l, r.P.swing);
            pat->ev.push_back ({ on / 480.0, juce::jmax (1, off - on) / 480.0, t, n.n, n.v });
        }
    std::stable_sort (pat->ev.begin(), pat->ev.end(), [] (const SeqEvent& a, const SeqEvent& b) { return a.ppq < b.ppq; });
    delete pendingPattern.exchange (pat);
}

void HitGenProcessor::syncLoopParams()
{
    // Loop ayarlarının otomasyonu: parametreler değişirse döngü yeniden işlenir; Öklid ayarı tetikleri yeniden dağıtır
    auto cs = loop.get(); bool ch = false;
    const int steps = cs.steps, pulses = juce::jmin (steps, (int) p ("lf_pulses")->load()), rot = (int) p ("lf_rot")->load() % steps;
    if (pulses != cs.pulses || rot != cs.rotation)
    {
        cs.pulses = pulses; cs.rotation = rot; ch = true;
        const auto e = lf::euclid (steps, pulses, rot); for (int i = 0; i < steps; ++i) cs.step[(size_t) i].on = e[(size_t) i];
    }
    auto f = [&] (const char* id, float& dst) { const float v = p (id)->load() / 100.0f; if (std::abs (v - dst) > 1e-4f) { dst = v; ch = true; } };
    f ("lf_swing", cs.swing); f ("lf_human", cs.human); f ("lf_space", cs.space); f ("lf_drive", cs.drive); f ("lf_motion", cs.motion);
    if (ch) loop.set (cs);
    loop.setPlaying (p ("lf_play")->load() > .5f);
}

void HitGenProcessor::syncChoirParams()
{
    auto cs = choir.get(); bool ch = false;
    auto f = [&] (const char* id, float& dst) { const float v = p (id)->load() / 100.0f; if (std::abs (v - dst) > 1e-4f) { dst = v; ch = true; } };
    f ("ch_level", cs.level); f ("ch_space", cs.space); f ("ch_detune", cs.detune); f ("ch_air", cs.air); f ("ch_vibrato", cs.vibrato); f ("ch_attack", cs.attack); f ("ch_char", cs.character);
    if (ch) choir.set (cs);
    choir.setPlaying (p ("ch_play")->load() > .5f);
}

void HitGenProcessor::pushChoirParams()
{
    const auto cs = choir.get();
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); };
    setP ("ch_level", cs.level * 100); setP ("ch_space", cs.space * 100); setP ("ch_detune", cs.detune * 100); setP ("ch_air", cs.air * 100);
    setP ("ch_vibrato", cs.vibrato * 100); setP ("ch_attack", cs.attack * 100); setP ("ch_char", cs.character * 100);
}

void HitGenProcessor::pushLoopParams()
{
    const auto cs = loop.get();
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); };
    setP ("lf_pulses", (float) cs.pulses); setP ("lf_rot", (float) cs.rotation);
    setP ("lf_swing", cs.swing * 100); setP ("lf_human", cs.human * 100); setP ("lf_space", cs.space * 100); setP ("lf_drive", cs.drive * 100); setP ("lf_motion", cs.motion * 100);
    setP ("lf_play", loop.isPlaying() ? 1.0f : 0.0f);
}

void HitGenProcessor::timerCallback()
{
    if (auto r = result())   // akor modu için gam (12 bitlik perde sınıfı maskesi)
    {
        int mask = 0; if (const auto* iv = data.scale (r->P.scale)["iv"].getArray()) for (auto& x : *iv) mask |= 1 << (((int) x % 12 + 12) % 12);
        if (mask != 0) { scaleMask.store (mask); scaleKey.store (r->P.key); }
    }
    { const double hb = hostBpm.load(); loop.setTempo (hb > 0 ? hb : (double) p ("bpm")->load()); syncLoopParams(); loop.collect(); choir.setTempo (hb > 0 ? hb : (double) p ("bpm")->load()); syncChoirParams(); choir.collect(); }
    // ses iş parçacığının bıraktığı eski desenleri ve yamaları sil
    int s1, n1, s2, n2; patTrash.prepareToRead (patTrash.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) delete patTrashSlots[(size_t) (s1 + i)];
    for (int i = 0; i < n2; ++i) delete patTrashSlots[(size_t) (s2 + i)];
    patTrash.finishedRead (n1 + n2);
    engine.collectGarbage();
    if (styleChanged.exchange (false))
    {
        const int si = (int) p ("style")->load();
        if (si != lastStyle) { lastStyle = si; manual = {}; userArtist.clear(); userPreset.clear(); applyStyleDefaults(); }
    }
    for (int part = 0; part < 3; ++part)
        if (sdDirty[(size_t) part].exchange (false)) loadPatch (trackOfPart (part), true);
    if (regenPending.exchange (false)) regenerateNow();
    midiLearn.timerTick();
}

void HitGenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    bool fromIn = false;   // yakalama girişi bağlıysa (ve ses geliyorsa) halka DAW sesini kaydeder
    if (auto* ib = getBus (true, 0); ib != nullptr && ib->isEnabled() && ib->getNumberOfChannels() > 0 && n <= capIn.getNumSamples())
    {
        const int nc = ib->getNumberOfChannels();
        for (int c = 0; c < 2; ++c)
        {
            const int src = getChannelIndexInProcessBlockBuffer (true, 0, juce::jmin (c, nc - 1));
            if (src < buffer.getNumChannels()) capIn.copyFrom (c, 0, buffer, src, 0, n); else capIn.clear (c, 0, n);
        }
        fromIn = true;
    }
    captureFromInput.store (fromIn);
    {   // Dynamic Link: giriş sesinin zarfı Synth X matrisine kaynak olur (giriş bağlı değilse 0)
        float pk = 0; if (fromIn) pk = juce::jmax (capIn.getMagnitude (0, 0, n), capIn.getMagnitude (1, 0, n));
        audioEnv = pk > audioEnv ? pk : audioEnv * std::pow (.5f, (float) n / (float) (getSampleRate() * .08));
        engine.audioIn.store (juce::jlimit (0.0f, 1.0f, audioEnv * 2), std::memory_order_relaxed);
    }
    if (fromIn) choir.feed (capIn.getReadPointer (0), capIn.getReadPointer (1), n, getSampleRate());   // koro kaydı: DAW girişi bağlıysa oradan
    buffer.clear();
    {   // çok çıkış: etkin ek bus'lara ilgili izi yönlendir (iz kimliği: 0 akor, 1 bas, 2 davul, 3 melodi)
        static const int busTrack[5] = { -1, hg::kChords, hg::kBass, hg::kDrums, hg::kMelody };
        for (int t = 0; t < 4; ++t) { engine.auxL[t] = nullptr; engine.auxR[t] = nullptr; }
        for (int b = 1; b < juce::jmin (5, getBusCount (false)); ++b)
            if (auto* bus = getBus (false, b); bus != nullptr && bus->isEnabled() && bus->getNumberOfChannels() == 2)
            {
                const int c0 = getChannelIndexInProcessBlockBuffer (false, b, 0), c1 = getChannelIndexInProcessBlockBuffer (false, b, 1);
                if (c1 < buffer.getNumChannels()) { engine.auxL[busTrack[b]] = buffer.getWritePointer (c0); engine.auxR[busTrack[b]] = buffer.getWritePointer (c1); }
            }
    }
    if (auto* np = pendingPattern.exchange (nullptr))
    {
        if (livePattern != nullptr)
        {
            int s1, n1, s2, n2; patTrash.prepareToWrite (1, s1, n1, s2, n2);
            if (n1 > 0) patTrashSlots[(size_t) s1] = livePattern; else delete livePattern;
            patTrash.finishedWrite (n1);
        }
        livePattern = np;
    }
    // parametreler
    for (int t = 0; t < 4; ++t)
    {
        const auto& q = pp[(size_t) t]; auto& m = engine.macros[(size_t) t];
        m.volume = q.vol->load(); m.mute = q.mute->load() > .5f;
        if (t == 2) continue;
        m.cutoff = q.cut->load(); m.reso = q.res->load(); m.attack = q.atk->load(); m.release = q.rel->load();
        m.drive = q.drv->load(); m.reverb = q.rev->load(); m.delay = q.dly->load(); m.movement = q.mov != nullptr ? q.mov->load() : 0.0f;
    }
    const int q = (int) pQuality->load();
    engine.quality = q == 0 ? hg::Quality { 2, 32, false } : q == 2 ? hg::Quality { 7, 8, true } : hg::Quality { 4, 16, true };
    engine.thick = pThick->load() / 100.0f; engine.pumpAmt = pPump->load() / 100.0f; engine.master = pMaster->load();
    engine.loudnessOn = pLoud->load() > .5f;
    engine.masterSet = { p ("mst_hp")->load() / 100.0f, p ("mst_lp")->load() / 100.0f, p ("mst_width")->load() / 100.0f, p ("mst_color")->load() / 100.0f, std::pow (10.0f, p ("mst_out")->load() / 20.0f), 0 };
    engine.alpha.store (p ("xs_alpha")->load() / 100.0f, std::memory_order_relaxed);
    {
        auto& dm = engine.drumMachine; dm.creative = pCreative->load() > .5f;
        for (int pad = 0; pad < hg::kNumPads; ++pad)
        {
            const auto& dp = dmPtr[(size_t) pad]; auto& c = dm.ctl[(size_t) pad];
            c.level = dp[0]->load() / 100.0f; c.tune = dp[1]->load(); c.decay = dp[2]->load() / 100.0f; c.punch = dp[3]->load() / 100.0f;
            c.soft = dp[4]->load() / 100.0f; c.mid = dp[5]->load(); c.high = dp[6]->load();
        }
    }

    // taşıma
    bool hostPlay = false; double ppq = 0, bpm = pBpm->load(); bool hasHostBpm = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) { bpm = *b; hasHostBpm = true; }
            if (auto qq = pos->getPpqPosition()) { ppq = *qq; hostPlay = pos->getIsPlaying(); }
        }
    bpm = juce::jlimit (20.0, 400.0, bpm);
    hostPlaying.store (hostPlay); hostBpm.store (hasHostBpm ? bpm : 0.0);
    engine.bpm = (float) bpm;
    const bool preview = previewPlaying.load() && ! hostPlay;
    if (preview) ppq = previewPos;
    const bool run = pPattern->load() > .5f && (hostPlay || preview) && livePattern != nullptr;
    const bool mOut = pMidiOut->load() > .5f;
    const int keysSel = (int) pKeysTo->load(), aud = auditionPart.load();
    const int keysTrack = aud == 3 ? hg::kDrums : aud >= 0 ? trackOfPart (aud) : keysSel == 0 ? hg::kChords : keysSel == 1 ? hg::kBass : hg::kMelody;   // 3: davul makinesi açık

    const bool sampOn = ref.sampler.enabled.load();
    numBlockEv = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isController() && midiLearn.handle (msg)) continue;      // eşlenmiş CC ya da MIDI Learn: kilitsiz, bellek ayırmadan
        if (msg.isControllerOfType (1)) engine.wheel.store (msg.getControllerValue() / 127.0f, std::memory_order_relaxed);   // Synth X matrisi: mod tekerleği
        else if (msg.isChannelPressure()) engine.after.store (msg.getChannelPressureValue() / 127.0f, std::memory_order_relaxed);
        else if (msg.isAftertouch()) engine.after.store (msg.getAfterTouchValue() / 127.0f, std::memory_order_relaxed);
        if (numBlockEv >= (int) blockEv.size()) break;
        if (msg.isNoteOn()) blockEv[(size_t) numBlockEv++] = { meta.samplePosition, 1, keysTrack, msg.getNoteNumber(), msg.getVelocity(), -1 };
        else if (msg.isNoteOff()) blockEv[(size_t) numBlockEv++] = { meta.samplePosition, 2, keysTrack, msg.getNoteNumber(), 0, 0 };
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) { engine.allNotesOff(); ref.samplerAllOff(); }
    }
    kbBuf.clear();
    keyboardState.processNextMidiBuffer (kbBuf, 0, n, true);   // ekran klavyesinden gelen notalar
    for (const auto meta : kbBuf)
    {
        const auto msg = meta.getMessage(); if (numBlockEv >= (int) blockEv.size()) break;
        if (msg.isNoteOn()) blockEv[(size_t) numBlockEv++] = { meta.samplePosition, 1, keysTrack, msg.getNoteNumber(), msg.getVelocity(), -1 };
        else if (msg.isNoteOff()) blockEv[(size_t) numBlockEv++] = { meta.samplePosition, 2, keysTrack, msg.getNoteNumber(), 0, 0 };
    }
    perform (n, bpm, hostPlay, ppq);   // akor modu, strum, arpej
    if (const int rq = demoReq.exchange (-1); rq != -1)
    {
        if (rq == -2) { if (demoRun) engine.allNotesOff(); demoRun = false; demoOn.store (false); }
        else { demoRun = true; demoTrack = rq >> 8; demoKind = juce::jlimit (0, hg::kNumDemo - 1, rq & 0xff); demoBeat = 0; }
    }
    if (demoRun)
    {
        const double bps = bpm / 60.0 / sr, b1 = demoBeat + n * bps;
        const auto& tab = demoTab.notes[(size_t) demoKind];
        for (int k = 0; k < demoTab.count[(size_t) demoKind] && numBlockEv < (int) blockEv.size(); ++k)
        {
            const auto& d = tab[(size_t) k];
            if (d.beat >= demoBeat && d.beat < b1)
                blockEv[(size_t) numBlockEv++] = { juce::jlimit (0, n - 1, (int) ((d.beat - demoBeat) / bps)), 3, demoTrack, d.note, d.vel, d.len * 60.0 / bpm };
        }
        demoBeat = b1;
        if (demoBeat >= hg::kDemoBeats) { demoRun = false; demoOn.store (false); }
    }
    midi.clear();
    if (! run && wasRunning)
    {
        engine.allNotesOff();
        for (int i = 0; i < numOffs; ++i) midi.addEvent (juce::MidiMessage::noteOff (offs[(size_t) i].ch, offs[(size_t) i].note), 0);
        numOffs = 0;
    }
    if (run)
    {
        const double pps = bpm / 60.0 / sr, ppqEnd = ppq + n * pps, L = livePattern->lenPpq;
        for (const auto& e : livePattern->ev)
        {
            const double k = std::ceil ((ppq - e.ppq) / L - 1e-9);
            for (double t = e.ppq + k * L; t < ppqEnd && numBlockEv < (int) blockEv.size(); t += L)
            {
                const int off = juce::jlimit (0, n - 1, (int) ((t - ppq) / pps));
                blockEv[(size_t) numBlockEv++] = { off, 0, e.track, e.note, e.vel, e.len * 60.0 / bpm };
            }
        }
        playPpq.store (std::fmod (std::fmod (ppq, L) + L, L));
        if (preview) previewPos = ppqEnd;
    }
    else if (! previewPlaying.load()) previewPos = 0;
    for (int i = 1; i < numBlockEv; ++i)   // kararlı ekleme sıralaması: ses iş parçacığında bellek ayırmaz
    {
        const auto e = blockEv[(size_t) i]; int j = i - 1;
        while (j >= 0 && blockEv[(size_t) j].off > e.off) { blockEv[(size_t) j + 1] = blockEv[(size_t) j]; --j; }
        blockEv[(size_t) j + 1] = e;
    }

    int pos = 0;
    for (int i = 0; i < numBlockEv; ++i)
    {
        const auto& e = blockEv[(size_t) i];
        if (e.off > pos) { engine.render (buffer, pos, e.off - pos); pos = e.off; }
        if (e.kind == 0)
        {
            const bool muted = engine.macros[(size_t) e.track].mute;
            engine.noteOn (e.track, e.note, e.vel, e.len, 0);
            if (mOut && ! muted && numOffs < (int) offs.size())
            {
                const int ch = kMidiCh[e.track];
                midi.addEvent (juce::MidiMessage::noteOn (ch, e.note, (juce::uint8) e.vel), e.off);
                const long long lenS = (long long) juce::jmax (1.0, (e.track == hg::kDrums ? 0.05 : e.len) * sr - 32);
                offs[(size_t) numOffs++] = { sampleClock + e.off + lenS, ch, e.note };
            }
        }
        else if (e.kind == 3) engine.noteOn (e.track, e.note, e.vel, e.len, 0);   // ön dinleme: MIDI çıkışına gitmez
        else if (e.kind == 1) { if (sampOn && e.track != hg::kDrums) ref.samplerNoteOn (e.note, e.vel, e.off); else engine.noteOn (e.track, e.note, e.vel, -1, 0); }
        else if (e.kind == 2) { if (sampOn && e.track != hg::kDrums) ref.samplerNoteOff (e.note); else engine.noteOff (e.track, e.note, 0); }   // -1: arpej tarafından tüketildi
    }
    if (pos < n) engine.render (buffer, pos, n - pos);
    for (int t = 0; t < 4; ++t)   // ek çıkışlar: master düzeyi ve -1 dBFS yumuşak sınır (ana çıkışın limiter'ı bunlara uygulanmaz)
        if (engine.auxL[t] != nullptr && engine.auxR[t] != nullptr)
            for (auto* x : { engine.auxL[t], engine.auxR[t] })
                for (int i = 0; i < n; ++i)
                {
                    const float y = x[i] * engine.master * .7f, ay = std::abs (y);
                    x[i] = ay <= .7f ? y : std::copysign (.7f + .191f * std::tanh ((ay - .7f) / .191f), y);
                }
    if (sampOn || wasSamp) ref.renderSampler (buffer, getSampleRate());
    if (wasSamp && ! sampOn) ref.samplerAllOff();
    wasSamp = sampOn;
    ref.render (buffer, getSampleRate());      // referans sesin orijinali (yalnızca kullanıcı çal dediğinde)
    intro.process (buffer, getSampleRate(), hostPlay || isNonRealtime());   // açılış sesi: taşıma ya da dışa aktarım başlarsa 20 ms içinde susar
    for (int i = 0; i < numOffs;)
    {
        if (offs[(size_t) i].at < sampleClock + n)
        {
            midi.addEvent (juce::MidiMessage::noteOff (offs[(size_t) i].ch, offs[(size_t) i].note), (int) juce::jmax (0LL, offs[(size_t) i].at - sampleClock));
            offs[(size_t) i] = offs[(size_t) --numOffs];
        }
        else ++i;
    }
    loop.capture (fromIn ? capIn : buffer, n);                  // son 30 sn: DAW girişi ya da HitGen çıkışı (döngünün kendisi hariç)
    loop.process (buffer, n, hostPlay || preview, ppq, bpm);
    choir.process (buffer, n, hostPlay || preview, ppq, bpm);
    sampleClock += n; wasRunning = run;
}

juce::String HitGenProcessor::baseName() const
{
    auto r = current; if (! r) return "HitGen";
    auto artist = (userArtist.isNotEmpty() ? userArtist : data.styleById (r->P.style)["artist"].toString()).toLowerCase();
    juce::String slug; for (auto c : artist) slug << (juce::CharacterFunctions::isLetterOrDigit (c) && c < 128 ? juce::String::charToString (c) : juce::String ("-"));
    while (slug.contains ("--")) slug = slug.replace ("--", "-");
    slug = slug.trimCharactersAtStart ("-").trimCharactersAtEnd ("-");
    return "HitGen_" + slug + "_" + data.noteNames[r->P.key].replace ("#", "s") + "_" + r->P.scale + "_" + juce::String (r->P.bpm) + "bpm_v" + juce::String (r->P.vi + 1) + "_seed" + juce::String (r->P.seed);
}

juce::MemoryBlock HitGenProcessor::midiBytes (int which) const
{
    auto r = current; if (! r) return {};
    hg::Result res = *r;
    const double hb = hostBpm.load(); res.P.bpm = hb > 0 ? (int) std::lround (hb) : (int) p ("bpm")->load();
    const std::vector<int> all { hg::kChords, hg::kBass, hg::kDrums, hg::kMelody };
    if (which == 5 || which == 6)
    {
        hg::Result song;
        if (skel.active) { auto sk = skeletonFor (res); song = std::move (sk.song); }
        else song = hg::assemble (data, res);
        return hg::midiFile (data, song, all, which == 6);   // 6: tam parça, tek iz (Format 0)
    }
    if (which == 4) return hg::midiFile (data, res, all, false);
    return hg::midiFile (data, res, { which }, false);
}

juce::File HitGenProcessor::writeMidi (int which)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("WavejackersHitGen");
    dir.createDirectory();
    const juce::String suffix = which == 6 ? "_tam-parca_format0" : which == 5 ? "_tam-parca" : which == 4 ? "_tum-izler" : juce::String ("_") + kPartIds[which];
    auto f = dir.getChildFile (baseName() + suffix + ".mid");
    auto bytes = midiBytes (which);
    if (bytes.getSize() > 0) f.replaceWithData (bytes.getData(), bytes.getSize());
    return f;
}

juce::ValueTree HitGenProcessor::stateTree()
{
    auto state = apvts.copyState();
    juce::ValueTree extra ("extra");
    for (int t = 0; t < 4; ++t) { extra.setProperty (juce::String ("sound_") + kPartIds[t], sounds[(size_t) t], nullptr); extra.setProperty (juce::String ("manual_") + kPartIds[t], manual[(size_t) t], nullptr); }
    extra.setProperty ("lang", lang, nullptr); extra.setProperty ("uiScale", uiScale, nullptr); extra.setProperty ("contrast", highContrast, nullptr);
    extra.setProperty ("sdv", 1, nullptr);   // ses tasarımı parametreleri geçerli (2.2+)
    for (int pt = 0; pt < 3; ++pt) if (xsEdit[(size_t) pt].isObject() && ! xsAuto[(size_t) pt]) extra.setProperty ("xs_" + juce::String (pt), juce::JSON::toString (xsEdit[(size_t) pt], true), nullptr);   // otomatik çevirme kaydedilmez: sesten yeniden kurulur
    extra.setProperty ("userArtist", userArtist, nullptr); extra.setProperty ("userPreset", userPreset, nullptr);
    if (ref.hasPath() || ref.sampler.enabled.load() || ref.sampler.enabled.load()) extra.setProperty ("ref", juce::JSON::toString (ref.stateToVar(), true), nullptr);
    for (int pad = 0; pad < hg::kNumPads; ++pad) { const auto sp = drumSamplePath (pad); if (sp.isNotEmpty()) extra.setProperty (juce::String ("dmSample_") + hg::kPadIds[pad], sp, nullptr); }
    for (int pt = 0; pt < 3; ++pt) { const auto sp = instSamplePath (pt); if (sp.isNotEmpty()) extra.setProperty ("instSample_" + juce::String (pt), sp, nullptr); }
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("kinds", skel.kinds.joinIntoString (","));
        juce::StringArray b; for (int x : skel.bars) b.add (juce::String (x));
        o->setProperty ("bars", b.joinIntoString (",")); o->setProperty ("dur", skel.durSec); o->setProperty ("struct", skel.structIdx);
        o->setProperty ("custom", skel.custom); o->setProperty ("active", skel.active);
        extra.setProperty ("skeleton", juce::JSON::toString (juce::var (o), true), nullptr);
    }
    if (coRole >= 0)
    {
        auto* o = new juce::DynamicObject(); juce::Array<juce::var> ns;
        for (auto& n : coNotes) ns.add (juce::Array<juce::var> { n.s, n.l, n.n, n.v });
        o->setProperty ("role", coRole); o->setProperty ("bars", coBars); o->setProperty ("name", coName); o->setProperty ("notes", ns);
        extra.setProperty ("coLock", juce::JSON::toString (juce::var (o), true), nullptr);
    }
    if (editActive && current != nullptr)
    {
        juce::Array<juce::var> trs;
        for (auto& tr : current->tracks) { juce::Array<juce::var> ns; for (auto& n : tr) ns.add (juce::Array<juce::var> { n.s, n.l, n.n, n.v }); trs.add (ns); }
        auto* o = new juce::DynamicObject(); o->setProperty ("key", editKey); o->setProperty ("tracks", trs);
        extra.setProperty ("midiEdit", juce::JSON::toString (juce::var (o), true), nullptr);
    }
    state.removeChild (state.getChildWithName ("extra"), nullptr);
    state.removeChild (state.getChildWithName ("midimap"), nullptr);     // MIDI eşlemeleri preset'e girmez
    state.appendChild (extra, nullptr);
    return state;
}

void HitGenProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = stateTree(); state.appendChild (midiLearn.toTree(), nullptr); state.appendChild (loop.toTree(), nullptr); state.appendChild (choir.toTree(), nullptr);   // eşlemeler yalnızca proje durumunda
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void HitGenProcessor::setStateInformation (const void* d, int size)
{
    auto xml = getXmlFromBinary (d, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    const auto mm = tree.getChildWithName ("midimap").createCopy(); tree.removeChild (tree.getChildWithName ("midimap"), nullptr);
    const auto lt = tree.getChildWithName ("loopforge").createCopy(); tree.removeChild (tree.getChildWithName ("loopforge"), nullptr);
    const auto ct = tree.getChildWithName ("choir").createCopy(); tree.removeChild (tree.getChildWithName ("choir"), nullptr);
    restoreFromTree (tree, true);
    if (ct.isValid()) { choir.fromTree (ct); pushChoirParams(); if (auto* pr = apvts.getParameter ("ch_play")) pr->setValueNotifyingHost (choir.isPlaying() ? 1.0f : 0.0f); }
    if (lt.isValid()) { loop.fromTree (lt); pushLoopParams(); }   // döngünün kendi kaydı esas: eski projelerde parametreler buradan gelir
    if (mm.isValid()) midiLearn.fromTree (mm);     // eski projelerde eşleme yok: mevcut (varsayılan) eşlemeler kalır
}

void HitGenProcessor::restoreFromTree (const juce::ValueTree& source, bool includeUi)
{
    if (! source.isValid() || ! source.hasType (apvts.state.getType())) return;
    auto tree = source.createCopy();
    auto extra = tree.getChildWithName ("extra");
    {
        const juce::ScopedValueSetter<bool> sv (suppressSd, true);
        apvts.replaceState (tree);
    }
    styleChanged.store (false); lastStyle = (int) p ("style")->load();
    bool sdValid = false;
    if (extra.isValid())
    {
        for (int t = 0; t < 4; ++t)
        {
            auto id = extra.getProperty (juce::String ("sound_") + kPartIds[t]).toString();
            const bool ok = t == 2 ? data.kitIndex (id) >= 0 : data.soundIndex (id) >= 0;   // bilinmeyen ses: stil varsayılanında kal
            if (ok) sounds[(size_t) t] = id;
            manual[(size_t) t] = (bool) extra.getProperty (juce::String ("manual_") + kPartIds[t]);
        }
        sdValid = (int) extra.getProperty ("sdv", 0) >= 1;
        for (int pt = 0; pt < 3; ++pt) { const auto js = extra.getProperty ("xs_" + juce::String (pt)).toString(); const auto v = js.length() < 2000000 ? juce::JSON::parse (js) : juce::var(); xsEdit[(size_t) pt] = v.isObject() ? v : juce::var(); xsAuto[(size_t) pt] = ! v.isObject() || (bool) extra.getProperty ("xsAuto_" + juce::String (pt), false); } ++xsVersion;
        missingSamples.clear();
        for (int pad = 0; pad < hg::kNumPads; ++pad)
        {
            const auto path = extra.getProperty (juce::String ("dmSample_") + hg::kPadIds[pad]).toString();
            juce::String err;
            if (path.isEmpty()) { if (drumSamplePath (pad).isNotEmpty()) clearDrumSample (pad); }
            else if (path != drumSamplePath (pad) && ! loadDrumSample (pad, juce::File (path), err, false)) { missingSamples.add (juce::File (path).getFileName()); clearDrumSample (pad); }
        }
        for (int pt = 0; pt < 3; ++pt)
        {
            const auto path = extra.getProperty ("instSample_" + juce::String (pt)).toString();
            juce::String err;
            if (path.isEmpty()) { if (instSamplePath (pt).isNotEmpty()) clearInstSample (pt); }
            else if (path != instSamplePath (pt) && ! loadInstSample (pt, juce::File (path), err)) { missingSamples.add (juce::File (path).getFileName()); clearInstSample (pt); }
        }
        editActive = false; editKey.clear(); savedEdit.reset();
        {
            const auto js = extra.getProperty ("midiEdit").toString();
            if (js.isNotEmpty() && js.length() < 4000000)
            {
                const auto v = juce::JSON::parse (js); const auto* trs = v["tracks"].getArray();
                if (trs != nullptr && trs->size() == hg::kNumTracks)
                {
                    auto e = std::make_unique<std::array<std::vector<hg::NoteEv>, hg::kNumTracks>>();
                    for (int t = 0; t < hg::kNumTracks; ++t)
                        if (const auto* ns = (*trs)[t].getArray())
                            for (const auto& n : *ns)
                                if (const auto* a = n.getArray(); a != nullptr && a->size() == 4)
                                    (*e)[(size_t) t].push_back ({ (double) (*a)[0], (double) (*a)[1], (int) (*a)[2], (int) (*a)[3] });
                    savedEdit = std::move (e); savedEditKey = v["key"].toString();
                }
            }
        }
        {
            skel = SkelState{}; skel.kinds = skStructKinds (0);
            const auto js = extra.getProperty ("skeleton").toString();
            if (js.isNotEmpty() && js.length() < 20000)
            {
                const auto v = juce::JSON::parse (js);
                juce::StringArray k; k.addTokens (v["kinds"].toString(), ",", "");
                juce::StringArray valid; for (auto& x : k) for (int i = 0; i < hg::kNumSkKinds; ++i) if (x == hg::kSkKinds[i]) valid.add (x);
                if (! valid.isEmpty() && valid.size() <= 24) skel.kinds = valid;
                juce::StringArray b; b.addTokens (v["bars"].toString(), ",", "");
                skel.bars.clear(); for (auto& x : b) skel.bars.push_back (juce::jlimit (8, 256, x.getIntValue() / 8 * 8));
                skel.durSec = juce::jlimit (0, 1200, (int) v["dur"]); skel.structIdx = juce::jlimit (0, kNumSkStructs - 1, (int) v["struct"]);
                skel.custom = (bool) v["custom"] && (int) skel.bars.size() == skel.kinds.size(); skel.active = (bool) v["active"];
            }
        }
        {
            coRole = -1; coNotes.clear(); coName.clear();
            const auto js = extra.getProperty ("coLock").toString();
            if (js.isNotEmpty() && js.length() < 2000000)
            {
                const auto v = juce::JSON::parse (js);
                const int role = (int) v["role"], bars = (int) v["bars"];
                if ((role == hg::kChords || role == hg::kBass || role == hg::kMelody) && (bars == 4 || bars == 8 || bars == 16))
                    if (const auto* ns = v["notes"].getArray())
                    {
                        for (auto& n : *ns) if (const auto* a = n.getArray(); a != nullptr && a->size() == 4)
                            coNotes.push_back ({ (double) (*a)[0], (double) (*a)[1], juce::jlimit (0, 127, (int) (*a)[2]), juce::jlimit (1, 127, (int) (*a)[3]) });
                        if (! coNotes.empty()) { coRole = role; coBars = bars; coName = v["name"].toString().substring (0, 120); }
                    }
            }
        }
        userArtist = extra.getProperty ("userArtist").toString().substring (0, 80);
        userPreset = extra.getProperty ("userPreset").toString().substring (0, 80);
        if (includeUi)
        {
            auto l = extra.getProperty ("lang").toString(); if (data.langCodes.contains (l)) lang = l;
            uiScale = juce::jlimit (.75f, 2.0f, (float) (double) extra.getProperty ("uiScale", 1.0));
            highContrast = (bool) extra.getProperty ("contrast", false);
            const auto rs = extra.getProperty ("ref").toString(); if (rs.isNotEmpty() && rs.length() < 100000) ref.restoreFromVar (juce::JSON::parse (rs));
        }
    }
    if (! sdValid) for (int part = 0; part < 3; ++part) applySoundToParams (part);   // 2.1 ve öncesi projeler: seçili sesin değerleri
    for (auto& f : sdDirty) f.store (false);
    const auto& S = data.style (juce::jlimit (0, data.numStyles() - 1, lastStyle));
    for (int t : { 0, 1, 3 }) { auto pm = S["pump"][kPartIds[t]]; engine.setPump (t, (float) (double) pm[0], (float) (double) pm[1]); }
    for (int t = 0; t < 4; ++t) loadPatch (t);
    regenPending.store (true);
}

bool HitGenProcessor::savePreset (const juce::String& artist, const juce::String& name, juce::String& fileOut)
{
    const auto a = artist.trim().substring (0, 80), n = name.trim().substring (0, 80);
    if (a.isEmpty() || n.isEmpty()) return false;
    userArtist = a; userPreset = n;
    const int si = juce::jlimit (0, data.numStyles() - 1, (int) p ("style")->load());
    auto f = presets.fileFor (a, n);
    fileOut = f.getFullPathName();
    return presets.write (f, a, n, data.styleId (si), stateTree());
}

bool HitGenProcessor::loadPreset (const juce::File& f)
{
    hg::PresetInfo info;
    auto tree = hg::PresetStore::readState (f, &info);
    if (! tree.isValid() || ! tree.hasType (apvts.state.getType())) return false;
    restoreFromTree (tree, false);
    userArtist = info.artist; userPreset = info.name;
    return true;
}

// ---------------------------------------------------------------- davul makinesi: örnekler ve kitler
juce::String HitGenProcessor::dmParamId (int pad, int k)
{
    static const char* const s[kDmCtl] = { "level", "tune", "decay", "punch", "soft", "mid", "high" };
    return juce::String ("dm_") + hg::kPadIds[juce::jlimit (0, hg::kNumPads - 1, pad)] + "_" + s[juce::jlimit (0, kDmCtl - 1, k)];
}

bool HitGenProcessor::isDrumSampleFile (const juce::File& f)
{
    for (auto* e : { ".wav", ".aif", ".aiff", ".flac", ".mp3", ".ogg" }) if (f.hasFileExtension (e)) return true;
    return false;
}

bool HitGenProcessor::loadDrumSample (int pad, const juce::File& f, juce::String& err, bool enableCreative)
{
    if (pad < 0 || pad >= hg::kNumPads) return false;
    if (! f.existsAsFile()) { err = "dmNoFile"; return false; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->sampleRate < 8000 || r->lengthInSamples < 16) { err = "dmBadFile"; return false; }
    const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 12.0));   // en çok 12 sn
    auto s = std::make_unique<hg::DrumSample>();
    const int ch = juce::jlimit (1, 2, (int) r->numChannels);
    s->buf.setSize (ch, len); s->buf.clear();
    if (! r->read (&s->buf, 0, len, 0, true, ch > 1)) { err = "dmBadFile"; return false; }
    // baştaki sessizliği kırp (ped hemen vursun) ve tepeyi 0.95'e getir
    int start = 0; const float thr = s->buf.getMagnitude (0, len) * 0.02f;
    while (start < len - 16) { bool loud = false; for (int c = 0; c < ch; ++c) if (std::abs (s->buf.getSample (c, start)) > thr) loud = true; if (loud) break; ++start; }
    if (start > 0) { juce::AudioBuffer<float> t (ch, len - start); for (int c = 0; c < ch; ++c) t.copyFrom (c, 0, s->buf, c, start, len - start); s->buf = std::move (t); }
    const float pk = s->buf.getMagnitude (0, s->buf.getNumSamples());
    if (pk < 1e-5f) { err = "dmSilent"; return false; }
    s->buf.applyGain (0.95f / pk);
    s->rate = r->sampleRate; s->path = f.getFullPathName(); s->name = f.getFileNameWithoutExtension();
    engine.drumMachine.setSample (pad, std::move (s));
    { const juce::ScopedLock sl (drumLock); drumSamples[(size_t) pad] = f.getFullPathName(); }
    if (enableCreative) if (auto* pr = apvts.getParameter ("drumCreative")) if (pr->getValue() < .5f) { pr->beginChangeGesture(); pr->setValueNotifyingHost (1.0f); pr->endChangeGesture(); }
    return true;
}

std::unique_ptr<hg::InstSample> HitGenProcessor::makeInstSample (const juce::File& f, juce::String& err)
{
    if (! f.existsAsFile()) { err = "dmNoFile"; return nullptr; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->sampleRate < 8000 || r->lengthInSamples < 64) { err = "dmBadFile"; return nullptr; }
    const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 30.0));   // en çok 30 sn
    auto s = std::make_unique<hg::InstSample>();
    const int ch = juce::jlimit (1, 2, (int) r->numChannels);
    s->buf.setSize (ch, len); s->buf.clear();
    if (! r->read (&s->buf, 0, len, 0, true, ch > 1)) { err = "dmBadFile"; return nullptr; }
    int start = 0; const float thr = s->buf.getMagnitude (0, len) * 0.02f;
    while (start < len - 64) { bool loud = false; for (int c = 0; c < ch; ++c) if (std::abs (s->buf.getSample (c, start)) > thr) loud = true; if (loud) break; ++start; }
    if (start > 0) { juce::AudioBuffer<float> t (ch, len - start); for (int c = 0; c < ch; ++c) t.copyFrom (c, 0, s->buf, c, start, len - start); s->buf = std::move (t); }
    const float pk = s->buf.getMagnitude (0, s->buf.getNumSamples());
    if (pk < 1e-5f) { err = "dmSilent"; return nullptr; }
    s->buf.applyGain (0.9f / pk);
    // dosya adında nota varsa (ör. "Pad_C3", "bass-F#1") onu, yoksa perde bulucuyu, o da yoksa C3 (60)
    int root = -1;
    {
        static const char* nm[] = { "C#", "Db", "D#", "Eb", "F#", "Gb", "G#", "Ab", "A#", "Bb", "C", "D", "E", "F", "G", "A", "B" };
        static const int pc[] = { 1, 1, 3, 3, 6, 6, 8, 8, 10, 10, 0, 2, 4, 5, 7, 9, 11 };
        const auto base = f.getFileNameWithoutExtension();
        for (int i = 0; i < 17 && root < 0; ++i)
        {
            const juce::String n (nm[i]);
            for (int pos = base.indexOf (n); pos >= 0 && root < 0; pos = base.indexOf (pos + 1, n))
            {
                const bool leftOk = pos == 0 || ! juce::CharacterFunctions::isLetter (base[pos - 1]);
                const auto rest = base.substring (pos + n.length());
                const bool oneDigit = rest.isNotEmpty() && juce::CharacterFunctions::isDigit (rest[0]) && (rest.length() == 1 || ! juce::CharacterFunctions::isDigit (rest[1]));
                if (leftOk && oneDigit)
                    root = juce::jlimit (0, 127, (rest.getIntValue() + 1) * 12 + pc[i]);   // bilimsel adlandırma: C4 = 60
            }
        }
    }
    if (root < 0) root = hg::detectRootNote (s->buf, r->sampleRate);
    if (root < 0) root = 60;
    s->root = root; s->rate = r->sampleRate; s->path = f.getFullPathName(); s->name = f.getFileNameWithoutExtension();
    return s;
}

bool HitGenProcessor::loadInstSample (int part, const juce::File& f, juce::String& err, int* rootOut)
{
    if (part < 0 || part > 2) return false;
    auto s = makeInstSample (f, err);
    if (s == nullptr) return false;
    const int root = s->root;
    engine.inst[part].setSample (std::move (s));
    { const juce::ScopedLock sl (drumLock); instSamples[(size_t) part] = f.getFullPathName(); }
    if (rootOut != nullptr) *rootOut = root;
    return true;
}

void HitGenProcessor::clearInstSample (int part)
{
    if (part < 0 || part > 2) return;
    engine.inst[part].setSample (nullptr); const juce::ScopedLock sl (drumLock); instSamples[(size_t) part].clear();
}

juce::String HitGenProcessor::paramsKey() const
{
    auto g = currentParams();
    return g.style + "|" + juce::String (g.vi) + "|" + juce::String (g.seed) + "|" + juce::String (g.key) + "|" + g.scale + "|" + juce::String (g.bars) + "|"
         + juce::String (g.orig) + "|" + juce::String (g.energy) + "|" + juce::String (g.human) + "|" + juce::String (g.swing) + "|" + juce::String (g.thick) + "|"
         + juce::String ((int) g.sevenths) + "|" + juce::String (g.alt[0]) + "," + juce::String (g.alt[1]) + "," + juce::String (g.alt[2]) + "," + juce::String (g.alt[3]);
}

void HitGenProcessor::applyEditedResult (std::shared_ptr<hg::Result> r)
{
    if (r == nullptr) return;
    for (auto& tr : r->tracks)
    {
        for (auto& n : tr) { n.s = juce::jlimit (0.0, r->bars * 16.0 - 0.25, n.s); n.l = juce::jlimit (0.25, r->bars * 16.0 - n.s, n.l); n.n = juce::jlimit (0, 127, n.n); n.v = juce::jlimit (1, 127, n.v); }
        std::stable_sort (tr.begin(), tr.end(), [] (const hg::NoteEv& a, const hg::NoteEv& b) { return a.s < b.s; });
    }
    pushPattern (*r);
    current = r; editActive = true; editKey = paramsKey();
    if (onResultChanged) onResultChanged();
}

void HitGenProcessor::revertEdits()
{
    editActive = false; editKey.clear();
    regenerateNow();
}

void HitGenProcessor::clearDrumSample (int pad)
{
    if (pad < 0 || pad >= hg::kNumPads) return;
    engine.drumMachine.setSample (pad, nullptr); const juce::ScopedLock sl (drumLock); drumSamples[(size_t) pad].clear();
}

juce::File HitGenProcessor::drumKitFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("Kits");
}

juce::Array<juce::File> HitGenProcessor::drumKitFiles()
{
    auto d = drumKitFolder(); juce::Array<juce::File> out;
    if (d.isDirectory()) out = d.findChildFiles (juce::File::findFiles, false, "*.hgkit");
    out.sort(); return out;
}

bool HitGenProcessor::saveDrumKit (const juce::String& name, juce::File& written)
{
    const auto nm = juce::File::createLegalFileName (name.trim().substring (0, 60)).trim(); if (nm.isEmpty()) return false;
    auto* root = new juce::DynamicObject(); root->setProperty ("hgkit", 1); root->setProperty ("name", name.trim().substring (0, 60));
    auto* pads = new juce::DynamicObject();
    for (int pad = 0; pad < hg::kNumPads; ++pad)
    {
        auto* o = new juce::DynamicObject();
        for (int k = 0; k < kDmCtl; ++k) o->setProperty (dmParamId (pad, k).fromLastOccurrenceOf ("_", false, false), dmPtr[(size_t) pad][(size_t) k]->load());
        if (drumSamplePath (pad).isNotEmpty()) o->setProperty ("sample", drumSamplePath (pad));
        pads->setProperty (hg::kPadIds[pad], juce::var (o));
    }
    root->setProperty ("pads", juce::var (pads));
    auto d = drumKitFolder(); d.createDirectory();
    written = d.getChildFile (nm + ".hgkit");
    return written.replaceWithText (juce::JSON::toString (juce::var (root), false));
}

bool HitGenProcessor::loadDrumKit (const juce::File& f, juce::StringArray& missing)
{
    if (f.getSize() > 200000) return false;
    const auto v = juce::JSON::parse (f.loadFileAsString());
    if (! v.isObject() || (int) v["hgkit"] < 1) return false;
    const auto pads = v["pads"];
    for (int pad = 0; pad < hg::kNumPads; ++pad)
    {
        const auto o = pads[juce::Identifier (hg::kPadIds[pad])];
        for (int k = 0; k < kDmCtl; ++k)
        {
            const auto key = dmParamId (pad, k).fromLastOccurrenceOf ("_", false, false);
            auto* pr = apvts.getParameter (dmParamId (pad, k)); if (pr == nullptr) continue;
            const auto& rg = apvts.getParameterRange (dmParamId (pad, k));
            const float plain = o.hasProperty (juce::Identifier (key)) ? (float) (double) o[juce::Identifier (key)] : rg.convertFrom0to1 (pr->getDefaultValue());
            pr->beginChangeGesture(); pr->setValueNotifyingHost (rg.convertTo0to1 (rg.snapToLegalValue (plain))); pr->endChangeGesture();
        }
        const auto path = o["sample"].toString(); juce::String err;
        if (path.isEmpty()) clearDrumSample (pad);
        else if (! loadDrumSample (pad, juce::File (path), err, false)) { missing.add (juce::File (path).getFileName()); clearDrumSample (pad); }
    }
    if (auto* pr = apvts.getParameter ("drumCreative")) { pr->beginChangeGesture(); pr->setValueNotifyingHost (1.0f); pr->endChangeGesture(); }
    return true;
}

void HitGenProcessor::resetDrumPad (int pad)
{
    for (int k = 0; k < kDmCtl; ++k)
        if (auto* pr = apvts.getParameter (dmParamId (pad, k))) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->getDefaultValue()); pr->endChangeGesture(); }
    clearDrumSample (pad);
}

juce::AudioProcessorEditor* HitGenProcessor::createEditor() { return new HitGenEditor (*this); }

#if ! HITGEN_TEST_BUILD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HitGenProcessor(); }
#endif


// ================================================================ proje iskeleti
const int HitGenProcessor::kSkDurations[8] = { 0, 240, 300, 360, 420, 480, 540, 600 };

juce::StringArray HitGenProcessor::skStructKinds (int idx) const
{
    static const char* const S[4][9] = {
        { "intro", "build", "drop", "break", "build", "drop", "outro" },
        { "intro", "drop", "break", "build", "drop", "outro" },
        { "intro", "build", "drop", "break", "groove", "break", "build", "drop", "outro" },
        { "intro", "groove", "break", "groove", "outro" } };
    juce::StringArray k;
    if (idx >= 0 && idx < 4) { for (auto* x : S[idx]) if (x != nullptr) k.add (x); return k; }
    // stilin kendi yapısı: bölüm adlarından tür
    const auto& St = data.style (juce::jlimit (0, data.numStyles() - 1, (int) p ("style")->load()));
    if (auto* arr = St["arr"].getArray())
        for (auto& sec : *arr)
        {
            const auto n = sec[0].toString().toLowerCase();
            k.add (n.contains ("intro") ? "intro" : n.contains ("outro") ? "outro" : n.contains ("build") ? "build" : n.contains ("break") ? "break"
                   : n.contains ("verse") ? "verse" : n.contains ("groove") ? "groove" : "drop");
        }
    return k;
}

void HitGenProcessor::skSetStructure (int idx)
{
    skel.structIdx = juce::jlimit (0, kNumSkStructs - 1, idx);
    skel.kinds = skStructKinds (skel.structIdx); skel.custom = false; skel.bars.clear();
}
void HitGenProcessor::skSetDuration (int sec) { skel.durSec = juce::jlimit (0, 1200, sec); skel.custom = false; skel.bars.clear(); }

void HitGenProcessor::skSetKinds (const juce::StringArray& kinds, bool keepBars)
{
    if (kinds.isEmpty() || kinds.size() > 24) return;
    if (keepBars && skel.custom && (int) skel.bars.size() == kinds.size()) { skel.kinds = kinds; return; }
    if (keepBars && ! skel.custom) { skel.kinds = kinds; return; }   // uzunluklar yine süreden hesaplanır
    skel.kinds = kinds; skel.custom = false; skel.bars.clear();
}

void HitGenProcessor::skSetBars (int section, int bars)
{
    auto sk = skeletonNow();
    if (section < 0 || section >= (int) sk.secs.size()) return;
    skel.bars.clear(); for (auto& x : sk.secs) skel.bars.push_back (x.bars);
    skel.bars[(size_t) section] = juce::jlimit (8, 256, bars / 8 * 8);
    skel.custom = true;
}

static hg::Skeleton skeletonOf (const hg::Data& data, const HitGenProcessor::SkelState& sk, const hg::Result& res, const juce::StringArray& kinds)
{
    std::vector<int> bars;
    if (sk.custom && (int) sk.bars.size() == kinds.size()) bars = sk.bars;
    else if (sk.durSec > 0) bars = hg::skBars (kinds, sk.durSec, res.P.bpm);
    else if (sk.structIdx == HitGenProcessor::kNumSkStructs - 1)
    {   // stil uzunluğu + stilin yapısı: stilin kendi ölçüleri
        if (auto* arr = data.styleById (res.P.style)["arr"].getArray()) for (auto& sec : *arr) bars.push_back ((int) sec[1]);
        if ((int) bars.size() != kinds.size()) bars = hg::skBars (kinds, 360, res.P.bpm);
    }
    else
    {   // stil uzunluğu: stil aranjmanının süresi kadar
        int tb = 0; if (auto* arr = data.styleById (res.P.style)["arr"].getArray()) for (auto& sec : *arr) tb += (int) sec[1];
        bars = hg::skBars (kinds, juce::jmax (60, (int) std::lround (tb * 240.0 / juce::jmax (1, res.P.bpm))), res.P.bpm);
    }
    return hg::skeleton (data, res, kinds, 0, &bars);
}

hg::Skeleton HitGenProcessor::skeletonFor (const hg::Result& res) const
{
    auto kinds = skel.kinds.isEmpty() ? skStructKinds (skel.structIdx) : skel.kinds;
    auto sk = skeletonOf (data, skel, res, kinds);
    for (auto& c : sk.cues) sk.song.marks.push_back ({ "> " + c.type, c.bar, c.len });   // DAW'da ipucu işaretçileri
    return sk;
}

hg::Skeleton HitGenProcessor::skeletonNow() const
{
    auto r = current;
    if (r == nullptr) return {};
    hg::Result res = *r;
    const double hb = hostBpm.load(); res.P.bpm = hb > 0 ? (int) std::lround (hb) : (int) p ("bpm")->load();
    return skeletonFor (res);
}

hg::Result HitGenProcessor::fullSong() const
{
    if (skel.active) return skeletonNow().song;
    auto r = current; if (r == nullptr) return {};
    return hg::assemble (data, *r);
}

juce::MemoryBlock HitGenProcessor::sectionMidi (int section) const
{
    const auto sk = skeletonNow();
    if (section < 0 || section >= (int) sk.secs.size()) return {};
    const auto& x = sk.secs[(size_t) section];
    return hg::midiFile (data, hg::sliceResult (sk.song, x.bar, x.bars), { hg::kChords, hg::kBass, hg::kDrums, hg::kMelody }, false);
}

std::vector<std::pair<hg::SkCue, juce::File>> HitGenProcessor::skPicks (const hg::Skeleton& sk) const
{
    std::vector<std::pair<hg::SkCue, juce::File>> out;
    auto L = lib; const int bpm = sk.song.P.bpm; const auto seed = (juce::uint32) sk.song.P.seed;
    for (auto& c : sk.cues)
    {
        if (hg::cueAudio (c.type).cat < 0) continue;
        const hg::SampleEntry* e = L != nullptr ? L->pick (c.type, bpm, seed, c.bar) : nullptr;
        out.push_back ({ c, e != nullptr ? e->file : juce::File() });
    }
    return out;
}

juce::File HitGenProcessor::projectsFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Wavejackers HitGen").getChildFile ("Projects");
}
juce::File HitGenProcessor::sampleFoldersFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("sample-folders.txt");
}

namespace
{
struct ScanJob : juce::ThreadPoolJob
{
    ScanJob (juce::StringArray f, std::function<void (hg::SampleLib)> d, std::atomic<int>& pr) : ThreadPoolJob ("scan"), folders (std::move (f)), done (std::move (d)), progress (pr) {}
    JobStatus runJob() override
    {
        auto L = hg::SampleLib::scan (folders, [this] { return shouldExit(); }, &progress);
        if (! shouldExit()) done (std::move (L)); else done ({});
        return jobHasFinished;
    }
    juce::StringArray folders; std::function<void (hg::SampleLib)> done; std::atomic<int>& progress;
};
struct ExportJob : juce::ThreadPoolJob
{
    ExportJob (hg::ExportJobInput in_, std::function<void (hg::ExportResult)> d, std::atomic<int>& pr) : ThreadPoolJob ("export"), in (std::move (in_)), done (std::move (d)), progress (pr) {}
    JobStatus runJob() override { done (hg::exportProject (in, [this] { return shouldExit(); }, &progress)); return jobHasFinished; }
    hg::ExportJobInput in; std::function<void (hg::ExportResult)> done; std::atomic<int>& progress;
};
}

void HitGenProcessor::setSampleFolders (const juce::StringArray& f, bool persist)
{
    folders.clear();
    for (auto& x : f) if (x.isNotEmpty() && ! folders.contains (x)) folders.add (x);
    libRequested = true;
    if (persist)
    {
        auto file = sampleFoldersFile(); file.getParentDirectory().createDirectory();
        file.replaceWithText (folders.joinIntoString ("\n"));
    }
    if (folders.isEmpty()) { lib = std::make_shared<const hg::SampleLib>(); ++libRevision; return; }
    if (scanBusy.exchange (true)) { rescan.store (true); return; }   // bitince yeniden taranır
    scanProgress.store (0);
    pool.addJob (new ScanJob (folders, [this] (hg::SampleLib L) { const std::lock_guard<std::mutex> g (jobMx); doneLib = std::move (L); }, scanProgress), true);
}

void HitGenProcessor::ensureLibrary()
{
    if (libRequested) return;
    libRequested = true;
    juce::StringArray f; f.addLines (sampleFoldersFile().loadFileAsString());
    f.removeEmptyStrings();
    if (! f.isEmpty()) setSampleFolders (f, false);
}

bool HitGenProcessor::pollLibrary()
{
    std::optional<hg::SampleLib> L;
    { const std::lock_guard<std::mutex> g (jobMx); L.swap (doneLib); }
    if (! L) return false;
    scanBusy.store (false);
    if (rescan.exchange (false)) { setSampleFolders (folders, false); return false; }
    L->folders = folders;
    lib = std::make_shared<const hg::SampleLib> (std::move (*L)); ++libRevision;
    return true;
}

int HitGenProcessor::loadSuggestedDrums()
{
    auto L = lib; if (L == nullptr) return 0;
    struct P { int pad, cat; const char* pref; };
    const P map[] = { { hg::kPadKick, hg::kCatKick, "" }, { hg::kPadSnare, hg::kCatSnare, "" }, { hg::kPadClap, hg::kCatClap, "" }, { hg::kPadHat, hg::kCatHat, "" },
                      { hg::kPadOpenHat, hg::kCatOpenHat, "" }, { hg::kPadRim, hg::kCatPerc, "rim" }, { hg::kPadPerc, hg::kCatShaker, "" } };
    int n = 0; auto r = current; const int bpm = (int) p ("bpm")->load(); const auto seed = (juce::uint32) (r ? r->P.seed : 7);
    for (auto& m : map)
        if (auto* e = L->pickCat (m.cat, m.pref, bpm, seed, m.pad * 131, true))
        {
            juce::String err;
            if (loadDrumSample (m.pad, e->file, err, true)) ++n;
        }
    return n;
}

bool HitGenProcessor::startExport (const juce::File& dir, const juce::String& planText)
{
    if (exportBusy.exchange (true)) return false;
    hg::ExportJobInput in;
    in.dir = dir; in.planText = planText; in.baseName = baseName();
    in.sk = skeletonNow(); in.bpm = in.sk.song.P.bpm; in.seed = (juce::uint32) in.sk.song.P.seed;
    in.sampleRate = sr >= 44100 ? sr : 44100;
    const std::vector<int> all { hg::kChords, hg::kBass, hg::kDrums, hg::kMelody };
    in.midis.push_back (hg::midiFile (data, in.sk.song, all, false));
    for (int t : all) { auto one = in.sk.song; one.marks.clear(); in.midis.push_back (hg::midiFile (data, one, { t }, false)); }
    in.picks = skPicks (in.sk);
    renderExportSounds (in);
    in.writeStems = exportStems;
    exportProgress.store (0);
    pool.addJob (new ExportJob (std::move (in), [this] (hg::ExportResult R) { const std::lock_guard<std::mutex> g (jobMx); doneExport = std::move (R); }, exportProgress), true);
    return true;
}

bool HitGenProcessor::pollExport (hg::ExportResult& out)
{
    std::optional<hg::ExportResult> R;
    { const std::lock_guard<std::mutex> g (jobMx); R.swap (doneExport); }
    if (! R) return false;
    exportBusy.store (false); out = std::move (*R);
    return true;
}

// FL Studio projesi için HitGen sesleri: ayrı bir motor örneğiyle (çalan motoru etkilemez) mesaj iş parçacığında render.
// Akor/bas/melodi C5'te (MIDI 60) tek nota: FL Sampler kökü C5 olduğundan notalar doğru perdeden çalar.
void HitGenProcessor::renderExportSounds (hg::ExportJobInput& in)
{
    const double rsr = 44100; in.renderRate = rsr;
    auto E = std::make_shared<hg::Engine>();
    E->prepare (rsr, 512);
    E->loudnessOn = false; E->pumpAmt = 0; E->master = 1; E->thick = pThick->load() / 100.0f; E->bpm = (float) juce::jmax (1, in.bpm);
    E->quality = hg::Quality { 7, 8, true };
    for (int t = 0; t < 4; ++t)
    {
        const auto& q = pp[(size_t) t]; auto& m = E->macros[(size_t) t];
        m.volume = q.vol->load(); m.mute = false;
        if (t == 2) continue;
        m.cutoff = q.cut->load(); m.reso = q.res->load(); m.attack = q.atk->load(); m.release = q.rel->load();
        m.drive = q.drv->load(); m.reverb = q.rev->load(); m.delay = q.dly->load(); m.movement = q.mov != nullptr ? q.mov->load() : 0.0f;
    }
    {
        auto ki = data.kitIndex (sounds[2]); E->setKit (data.kit (ki >= 0 ? ki : 0));
        auto& dm = E->drumMachine;
        for (int pad = 0; pad < hg::kNumPads; ++pad)
        {
            const auto& dp = dmPtr[(size_t) pad]; auto& c = dm.ctl[(size_t) pad];
            c.level = dp[0]->load() / 100.0f; c.tune = dp[1]->load(); c.decay = dp[2]->load() / 100.0f; c.punch = dp[3]->load() / 100.0f;
            c.soft = dp[4]->load() / 100.0f; c.mid = dp[5]->load(); c.high = dp[6]->load();
        }
    }
    for (int t : { hg::kChords, hg::kBass, hg::kMelody })
    {
        const auto& base = data.soundById (sounds[(size_t) t]);
        if (base.isVoid()) continue;
        const int part = partOfTrack (t);
        float vals[hg::kNumSp]; readSd (part, vals);
        E->setPatch (part, hg::Patch::fromVar (hg::spToSound (base, vals), data, rsr));
        const auto path = instSamplePath (part);
        juce::String err;
        if (path.isNotEmpty()) if (auto smp = makeInstSample (juce::File (path), err)) E->inst[part].setSample (std::move (smp));
    }
    juce::AudioBuffer<float> blk (2, 512);
    auto renderFor = [&] (double seconds, const std::function<void()>& trigger)
    {
        blk.clear(); E->render (blk, 0, 512);   // bekleyen yamaları al
        E->allNotesOff();
        for (int i = 0; i < 40; ++i) { blk.clear(); E->render (blk, 0, 512); }   // önceki notanın kuyruğu sönsün
        trigger();
        const int total = (int) (rsr * seconds);
        juce::AudioBuffer<float> out (2, total); out.clear();
        for (int pos = 0; pos < total; pos += 512)
        {
            const int n = juce::jmin (512, total - pos);
            blk.clear(); E->render (blk, 0, n);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, blk, c, 0, n);
        }
        // sondaki sessizliği kırp, kısa kuyruk sönümü, tepe 0.9
        int end = total;
        const float pk = out.getMagnitude (0, total);
        if (pk < 1.0e-5f) return juce::AudioBuffer<float>();
        while (end > 512 && out.getMagnitude (end - 256, 256) < pk * 0.001f) end -= 256;
        juce::AudioBuffer<float> r (2, end);
        for (int c = 0; c < 2; ++c) r.copyFrom (c, 0, out, c, 0, end);
        const int fade = juce::jmin (end / 4, (int) (rsr * .01));
        for (int c = 0; c < 2; ++c) r.applyGainRamp (c, end - fade, fade, 1.0f, 0.0f);
        r.applyGain (.9f / pk);
        return r;
    };
    // nota uzunluğu: şarkıdaki notaların ortancası (FL Sampler sample'ı sonuna dek çalar)
    const double stepSec = 60.0 / juce::jmax (1, in.bpm) / 4.0;
    // aranjmandaki her melodik şerit için tek nota (C5): bas, akor, lead, pad (akor sesi, uzun), arp (lead sesi, kısa)
    const auto arr = hg::buildArrangement (in.sk);
    for (auto& L : arr.lanes)
    {
        if (L.gm >= 0) continue;
        std::vector<double> lens; for (auto& n : L.notes) lens.push_back (n.l);
        std::nth_element (lens.begin(), lens.begin() + (long) lens.size() / 2, lens.end());
        const double med = lens.empty() ? 4.0 : lens[lens.size() / 2];
        const bool longOne = L.role == "chords" || L.role == "pad";
        const double lo = longOne ? .5 : .12, hi = L.role == "pad" ? 8.0 : longOne ? 6.0 : 2.0;
        const double dur = juce::jlimit (lo, hi, med * stepSec);
        auto r = renderFor (dur + (longOne ? 1.0 : .35), [&] { E->noteOn (L.part, 60, 104, dur, 0); });
        // FL Sampler örneği sonuna dek çalar: notadan sonra kısa bir sönüm, kuyruklar üst üste binmesin
        const int from = (int) (rsr * dur), rel = (int) (rsr * (longOne ? .6 : .12));
        if (r.getNumSamples() > from + 8)
        {
            const int n = juce::jmin (rel, r.getNumSamples() - from);
            for (int c = 0; c < r.getNumChannels(); ++c) r.applyGainRamp (c, from, n, 1.0f, 0.0f);
            r.setSize (r.getNumChannels(), from + n, true);
        }
        in.laneRenders[L.role] = std::move (r);
    }
    for (auto& L : arr.lanes)
        if (L.gm >= 0) in.laneRenders[L.role] = renderFor (1.5, [&] { E->drumHit (L.gm, 112, 0); });
    // kütüphane varsa davullar için önerilen one-shot'lar
    if (auto Lb = lib)
    {
        const struct { const char* role; int cat; const char* pref; int salt; } m[] = { { "kick", hg::kCatKick, "", 36 }, { "snare", hg::kCatSnare, "", 38 }, { "clap", hg::kCatClap, "", 39 },
            { "hat", hg::kCatHat, "", 42 }, { "ohat", hg::kCatOpenHat, "", 46 }, { "rim", hg::kCatPerc, "rim", 37 }, { "perc", hg::kCatShaker, "", 64 } };
        for (auto& x : m)
            if (in.laneRenders.count (x.role))
                if (auto* e = Lb->pickCat (x.cat, x.pref, in.bpm, in.seed, x.salt * 131, true)) in.laneFiles[x.role] = e->file;
    }
    E->collectGarbage();
    E->quality = hg::Quality { 4, 16, true };   // stem'ler: tam şarkı, dengeli kalite
    in.stemEngine = E;
}

// ================================================================ Co-Producer
void HitGenProcessor::coLock (const hg::UserMidi& um, const hg::CoCandidate& c)
{
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); pr->endChangeGesture(); } };
    coRole = um.role; coNotes = um.notes; coBars = um.bars; coName = um.name;
    editActive = false; editKey.clear(); savedEdit.reset();
    setP ("key", (float) c.P.key);
    const int si = data.scaleOrder.indexOf (c.P.scale); setP ("scale", (float) (si >= 0 ? si + 1 : 0));
    setP ("bars", (float) (um.bars == 4 ? 0 : um.bars == 16 ? 2 : 1));
    setP ("seed", (float) c.P.seed); setP ("variation", (float) c.P.vi);
    for (auto* a : { "altC", "altB", "altD", "altM" }) setP (a, 0.0f);
    if (um.bpm >= 60 && um.bpm <= 190) setP ("bpm", (float) juce::roundToInt (um.bpm));
    regenerateNow();
}

void HitGenProcessor::coUnlock()
{
    coRole = -1; coNotes.clear(); coName.clear();
    regenerateNow();
}

// ---------------------------------------------------------------- Synth X çalım yardımcıları (ses iş parçacığı; bellek ayırmaz)
int HitGenProcessor::chordNotes (int part, int note, int* out) const noexcept
{
    const int mode = perf[(size_t) part].chord.load (std::memory_order_relaxed);
    out[0] = note; if (mode == 0) return 1;
    if (mode == 3) { out[1] = note + 7; return 2; }
    if (mode == 4) { out[1] = note + 12; return 2; }
    const int mask = scaleMask.load (std::memory_order_relaxed), key = scaleKey.load (std::memory_order_relaxed);
    auto inScale = [&] (int n) { return (mask >> (((n - key) % 12 + 12) % 12) & 1) != 0; };
    auto up = [&] (int steps) { int n = note, k = 0; while (k < steps && n < note + 24) { ++n; if (inScale (n)) ++k; } return n; };   // gamda kaç adım yukarı
    if (mode == 5) { out[1] = up (1); out[2] = up (4); return 3; }
    if (mode == 6) { out[1] = up (3); out[2] = up (4); return 3; }
    out[1] = up (2); out[2] = up (4); if (mode == 2) { out[3] = up (6); return 4; }
    return 3;
}

void HitGenProcessor::perform (int n, double bpm, bool hostPlay, double ppq)
{
    const double sr = getSampleRate(); const int count = numBlockEv;
    auto push = [&] (BlockEv e) { if (numBlockEv < (int) blockEv.size()) blockEv[(size_t) numBlockEv++] = e; };
    for (int i = 0; i < count; ++i)
    {
        auto& e = blockEv[(size_t) i];
        if ((e.kind != 1 && e.kind != 2) || e.track == hg::kDrums) continue;
        const int part = partOfTrack (e.track); auto& pf = perf[(size_t) part];
        int cn[4]; const int nc = chordNotes (part, e.note, cn);
        if (pf.arpOn.load (std::memory_order_relaxed) != 0)
        {   // arpej: tuşlar yalnızca tutulan nota listesini değiştirir
            auto& H = held[(size_t) part]; int& nh = numHeld[(size_t) part];
            for (int k = 0; k < nc; ++k)
            {
                const int x = cn[k]; int at = -1; for (int j = 0; j < nh; ++j) if (H[(size_t) j] == x) at = j;
                if (e.kind == 1 && at < 0 && nh < (int) H.size()) { H[(size_t) nh++] = x; arpVel[(size_t) part] = e.vel; }
                if (e.kind == 2 && at >= 0) { for (int j = at; j < nh - 1; ++j) H[(size_t) j] = H[(size_t) j + 1]; --nh; }
            }
            if (nh == 0) arpIdx[(size_t) part] = 0;
            e.kind = -1; continue;
        }
        if (nc == 1) continue;
        const int strumS = (int) (pf.strum.load (std::memory_order_relaxed) * .12 * sr);
        if (e.kind == 2)
        {
            for (int k = 1; k < nc; ++k) push ({ e.off, 2, e.track, cn[k], 0, 0 });
            for (int j = 0; j < numPend;) if (pend[(size_t) j].track == e.track && pend[(size_t) j].root == e.note) pend[(size_t) j] = pend[(size_t) --numPend]; else ++j;   // bekleyen strum notaları iptal
            continue;
        }
        for (int k = 1; k < nc; ++k)
        {
            if (strumS <= 0) push ({ e.off, 1, e.track, cn[k], e.vel, -1 });
            else if (numPend < (int) pend.size()) pend[(size_t) numPend++] = { sampleClock + e.off + (long long) k * strumS, e.track, cn[k], juce::jmax (1, e.vel - k * 6), e.note };
        }
    }
    for (int j = 0; j < numPend;)
    {
        const auto& q = pend[(size_t) j];
        if (q.at < sampleClock + n) { push ({ (int) juce::jlimit (0LL, (long long) n - 1, q.at - sampleClock), 1, q.track, q.note, q.vel, -1 }); pend[(size_t) j] = pend[(size_t) --numPend]; }
        else ++j;
    }
    static const double kDiv[6] = { 1, .5, .25, 1.0 / 3, 1.0 / 6, .125 };
    const double bps = bpm / 60.0 / sr;
    for (int part = 0; part < 3; ++part)
    {
        auto& pf = perf[(size_t) part]; const int nh = numHeld[(size_t) part];
        const double b0 = hostPlay ? ppq : arpBeat[(size_t) part], b1 = b0 + n * bps; arpBeat[(size_t) part] = b1;
        if (pf.arpOn.load (std::memory_order_relaxed) == 0 || nh == 0) continue;
        const double step = kDiv[juce::jlimit (0, 5, pf.arpDiv.load())]; const int oct = juce::jlimit (1, 3, pf.arpOct.load()), mode = pf.arpMode.load(), total = nh * oct;
        int sorted[48]; for (int j = 0; j < nh; ++j) sorted[j] = held[(size_t) part][(size_t) j];
        if (mode != 4) std::sort (sorted, sorted + nh);
        for (double k = std::ceil (b0 / step - 1e-9) * step; k < b1; k += step)
        {
            int idx = arpIdx[(size_t) part]++, pos;
            if (mode == 1) pos = total - 1 - idx % total;
            else if (mode == 2) { const int cyc = juce::jmax (1, total * 2 - 2); const int q = idx % cyc; pos = total == 1 ? 0 : (q < total ? q : cyc - q); }
            else if (mode == 3) { arpRng = arpRng * 1664525u + 1013904223u; pos = (int) ((arpRng >> 8) % (uint32_t) total); }
            else pos = idx % total;
            const int note = juce::jlimit (0, 127, sorted[pos % nh] + 12 * (pos / nh));
            push ({ juce::jlimit (0, n - 1, (int) ((k - b0) / bps)), 3, trackOfPart (part), note, arpVel[(size_t) part], step * pf.gate.load() * 60.0 / bpm });
        }
    }
}
