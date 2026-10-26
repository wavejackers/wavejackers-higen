#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const char* const kGenParams[] = { "style", "variation", "seed", "key", "scale", "bars", "orig", "energy", "human", "swing", "thick", "sevenths" };
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
    return L;
}

HitGenProcessor::HitGenProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      data (hg::Data::get()),
      apvts (*this, nullptr, "HitGen", createLayout (data))
{
    for (auto* id : kGenParams) apvts.addParameterListener (id, this);
    for (int t = 0; t < 4; ++t)
    {
        const juce::String n (kPartIds[t]); auto& q = pp[(size_t) t];
        q.vol = apvts.getRawParameterValue ("vol_" + n); q.mute = apvts.getRawParameterValue ("mute_" + n);
        if (t == 2) continue;
        q.cut = apvts.getRawParameterValue ("cut_" + n); q.res = apvts.getRawParameterValue ("res_" + n); q.atk = apvts.getRawParameterValue ("atk_" + n);
        q.rel = apvts.getRawParameterValue ("rel_" + n); q.drv = apvts.getRawParameterValue ("drv_" + n); q.rev = apvts.getRawParameterValue ("rev_" + n); q.dly = apvts.getRawParameterValue ("dly_" + n);
    }
    pQuality = p ("quality"); pThick = p ("thick"); pPump = p ("pump"); pMaster = p ("master"); pBpm = p ("bpm"); pPattern = p ("pattern"); pMidiOut = p ("midiOut"); pKeysTo = p ("keysTo");
    auto ul = juce::SystemStats::getUserLanguage().substring (0, 2).toLowerCase();
    lang = data.langCodes.contains (ul) ? ul : "en";
    const auto& S = data.style (0);
    for (int t = 0; t < 4; ++t) sounds[(size_t) t] = S["sound"][kPartIds[t]].toString();
    lastStyle = 0;
    engine.prepare (44100, 512);
    for (int t = 0; t < 4; ++t) loadPatch (t);
    applyStyleDefaults();
    regenerateNow();
    startTimerHz (30);
}

HitGenProcessor::~HitGenProcessor()
{
    stopTimer();
    for (auto* id : kGenParams) apvts.removeParameterListener (id, this);
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
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void HitGenProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate; blockSize = samplesPerBlock;
    engine.prepare (sampleRate, juce::jmax (512, samplesPerBlock));
    for (int t = 0; t < 4; ++t) loadPatch (t);   // wavetable seviyeleri örnekleme hızına bağlı
    numOffs = 0; wasRunning = false;
}

void HitGenProcessor::loadPatch (int track)
{
    if (track == hg::kDrums)
    {
        auto ki = data.kitIndex (sounds[2]); engine.setKit (data.kit (ki >= 0 ? ki : 0));
        return;
    }
    const auto& v = data.soundById (sounds[(size_t) track]);
    if (v.isVoid()) return;
    engine.setPatch (track == hg::kMelody ? 2 : track, hg::Patch::fromVar (v, data, sr));
}

void HitGenProcessor::setSound (int track, const juce::String& id, bool isManual)
{
    sounds[(size_t) track] = id; if (isManual) manual[(size_t) track] = true;
    loadPatch (track);
}

void HitGenProcessor::applyStyleDefaults()
{
    const int si = (int) p ("style")->load();
    const auto& S = data.style (juce::jlimit (0, data.numStyles() - 1, si));
    for (int t = 0; t < 4; ++t) if (! manual[(size_t) t]) { sounds[(size_t) t] = S["sound"][kPartIds[t]].toString(); loadPatch (t); }
    auto setP = [this] (const char* id, float plain) { if (auto* pr = apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); };
    setP ("key", (float) (int) S["key"]); setP ("scale", 0); setP ("swing", (float) (double) S["swing"]); setP ("bpm", (float) (int) S["bpmDef"]); setP ("variation", 0);
    for (int t : { 0, 1, 3 })
    {
        auto pm = S["pump"][kPartIds[t]];
        engine.setPump (t, (float) (double) pm[0], (float) (double) pm[1]);
    }
}

void HitGenProcessor::parameterChanged (const juce::String& id, float)
{
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
    const double hb = hostBpm.load(); g.bpm = hb > 0 ? (int) std::lround (hb) : (int) p ("bpm")->load();
    return g;
}

void HitGenProcessor::regenerateNow()
{
    auto r = std::make_shared<hg::Result> (hg::generate (data, currentParams()));
    if (r->tracks[hg::kDrums].empty()) return;   // savunma: boş sonuç gelirse eskisini koru
    pushPattern (*r);
    current = r;
    if (onResultChanged) onResultChanged();
}

void HitGenProcessor::newIdea()
{
    if (auto* pr = apvts.getParameter ("seed")) pr->setValueNotifyingHost (pr->convertTo0to1 ((float) juce::Random::getSystemRandom().nextInt ({ 1, 99999 })));
}

void HitGenProcessor::pushPattern (const hg::Result& r)
{
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

void HitGenProcessor::timerCallback()
{
    // ses iş parçacığının bıraktığı eski desenleri ve yamaları sil
    int s1, n1, s2, n2; patTrash.prepareToRead (patTrash.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) delete patTrashSlots[(size_t) (s1 + i)];
    for (int i = 0; i < n2; ++i) delete patTrashSlots[(size_t) (s2 + i)];
    patTrash.finishedRead (n1 + n2);
    engine.collectGarbage();
    if (styleChanged.exchange (false))
    {
        const int si = (int) p ("style")->load();
        if (si != lastStyle) { lastStyle = si; manual = {}; applyStyleDefaults(); }
    }
    if (regenPending.exchange (false)) regenerateNow();
}

void HitGenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
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
        m.drive = q.drv->load(); m.reverb = q.rev->load(); m.delay = q.dly->load();
    }
    const int q = (int) pQuality->load();
    engine.quality = q == 0 ? hg::Quality { 2, 32, false } : q == 2 ? hg::Quality { 7, 8, true } : hg::Quality { 4, 16, true };
    engine.thick = pThick->load() / 100.0f; engine.pumpAmt = pPump->load() / 100.0f; engine.master = pMaster->load();

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
    const int keysSel = (int) pKeysTo->load(), keysTrack = keysSel == 0 ? hg::kChords : keysSel == 1 ? hg::kBass : hg::kMelody;

    numBlockEv = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage(); if (numBlockEv >= (int) blockEv.size()) break;
        if (msg.isNoteOn()) blockEv[(size_t) numBlockEv++] = { meta.samplePosition, 1, keysTrack, msg.getNoteNumber(), msg.getVelocity(), -1 };
        else if (msg.isNoteOff()) blockEv[(size_t) numBlockEv++] = { meta.samplePosition, 2, keysTrack, msg.getNoteNumber(), 0, 0 };
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
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
        else if (e.kind == 1) engine.noteOn (e.track, e.note, e.vel, -1, 0);
        else engine.noteOff (e.track, e.note, 0);
    }
    if (pos < n) engine.render (buffer, pos, n - pos);
    for (int i = 0; i < numOffs;)
    {
        if (offs[(size_t) i].at < sampleClock + n)
        {
            midi.addEvent (juce::MidiMessage::noteOff (offs[(size_t) i].ch, offs[(size_t) i].note), (int) juce::jmax (0LL, offs[(size_t) i].at - sampleClock));
            offs[(size_t) i] = offs[(size_t) --numOffs];
        }
        else ++i;
    }
    sampleClock += n; wasRunning = run;
}

juce::String HitGenProcessor::baseName() const
{
    auto r = current; if (! r) return "HitGen";
    auto artist = data.styleById (r->P.style)["artist"].toString().toLowerCase();
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
    if (which == 5) return hg::midiFile (data, hg::assemble (data, res), all, false);
    if (which == 4) return hg::midiFile (data, res, all, false);
    return hg::midiFile (data, res, { which }, false);
}

juce::File HitGenProcessor::writeMidi (int which)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("WavejackersHitGen");
    dir.createDirectory();
    const juce::String suffix = which == 5 ? "_tam-parca" : which == 4 ? "_tum-izler" : juce::String ("_") + kPartIds[which];
    auto f = dir.getChildFile (baseName() + suffix + ".mid");
    auto bytes = midiBytes (which);
    if (bytes.getSize() > 0) f.replaceWithData (bytes.getData(), bytes.getSize());
    return f;
}

void HitGenProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    juce::ValueTree extra ("extra");
    for (int t = 0; t < 4; ++t) { extra.setProperty (juce::String ("sound_") + kPartIds[t], sounds[(size_t) t], nullptr); extra.setProperty (juce::String ("manual_") + kPartIds[t], manual[(size_t) t], nullptr); }
    extra.setProperty ("lang", lang, nullptr); extra.setProperty ("uiScale", uiScale, nullptr); extra.setProperty ("contrast", highContrast, nullptr);
    state.removeChild (state.getChildWithName ("extra"), nullptr);
    state.appendChild (extra, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void HitGenProcessor::setStateInformation (const void* d, int size)
{
    auto xml = getXmlFromBinary (d, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    auto extra = tree.getChildWithName ("extra");
    apvts.replaceState (tree);
    styleChanged.store (false); lastStyle = (int) p ("style")->load();
    if (extra.isValid())
    {
        for (int t = 0; t < 4; ++t)
        {
            auto id = extra.getProperty (juce::String ("sound_") + kPartIds[t]).toString();
            const bool ok = t == 2 ? data.kitIndex (id) >= 0 : data.soundIndex (id) >= 0;   // bilinmeyen ses: stil varsayılanında kal
            if (ok) sounds[(size_t) t] = id;
            manual[(size_t) t] = (bool) extra.getProperty (juce::String ("manual_") + kPartIds[t]);
        }
        auto l = extra.getProperty ("lang").toString(); if (data.langCodes.contains (l)) lang = l;
        uiScale = juce::jlimit (.75f, 2.0f, (float) (double) extra.getProperty ("uiScale", 1.0));
        highContrast = (bool) extra.getProperty ("contrast", false);
    }
    const auto& S = data.style (juce::jlimit (0, data.numStyles() - 1, lastStyle));
    for (int t : { 0, 1, 3 }) { auto pm = S["pump"][kPartIds[t]]; engine.setPump (t, (float) (double) pm[0], (float) (double) pm[1]); }
    for (int t = 0; t < 4; ++t) loadPatch (t);
    regenPending.store (true);
}

juce::AudioProcessorEditor* HitGenProcessor::createEditor() { return new HitGenEditor (*this); }

#if ! HITGEN_TEST_BUILD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HitGenProcessor(); }
#endif
