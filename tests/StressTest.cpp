// Kararlılık ve performans testi: eklenti işlemcisini bir DAW gibi çalıştırır.
//  - 18 stil x 3 kalite: NaN, tepe seviyesi, gerçek zamana göre CPU oranı
//  - rastgele parametre otomasyonu, değişken blok boyutu ve örnekleme hızları
//  - durum kaydet/yükle, MIDI giriş, editörün 6 dilde ekran görüntüsü
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/FlpWriter.h"
#include "../Source/AlsWriter.h"
#include "../Source/AutoMix.h"
#include "../Source/Demo.h"
#include "../Source/Synth.h"
#include "../Source/Data.h"
#include <juce_dsp/juce_dsp.h>
#include "../Source/Arrangement.h"
#include "../Source/CoProducer.h"
#include "../Source/ProducerPanels.h"
#include "../Source/ProjectPage.h"
#include "../Source/PluginEditor.h"
#include <BinaryData.h>
#include "../Source/SoundParams.h"
#include "../Source/DnaLibrary.h"
#include "../Source/Assist.h"
#include "../Source/DrumMachine.h"
#include "../Source/Loudness.h"
#include <iostream>
#include <functional>
#include <set>
#include <map>
#include <memory>

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

// Büyük nesneler (HitGenProcessor) tek bir main() çerçevesinde birikip MSVC'de 1 MB yığını aşmasın diye her blok ayrı işlevde ve yığın dışında.
using FailFn = std::function<void (const juce::String&)>;
static void testReference (const juce::File& outDir, const FailFn& fail)
{
    // 6) Referans sekmesi: yükle -> SMART DETECT -> aday seç -> BENZERİNİ ÜRET -> orijinali çal (gerçek arka plan iş parçacıklarıyla)
    {
        const double sr = 44100.0; const int n = (int) (sr * 24); juce::AudioBuffer<float> sb (2, n); sb.clear();
        const double beat = 60.0 / 126.0; const double bar = beat * 4;
        for (int i = 0; i < n; ++i)
        {
            const double t = i / sr; float v = 0;
            const double qt = std::fmod (t, beat) / beat;                                              // bas: A1 vuruşta
            v += 0.5f * (float) (std::sin (2 * juce::MathConstants<double>::pi * 55.0 * t) * std::exp (-qt * 3.0));
            if (t >= bar * 4) { const double e = std::fmod (t - bar * 4, beat / 2); const double f = 440.0 * (std::fmod (std::floor ((t - bar * 4) / (beat / 2)), 2.0) < 1 ? 1.0 : 1.2599);
                                float sq = 0; for (int h = 1; h <= 9; h += 2) sq += (float) (std::sin (2 * juce::MathConstants<double>::pi * f * h * t) / h); v += 0.2f * sq * (float) std::exp (-e * 9.0); }
            sb.setSample (0, i, v); sb.setSample (1, i, v);
        }
        auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-ui-ref.wav"); wav.deleteFile();
        { juce::WavAudioFormat f; std::unique_ptr<juce::FileOutputStream> os (wav.createOutputStream()); std::unique_ptr<juce::AudioFormatWriter> w (f.createWriterFor (os.get(), sr, 2, 24, {}, 0)); if (w) { os.release(); w->writeFromAudioSampleBuffer (sb, 0, n); } }
        auto pump = [&] (const std::function<bool()>& done, int ms) { const auto t0 = juce::Time::getMillisecondCounter(); while (! done() && juce::Time::getMillisecondCounter() - t0 < (juce::uint32) ms) juce::MessageManager::getInstance()->runDispatchLoopUntil (20); return done(); };
        auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.regenerateNow(); p.setRateAndBufferSizeDetails (48000.0, 256); p.prepareToPlay (48000.0, 256);
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
            ed->setLanguage (l); ed->setWorkspace (1);
            auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
            auto f = outDir.getChildFile ("plugin-" + l + "-ref.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
        }
        ed->setLanguage ("en");
        for (int w = 0; w < 4; ++w) { ed->setWorkspace (w); auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-en-ws" + juce::String (w) + ".png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); }
        for (auto l : { "ar", "tr" }) { ed->setLanguage (l); ed->setWorkspace (3); auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-" + juce::String (l) + "-ws3.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); }
        ed->setLanguage ("en");
        ed->setWorkspace (2); ed->reference().showSampler (true);
        { auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-en-sampler.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); }
        ed->reference().showSampler (false);
        ed->reference().showLibrary();
        { ed->setTab (5); auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-en-project.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); ed->setTab (0); }
        { auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-en-library.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); }
        ed->reference().showSampler (false);
        // HIZLI mod: yüklü klip + analiz var; yeniden başlat -> aday seçilir ve ses üretilir; A/B
        { float b4[hg::kNumSp], af[hg::kNumSp]; const int part = ed->reference().currentPart(); p.readSoundParams (part, b4);
          ed->reference().quickNow(); pump ([&] { p.readSoundParams (part, af); return std::memcmp (b4, af, sizeof b4) != 0; }, 30000); p.readSoundParams (part, af);
          if (std::memcmp (b4, af, sizeof b4) == 0) fail ("quick mode produced no sound: " + ed->reference().statusText());
          float c1[hg::kNumSp], c2[hg::kNumSp], c3[hg::kNumSp]; ed->reference().pinNow(); ed->reference().generateNow(); pump ([&] { return false; }, 1500); p.readSoundParams (part, c1);
          ed->reference().swapNow(); p.readSoundParams (part, c2); ed->reference().swapNow(); p.readSoundParams (part, c3);
          if (std::memcmp (c1, c2, sizeof c1) == 0 || std::memcmp (c1, c3, sizeof c1) != 0) fail ("A/B swap");
          std::cout << "quick + A/B ok\n"; }
        // 2.5: varyasyon ızgarası, geri al / yinele, ritim MIDI'si
        { const int part = ed->reference().currentPart(); ed->setWorkspace (3);
          ed->reference().openGridNow(); pump ([&] { return ed->reference().gridVisible(); }, 30000);
          if (! ed->reference().gridVisible()) fail ("variation grid did not open: " + ed->reference().statusText());
          { auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-en-grid.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); }
          float g0[hg::kNumSp], u1[hg::kNumSp], u2[hg::kNumSp]; p.readSoundParams (part, g0);
          if (! ed->reference().undo()) fail ("undo refused, depth " + juce::String (ed->reference().undoDepth()));
          p.readSoundParams (part, u1); if (std::memcmp (g0, u1, sizeof g0) == 0) fail ("undo changed nothing");
          ed->reference().redo(); p.readSoundParams (part, u2); if (std::memcmp (g0, u2, sizeof g0) != 0) fail ("redo did not restore");
          ed->reference().openGridNow(); if (ed->reference().gridVisible()) fail ("grid did not close");
          auto mf = ed->reference().rhythmFileNow(); int ons = 0;
          if (! mf.existsAsFile()) fail ("rhythm midi not written: " + ed->reference().statusText());
          else { juce::FileInputStream in (mf); juce::MidiFile m; if (m.readFrom (in) && m.getNumTracks() > 0) for (auto* ev : *m.getTrack (0)) if (ev->message.isNoteOn()) ++ons; if (ons == 0) fail ("rhythm midi empty"); }
          std::cout << "grid + undo/redo + rhythm midi ok (" << ons << " notes, undo depth " << ed->reference().undoDepth() << ")\n"; }
        // geçersiz dosya: hata iletisi, çökme yok
        auto bad = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-bad.wav"); bad.replaceWithText ("not audio at all");
        ed->reference().openFile (bad); pump ([&] { return p.ref.state.load() == hg::RefEngine::Failed; }, 10000);
        if (p.ref.state.load() != hg::RefEngine::Failed) fail ("bad file not rejected");
        ed.reset();
    }
}
static void testSampler (const FailFn& fail)
{
    // 7) Örnekleyici: MIDI -> doğru perde, polifoni, ADSR, durum geri yükleme, kayıp dosya
    {
        const double sr = 48000.0; const int n = (int) (sr * 4); juce::AudioBuffer<float> sb (2, n);
        for (int i = 0; i < n; ++i) { const float v = 0.5f * (float) std::sin (2 * juce::MathConstants<double>::pi * 220.0 * i / sr); sb.setSample (0, i, v); sb.setSample (1, i, v); }   // A3 = MIDI 57
        auto wav = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-samp.wav"); wav.deleteFile();
        { juce::WavAudioFormat f; std::unique_ptr<juce::FileOutputStream> os (wav.createOutputStream()); std::unique_ptr<juce::AudioFormatWriter> w (f.createWriterFor (os.get(), sr, 2, 24, {}, 0)); if (w) { os.release(); w->writeFromAudioSampleBuffer (sb, 0, n); } }
        auto pump = [&] (const std::function<bool()>& done, int ms) { const auto t0 = juce::Time::getMillisecondCounter(); while (! done() && juce::Time::getMillisecondCounter() - t0 < (juce::uint32) ms) juce::MessageManager::getInstance()->runDispatchLoopUntil (20); return done(); };
        auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.regenerateNow(); p.setRateAndBufferSizeDetails (48000.0, 256); p.prepareToPlay (48000.0, 256);
        p.ref.load (wav); if (! pump ([&] { return p.ref.state.load() == hg::RefEngine::Loaded; }, 15000)) fail ("sampler clip load");
        auto& S = p.ref.sampler; S.enabled = true; S.root = 57; S.a = 0.001f; S.d = 0.01f; S.s = 1.0f; S.r = 0.05f; S.gain = 1.0f; S.velSens = 0.0f; S.loop = true; S.transpose = 0;
        // dominant frekansı sıfır geçişlerinden ölç
        auto measure = [&] (int note) -> double
        {
            juce::MidiBuffer m; juce::AudioBuffer<float> b (2, 256); std::vector<float> out; m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            for (int blk = 0; blk < 120; ++blk) { b.clear(); p.processBlock (b, m); m.clear(); for (int i = 0; i < 256; ++i) out.push_back (b.getSample (0, i)); }
            juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, note), 0); b.clear(); p.processBlock (b, off);
            for (int blk = 0; blk < 40; ++blk) { juce::MidiBuffer e; b.clear(); p.processBlock (b, e); }
            int z = 0, first = -1, last = -1; for (size_t i = 8000; i + 1 < out.size(); ++i) if (out[i] <= 0 && out[i + 1] > 0) { ++z; if (first < 0) first = (int) i; last = (int) i; }
            return z > 2 ? (z - 1) * 48000.0 / (last - first) : 0.0;
        };
        const double f0 = measure (57), f1 = measure (69), f2 = measure (45);
        std::cout << "sampler pitch: root " << f0 << " Hz, +12 " << f1 << " Hz, -12 " << f2 << " Hz\n";
        if (std::abs (f0 - 220.0) > 3.0 || std::abs (f1 - 440.0) > 6.0 || std::abs (f2 - 110.0) > 2.0) fail ("sampler pitch wrong");
        // polifoni: 20 nota -> en çok 8 ses, sessizlik ve tavan korunur
        { juce::MidiBuffer m; for (int k = 0; k < 20; ++k) m.addEvent (juce::MidiMessage::noteOn (1, 40 + k, (juce::uint8) 127), k); juce::AudioBuffer<float> b (2, 256); float pk = 0;
          for (int blk = 0; blk < 60; ++blk) { b.clear(); p.processBlock (b, m); m.clear(); pk = std::max (pk, b.getMagnitude (0, 256)); if (! std::isfinite (pk)) break; }
          if (! (pk > 0.01f && pk <= 0.8914f)) fail ("sampler poly peak " + juce::String (pk));
          juce::MidiBuffer a; a.addEvent (juce::MidiMessage::allNotesOff (1), 0); b.clear(); p.processBlock (b, a); for (int blk = 0; blk < 80; ++blk) { juce::MidiBuffer e; b.clear(); p.processBlock (b, e); }
          if (b.getMagnitude (0, 256) > 1e-4f) fail ("sampler stuck notes"); }
        // durum: kaydet -> yeni işlemciye yükle (yol + ayarlar), sonra dosya silinmişken kayıp dosya
        S.root = 61; S.transpose = 3; S.a = 0.123f; juce::MemoryBlock mb; p.getStateInformation (mb);
        { auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (mb.getData(), (int) mb.getSize());
          pump ([&] { return q.ref.state.load() == hg::RefEngine::Loaded; }, 15000);
          if (q.ref.state.load() != hg::RefEngine::Loaded || q.ref.sampler.root.load() != 61 || q.ref.sampler.transpose.load() != 3 || std::abs (q.ref.sampler.a.load() - 0.123f) > 1e-4f) fail ("sampler state not restored"); }
        wav.deleteFile();
        { auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (mb.getData(), (int) mb.getSize()); pump ([&] { return q.ref.state.load() == hg::RefEngine::Failed; }, 5000);
          if (q.ref.state.load() != hg::RefEngine::Failed || q.ref.error != "fileMissing" || q.ref.missingPath.isEmpty()) fail ("missing file not detected");
          juce::MemoryBlock again; q.getStateInformation (again); auto r2_h = std::make_unique<HitGenProcessor>(); auto& r2 = *r2_h; r2.setStateInformation (again.getData(), (int) again.getSize()); pump ([&] { return r2.ref.state.load() == hg::RefEngine::Failed; }, 5000);
          if (r2.ref.missingPath.isEmpty()) fail ("missing path lost on re-save"); }
        // örnekleyici preset JSON gidiş-dönüş
        { const auto v = p.ref.samplerToVar(); const auto txt = juce::JSON::toString (v); auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.ref.samplerFromVar (juce::JSON::parse (txt));
          if (q.ref.sampler.root.load() != 61 || q.ref.sampler.transpose.load() != 3) fail ("sampler preset roundtrip"); q.ref.samplerFromVar (juce::JSON::parse ("{\"root\":999,\"a\":\"x\",\"voices\":-4}"));
          if (q.ref.sampler.root.load() != 127 || q.ref.sampler.maxVoices.load() != 1) fail ("sampler preset clamp"); }
        std::cout << "sampler tests done\n";
    }
}
// MIDI Learn: öğrenme, yakalama (soft takeover), eşlenmemiş CC, kanal modu mesajları, durum kaydı/geri yükleme, preset ayrımı, arayüz rozeti
static void testMidiLearn (const juce::File& outDir, const FailFn& fail)
{
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.setRateAndBufferSizeDetails (48000.0, 256); p.prepareToPlay (48000.0, 256);
    auto& ml = p.midiLearn; ml.clear();
    juce::AudioBuffer<float> buf (2, 256);
    auto send = [&] (std::initializer_list<juce::MidiMessage> msgs) { juce::MidiBuffer mb; int t = 0; for (auto& m : msgs) mb.addEvent (m, t++); buf.clear(); p.processBlock (buf, mb); };
    auto* prm = p.apvts.getParameter ("cut_bass"); const int idx = ml.indexOf ("cut_bass");
    if (prm == nullptr || idx < 0 || ml.numParams() < 50) { fail ("midi learn: parameter list " + juce::String (ml.numParams())); return; }
    // öğren: kanal modu mesajı (123) öğrenmez, normal CC öğrenir
    ml.startLearn (idx); send ({ juce::MidiMessage::controllerEvent (1, 123, 0) });
    if (ml.learning() != idx) fail ("midi learn: channel mode message was learned");
    send ({ juce::MidiMessage::controllerEvent (1, 21, 64) });
    auto m = ml.mappingFor (idx); if (ml.learning() != -1 || m.cc != 21 || m.channel != 1) fail ("midi learn: CC 21 not learned (" + juce::String (m.cc) + "/" + juce::String (m.channel) + ")");
    // yakalama: parametre 0.2; CC 127 uzakta -> değişmez; yakın değer -> yakalar; sonra izler
    prm->setValueNotifyingHost (0.2f);
    send ({ juce::MidiMessage::controllerEvent (1, 21, 127) }); if (std::abs (prm->getValue() - 0.2f) > 0.02f) fail ("midi learn: jumped without pickup " + juce::String (prm->getValue()));
    send ({ juce::MidiMessage::controllerEvent (1, 21, 26) }); send ({ juce::MidiMessage::controllerEvent (1, 21, 100) });
    if (std::abs (prm->getValue() - 100.0f / 127.0f) > 0.02f) fail ("midi learn: did not follow after pickup " + juce::String (prm->getValue()));
    // dışarıdan değişim (otomasyon / fare): CC bırakır, değerin üzerinden geçince yeniden yakalar
    prm->setValueNotifyingHost (0.9f);
    send ({ juce::MidiMessage::controllerEvent (1, 21, 50) }); if (std::abs (prm->getValue() - 0.9f) > 0.02f) fail ("midi learn: overrode automation " + juce::String (prm->getValue()));
    send ({ juce::MidiMessage::controllerEvent (1, 21, 122) }); if (std::abs (prm->getValue() - 122.0f / 127.0f) > 0.02f) fail ("midi learn: no pickup after crossing " + juce::String (prm->getValue()));
    // eşlenmemiş CC ve başka kanal: dokunmaz
    const float keep = prm->getValue(); send ({ juce::MidiMessage::controllerEvent (1, 22, 0), juce::MidiMessage::controllerEvent (2, 21, 0) });
    if (std::abs (prm->getValue() - keep) > 1e-6f) fail ("midi learn: unmapped CC changed the parameter");
    // DSP'ye ulaşır mı: ham APVTS değeri
    if (std::abs (p.apvts.getParameter ("cut_bass")->convertFrom0to1 (prm->getValue()) - p.apvts.getRawParameterValue ("cut_bass")->load()) > 1e-3f) fail ("midi learn: raw value not updated");
    // ikinci eşleme aynı CC'yi alırsa ilki düşer; kademeli (seçim) parametre
    const int idx2 = ml.indexOf ("res_bass"); ml.setMapping (idx2, 1, 21); if (ml.mappingFor (idx).param != -1 || ml.mappingFor (idx2).cc != 21) fail ("midi learn: CC reassignment");
    ml.setMapping (idx, 3, 74);
    for (int i = 0; i < 20; ++i) ml.timerTick();   // jestleri aç/kapat
    // durum: proje durumuna yazılır, preset ağacına girmez, geri yüklenir
    juce::MemoryBlock mb; p.getStateInformation (mb);
    { auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.midiLearn.clear(); q.setStateInformation (mb.getData(), (int) mb.getSize());
      if (q.midiLearn.mappingFor (q.midiLearn.indexOf ("cut_bass")).cc != 74 || q.midiLearn.mappingFor (q.midiLearn.indexOf ("res_bass")).cc != 21) fail ("midi learn: mappings not restored");
      if (q.apvts.state.getChildWithName ("midimap").isValid()) fail ("midi learn: midimap leaked into the parameter tree (presets)");
      juce::MemoryBlock again; q.getStateInformation (again); auto xml = juce::AudioProcessor::getXmlFromBinary (again.getData(), (int) again.getSize());
      if (xml == nullptr || xml->getChildByName ("midimap") == nullptr || xml->getChildByName ("midimap")->getNumChildElements() != 2) fail ("midi learn: re-save lost mappings"); }
    // eski proje (midimap yok): mevcut eşlemeler korunur, çökme yok; bozuk midimap
    { auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize()); xml->deleteAllChildElementsWithTagName ("midimap");
      auto bad = std::make_unique<juce::XmlElement> ("midimap"); auto* e = bad->createNewChildElement ("map"); e->setAttribute ("param", "nope"); e->setAttribute ("ch", 99); e->setAttribute ("cc", 500);
      juce::MemoryBlock old; juce::AudioProcessor::copyXmlToBinary (*xml, old);
      auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.midiLearn.clear(); q.midiLearn.setMapping (q.midiLearn.indexOf ("vol_bass"), 1, 7);
      q.setStateInformation (old.getData(), (int) old.getSize()); if (q.midiLearn.all().size() != 1) fail ("midi learn: old project cleared mappings");
      xml->addChildElement (bad.release()); juce::MemoryBlock bb; juce::AudioProcessor::copyXmlToBinary (*xml, bb); q.setStateInformation (bb.getData(), (int) bb.getSize());
      if (! q.midiLearn.all().empty()) fail ("midi learn: invalid mapping accepted"); }
    // kaldır / temizle
    ml.removeMapping (idx); if (ml.mappingFor (idx).param != -1) fail ("midi learn: remove");
    ml.clear(); if (! ml.all().empty()) fail ("midi learn: clear");
    // notalar CC eşlemesinden etkilenmez
    ml.setMapping (idx, 1, 21); send ({ juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), juce::MidiMessage::controllerEvent (1, 21, 60) });
    // arayüz: MIDI Studio sekmesinde rozet ve öğrenme bildirimi
    { std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor())); ed->setTab (0); ed->refreshMidiNow();
      juce::Component* knob = nullptr; std::function<void (juce::Component&)> find = [&] (juce::Component& c) { if (knob == nullptr && c.getProperties()["hgParam"].toString().startsWith ("cut_")) knob = &c; for (auto* ch : c.getChildren()) find (*ch); };
      find (ed->contentComponent());
      if (knob != nullptr) { ml.clear(); ml.setMapping (ml.indexOf (knob->getProperties()["hgParam"].toString()), 1, 21); ed->refreshMidiNow(); }
      if (knob == nullptr) fail ("midi learn: filter knob not tagged");
      else if (knob->getProperties()["hgCC"].toString() != "CC21") fail ("midi learn: badge missing: " + knob->getProperties()["hgCC"].toString());
      ed->midiLearnFor ("res_bass"); ed->refreshMidiNow();
      { auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f); auto f = outDir.getChildFile ("plugin-en-midilearn.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os); }
      send ({ juce::MidiMessage::controllerEvent (1, 74, 10) }); ed->refreshMidiNow();
      if (ml.mappingFor (ml.indexOf ("res_bass")).cc != 74 || ! ed->midiToastText().contains ("74")) fail ("midi learn: editor learn flow: " + ed->midiToastText()); }
    ml.clear();
    std::cout << "midi learn ok\n";
}
static void testLibrary (const FailFn& fail)
{
    // 8) DNA kütüphanesi: kaydet, ara, favori, son kullanılan, yeniden adlandır, dışa/içe aktar, sil, bozuk dosya
    {
        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-lib-test"); dir.deleteRecursively(); dir.createDirectory();
        hg::DnaLibrary lib (dir); hg::TrackDna d; d.role = "lead"; d.sourceFile = "demo.wav"; d.f0Hz = 440; d.pitched = true; d.envDb.assign (16, -20.0f); d.harmonics.assign (8, 0.5f);
        juce::File f1, f2; bool ok = lib.save (d, "Wavejackers", "Emotional Lead", &f1) && lib.save (d, "Wavejackers", "Deep Bass", &f2) && lib.save (d, "Black Coffee", "My Pad");
        if (! ok || lib.entries().size() != 3) fail ("library save");
        if (lib.query ("deep", hg::DnaLibrary::All).size() != 1 || lib.query ("WAVEJACKERS", hg::DnaLibrary::All).size() != 2 || lib.query ("zzz", hg::DnaLibrary::All).size() != 0) fail ("library search");
        auto e = lib.query ("deep", hg::DnaLibrary::All)[0]; lib.setFavorite (e, true);
        if (lib.query ("", hg::DnaLibrary::Favorites).size() != 1) fail ("library favorite");
        hg::TrackDna got; e = lib.query ("deep", hg::DnaLibrary::All)[0]; if (! lib.load (e, got) || got.name != "Deep Bass" || got.artist != "Wavejackers") fail ("library load");
        if (lib.query ("", hg::DnaLibrary::Recent).size() != 1) fail ("library recent");
        e = lib.query ("deep", hg::DnaLibrary::All)[0]; if (! lib.rename (e, "Sub Bass") || lib.query ("sub", hg::DnaLibrary::All).size() != 1) fail ("library rename");
        if (lib.query ("", hg::DnaLibrary::Favorites).size() != 1 || lib.query ("", hg::DnaLibrary::Recent).size() != 0) fail ("library rename keeps favorite / drops stale recent");
        e = lib.query ("sub", hg::DnaLibrary::All)[0]; const auto ex = dir.getChildFile ("..").getChildFile ("hitgen-lib-export.hgdna"); ex.deleteFile();
        if (! lib.exportTo (e, ex)) fail ("library export");
        hg::DnaLibrary lib2 (dir.getChildFile ("..").getChildFile ("hitgen-lib-test2")); if (! lib2.importFile (ex) || lib2.entries().size() != 1) fail ("library import");
        auto junk = dir.getChildFile ("..").getChildFile ("junk.hgdna"); junk.replaceWithText ("{ not a dna"); if (lib2.importFile (junk)) fail ("library accepted junk");
        if (! lib.remove (e) || lib.entries().size() != 2) fail ("library delete");
        hg::DnaLibrary lib3 (dir);   // yeniden aç: kalıcılık
        if (lib3.entries().size() != 2) fail ("library persistence");
        hg::DnaEntry outside; outside.file = junk; if (lib.remove (outside)) fail ("library deleted a file outside the folder");
        dir.getChildFile ("..").getChildFile ("hitgen-lib-test2").deleteRecursively(); ex.deleteFile(); junk.deleteFile();
        // 2.5: etiket araması (anahtar ve çevrilmiş ad) ve klasörü toplu analiz
        { const auto td = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-tag-test"); td.deleteRecursively(); td.createDirectory();
          hg::DnaLibrary tl (td); hg::TrackDna t1 = d; t1.pitched = true; hg::TrackDna t2 = d; t2.pitched = false; tl.save (t1, "A", "One"); tl.save (t2, "A", "Two");
          if (tl.query ("tagUnpitched", hg::DnaLibrary::All).size() != 1) fail ("tag search by key");
          if (tl.query ("perdesiz", hg::DnaLibrary::All, [] (const juce::String& k) { return k == "tagUnpitched" ? juce::String ("Perdesiz") : k; }).size() != 1) fail ("tag search by translated name");
          // toplu analiz: 2 sentetik dosya + 1 bozuk dosya
          const auto bd = td.getChildFile ("Batch Artist"); bd.createDirectory(); const double sr = 44100; juce::AudioBuffer<float> b (2, (int) (sr * 8)); b.clear(); juce::Random rnd (7);
          for (int q = 0; q < 16; ++q) { const int s0 = (int) (q * 0.476 * sr); for (int i = 0; i < (int) (0.4 * sr) && s0 + i < b.getNumSamples(); ++i) { const double t = i / sr; float v = 0; for (int h = 1; h < 9; ++h) v += (float) std::sin (2 * juce::MathConstants<double>::pi * 55 * h * t) / h; v *= 0.3f * (float) std::exp (-t * 3); b.addSample (0, s0 + i, v); b.addSample (1, s0 + i, v); } }
          for (int k = 0; k < 2; ++k) { auto wf = bd.getChildFile ("loop" + juce::String (k) + ".wav"); std::unique_ptr<juce::FileOutputStream> os (wf.createOutputStream()); juce::WavAudioFormat fmt; std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (os.get(), sr, 2, 16, {}, 0)); if (w) { os.release(); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); } }
          bd.getChildFile ("broken.wav").replaceWithText ("nope");
          auto pump = [&] (const std::function<bool()>& done, int ms) { const auto t0 = juce::Time::getMillisecondCounter(); while (! done() && juce::Time::getMillisecondCounter() - t0 < (juce::uint32) ms) juce::MessageManager::getInstance()->runDispatchLoopUntil (20); return done(); };
          hg::BatchAnalyser ba; int filesSeen = 0, dnaCount = 0, errors = 0; bool done = false, cancelled = true;
          ba.onFile = [&] (int, int, const juce::File&, const std::vector<hg::TrackDna>& v, const juce::String& err) { ++filesSeen; dnaCount += (int) v.size(); if (err.isNotEmpty()) ++errors; for (auto& x : v) tl.save (x, bd.getFileName(), x.name); };
          ba.onDone = [&] (bool c) { done = true; cancelled = c; };
          const auto files = hg::BatchAnalyser::audioFilesIn (bd); if (files.size() != 3) fail ("batch file scan " + juce::String (files.size()));
          ba.start (files); pump ([&] { return done; }, 60000);
          if (! done || cancelled || filesSeen != 3 || errors != 1 || dnaCount < 2) fail ("batch analysis: files " + juce::String (filesSeen) + " dna " + juce::String (dnaCount) + " errors " + juce::String (errors));
          if (tl.query ("Batch Artist", hg::DnaLibrary::All).size() != (size_t) dnaCount) fail ("batch results not in library");
          // iptal: hemen iptal et, takılmadan bitmeli
          hg::BatchAnalyser bc; bool d2 = false; bc.onDone = [&] (bool) { d2 = true; }; bc.start (files); bc.cancel(); pump ([&] { return d2; }, 10000); if (! d2) fail ("batch cancel did not finish");
          std::cout << "tags + batch ok: " << dnaCount << " DNA from " << filesSeen << " files (" << errors << " broken)\n";
          td.deleteRecursively(); }
        std::cout << "library tests done\n";
    }
}


static void snapPng (juce::Component& c, const juce::File& f)
{
    auto img = c.createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
    f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
}

// kendi sesin (ses dosyası -> akor/bas/melodi örnekleyicisi) ve MIDI düzenleme: çalma, perde, durum, dışa aktarma, geri al
static void testOwnSoundAndEdit (const juce::File& outDir, const FailFn& fail)
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-own-test"); dir.deleteRecursively(); dir.createDirectory();
    auto writeSine = [&] (const juce::String& name, double hz)
    {
        auto f = dir.getChildFile (name); juce::AudioBuffer<float> b (1, 44100);
        for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (0, i, 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * hz * i / 44100.0) * (1.0f - i / 44100.0f));
        juce::WavAudioFormat w; std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream()); std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (os.get(), 44100, 1, 16, {}, 0));
        if (wr) { os.release(); wr->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
        return f;
    };
    auto named = writeSine ("Lead_A4.wav", 440.0), plain = writeSine ("tone.wav", 220.0);
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.regenerateNow(); p.setRateAndBufferSizeDetails (48000.0, 256); p.prepareToPlay (48000.0, 256);
    juce::String err; int root = -1;
    if (! p.loadInstSample (2, named, err, &root) || root != 69) fail ("own sound: root from file name " + juce::String (root) + " " + err);
    if (! p.loadInstSample (0, plain, err, &root) || root != 57) fail ("own sound: detected root " + juce::String (root) + " " + err);
    // melodi izinde A5 (81) çal: örnek bir oktav yukarı, ~880 Hz
    juce::AudioBuffer<float> out (2, 256); p.engine.render (out, 0, 256);   // bekleyen örneği al
    p.engine.noteOn (hg::kMelody, 81, 110, 0.5, 0);
    juce::AudioBuffer<float> rec (1, 48000 / 4); int at = 0; float pk = 0;
    while (at < rec.getNumSamples()) { out.clear(); p.engine.render (out, 0, 256); const int k = juce::jmin (256, rec.getNumSamples() - at); rec.copyFrom (0, at, out, 0, 0, k); pk = std::max (pk, out.getMagnitude (0, 256)); at += k; }
    int zc = 0; for (int i = 2400; i + 1 < rec.getNumSamples(); ++i) if ((rec.getSample (0, i) < 0) != (rec.getSample (0, i + 1) < 0)) ++zc;
    const double hz = zc / 2.0 / ((rec.getNumSamples() - 2400) / 48000.0);
    if (pk < 0.02f || std::abs (hz - 880.0) > 25.0) fail ("own sound: playback peak " + juce::String (pk) + " pitch " + juce::String (hz, 1));
    // MIDI düzenleme: melodiye nota ekle, dışa aktarımda görünür
    const auto before = p.midiBytes (hg::kMelody);
    auto e = std::make_shared<hg::Result> (*p.result()); e->tracks[hg::kMelody].push_back ({ 3.0, 2.0, 77, 99 });
    const size_t nMel = e->tracks[hg::kMelody].size();
    p.applyEditedResult (e);
    if (! p.hasEdits() || p.result()->tracks[hg::kMelody].size() != nMel || p.midiBytes (hg::kMelody) == before) fail ("edit: not applied to playback/export");
    // yalnızca tempo değişirse düzenleme korunur
    setP (p, "bpm", 131.0f); p.regenerateNow();
    if (! p.hasEdits() || p.result()->tracks[hg::kMelody].size() != nMel) fail ("edit: lost on tempo change");
    // proje durumu: kendi sesler ve düzenleme geri yüklenir
    juce::MemoryBlock st; p.getStateInformation (st);
    {
        auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (st.getData(), (int) st.getSize()); q.regenerateNow();
        if (q.instSamplePath (2) != named.getFullPathName() || q.instSamplePath (0) != plain.getFullPathName()) fail ("own sound: not restored from state");
        if (! q.hasEdits() || q.result()->tracks[hg::kMelody].size() != nMel) fail ("edit: not restored from state");
    }
    // müzikal ayar değişince yeni fikir gelir; geri dön düğmesi düzenlemeyi bırakır
    setP (p, "seed", 4242.0f); p.regenerateNow();
    if (p.hasEdits()) fail ("edit: kept after seed change");
    // piyano rulosu: klavyeyle seç, yukarı taşı, geri al
    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    auto& roll = ed->pianoRoll(); roll.setEditing (true, hg::kMelody);
    const auto r0 = p.result(); const int n0 = r0->tracks[hg::kMelody].empty() ? -1 : r0->tracks[hg::kMelody][0].n;
    roll.keyPressed (juce::KeyPress (juce::KeyPress::rightKey)); roll.keyPressed (juce::KeyPress (juce::KeyPress::upKey));
    if (n0 < 0 || ! p.hasEdits() || p.result()->tracks[hg::kMelody][0].n != n0 + 1) fail ("roll: arrow edit not applied");
    snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-en-midiedit.png"));
    {   // yerleşim: piyano rulosunun üstünde başka görünür denetim olmamalı (ör. ayarlar panelinin düğmesi dışarıda kalmış)
        auto& c = ed->contentComponent(); const auto rb = roll.getBounds();
        for (auto* ch : c.getChildren())
            if (ch != &roll && ch->isVisible() && ch->getBounds() != c.getLocalBounds() && ch->getBounds().intersects (rb))
                fail ("layout: '" + ch->getTitle() + "' / " + ch->getName() + " overlaps the piano roll at " + ch->getBounds().toString());
    }
    roll.undo();
    if (p.result()->tracks[hg::kMelody][0].n != n0) fail ("roll: undo failed");
    roll.redo();
    if (p.result()->tracks[hg::kMelody][0].n != n0 + 1) fail ("roll: redo failed");
    {   // boyama (sol tuş basılı sürükle) ve yol boyunca silme (sağ tuş basılı sürükle)
        auto empty = std::make_shared<hg::Result> (*p.result()); empty->tracks[hg::kMelody].clear(); p.applyEditedResult (empty);
        roll.setEditing (true, hg::kMelody);
        auto src = juce::Desktop::getInstance().getMainMouseSource();
        auto ev = [&] (juce::Point<float> pos, juce::ModifierKeys mods, juce::Point<float> down) {
            return juce::MouseEvent (src, pos, mods, 1.0f, 0, 0, 0, 0, &roll, &roll, juce::Time::getCurrentTime(), down, juce::Time::getCurrentTime(), 1, pos != down); };
        const auto b = roll.getLocalBounds().toFloat(); const float y = b.getCentreY();
        const juce::Point<float> a (b.getX() + b.getWidth() * .25f, y), z (b.getX() + b.getWidth() * .75f, y);
        const auto L = juce::ModifierKeys (juce::ModifierKeys::leftButtonModifier), R = juce::ModifierKeys (juce::ModifierKeys::rightButtonModifier);
        roll.mouseDown (ev (a, L, a));
        for (int k = 1; k <= 20; ++k) roll.mouseDrag (ev (a + (z - a) * (k / 20.0f), L, a));
        roll.mouseUp (ev (z, L, a));
        const auto painted = p.result()->tracks[hg::kMelody].size();
        if (painted < 4) fail ("roll: paint drag made only " + juce::String ((int) painted) + " notes");
        roll.mouseDown (ev (z, R, z));
        roll.mouseDrag (ev (a, R, z));   // tek büyük adım: aradaki notalar da silinmeli
        roll.mouseUp (ev (a, R, z));
        if (! p.result()->tracks[hg::kMelody].empty()) fail ("roll: right-drag erase left " + juce::String ((int) p.result()->tracks[hg::kMelody].size()) + " notes");
        roll.undo();
        if (p.result()->tracks[hg::kMelody].size() != painted) fail ("roll: undo after erase");
        std::cout << "roll paint/erase ok: " << (int) painted << " notes painted\n";
    }
    p.clearInstSample (0); p.clearInstSample (2); out.clear(); p.engine.render (out, 0, 256); p.engine.collectGarbage();
    if (p.instSamplePath (2).isNotEmpty()) fail ("own sound: clear failed");
    dir.deleteRecursively();
    std::cout << "own sound + midi edit ok: pitch " << juce::String (hz, 1) << " Hz, peak " << juce::String (pk, 3) << "\n";
}




// HitGen 3.0 köprüleri: çok izli MIDI (iz adları, nota sayıları, işaretçiler), manifest, FL Piano Roll script'leri, stem'ler (uzunluk, zamanlama)
static bool quickRun() { return juce::SystemStats::getEnvironmentVariable ("HITGEN_QUICK", {}).isNotEmpty(); }
static void checkBridges (const hg::ExportResult& er, const hg::Skeleton& sk, const FailFn& fail)
{
    const auto arr = hg::buildArrangement (sk);
    {   // türetilmiş katmanlar: pad (uzun notalar, 48-72), arp (drop'larda, 54-78), lead = melodi
        const hg::ArrLane* pad = nullptr; const hg::ArrLane* arp = nullptr; bool lead = false;
        for (auto& L : arr.lanes) { if (L.role == "pad") pad = &L; if (L.role == "arp") arp = &L; if (L.role == "lead") lead = true; }
        if (pad == nullptr || arp == nullptr || ! lead) fail ("bridges: pad/arp/lead lanes missing");
        else
        {
            for (auto& n : pad->notes) if (n.n < 48 || n.n > 72 || n.l < 16) { fail ("bridges: pad note out of range"); break; }
            for (auto& n : arp->notes) if (n.n < 54 || n.n > 78 || n.l > 2) { fail ("bridges: arp note out of range"); break; }
            int inDrop = 0, inBreak = 0;
            for (auto& n : arp->notes) for (auto& x : sk.secs) if (n.s >= x.bar * 16 && n.s < (x.bar + x.bars) * 16) { if (x.kind == "drop") ++inDrop; if (x.kind == "break") ++inBreak; }
            if (inDrop == 0 || inBreak != 0) fail ("bridges: arp placement drop " + juce::String (inDrop) + " break " + juce::String (inBreak));
        }
    }
    {   // "Blessings" teknikleri: seyrek perküsyon, oktav yükselen stab, hızlanan buildup + pitch bend
        const hg::ArrLane* stab = nullptr; const hg::ArrLane* stut = nullptr; const hg::ArrLane* perc = nullptr;
        for (auto& L : arr.lanes) { if (L.role == "stab") stab = &L; if (L.role == "stutter") stut = &L; if (L.role == "perc") perc = &L; }
        const bool organic = arr.profile == "organic" && hg::Data::get().styleById (arr.P.style)["grp"].toString() == "afro";
        if (organic != (stab != nullptr)) fail ("bridges: stab lane only for organic");
        if (organic && perc != nullptr)
        {
            std::map<int, int> perBar; for (auto& n : perc->notes) ++perBar[(int) (n.s / 16.0)];
            for (auto& [b, c] : perBar) if (c > 2) { fail ("bridges: perc not thinned, bar " + juce::String (b) + " has " + juce::String (c)); break; }
        }
        if (stab != nullptr)
        {
            int mains = 0, lowest = 999, highestFirst = -1, lowestLater = 999;
            for (auto& x : sk.secs) if (x.kind == "drop" || x.kind == "groove")
            {
                int lo = 999, hi = -1;
                for (auto& n : stab->notes) if (n.s >= x.bar * 16 && n.s < (x.bar + x.bars) * 16) { lo = std::min (lo, n.n); hi = std::max (hi, n.n); }
                if (lo == 999) continue;
                if (mains == 0) { lowest = lo; highestFirst = hi; } else lowestLater = std::min (lowestLater, lo);
                ++mains;
            }
            if (mains == 0 || lowest != highestFirst) fail ("bridges: stab first drop must be a single pitch");
            if (mains > 1 && lowestLater != lowest + 12) fail ("bridges: stab later drops must be one octave up");
            for (auto& n : stab->notes) if (n.n < 36 || n.n > 59) { fail ("bridges: stab note range"); break; }
        }
        int builds = 0; for (auto& x : sk.secs) if (x.kind == "build" && x.bars >= 4) ++builds;
        if (builds > 0)
        {
            if (stut == nullptr) fail ("bridges: stutter lane missing");
            else
            {
                int bendUp = 0, resets = 0, maxBend = 0;
                for (auto& e : stut->autos) if (e.kind < 0)
                {
                    maxBend = std::max (maxBend, e.value);
                    bool atEnd = false; for (auto& x : sk.secs) if (x.kind == "build" && e.s == (x.bar + x.bars) * 16.0) atEnd = true;
                    if (atEnd && e.value == 8192) ++resets; else if (e.value > 8192) ++bendUp;
                }
                if (bendUp == 0 || resets != builds || maxBend <= 8192 || maxBend > 16383 || (stut->bendRange != 12 && stut->bendRange != 24)) fail ("bridges: stutter pitch bend ramp");
                for (auto& x : sk.secs) if (x.kind == "build" && x.bars >= 4)
                {   // son ölçüde ilk ölçüden daha çok nota: hızlanma
                    const double end = (x.bar + x.bars) * 16.0; int first = 0, last = 0;
                    for (auto& n : stut->notes) { if (n.s >= end - 64 && n.s < end - 48) ++first; if (n.s >= end - 16 && n.s < end) ++last; }
                    if (! (first > 0 && last >= first * 4)) { fail ("bridges: stutter does not accelerate (" + juce::String (first) + " -> " + juce::String (last) + ")"); break; }
                }
                juce::FileInputStream in (er.midi); juce::MidiFile mf;
                if (in.openedOk() && mf.readFrom (in))
                {
                    int bends = 0, rpn = 0;
                    for (int t = 1; t < mf.getNumTracks(); ++t)
                    {
                        bool isStut = false; int b = 0, r = 0;
                        for (auto* e : *mf.getTrack (t)) { if (e->message.isTrackNameEvent() && e->message.getTextFromTextMetaEvent() == "Stutter") isStut = true; if (e->message.isPitchWheel()) ++b; if (e->message.isController() && e->message.getControllerNumber() == 101) ++r; }
                        if (isStut) { bends = b; rpn = r; }
                    }
                    if (bends != (int) std::count_if (stut->autos.begin(), stut->autos.end(), [] (const hg::ArrAuto& e) { return e.kind < 0; }) || rpn != 1) fail ("bridges: stutter midi bend/RPN events " + juce::String (bends) + "/" + juce::String (rpn));
                }
            }
        }
    }
    {
        juce::FileInputStream in (er.midi); juce::MidiFile mf;
        if (! in.openedOk() || ! mf.readFrom (in)) { fail ("bridges: arrangement midi unreadable"); return; }
        if (mf.getNumTracks() != (int) arr.lanes.size() + 1) fail ("bridges: midi tracks " + juce::String (mf.getNumTracks()));
        int markers = 0; for (auto* e : *mf.getTrack (0)) if (e->message.isTextMetaEvent() && e->message.getMetaEventType() == 6) ++markers;
        int wantMarkers = (int) sk.secs.size(); for (auto& c : sk.cues) if (c.type != "pad") ++wantMarkers;
        if (markers != wantMarkers) fail ("bridges: midi markers " + juce::String (markers) + " want " + juce::String (wantMarkers));
        for (int t = 1; t < mf.getNumTracks() && t <= (int) arr.lanes.size(); ++t)
        {
            juce::String name; int ons = 0, offs = 0;
            for (auto* e : *mf.getTrack (t)) { if (e->message.isTrackNameEvent()) name = e->message.getTextFromTextMetaEvent(); if (e->message.isNoteOn()) ++ons; if (e->message.isNoteOff()) ++offs; }
            const auto& L = arr.lanes[(size_t) t - 1];
            if (name != L.name || ons != (int) L.notes.size() || offs != ons) fail ("bridges: midi track " + name + " notes " + juce::String (ons) + "/" + juce::String ((int) L.notes.size()));
        }
    }
    {
        const auto v = juce::JSON::parse (er.manifest.loadFileAsString());
        if ((int) v["bpm"] != sk.song.P.bpm || (int) v["bars"] != sk.song.bars || v["sections"].size() != (int) sk.secs.size() || v["tracks"].size() != (int) arr.lanes.size())
            fail ("bridges: manifest fields");
        const bool stems = ! quickRun();
        if ((stems && v["files"]["stem_bass"].toString().isEmpty()) || v["files"]["flStudioProject"].toString().isEmpty() || v["files"]["midiArrangement"].toString().isEmpty()) fail ("bridges: manifest files");
        for (auto& k : { "stem_drums", "stem_bass", "midiArrangement", "flStudioProject", "plan" })
            if ((stems || ! juce::String (k).startsWith ("stem")) && ! er.dir.getChildFile (v["files"][k].toString()).exists()) fail (juce::String ("bridges: manifest points to a missing file: ") + k);
    }
    {
        auto scripts = er.dir.getChildFile ("FL Piano Roll Scripts").findChildFiles (juce::File::findFiles, false, "*.pyscript");
        if (scripts.size() != (int) arr.lanes.size() || er.scripts != scripts.size()) fail ("bridges: scripts " + juce::String (scripts.size()));
        for (auto& f : scripts)
        {
            const auto t = f.loadFileAsString();
            int rows = 0; for (auto& line : juce::StringArray::fromLines (t)) if (line.startsWith ("    (") && line.endsWith ("),") && line.containsChar (',') && ! line.contains ("\"")) ++rows;
            const auto lane = f.getFileNameWithoutExtension().fromFirstOccurrenceOf ("HitGen - ", false, false);
            int want = -1; for (auto& L : arr.lanes) if (L.name == lane) want = (int) L.notes.size();
            if (! t.contains ("import flpianoroll as flp") || ! t.contains ("def apply(form):") || ! t.contains ("def createDialog():") || rows != want)
                fail ("bridges: script " + f.getFileName() + " rows " + juce::String (rows) + " want " + juce::String (want));
        }
    }
    {
        int groups = 1; for (auto& L : arr.lanes) if (L.gm < 0) ++groups;   // davullar tek stem + her melodik şerit
        if (er.stems != groups && ! quickRun()) fail ("bridges: stems " + juce::String (er.stems) + " want " + juce::String (groups));
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        auto rms = [] (juce::AudioFormatReader& r, double t0, double t1)
        {
            const auto a = (juce::int64) (t0 * r.sampleRate), b = (juce::int64) (t1 * r.sampleRate);
            juce::AudioBuffer<float> x (2, (int) (b - a)); r.read (&x, 0, x.getNumSamples(), a, true, true);
            return x.getRMSLevel (0, 0, x.getNumSamples());
        };
        const double barSec = 240.0 / sk.song.P.bpm;
        for (auto& f : er.dir.getChildFile ("Stems").findChildFiles (juce::File::findFiles, false, "*.wav"))
        {
            std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
            if (r == nullptr) { fail ("bridges: stem unreadable " + f.getFileName()); continue; }
            const double secs = (double) r->lengthInSamples / r->sampleRate;
            if (std::abs (secs - (sk.song.bars * barSec + 2.0)) > .05) fail ("bridges: stem length " + f.getFileName() + " " + juce::String (secs, 2));
            if (r->numChannels != 2 || r->bitsPerSample != 24) fail ("bridges: stem format " + f.getFileName());
            // bas stem'i: breakdown'un ortasında sessiz, drop'ta sesli (zamanlama doğru)
            if (f.getFileName().contains ("Bass"))
                for (auto& x : sk.secs)
                {
                    const double mid0 = (x.bar + x.bars * .5) * barSec, mid1 = mid0 + barSec;
                    const float level = rms (*r, mid0, mid1);
                    if (x.kind == "break" && level > .003f) fail ("bridges: bass stem not silent in " + x.name + " (" + juce::String (level, 4) + ")");
                    if (x.kind == "drop" && level < .003f) fail ("bridges: bass stem silent in " + x.name);
                }
            if (f.getFileName().contains ("Drums")) { bool any = false; for (auto& x : sk.secs) if (x.kind == "drop" && rms (*r, x.bar * barSec, (x.bar + 2) * barSec) > .01f) any = true; if (! any) fail ("bridges: drums stem silent in drops"); }
        }
    }
    std::cout << "bridges ok: " << arr.lanes.size() << " midi tracks, " << er.scripts << " FL scripts, " << er.stems << " stems, manifest\n";
}

// Ableton Live seti: gzip XML ayrıştırılır; iz/klip/nota/işaretçi sayıları, tempo, benzersiz hedef kimlikleri,
// izlerde çakışan klip olmaması ve her sample referansının (göreli yol) diskte bir dosyaya çıkması
static double dbOf (double x) { return 20.0 * std::log10 (std::max (1e-12, x)); }
static double rmsDbOf (const juce::AudioBuffer<float>& b)
{
    double e = 0; for (int c = 0; c < b.getNumChannels(); ++c) { auto* d = b.getReadPointer (c); for (int i = 0; i < b.getNumSamples(); ++i) e += (double) d[i] * d[i]; }
    return dbOf (std::sqrt (e / (double) (b.getNumChannels() * b.getNumSamples())));
}

// Pembe gürültü referansı ve mix ölçütü
static void testMixCore (const FailFn& fail)
{
    const double sr = 44100;
    const auto n = hg::pinkNoise (sr, 20.0, hg::kRefNoiseRmsDb, 1);
    if (std::abs (rmsDbOf (n) - hg::kRefNoiseRmsDb) > .2) fail ("mix: pink noise rms " + juce::String (rmsDbOf (n), 2));
    if (n.getMagnitude (0, n.getNumSamples()) > 1.0f) fail ("mix: pink noise clips");
    // -3 dB/oktav: oktav bantlarının gücü (125-250 Hz ile 1-2 kHz arası 3 oktav = -9 dB, bant genişliği başına eşit enerji)
    juce::dsp::FFT fft (14); std::vector<float> buf (1 << 15); std::vector<double> acc (1 << 13, 0.0); int frames = 0;
    for (int pos = 0; pos + (1 << 14) <= n.getNumSamples(); pos += 1 << 14, ++frames)
    {
        std::fill (buf.begin(), buf.end(), 0.f);
        for (int i = 0; i < (1 << 14); ++i) buf[(size_t) i] = n.getSample (0, pos + i) * (.5f - .5f * std::cos (6.2831853f * i / (1 << 14)));
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        for (size_t k = 0; k < acc.size(); ++k) acc[k] += (double) buf[k] * buf[k];
    }
    auto band = [&] (double f0, double f1) { double e = 0; for (int k = (int) (f0 / sr * (1 << 14)); k < (int) (f1 / sr * (1 << 14)); ++k) e += acc[(size_t) k]; return e; };
    // pembe gürültü: her oktavda eşit enerji (güç yoğunluğu -3 dB/oktav)
    for (double f : { 125.0, 250.0, 500.0, 1000.0, 2000.0 })
    {
        const double dd = 10.0 * std::log10 (band (f * 2, f * 4) / band (f, f * 2));
        if (std::abs (dd) > 1.0) fail ("mix: pink noise octave " + juce::String (f, 0) + " Hz step " + juce::String (dd, 2) + " dB (expected 0)");
    }
    {   // pan: ortada değişmez, sağda sol kanal azalır, sağ kanal aynı kalır; yalnızca zayıflatır
        juce::AudioBuffer<float> x (2, 64); for (int c = 0; c < 2; ++c) for (int i = 0; i < 64; ++i) x.setSample (c, i, .5f);
        auto y = x; hg::applyPan (y, 0.0); if (std::abs (y.getSample (0, 3) - .5f) > 1e-6f) fail ("mix: pan center changed");
        y = x; hg::applyPan (y, 0.5); if (std::abs (y.getSample (1, 3) - .5f) > 1e-6f || std::abs (y.getSample (0, 3) - .5f * (float) std::cos (juce::MathConstants<double>::pi / 4)) > 1e-4f) fail ("mix: pan right");
        y = x; hg::applyPan (y, -1.0); if (y.getMagnitude (0, 0, 64) < .49f || y.getMagnitude (1, 0, 64) > 1e-4f) fail ("mix: pan hard left");
        if (hg::rolePan ("kick") != 0 || hg::rolePan ("bass") != 0 || hg::rolePan ("hat") == 0 || hg::rolePan ("perc") == hg::rolePan ("hat")) fail ("mix: pan table");
    }
    // duyulabilirlik ölçüsü doğrusal: gürültü +6 dB yükselince gereken kazanç tam 6 dB azalır
    hg::MixResult m;
    auto loud = hg::pinkNoise (sr, 5.0, hg::kRefNoiseRmsDb + 6.0, 3), quiet = hg::pinkNoise (sr, 5.0, hg::kRefNoiseRmsDb, 3);
    const double gl = hg::fxGainDb ("zz", loud, sr, m), gq = hg::fxGainDb ("zz", quiet, sr, m);
    if (std::abs ((gq - gl) - 6.0) > .3) fail ("mix: audibility not linear, delta " + juce::String (gq - gl, 2));
    // gürültünün kendisi: gürültüye göre ~ +3 dB hedefine yakın (en güçlü bantlarda yalnızca birkaç dB dalgalanma)
    if (std::abs (gq - (hg::kTargetAudDb - 0.0 + hg::fxOffsetDb ("zz"))) > 6.0) fail ("mix: noise vs noise gain " + juce::String (gq, 2));
}

// Dışa aktarılan projede mix: rapor, referans gürültü, -6 dBFS mix tepe, kırpma yok, kick/bas hi-hat/pad'den güçlü
static void checkMixExport (const hg::ExportResult& er, const juce::File& proj, const FailFn& fail)
{
    if (! er.mixReport.existsAsFile() || er.mixReport.getSize() < 200) fail ("mix: report missing");
    if (! er.pinkNoise.existsAsFile()) { fail ("mix: pink noise reference missing"); return; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    if (std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (er.pinkNoise)); rd == nullptr) fail ("mix: pink noise unreadable");
    else
    {
        juce::AudioBuffer<float> b (2, (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 10))); rd->read (&b, 0, b.getNumSamples(), 0, true, true);
        if (std::abs (rmsDbOf (b) - hg::kRefNoiseRmsDb) > .3) fail ("mix: reference noise rms " + juce::String (rmsDbOf (b), 2));
    }
    if (er.mixReport.loadFileAsString().contains ("pan") == false) fail ("mix: report has no pan column");
    if (std::abs (er.mixPeakDb - hg::kMixPeakDb) > .3) fail ("mix: peak " + juce::String (er.mixPeakDb, 2));
    if (er.mixRmsDb < -30 || er.mixRmsDb > -8) fail ("mix: rms " + juce::String (er.mixRmsDb, 2));
    for (auto& f : proj.getChildFile ("Samples").findChildFiles (juce::File::findFiles, true, "*.wav"))
        if (std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f)); rd != nullptr)
        {
            juce::AudioBuffer<float> b (2, (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 20))); rd->read (&b, 0, b.getNumSamples(), 0, true, true);
            if (b.getMagnitude (0, b.getNumSamples()) >= 1.0f) fail ("mix: sample clips: " + f.getFileName());
        }
}

static void checkAls (const juce::File& f, const hg::Skeleton& sk, const FailFn& fail)
{
    if (! f.existsAsFile()) { fail ("als: missing"); return; }
    auto xml = juce::parseXML (hg::readAlsXml (f));
    if (xml == nullptr || ! xml->hasTagName ("Ableton")) { fail ("als: not gzip XML"); return; }
    auto* ls = xml->getChildByName ("LiveSet");
    const auto arr = hg::buildArrangement (sk);
    auto val = [] (juce::XmlElement* e, const char* path) -> juce::String
    {
        for (auto& p : juce::StringArray::fromTokens (path, "/", "")) if (e == nullptr || (e = e->getChildByName (p)) == nullptr) return {};
        return e->getStringAttribute ("Value");
    };
    int midiT = 0, audioT = 0, notes = 0, refs = 0, badRefs = 0;
    std::set<int> trackIds, targets; int maxT = 0; bool dup = false;
    std::function<void (juce::XmlElement*)> walk = [&] (juce::XmlElement* e)
    {
        const auto t = e->getTagName();
        if (e->hasAttribute ("Id") && (t.endsWith ("Target") || t.startsWith ("ControllerTargets.")))
        { const int id = e->getIntAttribute ("Id"); dup |= ! targets.insert (id).second; maxT = juce::jmax (maxT, id); }
        if (t == "MidiNoteEvent") ++notes;
        if (t == "SampleRef")
            if (auto* fr = e->getChildByName ("FileRef"); fr != nullptr && val (fr, "HasRelativePath") == "true")
            {
                ++refs; auto d = f.getParentDirectory();
                if (auto* rp = fr->getChildByName ("RelativePath")) for (auto* r = rp->getFirstChildElement(); r != nullptr; r = r->getNextElement()) d = d.getChildFile (r->getStringAttribute ("Dir"));
                if (! d.getChildFile (val (fr, "Name")).existsAsFile()) ++badRefs;
            }
        for (auto* c = e->getFirstChildElement(); c != nullptr; c = c->getNextElement()) walk (c);
    };
    walk (ls);
    if (dup) fail ("als: duplicate automation target ids");
    if (val (ls, "NextPointeeId").getIntValue() <= maxT) fail ("als: NextPointeeId too small");
    if (refs == 0 || badRefs > 0) fail ("als: sample refs " + juce::String (badRefs) + "/" + juce::String (refs) + " missing");
    for (auto* t = ls->getChildByName ("Tracks")->getFirstChildElement(); t != nullptr; t = t->getNextElement())
    {
        if (! trackIds.insert (t->getIntAttribute ("Id")).second) fail ("als: duplicate track id");
        const bool m = t->hasTagName ("MidiTrack"); (m ? midiT : audioT)++;
        auto* ev = t->getChildByName ("DeviceChain")->getChildByName ("MainSequencer")->getChildByName (m ? "ClipTimeable" : "Sample")->getChildByName ("ArrangerAutomation")->getChildByName ("Events");
        double last = 0;
        for (auto* c = ev->getFirstChildElement(); c != nullptr; c = c->getNextElement())
        {
            const double a = val (c, "CurrentStart").getDoubleValue(), b = val (c, "CurrentEnd").getDoubleValue();
            if (a < last - 1e-6 || b <= a || b > arr.bars * 4.0 + 1e-6) fail ("als: clip overlap/range on " + val (t, "Name/EffectiveName"));
            last = b;
        }
    }
    if (midiT != (int) arr.lanes.size()) fail ("als: midi tracks " + juce::String (midiT) + " != lanes " + juce::String ((int) arr.lanes.size()));
    int want = 0; for (auto& L : arr.lanes) for (auto& n : L.notes) if (n.s < arr.bars * 16.0) ++want;
    if (notes != want) fail ("als: notes " + juce::String (notes) + " != " + juce::String (want));
    int locs = 0; for (auto* l = ls->getChildByName ("Locators")->getChildByName ("Locators")->getFirstChildElement(); l != nullptr; l = l->getNextElement()) ++locs;
    if (locs != (int) arr.secs.size()) fail ("als: locators");
    if (val (ls, "MasterTrack/DeviceChain/Mixer/Tempo/Manual").getIntValue() != arr.P.bpm) fail ("als: tempo");
    std::cout << "  als: " << midiT << " midi + " << audioT << " audio tracks, " << notes << " notes, " << refs << " sample refs\n";
}

// FL Studio projesi: biçim (başlık, olay boyları), kanal sayısı, tempo, bölüm işaretçileri, pattern'ler, playlist, 500 iz,
// sample yollarının diskte olması ve playlist'in şarkı boyunu kaplaması
static void checkFlp (const juce::File& f, const hg::Skeleton& sk, const FailFn& fail, bool withLibrary)
{
    juce::MemoryBlock mb;
    if (! f.existsAsFile() || ! f.loadFileAsData (mb)) { fail ("flp: not written " + f.getFullPathName()); return; }
    int ppq = 0, nch = 0; std::vector<hg::FlpEvent> ev;
    if (! hg::readFlpEvents (mb, ppq, nch, ev)) { fail ("flp: malformed"); return; }
    int chans = 0, markers = 0, tracks = 0, notesEv = 0, paths = 0, missing = 0, kick = 0; juce::uint32 tempo = 0; size_t plBytes = 0;
    int maxEnd = 0, clips = 0, patItems = 0;
    juce::StringArray names;
    auto utf16 = [] (const juce::MemoryBlock& m) { juce::String s; auto* d = static_cast<const juce::uint8*> (m.getData()); for (size_t i = 0; i + 1 < m.getSize(); i += 2) { const int c = d[i] | (d[i + 1] << 8); if (c == 0) break; s += juce::String::charToString ((juce::juce_wchar) c); } return s; };
    for (auto& e : ev)
    {
        if (e.id == 64) ++chans;
        if (e.id == 156) tempo = e.value;
        if (e.id == 148) ++markers;
        if (e.id == 238) ++tracks;
        if (e.id == 224) { ++notesEv; if (e.data.getSize() % 24 != 0) fail ("flp: note block size"); }
        if (e.id == 203) { names.add (utf16 (e.data)); if (names[names.size() - 1] == "Kick") ++kick; }
        if (e.id == 196) { ++paths; if (! juce::File (utf16 (e.data)).existsAsFile()) ++missing; }
        if (e.id == 233)
        {
            plBytes = e.data.getSize();
            auto* d = static_cast<const juce::uint8*> (e.data.getData());
            for (size_t k = 0; k + 32 <= plBytes; k += 32)
            {
                auto u32 = [&] (size_t o) { return (int) (d[k + o] | (d[k + o + 1] << 8) | (d[k + o + 2] << 16) | (d[k + o + 3] << 24)); };
                const int pos = u32 (0), len = u32 (8), idx = d[k + 6] | (d[k + 7] << 8);
                if (idx > 20480) ++patItems; else ++clips;
                maxEnd = std::max (maxEnd, pos + len);
            }
        }
    }
    if (ppq != 96 || nch != chans || chans < 5) fail ("flp: header ppq " + juce::String (ppq) + " channels " + juce::String (nch) + "/" + juce::String (chans));
    if ((int) tempo != sk.song.P.bpm * 1000) fail ("flp: tempo " + juce::String ((int) tempo));
    if (markers != (int) sk.secs.size()) fail ("flp: markers " + juce::String (markers));
    if (tracks != 500) fail ("flp: track data " + juce::String (tracks));
    if (plBytes == 0 || plBytes % 32 != 0) fail ("flp: playlist size");
    if (plBytes % 60 == 0) fail ("flp: playlist size is ambiguous (multiple of 60)");
    if (notesEv < (int) sk.secs.size() * 3 || (patItems != notesEv && patItems != notesEv + 1)) fail ("flp: patterns " + juce::String (notesEv) + " items " + juce::String (patItems));
    if (clips < 8) fail ("flp: audio clips " + juce::String (clips));
    if (maxEnd < sk.song.bars * 384 - 384 || maxEnd > sk.song.bars * 384) fail ("flp: playlist length " + juce::String (maxEnd / 384) + " bars, song " + juce::String (sk.song.bars));
    if (kick != 1 || ! names.contains ("Bass") || ! names.contains ("Chords")) fail ("flp: channel names " + names.joinIntoString (","));
    if (paths != chans || missing != 0) fail ("flp: sample paths " + juce::String (paths) + " missing " + juce::String (missing));
    std::cout << "flp ok (" << (withLibrary ? "library" : "HitGen sounds") << "): " << chans << " channels, " << notesEv << " patterns, " << clips << " clips, " << markers << " markers\n";
}

// proje iskeleti: süre hesabı, Black Coffee / FRERE kuralları, sample sınıflandırma ve önerileri, proje klasörü, arayüz
static void testSkeleton (const juce::File& outDir, const FailFn& fail)
{
    // 1) süre: her yapı, süre ve tempoda toplam ölçü hedefe yakın, bölümler 8'in katı, buildup en çok 16 ölçü
    const char* const structs[4][9] = { { "intro", "build", "drop", "break", "build", "drop", "outro" }, { "intro", "drop", "break", "build", "drop", "outro" },
                                        { "intro", "build", "drop", "break", "groove", "break", "build", "drop", "outro" }, { "intro", "groove", "break", "groove", "outro" } };
    for (auto& st : structs)
    {
        juce::StringArray k; for (auto* x : st) if (x != nullptr) k.add (x);
        for (int sec : { 240, 300, 360, 420, 480, 540, 600 })
            for (int bpm : { 118, 120, 122, 124, 126, 128, 132, 138, 145, 150 })
            {
                auto b = hg::skBars (k, sec, bpm); int tot = 0;
                for (int i = 0; i < (int) b.size(); ++i)
                {
                    tot += b[(size_t) i];
                    if (b[(size_t) i] % 8 != 0 || b[(size_t) i] < 8) fail ("skeleton: section not a multiple of 8");
                    if (k[i] == "build" && b[(size_t) i] > 16) fail ("skeleton: buildup longer than 16 bars");
                }
                if (std::abs (tot - sec * bpm / 240.0) > 4.01) fail ("skeleton: " + juce::String (sec) + "s @" + juce::String (bpm) + " -> " + juce::String (tot) + " bars");
            }
    }
    {
        juce::StringArray k; for (auto* x : structs[0]) if (x != nullptr) k.add (x);
        int t8 = 0, t6 = 0; for (int x : hg::skBars (k, 480, 120)) t8 += x; for (int x : hg::skBars (k, 360, 124)) t6 += x;
        if (t8 != 240) fail ("skeleton: 8:00 @120 should be 240 bars (Black Coffee length), got " + juce::String (t8));
        if (t6 != 184) fail ("skeleton: 6:00 @124 should be 184 bars, got " + juce::String (t6));
    }
    // 2) dosya adından tür (Black Coffee ve FRERE projelerindeki gerçek adlar)
    struct C { const char* n; int cat; int bpm; };
    const C cases[] = { { "TPS x CamelPhat - Signature Kick 01.wav", hg::kCatKick, 0 }, { "SM_909_Clap_50.wav", hg::kCatClap, 0 }, { "SM_Hi-hat_Main.wav", hg::kCatHat, 0 },
        { "Open Hat.wav", hg::kCatOpenHat, 0 }, { "Shaker 2.aif", hg::kCatShaker, 0 }, { "Tambourine Low.wav", hg::kCatShaker, 0 }, { "TPS - Astral - Riser 07 (124 BPM).wav", hg::kCatRiser, 124 },
        { "TPS - Astral - Uplifter 01 (124 BPM).wav", hg::kCatRiser, 124 }, { "PMSG_FX_119_16bars_Sweep_Up.wav", hg::kCatRiser, 119 }, { "TPS - Astral - Impact 09.wav", hg::kCatImpact, 0 },
        { "KSHMR Sub Drop 03 - Long (F).wav", hg::kCatSubDrop, 0 }, { "Downlifter  R .wav", hg::kCatDownlifter, 0 }, { "TPS - Astral - Downfilter 01 (124 BPM).wav", hg::kCatDownlifter, 124 },
        { "White Noise.aif", hg::kCatNoise, 0 }, { "LZ_vocal_whisper_wet_go.wav", hg::kCatVocal, 0 }, { "TPS - Astral - Snare Fill 08 (124 BPM).wav", hg::kCatFill, 124 },
        { "Cymatics - Drum Buildup 27 - 128 BPM.wav", hg::kCatFill, 128 }, { "SM_909_Snare_07.wav", hg::kCatSnare, 0 }, { "TPS - Astral - Tonal Ambience 04 C.wav", hg::kCatAtmos, 0 },
        { "TPS - Astral - Perc Loop 14 (124 BPM).wav", hg::kCatPerc, 124 }, { "Loop_041_-_(PML_MTD_FR_V1)_vox.wav", hg::kCatVocal, 0 }, { "kick anyma.wav", hg::kCatKick, 0 },
        { "Cymatics - Riser 14 - 128 BPM E.wav", hg::kCatRiser, 128 }, { "Chords 01.mid", hg::kCatMidi, 0 } };
    for (auto& c : cases)
    {
        int bpm = -1; const int cat = hg::classifySample (c.n, {}, &bpm, nullptr);
        if (cat != c.cat || bpm != c.bpm) fail (juce::String ("classify: ") + c.n + " -> " + hg::kCatIds[cat] + " " + juce::String (bpm));
    }
    // 3) işlemci: 8:00 Black Coffee iskeleti, organik kurallar
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h;
    const int bc = p.data.styleIndex ("blackcoffee");
    setP (p, "style", (float) bc); p.applyStyleDefaults(); setP (p, "bpm", 120.0f); p.regenerateNow();
    p.skSetStructure (0); p.skSetDuration (480);
    auto sk = p.skeletonNow();
    if (sk.profile != "organic" || sk.song.bars != 240 || sk.secs.size() != 7) fail ("skeleton: blackcoffee 8:00 -> " + sk.profile + " " + juce::String (sk.song.bars));
    const auto styleSong = p.midiBytes (5);
    p.skel.active = true;
    if (p.midiBytes (5) == styleSong) fail ("skeleton: full-song MIDI does not follow the skeleton");
    for (auto& x : sk.secs)
        if (x.kind == "break")
        {
            for (auto& n : sk.song.tracks[hg::kBass]) if (n.s >= x.bar * 16 && n.s < (x.bar + x.bars) * 16) { fail ("organic: bass in breakdown"); break; }
            int kicks = 0;
            for (auto& n : sk.song.tracks[hg::kDrums]) if (n.s >= x.bar * 16 && n.s < (x.bar + x.bars) * 16) { if (n.n != 36 && n.n != 42) { fail ("organic: breakdown drum " + juce::String (n.n)); break; } if (n.n == 36) ++kicks; }
            if (kicks == 0) fail ("organic: kick should stay (thinned) in the breakdown");
        }
    int reverse = 0, thr = 0; for (auto& c : sk.cues) { if (c.type == "reverse") ++reverse; if (c.type == "reverbThrow") ++thr; }
    if (reverse != 6 || thr != 2) fail ("organic cues: reverse " + juce::String (reverse) + " throw " + juce::String (thr));
    // techno: ilk 8 ölçü kicksiz
    setP (p, "style", (float) p.data.styleIndex ("anyma")); p.applyStyleDefaults(); p.regenerateNow(); p.skSetStructure (2); p.skSetDuration (360);
    auto tk = p.skeletonNow();
    if (tk.profile != "techno") fail ("skeleton: anyma profile " + tk.profile);
    for (auto& n : tk.song.tracks[hg::kDrums]) if (n.s < 8 * 16 && n.n != 42) { fail ("techno: kick in the first 8 bars"); break; }
    // 4) sample kütüphanesi: geçici klasör, arka planda tarama
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-sk-test"); dir.deleteRecursively(); dir.getChildFile ("FX").createDirectory();
    juce::Random rnd (5);
    for (auto& c : cases)
    {
        if (c.cat == hg::kCatMidi) { dir.getChildFile (c.n).replaceWithText ("x"); continue; }
        auto f = dir.getChildFile ("FX").getChildFile (juce::String (c.n).replace (".aif", ".wav"));
        juce::AudioBuffer<float> b (1, 22050); for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (0, i, (rnd.nextFloat() - .5f) * .6f);
        juce::WavAudioFormat w; std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream()); std::unique_ptr<juce::AudioFormatWriter> wr (w.createWriterFor (os.get(), 44100, 1, 16, {}, 0));
        if (wr) { os.release(); wr->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
    }
    p.setSampleFolders ({ dir.getFullPathName() }, false);
    for (int i = 0; i < 1000 && ! p.pollLibrary(); ++i) juce::Thread::sleep (10);
    auto L = p.library();
    if (L == nullptr || L->items.size() != std::size (cases)) fail ("library: scanned " + juce::String (L ? (int) L->items.size() : -1));
    else if (L->count (hg::kCatRiser) != 4 || L->count (hg::kCatKick) != 2) fail ("library: riser/kick counts");
    const auto picks = p.skPicks (tk), picks2 = p.skPicks (tk);
    int withFile = 0;
    for (size_t i = 0; i < picks.size(); ++i)
    {
        if (picks[i].second != picks2[i].second) fail ("picks: not deterministic");
        const auto& [c, f] = picks[i];
        if (f == juce::File()) continue;
        ++withFile;
        const int cat = hg::classifySample (f.getFileName(), {}, nullptr, nullptr), want = hg::cueAudio (c.type).cat, fb = hg::cueAudio (c.type).fallback;
        if (cat != want && cat != fb) fail ("picks: " + c.type + " got " + f.getFileName());
        if (c.type == "sweep" && ! f.getFileName().containsIgnoreCase ("sweep")) fail ("picks: sweep prefers a sweep sample, got " + f.getFileName());
    }
    if (withFile < 10) fail ("picks: only " + juce::String (withFile) + " cues got a sample");
    const int drums = p.loadSuggestedDrums();
    if (drums < 5 || p.drumSamplePath (hg::kPadKick).isEmpty()) fail ("suggested drums: loaded " + juce::String (drums));
    // 5) proje klasörü (arka planda)
    auto outProj = dir.getChildFile ("Out Project");
    if (quickRun()) p.exportStems = false;   // sanitizer turu: tam şarkı stem'leri çok yavaş; stem kodu aşağıda kısa şarkıyla sınanır
    {   // stem'ler: 16 ölçülük kısa şarkı (her turda)
        juce::StringArray k { "intro", "drop" }; std::vector<int> b { 8, 8 };
        hg::ExportJobInput in; in.sk = hg::skeleton (p.data, *p.result(), k, 0, &b); in.bpm = in.sk.song.P.bpm;
        p.renderExportSounds (in);
        auto sdir = dir.getChildFile ("short-stems"); juce::StringPairArray files; int made = 0;
        const auto a = hg::buildArrangement (in.sk);
        if (in.stemEngine == nullptr || ! hg::renderStems (a, *in.stemEngine, in.renderRate, sdir, {}, nullptr, 0, 100, files, &made) || made < 3)
            fail ("stems (short): made " + juce::String (made));
        for (auto& f : sdir.getChildFile ("Stems").findChildFiles (juce::File::findFiles, false, "*.wav"))
        {
            juce::AudioFormatManager fm; fm.registerBasicFormats(); std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
            juce::AudioBuffer<float> x (2, (int) r->lengthInSamples); r->read (&x, 0, x.getNumSamples(), 0, true, true);
            if (std::abs ((double) r->lengthInSamples / r->sampleRate - (16 * 240.0 / in.bpm + 2.0)) > .05 || x.getMagnitude (0, x.getNumSamples()) < .01f) fail ("stems (short): " + f.getFileName());
        }
    }
    if (! p.startExport (outProj, "plan")) fail ("export: did not start");
    hg::ExportResult er; bool got = false;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int i = 0; i < 60000 && ! got; ++i) { got = p.pollExport (er); if (! got) juce::Thread::sleep (10); }
    std::cout << "export with stems: " << juce::String ((juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0, 1) << " s for " << tk.song.bars << " bars\n";
    if (! got || ! er.ok) fail ("export: " + er.error);
    else
    {
        for (auto* f : { "MIDI/1 Chords.mid", "MIDI/2 Bass.mid", "MIDI/3 Drums.mid", "MIDI/4 Melody.mid", "Audio/FX Guide.wav", "Plan.txt" })
            if (! outProj.getChildFile (f).existsAsFile()) fail (juce::String ("export: missing ") + f);
        if (outProj.findChildFiles (juce::File::findFiles, false, "*Full Song.mid").size() != 1) fail ("export: full song midi");
        if (er.copied < 8 || er.placed < 10) fail ("export: copied " + juce::String (er.copied) + " placed " + juce::String (er.placed));
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (outProj.getChildFile ("Audio/FX Guide.wav")));
        if (rd == nullptr || std::abs ((double) rd->lengthInSamples - tk.song.bars * 240.0 / tk.song.P.bpm * rd->sampleRate) > 2) fail ("fx guide: length");
        else
        {   // ilk impact'in olduğu ölçüde ses var
            int ib = -1; for (auto& c : tk.cues) if (c.type == "impact") { ib = c.bar; break; }
            juce::AudioBuffer<float> b (2, (int) (rd->sampleRate * .5)); rd->read (&b, 0, b.getNumSamples(), (juce::int64) (ib * 240.0 / tk.song.P.bpm * rd->sampleRate), true, true);
            if (ib < 0 || b.getMagnitude (0, b.getNumSamples()) < .05f) fail ("fx guide: silent at the first impact (bar " + juce::String (ib + 1) + ")");
            if (b.getMagnitude (0, b.getNumSamples()) > 1.0f) fail ("fx guide: clipping");
        }
        if (outProj.getChildFile ("Samples").findChildFiles (juce::File::findFiles, false).size() != er.copied) fail ("export: sample copies");
        checkFlp (er.flp, tk, fail, true);
        checkAls (er.als, tk, fail);
        checkBridges (er, tk, fail);
        checkMixExport (er, outProj, fail);
    }
    // 6) kütüphane olmadan: FX kılavuzu HitGen sesleriyle
    p.setSampleFolders ({}, false);
    if (! p.skPicks (tk).empty() && p.skPicks (tk)[0].second != juce::File()) fail ("picks: library cleared but files still suggested");
    {
        auto g = dir.getChildFile ("synth.wav"); int placed = 0;
        if (! hg::renderFxGuide (g, tk, p.skPicks (tk), tk.song.P.bpm, 44100, 7, {}, nullptr, &placed) || placed < 10) fail ("fx guide (synth): placed " + juce::String (placed));
    }
    // 7) durum: iskelet projeyle saklanır
    p.skSetBars (2, 40);
    juce::MemoryBlock stt; p.getStateInformation (stt);
    {
        auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (stt.getData(), (int) stt.getSize()); q.regenerateNow();
        const auto a = p.skeletonNow(), b = q.skeletonNow();
        if (! q.skel.active || ! q.skel.custom || a.secs.size() != b.secs.size() || b.secs[2].bars != 40 || a.song.bars != b.song.bars) fail ("skeleton: not restored from state");
    }
    // 8) arayüz: Proje sekmesi, üret, bölüm komutları, yerleşim
    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    ed->setTab (5);
    auto* pg = ed->project();
    pg->revealAfterExport = false;
    pg->setStructure (0); pg->setDurationIndex (5);   // 8:00
    pg->generateProject();
    if (! p.skel.active || pg->shownSkeleton().secs.size() != 7 || pg->numListRows() < 14) fail ("ui: generate -> rows " + juce::String (pg->numListRows()));
    const int bars0 = pg->shownSkeleton().song.bars;
    pg->sectionCommand (2, ProjectPage::cmdLonger);
    if (pg->shownSkeleton().song.bars != bars0 + 8 || ! p.skel.custom) fail ("ui: lengthen");
    pg->sectionCommand (3, ProjectPage::cmdDelete);
    if (pg->shownSkeleton().secs.size() != 6) fail ("ui: delete");
    pg->sectionCommand (0, ProjectPage::cmdInsert, 2);
    if (pg->shownSkeleton().secs.size() != 7 || pg->shownSkeleton().secs[1].kind != "drop") fail ("ui: insert");
    pg->sectionCommand (1, ProjectPage::cmdLeft);
    if (pg->shownSkeleton().secs[0].kind != "drop") fail ("ui: move left");
    pg->sectionCommand (0, ProjectPage::cmdType, 0);
    if (pg->shownSkeleton().secs[0].kind != "intro") fail ("ui: change type");
    pg->setStructure (0); pg->setDurationIndex (3);
    if (pg->planText().indexOf ("Buildup 2") < 0) fail ("ui: plan text");
    {   // yerleşim: sayfadaki görünür denetimler üst üste binmez ve sayfa içinde kalır
        std::vector<juce::Component*> vis; for (auto* c : pg->getChildren()) if (c->isVisible()) vis.push_back (c);
        for (size_t i = 0; i < vis.size(); ++i)
        {
            if (! pg->getLocalBounds().contains (vis[i]->getBounds())) fail ("ui: outside page " + vis[i]->getTitle() + " " + vis[i]->getBounds().toString());
            for (size_t j = i + 1; j < vis.size(); ++j)
                if (vis[i]->getBounds().intersects (vis[j]->getBounds())) fail ("ui: overlap '" + vis[i]->getTitle() + "' / '" + vis[j]->getTitle() + "' " + vis[i]->getBounds().toString() + " " + vis[j]->getBounds().toString());
        }
    }
    for (auto l : { "en", "tr", "ar" })
    {
        ed->setLanguage (l); ed->setTab (5); pg->tick();
        snapPng (ed->contentComponent(), outDir.getChildFile (juce::String ("plugin-") + l + "-skeleton.png"));
    }
    ed->setLanguage ("en");
    pg->exportProject (dir.getChildFile ("UI Export"));
    for (int i = 0; i < 60000 && p.exporting(); ++i) { pg->tick(); juce::Thread::sleep (10); }
    pg->tick();
    if (! dir.getChildFile ("UI Export").getChildFile ("Plan.txt").loadFileAsString().contains ("Buildup")) fail ("ui: export plan");
    {   // kütüphane yokken FL projesi: HitGen sesleri ve sentez FX'leri
        auto flps = dir.getChildFile ("UI Export").findChildFiles (juce::File::findFiles, false, "*.flp");
        if (flps.size() != 1) fail ("flp: missing in UI export"); else checkFlp (flps[0], pg->shownSkeleton(), fail, false);
        auto alss = dir.getChildFile ("UI Export").findChildFiles (juce::File::findFiles, false, "*.als");
        if (alss.size() != 1) fail ("als: missing in UI export"); else checkAls (alss[0], pg->shownSkeleton(), fail);
    }
    ed.reset();
    p.setSampleFolders ({}, false);
    for (auto* sub : { "UI Export", "Out Project" })   // incelemek için çıktı klasörüne kopya (FL projesi dahil)
    { auto dst = outDir.getChildFile (juce::String ("flp-") + juce::String (sub).replace (" ", "-")); dst.deleteRecursively(); dir.getChildFile (sub).copyDirectoryTo (dst); }
    dir.deleteRecursively();
    std::cout << "skeleton ok: " << sk.song.bars << " bars organic, " << tk.song.bars << " bars techno, " << withFile << " cues with samples, " << er.placed << " FX placed\n";
}


// HitGen 3.0: Co-Producer (MIDI okuma, ton, rol, uyum araması, kilit) ve Auto Producer (5 tür, tek tık)
static void writeTestMidi (const juce::File& f, const std::vector<hg::NoteEv>& notes, int ch)
{
    juce::MidiFile mf; mf.setTicksPerQuarterNote (96); juce::MidiMessageSequence s;
    s.addEvent (juce::MidiMessage::tempoMetaEvent (500000), 0);
    for (auto& n : notes) { s.addEvent (juce::MidiMessage::noteOn (ch, n.n, (juce::uint8) n.v), n.s * 24.0); s.addEvent (juce::MidiMessage::noteOff (ch, n.n), (n.s + n.l) * 24.0 - 1); }
    s.sort(); s.updateMatchedPairs(); mf.addTrack (s);
    f.deleteFile(); juce::FileOutputStream os (f); mf.writeTo (os, 1);
}

static void testProducer (const juce::File& outDir, const FailFn& fail)
{
    const auto& D = hg::Data::get();
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-co-test"); dir.deleteRecursively(); dir.createDirectory();
    // 1) ton bulma: HitGen'in kendi melodi+akor içeriği bilinen tonlarda
    int exact = 0, close = 0, total = 0, cand = 0;
    for (auto* st : { "anyma", "blackcoffee", "solomun", "tiesto", "fisher", "prog", "indie" })
        for (int key : { 0, 2, 4, 5, 7, 9, 11 })
            for (auto* sc : { "minor", "major" })
            {
                hg::GenParams P; P.style = st; P.key = key; P.scale = sc; P.seed = 11 + key; P.bars = 8; P.bpm = 124;
                const auto r = hg::generate (D, P);
                std::vector<hg::NoteEv> all = r.tracks[hg::kMelody]; all.insert (all.end(), r.tracks[hg::kChords].begin(), r.tracks[hg::kChords].end());
                const auto k = hg::detectKey (all);
                const bool minor = juce::String (sc) == "minor";
                ++total;
                if (k.key == key && k.minor == minor) ++exact;
                const int rel = minor ? (key + 3) % 12 : (key + 9) % 12;   // göreli majör/minör aynı notaları paylaşır
                if ((k.key == key && k.minor == minor) || (k.key == rel && k.minor != minor)) ++close;
                bool inCands = false; for (auto& c : hg::keyCandidates (all, 4)) if (c.key == key && c.minor == minor) inCands = true;
                if (inCands) ++cand;
            }
    // doğru perde kümesi (ton ya da göreli tonu) her durumda, gerçek ton aday listesinde her durumda; tam ton (majör/minör ayrımı) yalnızca raporlanır
    if (close != total || cand != total) fail ("co: key detection pitch set " + juce::String (close) + "/" + juce::String (total) + ", true key in candidates " + juce::String (cand));
    // 2) MIDI okuma + rol tahmini + uyum
    hg::GenParams P; P.style = "anyma"; P.key = 2; P.scale = "minor"; P.seed = 77; P.bars = 8; P.bpm = 124;
    const auto src = hg::generate (D, P);
    auto mel = dir.getChildFile ("melody.mid"), chd = dir.getChildFile ("chords.mid"), bas = dir.getChildFile ("bass.mid");
    writeTestMidi (mel, src.tracks[hg::kMelody], 1); writeTestMidi (chd, src.tracks[hg::kChords], 2); writeTestMidi (bas, src.tracks[hg::kBass], 3);
    hg::UserMidi um; juce::String err;
    if (! hg::readUserMidi (chd, um, err) || um.role != hg::kChords) fail ("co: chords role " + juce::String (um.role) + " " + err);
    if (! hg::readUserMidi (bas, um, err) || um.role != hg::kBass) fail ("co: bass role " + juce::String (um.role) + " " + err);
    if (! hg::readUserMidi (mel, um, err) || um.role != hg::kMelody || um.bars != 8 || std::abs (um.bpm - 120) > .5) fail ("co: melody read role " + juce::String (um.role) + " bars " + juce::String (um.bars) + " bpm " + juce::String (um.bpm) + " " + err);
    const double selfFit = hg::chordFit (src, um.notes, hg::kMelody);
    auto base = P; base.seed = 4242; base.vi = 0;
    const auto cands = hg::coSearch (D, base, um, 12, 5, hg::keyCandidates (um.notes, 4));
    hg::GenParams rnd = base; rnd.seed = 31337; const double rndFit = hg::chordFit (hg::generate (D, rnd), um.notes, hg::kMelody);
    if (cands.size() != 5 || cands[0].fit < rndFit || cands[0].fit < .5 || selfFit < .6) fail ("co: fit best " + juce::String (cands.empty() ? -1 : cands[0].fit, 2) + " random " + juce::String (rndFit, 2) + " self " + juce::String (selfFit, 2));
    // 3) kilit: notalar her yeniden üretimde, stil değişiminde ve proje yüklemesinde aynı
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.regenerateNow();
    p.coLock (um, cands[0]);
    auto sameAsUser = [&] (const hg::Result& r) { const auto want = hg::fitLocked (um.notes, um.bars, r.bars); const auto& got = r.tracks[hg::kMelody]; if (got.size() != want.size()) return false;
                                                   for (size_t i = 0; i < got.size(); ++i) if (got[i].n != want[i].n || std::abs (got[i].s - want[i].s) > 1e-9 || got[i].v != want[i].v) return false; return true; };
    if (p.coLockedRole() != hg::kMelody || ! sameAsUser (*p.result())) fail ("co: lock not applied");
    setP (p, "style", (float) D.styleIndex ("blackcoffee")); p.regenerateNow();
    setP (p, "seed", 999.0f); p.regenerateNow();
    if (! sameAsUser (*p.result())) fail ("co: locked notes changed after style/seed change");
    setP (p, "bars", 2.0f); p.regenerateNow();   // 16 ölçü: 8 ölçülük kullanıcı döngüsü iki kez
    if (p.result()->bars != 16 || ! sameAsUser (*p.result())) fail ("co: lock at 16 bars");
    setP (p, "bars", 1.0f); p.regenerateNow();
    juce::MemoryBlock st; p.getStateInformation (st);
    { auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (st.getData(), (int) st.getSize()); q.regenerateNow();
      if (q.coLockedRole() != hg::kMelody || ! sameAsUser (*q.result())) fail ("co: lock not restored from state"); }
    p.skel.active = true;
    { const auto sk = p.skeletonNow(); int inDrop = 0; for (auto& n : sk.song.tracks[hg::kMelody]) for (auto& x : sk.secs) if (x.kind == "drop" && n.s >= x.bar * 16 && n.s < (x.bar + x.bars) * 16) ++inDrop;
      if (inDrop == 0) fail ("co: user melody missing from the drops of the full song"); }
    p.coUnlock();
    if (p.coLockedRole() >= 0 || sameAsUser (*p.result())) fail ("co: unlock");
    // 4) Auto Producer: her tür tek tıkla
    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    ed->setTab (5);
    for (int gi = 0; gi < 6; ++gi)
    {
        auto panel = ed->project()->makePanel (true);
        auto* ap = dynamic_cast<AutoProducerPanel*> (panel.get());
        ap->genre.setSelectedItemIndex (gi, juce::sendNotificationSync);
        ap->key.setSelectedItemIndex (5, juce::dontSendNotification);
        ap->produce();
        const auto& g = kHgGenres[gi];
        const auto sty = D.styleId ((int) p.apvts.getRawParameterValue ("style")->load());
        const auto sk = p.skeletonNow();
        const double secs = sk.song.bars * 240.0 / sk.song.P.bpm;
        if (sty != g.style || ! p.skel.active || sk.song.P.bpm != g.bpm || sk.song.P.key != 5 || std::abs (secs - g.seconds) > 10.0 || sk.song.tracks[hg::kDrums].empty())
            fail (juce::String ("auto: ") + g.name + " style " + sty + " bpm " + juce::String (sk.song.P.bpm) + " secs " + juce::String (secs, 1));
    }
    // 5) Co-Producer paneli: dosya yükle, tamamla, başka seçenek
    {
        auto panel = ed->project()->makePanel (false);
        auto* cp = dynamic_cast<CoProducerPanel*> (panel.get());
        if (! cp->loadFile (mel) || cp->infoText().isEmpty()) fail ("co ui: load");
        cp->complete();
        const auto first = p.generatorParams().seed;
        cp->next();
        if (p.coLockedRole() != hg::kMelody || p.generatorParams().seed == first && p.generatorParams().vi == cands[0].P.vi) fail ("co ui: complete / next");
        panel->setSize (440, 352); auto img = panel->createComponentSnapshot (panel->getLocalBounds(), true, 1.0f);
        auto f = outDir.getChildFile ("plugin-coproducer.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
        auto pa = ed->project()->makePanel (true); auto img2 = pa->createComponentSnapshot (pa->getLocalBounds(), true, 1.0f);
        auto f2 = outDir.getChildFile ("plugin-autoproducer.png"); f2.deleteFile(); juce::FileOutputStream os2 (f2); png.writeImageToStream (img2, os2);
    }
    snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-en-project3.png"));
    ed.reset(); p.coUnlock();
    dir.deleteRecursively();
    std::cout << "producer ok: key exact " << exact << "/" << total << ", pitch set " << close << ", in candidates " << cand << ", best fit " << juce::String (cands[0].fit, 2) << " vs random " << juce::String (rndFit, 2) << "\n";
}


// çok çıkış: ek bus'lar kapalıyken her şey ana çıkışta; açılınca her iz kendi çıkışında, ana çıkışta kalmaz
// Tech house stili ("Logic Pro tech house" anlatımından): bas 1. adımı kick'e bırakır, açık hat'ler off-beat, kapalı hat'lerde
// off-beat vurgu, hayalet rim, perküsyon fill'leri her seferinde farklı; Arrangement'ta afro teknikleri (Stab, perc inceltme) yok
static void testTechHouse (const FailFn& fail)
{
    HitGenProcessor p; const auto& D = p.data;
    if (D.styleIndex ("techhouse") < 0) { fail ("techhouse: style missing"); return; }
    setP (p, "style", (float) D.styleIndex ("techhouse")); p.applyStyleDefaults(); setP (p, "bars", 2.0f); p.regenerateNow();
    const auto r = p.result(); if (r == nullptr || r->bars < 16) { fail ("techhouse: result"); return; }
    int onKick = 0, badOpen = 0, ghost = 0; double accSum = 0, onSum = 0; int accN = 0, onN = 0;
    for (auto& n : r->tracks[hg::kBass]) if (std::fmod (n.s, 16.0) == 0.0) ++onKick;
    for (auto& n : r->tracks[hg::kDrums])
    {
        const int st = (int) std::fmod (n.s, 16.0);
        if (n.n == 46 && st % 4 != 2) ++badOpen;
        if (n.n == 37 && (st == 7 || st == 15)) ++ghost;
        if (n.n == 42) { if (st % 4 == 2) { accSum += n.v; ++accN; } else if (st % 4 == 0) { onSum += n.v; ++onN; } }
    }
    if (onKick != 0) fail ("techhouse: bass plays on the first 16th of a bar " + juce::String (onKick));
    if (badOpen != 0) fail ("techhouse: open hats must stay on the off-beat, got " + juce::String (badOpen) + " elsewhere");
    if (ghost < 8) fail ("techhouse: ghost rims missing");
    if (accN == 0 || onN == 0 || accSum / accN < onSum / onN + 10) fail ("techhouse: closed hat off-beat accent");
    std::set<juce::String> fills; int nonEmpty = 0;
    for (int b : { 3, 7, 11, 15 })
    {
        juce::String sig; for (auto& n : r->tracks[hg::kDrums]) if (n.n == 64 && (int) (n.s / 16) == b && std::fmod (n.s, 16.0) >= 12.0) sig << (int) n.s << ",";
        if (sig.isNotEmpty()) ++nonEmpty;
        fills.insert (sig);
    }
    if (nonEmpty < 3 || fills.size() < 2) fail ("techhouse: percussion fills must differ (" + juce::String ((int) fills.size()) + " distinct, " + juce::String (nonEmpty) + " non-empty)");
    // afro teknikleri tech house'a uygulanmamalı: Perc şeridi inceltilmemiş, Stab yok
    hg::Skeleton sk = p.skeletonNow(); const auto arr = hg::buildArrangement (sk);
    for (auto& L : arr.lanes) if (L.role == "stab") fail ("techhouse: afro stab lane must not exist");
    std::cout << "techhouse ok: " << r->bars << " bars, fills " << fills.size() << " distinct\n";
}


// ---------------------------------------------------------------- 3.1 fabrika bankası: ön dinleme cümlesiyle çevrimdışı çalma ve ölçüm
struct BankRender { juce::AudioBuffer<float> dry, out; int firstNote = 0, pt = 0; std::vector<int> onsets; };
static int bankPatchIndex (const juce::String& r) { return r.containsChar ('b') ? 1 : r.containsChar ('m') ? 2 : 0; }
static BankRender renderBank (const hg::Data& D, const juce::var& snd, double sr, float movement = 0.0f, float bpm = 124.0f)
{
    BankRender B;
    auto E = std::make_unique<hg::Engine>(); E->prepare (sr, 512);
    E->loudnessOn = false; E->pumpAmt = 0; E->thick = .7f; E->bpm = bpm; E->quality = hg::Quality { 7, 8, true };
    const int pi = bankPatchIndex (snd["r"].toString()), track = pi == 2 ? hg::kMelody : pi; B.pt = pi == 2 ? 3 : pi;
    E->macros[(size_t) B.pt].movement = movement;
    float sp[hg::kNumSp]; hg::spFromSound (snd, sp);   // eklentideki gibi: ses parametrelere açılır, parametrelerden yeniden kurulur
    E->setPatch (pi, hg::Patch::fromVar (hg::spToSound (snd, sp), D, sr));
    static const hg::DemoTable tab;
    const int kind = hg::demoKindFor (snd["cat"].toString(), snd["demo"].toString());
    const double spb = 60.0 / bpm;
    const int pre = 256, total = pre + (int) ((hg::kDemoBeats * spb + 3.0) * sr);
    B.out.setSize (2, total); B.out.clear(); B.dry.setSize (2, total); B.dry.clear();
    struct Ev { int at; hg::DemoNote d; };
    std::vector<Ev> ev; for (int k = 0; k < tab.count[(size_t) kind]; ++k) { const auto& d = tab.notes[(size_t) kind][(size_t) k]; ev.push_back ({ pre + (int) (d.beat * spb * sr), d }); }
    std::stable_sort (ev.begin(), ev.end(), [] (const Ev& a, const Ev& b) { return a.at < b.at; });
    B.firstNote = ev.empty() ? pre : ev.front().at;
    for (auto& e : ev) if (B.onsets.empty() || B.onsets.back() != e.at) B.onsets.push_back (e.at);
    E->auxL[B.pt] = B.dry.getWritePointer (0); E->auxR[B.pt] = B.dry.getWritePointer (1);
    int pos = 0; size_t k = 0;
    auto renderTo = [&] (int until) { while (pos < until) { const int n = std::min (256, until - pos); E->render (B.out, pos, n); pos += n; } };
    renderTo (pre);
    while (pos < total)
    {
        const int next = k < ev.size() ? std::min (total, ev[k].at) : total;
        renderTo (next);
        while (k < ev.size() && ev[k].at <= pos) { const auto& d = ev[k].d; E->noteOn (track, d.note, d.vel, d.len * spb, 0); ++k; }
    }
    return B;
}
struct BankStats { double lufs = -70, peak = 0, dcRatio = 0, sideLow = -99, monoLoss = 0, centroid = 0, width = 0, attack = 0, sustain = 0, clickRatio = 0, upper = 0, motion = 0; std::array<double, 26> prof {}; };
static BankStats bankStats (const BankRender& B, double sr)
{
    BankStats S; const auto& x = B.dry; const int N = x.getNumSamples();
    hg::LoudnessMeter m; m.prepare (sr);
    for (int i = 0; i + 512 <= N; i += 512) m.add (x.getReadPointer (0, i), x.getReadPointer (1, i), 512);
    S.lufs = m.integrated();
    double sum = 0, e = 0, eM = 0, eS = 0, eMono = 0, lpM1 = 0, lpM2 = 0, lpS1 = 0, lpS2 = 0, eML = 0, eSL = 0;
    const double a = std::exp (-2.0 * juce::MathConstants<double>::pi * 120.0 / sr);
    for (int i = 0; i < N; ++i)
    {
        const double L = x.getSample (0, i), R = x.getSample (1, i), M = (L + R) * .5, Sd = (L - R) * .5;
        S.peak = std::max (S.peak, std::max (std::abs (L), std::abs (R)));
        sum += M; e += (L * L + R * R) * .5; eM += M * M; eS += Sd * Sd; eMono += M * M;
        lpM1 = M + (lpM1 - M) * a; lpM2 = lpM1 + (lpM2 - lpM1) * a; lpS1 = Sd + (lpS1 - Sd) * a; lpS2 = lpS1 + (lpS2 - lpS1) * a;
        eML += lpM2 * lpM2; eSL += lpS2 * lpS2;
    }
    const double rms = std::sqrt (e / std::max (1, N));
    S.dcRatio = std::abs (sum / std::max (1, N)) / std::max (1e-9, rms);
    S.sideLow = 10 * std::log10 (std::max (1e-20, eSL) / std::max (1e-20, eML));
    S.monoLoss = 10 * std::log10 (std::max (1e-20, e) / std::max (1e-20, eMono));
    S.width = 10 * std::log10 (std::max (1e-20, eS) / std::max (1e-20, eM));
    // spektral ağırlık merkezi (mono, 4096 FFT, etkin çerçeveler)
    juce::dsp::FFT fft (12); std::vector<float> buf (8192); double num = 0, den = 0, hi = 0;
    for (int pos = 0; pos + 4096 <= N; pos += 2048)
    {
        std::fill (buf.begin(), buf.end(), 0.f);
        for (int i = 0; i < 4096; ++i) buf[(size_t) i] = (x.getSample (0, pos + i) + x.getSample (1, pos + i)) * .5f * (.5f - .5f * std::cos (6.2831853f * i / 4096));
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        for (int k = 1; k < 2048; ++k)
        {
            const double p = (double) buf[(size_t) k] * buf[(size_t) k], f = k * sr / 4096.0; num += p * std::log2 (f); den += p; if (f > 250) hi += p;
            const int b = (int) std::floor (std::log2 (f / 40.0) * 3.0); if (b >= 0 && b < 26) S.prof[(size_t) b] += p;   // 1/3 oktav bantları, 40 Hz'den
        }
    }
    S.centroid = den > 0 ? std::pow (2.0, num / den) : 0;
    S.upper = 10 * std::log10 ((hi + 1e-20) / (den + 1e-20));   // 250 Hz üstü enerji oranı (harmonik içerik)
    for (auto& b : S.prof) b = std::max (-60.0, 10 * std::log10 ((b + 1e-20) / (den + 1e-20)));
    {   // hareket: ilk iki ölçüde 20 ms çerçevelerin seviye değişimi (dB standart sapması, sessiz çerçeveler hariç)
        std::vector<double> fr; const int w = (int) (sr * .02), end = std::min (N, B.firstNote + (int) (sr * 3.5));
        for (int p0 = B.firstNote; p0 + w <= end; p0 += w) { double ee = 0; for (int i = 0; i < w; ++i) { const double v = x.getSample (0, p0 + i); ee += v * v; } const double db = 10 * std::log10 (ee / w + 1e-20); if (db > -70) fr.push_back (db); }
        double mu = 0; for (double d : fr) mu += d; mu /= std::max<size_t> (1, fr.size());
        double var = 0; for (double d : fr) var += (d - mu) * (d - mu); S.motion = std::sqrt (var / std::max<size_t> (1, fr.size()));
    }
    // zarf: ilk notadan sonra tepeye varış süresi ve 2. yarı / 1. yarı enerji (sürdürme)
    {
        const int f = B.firstNote, w = std::min (N - f, (int) (sr * .6)); double pk = 0; int at = 0;
        for (int i = 0; i < w; ++i) { const double v = std::abs (x.getSample (0, f + i)) + std::abs (x.getSample (1, f + i)); if (v > pk) { pk = v; at = i; } }
        S.attack = at / sr;
        double e1 = 0, e2 = 0; const int h = (int) (sr * .12);
        for (int i = 0; i < h && f + 2 * h < N; ++i) { const double v1 = x.getSample (0, f + i), v2 = x.getSample (0, f + h + i); e1 += v1 * v1; e2 += v2 * v2; }
        S.sustain = 10 * std::log10 ((e2 + 1e-20) / (e1 + 1e-20));
    }
    // tıklama: her nota başlangıcındaki ilk 0,3 ms'deki en büyük örnek-örnek sıçrama / o notanın sonraki 20 ms'deki tepe değeri
    for (int on : B.onsets)
    {
        double jump = 0, pk = 1e-6;
        for (int i = on; i < std::min (N - 1, on + (int) (sr * .0003)); ++i) jump = std::max (jump, (double) std::abs (x.getSample (0, i + 1) - x.getSample (0, i)));
        for (int i = on; i < std::min (N, on + (int) (sr * .02)); ++i) pk = std::max (pk, (double) std::abs (x.getSample (0, i)));
        S.clickRatio = std::max (S.clickRatio, jump / pk);
    }
    return S;
}

static void testFactoryBank (const FailFn& fail, bool calibrate)
{
    const auto& D = hg::Data::get(); const double sr = 48000;
    const juce::StringArray bassCats { "Sub", "Rolling", "Acid" };
    std::map<juce::String, std::vector<std::pair<juce::String, BankStats>>> byCat;
    juce::String trims; int n = 0;
    for (int i = 0; i < D.numSounds(); ++i)
    {
        const auto& snd = D.sound (i); const auto id = snd["id"].toString();
        if (snd["bank"].toString() != "wf") continue;
        ++n;
        const auto B = renderBank (D, snd, sr); const auto S = bankStats (B, sr);
        const auto cat = snd["cat"].toString();
        byCat[cat].push_back ({ id, S });
        const double target = cat == "Texture" || cat == "FX" ? -22.0 : -18.0;   // doku ve FX arka planda kalır
        if (calibrate)
        {
            std::printf ("%-20s %-8s lufs %6.1f peak %5.2f dc %.4f sideLow %6.1f monoLoss %5.1f width %6.1f cent %6.0f atk %5.3f sus %6.1f click %.3f up %6.1f mot %5.1f\n",
                         id.toRawUTF8(), cat.toRawUTF8(), S.lufs, S.peak, S.dcRatio, S.sideLow, S.monoLoss, S.width, S.centroid, S.attack, S.sustain, S.clickRatio, S.upper, S.motion);
            trims << "\"" << id << "\":" << juce::String (D.trim (id) * std::pow (10.0, (target - S.lufs) / 20.0), 3) << ",";
            continue;
        }
        if (! std::isfinite (S.lufs) || S.peak <= 0) { fail ("bank: silent or invalid " + id); continue; }
        if (std::abs (S.lufs - target) > 1.5) fail ("bank: loudness " + id + " " + juce::String (S.lufs, 1) + " LUFS (target " + juce::String (target, 0) + ")");
        if (S.peak >= .98) fail ("bank: clipping " + id + " peak " + juce::String (S.peak, 2));
        if (S.dcRatio > .02) fail ("bank: DC offset " + id + " " + juce::String (S.dcRatio, 4));
        if (S.clickRatio > .5) fail ("bank: click at note start " + id + " " + juce::String (S.clickRatio, 3));
        if (bassCats.contains (cat) && snd["r"].toString().containsChar ('b') && S.sideLow > -30) fail ("bank: bass not mono below 120 Hz " + id + " side/mid " + juce::String (S.sideLow, 1) + " dB");
        if (S.monoLoss > 3.0) fail ("bank: mono sum loses " + juce::String (S.monoLoss, 1) + " dB " + id);
    }
    if (calibrate) { std::printf ("TRIMS {%s}\n", trims.dropLastCharacters (1).toRawUTF8()); return; }
    if (n < 50) fail ("bank: only " + juce::String (n) + " factory sounds");
    // çeşitlilik: aynı kategoride iki ses ölçülebilir biçimde farklı olmalı (ağırlık merkezi, genişlik, atak, sürdürme)
    for (auto& [cat, v] : byCat)
        for (size_t a = 0; a < v.size(); ++a)
            for (size_t b = a + 1; b < v.size(); ++b)
            {
                const auto& x = v[a].second; const auto& y = v[b].second;
                const double d = std::abs (std::log2 (x.centroid / y.centroid)) / .5 + std::abs (std::max (-60.0, x.width) - std::max (-60.0, y.width)) / 6.0
                               + std::abs (std::log2 ((x.attack + .005) / (y.attack + .005))) / 1.0 + std::abs (x.sustain - y.sustain) / 6.0
                               + std::abs (x.upper - y.upper) / 6.0 + std::abs (x.motion - y.motion) / 3.0;
                double pd = 0; for (size_t q = 0; q < x.prof.size(); ++q) pd += std::abs (x.prof[q] - y.prof[q]);
                const double dist = d + pd / x.prof.size() / 4.0;   // tını profili: 1/3 oktav bantlarında ortalama 4 dB fark = 1
                if (dist < 1.5) fail ("bank: too similar in " + cat + ": " + v[a].first + " / " + v[b].first + " (" + juce::String (dist, 2) + ")");
                if (juce::SystemStats::getEnvironmentVariable ("HITGEN_BANKDIST", {}).isNotEmpty()) std::printf ("dist %-8s %-18s %-18s %.2f (profile %.2f)\n", cat.toRawUTF8(), v[a].first.toRawUTF8(), v[b].first.toRawUTF8(), dist, pd / x.prof.size() / 4.0);
            }
    std::cout << "factory bank: " << n << " sounds, " << byCat.size() << " categories\n";
}


// 3.1: ses çalma (voice steal) tıklaması, preset değiştirme, Movement makrosu, eski proje uyumu ve ön dinleme
static void testBankBehaviour (const FailFn& fail)
{
    const auto& D = hg::Data::get(); const double sr = 48000;
    auto maxJump = [] (const juce::AudioBuffer<float>& b, int from, int to) { double j = 0; for (int c = 0; c < 2; ++c) for (int i = std::max (1, from); i < to; ++i) j = std::max (j, (double) std::abs (b.getSample (c, i) - b.getSample (c, i - 1))); return j; };
    {   // 1) bas izinde 12 tutulan nota (sınır 8): çalınan sesler kesilmeden 5 ms'de söner
        auto E = std::make_unique<hg::Engine>(); E->prepare (sr, 512); E->loudnessOn = false; E->pumpAmt = 0;
        E->setPatch (1, hg::Patch::fromVar (D.soundById ("wf-sub-anchor"), D, sr));
        juce::AudioBuffer<float> out (2, (int) sr), dry (2, (int) sr); out.clear(); dry.clear();
        E->auxL[1] = dry.getWritePointer (0); E->auxR[1] = dry.getWritePointer (1);
        int pos = 0; E->render (out, 0, 256); pos = 256;
        for (int k = 0; k < 12; ++k) { E->noteOn (hg::kBass, 33 + k, 100, -1, 0); E->render (out, pos, 1440); pos += 1440; }
        E->render (out, pos, (int) sr - pos);
        const double j = maxJump (dry, 300, (int) sr);
        if (j > .05) fail ("voice steal clicks: jump " + juce::String (j, 3));
    }
    {   // 2) tutulan nota sürerken preset değiştirme (eski sesle bitir ve canlı geçiş): sayısal hata ya da patlama yok
        auto E = std::make_unique<hg::Engine>(); E->prepare (sr, 512); E->loudnessOn = false; E->pumpAmt = 0;
        juce::StringArray ids; for (int i = 0; i < D.numSounds(); ++i) if (D.isFactory (D.sound (i)["id"].toString())) ids.add (D.sound (i)["id"].toString());
        juce::AudioBuffer<float> out (2, 512); double pk = 0, jump = 0; float prev[2] {};
        E->setPatch (0, hg::Patch::fromVar (D.soundById (ids[0]), D, sr)); E->render (out, 0, 512);
        for (int k = 0; k < ids.size() * 2; ++k)
        {
            if (k % 3 == 0) { E->noteOn (hg::kChords, 57 + k % 12, 100, -1, 0); E->noteOff (hg::kChords, 57 + (k + 9) % 12, 0); }
            E->setPatch (0, hg::Patch::fromVar (D.soundById (ids[k % ids.size()]), D, sr), k % 2 == 1);
            E->collectGarbage();   // eklentide ileti iş parçacığı bunu zamanlayıcıyla yapar
            for (int b = 0; b < 8; ++b)
            {
                out.clear(); E->render (out, 0, 512);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = out.getSample (c, i); if (! std::isfinite (x)) { fail ("preset switch: NaN"); return; } pk = std::max (pk, (double) std::abs (x)); jump = std::max (jump, (double) std::abs (x - prev[c])); prev[c] = x; }
            }
        }
        if (pk > 1.0) fail ("preset switch: peak " + juce::String (pk, 2));
        if (jump > .6) fail ("preset switch: jump " + juce::String (jump, 2));
        E->collectGarbage();
    }
    {   // 3) Movement makrosu gerçekten hareketi değiştirir (LFO'lu sekans: +1 hareket > -1 hareket)
        const auto lo = bankStats (renderBank (D, D.soundById ("wf-roll-hypno"), sr, -1.0f), sr), hi = bankStats (renderBank (D, D.soundById ("wf-roll-hypno"), sr, 1.0f), sr);
        const auto tlo = bankStats (renderBank (D, D.soundById ("wf-tex-breath"), sr, -1.0f), sr), thi = bankStats (renderBank (D, D.soundById ("wf-tex-breath"), sr, 1.0f), sr);
        if (! (thi.motion > tlo.motion + 2.0)) fail ("movement macro: breath motion " + juce::String (tlo.motion, 1) + " -> " + juce::String (thi.motion, 1));
        if (std::abs (hi.centroid - lo.centroid) < 1 && std::abs (hi.motion - lo.motion) < .5) fail ("movement macro: no audible change on hypno roller");
    }
    {   // 4) eklenti: Movement durumu kaydedilir; Movement'sız eski durum varsayılanla açılır; ön dinleme MIDI çıkışına gitmez
        auto p = std::make_unique<HitGenProcessor>(); FakeHead head; head.playing = false; p->setPlayHead (&head);
        p->setPlayConfigDetails (0, 2, sr, 256); p->prepareToPlay (sr, 256);
        auto* mv = p->apvts.getParameter ("mov_bass");
        if (mv == nullptr) { fail ("movement param missing"); return; }
        mv->setValueNotifyingHost (mv->convertTo0to1 (.5f));
        juce::MemoryBlock mb; p->getStateInformation (mb);
        { auto q = std::make_unique<HitGenProcessor>(); q->setStateInformation (mb.getData(), (int) mb.getSize());
          if (std::abs (q->apvts.getRawParameterValue ("mov_bass")->load() - .5f) > .01f) fail ("movement: state round trip"); }
        if (auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize()))
        {
            std::vector<juce::XmlElement*> drop;
            for (auto* e : xml->getChildIterator()) if (e->getStringAttribute ("id").startsWith ("mov_")) drop.push_back (e);
            std::function<void (juce::XmlElement&)> strip = [&] (juce::XmlElement& x) { for (auto* c = x.getFirstChildElement(); c != nullptr;) { auto* nx = c->getNextElement(); if (c->getStringAttribute ("id").startsWith ("mov_")) x.removeChildElement (c, true); else strip (*c); c = nx; } };
            strip (*xml);
            juce::MemoryBlock old; juce::AudioProcessor::copyXmlToBinary (*xml, old);
            auto q = std::make_unique<HitGenProcessor>(); q->apvts.getParameter ("mov_bass")->setValueNotifyingHost (.9f);
            q->setStateInformation (old.getData(), (int) old.getSize());
            if (std::abs (q->apvts.getRawParameterValue ("mov_bass")->load()) > .01f) fail ("movement: old project without the macro should load neutral, got " + juce::String (q->apvts.getRawParameterValue ("mov_bass")->load(), 2));
        }
        p->apvts.getParameter ("pattern")->setValueNotifyingHost (0.0f);
        p->setSound (hg::kBass, "wf-acid-line", true); p->idle();
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi; double e = 0; bool midiSeen = false;
        p->playDemo (1);
        const int blocks = (int) (sr * 5.0 / 256);
        for (int b = 0; b < blocks; ++b) { buf.clear(); midi.clear(); p->processBlock (buf, midi); if (! midi.isEmpty()) midiSeen = true; for (int i = 0; i < 256; ++i) e += buf.getSample (0, i) * buf.getSample (0, i); }
        if (e < 1.0) fail ("demo: silent (" + juce::String (e, 3) + ")");
        if (p->demoPlaying()) fail ("demo: did not stop after two bars");
        if (midiSeen) fail ("demo: notes leaked to MIDI out");
        p->playDemo (2); for (int b = 0; b < 20; ++b) { buf.clear(); midi.clear(); p->processBlock (buf, midi); }
        p->stopDemo(); for (int b = 0; b < 2; ++b) { buf.clear(); midi.clear(); p->processBlock (buf, midi); }
        if (p->demoPlaying()) fail ("demo: stop ignored");
    }
}


// Ses sayfası görüntüleri ve arayüz davranışı: görünümler, tarayıcı sırası, kategori filtresi, önceki/sonraki
// LoopForge: Öklid ritmi, gam kilidi, kırpma, dikişsiz (başa saran) işleme, yakalama, çalma, dışa aktarım, durum ve arayüz
static void testLoopForge (const juce::File& outDir, const FailFn& fail, bool shots)
{
    const double sr = 48000;
    {   // Öklid ve gam
        const auto e = lf::euclid (16, 4, 0); for (int i = 0; i < 16; ++i) if (e[(size_t) i] != (i % 4 == 0)) { fail ("loop: euclid(16,4) should be four on the floor"); break; }
        int c = 0; const auto r = lf::euclid (8, 3, 2); for (int i = 0; i < 8; ++i) c += r[(size_t) i] ? 1 : 0;
        if (c != 3) fail ("loop: euclid(8,3) pulse count " + juce::String (c));
        if (lf::degreeToSemis (0, 2) != 3 || lf::degreeToSemis (0, 7) != 12 || lf::degreeToSemis (0, -1) != -2 || lf::degreeToSemis (4, 2) != 4) fail ("loop: scale degrees wrong");
    }
    // tek vuruş: 0,1 sn sessizlik + 60 ms sönen 220 Hz + 0,2 sn sessizlik
    juce::AudioBuffer<float> one (2, (int) (sr * .36)); one.clear();
    const int b0 = (int) (sr * .1), bl = (int) (sr * .06);
    for (int i = 0; i < bl; ++i) { const float v = (float) (std::sin (juce::MathConstants<double>::twoPi * 220.0 * i / sr) * std::exp (-i / (sr * .015)) * .8); one.setSample (0, b0 + i, v); one.setSample (1, b0 + i, v); }
    {
        float a = 0, z = 1; lf::autoTrim (one, a, z); const int N = one.getNumSamples();
        if (std::abs (a * N - b0) > 200 || z * N > b0 + bl + 200 || z <= a) fail ("loop: auto trim " + juce::String (a * N) + " .. " + juce::String (z * N));
        juce::AudioBuffer<float> big (2, (int) (sr * 8)); big.clear();
        for (int c = 0; c < 2; ++c) big.copyFrom (c, (int) (sr * 5), one, c, b0, bl);
        int f = 0, t = 0; lf::findHit (big, sr, 2.0, f, t);
        if (std::abs (f - (int) (sr * 5)) > 600 || t <= f || t - f > (int) (sr * .5)) fail ("loop: findHit " + juce::String (f) + " .. " + juce::String (t));
    }
    lf::Settings s; s.bars = 1; s.start = (float) b0 / one.getNumSamples(); s.end = (float) (b0 + bl) / one.getNumSamples(); s.space = 0; s.drive = 0; s.motion = 0;
    for (auto& st : s.step) st = lf::Step();
    s.step[14].on = true; s.step[14].decay = .2f;
    {
        const auto dry = lf::render (s, one, sr, 120, sr); const int N = dry->buf.getNumSamples();
        if (N != (int) (sr * 2)) fail ("loop: 1 bar at 120 BPM should be 2 s, got " + juce::String (N));
        const int head = (int) (sr * .05);
        if (dry->buf.getMagnitude (0, head) > 1e-4f) fail ("loop: dry loop start should be silent");
        if (dry->buf.getMagnitude (0, N) < .05f) fail ("loop: dry loop is silent");
        s.space = 1;
        const auto wet = lf::render (s, one, sr, 120, sr);
        if (wet->buf.getMagnitude (0, head) < 1e-3f) fail ("loop: effect tail does not wrap around to bar 1");
        const float pk = wet->buf.getMagnitude (0, N);
        if (pk > .9f) fail ("loop: peak above -1 dBFS " + juce::String (pk));
        for (int c = 0; c < 2; ++c) if (std::abs (wet->buf.getSample (c, N - 1) - wet->buf.getSample (c, 0)) > .1f * pk) fail ("loop: click at the loop point");
        s.step[14].pitch = 7; s.space = 0;   // bir oktav yukarı: ses yarı sürede biter
        s.step[14].rev = true; s.step[14].ratchet = 3;
        const auto var = lf::render (s, one, sr, 120, sr);
        if (var->buf.getMagnitude (0, N) < .05f) fail ("loop: reverse / ratchet / pitch render is silent");
    }
    {   // durum gidiş-dönüş ve mutasyon
        lf::Settings m = s; lf::mutate (m, lf::mArp, 77); m.swing = .3f; m.steps = 32; m.bars = 4;
        lf::Settings q; q.fromTree (m.toTree());
        bool same = q.steps == 32 && q.bars == 4 && std::abs (q.swing - .3f) < 1e-4f && q.pulses == m.pulses;
        for (int i = 0; i < lf::kMaxSteps; ++i) same = same && q.step[(size_t) i].on == m.step[(size_t) i].on && q.step[(size_t) i].pitch == m.step[(size_t) i].pitch && q.step[(size_t) i].ratchet == m.step[(size_t) i].ratchet;
        if (! same) fail ("loop: settings do not survive save / load");
        int on = 0; for (int i = 0; i < 16; ++i) on += m.step[(size_t) i].on ? 1 : 0;
        if (on != m.pulses || on < 2) fail ("loop: mutate pulses " + juce::String (on));
    }
    // motor: eklentinin kendi çıkışını yakala, çal, dışa aktar
    HitGenProcessor p; p.setPlayConfigDetails (0, 2, sr, 256); p.prepareToPlay (sr, 256);
    auto waitDone = [&] { for (int i = 0; i < 1000 && p.loop.busy(); ++i) juce::Thread::sleep (10); };
    p.idle();   // tempo eklentiden döngü motoruna geçer
    if (p.loop.captureNow()) fail ("loop: capture of silence should fail");
    for (int pos = 0; pos < one.getNumSamples(); pos += 256)
    {
        juce::AudioBuffer<float> blk (2, 256); blk.clear();
        const int n = juce::jmin (256, one.getNumSamples() - pos); for (int c = 0; c < 2; ++c) blk.copyFrom (c, 0, one, c, pos, n);
        p.loop.capture (blk, 256);
    }
    if (! p.loop.captureNow() || ! p.loop.hasSample()) fail ("loop: capture failed");
    p.idle(); waitDone();
    p.loop.setPlaying (true);
    juce::AudioBuffer<float> out (2, 256); float mx = 0;
    for (int i = 0; i < 400; ++i) { out.clear(); p.loop.process (out, 256, false, 0, 120); mx = juce::jmax (mx, out.getMagnitude (0, 256)); }
    if (mx < .01f) fail ("loop: playback is silent");
    {   // ev sahibi konumuna kilit: aynı PPQ aynı örneği verir
        juce::AudioBuffer<float> a (2, 64), b (2, 64); a.clear(); b.clear();
        p.loop.process (a, 64, true, 1.0, 120); p.loop.process (b, 64, true, 1.0 + 8.0, 120);   // 2 ölçü = 8 vuruş sonra
        if (std::abs (a.getSample (0, 10) - b.getSample (0, 10)) > 1e-6f) fail ("loop: not locked to host position");
    }
    const auto wav = p.loop.exportWav();
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    if (std::unique_ptr<juce::AudioFormatReader> rd { fm.createReaderFor (wav) })
    {
        const auto cs = p.loop.get(); const double bpm = (double) p.apvts.getRawParameterValue ("bpm")->load();
        if (rd->bitsPerSample != 24 || rd->numChannels != 2) fail ("loop: export should be 24-bit stereo");
        if (std::abs ((double) rd->lengthInSamples - cs.bars * 4 * 60.0 / bpm * sr) > 2) fail ("loop: export length " + juce::String (rd->lengthInSamples));
    }
    else fail ("loop: export file missing");
    {   // proje durumu: ayarlar ve ses dosyası geri gelir
        auto cs = p.loop.get(); cs.bars = 4; cs.drive = .6f; p.loop.set (cs);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        HitGenProcessor q; q.setPlayConfigDetails (0, 2, sr, 256); q.prepareToPlay (sr, 256); q.setStateInformation (mb.getData(), (int) mb.getSize());
        if (q.loop.get().bars != 4 || std::abs (q.loop.get().drive - .6f) > 1e-4f || ! q.loop.hasSample() || ! q.loop.isPlaying()) fail ("loop: project state not restored");
    }
    // arayüz
    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    ed->setTab (6); auto& lp = ed->loops();
    if (! lp.isVisible()) fail ("loop: page not visible on its tab");
    lp.mutateNow (lf::mArp); lp.clickStep (1, 3, .1f); lp.clickStep (3, 3, 0); lp.clickStep (4, 3, 0);
    const auto us = lp.settings();
    if (! us.step[3].on || us.step[3].pitch < 5 || ! us.step[3].rev || us.step[3].ratchet != 2) fail ("loop: grid edits not applied");
    if (p.loop.get().step[3].pitch != us.step[3].pitch) fail ("loop: page edits did not reach the engine");
    waitDone(); lp.tick();
    if (shots)
    {
        auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
        auto f = outDir.getChildFile ("loop-page.png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
    }
    p.loop.setPlaying (false);
    std::cout << "loopforge: ok\n";
}

static void testSoundPageUi (const juce::File& outDir, const FailFn& fail, bool shots)
{
    HitGenProcessor p; p.setPlayConfigDetails (0, 2, 48000, 256); p.prepareToPlay (48000, 256);
    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    ed->setTab (1); auto& sp = ed->sounds();
    auto shot = [&] (const juce::String& name)
    {
        if (! shots) return;
        sp.tick(); auto img = ed->contentComponent().createComponentSnapshot ({ 0, 0, HitGenEditor::kW, HitGenEditor::kH }, true, 1.0f);
        auto f = outDir.getChildFile ("sound-" + name + ".png"); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat png; png.writeImageToStream (img, os);
    };
    sp.setPart (1); p.setSound (hg::kBass, "wf-acid-line", true); p.idle(); sp.tick();
    shot ("macro-bass");
    // tarayıcı: bas izinde fabrika sesleri önce gelir, yalnızca bas rolündeki sesler listelenir
    const auto ids = sp.browserIds();
    if (ids.isEmpty() || ! p.data.isFactory (ids[0])) fail ("sound browser: factory sounds should come first");
    for (auto& id : ids) if (! p.data.soundById (id)["r"].toString().containsChar ('b')) { fail ("sound browser: non-bass sound in bass list " + id); break; }
    sp.setView (SoundPage::vBrowse); shot ("browse-bass");
    sp.setView (SoundPage::vEdit); shot ("edit-bass");
    sp.setPart (0); p.setSound (hg::kChords, "wf-pad-horizon", true); p.idle(); sp.setView (SoundPage::vMacro); sp.tick(); shot ("macro-chords");
    sp.setPart (2); p.setSound (hg::kMelody, "wf-lead-obsidian", true); p.idle(); sp.tick(); shot ("macro-melody");
    ed->setLanguage ("ar"); sp.tick(); shot ("macro-ar"); ed->setLanguage ("tr"); sp.setView (SoundPage::vBrowse); shot ("browse-tr");
    ed->setLanguage ("en"); sp.setView (SoundPage::vMacro);
    // makro yayları parametreleri gerçekten değiştirir (Hareket = mov_melody)
    p.apvts.getParameter ("mov_melody")->setValueNotifyingHost (p.apvts.getParameter ("mov_melody")->convertTo0to1 (.6f));
    if (std::abs (p.apvts.getRawParameterValue ("mov_melody")->load() - .6f) > .02f) fail ("sound page: movement param not settable");
}


// Hook kuralları (A B A B' motif): tekrar, tek tepe, akor tonunda bitiş, aralık, son ölçüde köke iniş
// Riff ve chop melodileri: ritim her ölçüde (chop: iki ölçüde bir) tekrar eder, riff notaları gamın içindedir; fikir puanı sonlu.
static void testRiffChopRules (const FailFn& fail)
{
    const auto& D = hg::Data::get(); int checked = 0;
    auto onsets = [] (const hg::Result& r, int bar) { std::set<int> o; for (auto& n : r.tracks[hg::kMelody]) if ((int) n.s / 16 == bar) o.insert ((int) n.s % 16); return o; };
    for (int si = 0; si < D.numStyles(); ++si) for (int vi = 0; vi < 4; ++vi)
    {
        const auto& S = D.style (si); const auto& V = S["var"][vi];
        const auto mode = V.hasProperty ("mm") ? V["mm"].toString() : S["mel"]["mode"].toString();
        for (int seed : { 1, 7, 42, 999 })
        {
            hg::GenParams P; P.style = D.styleId (si); P.vi = vi; P.seed = seed; P.key = (int) S["key"]; P.orig = 0; P.human = 0; P.thick = 0;
            const auto r = hg::generate (D, P); const auto tag = P.style + "/" + juce::String (vi) + "/" + juce::String (seed);
            if (! std::isfinite (hg::ideaScore (r))) fail ("idea score not finite: " + tag);
            if (mode != "riff" && mode != "chop") continue;
            ++checked;
            if (onsets (r, 0) != onsets (r, mode == "riff" ? 1 : 2)) fail ("melody rhythm does not repeat: " + tag);
            if (mode == "riff")
            {
                bool in[12] = {}; if (auto* iv = D.scale (r.P.scale)["iv"].getArray()) for (auto& x : *iv) in[((int) x + P.key) % 12] = true;
                for (auto& n : r.tracks[hg::kMelody]) if (! in[n.n % 12]) { fail ("riff note out of scale: " + tag); break; }
            }
        }
    }
    std::cout << "riff/chop rules: " << checked << " cases\n";
}

static void testHookRules (const FailFn& fail)
{
    const auto& D = hg::Data::get(); int checked = 0;
    for (int si = 0; si < D.numStyles(); ++si)
    {
        const auto& S = D.style (si);
        for (int vi = 0; vi < 4; ++vi)
        {
            const auto& V = S["var"][vi];
            const auto mode = V.hasProperty ("mm") ? V["mm"].toString() : S["mel"]["mode"].toString();
            if (mode != "hook") continue;
            for (int seed : { 1, 7, 42, 999, 54321, 2026 })
            {
                hg::GenParams P; P.style = D.styleId (si); P.vi = vi; P.seed = seed; P.bars = 8; P.key = (int) S["key"]; P.thick = 0; P.human = 0;
                const auto r = hg::generate (D, P); ++checked;
                std::array<std::vector<hg::NoteEv>, 8> bar;
                for (auto& n : r.tracks[hg::kMelody]) bar[(size_t) juce::jlimit (0, 7, (int) (n.s / 16))].push_back (n);
                const juce::String tag = P.style + " v" + juce::String (vi) + " s" + juce::String (seed);
                auto sig = [] (const std::vector<hg::NoteEv>& v) { juce::String x; for (auto& n : v) x << std::fmod (n.s, 16.0) << ":" << n.n << " "; return x; };
                if (sig (bar[0]) != sig (bar[4]) || sig (bar[1]) != sig (bar[5])) fail ("hook: bars 1-2 do not repeat in bars 5-6 (" + tag + ")");
                for (int b = 0; b < 8; ++b)
                {
                    if (bar[(size_t) b].empty()) { fail ("hook: empty bar " + juce::String (b + 1) + " (" + tag + ")"); continue; }
                    int lo = 127, hi = 0; for (auto& n : bar[(size_t) b]) { lo = std::min (lo, n.n); hi = std::max (hi, n.n); }
                    if (hi - lo > 14) fail ("hook: bar range " + juce::String (hi - lo) + " semitones (" + tag + ")");
                    int peaks = 0; for (auto& n : bar[(size_t) b]) if (n.v >= 108) { ++peaks; if (n.n != hi && n.n < hi - 2) fail ("hook: accented note is not the peak (" + tag + ")"); }
                    if (peaks != 1) fail ("hook: " + juce::String (peaks) + " peak notes in a bar (" + tag + ")");
                    const auto& c = r.chords[(size_t) b]; const auto& last = bar[(size_t) b].back();
                    bool tone = false; for (int m : c.midi) if ((m - last.n) % 12 == 0) tone = true;
                    if (! tone) fail ("hook: bar " + juce::String (b + 1) + " ends off the chord (" + tag + ")");
                    if (b == 7 && (last.n % 12) != c.pc) fail ("hook: last bar does not land on the root (" + tag + ")");
                }
            }
        }
    }
    if (checked < 50) fail ("hook: only " + juce::String (checked) + " cases");
    {   // yeni fikir: elle seçilmemiş izlere stile uygun kütüphane sesleri, her fikirde değişir; elle seçilen korunur
        auto p = std::make_unique<HitGenProcessor>();
        p->apvts.getParameter ("style")->setValueNotifyingHost (p->apvts.getParameter ("style")->convertTo0to1 ((float) D.styleIndex ("charlotte"))); p->idle();
        p->setSound (hg::kMelody, "wf-lead-ghost", true);
        std::set<juce::String> seen;
        for (int k = 0; k < 6; ++k)
        {
            p->newIdea(); p->idle();
            for (int t : { hg::kChords, hg::kBass })
            {
                const auto id = p->soundId (t); seen.insert (id);
                const auto& v = D.soundById (id);
                if (! D.isFactory (id)) fail ("surprise: not a library sound " + id);
                if (! v["r"].toString().containsChar (t == hg::kBass ? 'b' : 'c')) fail ("surprise: wrong role " + id);
                if (t == hg::kBass && ! juce::StringArray { "Sub", "Rolling", "Acid" }.contains (v["cat"].toString())) fail ("surprise: bass category " + id);
            }
            if (p->soundId (hg::kMelody) != "wf-lead-ghost") fail ("surprise: manual melody sound was replaced");
        }
        if (seen.size() < 5) fail ("surprise: sounds do not change between ideas (" + juce::String ((int) seen.size()) + ")");
    }
    std::cout << "hook rules: " << checked << " cases\n";
}

static void testMultiOut (const FailFn& fail)
{
    auto measure = [&] (bool aux, std::array<double, 5>& rms)
    {
        auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h;
        auto L = p.getBusesLayout();
        if (L.outputBuses.size() != 5) { fail ("multiout: bus count " + juce::String (L.outputBuses.size())); return false; }
        for (int b = 1; b < 5; ++b) L.outputBuses.getReference (b) = aux ? juce::AudioChannelSet::stereo() : juce::AudioChannelSet::disabled();
        if (! p.setBusesLayout (L)) { fail ("multiout: layout rejected"); return false; }
        { auto M = L; M.outputBuses.getReference (0) = juce::AudioChannelSet::mono(); if (aux && p.checkBusesLayoutSupported (M)) fail ("multiout: mono main with aux accepted"); }
        setP (p, "loudness", 0.0f);
        p.regenerateNow();
        const int nch = p.getTotalNumOutputChannels(), sr = 48000, blk = 512;
        if (nch != (aux ? 10 : 2)) fail ("multiout: channels " + juce::String (nch));
        p.setRateAndBufferSizeDetails (sr, blk); p.prepareToPlay (sr, blk);
        FakeHead head; head.bpm = 124; p.setPlayHead (&head);
        juce::AudioBuffer<float> buf (nch, blk); juce::MidiBuffer midi;
        std::array<double, 5> sum {};
        const int blocks = (int) (sr * 8.0 / blk);
        for (int k = 0; k < blocks; ++k)
        {
            midi.clear(); p.processBlock (buf, midi);
            for (int b = 0; b < (aux ? 5 : 1); ++b)
                for (int c = 0; c < 2; ++c) { const float* x = buf.getReadPointer (b * 2 + c); for (int i = 0; i < blk; ++i) { if (! std::isfinite (x[i]) || std::abs (x[i]) > .9f) { fail ("multiout: NaN or over -1 dBFS on bus " + juce::String (b)); break; } sum[(size_t) b] += x[i] * x[i]; } }
            head.ppq += blk * head.bpm / 60.0 / sr;
        }
        for (int b = 0; b < 5; ++b) rms[(size_t) b] = std::sqrt (sum[(size_t) b] / (2.0 * blocks * blk));
        p.setPlayHead (nullptr);
        return true;
    };
    std::array<double, 5> off {}, on {};
    if (! measure (false, off) || ! measure (true, on)) return;
    // açıkken: dört iz kendi çıkışında sesli; ana çıkışta yalnızca reverb/delay dönüşleri kalır (kapalıdakinden çok daha sessiz)
    for (int b = 1; b < 5; ++b) if (on[(size_t) b] < 1e-3) fail ("multiout: bus " + juce::String (b) + " silent");
    if (off[0] < 1e-3 || on[0] > off[0] * .6) fail ("multiout: main " + juce::String (on[0], 4) + " vs " + juce::String (off[0], 4));
    std::cout << "multi-out ok: main " << juce::String (off[0], 3) << " -> " << juce::String (on[0], 3) << ", chords " << juce::String (on[1], 3) << " bass " << juce::String (on[2], 3)
              << " drums " << juce::String (on[3], 3) << " melody " << juce::String (on[4], 3) << "\n";
}

// açılış: ses çalar ve kendiliğinden biter, dışa aktarımda ve atlamada susar; göz gezip yerine oturur
static void testIntro (const juce::File& outDir, const FailFn& fail)
{
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.setRateAndBufferSizeDetails (48000.0, 256); p.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer midi;
    auto block = [&] { buf.clear(); midi.clear(); p.processBlock (buf, midi); return buf.getMagnitude (0, 256); };
    if (! p.intro.start (BinaryData::intro_wav, (size_t) BinaryData::intro_wavSize)) { fail ("intro: decode failed"); return; }
    float pk = 0; int b = 0;
    for (; b < 4000 && (b < 2 || p.intro.playing()); ++b) pk = std::max (pk, block());
    const double secs = b * 256.0 / 48000.0;
    if (pk < 0.05f || pk > 0.8914f) fail ("intro: peak " + juce::String (pk));
    if (p.intro.playing() || secs < 4.0 || secs > 12.0) fail ("intro: length " + juce::String (secs));
    p.setNonRealtime (true); p.intro.start (BinaryData::intro_wav, (size_t) BinaryData::intro_wavSize);
    for (int i = 0; i < 8; ++i) block();
    if (block() > 0.0f || p.intro.playing()) fail ("intro: not silent during offline export");
    p.setNonRealtime (false); p.intro.start (BinaryData::intro_wav, (size_t) BinaryData::intro_wavSize);
    for (int i = 0; i < 40; ++i) block();
    p.intro.stop(); for (int i = 0; i < 8; ++i) block();
    if (block() > 0.0f || p.intro.playing()) fail ("intro: skip did not silence");

    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    ed->startIntro(); auto& ov = ed->introOverlay(); const auto home = ed->eyeHome();
    if (! ov.running()) { fail ("intro: overlay not running"); return; }
    bool moved = false; juce::Rectangle<int> r;
    for (double t = 0; t < 2400; t += 33.0) { ed->advanceIntro (33.0); r = ed->eyeBounds(); moved = moved || r.getCentre().getDistanceFrom (home.getCentre()) > 200; }
    snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-en-intro.png"));
    for (double t = 2400; t < 7000 && ov.running(); t += 33.0) ed->advanceIntro (33.0);
    if (ed->eyeBounds() != home) fail ("intro: eye not back home");
    if (! moved) fail ("intro: eye did not travel");
    if (ov.running() || ov.isVisible()) fail ("intro: overlay did not finish");
    std::cout << "intro ok: " << juce::String (secs, 2) << " s, peak " << juce::String (pk, 3) << "\n";
}

// -14 LUFS: oynatma sırasında kayan entegre ses yüksekliği hedefe oturmalı, tepe -1 dBFS altında kalmalı
static void testLoudness (const juce::File& outDir, const FailFn& fail, bool quick)
{
    for (auto* id : { "hugel", "wavejackers", "guetta", "landry", "blackcoffee", "moblack", "solomun", "monique", "orhan" })
    {
        auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; FakeHead head; p.setPlayHead (&head);
        p.setPlayConfigDetails (0, 2, 48000, 512); p.prepareToPlay (48000, 512);
        setP (p, "style", (float) p.data.styleIndex (id)); p.applyStyleDefaults(); p.regenerateNow(); head.bpm = (double) p.data.style (p.data.styleIndex (id))["bpmDef"];
        const double secs = quick ? 24.0 : 40.0, measure = quick ? 10.0 : 20.0; const int total = (int) (48000 * secs) / 512 * 512, mstart = (int) (48000 * (secs - measure)) / 512 * 512;
        juce::AudioBuffer<float> buf (2, 512), rec (2, total - mstart); rec.clear(); juce::MidiBuffer midi; float pk = 0; bool nan = false;
        for (int pos = 0; pos < total; pos += 512)
        {
            buf.clear(); midi.clear(); p.processBlock (buf, midi); head.ppq += 512 * head.bpm / 60.0 / 48000.0;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float x = buf.getSample (c, i); if (! std::isfinite (x)) nan = true; }
            if (pos >= mstart) { const int n = juce::jmin (512, total - pos); for (int c = 0; c < 2; ++c) rec.copyFrom (c, pos - mstart, buf, c, 0, n); pk = juce::jmax (pk, buf.getMagnitude (0, 512)); }
        }
        hg::LoudnessMeter m; m.prepare (48000); m.add (rec.getReadPointer (0), rec.getReadPointer (1), rec.getNumSamples());
        std::cout << "loudness " << id << " " << juce::String (m.integrated(), 2) << " LUFS (last " << measure << " s), peak " << juce::String (juce::Decibels::gainToDecibels (pk), 2) << " dBFS, gain " << juce::String (p.engine.loudness.gainDb.load(), 1) << " dB\n";
        if (nan) fail ("loudness NaN " + juce::String (id));
        if (std::abs (m.integrated() + 14.0f) > 1.0f) fail ("loudness off target " + juce::String (id) + ": " + juce::String (m.integrated(), 2));
        if (pk > .8914f) fail ("loudness peak above -1 dBFS " + juce::String (id));
        const bool demo = juce::SystemStats::getEnvironmentVariable ("HITGEN_DEMO", {}).isNotEmpty();   // dinleme için WAV (CI'da yazılmaz)
        if (demo)
        {
            auto f = outDir.getChildFile ("demo-" + juce::String (id) + ".wav"); f.deleteFile();
            if (auto os = std::unique_ptr<juce::FileOutputStream> (f.createOutputStream()))
            {
                juce::WavAudioFormat wav;
                if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), 48000, 2, 24, {}, 0))) { os.release(); w->writeFromAudioSampleBuffer (rec, 0, rec.getNumSamples()); }
            }
        }
        if (juce::String (id) == "hugel")
        {
            // master %50: normalleştirmeden sonra uygulanır (yaklaşık -20 LUFS)
            setP (p, "master", .5f); const int t2 = 48000 * 8 / 512 * 512; juce::AudioBuffer<float> r2 (2, t2); r2.clear();
            for (int pos = 0; pos < t2; pos += 512) { buf.clear(); midi.clear(); p.processBlock (buf, midi); head.ppq += 512 * head.bpm / 60.0 / 48000.0; for (int c = 0; c < 2; ++c) r2.copyFrom (c, pos, buf, c, 0, juce::jmin (512, t2 - pos)); }
            hg::LoudnessMeter m2; m2.prepare (48000); m2.add (r2.getReadPointer (0), r2.getReadPointer (1), t2);
            if (std::abs (m2.integrated() + 20.0f) > 1.5f) fail ("loudness with master 50%: " + juce::String (m2.integrated(), 2));
            std::cout << "loudness hugel master 50%: " << juce::String (m2.integrated(), 2) << " LUFS\n";
        }
    }
    // ölçer doğrulaması: 997 Hz, -20 dBFS tepe sinüs (stereo) = -23.0 LUFS civarı (BS.1770 kalibrasyonu: 1 kHz 0 dBFS mono -3.01)
    {
        hg::LoudnessMeter m; m.prepare (48000); juce::AudioBuffer<float> b (2, 48000 * 6);
        for (int i = 0; i < b.getNumSamples(); ++i) { const float x = 0.1f * (float) std::sin (2 * juce::MathConstants<double>::pi * 997.0 * i / 48000.0); b.setSample (0, i, x); b.setSample (1, i, x); }
        m.add (b.getReadPointer (0), b.getReadPointer (1), b.getNumSamples());
        std::cout << "meter check: 997 Hz -20 dBFS stereo sine = " << juce::String (m.integrated(), 2) << " LUFS (expected -20.0 +/- 0.2)\n";
        if (std::abs (m.integrated() + 20.0f) > 0.2f) fail ("loudness meter calibration " + juce::String (m.integrated(), 2));
    }
}

static double zcrOf (const juce::AudioBuffer<float>& b, int from, int n, double sr)
{
    int z = 0; for (int i = from + 1; i < from + n; ++i) if ((b.getSample (0, i - 1) < 0) != (b.getSample (0, i) < 0)) ++z;
    return z / 2.0 / (n / sr);
}

static void testDrumMachine (const juce::File& outDir, const FailFn& fail)
{
    // 1) DSP birimi: her ped sesli ve sonlu; Punch / Soft / Mid / High gerçekten etkili
    {
        hg::DrumMachine dm; dm.prepare (48000); dm.setKit (hg::AnalogKit {});
        auto renderPad = [&dm] (int pad, const hg::PadControls& c, juce::AudioBuffer<float>& out)
        {
            dm.reset(); dm.creative = true; dm.ctl[(size_t) pad] = c; out.setSize (2, 48000 / 2); out.clear();
            dm.hit (hg::kPadNotes[pad], 110); dm.render (out.getWritePointer (0), out.getWritePointer (1), out.getNumSamples());
        };
        auto hfRatio = [] (const juce::AudioBuffer<float>& b, int len = 0) { double e = 0, d = 0; for (int i = 1; i < (len > 0 ? len : b.getNumSamples()); ++i) { const float x = b.getSample (0, i), y = x - b.getSample (0, i - 1); e += x * x; d += y * y; } return e > 0 ? d / e : 0.0; };
        auto energy = [] (const juce::AudioBuffer<float>& b, int a, int n) { double e = 0; for (int i = a; i < a + n && i < b.getNumSamples(); ++i) e += b.getSample (0, i) * b.getSample (0, i); return e; };
        juce::AudioBuffer<float> base, other;
        for (int pad = 0; pad < hg::kNumPads; ++pad)
        {
            renderPad (pad, {}, base); bool finite = true; for (int i = 0; i < base.getNumSamples(); ++i) if (! std::isfinite (base.getSample (0, i))) finite = false;
            if (! finite || base.getMagnitude (0, base.getNumSamples()) < 0.02f) fail ("drum pad silent or NaN: " + juce::String (hg::kPadIds[pad]));
            hg::PadControls c; c.punch = 1.0f; renderPad (pad, c, other);
            if (energy (other, 0, 480) <= energy (base, 0, 480) * 1.15) fail ("punch has no effect on " + juce::String (hg::kPadIds[pad]));
            c = {}; c.soft = 1.0f; renderPad (pad, c, other);
            if (hfRatio (other, 960) >= hfRatio (base, 960) * 0.9) fail ("soft does not soften the attack of " + juce::String (hg::kPadIds[pad]));   // ilk 20 ms: vuruş anı
            c = {}; c.high = 12.0f; renderPad (pad, c, other);
            if (hfRatio (other) <= hfRatio (base) * 1.05) fail ("high EQ has no effect on " + juce::String (hg::kPadIds[pad]));
            c = {}; c.mid = -12.0f; renderPad (pad, c, other);
            if (std::abs (energy (other, 0, 24000) - energy (base, 0, 24000)) < energy (base, 0, 24000) * 0.02) fail ("mid EQ has no effect on " + juce::String (hg::kPadIds[pad]));
            c = {}; c.decay = 0.3f; renderPad (pad, c, other);
            if (energy (other, 4800, 19000) >= energy (base, 4800, 19000) && energy (base, 4800, 19000) > 1e-6) fail ("decay has no effect on " + juce::String (hg::kPadIds[pad]));
        }
        // kick tonu: +12 yarım ses gövde frekansını ikiye katlar
        { hg::PadControls c; renderPad (hg::kPadKick, c, base); c.tune = 12; renderPad (hg::kPadKick, c, other);
          const double z0 = zcrOf (base, 4800, 4800, 48000), z1 = zcrOf (other, 4800, 4800, 48000);
          if (z1 < z0 * 1.6) fail ("kick tune: " + juce::String (z0) + " -> " + juce::String (z1)); }
        // kapalı mod: kontroller yok sayılır
        { hg::PadControls c; c.level = 0.0f; dm.reset(); dm.creative = false; dm.ctl[0] = c; base.setSize (2, 4800); base.clear(); dm.hit (36, 110); dm.render (base.getWritePointer (0), base.getWritePointer (1), 4800);
          if (base.getMagnitude (0, 4800) < 0.05f) fail ("creative off should ignore pad controls"); }
        std::cout << "drum machine DSP ok (7 pads: punch, soft, mid, high, decay, tune)\n";
    }
    // 2) örnek yükleme, ton, durum, kit dosyaları, sıfırlama
    auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-drum-test"); tmp.deleteRecursively(); tmp.createDirectory();
    auto wavFile = tmp.getChildFile ("my kick.wav");
    {
        juce::AudioBuffer<float> s (2, 44100 * 3 / 10);
        for (int i = 0; i < s.getNumSamples(); ++i) { const float x = (i < 200 ? 0.0f : 0.6f) * (float) std::sin (2 * juce::MathConstants<double>::pi * 1000.0 * i / 44100.0) * std::exp (-i / 9000.0f); s.setSample (0, i, x); s.setSample (1, i, x); }
        juce::WavAudioFormat wav; auto os = std::unique_ptr<juce::FileOutputStream> (wavFile.createOutputStream());
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), 44100, 2, 16, {}, 0))) { os.release(); w->writeFromAudioSampleBuffer (s, 0, s.getNumSamples()); }
    }
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; FakeHead head; head.playing = false; p.setPlayHead (&head);
    p.setPlayConfigDetails (0, 2, 48000, 512); p.prepareToPlay (48000, 512);
    setP (p, "loudness", 0); setP (p, "pattern", 0); setP (p, "vol_drums", .5f);
    auto hear = [&p] (int pad, double secs) { juce::AudioBuffer<float> out (2, (int) (48000 * secs)), b (2, 512); juce::MidiBuffer mb; p.engine.drumMachine.audition (pad, 110);
        for (int pos = 0; pos < out.getNumSamples(); pos += 512) { b.clear(); mb.clear(); p.processBlock (b, mb); for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, b, c, 0, juce::jmin (512, out.getNumSamples() - pos)); }
        return out; };
    juce::String err;
    if (p.loadDrumSample (hg::kPadKick, tmp.getChildFile ("missing.wav"), err) || err != "dmNoFile") fail ("missing sample accepted");
    { auto junk = tmp.getChildFile ("junk.wav"); junk.replaceWithText ("not audio"); if (p.loadDrumSample (hg::kPadKick, junk, err)) fail ("junk sample accepted"); }
    if (! p.loadDrumSample (hg::kPadKick, wavFile, err)) fail ("sample load: " + err);
    if (p.apvts.getRawParameterValue ("drumCreative")->load() < .5f) fail ("loading a sample should turn creative mode on");
    { auto out = hear (hg::kPadKick, 0.25); const double z = zcrOf (out, 1000, 6000, 48000); if (z < 850 || z > 1150) fail ("sample pad pitch: " + juce::String (z) + " Hz");
      setP (p, "dm_kick_tune", 12); auto o2 = hear (hg::kPadKick, 0.25); const double z2 = zcrOf (o2, 1000, 4000, 48000); if (z2 < 1800 || z2 > 2200) fail ("sample tune +12: " + juce::String (z2) + " Hz");
      setP (p, "dm_kick_tune", 0); std::cout << "sample pad: " << juce::String (z, 0) << " Hz, +12 st " << juce::String (z2, 0) << " Hz\n"; }
    // klavye: davul sayfası açıkken MIDI notaları pedleri çalar
    { p.auditionPart.store (3); p.engine.drumMachine.lastHit.store (-1); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer mb; mb.addEvent (juce::MidiMessage::noteOn (1, 38, (juce::uint8) 100), 0); p.processBlock (b, mb);
      if (p.engine.drumMachine.lastHit.load() != hg::kPadSnare) fail ("MIDI note 38 did not play the snare pad"); p.auditionPart.store (-1); }
    // durum: örnek yolu projeyle kaydedilir; dosya yoksa bildirilir
    setP (p, "dm_snare_punch", 80); setP (p, "dm_hat_high", 6);
    juce::MemoryBlock mb; p.getStateInformation (mb);
    { auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (mb.getData(), (int) mb.getSize());
      if (q.drumSamplePath (hg::kPadKick) != wavFile.getFullPathName()) fail ("sample path not restored");
      if (std::abs (q.apvts.getRawParameterValue ("dm_snare_punch")->load() - 80) > .5f || std::abs (q.apvts.getRawParameterValue ("dm_hat_high")->load() - 6) > .05f) fail ("pad controls not restored"); }
    { auto moved = tmp.getChildFile ("moved.wav"); wavFile.copyFileTo (moved); juce::String e2; p.loadDrumSample (hg::kPadPerc, moved, e2); juce::MemoryBlock m2; p.getStateInformation (m2); moved.deleteFile();
      auto q_h = std::make_unique<HitGenProcessor>(); auto& q = *q_h; q.setStateInformation (m2.getData(), (int) m2.getSize());
      if (! q.missingSamples.contains ("moved.wav") || q.drumSamplePath (hg::kPadPerc).isNotEmpty()) fail ("missing sample not reported"); p.clearDrumSample (hg::kPadPerc); }
    // kit kaydet / yükle
    juce::File kitFile;
    if (! p.saveDrumKit ("Test Kit 27", kitFile) || ! kitFile.existsAsFile()) fail ("kit save");
    else
    {
        setP (p, "dm_snare_punch", 10); p.clearDrumSample (hg::kPadKick); setP (p, "drumCreative", 0);
        juce::StringArray missing;
        if (! p.loadDrumKit (kitFile, missing) || ! missing.isEmpty()) fail ("kit load");
        if (std::abs (p.apvts.getRawParameterValue ("dm_snare_punch")->load() - 80) > .5f || p.drumSamplePath (hg::kPadKick) != wavFile.getFullPathName() || p.apvts.getRawParameterValue ("drumCreative")->load() < .5f) fail ("kit contents");
        auto junk = tmp.getChildFile ("bad.hgkit"); junk.replaceWithText ("{\"nope\":1}"); if (p.loadDrumKit (junk, missing)) fail ("junk kit accepted");
        kitFile.deleteFile();
    }
    p.resetDrumPad (hg::kPadSnare); if (std::abs (p.apvts.getRawParameterValue ("dm_snare_punch")->load() - 50) > .5f) fail ("pad reset");
    // tüm stiller davul makinesi ile sesli
    std::cout << "drum samples, state and kits ok\n";
    // 3) arayüz: davul sayfası (kapalı / açık + örnek), sağdan sola
    {
        std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
        setP (p, "drumCreative", 0); ed->setLanguage ("en"); ed->setTab (4); ed->drums().tick();
        snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-en-drums.png"));
        setP (p, "drumCreative", 1); ed->setLanguage ("tr"); ed->drums().loadSampleFile (hg::kPadClap, wavFile); ed->drums().tick();
        setP (p, "dm_kick_punch", 85); setP (p, "dm_kick_soft", 20); setP (p, "dm_hat_high", 6);
        snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-tr-drums.png"));
        ed->setLanguage ("ar"); ed->drums().tick(); snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-ar-drums.png"));
        if (p.drumSamplePath (hg::kPadClap).isEmpty()) fail ("drop on pad did not load");
        ed->setLanguage ("en");
    }
    for (int i = 0; i < hg::kNumPads; ++i) p.clearDrumSample (i);
    tmp.deleteRecursively();
    std::cout << "drum machine ui ok\n";
}

static void testGuideAndAssistant (const juce::File& outDir, const FailFn& fail)
{
    auto p_h = std::make_unique<HitGenProcessor>(); auto& p = *p_h; p.regenerateNow();
    std::unique_ptr<HitGenEditor> ed (dynamic_cast<HitGenEditor*> (p.createEditor()));
    // çeviriler: yeni anahtarlar tüm dillerde
    for (auto l : p.data.langCodes)
        for (auto* k : { "navDrums", "dmTitle", "dmCreative", "dmHintOn", "dmPunch", "dmSoftHint", "pLoud", "asName", "as_hook", "gd1", "gd12_t", "gdStep" })
            if (p.data.tr (l, k) == k || p.data.tr (l, k).isEmpty()) fail ("missing translation " + l + ":" + k);
    for (auto l : { "tr", "ar" })
    {
        ed->setLanguage (l); ed->startGuide();
        auto& g = ed->guideOverlay();
        if (! g.isVisible() || g.count() != 12) fail ("guide steps " + juce::String (g.count()));
        for (int s : { 0, 1, 6, 7, 10 }) { g.go (s); snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-" + juce::String (l) + "-guide-" + juce::String (s + 1) + ".png")); }
        g.go (g.count() - 1);
        juce::KeyPress esc (juce::KeyPress::escapeKey); g.keyPressed (esc);
        if (g.isVisible()) fail ("guide did not close");
    }
    ed->setLanguage ("tr"); ed->setTab (0);
    ed->openAssistant();
    if (! ed->assistantBubble().isVisible() || ed->assistantCount() < 3) fail ("assistant suggestions " + juce::String (ed->assistantCount()));
    if (! ed->assistantText().contains (p.data.tr ("tr", "as_play").substring (0, 12))) fail ("assistant first suggestion: " + ed->assistantText());
    snapPng (ed->contentComponent(), outDir.getChildFile ("plugin-tr-assistant.png"));
    // apply: "kısa hook" önerisi uzun melodili varyasyonda çıkar ve kısa motifli varyasyona geçer
    setP (p, "style", (float) p.data.styleIndex ("guetta")); p.applyStyleDefaults(); setP (p, "variation", 0); ed->openAssistant();
    bool hasHook = false; for (int i = 0; i < ed->assistantCount(); ++i) { ed->assistantBubble().nextBtn.triggerClick(); if (ed->assistantText() == p.data.tr ("tr", "as_hook")) { hasHook = true; break; } }
    if (! hasHook) fail ("hook suggestion missing for a long-melody style");
    std::cout << "guide and assistant ok\n";
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    int failures = 0;
    const bool quick = juce::SystemStats::getEnvironmentVariable ("HITGEN_QUICK", {}).isNotEmpty();   // valgrind/sanitizer için kısa tur
    auto fail = [&] (const juce::String& m) { std::cout << "FAIL " << m << "\n"; ++failures; };
    if (juce::SystemStats::getEnvironmentVariable ("HITGEN_DEMO", {}).isNotEmpty()) { testLoudness (outDir, fail, false); return failures == 0 ? 0 : 1; }   // yalnızca dinleme demoları
    if (juce::SystemStats::getEnvironmentVariable ("HITGEN_CALIBRATE", {}).isNotEmpty()) { testFactoryBank (fail, true); return 0; }   // fabrika bankası seviye ayarı
    if (juce::SystemStats::getEnvironmentVariable ("HITGEN_SHOT", {}).isNotEmpty()) { testSoundPageUi (outDir, fail, true); testLoopForge (outDir, fail, true); std::cout << (failures == 0 ? "ALL PASSED\n" : "FAILURES\n"); return failures == 0 ? 0 : 1; }
    if (juce::SystemStats::getEnvironmentVariable ("HITGEN_BANK", {}).isNotEmpty()) { testHookRules (fail); testRiffChopRules (fail); testFactoryBank (fail, false); testBankBehaviour (fail); std::cout << (failures == 0 ? "ALL PASSED\n" : "FAILURES\n"); return failures == 0 ? 0 : 1; }

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
    testIntro (outDir, fail);
    testOwnSoundAndEdit (outDir, fail);
    testSkeleton (outDir, fail);
    testProducer (outDir, fail);
    testTechHouse (fail);
    testMixCore (fail);
    testHookRules (fail); testRiffChopRules (fail);
    testFactoryBank (fail, false);
    testBankBehaviour (fail);
    testSoundPageUi (outDir, fail, false);
    testLoopForge (outDir, fail, false);
    testMultiOut (fail);
    testReference (outDir, fail); testSampler (fail); testLibrary (fail); testMidiLearn (outDir, fail);
    testLoudness (outDir, fail, quick); testDrumMachine (outDir, fail); testGuideAndAssistant (outDir, fail);
    std::cout << (failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string (failures)) << "\n";
    return failures == 0 ? 0 : 1;
}
