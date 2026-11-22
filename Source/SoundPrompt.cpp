#include "SoundPrompt.h"
#include "SynthX.h"
#include "Synth.h"
#include "Data.h"
#include <cmath>
#include <map>

namespace sp
{
using juce::String; using juce::var;

static const char* const kRateNames[kNumRates] = { "1/1", "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
static const int kRateLfo[kNumRates] = { 3, 4, 5, 7, 8, 9, 10, 11 };          // xs kSyncBeats dizini
static const int kRateArp[kNumRates] = { 0, 0, 0, 1, 3, 2, 4, 5 };            // xs arpej bölümü
static const double kRateBeats[kNumRates] = { 4, 2, 1, .5, 1.0 / 3, .25, 1.0 / 6, .125 };
String rateName (int i) { return kRateNames[juce::jlimit (0, kNumRates - 1, i)]; }
int rateLfoIndex (int i) { return kRateLfo[juce::jlimit (0, kNumRates - 1, i)]; }
int rateArpDiv (int i) { return kRateArp[juce::jlimit (0, kNumRates - 1, i)]; }
int defaultRate (Kind k) { return k == kPad ? 1 : k == kPluck || k == kBell ? 5 : 3; }

namespace
{
bool tr (const String& lang) { return lang == "tr"; }
String L (const String& lang, const char* t, const char* e) { return tr (lang) ? String::fromUTF8 (t) : String (e); }

// ---------------------------------------------------------------- metin çözümleme
String norm (const String& in)
{
    String s = in.toLowerCase(), o;
    for (auto c : s)
    {
        switch (c)
        {
            case 0x131: case 0x130: o << 'i'; continue;   // ı İ
            case 0x15f: o << 's'; continue; case 0xe7: o << 'c'; continue; case 0x11f: o << 'g'; continue;
            case 0xf6: o << 'o'; continue; case 0xfc: o << 'u'; continue; case 0xe2: o << 'a'; continue; case 0xee: o << 'i'; continue;
            default: break;
        }
        if (juce::CharacterFunctions::isLetterOrDigit (c) || c == '#' || c == '/') o << String::charToString (c); else o << ' ';
    }
    o = " " + o + " ";
    // iki kelimelik adlar tek simgeye
    const char* const join[][2] = { { " afro house ", " afrohouse " }, { " deep house ", " deephouse " }, { " tech house ", " techhouse " }, { " future bass ", " futurebass " },
                                    { " drum and bass ", " dnb " }, { " drum n bass ", " dnb " }, { " drum bass ", " dnb " }, { " melodic techno ", " melodictechno " }, { " lo fi ", " lofi " },
                                    { " big room ", " bigroom " }, { " uk garage ", " ukg " }, { " 2 step ", " ukg " }, { " yan zincir ", " sidechain " }, { " side chain ", " sidechain " },
                                    { " 8 bit ", " 8bit " }, { " super saw ", " supersaw " }, { " progressive house ", " progressive " }, { " hard style ", " hardstyle " } };
    for (auto& j : join) while (o.contains (j[0])) o = o.replace (j[0], j[1]);
    return o;
}

struct Scan
{
    juce::StringArray t;
    explicit Scan (const String& text) { t.addTokens (norm (text), " ", ""); t.removeEmptyStrings(); }
    static bool match (const String& tok, std::initializer_list<const char*> pats, bool* suffixNeg = nullptr)
    {
        for (auto* p : pats)
        {
            if (p[0] == '=') { if (tok == String (p + 1)) return true; continue; }
            if (tok.startsWith (p))
            {
                if (suffixNeg != nullptr) { const auto rest = tok.substring ((int) strlen (p)); *suffixNeg = rest.endsWith ("siz") || rest.endsWith ("suz") || rest.endsWith ("sizin") || rest.endsWith ("suz"); }
                return true;
            }
        }
        return false;
    }
    bool has (int i, std::initializer_list<const char*> pats) const { return i >= 0 && i < t.size() && match (t[i], pats); }
    float strengthAt (int i) const
    {
        float m = 1;
        for (int k = i - 1; k >= juce::jmax (0, i - 2); --k)
        {
            if (has (k, { "=hafif", "hafifce", "=biraz", "=az", "=slight", "slightly", "subtle", "=light", "=little", "=bit", "=gentle", "=tatli", "=minik" })) m = .55f;
            if (has (k, { "=cok", "asiri", "=very", "heavy", "heavily", "=yogun", "=agir", "extreme", "=bol", "=mega", "=fazla", "=ultra", "=lots" })) m = 1.4f;
        }
        return m;
    }
    bool negAt (int i) const
    {
        for (int k = i + 1; k <= i + 2; ++k) if (has (k, { "=yok", "olmasin", "olmadan", "istemiyorum", "istemem", "=degil", "haric", "=none", "=off", "=free" })) return true;
        for (int k = i - 1; k >= juce::jmax (0, i - 2); --k) if (has (k, { "=no", "=without", "=not", "=non", "=less", "=sans", "=never", "=hic" })) return true;
        return false;
    }
    // en güçlü eşleşme: 0 yok, >0 var (şiddet), <0 olumsuz
    float find (std::initializer_list<const char*> pats) const
    {
        float best = 0;
        for (int i = 0; i < t.size(); ++i)
        {
            bool sneg = false;
            if (! match (t[i], pats, &sneg)) continue;
            const float v = strengthAt (i) * ((negAt (i) || sneg) ? -1.0f : 1.0f);
            if (std::abs (v) > std::abs (best) || (best > 0 && v < 0)) best = v;
        }
        return best;
    }
    int index (std::initializer_list<const char*> pats) const { for (int i = 0; i < t.size(); ++i) if (match (t[i], pats)) return i; return -1; }
    bool isNum (int i) const { return i >= 0 && i < t.size() && t[i].containsOnly ("0123456789") && t[i].isNotEmpty(); }
};

int noteOf (const String& s)
{
    static const std::map<String, int> m { { "c", 0 }, { "d", 2 }, { "e", 4 }, { "f", 5 }, { "g", 7 }, { "a", 9 }, { "b", 11 },
                                           { "do", 0 }, { "re", 2 }, { "mi", 4 }, { "fa", 5 }, { "sol", 7 }, { "la", 9 }, { "si", 11 } };
    auto it = m.find (s); return it == m.end() ? -1 : it->second;
}
bool isMinor (const String& s) { return s.startsWith ("minor") || s.startsWith ("moll") || s == "min" || s == "m" || s.startsWith ("minore"); }
bool isMajor (const String& s) { return s.startsWith ("major") || s.startsWith ("majo") || s == "maj" || s == "dur"; }

// ton: "Fa diyez minör", "F# minor", "f#m", "Ebm", "A minor", "C major", "key of D"
bool parseKey (const Scan& S, int& key, int& mode)   // mode: 1 minör, 0 majör, -1 belirtilmedi
{
    for (int i = 0; i < S.t.size(); ++i)
    {
        String tok = S.t[i]; int acc = 0; bool attachedM = false;
        if (tok.length() >= 2 && tok.length() <= 3 && tok.endsWithChar ('m'))
        {   // "am", "f#m", "ebm"
            const auto r = tok.dropLastCharacters (1);
            if (noteOf (r) >= 0 || (r.length() == 2 && (r[1] == '#' || r[1] == 'b') && noteOf (r.substring (0, 1)) >= 0)) { attachedM = true; tok = r; }
        }
        if (tok.endsWithChar ('#')) { acc = 1; tok = tok.dropLastCharacters (1); }
        else if (tok.length() == 2 && tok[1] == 'b' && noteOf (tok.substring (0, 1)) >= 0) { acc = -1; tok = tok.substring (0, 1); }
        const int n = noteOf (tok); if (n < 0) continue;
        int j = i + 1;
        if (acc == 0 && S.has (j, { "=#", "diyez", "=sharp" })) { acc = 1; ++j; }
        else if (acc == 0 && S.has (j, { "bemol", "=flat" })) { acc = -1; ++j; }
        int md = attachedM ? 1 : -1;
        if (md < 0 && j < S.t.size()) { if (isMinor (S.t[j])) md = 1; else if (isMajor (S.t[j])) md = 0; }
        const bool keyWord = S.has (i - 1, { "=key", "=ton", "=in", "=tonu", "=tonda", "=tonunda" }) || S.has (j, { "=key", "=ton", "=tonu", "=tonda", "=tonunda", "tonlu" });
        const bool letter = tok.length() == 1;
        if (md >= 0 || keyWord || (letter && acc != 0))
        {
            key = ((n + acc) % 12 + 12) % 12; mode = md; return true;
        }
    }
    return false;
}

const std::map<String, String>& genreNames()
{
    static const std::map<String, String> g { { "afrohouse", "Afro House" }, { "deephouse", "Deep House" }, { "techhouse", "Tech House" }, { "futurebass", "Future Bass" },
                                              { "dnb", "Drum & Bass" }, { "melodictechno", "Melodic Techno" }, { "lofi", "Lo-fi" }, { "synthwave", "Synthwave" },
                                              { "progressive", "Progressive" }, { "techno", "Techno" }, { "trance", "Trance" }, { "dubstep", "Dubstep" }, { "trap", "Trap" },
                                              { "drill", "Drill" }, { "phonk", "Phonk" }, { "amapiano", "Amapiano" }, { "ukg", "UK Garage" }, { "hardstyle", "Hardstyle" },
                                              { "bigroom", "Big Room" }, { "minimal", "Minimal" }, { "house", "House" }, { "disco", "Nu Disco" }, { "ambient", "Ambient" } };
    return g;
}
String findGenre (const Scan& S)
{
    for (auto& tok : S.t)
    {
        String k = tok;
        if (k == "retrowave" || k == "outrun") k = "synthwave";
        if (k == "jungle" || k == "drumandbass" || k == "drumnbass") k = "dnb";
        if (k == "garage") k = "ukg";
        if (k == "nudisco") k = "disco";
        if (genreNames().count (k)) return k;
    }
    return {};
}
bool genreIs (const String& g, std::initializer_list<const char*> l) { for (auto* x : l) if (g == x) return true; return false; }

// ---------------------------------------------------------------- program yazımı (yol: "o.0.f.c", "env.1", "fx.2")
var* slot (var& root, const String& path)
{
    juce::StringArray parts; parts.addTokens (path, ".", ""); var* cur = &root;
    for (auto& p : parts)
    {
        if (p.containsOnly ("0123456789"))
        {
            auto* a = cur->getArray(); const int i = p.getIntValue(); if (a == nullptr) return nullptr;
            while (a->size() <= i) a->add (var());
            cur = &a->getReference (i);
        }
        else
        {
            auto* o = cur->getDynamicObject();
            if (o == nullptr) { *cur = var (new juce::DynamicObject()); o = cur->getDynamicObject(); }
            if (! o->hasProperty (p)) o->setProperty (p, var());
            cur = o->getProperties().getVarPointer (p);
        }
    }
    return cur;
}
void set (var& root, const String& path, const var& v) { if (auto* x = slot (root, path)) *x = v; }
var arr (std::initializer_list<double> l) { juce::Array<var> a; for (auto x : l) a.add (x); return a; }

int degreeSemis (int deg, bool minor)
{
    static const int mi[7] = { 0, 2, 3, 5, 7, 8, 10 }, ma[7] = { 0, 2, 4, 5, 7, 9, 11 };
    const int o = (int) std::floor (deg / 7.0), d = ((deg % 7) + 7) % 7;
    return o * 12 + (minor ? mi[d] : ma[d]);
}
}

String kindName (Kind k, const String& lang)
{
    switch (k)
    {
        case kBass: return L (lang, "Bas", "Bass");
        case kPad: return "Pad";
        case kChord: return L (lang, "Akor", "Chords");
        case kLead: return "Lead";
        case kPluck: return "Pluck";
        default: return L (lang, "Çan", "Bell");
    }
}
String keyName (int key, bool minor, const String& lang)
{
    static const char* const n[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return String (n[((key % 12) + 12) % 12]) + " " + (minor ? L (lang, "minör", "minor") : L (lang, "majör", "major"));
}

// ---------------------------------------------------------------- tarif -> tasarım
Design design (const String& text, const Context& C, uint32_t seed)
{
    const String& lg = C.lang; Design D; D.seed = seed; D.bpm = C.bpm; D.key = C.key; D.minor = C.minor; D.bars = C.bars;
    const Scan S (text);
    auto heard = [&] (const String& s) { D.heard.addIfNotAlreadyThere (s); };
    auto filled = [&] (const String& s) { D.filled.addIfNotAlreadyThere (s); };

    // tür (yazılmadıysa seçili parti)
    const float isBass = S.find ({ "bass", "=bas", "=basi", "=basa", "=basin", "=baslar", "=808", "=sub", "subbass", "reese", "bassline" });
    const float isPad = S.find ({ "pad", "atmosfer", "atmosphere", "ambient", "drone", "=texture", "=doku", "string", "yayli" });
    const float isChord = S.find ({ "akor", "chord", "stab", "=keys", "piyano", "piano", "organ", "rhodes", "=ep", "klavye" });
    const float isLead = S.find ({ "lead", "=lid", "melodi", "melody", "=solo", "=hook", "=synth" });
    const float isPluck = S.find ({ "pluck", "pluk", "pizz", "=pling" });
    const float isBell = S.find ({ "=bell", "bells", "=zil", "kalimba", "marimba", "mallet", "glock", "celesta" });
    const float arpWord = S.find ({ "arp", "arpej", "arpeg" });
    if (isBass > 0) D.kind = kBass; else if (isPluck > 0) D.kind = kPluck; else if (isBell > 0) D.kind = kBell; else if (isPad > 0) D.kind = kPad;
    else if (isChord > 0) D.kind = kChord; else if (isLead > 0) D.kind = kLead;
    else if (arpWord > 0) D.kind = C.part == 0 ? kChord : kPluck;
    else { D.kind = C.part == 1 ? kBass : C.part == 0 ? kChord : kLead; filled (kindName (D.kind, lg) + L (lg, " (seçili parti)", " (selected part)")); }
    D.part = D.kind == kBass ? 1 : (D.kind == kPad || D.kind == kChord) ? 0 : 2;
    if (D.filled.isEmpty()) heard (kindName (D.kind, lg));

    // tempo, ton, süre
    for (int i = 0; i < S.t.size(); ++i)
    {
        String tok = S.t[i]; double v = 0;
        if (tok.endsWith ("bpm") && tok.length() > 3 && tok.dropLastCharacters (3).containsOnly ("0123456789")) v = tok.dropLastCharacters (3).getDoubleValue();
        else if (S.isNum (i) && (S.has (i + 1, { "=bpm", "=tempo" }) || S.has (i - 1, { "=bpm", "=tempo" }))) v = tok.getDoubleValue();
        if (v >= 50 && v <= 220) { D.bpm = v; heard (String (juce::roundToInt (v)) + " BPM"); break; }
    }
    if (! D.heard.contains (String (juce::roundToInt (D.bpm)) + " BPM")) filled (String (juce::roundToInt (D.bpm)) + " BPM" + L (lg, " (proje)", " (project)"));
    int key = C.key, mode = -1;
    if (parseKey (S, key, mode)) { D.key = key; if (mode >= 0) D.minor = mode == 1; heard (keyName (D.key, D.minor, lg)); }
    else filled (keyName (D.key, D.minor, lg) + L (lg, " (proje)", " (project)"));
    for (int i = 0; i < S.t.size(); ++i)
    {
        if (! S.isNum (i)) continue; const double v = S.t[i].getDoubleValue();
        if (S.has (i + 1, { "olcu", "=bar", "=bars", "measure" })) { D.bars = v <= 1 ? 1 : v <= 2 ? 2 : v <= 4 ? 4 : 8; heard (String (D.bars) + L (lg, " ölçü", D.bars == 1 ? " bar" : " bars")); break; }
        if (S.has (i + 1, { "=sn", "saniye", "=sec", "=secs", "second", "=s" })) { const double b = v * D.bpm / 240.0; D.bars = b <= 1.4 ? 1 : b <= 2.8 ? 2 : b <= 5.6 ? 4 : 8; heard (String (juce::roundToInt (v)) + L (lg, " sn", " s")); break; }
    }

    // tarz
    D.genre = findGenre (S);
    if (D.genre.isNotEmpty()) heard (genreNames().at (D.genre));

    // sıfatlar
    struct Adj { float* to; float sign; std::initializer_list<const char*> pats; const char* tr; const char* en; };
    float dark = 0, bright = 0, cold = 0, lng = 0, shrt = 0, clean = 0, dry = 0, analog = 0, hollow = 0, acid = 0, reese = 0, growl = 0, is808 = 0, sub = 0, supersaw = 0;
    const Adj adj[] = {
        { &dark, 1, { "karanl", "=dark", "darker", "=koyu", "bogu", "kasvet", "gloomy", "murky", "muffled", "=moody", "=gizemli", "mysterious" }, "karanlık", "dark" },
        { &bright, 1, { "parlak", "=bright", "brighter", "berrak", "=crisp", "=shiny", "=net", "parilti" }, "parlak", "bright" },
        { &D.warm, 1, { "sicak", "=warm", "warmer", "=analog", "vintage", "nostalji", "=retro" }, "sıcak", "warm" },
        { &cold, 1, { "soguk", "=cold", "dijital", "digital", "steril" }, "soğuk", "cold" },
        { &D.deep, 1, { "derin", "=deep", "deeper", "subby", "=tok", "=boomy", "=heavy" }, "derin", "deep" },
        { &D.punch, 1, { "vurucu", "punch", "=sert", "=hard", "agresif", "aggressive", "perkusif", "percuss", "snap", "=knock", "=attack", "=atak", "=guclu", "=powerful" }, "vurucu", "punchy" },
        { &shrt, 1, { "=kisa", "kisacik", "=short", "staccato", "kesik", "=tight", "=siki", "=plucky" }, "kısa", "short" },
        { &lng, 1, { "=uzun", "=long", "sustain", "surekli", "legato", "akici", "flowing", "evolving", "gelisen" }, "uzun", "long" },
        { &D.soft, 1, { "yumusak", "=soft", "smooth", "nazik", "=mellow", "=round", "yuvarlak", "puruzsuz", "=sakin", "=calm" }, "yumuşak", "soft" },
        { &D.wide, 1, { "genis", "=wide", "stereo", "=huge", "=dev", "=buyuk", "=epic", "epik", "=big" }, "geniş", "wide" },
        { &D.fat, 1, { "kalin", "=fat", "dolgun", "=thick", "massive", "=dolu" }, "kalın", "fat" },
        { &D.dirt, 1, { "distort", "distors", "overdrive", "=drive", "saturas", "satur", "crunch", "gritty", "=grit", "=fuzz", "=kirli", "=dirty", "bozuk", "=raspy", "=tube", "=tup", "hirpal" }, "distortion", "distortion" },
        { &clean, 1, { "temiz", "=clean", "=pure", "=saf" }, "temiz", "clean" },
        { &analog, 1, { "analog", "=tape", "=teyp", "=tube", "=tup", "vintage" }, "analog", "analog" },
        { &D.space, 1, { "reverb", "yanki", "=hall", "ferah", "=uzay", "=space", "spacious", "=airy", "havadar", "ethereal", "=ruya", "dreamy", "=room" }, "alan (reverb)", "space (reverb)" },
        { &dry, 1, { "=kuru", "=dry" }, "kuru", "dry" },
        { &D.delay, 1, { "delay", "=eko", "=echo", "gecikme" }, "delay", "delay" },
        { &D.glide, 1, { "glide", "=slide", "kayan", "kayma", "portamento", "=porta", "sliding" }, "glide", "glide" },
        { &D.metal, 1, { "metal", "=fm", "=cam", "=glass", "glassy", "kristal", "crystal" }, "metalik", "metallic" },
        { &hollow, 1, { "=kare", "=square", "hollow", "=oyuk", "chiptune", "=chip", "8bit", "=gameboy" }, "kare dalga", "square" },
        { &acid, 1, { "acid", "=303", "squelch", "=asit" }, "acid", "acid" },
        { &reese, 1, { "reese", "=neuro" }, "reese", "reese" },
        { &growl, 1, { "growl", "hirla", "=yell" }, "growl", "growl" },
        { &is808, 1, { "=808" }, "808", "808" },
        { &sub, 1, { "=sub", "subbass" }, "sub", "sub" },
        { &supersaw, 1, { "supersaw" }, "supersaw", "supersaw" },
    };
    for (auto& a : adj)
    {
        const float v = S.find (a.pats); if (v == 0) continue;
        *a.to = juce::jlimit (-1.0f, 1.0f, v * a.sign);
        if (a.to == &is808 || a.to == &sub) continue;   // türle zaten yazıldı
        const String nm = L (lg, a.tr, a.en);
        heard (v < 0 ? L (lg, "yok: ", "no ") + nm : (v < .7f ? L (lg, "hafif ", "slight ") + nm : nm));
    }
    // modülasyon ve yerleşim
    const float move = S.find ({ "=lfo", "hareket", "movement", "=moving", "ritmik", "rhythmic", "pulse", "pulsing", "nabiz", "dalgal", "titres", "modulas", "modulat", "canli", "=alive" });
    const float wob = S.find ({ "wobble", "=wub", "=wobbly", "yalpal" });
    const float mono = S.find ({ "=mono" });
    float side = S.find ({ "sidechain", "pump", "ducking", "=duck" });
    {   // "kick'e yer bıraksın", "leave room for the kick"
        const int k = S.index ({ "kick" });
        if (k >= 0) for (int j = k - 3; j <= k + 3; ++j) if (S.has (j, { "=yer", "=room", "=space", "=alan", "birak", "=leave", "=make" })) side = juce::jmax (side, .6f);
    }
    const bool lowClean = (mono > 0) || (S.index ({ "=alt" }) >= 0 && (clean > 0 || mono > 0)) || (S.index ({ "=low", "=lows", "=bottom" }) >= 0 && (clean > 0 || mono > 0));
    if (wob > 0) { D.wobble = true; D.lfo = true; heard ("wobble"); }
    if (move > 0) { D.lfo = true; heard (L (lg, "ritmik LFO", "rhythmic LFO")); }
    else if (move < 0) { D.lfo = false; heard (L (lg, "LFO yok", "no LFO")); }
    if (arpWord > 0) { D.arp = true; heard (L (lg, "arpej", "arpeggio")); }
    if (side > 0) { D.side = juce::jlimit (.3f, 1.0f, side * .7f); heard (L (lg, "kick'e yer (sidechain)", "room for the kick (sidechain)")); }
    else if (side < 0) { D.side = -1; heard (L (lg, "sidechain yok", "no sidechain")); }
    if (lowClean || mono > 0) { D.mono = 120; heard (L (lg, "mono ve temiz alt frekans", "mono, clean low end")); }

    // yorum: zıt sıfatlar
    D.bright = juce::jlimit (-1.0f, 1.0f, bright - dark - .25f * D.soft - .15f * juce::jmax (0.0f, D.warm) + .3f * cold);
    D.shortness = juce::jlimit (-1.0f, 1.0f, shrt - lng);
    if (dry > 0) D.space = -1;                       // "kuru": otomatik reverb da eklenmez
    if (clean > 0 && D.dirt == 0) D.dirt = -1;        // "temiz": otomatik distortion eklenmez
    D.dirtMode = analog > 0 || D.warm > 0 ? 2 : D.punch > .8f ? 5 : 0;
    if (D.dirt > 0 && analog > 0)
    {   // "hafif analog distortion" tek etiket
        for (int i = D.heard.size(); --i >= 0;) if (D.heard[i].endsWith ("analog")) D.heard.remove (i);
        for (auto& h : D.heard) if (h.endsWith ("distortion")) h = h.replace ("distortion", "analog distortion");
    }
    if (D.mono > 0) D.heard.removeString (L (lg, "temiz", "clean"));   // "mono ve temiz alt frekans" etiketi kapsar
    if (hollow > 0) D.metal = juce::jmin (D.metal, 0.0f);

    // tarzın tipik ayarları (yazılmayanlar)
    const String& g = D.genre; auto fillIf = [&] (float& f, float v, const char* t, const char* e) { if (f == 0) { f = v; filled (L (lg, t, e)); } };
    if (D.kind == kBass)
    {
        if (genreIs (g, { "afrohouse", "deephouse", "house", "techhouse", "ukg", "disco", "amapiano", "melodictechno", "progressive", "minimal" }))
        { if (side == 0) { D.side = .35f; filled (L (lg, "hafif sidechain", "light sidechain")); } if (D.mono == 0) { D.mono = 120; filled (L (lg, "mono alt", "mono lows")); } }
        if (D.mono == 0) { D.mono = 100; filled (L (lg, "mono alt", "mono lows")); }
        if (genreIs (g, { "afrohouse", "deephouse" })) fillIf (D.warm, .5f, "sıcak ton", "warm tone");
        if (genreIs (g, { "techhouse", "techno", "minimal" })) fillIf (D.punch, .6f, "vurucu atak", "punchy attack");
        if (genreIs (g, { "trap", "drill", "phonk" })) { fillIf (D.glide, .7f, "808 glide", "808 glide"); fillIf (D.dirt, .35f, "hafif sürüş", "light drive"); }
        if (genreIs (g, { "dubstep" }) && ! D.lfo) { D.wobble = D.lfo = true; filled ("wobble LFO"); }
    }
    if (D.kind == kPad) { fillIf (D.space, .55f, "reverb alanı", "reverb space"); fillIf (D.wide, .7f, "geniş stereo", "wide stereo"); }
    if (D.kind == kChord && genreIs (g, { "futurebass", "progressive", "bigroom", "trance" })) { fillIf (D.side, .7f, "sidechain pompası", "sidechain pump"); fillIf (D.wide, .6f, "geniş stereo", "wide stereo"); }
    if (D.kind == kChord && genreIs (g, { "deephouse", "house", "ukg", "afrohouse" })) fillIf (D.space, .25f, "hafif reverb", "light reverb");
    if ((D.kind == kLead || D.kind == kPluck || D.kind == kBell) && D.space == 0) { D.space = .3f; filled (L (lg, "hafif reverb", "light reverb")); }
    if ((D.kind == kPluck || D.kind == kBell) && D.delay == 0 && genreIs (g, { "trance", "progressive", "melodictechno", "deephouse", "afrohouse", "synthwave", "lofi", "ambient" })) { D.delay = .35f; filled (L (lg, "senkron delay", "synced delay")); }
    if (genreIs (g, { "synthwave", "lofi", "disco" })) fillIf (D.warm, .5f, "analog sıcaklık", "analog warmth");
    if (genreIs (g, { "hardstyle", "dubstep", "phonk" }) && D.dirt == 0) { D.dirt = .5f; filled (L (lg, "distortion", "distortion")); }

    // olumsuzlar ve "kuru/temiz" yalnızca otomatik doldurmayı engeller
    D.dirt = juce::jmax (0.0f, D.dirt); D.space = juce::jmax (0.0f, D.space); D.delay = juce::jmax (0.0f, D.delay); D.side = juce::jmax (0.0f, D.side); D.glide = juce::jmax (0.0f, D.glide);

    // mimari
    const uint32_t v = seed > 1 ? seed - 1 : 0;   // yeniden üretme: yazılı karakter kalır, mimari ve küçük ayrıntılar değişir
    auto pickArch = [&] (std::initializer_list<int> alts, bool explicitly) { if (explicitly || v == 0) return *alts.begin(); return *(alts.begin() + (int) (v % alts.size())); };
    switch (D.kind)
    {
        case kBass:
            if (acid > 0) D.arch = pickArch ({ 3 }, true);
            else if (reese > 0) D.arch = pickArch ({ 2 }, true);
            else if (is808 > 0) D.arch = pickArch ({ 4 }, true);
            else if (D.wobble && ! (move > 0 && wob <= 0)) D.arch = pickArch ({ 5, 6 }, wob > 0);
            else if (growl > 0) D.arch = pickArch ({ 6 }, true);
            else if (sub > 0) D.arch = pickArch ({ 1 }, true);
            else if (genreIs (g, { "amapiano" })) D.arch = pickArch ({ 7, 0, 1 }, false);
            else if (genreIs (g, { "trap", "drill", "phonk" })) D.arch = pickArch ({ 4, 1 }, false);
            else if (genreIs (g, { "dnb" })) D.arch = pickArch ({ 2, 6 }, false);
            else if (genreIs (g, { "dubstep", "hardstyle" })) D.arch = pickArch ({ 6, 5 }, false);
            else if (genreIs (g, { "techno", "trance", "melodictechno", "minimal" })) D.arch = pickArch ({ 9, 8, 0 }, false);
            else if (genreIs (g, { "techhouse", "synthwave", "disco" })) D.arch = pickArch ({ 8, 0, 9 }, false);
            else if (D.dirt > .6f && D.bright > .2f) D.arch = pickArch ({ 2, 8 }, false);
            else D.arch = pickArch ({ 0, 8, 1 }, false);
            break;
        case kPad: D.arch = supersaw > 0 ? 1 : pickArch ({ D.bright < -.3f ? 2 : D.metal > 0 ? 3 : 0, 1, 0, 3 }, false); break;
        case kChord:
            if (S.find ({ "organ" }) > 0) D.arch = 1; else if (S.find ({ "piyano", "piano", "=keys", "rhodes", "=ep" }) > 0) D.arch = 2; else if (supersaw > 0) D.arch = 3;
            else D.arch = pickArch ({ genreIs (g, { "futurebass", "trance", "bigroom", "progressive" }) ? 3 : genreIs (g, { "deephouse", "ukg" }) ? 1 : genreIs (g, { "minimal", "techno", "melodictechno" }) ? 4 : 0, 0, 2, 3 }, false);
            break;
        case kLead:
            if (supersaw > 0) D.arch = 0; else if (hollow > 0) D.arch = 1; else if (D.metal > 0) D.arch = 4; else if (D.dirt > .5f || growl > 0) D.arch = 3; else if (D.soft > 0) D.arch = 2;
            else D.arch = pickArch ({ genreIs (g, { "trance", "bigroom", "hardstyle", "progressive" }) ? 0 : genreIs (g, { "synthwave", "lofi" }) ? 1 : 0, 1, 2, 4 }, false);
            break;
        case kPluck: D.arch = hollow > 0 ? 1 : D.metal > 0 ? 2 : pickArch ({ 0, 1, 2 }, false); break;
        default: D.arch = pickArch ({ D.metal < 0 ? 1 : 0, 1 }, false); break;
    }
    if (D.lfo || D.arp) D.rate = defaultRate (D.kind);
    if (D.wobble) D.rate = genreIs (g, { "dubstep" }) ? 3 : 3;
    // metindeki hız: "1/16", "1/8T"
    for (int i = 0; i < S.t.size(); ++i) for (int r = 0; r < kNumRates; ++r) if (S.t[i] == String (kRateNames[r]).toLowerCase()) { D.rate = r; heard (L (lg, "hız ", "rate ") + kRateNames[r]); }
    D.title = (D.genre.isNotEmpty() ? genreNames().at (D.genre) + " " : String()) + kindName (D.kind, "en");
    return D;
}

// ---------------------------------------------------------------- tasarım -> Synth X programı
var build (const Design& D, double sr)
{
    var P = xs::Program::toVar (xs::Program::init());
    juce::Random R ((juce::int64) D.seed * 7919 + D.arch);
    auto j = [&] (double a, double b) { return D.seed <= 1 ? (a + b) * .5 : a + (b - a) * R.nextDouble(); };   // ilk üretim ortalama, yeniden üretme hafif değişken
    struct O { int t = 0, a = 0, b = 1, u = 1, oc = 0, se = 0, sm = 0, pm = 0; double d = .2, sp = .7, lv = .8, fi = 0, fm = 0, fr = 1, sh = 0, pw = .5, pa = 0; int ft = 1, sat = 0; double res = .15, fe = 0, dr = 0, kt = .4; };
    std::array<O, 3> o {};
    double cut = 2000, atk = .003, dec = .4, sus = .6, rel = .25, fDec = .25, fSus = 0, pitchEnv = 0, pitchDec = .05, glide = 0;
    double vibrato = 0;
    const float br = D.bright, warm = juce::jmax (0.0f, D.warm), punch = juce::jmax (0.0f, D.punch), deep = juce::jmax (0.0f, D.deep), soft = juce::jmax (0.0f, D.soft);
    switch (D.kind)
    {
        case kBass:
            sus = .55; dec = .4; rel = .1; atk = .002;
            switch (D.arch)
            {
                case 1: o[0] = { 1, 3 }; o[1] = { 1, 2 }; o[1].lv = .25 + .3 * warm; cut = 420; break;                                                     // sub
                case 2: o[0] = { 1, 0 }; o[0].u = 4; o[0].d = .28; o[1] = { 1, 0 }; o[1].u = 3; o[1].d = .38; o[1].fi = .12; o[1].lv = .7; o[0].ft = o[1].ft = 2; cut = 950; o[0].res = o[1].res = .22; break;   // reese
                case 3: o[0] = { 1, D.metal < 0 ? 1 : 0 }; o[0].ft = 2; o[0].res = .72; o[0].fe = .75; cut = 360; fDec = .18; sus = .7; glide = .07; break;      // acid
                case 4: o[0] = { 1, 3 }; o[1] = { 1, 2 }; o[1].lv = .2; cut = 600; sus = .85; dec = 1.4; rel = .35; pitchEnv = .28; pitchDec = .06; glide = .08; break;   // 808
                case 5: o[0] = { 1, 0 }; o[0].u = 3; o[0].d = .2; o[1] = { 1, 1 }; o[1].lv = .6; o[0].ft = o[1].ft = 2; o[0].res = o[1].res = .5; cut = 650; sus = .9; o[2] = { 1, 3 }; o[2].lv = .55; o[2].ft = 0; break;   // wobble: filtreli testere+kare, temiz sinüs sub
                case 6: o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .55; o[0].fr = 2; o[1] = { 1, 0 }; o[1].lv = .55; o[0].ft = o[1].ft = 2; o[0].res = o[1].res = .35; cut = 1200; sus = .9; break;   // growl (FM)
                case 7: o[0] = { 1, 3 }; o[1] = { 1, 2 }; o[1].oc = 1; o[1].lv = .18; cut = 900; dec = .45; sus = 0; rel = .2; pitchEnv = -.1; pitchDec = .12; break;          // log drum
                case 8: o[0] = { 1, 0 }; o[1] = { 1, 1 }; o[1].oc = -1; o[1].lv = .45; o[0].ft = o[1].ft = 2; o[0].res = o[1].res = .25; o[0].fe = o[1].fe = .55; cut = 260; fDec = .16; dec = .3; sus = .35; break;   // pluck bas
                case 9: o[0] = { 1, 0 }; o[0].u = 2; o[0].d = .12; o[0].ft = 3; o[0].res = .3; o[0].fe = .4; cut = 320; fDec = .12; dec = .22; sus = .15; rel = .06; break;        // rolling
                default: o[0] = { 1, 2 }; o[1] = { 1, 0 }; o[1].lv = .42; o[0].ft = o[1].ft = 3; o[0].res = o[1].res = .18; o[0].fe = o[1].fe = .35; cut = 720; fDec = .2; dec = .38; sus = .45; break;   // yuvarlak, sıcak
            }
            if (deep > 0 && D.arch != 1 && D.arch != 4 && D.arch != 5 && D.arch != 7) { o[2] = { 1, 3 }; o[2].lv = .35 + .4 * deep; o[2].ft = 1; }   // temel frekansı güçlendiren sinüs (oktav altı değil: duyulmaz)
            break;
        case kPad:
            atk = .55; dec = 1.4; sus = .82; rel = 1.6;
            switch (D.arch)
            {
                case 1: o[0] = { 1, 0 }; o[0].u = 7; o[0].d = .3; o[0].sp = .95; o[1] = { 1, 0 }; o[1].u = 5; o[1].oc = 1; o[1].lv = .4; o[1].d = .25; cut = 3200; break;
                case 2: o[0] = { 1, 0 }; o[0].u = 3; o[0].d = .15; o[1] = { 1, 1 }; o[1].oc = -1; o[1].lv = .5; o[0].ft = o[1].ft = 2; cut = 700; break;
                case 3: o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .22; o[0].fr = 3; o[1] = { 1, 2 }; o[1].u = 4; o[1].oc = 1; o[1].lv = .4; cut = 5200; break;
                default: o[0] = { 1, 2 }; o[0].u = 4; o[0].d = .16; o[1] = { 1, 0 }; o[1].u = 4; o[1].d = .22; o[1].lv = .55; o[0].ft = o[1].ft = 3; cut = 2200; break;
            }
            break;
        case kChord:
            atk = .004; dec = .45; sus = .25; rel = .3;
            switch (D.arch)
            {
                case 1: o[0] = { 1, 5 }; o[1] = { 1, 3 }; o[1].oc = 1; o[1].lv = .25; cut = 3800; dec = .5; sus = .4; break;                          // organ
                case 2: o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .28; o[0].fr = 1; o[1] = { 1, 2 }; o[1].lv = .35; cut = 4200; dec = 1.1; sus = .15; rel = .45; break;   // elektrik piyano
                case 3: o[0] = { 1, 0 }; o[0].u = 7; o[0].d = .32; o[0].sp = .95; o[1] = { 1, 0 }; o[1].u = 5; o[1].oc = 1; o[1].lv = .35; cut = 4500; sus = .75; dec = .8; rel = .35; break;   // supersaw akor
                case 4: o[0] = { 1, 0 }; o[1] = { 1, 1 }; o[1].lv = .5; o[0].ft = o[1].ft = 3; o[0].res = o[1].res = .35; o[0].fe = o[1].fe = .4; cut = 700; fDec = .12; dec = .25; sus = 0; break;   // dub akor
                default: o[0] = { 1, 0 }; o[0].u = 5; o[0].d = .22; o[1] = { 1, 1 }; o[1].lv = .45; o[0].ft = o[1].ft = 3; o[0].fe = o[1].fe = .45; cut = 1300; fDec = .22; break;   // stab
            }
            break;
        case kLead:
            atk = .006; dec = .6; sus = .78; rel = .3; vibrato = .12;
            switch (D.arch)
            {
                case 1: o[0] = { 1, 4 }; o[0].pm = 1; o[0].pw = .32; o[1] = { 1, 1 }; o[1].oc = -1; o[1].lv = .35; cut = 3600; break;
                case 2: o[0] = { 1, 3 }; o[1] = { 1, 2 }; o[1].oc = 1; o[1].lv = .3; cut = 4000; atk = .02; break;
                case 3: o[0] = { 1, 0 }; o[0].pm = 3; o[0].pw = .65; o[1] = { 1, 0 }; o[1].u = 3; o[1].d = .18; o[1].lv = .5; cut = 5200; o[0].res = .3; break;
                case 4: o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .45; o[0].fr = 2; o[1] = { 1, 4 }; o[1].lv = .3; cut = 5000; break;
                default: o[0] = { 1, 0 }; o[0].u = 7; o[0].d = .26; o[0].sp = .85; o[1] = { 1, 0 }; o[1].u = 5; o[1].oc = 1; o[1].lv = .35; o[1].d = .2; cut = 6500; break;
            }
            break;
        case kPluck:
            atk = .001; dec = .32; sus = 0; rel = .28; fDec = .14;
            switch (D.arch)
            {
                case 1: o[0] = { 1, 1 }; o[1] = { 1, 4 }; o[1].oc = 1; o[1].lv = .3; o[0].fe = o[1].fe = .6; cut = 900; break;
                case 2: o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .5; o[0].fr = 4; o[0].fe = .3; cut = 3000; dec = .5; break;
                default: o[0] = { 1, 0 }; o[0].u = 3; o[0].d = .16; o[1] = { 1, 1 }; o[1].lv = .35; o[0].fe = o[1].fe = .65; cut = 800; break;
            }
            break;
        default:
            atk = .001; dec = 1.6; sus = 0; rel = 1.1;
            if (D.arch == 1) { o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .35; o[0].fr = 7; o[1] = { 1, 3 }; o[1].oc = 2; o[1].lv = .2; cut = 9000; }
            else { o[0] = { 1, 3 }; o[0].sm = 1; o[0].fm = .45; o[0].fr = 3.5; o[1] = { 1, 2 }; o[1].oc = 1; o[1].lv = .25; cut = 7000; }
            break;
    }

    // karakter
    cut *= std::pow (2.0, br * 1.7 + j (-.15, .15));
    if (soft > 0) { atk = juce::jmax (atk, D.kind == kBass ? .006 : .015 + .1 * soft); }
    if (punch > 0)
    {
        atk = juce::jmin (atk, .0015); fDec = juce::jmin (fDec, .08 + .12 * (1 - punch));
        for (auto& x : o) if (x.t != 0) x.fe = juce::jmax (x.fe, .25 + .4 * punch);
        if (D.kind == kBass && pitchEnv == 0) { pitchEnv = .03 + .05 * punch; pitchDec = .025; }
        if (D.kind == kChord || D.kind == kPluck) { dec = juce::jmin (dec, .35); }
    }
    const float s = D.shortness;
    if (s > 0) { dec *= std::pow (2.0, -1.4 * s); sus *= 1 - .85 * s; rel = juce::jmin (rel, D.kind == kBass ? .06 : .12 + .2 * (1 - s)); }
    else if (s < 0) { sus = juce::jmax (sus, .7); rel *= std::pow (2.0, -s * 1.3); dec *= std::pow (2.0, -s); }
    if (warm > 0) for (int k = 0; k < 3; ++k) if (o[(size_t) k].t != 0) { o[(size_t) k].sat = 3; o[(size_t) k].dr = juce::jmax (o[(size_t) k].dr, .12 + .2 * warm); }
    if (D.fat > 0 || D.wide > 0)
        for (auto& x : o) if (x.t != 0 && x.a != 3)
        {
            const int add = (int) std::lround (2 + 3 * juce::jmax (D.fat, D.wide));
            x.u = juce::jlimit (1, D.kind == kBass ? 4 : 9, x.u + add); x.d = juce::jlimit (.1, .45, x.d + .08 * D.fat); x.sp = juce::jlimit (.5, 1.0, .7 + .3 * D.wide);
        }
    if (D.metal > 0) for (auto& x : o) if (x.t != 0 && x.sm == 0) { x.sm = 1; x.fm = .2 + .25 * D.metal; x.fr = D.kind == kBass ? 2 : 3.5; break; }
    for (auto& x : o) { x.d = juce::jlimit (0.0, 1.0, x.d + j (-.03, .03)); x.res = juce::jlimit (0.0, .9, x.res + j (-.04, .04)); }
    if (D.advCut > 0) cut = D.advCut;
    if (D.advAtk >= 0) atk = D.advAtk;
    if (D.advRel >= 0) rel = D.advRel;
    cut = juce::jlimit (60.0, 18000.0, cut);

    for (int k = 0; k < 3; ++k)
    {
        const auto& x = o[(size_t) k]; const String b = "o." + String (k) + ".";
        set (P, b + "t", x.t); if (x.t == 0) continue;
        set (P, b + "a", x.a); set (P, b + "b", x.a); set (P, b + "u", x.u); set (P, b + "d", x.d); set (P, b + "sp", D.kind == kBass ? juce::jmin (x.sp, .4) : x.sp);
        set (P, b + "oc", x.oc); set (P, b + "se", x.se); set (P, b + "fi", x.fi); set (P, b + "lv", x.lv); set (P, b + "pa", x.pa);
        set (P, b + "sm", x.sm); set (P, b + "fm", x.fm); set (P, b + "fr", x.fr); set (P, b + "sh", x.sh); set (P, b + "pm", x.pm); set (P, b + "pw", x.pw);
        set (P, b + "f.t", x.ft); set (P, b + "f.c", cut * (k == 2 ? .7 : 1.0)); set (P, b + "f.r", D.advRes >= 0 ? (double) D.advRes : x.res); set (P, b + "f.e", x.fe); set (P, b + "f.k", x.kt);
        set (P, b + "f.sa", x.sat); set (P, b + "f.dr", x.dr); set (P, b + "ph", D.kind == kBass ? 1 : 0);
    }
    set (P, "env.0", arr ({ atk, dec, sus, rel, 0 }));
    set (P, "env.1", arr ({ .001, fDec, fSus, juce::jmax (.05, rel), 0 }));
    if (pitchEnv != 0) { set (P, "env.2", arr ({ .0005, pitchDec, 0, pitchDec, .4 })); set (P, "ed.1.0", arr ({ (double) xs::dPitch, pitchEnv })); }
    // LFO 1: ritmik hareket (tempoya kilitli); LFO 2: lead vibratosu
    if (D.lfo)
    {
        const double amt = D.wobble ? .55 : D.kind == kBass ? .3 : D.kind == kPad ? .22 : D.kind == kBell ? 0 : .25;
        set (P, "lfo.0.w", D.wobble ? 0 : D.kind == kPad ? 1 : 0); set (P, "lfo.0.sy", 1); set (P, "lfo.0.r", rateLfoIndex (D.rate)); set (P, "lfo.0.rt", D.kind == kPad ? 0 : 1);
        int slotI = 0;
        if (D.kind == kBell) set (P, "lfo.0.d.0", arr ({ (double) xs::dAmp, -.35 }));
        else for (int k = 0; k < 3 && slotI < 4; ++k) if (o[(size_t) k].t != 0 && o[(size_t) k].ft != 0) set (P, "lfo.0.d." + String (slotI++), arr ({ (double) (k * xs::kOscDests + 11), amt }));
        if (D.wobble && o[0].sm == 1) set (P, "lfo.0.d." + String (slotI), arr ({ 9.0, .4 }));
    }
    if (vibrato > 0 && D.kind == kLead) { set (P, "lfo.1.w", 0); set (P, "lfo.1.r", 5.2); set (P, "lfo.1.fa", .35); set (P, "lfo.1.d.0", arr ({ (double) xs::dPitch, .006 })); }
    if (D.arp)
    {
        const int md = D.kind == kChord || D.kind == kPad ? 2 : 0, oc = D.kind == kBass ? 1 : D.kind == kLead ? 1 : 2;
        set (P, "arp.on", 1); set (P, "arp.md", md); set (P, "arp.dv", rateArpDiv (D.rate)); set (P, "arp.oc", oc); set (P, "arp.gt", D.kind == kBass ? .45 : .6);
    }
    glide = juce::jmax (glide, D.glide > 0 ? .05 + .08 * D.glide : 0.0);
    set (P, "gl", glide); set (P, "glm", glide > 0 ? 1 : 0);
    // efekt rafı
    int fxI = 0; auto fx = [&] (int type, double a, double b, double c, double d, double mix) { if (fxI < xs::kFxSlots) set (P, "fx." + String (fxI++), arr ({ (double) type, a, b, c, d, mix })); };
    if (D.dirt > 0)
    {
        const double mode = (D.dirtMode + .5) / 11.99;   // dağıtım kipi: 0 yumuşak, 2 analog (asimetrik), 5 sert
        fx (1, juce::jlimit (.05, .9, .12 + .45 * D.dirt), mode, D.kind == kBass ? .52 : .62, .5 - .12 * D.dirt, juce::jlimit (.2, 1.0, .35 + .45 * D.dirt));
    }
    if (D.kind == kBass && D.mono > 0) fx (10, .5, .5, .3, .48 - .06 * juce::jmax (0.0f, D.dirt), 1);   // temiz alt: tiz hafif yumuşatılır
    if ((D.kind == kPad || D.kind == kChord || D.kind == kLead) && D.wide > .3f) fx (7, .3, .45, .9, .55, .3 + .2 * D.wide);   // ensemble
    if (D.delay > 0) fx (11, (5 + .5) / 7.99, .25 + .3 * D.delay, 1, .55, .12 + .22 * D.delay);   // 1/8 noktalı yerine 1/8 ping-pong
    if (D.space > 0) fx (12, .35 + .5 * D.space, .5, D.kind == kBass ? .2 : 1, .12, (D.kind == kBass ? .06 : .1) + .3 * D.space);
    if (D.side > 0) fx (13, juce::jlimit (.2, .95, .35 + .55 * D.side), (3 + .5) / 7.99, .3, 0, 1);   // her vuruşta (kick) kısılır
    set (P, "mono", D.kind == kBass ? (double) D.mono : 0.0); set (P, "own", 1); set (P, "cat", kindName (D.kind, "en"));
    set (P, "vol", .7);
    set (P, "vol", xs::calibratedVolume (P, D.part, sr > 0 ? sr : 48000.0));
    return P;
}

// ---------------------------------------------------------------- ön dinleme cümlesi
std::vector<Note> phrase (const Design& D)
{
    std::vector<Note> out; const int bars = juce::jlimit (1, 8, D.bars); const String& g = D.genre;
    static const int progMinor[4] = { 0, 5, 2, 6 }, progMajor[4] = { 0, 4, 5, 3 };   // i-VI-III-VII / I-V-vi-IV
    auto chordRoot = [&] (int b) { return (D.minor ? progMinor : progMajor)[b % 4]; };
    auto add = [&] (double beat, double len, int note, int vel) { out.push_back ({ beat, len, juce::jlimit (12, 115, note), juce::jlimit (1, 127, vel) }); };
    const int pc = ((D.key % 12) + 12) % 12;
    if (D.kind == kBass)
    {
        const int base = 36 + pc;   // kök E1 (41 Hz) ile E2 arasında: duyulur ve kick'in altına inmez
        for (int b = 0; b < bars; ++b)
        {
            int r = base + degreeSemis (chordRoot (b), D.minor); while (r > 40) r -= 12; while (r < 28) r += 12;
            const int fifth = r + 7, oct = r + 12, f5 = r - 5 >= 28 ? r - 5 : r + 7; const double B = b * 4;   // f5: alttaki beşli (çok derine inmez)
            if (D.arp) { for (double t = 0; t < 4; t += kRateBeats[D.rate]) { const int i = (int) std::lround (t / kRateBeats[D.rate]); add (B + t, kRateBeats[D.rate] * .45, i % 3 == 0 ? r : i % 3 == 1 ? fifth : oct, i % 2 ? 92 : 108); } continue; }
            switch (D.arch)
            {
                case 4: case 1: if (genreIs (g, { "trap", "drill", "phonk" }) || D.arch == 4) { add (B, 1.5, r, 112); add (B + 1.75, .5, r, 100); add (B + 2.5, 1.25, b % 2 ? oct : f5, 104); }
                                else { add (B + .5, 1.2, r, 105); add (B + 2.5, 1.2, r, 105); } break;
                case 3: { const int seq[16] = { 0, 0, 12, 0, 3, 0, 7, 0, 0, 12, 0, 10, 0, 3, 0, -2 }; for (int i = 0; i < 16; ++i) add (B + i * .25, i % 3 == 1 ? .3 : .16, r + (D.minor ? seq[i] : (seq[i] == 3 ? 4 : seq[i])), i % 4 == 0 ? 120 : 86); } break;
                case 5: case 6: add (B, 2, r, 110); add (B + 2, 1.5, b % 2 ? f5 : r, 104); add (B + 3.5, .5, oct, 96); break;
                case 9: for (int q = 0; q < 4; ++q) for (int s = 1; s < 4; ++s) add (B + q + s * .25, .2, s == 2 && q == 3 ? oct : r, s == 2 ? 112 : 88); break;
                case 7: add (B + .75, .5, r, 112); add (B + 1.5, .5, r + 12, 96); add (B + 2.75, .4, f5, 104); add (B + 3.5, .4, r, 100); break;
                case 2: add (B, 1.5, r, 105); add (B + 1.5, .5, r, 95); add (B + 2.5, 1.5, b % 2 ? r + 3 : r, 105); break;
                default:
                    if (genreIs (g, { "afrohouse", "amapiano" }))      // afro: senkoplu, vuruş başlarını kick'e bırakır
                    { add (B + .75, .25, r, 110); add (B + 1.5, .25, r, 96); add (B + 2.25, .5, r, 104); add (B + 3.0 + .5, .25, b % 2 ? f5 : oct, 100); add (B + 3.75, .2, f5, 90); }
                    else if (genreIs (g, { "deephouse", "ukg", "disco" })) { add (B + .5, .4, r, 108); add (B + 1.75, .25, r, 92); add (B + 2.5, .4, r, 104); add (B + 3.25, .25, oct, 92); add (B + 3.5, .4, f5, 98); }
                    else for (int q = 0; q < 4; ++q) add (B + q + .5, .35, q == 3 && b % 2 ? oct : r, 104);   // off-beat
                    break;
            }
        }
    }
    else if (D.kind == kPad || D.kind == kChord)
    {
        for (int b = 0; b < bars; ++b)
        {
            const int d = chordRoot (b); std::vector<int> notes;
            for (int t : { 0, 2, 4, 6 }) { int n = 48 + pc + degreeSemis (d + t, D.minor); while (n > 67) n -= 12; while (n < 52) n += 12; if (t == 6 && (D.kind == kPad || D.arch == 3)) continue; notes.push_back (n); }
            const double B = b * 4;
            if (D.arp) { std::sort (notes.begin(), notes.end()); int i = 0; for (double t = 0; t < 4; t += kRateBeats[D.rate], ++i) { const int k = i % ((int) notes.size() * 2 - 2 > 0 ? (int) notes.size() * 2 - 2 : 1); const int idx = k < (int) notes.size() ? k : (int) notes.size() * 2 - 2 - k; add (B + t, kRateBeats[D.rate] * .6, notes[(size_t) idx] + (i / 8 % 2) * 12, i % 4 == 0 ? 104 : 88); } continue; }
            if (D.kind == kPad || D.arch == 3) for (int n : notes) add (B, 4, n, 90);
            else if (D.arch == 2) { for (int n : notes) add (B, 1.5, n, 96); for (int n : notes) add (B + 2.5, 1.2, n, 88); }
            else for (double t : { .5, 1.5, 2.5, 3.25 }) for (int n : notes) add (B + t, .3, n, t == .5 ? 104 : 92);   // off-beat stab
        }
    }
    else
    {
        const int top = 72 + pc > 79 ? 60 + pc : 72 + pc;
        for (int b = 0; b < bars; ++b)
        {
            const int d = chordRoot (b); const double B = b * 4;
            auto tone = [&] (int t) { int n = top - 12 + degreeSemis (d + t, D.minor); while (n < top - 7) n += 12; while (n > top + 7) n -= 12; return n; };
            if (D.arp || D.kind == kPluck)
            {
                const double st = D.arp ? kRateBeats[D.rate] : .5; int i = 0; const int pat[8] = { 0, 2, 4, 7, 4, 2, 0, 4 };
                for (double t = 0; t < 4; t += st, ++i) add (B + t, st * (D.kind == kPluck ? .8 : .6), tone (pat[i % 8]) + (pat[i % 8] == 7 ? 0 : 0), i % 4 == 0 ? 108 : 90);
            }
            else if (D.kind == kBell) { const int pat[4] = { 4, 2, 0, 2 }; for (int q = 0; q < 4; ++q) add (B + q, 1.4, tone (pat[q]) + 12 * (q == 0), q == 0 ? 104 : 90); }
            else
            {   // lead: soru (1-2) cevap (3-4)
                const bool answer = b % 2 == 1;
                add (B, 1.5, tone (answer ? 2 : 4), 108); add (B + 1.5, .5, tone (3), 92); add (B + 2, 1, tone (answer ? 0 : 2), 100);
                if (! answer) { add (B + 3, .5, tone (1), 90); add (B + 3.5, .5, tone (2), 96); }
            }
        }
    }
    return out;
}

double phraseSeconds (const Design& D) { return juce::jlimit (1, 8, D.bars) * 4 * 60.0 / juce::jlimit (50.0, 220.0, D.bpm); }

// ---------------------------------------------------------------- çevrimdışı işleme
std::unique_ptr<juce::AudioBuffer<float>> render (const Design& D, const var& program, const hg::Data& data, double sr)
{
    if (sr <= 0) sr = 48000;
    auto E = std::make_unique<hg::Engine>(); const int block = 64;
    E->prepare (sr, 512); E->loudnessOn = false; E->bpm = (float) D.bpm; E->pumpAmt = 0; E->master = 1;
    auto* so = new juce::DynamicObject(); so->setProperty ("id", "prompt"); so->setProperty ("xs", program);
    so->setProperty ("o", juce::Array<var> { juce::Array<var> { "s", 0, 0, 1 } }); so->setProperty ("vol", .2);
    const int track = D.part == 2 ? 3 : D.part, slotI = D.part;
    E->setPatch (slotI, hg::Patch::fromVar (var (so), data, sr), false);
    const auto notes = phrase (D); const double spb = 60.0 / D.bpm, total = phraseSeconds (D);
    const double tail = D.kind == kPad || D.kind == kBell ? 2.2 : 1.2;
    const int N = (int) ((total + tail) * sr);
    auto out = std::make_unique<juce::AudioBuffer<float>> (2, N); out->clear();
    juce::AudioBuffer<float> tmp (2, block); size_t next = 0;
    E->render (tmp, 0, block);   // yama ilk blokta devreye girer: 0. vuruştaki notalar kaybolmasın
    std::vector<size_t> order (notes.size()); for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort (order.begin(), order.end(), [&] (size_t a, size_t b) { return notes[a].beat < notes[b].beat; });
    for (int at = 0; at < N; at += block)
    {
        const int n = juce::jmin (block, N - at); const double tNow = at / sr;
        while (next < order.size() && notes[order[next]].beat * spb <= tNow + 1e-9)
        { const auto& x = notes[order[next++]]; E->noteOn (track, x.note, x.vel, x.len * spb, 0); }
        tmp.clear(); E->render (tmp, 0, n);
        for (int c = 0; c < 2; ++c) out->copyFrom (c, at, tmp, c, 0, n);
    }
    E->collectGarbage();
    // tepe -1 dBFS; kısa sessiz kuyruk kırpılır
    const float pk = out->getMagnitude (0, N);
    if (pk > 1e-5f) out->applyGain (.891f / pk);
    int end = N; while (end > (int) (total * sr) && out->getMagnitude (end - juce::jmin (end, 256), juce::jmin (end, 256)) < .0005f) end -= 256;
    if (end < N) out->setSize (2, juce::jmax (1, end), true);
    const int fade = juce::jmin (out->getNumSamples(), (int) (sr * .02)); out->applyGainRamp (out->getNumSamples() - fade, fade, 1, 0);
    return out;
}

bool writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
{
    f.deleteFile(); auto os = f.createOutputStream(); if (os == nullptr) return false;
    std::unique_ptr<juce::AudioFormatWriter> w { juce::WavAudioFormat().createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0) };
    if (w == nullptr) return false;
    os.release(); const bool ok = w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); w.reset();
    return ok;
}
}
