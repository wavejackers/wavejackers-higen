// Koro sekmesi: sesini kaydet ya da bir ses yükle (ya da dahili koroyu kullan); koro akorları izler.
// Sahnedeki melekler partilerdir (bas, tenor, alto, soprano): tıklayınca parti açılır / kapanır.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Stage3D.h"
#include "Ui.h"

class ChoirPage : public juce::Component, private juce::AudioIODeviceCallback
{
public:
    ChoirPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~ChoirPage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void tick (float level);                        // editör zamanlayıcısından: sahne, kayıt göstergesi, yeni döngü
    bool openFile (const juce::File&);
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int kW = 1164, kH = 654;
    // testler
    void applyPreset (int i);                       // 0 katedral, 1 Gregoryen, 2 melekler, 3 stab
    void togglePart (int part);
    bool toggleRecord();                            // kayıt başlat / bitir; mikrofon açılamazsa false
    juce::String statusText() const { return status.getText(); }

private:
    struct StageView : juce::Component, juce::SettableTooltipClient
    {
        ChoirPage& o; juce::Image img; s3d::Stage stage; s3d::Pose pose; bool dirty = true;
        explicit StageView (ChoirPage& p) : o (p) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
    };
    struct DragHandle : juce::TextButton
    {
        ChoirPage& o; bool busy = false; explicit DragHandle (ChoirPage& p) : o (p) {}
        void mouseDrag (const juce::MouseEvent&) override;
    };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; ch::Settings cur; int shownVersion = -1, frame = 0; float smooth[4] = { 0, 0, 0, 0 }, lvl = 0; double clock = 0;
    juce::TextButton recBtn, loadBtn, playBtn; DragHandle exportBtn { *this };
    juce::ComboBox sourceBox, rhythmBox, sizeBox, presetBox;
    std::array<hgui::Knob, 7> knobs;                // seviye, mekan, kalabalık, hava, vibrato, atak, karakter
    juce::Label status; StageView stage { *this }; std::vector<float> pk;
    std::unique_ptr<juce::FileChooser> chooser;
    std::unique_ptr<juce::AudioDeviceManager> mic; std::atomic<double> micRate { 0 };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sa;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> playAtt;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void commit() { proc.choir.set (cur); proc.pushChoirParams(); stage.dirty = true; }
    void syncControls();
    void stopMic();
    juce::String voiceLine() const;
    void audioDeviceIOCallbackWithContext (const float* const* in, int numIn, float* const* out, int numOut, int n, const juce::AudioIODeviceCallbackContext&) override;
    void audioDeviceAboutToStart (juce::AudioIODevice* d) override { micRate.store (d != nullptr ? d->getCurrentSampleRate() : 0.0); }
    void audioDeviceStopped() override {}
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChoirPage)
};
