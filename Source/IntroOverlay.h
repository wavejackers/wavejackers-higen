// Açılış ekranı: gece zemininde marka yazısı belirir, asistan gözü arayüzün üzerinde gezinir ve yerine oturur, sonra arayüz açılır.
// Tıklama ya da herhangi bir tuş atlar. Hareket kapalıysa göz gezmez, kısa bir geçişle açılır.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Ui.h"

class IntroOverlay : public juce::Component
{
public:
    explicit IntroOverlay (hgui::LookAndFeel& l);
    void begin (bool motion, juce::Rectangle<int> eyeHome);
    // editör zamanlayıcısından (ya da testten) çağrılır: geçen süre ms; gözün o anki yerini döner
    juce::Rectangle<int> advance (double ms);
    juce::Point<float> eyeLook() const { return look; }
    bool running() const { return on; }
    void finish();
    void setSkipText (const juce::String& s) { skipText = s; setDescription (s); }
    std::function<void()> onFinished, onSkip;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { skip(); }
    bool keyPressed (const juce::KeyPress&) override { skip(); return true; }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    static constexpr double kTotalMs = 5800.0, kRevealMs = 4600.0;

private:
    void skip() { if (on) { if (onSkip) onSkip(); finish(); } }
    hgui::LookAndFeel& lnf;
    bool on = false, motion = true; double t = 0.0; juce::Rectangle<int> home; juce::Point<float> look; juce::String skipText { "Click or press any key to skip" };
};
