#include "FlpWriter.h"
#include "SampleLib.h"
#include <map>

namespace hg
{
namespace
{
// ---- olay kodlama: 0-63 bayt, 64-127 kelime, 128-191 dword, 192+ değişken uzunluk
struct Out
{
    std::vector<uint8_t> b;
    void ev (int id, juce::uint32 v)
    {
        b.push_back ((uint8_t) id);
        if (id < 64) b.push_back ((uint8_t) v);
        else if (id < 128) { b.push_back ((uint8_t) v); b.push_back ((uint8_t) (v >> 8)); }
        else for (int i = 0; i < 4; ++i) b.push_back ((uint8_t) (v >> (8 * i)));
    }
    void data (int id, const void* p, size_t n)
    {
        b.push_back ((uint8_t) id);
        size_t len = n;
        do { uint8_t c = (uint8_t) (len & 0x7f); len >>= 7; if (len) c |= 0x80; b.push_back (c); } while (len);
        auto* q = static_cast<const uint8_t*> (p); b.insert (b.end(), q, q + n);
    }
    void data (int id, const std::vector<uint8_t>& v) { data (id, v.data(), v.size()); }
    void text (int id, const juce::String& s)   // UTF-16LE, sonda sıfır
    {
        std::vector<uint8_t> v;
        for (auto c : s) { const auto u = (juce::uint32) c; if (u < 0x10000) { v.push_back ((uint8_t) u); v.push_back ((uint8_t) (u >> 8)); }
                           else { const auto x = u - 0x10000, hi = 0xD800 + (x >> 10), lo = 0xDC00 + (x & 0x3ff); v.push_back ((uint8_t) hi); v.push_back ((uint8_t) (hi >> 8)); v.push_back ((uint8_t) lo); v.push_back ((uint8_t) (lo >> 8)); } }
        v.push_back (0); v.push_back (0);
        data (id, v);
    }
    void ascii (int id, const juce::String& s) { std::vector<uint8_t> v; for (auto c : s) v.push_back ((uint8_t) (c < 128 ? c : '?')); v.push_back (0); data (id, v); }
};
std::vector<uint8_t> hex (const char* h)
{
    std::vector<uint8_t> v;
    for (size_t i = 0; h[i] && h[i + 1]; i += 2) v.push_back ((uint8_t) juce::String (juce::CharPointer_ASCII (h + i), 2).getHexValue32());
    return v;
}
void put32 (std::vector<uint8_t>& v, juce::uint32 x) { for (int i = 0; i < 4; ++i) v.push_back ((uint8_t) (x >> (8 * i))); }
void put16 (std::vector<uint8_t>& v, juce::uint32 x) { v.push_back ((uint8_t) x); v.push_back ((uint8_t) (x >> 8)); }

// FL Studio 20.9 Sampler kanalının varsayılan olayları (şablon projeden). 0 = yer tutucu: ad, renk, mikser kanalı, sample yolu.
struct Tmpl { int id; juce::uint32 v; const char* h; };
const Tmpl kSampler[] = {
    { 21, 4, nullptr }, { 201, 0, "0000" },
    { 212, 0, "0000000000000000ffffffff000000005000000003000000000000000000000000000000f7010000cc0000000000000000000000" },
    { 203, 0, nullptr }, { 155, 75, nullptr }, { 128, 0, nullptr }, { 0, 1, nullptr },
    { 209, 0, "0000000000190000000000000400000090000000" },
    { 138, 8388736, nullptr }, { 139, 65536, nullptr }, { 89, 0, nullptr }, { 97, 128, nullptr }, { 69, 128, nullptr }, { 86, 256, nullptr },
    { 71, 1024, nullptr }, { 83, 0, nullptr }, { 74, 0, nullptr }, { 75, 340, nullptr }, { 76, 0, nullptr }, { 85, 2048, nullptr },
    { 131, 8388608, nullptr }, { 70, 0, nullptr }, { 22, 0, nullptr },
    { 219, 0, "00190000e21d000000000000000100000000000000000000" },
    { 229, 0, "0000000000320000000000000000000000000000" },
    { 221, 0, "00000000f401000000" },
    { 215, 0, "8c0000000000000001000000ffffffff3c0000000000803f0000803f0000803f0000803f0000803f000000000300000000000000000400003000000000000100a7050000000000000001000000000000000000010000000000000000010000000000000000000000000000000000000002000000feffffffffffffff000000000000000000000000000000000000f03f0000000000000000ffffffff01010000000000000000e03f" },
    { 132, 0, nullptr }, { 144, 0, nullptr }, { 145, 0, nullptr }, { 32, 0, nullptr },
    { 228, 0, "64000000000000000000000000000000" }, { 228, 0, "3c000000000000000000000000000000" },
    { 218, 0, "000000000000000064000000204e0000204e00003075000032000000204e00000000000064000000204e000000000000b680000000000000000000000000000000000000" },
    { 218, 0, "040000000000000064000000204e0000204e00003075000032000000204e00000000000064000000204e000000000000b68000000000000000000000000000009bffffff" },
    { 218, 0, "000000000000000064000000204e0000204e00003075000032000000204e00000000000064000000204e000000000000b680000000000000000000000000000000000000" },
    { 218, 0, "000000000000000064000000204e0000204e00003075000032000000204e00000000000064000000204e000000000000b680000000000000000000000000000000000000" },
    { 218, 0, "000000000000000064000000204e0000204e00003075000032000000204e00000000000064000000204e000000000000b680000000000000000000000000000000000000" },
    { 143, 2, nullptr }, { 20, 0, nullptr }, { 196, 0, nullptr } };
// Playlist izinin varsayılan verisi (66 bayt; ilk 4 bayt iz numarası, sonraki 4 bayt renk)
const char* const kTrackData = "910100004851560000000000010000803fffffffff0000000000000000000500000000000000010000000000000000000000000000ffffffffffffffff0100000000";
constexpr int kInserts = 127;
}

juce::MemoryBlock writeFlp (const FlpProject& P)
{
    Out o;
    // ---- proje başlığı
    o.ascii (199, "20.9.2.2963"); o.ev (159, 2963); o.ev (28, 1); o.ev (37, 1); o.text (200, {});
    o.ev (156, (juce::uint32) juce::roundToInt (P.bpm * 1000.0));
    o.ev (67, 1); o.ev (9, 1); o.ev (11, 0); o.ev (80, 0); o.ev (17, 4); o.ev (18, 4); o.ev (35, 1); o.ev (23, 0); o.ev (30, 1); o.ev (10, 1);
    o.text (194, P.title); o.text (206, P.genre); o.text (207, "Wavejackers HitGen"); o.text (202, {}); o.text (195, P.comment);
    {   // oluşturma zamanı (1899-12-30'dan beri gün) + çalışma süresi
        const double days = (double) juce::Time::getCurrentTime().toMilliseconds() / 86400000.0 + 25569.0, work = 0;
        std::vector<uint8_t> t (16); std::memcpy (t.data(), &days, 8); std::memcpy (t.data() + 8, &work, 8);
        o.data (237, t);
    }
    o.text (231, "Audio"); o.text (231, "Automation"); o.text (231, "Unsorted"); o.ev (146, 0xffffffffu);
    // ---- pattern'ler: notalar, ad, renk, uzunluk
    for (size_t i = 0; i < P.patterns.size(); ++i)
    {
        const auto& pt = P.patterns[i];
        o.ev (65, (juce::uint32) (i + 1));
        if (! pt.notes.empty())
        {
            std::vector<uint8_t> v; v.reserve (pt.notes.size() * 24);
            for (auto& n : pt.notes)
            {
                put32 (v, (juce::uint32) juce::jmax (0, n.pos)); put16 (v, 0x4000); put16 (v, (juce::uint32) pt.channel);
                put32 (v, (juce::uint32) juce::jmax (1, n.len)); put16 (v, (juce::uint32) juce::jlimit (0, 131, n.key)); put16 (v, 0);
                v.push_back (120); v.push_back (0); v.push_back (64); v.push_back (0); v.push_back (64);
                v.push_back ((uint8_t) juce::jlimit (1, 128, n.vel)); v.push_back (128); v.push_back (128);
            }
            o.data (224, v);
        }
        o.ev (65, (juce::uint32) (i + 1)); o.text (193, pt.name); o.ev (150, pt.colour);
        o.ev (157, 0xffffffffu); o.ev (158, 0xffffffffu); o.ev (164, (juce::uint32) juce::jmax (0, pt.lengthTicks));
    }
    // ---- kanallar (hepsi Sampler)
    for (size_t c = 0; c < P.channels.size(); ++c)
    {
        const auto& ch = P.channels[c];
        o.ev (64, (juce::uint32) c);
        for (auto& t : kSampler)
        {
            if (t.id == 203) o.text (203, ch.name);
            else if (t.id == 128) o.ev (128, ch.colour);
            else if (t.id == 22) o.ev (22, (juce::uint32) (ch.insert >= 0 ? ch.insert : 0));
            else if (t.id == 196) { if (ch.samplePath.isNotEmpty()) o.text (196, ch.samplePath); }
            else if (t.h != nullptr) o.data (t.id, hex (t.h));
            else o.ev (t.id, t.v);
        }
    }
    // ---- aranjman: playlist, işaretçiler, izler
    o.ev (99, 0); o.text (241, "Arrangement"); o.ev (36, 0);
    {
        auto items = P.items;
        std::stable_sort (items.begin(), items.end(), [] (const FlpItem& a, const FlpItem& b) { return a.pos < b.pos || (a.pos == b.pos && a.track < b.track); });
        std::vector<uint8_t> v; v.reserve (items.size() * 32);
        for (auto& it : items)
        {
            put32 (v, (juce::uint32) juce::jmax (0, it.pos)); put16 (v, 20480);
            put16 (v, (juce::uint32) (it.pattern ? 20480 + it.index : it.index));
            put32 (v, (juce::uint32) juce::jmax (1, it.len)); put16 (v, (juce::uint32) (499 - juce::jlimit (0, 499, it.track))); put16 (v, 0);
            v.push_back (120); v.push_back (0); v.push_back (64); v.push_back (0); v.push_back (64); v.push_back (100); v.push_back (128); v.push_back (128);
            if (it.pattern) { put32 (v, 0xffffffffu); put32 (v, 0xffffffffu); }
            else { const float m1 = -1.0f; juce::uint32 u; std::memcpy (&u, &m1, 4); put32 (v, u); put32 (v, u); }
        }
        o.data (233, v);
    }
    for (auto& m : P.markers) { o.ev (148, (juce::uint32) juce::jmax (0, m.pos)); o.text (205, m.name); }
    {
        const auto base = hex (kTrackData);
        for (int t = 0; t < 500; ++t)
        {
            auto d = base;
            const auto iid = (juce::uint32) (t + 1);
            for (int i = 0; i < 4; ++i) d[(size_t) i] = (uint8_t) (iid >> (8 * i));
            if (t < (int) P.tracks.size()) for (int i = 0; i < 4; ++i) d[(size_t) (4 + i)] = (uint8_t) (P.tracks[(size_t) t].colour >> (8 * i));
            o.data (238, d);
            if (t < (int) P.tracks.size()) o.text (239, P.tracks[(size_t) t].name);
        }
    }
    o.ev (100, 0); o.ev (29, 1); o.ev (39, 1); o.ev (40, 0); o.ev (31, 0); o.ev (38, 1);
    // ---- mikser: boş eklentisiz kanallar; 1..125 ana çıkışa gider, kullanılanlar kanal adını taşır
    std::map<int, const FlpChannel*> named;
    for (auto& ch : P.channels) if (ch.insert > 0 && ch.insert < kInserts - 1) named[ch.insert] = &ch;
    for (int k = 0; k < kInserts; ++k)
    {
        const bool edge = k == 0 || k == kInserts - 1;
        // FL, bir kanalın rengini ve adını o kanalın bayraklarından (236) önce yazar
        if (auto it = named.find (k); it != named.end()) { o.ev (149, it->second->colour); o.text (204, it->second->name); }
        std::vector<uint8_t> fl (12, 0); fl[4] = edge ? 0x0c : 0x4c;
        o.data (236, fl);
        for (int s = 0; s < 10; ++s) o.ev (98, (juce::uint32) s);
        std::vector<uint8_t> route (127, 0); if (! edge) route[0] = 1;
        o.data (235, route);
        o.ev (165, 3); o.ev (166, 1); o.ev (154, 0xffffffffu); o.ev (147, k == 0 ? 0u : 0xffffffffu);
    }
    // ---- dosya: FLhd + FLdt
    juce::MemoryOutputStream f;
    f.write ("FLhd", 4); f.writeInt (6); f.writeShort (0); f.writeShort ((short) P.channels.size()); f.writeShort ((short) P.ppq);
    f.write ("FLdt", 4); f.writeInt ((int) o.b.size()); f.write (o.b.data(), o.b.size());
    return f.getMemoryBlock();
}

bool readFlpEvents (const juce::MemoryBlock& file, int& ppq, int& numChannels, std::vector<FlpEvent>& out)
{
    auto* d = static_cast<const uint8_t*> (file.getData()); const size_t n = file.getSize();
    if (n < 22 || std::memcmp (d, "FLhd", 4) != 0 || std::memcmp (d + 14, "FLdt", 4) != 0) return false;
    numChannels = d[10] | (d[11] << 8); ppq = d[12] | (d[13] << 8);
    const size_t size = (size_t) (d[18] | (d[19] << 8) | (d[20] << 16) | ((size_t) d[21] << 24));
    if (22 + size != n) return false;
    size_t p = 22;
    while (p < n)
    {
        FlpEvent e; e.id = d[p++];
        auto need = [&] (size_t k) { return p + k <= n; };
        if (e.id < 64) { if (! need (1)) return false; e.value = d[p]; p += 1; }
        else if (e.id < 128) { if (! need (2)) return false; e.value = (juce::uint32) (d[p] | (d[p + 1] << 8)); p += 2; }
        else if (e.id < 192) { if (! need (4)) return false; e.value = (juce::uint32) d[p] | ((juce::uint32) d[p + 1] << 8) | ((juce::uint32) d[p + 2] << 16) | ((juce::uint32) d[p + 3] << 24); p += 4; }
        else
        {
            size_t len = 0; int sh = 0;
            while (true) { if (! need (1) || sh > 28) return false; const uint8_t c = d[p++]; len |= (size_t) (c & 0x7f) << sh; sh += 7; if (! (c & 0x80)) break; }
            if (! need (len)) return false;
            e.data.append (d + p, len); p += len;
        }
        out.push_back (std::move (e));
    }
    return true;
}

// ---------------------------------------------------------------- iskeletten FL projesi
namespace
{
juce::uint32 rgb (int r, int g, int b) { return (juce::uint32) ((b << 16) | (g << 8) | r); }   // FL: 0x00BBGGRR
struct Lane { juce::String name; juce::uint32 colour; int track = -1, gm = -1; };
}

FlpProject buildFlp (const Skeleton& sk, const FlpSources& src, const juce::String& title)
{
    FlpProject P; P.ppq = 96; P.bpm = sk.song.P.bpm > 0 ? sk.song.P.bpm : 120; P.title = title;
    P.genre = sk.profile == "organic" ? "Organic House" : sk.profile == "techno" ? "Melodic Techno" : "EDM";
    P.comment = "Wavejackers HitGen project skeleton";
    const int step = P.ppq / 4, bar = P.ppq * 4;
    // davul şeritleri: şarkıda notası olan her GM vuruşu ayrı kanal ve iz
    const struct { int gm; const char* name; juce::uint32 col; } drumDefs[] = {
        { 36, "Kick", rgb (226, 87, 76) }, { 39, "Clap", rgb (240, 160, 70) }, { 38, "Snare", rgb (236, 190, 80) }, { 42, "Hat", rgb (227, 187, 98) },
        { 46, "Open Hat", rgb (210, 200, 120) }, { 37, "Rim", rgb (180, 160, 110) }, { 64, "Perc", rgb (170, 140, 90) } };
    std::vector<Lane> lanes; std::map<int, int> laneOfGm;
    for (auto& d : drumDefs)
    {
        bool has = false; for (auto& n : sk.song.tracks[kDrums]) if (n.n == d.gm) { has = true; break; }
        if (! has) continue;
        laneOfGm[d.gm] = (int) lanes.size();
        lanes.push_back ({ d.name, d.col, -1, d.gm });
    }
    const struct { int t; const char* name; juce::uint32 col; } partDefs[] = { { kBass, "Bass", rgb (90, 150, 230) }, { kChords, "Chords", rgb (150, 110, 220) }, { kMelody, "Melody", rgb (90, 200, 170) } };
    std::map<int, int> laneOfPart;
    for (auto& d : partDefs)
        if (! sk.song.tracks[(size_t) d.t].empty()) { laneOfPart[d.t] = (int) lanes.size(); lanes.push_back ({ d.name, d.col, -1, -1 }); }
    // kanallar + izler (şerit sırası)
    for (size_t i = 0; i < lanes.size(); ++i)
    {
        auto& L = lanes[i]; L.track = (int) i;
        FlpChannel ch; ch.name = L.name; ch.colour = L.colour; ch.insert = (int) i + 1;
        if (L.gm >= 0) { auto it = src.drums.find (L.gm); if (it != src.drums.end()) ch.samplePath = it->second; }
        else for (auto& [t, li] : laneOfPart) if (li == (int) i) ch.samplePath = src.parts[(size_t) t];
        P.channels.push_back (ch);
        P.tracks.push_back ({ L.name, L.colour });
    }
    // pattern'ler: şerit x bölüm
    for (auto& s : sk.secs)
    {
        const double a = s.bar * 16.0, b = (s.bar + s.bars) * 16.0;
        auto addPattern = [&] (int laneIdx, const std::vector<NoteEv>& src_, int gm, bool melodic)
        {
            FlpPattern pt; pt.channel = laneIdx; pt.colour = lanes[(size_t) laneIdx].colour;
            pt.name = lanes[(size_t) laneIdx].name + " - " + s.name; pt.lengthTicks = s.bars * bar;
            for (auto& n : src_)
            {
                if (n.s < a || n.s >= b || (gm >= 0 && n.n != gm)) continue;
                FlpNote fn; fn.pos = juce::roundToInt ((n.s - a) * step); fn.len = juce::jmax (melodic ? 6 : step, juce::roundToInt (n.l * step));
                fn.key = melodic ? n.n : 60; fn.vel = juce::jlimit (1, 128, juce::roundToInt (n.v * 128.0 / 127.0));
                pt.notes.push_back (fn);
            }
            if (pt.notes.empty()) return;
            P.patterns.push_back (std::move (pt));
            P.items.push_back ({ true, (int) P.patterns.size(), s.bar * bar, s.bars * bar, lanes[(size_t) laneIdx].track });
        };
        for (auto& [gm, li] : laneOfGm) addPattern (li, sk.song.tracks[kDrums], gm, false);
        for (auto& [t, li] : laneOfPart) addPattern (li, sk.song.tracks[(size_t) t], -1, true);
    }
    // FX ses klipleri: ipucu türü başına bir iz, dosya başına bir kanal
    std::map<juce::String, int> fxTrack, fxChannel;
    const int total = sk.song.bars * bar;
    for (auto& f : src.fx)
    {
        if (f.path.isEmpty() || f.seconds <= 0) continue;
        if (! fxTrack.count (f.cue.type))
        {
            fxTrack[f.cue.type] = (int) P.tracks.size();
            P.tracks.push_back ({ "FX " + f.cue.type, rgb (120, 130, 150) });
        }
        if (! fxChannel.count (f.path))
        {
            fxChannel[f.path] = (int) P.channels.size();
            FlpChannel ch; ch.name = juce::File (f.path).getFileNameWithoutExtension(); ch.samplePath = f.path; ch.colour = rgb (120, 130, 150);
            ch.insert = juce::jmin (125, (int) P.channels.size() + 1);   // her ses kendi mikser kanalında
            P.channels.push_back (ch);
        }
        const int clip = juce::jmax (1, juce::roundToInt (f.seconds * P.bpm / 60.0 * P.ppq));
        const int c0 = f.cue.bar * bar, c1 = (f.cue.bar + f.cue.len) * bar;
        FlpItem it; it.pattern = false; it.index = fxChannel[f.path]; it.track = fxTrack[f.cue.type];
        if (f.endAligned) { it.pos = juce::jmax (0, juce::jmax (c0 - 8 * bar, c1 - clip)); it.len = juce::jmin (clip, c1 - it.pos); }
        else { it.pos = c0; it.len = juce::jmin (clip, juce::jmax (c1 - c0, 4 * bar) + 4 * bar); }
        it.len = juce::jmin (it.len, total - it.pos);
        if (it.len > 0) P.items.push_back (it);
    }
    for (auto& s : sk.secs) P.markers.push_back ({ s.bar * bar, s.name });
    // FL 21 playlist öğeleri 60 bayt, FL 20.9 öğeleri 32 bayt: toplam bayt 60'a bölünürse bazı okuyucular boyu yanlış tahmin eder.
    // Belirsizlik kalmasın: gerekirse boş, 1 ölçülük bir bilgi klibi eklenir (ses üretmez).
    if (P.items.size() % 15 == 0)
    {
        FlpPattern info; info.name = "Wavejackers HitGen"; info.colour = rgb (227, 187, 98); info.lengthTicks = bar; info.channel = 0;
        P.patterns.push_back (info);
        P.tracks.push_back ({ "HitGen", rgb (227, 187, 98) });
        P.items.push_back ({ true, (int) P.patterns.size(), 0, bar, (int) P.tracks.size() - 1 });
    }
    return P;
}
}
