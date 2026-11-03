#include "DnaLibrary.h"
#include "Assist.h"
#include <algorithm>

namespace hg
{
using namespace juce;

DnaLibrary::DnaLibrary (const File& folder) : dir (folder) { dir.createDirectory(); loadIndex(); refresh(); }

File DnaLibrary::defaultFolder() { return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("DNA"); }

String DnaLibrary::legal (const String& s) { auto t = File::createLegalFileName (s.trim().substring (0, 60)).trim(); return t.isEmpty() ? String ("profile") : t; }

File DnaLibrary::uniqueFile (const String& base) const
{
    auto f = dir.getChildFile (base + ".hgdna");
    for (int i = 2; f.exists() && i < 1000; ++i) f = dir.getChildFile (base + " (" + String (i) + ").hgdna");
    return f;
}

void DnaLibrary::loadIndex()
{
    favs.clear(); recents.clear();
    const auto v = JSON::parse (dir.getChildFile ("library.json").loadFileAsString());
    if (auto* fa = v.getProperty ("fav", {}).getArray()) for (auto& x : *fa) if (favs.size() < 2000) favs.push_back (x.toString());
    if (auto* ra = v.getProperty ("recent", {}).getArray()) for (auto& x : *ra) if (recents.size() < 50) recents.push_back ({ x.getProperty ("f", {}).toString(), (int64) x.getProperty ("t", 0) });
}

void DnaLibrary::saveIndex() const
{
    Array<var> fa, ra; for (auto& f : favs) fa.add (f);
    for (auto& r : recents) { auto* o = new DynamicObject(); o->setProperty ("f", r.first); o->setProperty ("t", r.second); ra.add (var (o)); }
    auto* o = new DynamicObject(); o->setProperty ("fav", fa); o->setProperty ("recent", ra);
    dir.getChildFile ("library.json").replaceWithText (JSON::toString (var (o)));
}

void DnaLibrary::refresh()
{
    all.clear();
    for (auto& f : dir.findChildFiles (File::findFiles, false, "*.hgdna"))
    {
        if (all.size() >= 500) break;
        if (f.getSize() > 1000000) continue;
        TrackDna d; if (! TrackDna::fromVar (JSON::parse (f.loadFileAsString()), d)) continue;
        DnaEntry e; e.file = f; e.name = d.name.isNotEmpty() ? d.name : f.getFileNameWithoutExtension(); e.artist = d.artist; e.role = d.role; e.source = d.sourceFile; e.tags = dnaTags (d);
        e.favorite = std::find (favs.begin(), favs.end(), f.getFileName()) != favs.end();
        for (auto& r : recents) if (r.first == f.getFileName()) e.lastUsed = r.second;
        all.push_back (e);
    }
    std::sort (all.begin(), all.end(), [] (const DnaEntry& a, const DnaEntry& b) { const int c = a.artist.compareIgnoreCase (b.artist); return c != 0 ? c < 0 : a.name.compareIgnoreCase (b.name) < 0; });
}

std::vector<DnaEntry> DnaLibrary::query (const String& text, Filter f, const std::function<String (const String&)>& tagName) const
{
    std::vector<DnaEntry> out; const auto q = text.trim();
    for (auto& e : all)
    {
        if (f == Favorites && ! e.favorite) continue;
        if (f == Recent && e.lastUsed == 0) continue;
        auto tagHit = [&] { for (auto& t : e.tags) if (t.containsIgnoreCase (q) || (tagName && tagName (t).containsIgnoreCase (q))) return true; return false; };
        if (q.isNotEmpty() && ! (e.name.containsIgnoreCase (q) || e.artist.containsIgnoreCase (q) || e.role.containsIgnoreCase (q) || e.source.containsIgnoreCase (q) || tagHit())) continue;
        out.push_back (e);
    }
    if (f == Recent) std::sort (out.begin(), out.end(), [] (const DnaEntry& a, const DnaEntry& b) { return a.lastUsed > b.lastUsed; });
    return out;
}

bool DnaLibrary::save (const TrackDna& d0, const String& artist, const String& name, File* written)
{
    if (name.trim().isEmpty()) return false;
    TrackDna d = d0; d.name = name.trim().substring (0, 80); d.artist = artist.trim().substring (0, 80);
    const auto f = uniqueFile (legal ((d.artist.isNotEmpty() ? d.artist + " - " : String()) + d.name));
    if (! f.replaceWithText (JSON::toString (d.toVar()))) return false;
    if (written) *written = f; refresh(); return true;
}

bool DnaLibrary::rename (const DnaEntry& e, const String& newName)
{
    if (newName.trim().isEmpty() || ! e.file.existsAsFile() || e.file.getParentDirectory() != dir) return false;
    TrackDna d; if (! TrackDna::fromVar (JSON::parse (e.file.loadFileAsString()), d)) return false;
    d.name = newName.trim().substring (0, 80);
    const auto nf = uniqueFile (legal ((d.artist.isNotEmpty() ? d.artist + " - " : String()) + d.name));
    if (! nf.replaceWithText (JSON::toString (d.toVar()))) return false;
    const bool wasFav = e.favorite; const auto oldName = e.file.getFileName(); e.file.deleteFile();
    favs.erase (std::remove (favs.begin(), favs.end(), oldName), favs.end()); recents.erase (std::remove_if (recents.begin(), recents.end(), [&] (auto& r) { return r.first == oldName; }), recents.end());
    if (wasFav) favs.push_back (nf.getFileName());
    saveIndex(); refresh(); return true;
}

bool DnaLibrary::remove (const DnaEntry& e)
{
    if (e.file.getParentDirectory() != dir) return false;                       // kütüphane dışındaki dosyalara asla dokunma
    const auto n = e.file.getFileName(); if (! e.file.deleteFile()) return false;
    favs.erase (std::remove (favs.begin(), favs.end(), n), favs.end()); recents.erase (std::remove_if (recents.begin(), recents.end(), [&] (auto& r) { return r.first == n; }), recents.end());
    saveIndex(); refresh(); return true;
}

bool DnaLibrary::importFile (const File& src, File* written)
{
    if (! src.existsAsFile() || src.getSize() > 1000000) return false;
    TrackDna d; if (! TrackDna::fromVar (JSON::parse (src.loadFileAsString()), d)) return false;
    const auto f = uniqueFile (legal (src.getFileNameWithoutExtension()));
    if (! src.copyFileTo (f)) return false;
    if (written) *written = f; refresh(); return true;
}

bool DnaLibrary::exportTo (const DnaEntry& e, const File& dest) const { return e.file.existsAsFile() && dest != File() && e.file.copyFileTo (dest.withFileExtension ("hgdna")); }

bool DnaLibrary::load (const DnaEntry& e, TrackDna& out)
{
    if (e.file.getSize() > 1000000 || ! TrackDna::fromVar (JSON::parse (e.file.loadFileAsString()), out)) return false;
    const auto n = e.file.getFileName(); recents.erase (std::remove_if (recents.begin(), recents.end(), [&] (auto& r) { return r.first == n; }), recents.end());
    recents.insert (recents.begin(), { n, Time::currentTimeMillis() }); if (recents.size() > 20) recents.resize (20);
    saveIndex(); refresh(); return true;
}

void DnaLibrary::setFavorite (const DnaEntry& e, bool on)
{
    const auto n = e.file.getFileName(); favs.erase (std::remove (favs.begin(), favs.end(), n), favs.end()); if (on) favs.push_back (n);
    saveIndex(); refresh();
}
}
