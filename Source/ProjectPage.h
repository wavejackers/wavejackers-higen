// Proje sekmesi: bir stil şablonundan tek tıkla başla, kendi projeni kaydet/aç ve "Proje üret" ile parça iskeleti kur.
// Şablon = stil + stile uygun ton, makam, tempo, enerji ve yeni bir fikir (ses ve MIDI dahil).
// Projelerim = mevcut sanatçı presetleriyle aynı dosya biçimi (.hgpreset): tüm durum saklanır.
// İskelet = düzenlenebilir bölüm sırası (Intro, Buildup, Drop, Breakdown...), hedef süre (ör. 6:00, 8:00),
// bölüm başına MIDI ve Black Coffee / FRERE projelerinden çıkarılan tekniklere göre sample önerileri.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Ui.h"
#include "ProducerPanels.h"

class ProjectPage : public juce::Component
{
public:
    ProjectPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~ProjectPage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void tick();
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void()> onLoaded;   // editör arayüzü yenilesin
    static constexpr int kW = 1164, kH = 654;

    // ---- iskelet (testler de kullanır)
    void generateProject();                       // "Proje üret"
    void setStructure (int idx);                  // 0..4
    void setDurationIndex (int idx);              // HitGenProcessor::kSkDurations
    void sectionCommand (int section, int cmd, int arg = 0);   // aşağıdaki komutlar
    enum SecCmd { cmdType = 1, cmdLonger, cmdShorter, cmdLeft, cmdRight, cmdDup, cmdInsert, cmdDelete };
    void exportProject (const juce::File& dir = {});   // boş: Belgeler/Wavejackers HitGen/Projects/<ad>
    bool revealAfterExport = true;
    juce::String planText() const;
    const hg::Skeleton& shownSkeleton() const { return sk; }
    int numListRows() const { return (int) rows.size(); }
    juce::String statusText() const { return status.getText(); }
    juce::Component* skeletonView() const;
    std::unique_ptr<juce::Component> makePanel (bool autoProducer);   // testler: paneli kutusuz oluştur
    void openPanel (bool autoProducer);

private:
    struct Model;      // iki liste modeli (şablonlar, projelerim)
    struct CueModel;   // iskelet bölümleri ve ipuçları
    struct SkView;     // bölüm şeridi
    struct Row { bool section = false; int sec = 0; hg::SkCue cue; juce::File file; bool hasAudio = false; };
    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false; int shownRev = -1, pendingDelete = -1;
    juce::Label help, status, skHint, libInfo, exportInfo;
    juce::TextEditor nameEd;
    juce::TextButton startBtn, saveBtn, openBtn, deleteBtn, genBtn, addSecBtn, libBtn, drumsBtn, exportBtn, autoBtn, coBtn, openFlBtn;
    juce::ComboBox structBox, lenBox;
    juce::ListBox tmplList, mineList, cueList;
    hgui::DragTile fullTile, full0Tile, loopTile;   // DAW'a sürüklenen MIDI: tam parça (çok izli), tam parça (Format 0), tüm izler (döngü)
    std::unique_ptr<Model> tmplModel, mineModel;
    std::unique_ptr<CueModel> cueModel;
    std::unique_ptr<SkView> skView;
    std::unique_ptr<juce::FileChooser> chooser;
    hg::Skeleton sk; std::vector<Row> rows;
    const void* shownRes = nullptr; int shownBpm = -1, shownLib = -1, selSec = -1;
    juce::File lastExport, lastFlp;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void startTemplate (int style);
    void save();
    void openRow (int row);
    void say (const juce::String& s);
    void updateButtons();
    void refreshSkeleton();
    void syncCombos();
    void showSectionMenu (int section);
    void showLibraryMenu();
    juce::String timeOf (int bar) const;
    juce::String kindName (const juce::String& kind) const { return T ("sk_" + kind); }
    juce::File sectionMidiFile (int section) const;
    juce::Rectangle<int> mir (juce::Rectangle<int> r) const { return rtl ? r.withX (kW - r.getRight()) : r; }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectPage)
};
