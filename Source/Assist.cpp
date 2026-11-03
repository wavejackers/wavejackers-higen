#include "Assist.h"
#include <cmath>

using namespace juce;

namespace hg
{
bool snapToBars (double a, double b, double bpm, double anchor, double duration, double& outA, double& outB)
{
    if (! (bpm > 20.0 && bpm < 400.0) || duration <= 0) return false;
    if (b < a) std::swap (a, b);
    const double bar = 4.0 * 60.0 / bpm;
    if (bar > duration) return false;
    auto snap = [&] (double t) { return anchor + std::round ((t - anchor) / bar) * bar; };
    double sa = snap (a), sb = snap (b);
    if (sb - sa < bar - 1e-9) sb = sa + bar;                          // en az bir bar
    // dosyanın içinde kal: başı negatifse bir bar ileri, sonu taşarsa bar sınırında geri çek
    while (sa < -1e-9) { sa += bar; sb += bar; }
    while (sb > duration + 1e-9 && sb - bar >= sa + bar - 1e-9) sb -= bar;
    if (sb > duration + 1e-9) { const double len = sb - sa; sb = std::floor ((duration - anchor) / bar) * bar + anchor; sa = sb - len; if (sa < -1e-9) return false; }
    outA = jmax (0.0, sa); outB = jmin (duration, sb);
    return outB > outA + 1e-6;
}

StringArray dnaTags (const TrackDna& d)
{
    StringArray t;
    if (d.brightness < 0.30f) t.add ("tagDark"); else if (d.brightness > 0.65f) t.add ("tagBright");
    if (d.warmth > 0.60f) t.add ("tagWarm");
    if (d.attackMs > 0 && d.attackMs < 15.0f && d.sustainLevel < 0.35f) t.add ("tagPluck");
    if (d.attackMs > 80.0f && d.sustainLevel > 0.55f) t.add ("tagSoft");
    if (d.width > 0.40f) t.add ("tagWide");
    if (d.noise > 0.40f) t.add ("tagNoisy");
    if (d.ampModDepth > 0.25f || d.brightModDepth > 0.25f) t.add ("tagMoving");
    if (d.subFrac > 0.35f) t.add ("tagSub");
    t.add (d.pitched ? "tagPitched" : "tagUnpitched");
    return t;
}

bool onsetsToMidi (const RefClip& clip, const RefAnalysis& an, double t0, double t1, RhythmSketch& out)
{
    out = {}; if (clip.numSamples() == 0 || ! an.ok) return false;
    const double sr = clip.sampleRate, dur = clip.seconds();
    if (t1 <= t0 + 0.05) { t0 = 0; t1 = dur; }
    t0 = jlimit (0.0, dur, t0); t1 = jlimit (0.0, dur, t1);
    const bool grid = an.bpm > 20.0; out.bpm = grid ? an.bpm : 120.0;
    const double spb = 60.0 / out.bpm; constexpr int ppq = 960, sixteenth = ppq / 4;
    const float* L = clip.audio.getReadPointer (0); const float* R = clip.audio.getNumChannels() > 1 ? clip.audio.getReadPointer (1) : L; const int n = clip.numSamples();
    // dosyanın en yüksek tepe seviyesi: hızlar buna göre ölçeklenir
    float top = 1e-6f; for (int i = 0; i < n; i += 4) top = jmax (top, std::abs (0.5f * (L[i] + R[i])));
    // basit bantlar: tek kutuplu alçak geçiren (~150 Hz) ve birinci fark (yüksek frekans)
    const float lpA = (float) std::exp (-MathConstants<double>::twoPi * 150.0 / sr);
    MidiMessageSequence seq;
    seq.addEvent (MidiMessage::tempoMetaEvent (roundToInt (spb * 1.0e6)), 0); seq.addEvent (MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    std::vector<std::pair<int, int>> used;                            // (tick, nota) yinelenmesin
    for (float ot : an.onsets)
    {
        if (ot < t0 - 1e-6 || ot >= t1) continue;
        // onset zamanı analiz adımı kadar kayabilir: önce 30 ms içindeki tepeyi bul, pencereleri tepeden başlat.
        // 0-40 ms: bant oranı; 0-15 ms ile 15-60 ms enerji oranı: sönüm hızı (hi-hat kısa, snare uzun)
        int a = jlimit (0, n - 1, (int) (ot * sr - 0.005 * sr)); float pk = 0;
        { int best = a; const int lim = jmin (n, a + (int) (0.03 * sr)); for (int i = a; i < lim; ++i) { const float v = std::abs (0.5f * (L[i] + R[i])); if (v > pk) { pk = v; best = i; } } a = best; }
        const int e = jmin (n, a + (int) (0.04 * sr)), eMid = jmin (n, a + (int) (0.015 * sr)), eEnd = jmin (n, a + (int) (0.06 * sr));
        double eAll = 0, eLow = 0, eEarly = 0, eLate = 0; float lp = 0;
        for (int i = jmax (0, a - (int) (0.01 * sr)); i < a; ++i) lp = lp * lpA + 0.5f * (L[i] + R[i]) * (1.0f - lpA);   // filtreyi ısıt
        for (int i = a; i < eEnd; ++i)
        {
            const float v = 0.5f * (L[i] + R[i]); lp = lp * lpA + v * (1.0f - lpA);
            if (i < e) { eAll += (double) v * v; eLow += (double) lp * lp; }
            (i < eMid ? eEarly : eLate) += (double) v * v;
        }
        if (eAll <= 1e-12 || eEarly <= 1e-12) continue;
        const double lowR = eLow / eAll, decay = eLate / eEarly;
        const int note = lowR > 0.45 ? 36 : decay < 0.35 ? 42 : 38;
        const float db = Decibels::gainToDecibels (pk / top, -60.0f);
        const int vel = jlimit (20, 127, roundToInt (127.0f + db * 3.2f));   // 0 dB -> 127, -30 dB -> ~31
        double beats = (ot - t0) / spb; if (grid) beats = std::round (beats * 4.0) / 4.0;
        const int tick = jmax (0, roundToInt (beats * ppq));
        if (std::find (used.begin(), used.end(), std::make_pair (tick, note)) != used.end()) continue;
        used.push_back ({ tick, note });
        seq.addEvent (MidiMessage::noteOn (10, note, (uint8) vel), tick); seq.addEvent (MidiMessage::noteOff (10, note), tick + sixteenth / 2);
        ++out.notes; if (note == 36) ++out.kicks; else if (note == 38) ++out.snares; else ++out.hats;
    }
    seq.updateMatchedPairs(); seq.sort();
    out.midi.setTicksPerQuarterNote (ppq); out.midi.addTrack (seq);
    return out.notes > 0;
}

int analyseFileToDna (const File& f, std::vector<TrackDna>& out, String& error, const std::atomic<bool>* cancel, float minConf)
{
    out.clear(); error = {};
    auto clip = loadRefAudio (f, error, cancel); if (clip == nullptr) { if (error.isEmpty()) error = "fileBad"; return 0; }
    RefAnalysis an; if (! analyzeRef (*clip, an, cancel) || ! an.ok) { error = "rfDnaFail"; return 0; }
    for (int k = 0; k < kNumRefCats; ++k)
    {
        if (cancel != nullptr && cancel->load()) break;
        const auto best = an.top (k, 1); if (best.empty() || best[0].confidence < minConf) continue;
        TrackDna d; if (! extractDna (*clip, best[0].t0, best[0].t1, k, d, cancel)) continue;
        d.name = f.getFileNameWithoutExtension() + " " + String (refCatKey (k)); out.push_back (d);
    }
    return (int) out.size();
}

Array<File> BatchAnalyser::audioFilesIn (const File& folder, int maxFiles)
{
    Array<File> r;
    for (auto& e : RangedDirectoryIterator (folder, false, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3", File::findFiles))
    {
        if (r.size() >= maxFiles) break;
        r.add (e.getFile());
    }
    r.sort();
    return r;
}

BatchAnalyser::~BatchAnalyser() { *alive = false; cancel(); }

bool BatchAnalyser::start (const Array<File>& files)
{
    if (isThreadRunning() || files.isEmpty()) return false;
    list = files; stop = false; startThread (Thread::Priority::low); return true;
}

void BatchAnalyser::run()
{
    const int total = list.size(); auto al = alive;
    for (int i = 0; i < total && ! stop.load() && ! threadShouldExit(); ++i)
    {
        std::vector<TrackDna> dnas; String err; const auto f = list[i];
        analyseFileToDna (f, dnas, err, &stop);
        if (stop.load()) break;
        MessageManager::callAsync ([this, al, i, total, f, dnas, err] { if (al->load() && onFile) onFile (i, total, f, dnas, err); });
    }
    const bool cancelled = stop.load();
    MessageManager::callAsync ([this, al, cancelled] { if (al->load() && onDone) onDone (cancelled); });
}
}
