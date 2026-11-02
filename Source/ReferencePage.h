// Referans sekmesi: ses içe aktarma (sürükle-bırak / dosya seçici), dalga formu, SMART DETECT, bölge adayları,
// orijinali çalma, Track DNA ve BENZERİNİ ÜRET. Ağır işler RefEngine'in arka plan iş parçacığında yapılır.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Ui.h"

class ReferencePage : public juce::Component, private juce::ScrollBar::Listener
{
public:
    ReferencePage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~ReferencePage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void tick();                        // editör zamanlayıcısı (30 Hz)
    void paint (juce::Graphics&) override;
    void resized() override;
    void openFile (const juce::File& f);
    int currentPart() const noexcept { return part; }
    static constexpr int kW = 1164, kH = 654;

    // testler / otomasyon
    void selectCandidate (int cat, int row);
    void generateNow() { generate(); }
    void showSampler (bool on) { setView (on ? 1 : 0); }
    juce::String statusText() const { return status.getText(); }

private:
    struct Wave : public juce::Component
    {
        ReferencePage* o = nullptr; bool dragging = false; double dragA = 0;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
        double xToSec (float x) const; float secToX (double s) const;
    };
    struct Cands : public juce::Component
    {
        ReferencePage* o = nullptr; void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override;
        juce::Rectangle<int> rowRect (int cat, int row) const;
    };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false; int part = 1;          // part: 0 akor, 1 bas, 2 melodi
    juce::TextButton openBtn, smartBtn, playBtn, applyBtn, zoomIn, zoomOut, zoomFit, genBtn, prevBtn, nextBtn, saveDnaBtn, loadDnaBtn, seedNewBtn;
    juce::ToggleButton loopBtn, previewBtn;
    std::array<juce::TextButton, 3> partBtns;
    hgui::Knob simK, varK; juce::Slider gainS; juce::Label gainL;
    juce::Slider seed; juce::Label seedLabel, status, info, dnaInfo, banner, histLabel;
    // örnekleyici görünümü (Studio): kök nota, ADSR, polifoni, transpoze, kırpma, döngü, preset
    int view = 0;                                                      // 0 sentez (DNA), 1 örnekleyici
    juce::TextButton viewSynth, viewSamp, sTrimBtn, sLoopSelBtn, sSaveBtn, sLoadBtn;
    juce::ToggleButton sEnable, sLoop; juce::ComboBox rootBox; juce::Slider trS, voiceS; juce::Label rootL, trL, voiceL, sInfo;
    std::array<hgui::Knob, 6> sKnobs;                                  // A D S R Gain Vel
    void setView (int v); void syncSampler(); void trimFromSelection(); void saveSampler(); void loadSampler();
    Wave wave; Cands cands; juce::ScrollBar bar { false };
    std::unique_ptr<juce::FileChooser> chooser;

    // görünüm / seçim
    double viewStart = 0, viewLen = 10; double selA = 0, selB = 0; int selCat = -1;
    std::vector<std::vector<hg::RefRegion>> candList;
    // DNA ve geçmiş
    bool haveDna = false; hg::TrackDna dna; double dnaA = -1, dnaB = -1; int dnaRole = -1; juce::String dnaClip;
    struct Hist { std::array<float, hg::kNumSp> v; juce::uint32 seed; }; std::vector<Hist> history; int histPos = -1;
    // önizleme cümlesi
    struct Ev { double at; int note; bool on; }; std::vector<Ev> phrase; double phraseT0 = 0; bool phraseRun = false; 
    juce::uint32 genCount = 0; int lastState = -1; int shownClipId = 0;

    void scrollBarMoved (juce::ScrollBar*, double newStart) override;
    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void relabel();
    void refreshAfterChange();
    void setView (double start, double len);
    void setSelection (double a, double b, int cat);
    void togglePlay();
    void generate();
    void doGenerate (const hg::TrackDna& d);
    void pushHistory (const float* vals, juce::uint32 sd);
    void applyHistory (int pos);
    void startPhrase();
    void saveDna(); void loadDna();
    static juce::String fmtTime (double s);
    juce::Colour catColour (int c) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferencePage)
};
