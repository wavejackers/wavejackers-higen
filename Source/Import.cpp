#include "Import.h"
#include <zstd.h>
#include <cmath>

namespace hg
{
namespace
{
// Dosyadan gelen metin: geçersiz UTF-8 (bozuk / kasıtlı hazırlanmış dosya) juce::String'e verilmez; aksi halde JUCE
// eksik çok baytlı diziyi okurken tamponun sonunu aşabilir (ASan: heap-buffer-overflow). Geçersizse bayt bayt Latin-1 alınır.
juce::String safeText (const char* p, size_t len)
{
    len = juce::jmin (len, (size_t) (1 << 24));
    if (len == 0) return {};
    if (juce::CharPointer_UTF8::isValidString (p, (int) len)) return juce::String::fromUTF8 (p, (int) len);
    juce::String s; s.preallocateBytes (len * 2);
    for (size_t k = 0; k < len && p[k] != 0; ++k) s += (juce::juce_wchar) (uint8_t) p[k];
    return s;
}

constexpr size_t kMaxFile = 256u * 1024u * 1024u, kMaxCbor = 16u * 1024u * 1024u;

uint32_t be32 (const uint8_t* p) { return ((uint32_t) p[0] << 24) | ((uint32_t) p[1] << 16) | ((uint32_t) p[2] << 8) | p[3]; }
uint32_t le32 (const uint8_t* p) { return ((uint32_t) p[3] << 24) | ((uint32_t) p[2] << 16) | ((uint32_t) p[1] << 8) | p[0]; }
juce::String fourCC (const uint8_t* p) { char c[5] = { (char) p[0], (char) p[1], (char) p[2], (char) p[3], 0 }; for (int i = 0; i < 4; ++i) if (c[i] < 32 || c[i] > 126) c[i] = '?'; return c; }

// ------------------------------------------------------------ CBOR (yalnızca okuma, sınırlı)
struct Cbor
{
    const uint8_t* b; size_t n, i = 0; int depth = 0; int budget = 400000; bool bad = false;
    bool need (size_t k) { if (i + k > n) { bad = true; return false; } return true; }
    uint64_t arg (int ai)
    {
        if (ai < 24) return (uint64_t) ai;
        const int k = ai == 24 ? 1 : ai == 25 ? 2 : ai == 26 ? 4 : ai == 27 ? 8 : 0;
        if (k == 0 || ! need ((size_t) k)) { bad = true; return 0; }
        uint64_t v = 0; for (int j = 0; j < k; ++j) v = (v << 8) | b[i++];
        return v;
    }
    juce::var read()
    {
        if (bad || --budget < 0 || ! need (1) || depth > 24) { bad = true; return {}; }
        const int ib = b[i++], mt = ib >> 5, ai = ib & 31;
        switch (mt)
        {
            case 0: { const auto v = arg (ai); return (double) v; }
            case 1: { const auto v = arg (ai); return -1.0 - (double) v; }
            case 2: { const auto l = arg (ai); if (l > n - i) { bad = true; return {}; } i += (size_t) l; return {}; }
            case 3: { const auto l = arg (ai); if (l > n - i) { bad = true; return {}; } juce::String s = safeText ((const char*) b + i, (size_t) l); i += (size_t) l; return s; }
            case 4:
            {
                const auto c = arg (ai); if (bad || c > 100000) { bad = true; return {}; }
                juce::Array<juce::var> a; ++depth;
                for (uint64_t k = 0; k < c && ! bad; ++k) a.add (read());
                --depth; return a;
            }
            case 5:
            {
                const auto c = arg (ai); if (bad || c > 100000) { bad = true; return {}; }
                auto* o = new juce::DynamicObject(); juce::var res (o); ++depth;
                for (uint64_t k = 0; k < c && ! bad; ++k)
                {
                    auto key = read(); auto val = read();
                    if (key.isString()) o->setProperty (juce::Identifier (key.toString()), val);
                }
                --depth; return res;
            }
            case 6: arg (ai); return read();
            default:
                if (ai == 20) return false; if (ai == 21) return true; if (ai == 22 || ai == 23) return {};
                if (ai == 25) { if (! need (2)) return {}; const int h = (b[i] << 8) | b[i + 1]; i += 2; const int e = (h >> 10) & 31, m = h & 1023; double v = e == 0 ? std::ldexp (m, -24) : e == 31 ? 0.0 : std::ldexp (m + 1024, e - 25); return (h & 0x8000) ? -v : v; }
                if (ai == 26) { if (! need (4)) return {}; uint32_t u = ((uint32_t) b[i] << 24) | ((uint32_t) b[i + 1] << 16) | ((uint32_t) b[i + 2] << 8) | b[i + 3]; i += 4; float f; std::memcpy (&f, &u, 4); return (double) f; }
                if (ai == 27) { if (! need (8)) return {}; uint64_t u = 0; for (int j = 0; j < 8; ++j) u = (u << 8) | b[i++]; double d; std::memcpy (&d, &u, 8); return d; }
                bad = true; return {};
        }
    }
};

double D (const juce::var& v, double d) { return v.isDouble() || v.isInt() || v.isInt64() ? (double) v : (v.isBool() ? ((bool) v ? 1.0 : 0.0) : d); }
const juce::var P (const juce::var& o, const char* key) { return o.isObject() ? o.getProperty (juce::Identifier (key), juce::var()) : juce::var(); }
double pp (const juce::var& module, const char* k, double d) { return D (P (P (module, "plainParams"), k), d); }
double cl (double x, double a, double b) { return juce::jlimit (a, b, std::isfinite (x) ? x : a); }

juce::String wtName (const juce::var& osc, int i)
{
    const auto w = P (osc, ("WTOsc" + juce::String (i)).toRawUTF8());
    return P (w, "relativePathToWT").toString();
}

void fromSerum2 (const juce::var& root, ImportedSound& out)
{
    auto& x = out.x;
    for (int i = 0; i < kNumSp; ++i) x[(size_t) i] = spDef (i).def;
    const int oscBase[3] = { o1w, o2w, o3w };
    double sumLvl = 0; int maxUni = 1; double uniDetune = 0; int active = 0;
    for (int j = 0; j < 3; ++j)
    {
        const auto o = P (root, ("Oscillator" + juce::String (j)).toRawUTF8());
        const int b = oscBase[j];
        const bool enabled = pp (o, "kParamEnable", j == 0 ? 1.0 : 0.0) > .5;
        const double vol = pp (o, "kParamVolume", .75);
        if (! enabled || vol <= .001) { x[(size_t) b] = 0; continue; }
        const auto wt = P (o, ("WTOsc" + juce::String (j)).toRawUTF8());
        const auto nm = wtName (o, j).toLowerCase();
        const double pos = cl (pp (wt, "kParamTablePos", 0) / 256.0, 0, 1);
        int wave;
        if (nm.contains ("default shapes") || nm.contains ("basic shapes") || nm.contains ("analog"))
            wave = pos < .2 ? 4 : pos < .4 ? 3 : pos < .75 ? 1 : 2;       // sinüs, üçgen, testere, kare
        else
        {
            wave = 5;
            const int h = (int) (juce::String (nm).hashCode() & 0x7fffffff);
            x[wtA] = (float) (h % 12); x[wtB] = (float) ((h / 12) % 12); x[wtMorph] = (float) cl (pos, 0, 1); x[wtBright] = .85f;
        }
        const double totalCents = cl (pp (o, "kParamOctave", 0), -3, 3) * 1200.0 + cl (pp (o, "kParamCoarsePit", 0), -24, 24) * 100.0 + cl (pp (o, "kParamFinePit", 0), -100, 100);
        const int oct = (int) std::lround (totalCents / 1200.0); const double rem = totalCents - oct * 1200.0;
        const int semi = (int) std::lround (rem / 100.0);
        x[(size_t) b] = (float) wave; x[(size_t) b + 1] = (float) cl (oct, -3, 3); x[(size_t) b + 2] = (float) cl (semi, -12, 12); x[(size_t) b + 3] = (float) cl (rem - semi * 100.0, -50, 50);
        x[(size_t) b + 4] = (float) cl (vol, 0, 1); sumLvl += vol; ++active;
        maxUni = juce::jmax (maxUni, (int) cl (pp (o, "kParamUnison", 1), 1, 16));
        uniDetune = juce::jmax (uniDetune, pp (o, "kParamDetune", .25));
    }
    if (active == 0) { x[o1w] = 1; x[o1lvl] = .6f; sumLvl = .6; }
    if (sumLvl > 1.3) for (int j = 0; j < 3; ++j) x[(size_t) oscBase[j] + 4] *= (float) (1.3 / sumLvl);
    x[uni] = (float) juce::jmin (7, maxUni); x[uniDet] = maxUni > 1 ? (float) cl (uniDetune * 100.0, 3, 45) : 0.0f;
    // alt osilatör ve gürültü
    const auto sb = P (root, "Oscillator4"); if (pp (sb, "kParamEnable", 0) > .5) x[sub] = (float) cl (pp (sb, "kParamVolume", .5), 0, 1);
    const auto nz = P (root, "Oscillator3"); if (pp (nz, "kParamEnable", 0) > .5) x[noise] = (float) cl (pp (nz, "kParamVolume", .3) * .4, 0, .5);
    // filtre
    const auto vf = P (root, "VoiceFilter0");
    if (pp (vf, "kParamEnable", 0) > .5)
    {
        const auto t = P (P (vf, "plainParams"), "kParamType").toString().toUpperCase();
        x[ftype] = t.startsWithChar ('H') ? 1.0f : t.startsWithChar ('B') ? 2.0f : 0.0f;
        const int num = t.retainCharacters ("0123456789").getIntValue();
        x[fslope] = num >= 18 ? 1.0f : 0.0f;
        x[cut] = (float) cl (20.0 * std::pow (1000.0, cl (pp (vf, "kParamFreq", .6), 0, 1)), 30, 18000);
        x[reso] = (float) cl (.7 + pp (vf, "kParamReso", 10) / 100.0 * 9.0, 0, 14);
        x[fdrive] = (float) cl (pp (vf, "kParamDrive", 0) / 100.0 * 1.2, 0, 1.5);
    }
    else { x[ftype] = 0; x[cut] = 18000; x[reso] = .7f; }
    // zarf 1: genlik
    const auto e = P (root, "Env0");
    x[envA] = (float) cl (pp (e, "kParamAttack", .001), .001, 5); x[envD] = (float) cl (pp (e, "kParamDecay", 1.0), .01, 5);
    x[envS] = (float) cl (pp (e, "kParamSustain", 1.0), 0, 1); x[envR] = (float) cl (pp (e, "kParamRelease", .3), .01, 8);
    x[fenv] = x[envS] < .4f ? 2.0f : .6f; x[fdec] = (float) cl (x[envD] * .8, .05, 3); x[fatk] = 0;
    // efekt rafı
    if (auto* fx = P (P (root, "FXRack0"), "FX").getArray())
        for (auto& f : *fx)
        {
            if (! f.isObject()) continue;
            if (P (f, "FXDistortion").isObject()) x[fxDrive] = .3f;
            if (P (f, "FXReverb").isObject()) x[fxReverb] = .35f;
            if (P (f, "FXDelay").isObject()) x[fxDelay] = .3f;
            if (P (f, "FXHyperD").isObject()) x[fxChorus] = .4f;
        }
    x[level] = .2f;
}
}

int guessPart (const juce::String& nm)
{
    const auto n = nm.toLowerCase();
    for (auto k : { "bass", "sub", "reese", "808" }) if (n.contains (k)) return 1;
    for (auto k : { "pad", "chord", "stab", "string", "keys", "piano", "organ", "braam", "brass", "drone" }) if (n.contains (k)) return 0;
    return 2;
}

juce::String fallbackFor (const juce::String& nm, int part)
{
    const auto n = nm.toLowerCase();
    auto has = [&] (std::initializer_list<const char*> l) { for (auto k : l) if (n.contains (k)) return true; return false; };
    if (part == 1) return has ({ "reese", "growl" }) ? "ws-reese-wide" : has ({ "sub", "808" }) ? "ws-sub-heavy" : has ({ "deep", "soft", "warm" }) ? "ws-deep-warm-bass" : "ws-fat-roll";
    if (part == 0)
        return has ({ "pad", "string", "vox", "ambien", "drone", "braam" }) ? "ws-lush-pad" : has ({ "piano", "keys", "ep", "rhodes" }) ? "ws-grand-warm" : has ({ "brass" }) ? "ws-brass-hit" : has ({ "organ" }) ? "ws-rave-organ" : "ws-super-chord";
    return has ({ "pluck", "arp", "bell", "kalimba" }) ? "ws-wide-pluck" : has ({ "soft", "silk", "warm" }) ? "ws-silk-lead" : "ws-anthem-lead";
}

ImportedSound importSoundBytes (const void* data, size_t size, const juce::String& fileName)
{
    ImportedSound r;
    auto fail = [&r] (const char* e) { r.ok = false; r.error = e; return r; };
    const auto* b = static_cast<const uint8_t*> (data);
    if (size < 16) return fail ("empty");
    r.name = juce::File::createFileWithoutCheckingPath (fileName).getFileNameWithoutExtension();

    if (size > 17 && std::memcmp (b, "XferJson", 8) == 0)   // Serum 2
    {
        const uint32_t jl = le32 (b + 9);
        if (jl > size || 17 + (size_t) jl + 8 > size) return fail ("corrupt");
        const auto meta = juce::JSON::parse (safeText ((const char*) b + 17, (size_t) jl));
        if (meta.isObject()) { r.name = meta["presetName"].toString().isNotEmpty() ? meta["presetName"].toString() : r.name; r.author = meta["presetAuthor"].toString(); }
        const uint8_t* tail = b + 17 + jl; const size_t tl = size - 17 - jl;
        const uint32_t rawLen = le32 (tail);
        if (rawLen == 0 || rawLen > kMaxCbor || tl <= 8) return fail ("corrupt");
        juce::MemoryBlock raw (rawLen);
        const size_t got = ZSTD_decompress (raw.getData(), rawLen, tail + 8, tl - 8);
        if (ZSTD_isError (got) || got == 0) return fail ("decompress");
        Cbor c { static_cast<const uint8_t*> (raw.getData()), got };
        const auto root = c.read();
        if (c.bad || ! root.isObject()) return fail ("corrupt");
        r.format = "Serum 2"; r.part = guessPart (r.name); r.exact = true; r.ok = true;
        fromSerum2 (root, r);
        return r;
    }
    if (std::memcmp (b, "CcnK", 4) == 0 && size >= 60)   // VST2 FXP / FXB
    {
        const auto type = fourCC (b + 8);
        r.format = "VST2 " + juce::String (type.startsWith ("FxB") || type == "FBCh" ? "FXB" : "FXP") + " (" + fourCC (b + 16) + ")";
        if (fourCC (b + 16) == "XfsX") r.format += " Serum 1";
        if (type == "FxCk" || type == "FPCh")
        {
            char nm[29] = {}; std::memcpy (nm, b + 28, 28);
            juce::String s = safeText (nm, std::strlen (nm)).trim(); if (s.isNotEmpty()) r.name = s;
        }
        else if (size >= 156 && (type == "FxBk" || type == "FBCh")) r.programs = (int) juce::jlimit (0u, 100000u, be32 (b + 24));
        r.part = guessPart (r.name); r.fallbackId = fallbackFor (r.name, r.part); r.exact = false; r.ok = true;
        return r;
    }
    return fail ("format");
}

ImportedSound importSoundFile (const juce::File& f)
{
    ImportedSound r;
    if (! f.existsAsFile()) { r.error = "missing"; return r; }
    if ((size_t) f.getSize() > kMaxFile) { r.error = "too large"; return r; }
    juce::MemoryBlock mb;
    if (! f.loadFileAsData (mb)) { r.error = "read"; return r; }
    return importSoundBytes (mb.getData(), mb.getSize(), f.getFileName());
}
}
