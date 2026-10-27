// Kararlılık ve performans testi: eklenti işlemcisini bir DAW gibi çalıştırır.
//  - 18 stil x 3 kalite: NaN, tepe seviyesi, gerçek zamana göre CPU oranı
//  - rastgele parametre otomasyonu, değişken blok boyutu ve örnekleme hızları
//  - durum kaydet/yükle, MIDI giriş, editörün 6 dilde ekran görüntüsü
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/SoundParams.h"
#include <iostream>

struct FakeHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 128; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (playing); p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};

static void setP (HitGenProcessor& p, const char* id, float plain) { if (auto* pr = p.apvts.getParameter (id)) pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); }

struct Stats { double peak = 0, rms = 0, cpu = 0; bool nan = false; int maxVoices = 0; long long samples = 0; };

static Stats run (HitGenProcessor& p, FakeHead& head, double sr, int block, double seconds, bool randomBlocks, juce::Random* fuzz)
{
    Stats st; juce::AudioBuffer<float> buf (2, 4096); juce::MidiBuffer midi;
    long long total = (long long) (seconds * sr); double sum = 0;
    const auto t0 = juce::Time::getHighResolutionTicks();
    while (st.samples < total)
    {
        int n = randomBlocks ? 1 + juce::Random::getSystemRandom().nextInt (2048) : block;
        n = (int) juce::jmin ((long long) n, total - st.samples);
        midi.clear();
        if (fuzz != nullptr && fuzz->nextInt (40) == 0)
        {
            const char* ids[] = { "orig", "energy", "thick", "pump", "cut_chords", "res_bass", "atk_melody", "rel_chords", "drv_bass", "rev_melody", "dly_chords", "vol_drums", "master", "quality", "swing", "human" };
            auto* pr = fuzz->nextBool() ? p.apvts.getParameter (ids[fuzz->nextInt (16)]) : p.apvts.getParameter (hg::spId (fuzz->nextInt (3), fuzz->nextInt (hg::kNumSp)));   // ses tasarımı otomasyonu da
            if (pr) pr->setValueNotifyingHost (fuzz->nextFloat());
        }
        if (fuzz != nullptr && fuzz->nextInt (25) == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + fuzz->nextInt (40), (juce::uint8) (20 + fuzz->nextInt (100))), fuzz->nextInt (n));
        if (fuzz != nullptr && fuzz->nextInt (25) == 0) midi.addEvent (juce::MidiMessage::noteOff (1, 40 + fuzz->nextInt (40)), fuzz->nextInt (n));
        buf.setSize (2, n, false, false, true);
        p.processBlock (buf, midi);
        if (fuzz != nullptr && fuzz->nextInt (60) == 0) p.idle();   // zamanlayıcı: canlı yama yenileme
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = buf.getSample (c, i);
                if (! std::isfinite (x)) st.nan = true; else { st.peak = juce::jmax (st.peak, (double) std::abs (x)); sum += x * x; }
            }
        st.maxVoices = juce::jmax (st.maxVoices, p.engine.activeVoices());
        head.ppq += n * head.bpm / 60.0 / sr; st.samples += n;
    }
    const double el = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
    st.cpu = el / seconds; st.rms = std::sqrt (sum / (2.0 * st.samples));
    return st;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    int failures = 0;
    const bool quick = juce::SystemStats::getEnvironmentVariable ("HITGEN_QUICK", {}).isNotEmpty();   // valgrind/sanitizer için kısa tur
    auto fail = [&] (const juce::String& m) { std::cout << "FAIL " << m << "\n"; ++failures; };

    // 1) stiller x kalite
    {
        HitGenProcessor p; FakeHead head; p.setPlayHead (&head);
        p.setPlayConfigDetails (0, 2, 48000, 256); p.prepareToPlay (48000, 256);
        const char* qn[3] = { "eco", "bal", "high" };
        double worst[3] = { 0, 0, 0 };
        for (int si = 0; si < p.data.numStyles(); si += quick ? 6 : 1)
        {
            setP (p, "style", (float) si); p.applyStyleDefaults(); p.regenerateNow();
            head.bpm = (double) p.data.style (si)["bpmDef"];
            for (int q = 0; q < 3; ++q)
            {
                setP (p, "quality", (float) q);
                head.ppq = 0; auto s = run (p, head, 48000, 256, (quick ? 2 : 8) * 4 * 60.0 / head.bpm, false, nullptr);
                worst[q] = juce::jmax (worst[q], s.cpu);
                std::cout << p.data.styleId (si) << " " << qn[q] << " peak " << juce::String (s.peak, 3) << " rmsdB " << juce::String (juce::Decibels::gainToDecibels (s.rms), 1)
                          << " cpu " << juce::String (s.cpu * 100, 2) << "% voices " << s.maxVoices << (s.nan ? " NAN" : "") << "\n";
                if (s.nan) fail ("NaN in " + p.data.styleId (si));
                if (s.peak > 1.0) fail ("clip in " + p.data.styleId (si));
                if (s.rms < 1e-4) fail ("silent " + p.data.styleId (si));
            }
        }
        std::cout << "worst CPU (single core, % of realtime): eco " << worst[0] * 100 << " bal " << worst[1] * 100 << " high " << worst[2] * 100 << "\n";
    }
    // 2) rastgele otomasyon, blok boyutu, örnekleme hızları, MIDI giriş, durdur/başlat
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        if (! quick || sr == 48000.0)
    {
        HitGenProcessor p; FakeHead head; p.setPlayHead (&head);
        p.setPlayConfigDetails (0, 2, sr, 512); p.prepareToPlay (sr, 512);
        juce::Random fz (1234 + (int) sr);
        for (int round = 0; round < (quick ? 3 : 6); ++round)
        {
            setP (p, "style", (float) fz.nextInt (p.data.numStyles())); p.applyStyleDefaults(); p.regenerateNow();
            head.playing = round % 3 != 2; p.previewPlaying.store (round % 3 == 2);
            auto s = run (p, head, sr, 512, quick ? 1.5 : 6.0, true, &fz);
            if (s.nan) fail ("NaN fuzz sr " + juce::String (sr)); if (s.peak > 1.0) fail ("clip fuzz");
            std::cout << "fuzz sr " << sr << " round " << round << " peak " << juce::String (s.peak, 3) << " cpu " << juce::String (s.cpu * 100, 2) << "%\n";
            p.newIdea(); p.regenerateNow();
        }
    }
    // 3) durum kaydet / yükle
    {
        HitGenProcessor a; setP (a, "style", 3); a.applyStyleDefaults(); setP (a, "seed", 4242); setP (a, "thick", 90); a.setSound (0, "an-cine-pad", true); a.lang = "ar";
        juce::MemoryBlock mb; a.getStateInformation (mb);
        HitGenProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        if ((int) b.apvts.getRawParameterValue ("style")->load() != 3) fail ("state style");
        if ((int) b.apvts.getRawParameterValue ("seed")->load() != 4242) fail ("state seed");
        if (b.soundId (0) != "an-cine-pad") fail ("state sound");
        if (b.lang != "ar") fail ("state lang");
        b.regenerateNow(); a.regenerateNow();
        if (a.midiBytes (4) != b.midiBytes (4)) fail ("state midi mismatch");
        HitGenProcessor c; c.setStateInformation ("garbage", 7);   // bozuk veri çökme yapmamalı
        std::cout << "state roundtrip ok\n";
    }
    // 4) MIDI dosyaları
    {
        HitGenProcessor p; p.regenerateNow();
        for (int w = 0; w < 6; ++w) { auto f = p.writeMidi (w); if (! f.existsAsFile() || f.getSize() < 30) fail ("midi file " + juce::String (w)); }
        std::cout << "midi files ok: " << p.writeMidi (4).getFullPathName() << "\n";
    }
    // 4b) ses tasarımı: her ses parametrelere açılıp geri kurulunca aynı yamayı vermeli
    {
        HitGenProcessor p; int bad = 0;
        for (int i = 0; i < p.data.numSounds(); ++i)
        {
            const auto& v = p.data.sound (i); float x[hg::kNumSp]; hg::spFromSound (v, x);
            auto a = hg::Patch::fromVar (v, p.data, 48000), b = hg::Patch::fromVar (hg::spToSound (v, x), p.data, 48000);
            bool same = a->numOsc == b->numOsc && a->uniN == b->uniN && a->ftype == b->ftype && a->slope24 == b->slope24 && a->hasFm == b->hasFm && a->vowel == b->vowel;
            auto near = [] (float u, float w) { return std::abs (u - w) <= 1e-3f * juce::jmax (1.0f, std::abs (u)); };
            for (int j = 0; same && j < a->numOsc; ++j)
            {
                const auto& o1 = a->osc[(size_t) j]; const auto& o2 = b->osc[(size_t) j];
                const double f1 = o1.octave * 1200.0 + o1.detune, f2 = o2.octave * 1200.0 + o2.detune;
                same = o1.wave == o2.wave && std::abs (f1 - f2) < .01 && near (o1.gain, o2.gain);
            }
            for (auto [u, w] : { std::pair<float, float> { a->cut, b->cut }, { a->q, b->q }, { a->envAmt, b->envAmt }, { a->envDec, b->envDec }, { a->A, b->A }, { a->D, b->D }, { a->S, b->S }, { a->R, b->R },
                                 { a->uniSpread, b->uniSpread }, { a->rv, b->rv }, { a->dl, b->dl }, { a->ch, b->ch }, { a->dr, b->dr }, { a->fd, b->fd }, { a->vol, b->vol }, { a->kt, b->kt }, { a->sub, b->sub }, { a->noi, b->noi },
                                 { a->lfo[0].rate, b->lfo[0].rate }, { a->lfo[0].depth, b->lfo[0].depth }, { a->lfo[1].depth, b->lfo[1].depth }, { a->lfo[2].depth, b->lfo[2].depth } })
                if (! near (u, w)) same = false;
            if (a->usesTable()) for (int k = 0; same && k < 2048; k += 97) same = near (a->table.lv[0][(size_t) k], b->table.lv[0][(size_t) k]);
            if (! same) { ++bad; std::cout << "sound roundtrip differs: " << v["id"].toString() << "\n"; }
        }
        if (bad) fail ("sound param roundtrip " + juce::String (bad));
        std::cout << "sound design roundtrip ok for " << p.data.numSounds() << " sounds\n";
    }
    // 4c) rastgele ses + canlı düğme çevirme, tutulan notalarla (ses iş parçacığında yama göçü)
    {
        HitGenProcessor p; FakeHead head; head.playing = false; p.setPlayHead (&head);
        p.setPlayConfigDetails (0, 2, 48000, 256); p.prepareToPlay (48000, 256);
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi; double peak = 0; bool nan = false;
        juce::Random r (99);
        for (int round = 0; round < 40; ++round)
        {
            const int part = round % 3; p.auditionPart.store (part);
            p.randomizeSound (part);
            for (int n : { 48, 55, 60, 64 }) p.keyboardState.noteOn (1, n, .8f);
            for (int b = 0; b < 60; ++b)
            {
                if (b % 3 == 0) if (auto* pr = p.apvts.getParameter (hg::spId (part, r.nextInt (hg::kNumSp)))) pr->setValueNotifyingHost (r.nextFloat());
                p.idle();
                midi.clear(); p.processBlock (buf, midi);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const float x = buf.getSample (c, i); if (! std::isfinite (x)) nan = true; else peak = juce::jmax (peak, (double) std::abs (x)); }
            }
            p.keyboardState.allNotesOff (1);
            p.resetSoundDesign (part);
        }
        if (nan) fail ("NaN in live sound design"); if (peak > 1.0) fail ("clip in live sound design");
        std::cout << "live sound design ok, peak " << juce::String (peak, 3) << "\n";
    }
    // 4d) sanatçı presetleri: kaydet, değiştir, yükle; eski proje (ses tasarımı olmadan) uyumu
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-preset-test"); dir.deleteRecursively(); dir.createDirectory();
        HitGenProcessor a; a.presets.overrideDir = dir; a.presets.rescan();
        setP (a, "style", (float) a.data.styleIndex ("blackcoffee")); a.applyStyleDefaults(); setP (a, "seed", 777);
        a.setSound (1, "bc-afro-sub", true); setP (a, "sd_b_cut", 900); setP (a, "sd_m_o1w", 5);
        juce::String path;
        if (! a.savePreset ("DJ Test", "Gece 1", path)) fail ("preset save");
        if (a.presets.items().size() != 1) fail ("preset list");
        a.regenerateNow(); const auto before = a.midiBytes (4);
        setP (a, "style", 0); a.applyStyleDefaults(); setP (a, "seed", 5); setP (a, "sd_b_cut", 300);
        if (! a.loadPreset (juce::File (path))) fail ("preset load");
        a.regenerateNow();
        if (a.midiBytes (4) != before) fail ("preset midi mismatch");
        if (std::abs (a.apvts.getRawParameterValue ("sd_b_cut")->load() - 900) > 1) fail ("preset sound design");
        if (a.soundId (1) != "bc-afro-sub" || a.userArtist != "DJ Test") fail ("preset sound/artist");
        if (! a.baseName().contains ("dj-test")) fail ("artist in file name: " + a.baseName());
        if (a.apvts.getRawParameterValue ("sevenths")->load() < .5f) fail ("style sevenths default");
        // bozuk preset dosyası
        auto junk = dir.getChildFile ("bozuk.hgpreset"); junk.replaceWithText ("<HitGenPreset artist='x'><nope/></HitGenPreset>");
        if (a.loadPreset (junk)) fail ("junk preset accepted");
        // eski durum: ses tasarımı parametreleri yok -> seçili sesin değerleri
        HitGenProcessor b; auto tree = b.stateTree(); auto extra = tree.getChildWithName ("extra"); extra.removeProperty ("sdv", nullptr);
        for (int i = 0; i < tree.getNumChildren(); ++i) { auto c = tree.getChild (i); if (c.getProperty ("id").toString() == "sd_c_cut") c.setProperty ("value", 31.0f, nullptr); }
        juce::MemoryBlock mb; if (auto x = tree.createXml()) juce::AudioProcessor::copyXmlToBinary (*x, mb);
        HitGenProcessor c; c.setStateInformation (mb.getData(), (int) mb.getSize());
        float vals[hg::kNumSp]; hg::spFromSound (c.data.soundById (c.soundId (0)), vals);
        if (std::abs (c.apvts.getRawParameterValue ("sd_c_cut")->load() - vals[hg::cut]) > 1) fail ("old state sound design");
        std::cout << "presets ok: " << path << "\n";
    }
    // 5) editör: 6 dilde ekran görüntüsü
    {
        HitGenProcessor p; p.regenerateNow();
        std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
        for (auto l : p.data.langCodes)
        {
            ed->setLanguage (l);
            auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
            auto f = outDir.getChildFile ("plugin-" + l + ".png"); f.deleteFile();
            juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
        }
        // ses tasarımı ve preset sekmeleri
        for (auto l : p.data.langCodes)
            for (int tb : { 1, 2 })
            {
                ed->setLanguage (l); ed->setTab (tb);
                if (tb == 1) ed->sounds().setPart (l == "tr" ? 0 : l == "ar" ? 2 : 1);
                auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
                auto f = outDir.getChildFile ("plugin-" + l + (tb == 1 ? "-sound" : "-presets") + ".png"); f.deleteFile();
                juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
            }
        ed->setTab (0);
        p.highContrast = true; ed->setLanguage ("tr");
        // yüksek kontrast görüntüsü
        ed.reset(); auto* e2 = dynamic_cast<HitGenEditor*> (p.createEditor());
        auto img = e2->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
        auto f = outDir.getChildFile ("plugin-contrast.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
        delete e2;
        std::cout << "editor snapshots written\n";
    }
    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
