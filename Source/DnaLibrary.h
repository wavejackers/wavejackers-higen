// Sanatçı ve Track DNA kütüphanesi: kullanıcının kendi DNA profillerini (.hgdna) klasörde tutar.
// Telifli kayıt paketlenmez; "sanatçı" yalnızca kullanıcının girdiği üst veridir. Mesaj iş parçacığında kullanılır.
#pragma once
#include "RefAnalysis.h"

namespace hg
{
struct DnaEntry { juce::File file; juce::String name, artist, role, source; bool favorite = false; juce::int64 lastUsed = 0; juce::StringArray tags; };   // tags: dnaTags() anahtarları

class DnaLibrary
{
public:
    explicit DnaLibrary (const juce::File& folder);
    static juce::File defaultFolder();
    void refresh();                                                    // klasörü yeniden tarar (en çok 500 dosya, her biri ≤ 1 MB)
    const std::vector<DnaEntry>& entries() const noexcept { return all; }
    enum Filter { All, Favorites, Recent };
    // ad/sanatçı/rol/kaynak/etiket içinde büyük-küçük harf duyarsız arama; tagName verilirse etiketlerin çevrilmiş adları da aranır
    std::vector<DnaEntry> query (const juce::String& text, Filter f, const std::function<juce::String (const juce::String&)>& tagName = {}) const;
    bool save (const TrackDna& d, const juce::String& artist, const juce::String& name, juce::File* written = nullptr);
    bool rename (const DnaEntry& e, const juce::String& newName);
    bool remove (const DnaEntry& e);
    bool importFile (const juce::File& src, juce::File* written = nullptr);     // geçerli DNA değilse reddedilir
    bool exportTo (const DnaEntry& e, const juce::File& dest) const;
    bool load (const DnaEntry& e, TrackDna& out);                               // yükler ve "son kullanılan"a ekler
    void setFavorite (const DnaEntry& e, bool on);
    juce::File folder() const { return dir; }

private:
    juce::File dir; std::vector<DnaEntry> all; std::vector<juce::String> favs; std::vector<std::pair<juce::String, juce::int64>> recents;
    void loadIndex(); void saveIndex() const;
    static juce::String legal (const juce::String& s);
    juce::File uniqueFile (const juce::String& base) const;
};
}
