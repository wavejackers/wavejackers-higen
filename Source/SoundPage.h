// Ses tasarımı sekmesi: osilatörler, wavetable, filtre, zarf, LFO, efektler, önizleme ve ekran klavyesi
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Ui.h"

class SoundPage : public juce::Component
{
public:
    SoundPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~SoundPage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void setPart (int part);
    int currentPart() const noexcept { return part; }
    void tick();                       // editör zamanlayıcısından: önizleme ve ses listesi eşitlemesi
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int kW = 1164, kH = 654;

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Scope : public juce::Component
    {
        SoundPage* owner = nullptr; std::array<float, hg::kNumSp> v {};
        void paint (juce::Graphics&) override;
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false; int part = 0;
    std::array<juce::TextButton, 3> partBtns;
    juce::ComboBox baseBox; juce::StringArray baseIds; juce::String shownBase;
    juce::TextButton randBtn, resetBtn;
    juce::Label hint, kbdHint;
    std::map<int, std::unique_ptr<hgui::Knob>> knobs;
    std::map<int, std::unique_ptr<juce::ComboBox>> combos;
    std::map<int, std::unique_ptr<juce::ToggleButton>> toggles;
    std::vector<std::unique_ptr<SA>> sa; std::vector<std::unique_ptr<CA>> ca; std::vector<std::unique_ptr<BA>> ba;
    Scope scope;
    juce::MidiKeyboardComponent keyboard;
    std::vector<std::pair<juce::Component*, juce::Rectangle<int>>> layoutList;
    struct Title { juce::String key, arg; juce::Rectangle<int> r; bool small; };
    std::vector<Title> titles;
    std::vector<juce::Rectangle<int>> panels;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void bind();
    void fillBase();
    void fillCombos();
    void place (juce::Component& c, juce::Rectangle<int> r) { layoutList.push_back ({ &c, r }); }
    juce::Rectangle<int> mir (juce::Rectangle<int> r) const { return rtl ? r.withX (kW - r.getRight()) : r; }
    int trackOfPart() const { return part == 2 ? hg::kMelody : part; }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundPage)
};
