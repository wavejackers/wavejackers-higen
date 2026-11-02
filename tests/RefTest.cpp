// Referans ses ve Track DNA testleri: gerçek DSP analizi (sentetik ama ölçülebilir bir parça üzerinde), bozuk dosyalar, iptal, seri hale getirme, determinizm.
#include "../Source/RefAnalysis.h"
#include "../Source/SoundParams.h"
#include <cstdio>
#include <cmath>
#include <chrono>

using namespace hg;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf ("FAIL %s:%d  %s  ", __FILE__, __LINE__, #c); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static const double BPM = 126.0, BAR = 4 * 60.0 / BPM;

// 4 bölüm x 4 ölçü: bas | vurmalı | pad | lead (A minör)
static juce::AudioBuffer<float> makeSong (double sr, int ch, double& secOut)
{
    const double sec = BAR * 16; secOut = sec; const int n = (int) (sec * sr);
    juce::AudioBuffer<float> b (ch, n); b.clear(); juce::Random rnd (42);
    auto put = [&] (int c, int i, float v) { if (i >= 0 && i < n) b.getWritePointer (c)[i] += v; };
    auto midiHz = [] (int m) { return 440.0 * std::pow (2.0, (m - 69) / 12.0); };
    const double beat = 60.0 / BPM;
    // 1) bas: dörtlük notalar, testere benzeri (harmonikler 1/h), tek kanal-merkez
    { const int notes[4] = { 33, 33, 36, 31 };   // A1 A1 C2 G1
      for (int q = 0; q < 16; ++q) { const double t0 = q * beat; const int nn = notes[(q / 2) % 4]; const double f = midiHz (nn); const int len = (int) (beat * 0.9 * sr), s0 = (int) (t0 * sr);
        for (int i = 0; i < len; ++i) { const double t = (double) i / sr; float v = 0; for (int h = 1; h <= 10; ++h) v += (float) (std::sin (2 * juce::MathConstants<double>::pi * f * h * t) / h) * (h * f < 1500 ? 1.0f : 0.0f);
          const float env = (float) std::min (1.0, t / 0.004) * (float) std::exp (-t * 2.0); for (int c = 0; c < ch; ++c) put (c, s0 + i, 0.35f * v * env); } } }
    // 2) vurmalı: 16'lık hi-hat gürültü patlamaları, 2 ve 4'te snare
    { const double base = BAR * 4;
      for (int s = 0; s < 64; ++s) { const double t0 = base + s * beat / 4; const int s0 = (int) (t0 * sr), len = (int) (0.05 * sr); float hp = 0;
        for (int i = 0; i < len; ++i) { const float nz = rnd.nextFloat() * 2 - 1; const float hpv = nz - hp; hp = nz * 0.3f + hp * 0.7f; const float env = std::exp (-(float) i / (0.012f * (float) sr)); for (int c = 0; c < ch; ++c) put (c, s0 + i, 0.25f * hpv * env); } }
      for (int q = 0; q < 16; ++q) if (q % 2 == 1) { const double t0 = base + q * beat; const int s0 = (int) (t0 * sr), len = (int) (0.18 * sr);
        for (int i = 0; i < len; ++i) { const float env = std::exp (-(float) i / (0.045f * (float) sr)); for (int c = 0; c < ch; ++c) put (c, s0 + i, 0.4f * (rnd.nextFloat() * 2 - 1) * env); } } }
    // 3) pad: Am (A3 C4 E4), yavaş atak, L/R farklı detune (geniş)
    { const double base = BAR * 8; const int s0 = (int) (base * sr), len = (int) (BAR * 4 * sr); const int ns[3] = { 57, 60, 64 };
      for (int i = 0; i < len; ++i) { const double t = (double) i / sr; const float env = (float) std::min (1.0, t / 0.8) * (float) std::min (1.0, (BAR * 4 - t) / 0.5);
        for (int c = 0; c < ch; ++c) { float v = 0; const double det = c == 0 ? 0.994 : 1.006; for (int k = 0; k < 3; ++k) { const double f = midiHz (ns[k]) * det; for (int h = 1; h <= 4; ++h) v += (float) (std::sin (2 * juce::MathConstants<double>::pi * f * h * t + c * 0.7) / (h * h)); }
          put (c, s0 + i, 0.12f * v * env); } } }
    // 4) lead: sekizlik pluck, A minör gam, kare dalga (tek harmonikler)
    { const double base = BAR * 12; const int sc[8] = { 69, 72, 76, 74, 72, 76, 79, 76 };
      for (int e = 0; e < 32; ++e) { const double t0 = base + e * beat / 2; const double f = midiHz (sc[e % 8]); const int s0 = (int) (t0 * sr), len = (int) (beat * 0.45 * sr);
        for (int i = 0; i < len; ++i) { const double t = (double) i / sr; float v = 0; for (int h = 1; h <= 9; h += 2) v += (float) (std::sin (2 * juce::MathConstants<double>::pi * f * h * t) / h);
          const float env = (float) std::min (1.0, t / 0.003) * (float) std::exp (-t * 7.0); for (int c = 0; c < ch; ++c) put (c, s0 + i, 0.22f * v * env); } } }
    return b;
}

static juce::File writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::String& ext, const juce::File& dir)
{
    auto f = dir.getChildFile ("song_" + juce::String ((int) sr) + "_" + juce::String (b.getNumChannels()) + ext); f.deleteFile();
    std::unique_ptr<juce::AudioFormat> fmt; if (ext == ".aiff") fmt = std::make_unique<juce::AiffAudioFormat>(); else fmt = std::make_unique<juce::WavAudioFormat>();
    std::unique_ptr<juce::FileOutputStream> os (f.createOutputStream()); std::unique_ptr<juce::AudioFormatWriter> w (fmt->createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0));
    if (w) { os.release(); w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
    return f;
}

static bool centreIn (const RefRegion& r, double a, double b) { const double c = 0.5 * (r.t0 + r.t1); return c >= a && c <= b; }

int main()
{
    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("hitgen-ref-test"); dir.createDirectory();
    double sec = 0; auto song = makeSong (44100.0, 2, sec);
    std::printf ("MP3 desteği derlemede: %s\n", JUCE_USE_MP3AUDIOFORMAT ? "evet" : "hayır");

    // 1) içe aktarma: WAV ve AIFF, farklı örnekleme hızı ve kanal sayısı
    juce::String err; auto wav = writeWav (song, 44100.0, ".wav", dir); auto clip = loadRefAudio (wav, err);
    CHECK (clip != nullptr, "wav yuklenemedi: %s", err.toRawUTF8());
    if (! clip) return 1;
    CHECK (std::abs (clip->seconds() - sec) < 0.01 && clip->audio.getNumChannels() == 2, "sure %.3f", clip->seconds());
    auto aiff = writeWav (song, 44100.0, ".aiff", dir); auto cAiff = loadRefAudio (aiff, err); CHECK (cAiff != nullptr && std::abs (cAiff->seconds() - sec) < 0.01, "aiff");
    double sec48 = 0; auto s48 = makeSong (48000.0, 1, sec48); auto c48 = loadRefAudio (writeWav (s48, 48000.0, ".wav", dir), err); CHECK (c48 && c48->audio.getNumChannels() == 1 && c48->sampleRate == 48000.0, "mono 48k");
    std::printf ("import ok: wav %.2fs, aiff, mono 48k\n", clip->seconds());

    // 2) bozuk / geçersiz dosyalar: çökmemeli, hata anahtarı dönmeli
    { auto bad = dir.getChildFile ("bad.wav"); { juce::Random r (7); juce::MemoryBlock mb (5000); for (size_t i = 0; i < mb.getSize(); ++i) mb[i] = (char) r.nextInt (256); bad.replaceWithData (mb.getData(), mb.getSize()); }
      CHECK (loadRefAudio (bad, err) == nullptr && err.isNotEmpty(), "rastgele bayt dosyasi kabul edildi");
      auto emp = dir.getChildFile ("empty.wav"); emp.replaceWithText (""); CHECK (loadRefAudio (emp, err) == nullptr && err.isNotEmpty(), "bos dosya");
      CHECK (loadRefAudio (dir.getChildFile ("yok.wav"), err) == nullptr && err == "fileMissing", "olmayan dosya");
      juce::MemoryBlock whole; wav.loadFileAsData (whole); auto tr = dir.getChildFile ("trunc.wav"); tr.replaceWithData (whole.getData(), 1000);
      auto ct = loadRefAudio (tr, err); CHECK (ct == nullptr || ct->numSamples() < 1000, "kesik wav");
      juce::AudioBuffer<float> sil (2, 44100); sil.clear(); auto silF = writeWav (sil, 44100.0, ".wav", dir.getChildFile ("..").getChildFile ("hitgen-ref-test")); // sessiz
      silF = dir.getChildFile ("silent.wav"); { auto t = writeWav (sil, 44100.0, ".wav", dir); t.copyFileTo (silF); }
      CHECK (loadRefAudio (silF, err) == nullptr && err == "fileSilent", "sessiz dosya: %s", err.toRawUTF8());
      std::printf ("bozuk dosya testleri ok\n"); }

    // 3) analiz: tempo, ton, bölgeler
    RefAnalysis A; const auto t0 = std::chrono::steady_clock::now();
    CHECK (analyzeRef (*clip, A), "analiz"); const double took = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    std::printf ("analiz %.1fs sure icin %.2fs surdu. BPM %.1f (guven %.2f)  ton %d%s (guven %.2f)  alt %d%s  onset %zu  harm %.2f  bolge %zu\n", sec, took, A.bpm, A.bpmConf, A.key, A.minor ? "m" : "M", A.keyConf, A.keyAlt, A.keyAltMinor ? "m" : "M", A.onsets.size(), A.harmonicRatio, A.regions.size());
    CHECK (A.ok, "ok degil");
    CHECK (std::abs (A.bpm - BPM) < BPM * 0.02, "bpm %.1f", A.bpm);
    const bool keyOk = (A.key == 9 && A.minor) || (A.keyAlt == 9 && A.keyAltMinor) || (A.key == 0 && ! A.minor);
    CHECK (keyOk, "ton %d %d", A.key, (int) A.minor);
    for (int c = 0; c < kNumRefCats; ++c) { auto t = A.top (c, 1); if (t.empty()) { std::printf ("  %s: aday yok\n", refCatKey (c)); continue; } std::printf ("  %s en iyi: %.1f-%.1fs puan %.2f guven %.2f (low %.2f mid %.2f harm %.2f on/sn %.1f)\n", refCatKey (c), t[0].t0, t[0].t1, t[0].score, t[0].confidence, t[0].lowFrac, t[0].midFrac, t[0].harmonic, t[0].onsetsPerSec); }
    auto tb = A.top (catBass, 1), tp = A.top (catPerc, 1), td = A.top (catPad, 1), tl = A.top (catLead, 1);
    CHECK (! tb.empty() && centreIn (tb[0], 0, BAR * 4), "bas bolgesi yanlis");
    CHECK (! tp.empty() && centreIn (tp[0], BAR * 4, BAR * 8), "vurmali bolgesi yanlis");
    CHECK (! td.empty() && centreIn (td[0], BAR * 8, BAR * 12), "pad bolgesi yanlis");
    CHECK (! tl.empty() && centreIn (tl[0], BAR * 12, BAR * 16), "lead bolgesi yanlis");
    for (auto& g : A.regions) CHECK (g.confidence >= 0 && g.confidence <= 1 && g.t1 > g.t0, "gecersiz bolge");
    // başka örnekleme hızı/kanal: aynı sonuca yakın
    RefAnalysis A48; analyzeRef (*c48, A48); CHECK (std::abs (A48.bpm - BPM) < BPM * 0.02, "48k mono bpm %.1f", A48.bpm);

    // 4) iptal
    { std::atomic<bool> stop { true }; RefAnalysis X; const auto s = std::chrono::steady_clock::now(); const bool r = analyzeRef (*clip, X, &stop);
      CHECK (! r, "iptal edilen analiz true dondu"); CHECK (std::chrono::duration<double> (std::chrono::steady_clock::now() - s).count() < 1.0, "iptal gec");
      std::atomic<bool> stop2 { false }; std::atomic<bool> seen { false }; RefAnalysis Y; const bool r2 = analyzeRef (*clip, Y, &stop2, [&] (float p) { if (p > 0.3f) { stop2 = true; seen = true; } });
      CHECK (! r2 && seen, "ilerleme sirasinda iptal"); std::printf ("iptal ok\n"); }

    // 5) DNA: çıkarım, seri hale getirme
    TrackDna D; CHECK (extractDna (*clip, 0.0, BAR * 4, catBass, D), "dna cikarimi");
    std::printf ("bas DNA: f0 %.1f Hz pitched %d conf %.2f  low %.2f sub %.2f  cent %.0f  oddEven %.2f slope %.1f  A %.0fms D %.0fms S %.2f  noise %.2f width %.2f ampMod %.1fHz/%.2f onsets/s %.1f\n", D.f0Hz, D.pitched, D.pitchConf, D.lowFrac, D.subFrac, D.centroidHz, D.oddEven, D.harmonicSlope, D.attackMs, D.decayMs, D.sustainLevel, D.noise, D.width, D.ampModHz, D.ampModDepth, D.onsetsPerSec);
    CHECK (D.pitched && D.f0Hz > 40 && D.f0Hz < 75, "bas f0 %.1f", D.f0Hz); CHECK (D.lowFrac > 0.5f, "bas lowFrac %.2f", D.lowFrac); CHECK (D.width < 0.05f, "bas genislik");
    TrackDna Pd; CHECK (extractDna (*clip, BAR * 8, BAR * 12, catPad, Pd), "pad dna"); CHECK (Pd.width > 0.1f, "pad genisligi olculmedi %.3f", Pd.width); CHECK (Pd.attackMs > 200, "pad atagi %.0f", Pd.attackMs);
    TrackDna Ld; CHECK (extractDna (*clip, BAR * 12, BAR * 16, catLead, Ld), "lead dna"); CHECK (Ld.oddEven > 1.5f, "lead tek harmonik %.2f", Ld.oddEven); CHECK (Ld.onsetsPerSec > 2.0f, "lead onset %.1f", Ld.onsetsPerSec);
    CHECK (Ld.attackMs < Pd.attackMs, "lead atak pad'den uzun");
    { auto js = juce::JSON::toString (D.toVar()); juce::var back = juce::JSON::parse (js); TrackDna R; CHECK (TrackDna::fromVar (back, R), "dna okunamadi");
      CHECK (std::abs (R.centroidHz - D.centroidHz) < 0.01f * D.centroidHz + 1e-3f && R.envDb.size() == 16 && R.pitched == D.pitched && R.role == D.role && std::abs (R.f0Hz - D.f0Hz) < 0.01f, "dna gidis-donus");
      TrackDna Bad; CHECK (! TrackDna::fromVar (juce::JSON::parse ("{\"format\":\"x\"}"), Bad), "yanlis format kabul"); CHECK (! TrackDna::fromVar (juce::JSON::parse ("{\"format\":\"wavejackers-track-dna\",\"version\":99}"), Bad), "gelecek surum kabul");
      std::printf ("dna seri hale getirme ok (%d bayt)\n", js.length()); }
    { TrackDna S; CHECK (! extractDna (*clip, 1.0, 1.05, catLead, S), "cok kisa bolge kabul"); }

    // 6) DNA -> synth: determinizm, aralıklar, benzerlik/varyasyon davranışı
    {
        float a[kNumSp], b[kNumSp], c[kNumSp], d2[kNumSp], e[kNumSp];
        dnaToSoundParams (Ld, 'm', 80, 30, 123, a); dnaToSoundParams (Ld, 'm', 80, 30, 123, b);
        CHECK (std::memcmp (a, b, sizeof a) == 0, "ayni tohum ayni sonucu vermedi");
        dnaToSoundParams (Ld, 'm', 80, 30, 124, c); CHECK (std::memcmp (a, c, sizeof a) != 0, "farkli tohum ayni");
        for (int i = 0; i < kNumSp; ++i) { const auto& f = spDef (i); CHECK (std::isfinite (a[i]) && a[i] >= f.lo - 1e-4f && a[i] <= f.hi + 1e-4f, "aralik disi %s %f", f.key, a[i]); }
        // benzerlik 100 + varyasyon 0: eşlenen değerler tohumdan bağımsız
        dnaToSoundParams (Ld, 'm', 100, 0, 1, d2); dnaToSoundParams (Ld, 'm', 100, 0, 999, e);
        CHECK (std::abs (d2[cut] - e[cut]) < 1e-3f && std::abs (d2[envD] - e[envD]) < 1e-5f && d2[o1w] == e[o1w], "sim100/var0 deterministik hedef degil");
        // kare benzeri (tek harmonik) lead -> kare dalga; kesim DNA'nın rolloff'una yakın
        CHECK ((int) d2[o1w] == 2 || (int) d2[o1w] == 3, "lead dalga %d", (int) d2[o1w]);
        CHECK (d2[envA] < 0.05f, "lead atak %.3f", d2[envA]);
        float p100[kNumSp]; dnaToSoundParams (Pd, 'c', 100, 0, 5, p100); CHECK (p100[envA] > 0.2f && p100[uni] >= 3, "pad: atak %.3f uni %.0f", p100[envA], p100[uni]);
        // varyasyon büyüdükçe sapma artar (ortalama |log(cut)| farkı)
        auto spread = [&] (float var) { double s = 0; for (int sd = 1; sd <= 60; ++sd) { float v[kNumSp]; dnaToSoundParams (Ld, 'm', 100, var, (juce::uint32) sd, v); s += std::abs (std::log (v[cut] / d2[cut])); } return s / 60; };
        const double s0 = spread (0), s30 = spread (30), s100 = spread (100); CHECK (s0 < 1e-3 && s30 > 0.02 && s100 > s30, "varyasyon sapmasi %.4f %.4f %.4f", s0, s30, s100);
        // benzerlik 0: DNA'dan bağımsız (aynı tohumla iki farklı DNA aynı değerleri verir)
        float z1[kNumSp], z2[kNumSp]; dnaToSoundParams (Ld, 'm', 0, 0, 77, z1); dnaToSoundParams (D, 'm', 0, 0, 77, z2); CHECK (std::memcmp (z1, z2, sizeof z1) == 0, "sim0 DNA'dan etkileniyor");
        // 200 üretim: hepsi aralık içinde, en az 150'si farklı
        int uniq = 0; std::vector<std::vector<float>> seen; for (int sd = 0; sd < 200; ++sd) { float v[kNumSp]; dnaToSoundParams (Ld, 'm', 70, 50, (juce::uint32) sd, v); std::vector<float> vv (v, v + kNumSp); bool dup = false; for (auto& s : seen) if (s == vv) dup = true; if (! dup) { ++uniq; seen.push_back (vv); } for (int i = 0; i < kNumSp; ++i) if (! (v[i] >= spDef (i).lo - 1e-4f && v[i] <= spDef (i).hi + 1e-4f)) { CHECK (false, "aralik %d", i); break; } }
        CHECK (uniq >= 190, "benzersiz uretim %d", uniq);
        std::printf ("synth esleme ok: dalga %d, kesim %.0f Hz (DNA rolloff %.0f), atak %.0f ms, benzersiz %d/200, varyasyon sapmasi %.3f/%.3f/%.3f\n", (int) d2[o1w], d2[cut], Ld.rolloffHz, d2[envA] * 1000, uniq, s0, s30, s100);
    }
    std::printf (fails ? "\n%d BASARISIZ\n" : "\nREF TESTLERI GECTI\n", fails);
    return fails ? 1 : 0;
}
