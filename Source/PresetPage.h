// Sanatçı presetleri sekmesi: kendi sanatçı adınla kaydet, yükle, sil, dışa/içe aktar
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Ui.h"

class PresetPage : public juce::Component, private juce::ListBoxModel
{
public:
    PresetPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~PresetPage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void tick();
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void()> onLoaded;   // editör arayüzü yenilesin
    static constexpr int kW = 1164, kH = 654;

private:
    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false; int shownRev = -1, pendingDelete = -1;
    juce::Label artistLabel, nameLabel, help, status, current;
    juce::TextEditor artistEd, nameEd;
    juce::TextButton saveBtn, loadBtn, deleteBtn, exportBtn, importBtn, folderBtn;
    juce::ListBox list;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    juce::String getNameForRow (int row) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { loadRow (row); }
    void returnKeyPressed (int row) override { loadRow (row); }
    void selectedRowsChanged (int) override { pendingDelete = -1; updateButtons(); }
    void loadRow (int row);
    void save();
    void say (const juce::String& s);
    void updateButtons();
    juce::Rectangle<int> mir (juce::Rectangle<int> r) const { return rtl ? r.withX (kW - r.getRight()) : r; }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPage)
};
