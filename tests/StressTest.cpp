// Kararlılık ve performans testi: eklenti işlemcisini bir DAW gibi çalıştırır.
//  - 18 stil x 3 kalite: NaN, tepe seviyesi, gerçek zamana göre CPU oranı
//  - rastgele parametre otomasyonu, değişken blok boyutu ve örnekleme hızları
//  - durum kaydet/yükle, MIDI giriş, editörün 6 dilde ekran görüntüsü
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
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
            auto* pr = p.apvts.getParameter (ids[fuzz->nextInt (16)]); if (pr) pr->setValueNotifyingHost (fuzz->nextFloat());
        }
        if (fuzz != nullptr && fuzz->nextInt (25) == 0) midi.addEvent (juce::MidiMessage::noteOn (1, 40 + fuzz->nextInt (40), (juce::uint8) (20 + fuzz->nextInt (100))), fuzz->nextInt (n));
        if (fuzz != nullptr && fuzz->nextInt (25) == 0) midi.addEvent (juce::MidiMessage::noteOff (1, 40 + fuzz->nextInt (40)), fuzz->nextInt (n));
        buf.setSize (2, n, false, false, true);
        p.processBlock (buf, midi);
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
