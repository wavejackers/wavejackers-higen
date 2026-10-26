#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Ui.h"

class HitGenEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit HitGenEditor (HitGenProcessor&);
    ~HitGenEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    static constexpr int kW = 1180, kH = 760;

    // testler için
    juce::Component& contentComponent() { return content; }
    void setLanguage (const juce::String& l);

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Content : public juce::Component { HitGenEditor* owner = nullptr; void paint (juce::Graphics&) override; };
    struct StyleModel : public juce::ListBoxModel
    {
        HitGenEditor& ed; explicit StyleModel (HitGenEditor& e) : ed (e) {}
        int getNumRows() override { return ed.filtered.size(); }
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        void selectedRowsChanged (int row) override;
        juce::String getNameForRow (int row) override;
    };

    HitGenProcessor& proc;
    const hg::Data& data;
    hgui::LookAndFeel lnf;
    Content content;
    juce::TooltipWindow tooltips { this, 600 };

    juce::Label tagline, status, disclaimer, varLabel, soundLabel, hostLabel;
    juce::TextButton playBtn, newBtn;
    juce::Slider seed; juce::Label seedLabel;
    juce::ComboBox langBox, qualityBox, sizeBox, keyBox, scaleBox, barsBox, keysToBox;
    juce::Label langLabel, qualityLabel, sizeLabel, keyLabel, scaleLabel, barsLabel, keysToLabel;
    juce::ToggleButton contrastBtn, seventhsBtn, patternBtn, midiOutBtn;
    std::array<juce::TextButton, 6> groupBtns;
    juce::ListBox styleList; StyleModel styleModel { *this }; juce::Array<int> filtered; int group = 0;
    std::array<juce::TextButton, 4> varBtns;
    std::array<hgui::Knob, 6> genKnobs;
    hgui::ChordStrip chords; hgui::PianoRoll roll;
    std::array<juce::TextButton, 4> muteBtns; std::array<juce::ComboBox, 4> soundBoxes; std::array<hgui::DragTile, 4> tiles; std::array<juce::Label, 4> trackLabels;
    std::array<juce::StringArray, 4> soundIds;
    std::array<juce::TextButton, 3> partBtns; int part = 0;
    std::array<hgui::Knob, 8> partKnobs;
    hgui::DragTile loopTile, fullTile;

    std::vector<std::unique_ptr<SA>> sa; std::vector<std::unique_ptr<CA>> ca; std::vector<std::unique_ptr<BA>> ba;
    std::vector<std::unique_ptr<SA>> partSa; std::vector<std::unique_ptr<BA>> muteBa;

    juce::String lang; bool rtl = false; int shownStyle = -1, shownVar = -1;
    std::array<juce::String, 4> shownSounds;
    std::vector<std::pair<juce::Component*, juce::Rectangle<int>>> layoutList;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void timerCallback() override;
    void relabel();
    void refreshResult();
    void refreshStyleList();
    void fillSoundBoxes();
    void bindPart (int p);
    void applyPalette();
    void setParam (const char* id, float plain);
    void place (juce::Component& c, juce::Rectangle<int> r) { layoutList.push_back ({ &c, r }); }
    void layoutContent();
    juce::Colour styleAccent() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitGenEditor)
};
