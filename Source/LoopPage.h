// Loop sekmesi (LoopForge): ses yükle ya da yakala, adım ızgarasında ritmi ve perdeyi kur, döngüyü çal ya da DAW'a sürükle.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Ui.h"

class LoopPage : public juce::Component
{
public:
    LoopPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    void setLanguage (const juce::String& lang, bool rtl);
    void tick();                                   // editör zamanlayıcısından: çalma başlığı ve yeni döngü
    bool openFile (const juce::File&);             // sürükle-bırak ve "Ses yükle"
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int kW = 1164, kH = 654;
    // testler
    lf::Settings settings() const { return cur; }
    void apply (const lf::Settings& s) { cur = s; commit(); syncControls(); }
    void mutateNow (lf::MutateMode m);
    void clickStep (int lane, int step, float y01);   // lane: 0 tetik, 1 perde, 2 değer, 3 ters, 4 ratchet

private:
    struct Wave : juce::Component
    {
        LoopPage& o; std::vector<float> pk; int drag = -1;
        explicit Wave (LoopPage& p) : o (p) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
    };
    struct Grid : juce::Component
    {
        LoopPage& o; int paintOn = -1;
        explicit Grid (LoopPage& p) : o (p) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent& e) override { hit (e, true); }
        void mouseDrag (const juce::MouseEvent& e) override { hit (e, false); }
        void hit (const juce::MouseEvent&, bool down);
        juce::Rectangle<int> lane (int i) const;
    };
    struct DragHandle : juce::TextButton   // sürükleyince döngüyü WAV olarak DAW'a bırakır
    {
        LoopPage& o; bool busy = false; explicit DragHandle (LoopPage& p) : o (p) {}
        void mouseDrag (const juce::MouseEvent&) override;
    };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; lf::Settings cur; int shownVersion = -1, valueLane = 0;
    juce::TextButton loadBtn, captureBtn, playBtn, mutateBtn; DragHandle exportBtn { *this };
    juce::ComboBox barsBox, stepsBox, rootBox, scaleBox, laneBox, modeBox;
    std::array<hgui::Knob, 7> knobs;   // pulses, rotation, swing, humanize, space, drive, motion
    juce::Label status; Wave wave { *this }; Grid grid { *this };
    std::unique_ptr<juce::FileChooser> chooser;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;   // makrolar ve Öklid: DAW otomasyonu
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void commit() { proc.loop.set (cur); grid.repaint(); wave.repaint(); }
    void syncControls();
    void applyEuclid();
    float& value (lf::Step& s) const { return valueLane == 0 ? s.vel : valueLane == 1 ? s.cut : valueLane == 2 ? s.pan : valueLane == 3 ? s.decay : valueLane == 4 ? s.shift : s.attack; }
    bool bipolar() const noexcept { return valueLane == 2 || valueLane == 4; }   // pan ve zaman kaydırma: ortada sıfır
    void setParam (const char* id, float plain);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoopPage)
};
