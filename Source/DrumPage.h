// Davul makinesi sayfası: yedi ped (Kick, Snare, Clap, Hi-hat, Açık hat, Rim, Perküsyon).
// Yaratıcı mod açıkken her pede örnek yüklenebilir (sürükle-bırak ya da düğme) ve Punch / Soft / Mid / High,
// Ton, Sönüm ve Seviye ile şekillendirilir. Kapalıyken stilin analog kiti çalar. Kitler .hgkit olarak kaydedilir.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Ui.h"

class DrumPage : public juce::Component
{
public:
    DrumPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~DrumPage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void tick();                                  // editör zamanlayıcısından: ped ışıkları, örnek adları, mod
    void paint (juce::Graphics&) override;
    void resized() override;
    std::function<void (const juce::String&)> onMessage;   // kısa bildirim (editör altta gösterir)
    std::function<void()> onKitChanged;                    // stil kiti değişti (ses kutularını yenile)
    static constexpr int kW = 1164, kH = 654;
    // testler için
    juce::Component& padComponent (int pad);
    void loadSampleFile (int pad, const juce::File& f);

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Pad : public juce::Component, public juce::FileDragAndDropTarget, public juce::SettableTooltipClient
    {
        DrumPage* owner = nullptr; int index = 0; float flash = 0.0f; bool dragOver = false;
        juce::String name, source;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        bool keyPressed (const juce::KeyPress&) override;
        void focusGained (FocusChangeType) override { repaint(); }
        void focusLost (FocusChangeType) override { repaint(); }
        bool isInterestedInFileDrag (const juce::StringArray& files) override;
        void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
        void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
        void filesDropped (const juce::StringArray& files, int, int) override;
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false;
    juce::ToggleButton creativeBtn; std::unique_ptr<BA> creativeBa;
    juce::ComboBox kitBox; juce::StringArray kitIds;
    juce::TextButton saveKitBtn, loadKitBtn, resetBtn;
    juce::Label title, hint, kitLabel;
    std::array<Pad, hg::kNumPads> pads;
    std::array<juce::TextButton, hg::kNumPads> loadBtns, clearBtns;
    std::array<std::array<hgui::Knob, HitGenProcessor::kDmCtl>, hg::kNumPads> knobs;
    std::vector<std::unique_ptr<SA>> sa;
    std::unique_ptr<juce::FileChooser> chooser;
    std::array<juce::String, hg::kNumPads> shownPaths; bool shownCreative = false; juce::String shownKit;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    juce::Rectangle<int> mir (juce::Rectangle<int> r) const { return rtl ? r.withX (kW - r.getRight()) : r; }
    juce::Rectangle<int> column (int pad) const;
    void chooseSample (int pad);
    void refreshPads();
    void fillKits();
    void saveKit();
    void loadKitMenu();
    void say (const juce::String& s) { if (onMessage) onMessage (s); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumPage)
};
