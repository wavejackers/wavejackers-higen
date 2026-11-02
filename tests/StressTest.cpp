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
    // 4d) içe aktarma: sentetik Serum 2 + FXP, bozuk veri çökmemeli
    {
        const juce::File fx (HITGEN_FIXTURES);
        auto s2 = hg::importSoundFile (fx.getChildFile ("synthetic.SerumPreset"));
        if (! s2.ok || ! s2.exact) fail ("serum2 import");
        else
        {
            auto& x = s2.x;
            if ((int) x[hg::uni] != 5 || x[hg::sub] < .39f || x[hg::sub] > .41f || x[hg::fxReverb] <= 0 || x[hg::fxDrive] <= 0) fail ("serum2 fields");
            if (std::abs (x[hg::cut] - 631.0f) > 8 || (int) x[hg::fslope] != 1 || (int) x[hg::ftype] != 0) fail ("serum2 filter");
            if (std::abs (x[hg::envA] - .4f) > .001f || std::abs (x[hg::envR] - .9f) > .001f) fail ("serum2 env");
            if ((int) x[hg::o1w] != 1 || (int) x[hg::o2w] != 5 || (int) x[hg::o1oct] != -1) fail ("serum2 osc");
            if (s2.name != "Synthetic Test Pad") fail ("serum2 name");
        }
        auto f1 = hg::importSoundFile (fx.getChildFile ("synthetic.fxp"));
        if (! f1.ok || f1.exact || f1.name != "Synthetic Bass" || f1.part != 1 || f1.fallbackId != "ws-fat-roll") fail ("fxp import " + f1.name + " " + f1.fallbackId);
        // fuzz: kesik ve bozulmuş baytlar
        juce::MemoryBlock src; fx.getChildFile ("synthetic.SerumPreset").loadFileAsData (src);
        juce::Random r (5); int accepted = 0;
        for (int i = 0; i < 3000; ++i)
        {
            juce::MemoryBlock m (src);
            if (i % 3 == 0) m.setSize (r.nextInt ((int) m.getSize()));
            else for (int k = 0; k < 1 + r.nextInt (6); ++k) m[(size_t) r.nextInt ((int) m.getSize())] = (char) r.nextInt (256);
            auto res = hg::importSoundBytes (m.getData(), m.getSize(), "x.SerumPreset");
            if (res.ok) { ++accepted; for (float v : res.x) if (! std::isfinite (v)) fail ("import nan"); }
        }
        for (int i = 0; i < 500; ++i) { juce::MemoryBlock m ((size_t) r.nextInt (300)); for (size_t k = 0; k < m.getSize(); ++k) m[k] = (char) r.nextInt (256); hg::importSoundBytes (m.getData(), m.getSize(), "z.fxp"); }
        // işlemciye uygula
        HitGenProcessor p; p.regenerateNow();
        if (p.importSynthFile (fx.getChildFile ("synthetic.SerumPreset"), -1, nullptr) != "impOk") fail ("apply serum2");
        p.idle();
        if (std::abs (p.apvts.getRawParameterValue ("sd_c_uni")->load() - 5.0f) > .01f) fail ("applied unison");
        if (p.importSynthFile (fx.getChildFile ("synthetic.fxp"), -1, nullptr) != "impApprox" || p.soundId (hg::kBass) != "ws-fat-roll") fail ("apply fxp");
        std::cout << "import ok (fuzz accepted " << accepted << "/3000)\n";
    }
    // 4e) "başka öner": yalnızca o iz değişir
    {
        HitGenProcessor p; p.regenerateNow(); auto base = p.result();
        for (int t = 0; t < 4; ++t)
        {
            p.suggestAlternative (t); p.idle(); auto r = p.result();
            for (int k = 0; k < 4; ++k)
            {
                bool same = r->tracks[(size_t) k].size() == base->tracks[(size_t) k].size();
                for (size_t i = 0; same && i < r->tracks[(size_t) k].size(); ++i) same = r->tracks[(size_t) k][i].n == base->tracks[(size_t) k][i].n && r->tracks[(size_t) k][i].s == base->tracks[(size_t) k][i].s;
                if (k == t && same) fail ("alt did not change track " + juce::String (t));
                if (k != t && ! same && ! (t == 1 && k == 3)) fail ("alt changed other track " + juce::String (k) + " via " + juce::String (t));
            }
            for (auto* an : { "altC", "altB", "altD", "altM" }) if (auto* pr = p.apvts.getParameter (an)) pr->setValueNotifyingHost (0);
            p.idle();
        }
        std::cout << "alternatives ok\n";
    }
    // 4f) güvenli seviye: en yüksek ayarlarda bile tepe -1 dBFS (0.891) altında
    {
        HitGenProcessor p; FakeHead head; head.playing = true; p.setPlayHead (&head);
        p.setPlayConfigDetails (0, 2, 48000, 256); p.prepareToPlay (48000, 256);
        for (auto* id : { "master", "vol_chords", "vol_bass", "vol_drums", "vol_melody" }) if (auto* pr = p.apvts.getParameter (id)) pr->setValueNotifyingHost (1.0f);
        if (auto* pr = p.apvts.getParameter ("energy")) pr->setValueNotifyingHost (1.0f);
        p.regenerateNow();
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi; float pk = 0;
        for (int blk = 0; blk < 48000 * 6 / 256; ++blk)
        {
            head.ppq = blk * 256.0 / 48000.0 * 2.1; buf.clear(); midi.clear(); p.processBlock (buf, midi);
            pk = std::max ({ pk, buf.getMagnitude (0, 0, 256), buf.getMagnitude (1, 0, 256) });
        }
        std::cout << "max-setting peak " << pk << " (" << 20 * std::log10 (pk + 1e-9) << " dBFS)\n";
        if (pk > .8914f) fail ("peak above -1 dBFS: " + juce::String (pk));
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
    // 6) Referans sekmesi: yükle -> SMART DETECT -> aday seç -> BENZERİNİ ÜRET -> orijinali çal (gerçek arka plan iş parçacıklarıyla)
    {
        const double sr = 44100.0; const int n = (int) (sr * 24); juce::AudioBuffer<float> sb (2, n); sb.clear();
        const double beat = 60.0 / 126.0; const double bar = beat * 4;
        for (int i = 0; i < n; ++i)
        {
            const double t = i / sr; float v = 0;
            const double qt = std::fmod (t, beat) / beat;                                              // bas: A1 vuruşta
            v += 0.5f * (float) (std::sin (2 * M_PI * 55.0 * t) * std::exp (-qt * 3.0));
            if (t >= bar * 4) { const double e = std::fmod (t - bar * 4, beat / 2); const double f = 440.0 * (std::fmod (std::floor ((t - bar * 4) / (beat / 2)), 2.0) < 1 ? 1.0 : 1.2599);
                                float sq = 0; for (int h = 1; h <= 9; h += 2) sq += (float) (std::sin (2 * M_PI * f * h * t) / h); v += 0.2f * sq * (float) std::exp (-e * 9.0); }
            sb.setSample (0, i, v); sb.setSample (1, i, v);
        }
        auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-ui-ref.wav"); wav.deleteFile();
        { juce::WavAudioFormat f; std::unique_ptr<juce::FileOutputStream> os (wav.createOutputStream()); std::unique_ptr<juce::AudioFormatWriter> w (f.createWriterFor (os.get(), sr, 2, 24, {}, 0)); if (w) { os.release(); w->writeFromAudioSampleBuffer (sb, 0, n); } }
        auto pump = [&] (const std::function<bool()>& done, int ms) { const auto t0 = juce::Time::getMillisecondCounter(); while (! done() && juce::Time::getMillisecondCounter() - t0 < (juce::uint32) ms) juce::MessageManager::getInstance()->runDispatchLoopUntil (20); return done(); };
        HitGenProcessor p; p.regenerateNow(); p.setRateAndBufferSizeDetails (48000.0, 256); p.prepareToPlay (48000.0, 256);
        std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
        ed->filesDropped ({ wav.getFullPathName() }, 0, 0);
        if (ed->currentTab() != 3) fail ("drop did not open the reference tab");
        if (! pump ([&] { return p.ref.state.load() == hg::RefEngine::Loaded; }, 15000)) fail ("reference did not load");
        p.ref.analyse();
        if (! pump ([&] { return p.ref.state.load() == hg::RefEngine::Ready; }, 60000)) fail ("analysis did not finish");
        auto an = p.ref.analysis();
        if (! an || ! an->ok || an->regions.empty()) fail ("no regions");
        else
        {
            std::cout << "ui ref: bpm " << an->bpm << " key " << an->key << (an->minor ? "m" : "M") << " regions " << an->regions.size() << "\n";
            if (std::abs (an->bpm - 126.0) > 2.0) fail ("ui ref bpm " + juce::String (an->bpm));
            int cat = -1; for (int c : { hg::catLead, hg::catBass }) if (! an->top (c, 1).empty()) { cat = c; break; }
            ed->reference().selectCandidate (cat, 0);
            float before[hg::kNumSp], after[hg::kNumSp]; const int part = ed->reference().currentPart(); p.readSoundParams (part, before);
            bool gotDna = false; ed->reference().generateNow();
            pump ([&] { p.readSoundParams (part, after); return std::memcmp (before, after, sizeof before) != 0; }, 30000);
            p.readSoundParams (part, after); gotDna = std::memcmp (before, after, sizeof before) != 0;
            if (! gotDna) fail ("generate similar changed nothing: " + ed->reference().statusText());
            else std::cout << "generate similar ok: " << ed->reference().statusText() << "\n";
        }
        // orijinali çal: çıkışta ses var ve tavan aşılmıyor; durdurunca sessiz
        p.ref.regionStart = 2.0; p.ref.regionEnd = 6.0; p.ref.seekTo = 2.0; p.ref.loop = true; p.ref.playing = true;
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi; float pk = 0;
        for (int b = 0; b < 400; ++b) { buf.clear(); midi.clear(); p.processBlock (buf, midi); pk = std::max (pk, buf.getMagnitude (0, 256)); }
        if (pk < 0.01f || pk > 0.8914f) fail ("original playback peak " + juce::String (pk));
        p.ref.playing = false; buf.clear(); midi.clear(); p.processBlock (buf, midi); p.processBlock (buf, midi);
        if (buf.getMagnitude (0, 256) > 1e-4f) fail ("playback did not stop");
        std::cout << "original playback ok, peak " << pk << "\n";
        for (auto l : p.data.langCodes)
        {
            ed->setLanguage (l); ed->setTab (3);
            auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
            auto f = outDir.getChildFile ("plugin-" + l + "-ref.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
        }
        // geçersiz dosya: hata iletisi, çökme yok
        auto bad = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-bad.wav"); bad.replaceWithText ("not audio at all");
        ed->reference().openFile (bad); pump ([&] { return p.ref.state.load() == hg::RefEngine::Failed; }, 10000);
        if (p.ref.state.load() != hg::RefEngine::Failed) fail ("bad file not rejected");
        ed.reset();
    }
    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
