#include "SampleLib.h"
#include "AutoMix.h"
#include "AlsWriter.h"
#include "FlpWriter.h"
#include "Arrangement.h"
#include "Synth.h"
#include "Data.h"
#include <map>
#include <memory>

namespace hg
{
const char* const kCatIds[kNumCats] = { "kick", "clap", "snare", "hat", "ohat", "shaker", "perc", "ride", "crash",
                                        "riser", "downlifter", "impact", "subdrop", "noise", "fill", "vocal", "atmos", "other", "midi" };

namespace
{
juce::String norm (const juce::String& s)
{
    auto t = s.toLowerCase();
    for (auto c : juce::String ("_-.()[]{}#,+")) t = t.replace (juce::String::charToString (c), " ");
    while (t.contains ("  ")) t = t.replace ("  ", " ");
    return " " + t.trim() + " ";
}
bool any (const juce::String& n, std::initializer_list<const char*> keys)
{
    for (auto* k : keys) if (n.contains (k)) return true;
    return false;
}
int catOf (const juce::String& n)
{
    if (any (n, { "sub drop", "subdrop", "808 drop", "sub boom" })) return kCatSubDrop;
    if (any (n, { "downlifter", "downfilter", "down lifter", "downsweep", "sweep down", "down sweep", "falling", " fall " })) return kCatDownlifter;
    if (any (n, { "uplifter", "riser", " rise", "uplift", "sweep up", "upsweep", "sweepup", " sweep", "transition", "whoosh" })) return kCatRiser;
    if (any (n, { "noise" })) return kCatNoise;
    if (any (n, { "impact", " boom", "slam", "hit fx" })) return kCatImpact;
    if (any (n, { "fill", " roll", "drum build", "buildup", "build up", "snare build" })) return kCatFill;
    if (any (n, { "vocal", " vox", "voice", "whisper", "chant", "acapella", "speech", " shout" })) return kCatVocal;
    if (any (n, { "ambience", "ambient", "atmos", "texture", "drone", " pad " })) return kCatAtmos;
    if (any (n, { "kick", " kik", " bd ", "bassdrum", "bass drum" })) return kCatKick;
    if (any (n, { "clap" })) return kCatClap;
    if (any (n, { "snare", " snr" })) return kCatSnare;
    if (any (n, { "open hat", "openhat", " ohat", " oh " })) return kCatOpenHat;
    if (any (n, { "hat", " hh ", "hihat" })) return kCatHat;
    if (any (n, { "shaker", "shake", "tamb" })) return kCatShaker;
    if (any (n, { " ride" })) return kCatRide;
    if (any (n, { "crash", "cymbal" })) return kCatCrash;
    if (any (n, { "perc", "conga", "bongo", " rim", " tom", "clave", "cowbell", "djembe", "shaker", "block" })) return kCatPerc;
    return kCatOther;
}
int bpmOf (const juce::String& n)
{
    // "124 bpm", "124bpm"
    for (int at = n.indexOf ("bpm"); at >= 0; at = n.indexOf (at + 3, "bpm"))
    {
        int e = at; while (e > 0 && n[e - 1] == ' ') --e;
        int b = e; while (b > 0 && juce::CharacterFunctions::isDigit (n[b - 1])) --b;
        if (e - b >= 2 && e - b <= 3) { const int v = n.substring (b, e).getIntValue(); if (v >= 60 && v <= 200) return v; }
    }
    // " fx 119 16bars " gibi: "bars" sözcüğü olan adlarda 3 haneli sayı
    if (n.contains ("bars") || n.contains ("loop"))
    {
        juce::StringArray tok; tok.addTokens (n, " ", "");
        for (auto& t : tok) if (t.length() == 3 && t.containsOnly ("0123456789")) { const int v = t.getIntValue(); if (v >= 70 && v <= 180) return v; }
    }
    return 0;
}
juce::uint32 mix (juce::uint32 h)
{
    h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;
    return h;
}
juce::uint32 strHash (const juce::String& s) { juce::uint32 h = 2166136261u; for (auto c : s) { h ^= (juce::uint32) c; h *= 16777619u; } return h; }
}

int classifySample (const juce::String& fileName, const juce::String& folderHint, int* bpmOut, bool* loopOut)
{
    const auto ext = juce::File::createFileWithoutCheckingPath ("/" + fileName).getFileExtension().toLowerCase();
    const auto base = fileName.upToLastOccurrenceOf (".", false, false);
    const auto n = norm (base.isEmpty() ? fileName : base);
    const int bpm = bpmOf (n);
    if (bpmOut != nullptr) *bpmOut = bpm;
    if (loopOut != nullptr) *loopOut = n.contains ("loop") || n.contains ("bars") || bpm > 0;
    if (ext == ".mid" || ext == ".midi") return kCatMidi;
    int c = catOf (n);
    if (c == kCatOther && folderHint.isNotEmpty()) c = catOf (norm (folderHint));
    return c;
}

CueAudio cueAudio (const juce::String& t)
{
    CueAudio a;
    auto set = [&a] (int cat, int fb, const char* pref, bool end, bool rev, bool render) { a.cat = cat; a.fallback = fb; a.pref = pref; a.endAligned = end; a.reversed = rev; a.render = render; };
    if (t == "reverse") set (kCatDownlifter, kCatNoise, "down", true, true, true);
    else if (t == "downlifter") set (kCatDownlifter, kCatNoise, "", false, false, true);
    else if (t == "noise") set (kCatNoise, kCatRiser, "", true, false, true);
    else if (t == "riser") set (kCatRiser, -1, "rise", true, false, true);
    else if (t == "sweep") set (kCatRiser, -1, "sweep", true, false, true);
    else if (t == "uplifter") set (kCatRiser, -1, "uplift", true, false, true);
    else if (t == "impact") set (kCatImpact, kCatCrash, "", false, false, true);
    else if (t == "subDrop") set (kCatSubDrop, kCatImpact, "", false, false, true);
    else if (t == "vocal") set (kCatVocal, -1, "", false, false, true);
    else if (t == "fill") set (kCatFill, kCatSnare, "fill", true, false, true);
    else if (t == "drumBuild") set (kCatFill, -1, "build", true, false, true);
    else if (t == "atmos") set (kCatAtmos, -1, "", false, false, false);
    else if (t == "kickIn") set (kCatKick, -1, "", false, false, false);
    else if (t == "perc") set (kCatShaker, kCatPerc, "", false, false, false);
    else if (t == "percUp") set (kCatPerc, kCatShaker, "", false, false, false);
    else if (t == "hatsOnly") set (kCatHat, kCatOpenHat, "loop", false, false, false);
    else if (t == "clapIn") set (kCatClap, kCatSnare, "", false, false, false);
    else if (t == "roll") set (kCatSnare, kCatClap, "", false, false, false);
    return a;
}

int SampleLib::count (int cat) const
{
    int n = 0; for (auto& e : items) if (e.cat == cat) ++n;
    return n;
}

const SampleEntry* SampleLib::pickCat (int cat, const char* pref, int bpm, juce::uint32 seed, int salt, bool oneShot) const
{
    std::vector<const SampleEntry*> c;
    for (auto& e : items) if (e.cat == cat) c.push_back (&e);
    if (c.empty()) return nullptr;
    auto keepIf = [&c] (auto pred) { std::vector<const SampleEntry*> k; for (auto* e : c) if (pred (*e)) k.push_back (e); if (! k.empty()) c.swap (k); };
    if (pref != nullptr && *pref != 0) { const juce::String p (pref); keepIf ([&p] (const SampleEntry& e) { return e.lname.contains (p); }); }
    if (oneShot) keepIf ([] (const SampleEntry& e) { return ! e.loop && e.size < 4 * 1024 * 1024; });
    keepIf ([bpm] (const SampleEntry& e) { return e.bpm > 0 && std::abs (e.bpm - bpm) <= 4; });   // tempoya yakın döngü varsa onu
    keepIf ([bpm] (const SampleEntry& e) { return e.bpm == 0 || std::abs (e.bpm - bpm) <= 4; });  // uzak tempolu döngülerden kaçın
    const juce::uint32 h = mix (seed * 2654435761u ^ (juce::uint32) salt * 40503u ^ (juce::uint32) cat * 2246822519u);
    return c[(size_t) (h % (juce::uint32) c.size())];
}

const SampleEntry* SampleLib::pick (const juce::String& cueType, int bpm, juce::uint32 seed, int bar) const
{
    const auto a = cueAudio (cueType);
    if (a.cat < 0) return nullptr;
    const bool oneShot = a.cat <= kCatCrash && cueType != "hatsOnly";
    const int salt = bar * 7919 + (int) (strHash (cueType) & 0xffff);
    if (auto* e = pickCat (a.cat, a.pref, bpm, seed, salt, oneShot)) return e;
    return a.fallback >= 0 ? pickCat (a.fallback, "", bpm, seed, salt, oneShot) : nullptr;
}

SampleLib SampleLib::scan (const juce::StringArray& folders, const std::function<bool()>& shouldExit, std::atomic<int>* progress, int maxFiles)
{
    SampleLib L; L.folders = folders;
    const juce::String exts ("*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.mid;*.midi");
    int seen = 0;
    for (auto& f : folders)
    {
        const juce::File dir (f);
        if (! dir.isDirectory()) continue;
        for (const auto& entry : juce::RangedDirectoryIterator (dir, true, exts, juce::File::findFiles))
        {
            if ((++seen & 127) == 0) { if (shouldExit && shouldExit()) return L; if (progress) progress->store (seen); }
            const auto file = entry.getFile();
            if (file.isHidden() || file.getFileName().startsWithChar ('.')) continue;
            SampleEntry e; e.file = file; e.size = entry.getFileSize();
            e.cat = classifySample (file.getFileName(), file.getParentDirectory().getFileName(), &e.bpm, &e.loop);
            e.lname = norm (file.getFileNameWithoutExtension());
            L.items.push_back (std::move (e));
            if ((int) L.items.size() >= maxFiles) break;
        }
        if ((int) L.items.size() >= maxFiles) break;
    }
    std::sort (L.items.begin(), L.items.end(), [] (const SampleEntry& a, const SampleEntry& b) { return a.file.getFullPathName().compareNatural (b.file.getFullPathName()) < 0; });
    if (progress) progress->store (seen);
    return L;
}

Result sliceResult (const Result& song, int bar, int bars)
{
    Result r; r.P = song.P;
    bar = juce::jlimit (0, juce::jmax (0, song.bars), bar); bars = juce::jlimit (0, song.bars - bar, bars);
    const double a = bar * 16.0, b = (bar + bars) * 16.0;
    for (int k = 0; k < kNumTracks; ++k)
        for (auto n : song.tracks[(size_t) k])
            if (n.s >= a && n.s < b) { n.s -= a; n.l = std::min (n.l, b - (n.s + a)); r.tracks[(size_t) k].push_back (n); }
    for (int i = bar; i < bar + bars && i < (int) song.chords.size(); ++i) r.chords.push_back (song.chords[(size_t) i]);
    for (auto& s : song.sections) if (s.bar >= bar && s.bar < bar + bars) r.sections.push_back ({ s.name, s.bar - bar, std::min (s.bars, bar + bars - s.bar) });
    r.bars = bars; r.steps = bars * 16;
    return r;
}

// ---------------------------------------------------------------- FX kılavuzu
namespace
{
using Buf = std::shared_ptr<juce::AudioBuffer<float>>;

Buf loadAudio (juce::AudioFormatManager& fm, const juce::File& f, double sr)
{
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
    if (rd == nullptr || rd->sampleRate <= 0 || rd->lengthInSamples <= 0) return nullptr;
    const int n = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 45));
    juce::AudioBuffer<float> in (2, n);
    rd->read (&in, 0, n, 0, true, true);
    if (rd->numChannels == 1) in.copyFrom (1, 0, in, 0, 0, n);
    const double ratio = rd->sampleRate / sr;
    const int m = (int) std::floor ((n - 1) / ratio) + 1;
    auto out = std::make_shared<juce::AudioBuffer<float>> (2, juce::jmax (1, m));
    for (int c = 0; c < 2; ++c)
    {
        const float* s = in.getReadPointer (c); float* d = out->getWritePointer (c);
        for (int i = 0; i < m; ++i)
        {
            const double p = i * ratio; const int i0 = (int) p; const int i1 = juce::jmin (n - 1, i0 + 1); const float fr = (float) (p - i0);
            d[i] = s[i0] + (s[i1] - s[i0]) * fr;
        }
    }
    return out;
}

// Sample yoksa HitGen sesi: gürültü yükselişi/inişi, impact ve sub drop
Buf synth (const juce::String& type, int frames, double sr, juce::uint32 seed)
{
    frames = juce::jlimit (1, (int) (sr * 40), frames);
    auto b = std::make_shared<juce::AudioBuffer<float>> (2, frames);
    b->clear();
    juce::Random rnd ((juce::int64) seed);
    const bool rise = type == "riser" || type == "sweep" || type == "uplifter" || type == "noise" || type == "reverse" || type == "fill" || type == "drumBuild";
    const bool fall = type == "downlifter";
    if (rise || fall)
    {
        float lpL = 0, lpR = 0, hpL = 0, hpR = 0;
        for (int i = 0; i < frames; ++i)
        {
            const float t = (float) i / (float) frames, u = rise ? t : 1.0f - t;
            const float fc = 200.0f * std::pow (60.0f, u);                       // 200 Hz -> 12 kHz
            const float a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) sr);
            const float nl = rnd.nextFloat() * 2 - 1, nr = rnd.nextFloat() * 2 - 1;
            lpL += a * (nl - lpL); lpR += a * (nr - lpR);
            hpL += .002f * (lpL - hpL); hpR += .002f * (lpR - hpR);
            const float env = rise ? std::pow (t, 1.6f) : std::pow (1.0f - t, 1.4f);
            b->setSample (0, i, (lpL - hpL) * env); b->setSample (1, i, (lpR - hpR) * env);
        }
    }
    else if (type == "impact" || type == "subDrop")
    {
        double ph = 0;
        for (int i = 0; i < frames; ++i)
        {
            const double t = i / sr;
            const double f = type == "impact" ? 30 + 70 * std::exp (-t * 9) : 35 + 40 * std::exp (-t * 1.8);
            ph += juce::MathConstants<double>::twoPi * f / sr;
            float v = (float) std::sin (ph) * (float) std::exp (-t * (type == "impact" ? 1.6 : 1.1));
            if (type == "impact") v += (rnd.nextFloat() * 2 - 1) * (float) std::exp (-t * 7) * .5f;
            b->setSample (0, i, v); b->setSample (1, i, v);
        }
    }
    else return nullptr;
    return b;
}

struct Place { Buf buf; juce::int64 at = 0; int from = 0, len = 0; float gain = 1; int fadeIn = 0, fadeOut = 0; };
}

bool renderFxGuide (const juce::File& out, const Skeleton& sk, const std::vector<std::pair<SkCue, juce::File>>& picks,
                    int bpm, double sr, juce::uint32 seed, const std::function<bool()>& shouldExit, std::atomic<int>* progress, int* placedOut)
{
    if (bpm <= 0 || sk.song.bars <= 0) return false;
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    const double barF = sr * 240.0 / bpm;
    const juce::int64 N = (juce::int64) std::llround (sk.song.bars * barF);
    std::map<juce::String, Buf> cache;
    std::vector<Place> places;
    for (size_t i = 0; i < picks.size(); ++i)
    {
        if (shouldExit && shouldExit()) return false;
        const auto& cue = picks[i].first; const auto& file = picks[i].second;
        const auto a = cueAudio (cue.type);
        if (! a.render) continue;
        const juce::int64 w0 = (juce::int64) std::llround (cue.bar * barF), w1 = (juce::int64) std::llround ((cue.bar + cue.len) * barF);
        Buf b;
        if (file != juce::File())
        {
            const auto key = file.getFullPathName() + (a.reversed ? "#r" : "");
            auto it = cache.find (key);
            if (it != cache.end()) b = it->second;
            else
            {
                b = loadAudio (fm, file, sr);
                if (b != nullptr && a.reversed) for (int c = 0; c < 2; ++c) b->reverse (c, 0, b->getNumSamples());
                cache[key] = b;
            }
        }
        if (b == nullptr)
        {
            int len = (int) std::min<juce::int64> (w1 - w0, (juce::int64) (barF * 16));
            if (cue.type == "impact") len = (int) (sr * 3); else if (cue.type == "subDrop") len = (int) (sr * 2.5); else if (cue.type == "downlifter") len = (int) std::min<juce::int64> (w1 - w0, (juce::int64) (barF * 4));
            b = synth (cue.type, len, sr, seed + (juce::uint32) i * 977u);
        }
        if (b == nullptr || b->getNumSamples() <= 0) continue;
        const float pk = b->getMagnitude (0, b->getNumSamples());
        if (pk < 1.0e-5f) continue;
        Place p; p.buf = b; p.gain = .45f / pk;
        const int L = b->getNumSamples();
        if (a.endAligned)
        {
            const juce::int64 win = std::max<juce::int64> (1, w1 - w0);
            if (L > win) { p.from = (int) (L - win); p.len = (int) win; p.at = w0; p.fadeIn = (int) (sr * .05); }
            else { p.from = 0; p.len = L; p.at = w1 - L; }
        }
        else
        {
            const juce::int64 maxLen = std::max<juce::int64> (w1 - w0, (juce::int64) (barF * 4)) + (juce::int64) (barF * 4);
            p.from = 0; p.len = (int) std::min<juce::int64> (L, maxLen); p.at = w0;
            if (p.len < L) p.fadeOut = (int) std::min<double> (sr * .25, p.len / 2.0);
        }
        if (p.at + p.len > N) p.len = (int) std::max<juce::int64> (0, N - p.at);
        if (p.at < 0) { p.from += (int) -p.at; p.len -= (int) -p.at; p.at = 0; }
        p.fadeOut = juce::jmax (p.fadeOut, (int) std::min<double> (sr * .01, p.len / 2.0));
        if (p.len > 0) places.push_back (p);
    }
    if (placedOut) *placedOut = (int) places.size();

    out.deleteFile();
    std::unique_ptr<juce::OutputStream> os (out.createOutputStream());
    if (os == nullptr) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> wr (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (wr == nullptr) return false;
    os.release();
    const int CH = 1 << 16;
    juce::AudioBuffer<float> chunk (2, CH);
    for (juce::int64 c0 = 0; c0 < N; c0 += CH)
    {
        if (shouldExit && shouldExit()) return false;
        const int n = (int) std::min<juce::int64> (CH, N - c0);
        chunk.clear();
        for (auto& p : places)
        {
            const juce::int64 a0 = juce::jmax (c0, p.at), a1 = juce::jmin (c0 + n, p.at + p.len);
            for (juce::int64 t = a0; t < a1; ++t)
            {
                const int k = (int) (t - p.at);
                float g = p.gain;
                if (k < p.fadeIn) g *= (float) k / (float) p.fadeIn;
                if (p.len - k < p.fadeOut) g *= (float) (p.len - k) / (float) p.fadeOut;
                for (int ch = 0; ch < 2; ++ch) chunk.addSample (ch, (int) (t - c0), p.buf->getSample (ch, p.from + k) * g);
            }
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            float* d = chunk.getWritePointer (ch);
            for (int i = 0; i < n; ++i) { const float x = d[i], ax = std::abs (x); if (ax > .8f) d[i] = std::copysign (.8f + .2f * std::tanh ((ax - .8f) / .2f), x); }
        }
        if (! wr->writeFromAudioSampleBuffer (chunk, 0, n)) return false;
        if (progress) progress->store (40 + (int) (55.0 * (double) (c0 + n) / (double) N));
    }
    return true;
}


// FL Studio projesi: HitGen sesleri ve FX'ler WAV olarak Samples/ altına, .flp proje klasörünün köküne
static bool writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.getParentDirectory().createDirectory(); f.deleteFile();
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
    if (os == nullptr) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> wr (wav.createWriterFor (os.get(), sr, (unsigned int) juce::jmax (1, b.getNumChannels()), 24, {}, 0));
    if (wr == nullptr) return false;
    os.release();
    return wr->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

static double audioSeconds (juce::AudioFormatManager& fm, const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    return r != nullptr && r->sampleRate > 0 ? (double) r->lengthInSamples / r->sampleRate : 0.0;
}

bool writeFlpProject (const ExportJobInput& in, ExportResult& R, juce::String& err)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    const auto hgDir = in.dir.getChildFile ("Samples").getChildFile ("HitGen");
    if (! hgDir.createDirectory()) { err = "mkdir " + hgDir.getFullPathName(); return false; }
    FlpSources src;
    const auto arr = buildArrangement (in.sk);
    // Mix: pembe gürültü referansıyla denge + gain staging. Her şeridin tek vuruşu/notası yüklenir, kazançlar örneklere yazılır (fader 0 dB).
    std::map<juce::String, juce::AudioBuffer<float>> shots;
    std::map<juce::String, Buf> libShots;
    for (auto& L : arr.lanes)
    {
        if (auto it = in.laneFiles.find (L.role); it != in.laneFiles.end() && it->second.existsAsFile())
        {
            if (auto b = loadAudio (fm, it->second, in.renderRate)) { shots[L.role] = *b; libShots[L.role] = b; }
        }
        else if (auto r = in.laneRenders.find (L.role); r != in.laneRenders.end() && r->second.getNumSamples() > 0)
            shots[L.role] = r->second;
    }
    const auto mix = autoMix (arr, shots, in.renderRate);
    auto applied = [&] (const juce::String& role, juce::AudioBuffer<float> b)
    {
        b.applyGain ((float) std::pow (10.0, mix.gainFor (role) / 20.0));
        applyPan (b, rolePan (role));
        return b;
    };
    for (auto& L : arr.lanes)
    {
        if (auto it = in.laneFiles.find (L.role); it != in.laneFiles.end() && it->second.existsAsFile())
        {
            if (auto lb = libShots.find (L.role); lb != libShots.end())
            {
                auto dst = hgDir.getChildFile (L.name + " - " + it->second.getFileNameWithoutExtension() + " (mix).wav");
                if (! writeWav (dst, applied (L.role, *lb->second), in.renderRate)) { err = "write " + dst.getFullPathName(); return false; }
                src.lanes[L.role] = dst.getFullPathName();
            }
            else   // okunamayan dosya: eski davranış (kopya, kazançsız)
            {
                auto dst = hgDir.getChildFile (L.name + " - " + it->second.getFileName());
                if (! dst.existsAsFile() && ! it->second.copyFileTo (dst)) { err = "write " + dst.getFullPathName(); return false; }
                src.lanes[L.role] = dst.getFullPathName();
            }
        }
        else if (auto r = in.laneRenders.find (L.role); r != in.laneRenders.end() && r->second.getNumSamples() > 0)
        {
            auto dst = hgDir.getChildFile ("HitGen " + L.name + (L.gm < 0 ? " C5" : "") + ".wav");
            if (! writeWav (dst, applied (L.role, r->second), in.renderRate)) { err = "write " + dst.getFullPathName(); return false; }
            src.lanes[L.role] = dst.getFullPathName();
        }
    }
    {   // referans pembe gürültü (-23 dBFS RMS) ve rapor
        const auto ref = in.dir.getChildFile ("Audio").getChildFile ("Pink Noise Reference.wav");
        if (! writeWav (ref, pinkNoise (in.renderRate, 30.0, kRefNoiseRmsDb), in.renderRate)) { err = "write " + ref.getFullPathName(); return false; }
        R.pinkNoise = ref;
        R.mixReport = in.dir.getChildFile ("Mix Report.txt");
        if (! R.mixReport.replaceWithText (mixReport (mix, arr))) { err = "write " + R.mixReport.getFullPathName(); return false; }
        R.mixCommonDb = mix.commonDb; R.mixPeakDb = mix.mixPeakDb; R.mixRmsDb = mix.mixRmsDb;
    }
    // FX: önerilen sample (ters ipucunda ters çevrilmiş kopya) ya da HitGen sentezi
    const auto fxDir = in.dir.getChildFile ("Samples").getChildFile ("FX");
    std::map<juce::String, juce::String> made;
    for (size_t i = 0; i < in.picks.size(); ++i)
    {
        const auto& [cue, file] = in.picks[i];
        const auto a = cueAudio (cue.type);
        if (! a.render) continue;
        juce::String path;
        if (file != juce::File() && file.existsAsFile())
        {
            const auto key = file.getFullPathName() + (a.reversed ? "#r" : "");
            if (auto it = made.find (key); it != made.end()) path = it->second;
            else
            {
                juce::File dst;
                if (a.reversed)
                {
                    auto b = loadAudio (fm, file, in.renderRate);
                    if (b == nullptr) continue;
                    for (int c = 0; c < 2; ++c) b->reverse (c, 0, b->getNumSamples());
                    b->applyGain ((float) std::pow (10.0, fxGainDb (cue.type, *b, in.renderRate, mix) / 20.0));
                    dst = fxDir.getChildFile (file.getFileNameWithoutExtension() + " (reverse, mix).wav");
                    if (! writeWav (dst, *b, in.renderRate)) { err = "write " + dst.getFullPathName(); return false; }
                }
                else
                {
                    dst = fxDir.getChildFile (file.getFileNameWithoutExtension() + " (mix).wav");
                    auto b = loadAudio (fm, file, in.renderRate);
                    if (b != nullptr)
                    {
                        b->applyGain ((float) std::pow (10.0, fxGainDb (cue.type, *b, in.renderRate, mix) / 20.0));
                        if (! writeWav (dst, *b, in.renderRate)) { err = "write " + dst.getFullPathName(); return false; }
                    }
                    else
                    {
                        dst = fxDir.getChildFile (file.getFileName()); fxDir.createDirectory();
                        if (! dst.existsAsFile() && ! file.copyFileTo (dst)) { err = "write " + dst.getFullPathName(); return false; }
                    }
                }
                path = made[key] = dst.getFullPathName();
            }
        }
        else
        {
            const double barSec = 240.0 / juce::jmax (1, in.bpm);
            int len = (int) (in.renderRate * juce::jmin (cue.len, 16) * barSec);
            if (cue.type == "impact") len = (int) (in.renderRate * 3); else if (cue.type == "subDrop") len = (int) (in.renderRate * 2.5);
            else if (cue.type == "downlifter") len = (int) (in.renderRate * juce::jmin (cue.len, 4) * barSec);
            auto b = synth (cue.type, len, in.renderRate, in.seed + (juce::uint32) i * 977u);
            if (b == nullptr) continue;
            const float pk = b->getMagnitude (0, b->getNumSamples()); if (pk > 1e-5f) b->applyGain (.7f / pk);
            b->applyGain ((float) std::pow (10.0, fxGainDb (cue.type, *b, in.renderRate, mix) / 20.0));
            auto dst = fxDir.getChildFile ("HitGen " + cue.type + " bar " + juce::String (cue.bar + 1).paddedLeft ('0', 3) + ".wav");
            if (! writeWav (dst, *b, in.renderRate)) { err = "write " + dst.getFullPathName(); return false; }
            path = dst.getFullPathName();
        }
        FlpSources::Fx fx; fx.cue = cue; fx.path = path; fx.endAligned = a.endAligned; fx.seconds = audioSeconds (fm, juce::File (path));
        src.fx.push_back (fx);
    }
    auto proj = buildFlp (arr, src, in.baseName);
    R.flp = in.dir.getChildFile (juce::File::createLegalFileName (in.baseName + ".flp"));
    const auto bytes = writeFlp (proj);
    if (! R.flp.replaceWithData (bytes.getData(), bytes.getSize())) { err = "write " + R.flp.getFullPathName(); return false; }
    R.flpPatterns = (int) proj.patterns.size();
    for (auto& it : proj.items) if (! it.pattern) ++R.flpClips;
    // Ableton Live: aynı aranjman ve aynı sesler (.als)
    AlsInfo ai;
    const auto als = buildAlsXml (arr, src, in.dir, &ai);
    R.als = in.dir.getChildFile (juce::File::createLegalFileName (in.baseName + ".als"));
    if (! writeAls (R.als, als)) { err = "write " + R.als.getFullPathName(); return false; }
    R.alsClips = ai.midiClips + ai.audioClips;
    return true;
}

bool renderStems (const Arrangement& a, Engine& E, double sr, const juce::File& dir, const std::function<bool()>& shouldExit,
                  std::atomic<int>* progress, int progressFrom, int progressTo, juce::StringPairArray& files, int* countOut)
{
    if (a.P.bpm <= 0 || a.bars <= 0) return false;
    struct Ev { juce::int64 at; int track, note, vel; double len; int lane; };
    std::vector<Ev> ev;
    const double secPerTick = 60.0 / a.P.bpm / 480.0;
    for (size_t li = 0; li < a.lanes.size(); ++li)
        for (auto& n : a.lanes[li].notes)
        {
            const int t0 = tickOf (n.s, a.P.swing), t1 = juce::jmax (t0 + 1, tickOf (n.s + n.l, a.P.swing));
            ev.push_back ({ (juce::int64) std::llround (t0 * secPerTick * sr), a.lanes[li].part, n.n, n.v, (t1 - t0) * secPerTick, (int) li });
        }
    std::stable_sort (ev.begin(), ev.end(), [] (const Ev& x, const Ev& y) { return x.at < y.at; });
    // stem grupları: tüm davul şeritleri tek stem, her melodik şerit (bas, akor, pad, lead, arp) ayrı stem
    struct Group { juce::String name; std::vector<int> lanes; int bus; };
    std::vector<Group> groups;
    { Group d { "Drums", {}, kDrums }; for (size_t li = 0; li < a.lanes.size(); ++li) if (a.lanes[li].gm >= 0) d.lanes.push_back ((int) li); if (! d.lanes.empty()) groups.push_back (d); }
    for (size_t li = 0; li < a.lanes.size(); ++li) if (a.lanes[li].gm < 0) groups.push_back ({ a.lanes[li].name, { (int) li }, a.lanes[li].part });
    const juce::int64 N = (juce::int64) std::llround (a.seconds() * sr) + (juce::int64) (sr * 2);   // 2 sn kuyruk
    const auto stemDir = dir.getChildFile ("Stems");
    if (! stemDir.createDirectory()) return false;
    int made = 0;
    for (size_t gi = 0; gi < groups.size(); ++gi)
    {
        const auto& G = groups[gi];
        std::vector<bool> on (a.lanes.size(), false); for (int li : G.lanes) on[(size_t) li] = true;
        // yalnızca bu grubun bus'ı duyulur; davul vuruşları her zaman tetiklenir (sidechain pump karışımdaki gibi), sesleri susturulur
        for (int t = 0; t < kNumTracks; ++t) E.macros[(size_t) t].mute = t != G.bus;
        E.reset(); E.allNotesOff();
        juce::AudioBuffer<float> blk (2, 512);
        for (int i = 0; i < 8; ++i) { blk.clear(); E.render (blk, 0, 512); }
        const auto f = stemDir.getChildFile (juce::File::createLegalFileName (juce::String (made + 1).paddedLeft ('0', 2) + " " + G.name + ".wav"));
        f.deleteFile();
        std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
        if (os == nullptr) return false;
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> wr (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
        if (wr == nullptr) return false;
        os.release();
        size_t k = 0; juce::int64 pos = 0;
        while (pos < N)
        {
            if (shouldExit && shouldExit()) return false;
            const int n = (int) std::min<juce::int64> (512, N - pos);
            blk.clear();
            int at = 0;
            while (k < ev.size() && ev[k].at < pos + n)
            {
                const auto& e = ev[k++];
                const bool drum = e.track == kDrums;
                if (! drum && ! on[(size_t) e.lane]) continue;
                const int off = (int) std::clamp<juce::int64> (e.at - pos, 0, n);
                if (off > at) { E.render (blk, at, off - at); at = off; }
                if (drum) E.drumHit (e.note, e.vel, 0); else E.noteOn (e.track, e.note, e.vel, e.len, 0);
            }
            if (at < n) E.render (blk, at, n - at);
            if (! wr->writeFromAudioSampleBuffer (blk, 0, n)) return false;
            pos += n;
            if (progress != nullptr && (pos & 0xffff) < 512)
                progress->store (progressFrom + (int) ((progressTo - progressFrom) * ((double) gi + (double) pos / (double) N) / (double) std::max<size_t> (1, groups.size())));
        }
        files.set ("stem_" + G.name.toLowerCase().replace (" ", ""), "Stems/" + f.getFileName());
        ++made;
    }
    for (int t = 0; t < kNumTracks; ++t) E.macros[(size_t) t].mute = false;
    if (countOut != nullptr) *countOut = made;
    return true;
}

ExportResult exportProject (const ExportJobInput& in, const std::function<bool()>& shouldExit, std::atomic<int>* progress)
{
    ExportResult R; R.dir = in.dir;
    auto fail = [&R] (const juce::String& e) { R.ok = false; R.error = e; return R; };
    if (! in.dir.createDirectory()) return fail ("mkdir " + in.dir.getFullPathName());
    if (progress) progress->store (2);
    const char* const names[5] = { " - Full Song.mid", "1 Chords.mid", "2 Bass.mid", "3 Drums.mid", "4 Melody.mid" };
    auto midiDir = in.dir.getChildFile ("MIDI"); midiDir.createDirectory();
    for (size_t i = 0; i < in.midis.size() && i < 5; ++i)
    {
        auto f = i == 0 ? in.dir.getChildFile (juce::File::createLegalFileName (in.baseName + names[0])) : midiDir.getChildFile (names[i]);
        if (! f.replaceWithData (in.midis[i].getData(), in.midis[i].getSize())) return fail ("write " + f.getFullPathName());
    }
    if (progress) progress->store (8);
    auto smpDir = in.dir.getChildFile ("Samples");
    std::map<juce::String, bool> done;
    for (size_t i = 0; i < in.picks.size(); ++i)
    {
        if (shouldExit && shouldExit()) return fail ("cancelled");
        const auto& [cue, f] = in.picks[i];
        if (f == juce::File() || ! f.existsAsFile() || f.getSize() > 200ll * 1024 * 1024) continue;
        const auto key = cue.type + "|" + f.getFullPathName();
        if (done.count (key)) continue;
        done[key] = true;
        smpDir.createDirectory();
        const auto dst = smpDir.getChildFile (juce::File::createLegalFileName ("Bar " + juce::String (cue.bar + 1).paddedLeft ('0', 3) + " - " + cue.type + " - " + f.getFileName()));
        if (f.copyFileTo (dst)) ++R.copied;
        if (progress) progress->store (8 + (int) (30.0 * (double) (i + 1) / (double) std::max<size_t> (1, in.picks.size())));
    }
    auto audioDir = in.dir.getChildFile ("Audio"); audioDir.createDirectory();
    R.guide = audioDir.getChildFile ("FX Guide.wav");
    if (! renderFxGuide (R.guide, in.sk, in.picks, in.bpm, in.sampleRate, in.seed, shouldExit, progress, &R.placed))
        return fail (shouldExit && shouldExit() ? "cancelled" : "write " + R.guide.getFullPathName());
    juce::StringPairArray files;
    files.set ("plan", "Plan.txt"); files.set ("fxGuide", "Audio/FX Guide.wav");
    files.set ("midiFullSong", juce::File::createLegalFileName (in.baseName + names[0]));
    for (int i = 1; i < 5; ++i) files.set ("midi_" + juce::String (names[i]).fromFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf (".", false, false).toLowerCase(), juce::String ("MIDI/") + names[i]);
    const auto arr = buildArrangement (in.sk);
    if (in.writeBridges)
    {   // çok izli MIDI (iz adlı) ve FL Piano Roll script'leri
        R.midi = in.dir.getChildFile ("HitGen Arrangement.mid");
        const auto mb = arrangementMidi (Data::get(), arr);
        if (! R.midi.replaceWithData (mb.getData(), mb.getSize())) return fail ("write " + R.midi.getFullPathName());
        files.set ("midiArrangement", R.midi.getFileName());
        const auto sdir = in.dir.getChildFile ("FL Piano Roll Scripts");
        if (! sdir.createDirectory()) return fail ("mkdir " + sdir.getFullPathName());
        for (auto& L : arr.lanes)
        {
            const auto f = sdir.getChildFile (juce::File::createLegalFileName ("HitGen - " + L.name + ".pyscript"));
            if (! f.replaceWithText (flPianoRollScript (arr, L, in.baseName), false, false, "\r\n")) return fail ("write " + f.getFullPathName());
            ++R.scripts;
        }
        files.set ("flPianoRollScripts", sdir.getFileName());
    }
    if (in.writeStems && in.stemEngine != nullptr)
        if (! renderStems (arr, *in.stemEngine, in.renderRate, in.dir, shouldExit, progress, 95, 99, files, &R.stems))
            return fail (shouldExit && shouldExit() ? "cancelled" : "stems");
    if (in.writeFlp)
    {
        juce::String e;
        if (! writeFlpProject (in, R, e)) return fail (e);
        if (shouldExit && shouldExit()) return fail ("cancelled");
    }
    if (R.flp != juce::File()) files.set ("flStudioProject", R.flp.getFileName());
    if (R.als != juce::File()) files.set ("abletonProject", R.als.getFileName());
    if (R.mixReport != juce::File()) { files.set ("mixReport", R.mixReport.getFileName()); files.set ("pinkNoiseReference", "Audio/Pink Noise Reference.wav"); }
    if (in.writeBridges)
    {
        R.manifest = in.dir.getChildFile ("manifest.json");
        if (! R.manifest.replaceWithText (arrangementManifest (Data::get(), arr, files))) return fail ("write " + R.manifest.getFullPathName());
    }
    auto plan = in.dir.getChildFile ("Plan.txt");
    if (! plan.replaceWithText (in.planText, false, false, "\r\n")) return fail ("write " + plan.getFullPathName());
    if (progress) progress->store (100);
    R.ok = true;
    return R;
}
}
