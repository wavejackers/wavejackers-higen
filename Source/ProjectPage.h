// Proje sekmesi: bir stil şablonundan tek tıkla başla ya da kendi projeni kaydet/aç.
// Şablon = stil + stile uygun ton, makam, tempo, enerji ve yeni bir fikir (ses ve MIDI dahil).
// Projelerim = mevcut sanatçı presetleriyle aynı dosya biçimi (.hgpreset): tüm durum saklanır.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Ui.h"
#include "CharacterStage.h"

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

private:
    struct Model;   // iki liste modeli (şablonlar, projelerim)
    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false; int shownRev = -1, pendingDelete = -1;
    juce::Label help, status, nameLabel;
    juce::TextEditor nameEd;
    juce::TextButton startBtn, saveBtn, openBtn, deleteBtn;
    juce::ListBox tmplList, mineList;
    hgui::DragTile fullTile, full0Tile, loopTile;   // DAW'a sürüklenen MIDI: tam parça (çok izli), tam parça (Format 0), tüm izler (döngü)
    std::unique_ptr<Model> tmplModel, mineModel;
    std::unique_ptr<CharacterStage> stage;   // isteğe bağlı karakter katmanı (bilge + iki melek)

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void startTemplate (int style);
    void save();
    void openRow (int row);
    void say (const juce::String& s);
    void updateButtons();
    void paintSkeleton (juce::Graphics& g, juce::Rectangle<int> area, int style) const;
    juce::Rectangle<int> mir (juce::Rectangle<int> r) const { return rtl ? r.withX (kW - r.getRight()) : r; }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectPage)
};
