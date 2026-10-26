// Eklenti arayüz parçaları: tema, knob, piyano rulosu, akor şeridi, sürüklenebilir MIDI kutusu
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Generator.h"

class HitGenProcessor;

namespace hgui
{
struct Palette
{
    juce::Colour bg, panel, panel2, ink, mute, line, accent, accentInk, grid, grid2, focus;
    juce::Colour track[4];
    static Palette make (bool highContrast, juce::Colour accent);
};

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel();
    void setLanguage (const juce::String& lang);
    void setPalette (const Palette& p);
    const Palette& pal() const noexcept { return palette; }
    juce::Font font (float h, bool bold = false) const;

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font&) override;
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (14); }
    juce::Font getPopupMenuFont() override { return font (15); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return font (juce::jmin (15.0f, h * .5f), true); }
    juce::Font getLabelFont (juce::Label& l) override { return font (l.getFont().getHeight(), l.getFont().isBold()); }
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawFocusRing (juce::Graphics& g, juce::Rectangle<float> r, float radius);

private:
    Palette palette;
    juce::Typeface::Ptr latR, latB, arR, arB, hiR, hiB, curR, curB;
};

// Etiketli döner knob (ekran okuyucu başlığı ve açıklaması ile)
class Knob : public juce::Component
{
public:
    Knob();
    juce::Slider slider; juce::Label caption;
    void setText (const juce::String& title, const juce::String& help);
    void resized() override;
};

// Dört izli piyano rulosu; erişilebilirlik için görüntü rolü ve metin özeti
class PianoRoll : public juce::Component
{
public:
    PianoRoll();
    void setResult (std::shared_ptr<const hg::Result> r, const juce::StringArray& trackNames, const juce::String& summary);
    void setPlayhead (double ppqInLoop, bool playing);
    void setRtl (bool r) { rtl = r; repaint(); }
    void paint (juce::Graphics&) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    std::function<const Palette&()> palette;
    std::array<juce::Rectangle<int>, 4> laneBounds() const;
private:
    std::shared_ptr<const hg::Result> res; juce::StringArray names; double ph = -1; bool rtl = false;
};

class ChordStrip : public juce::Component
{
public:
    void setResult (std::shared_ptr<const hg::Result> r, const juce::String& barWord) { res = std::move (r); bar = barWord; repaint(); }
    void setCurrentBar (int b) { if (b != cur) { cur = b; repaint(); } }
    void setRtl (bool r) { rtl = r; repaint(); }
    void paint (juce::Graphics&) override;
    std::function<const Palette&()> palette; std::function<juce::Font (float, bool)> font;
private:
    std::shared_ptr<const hg::Result> res; juce::String bar; int cur = -1; bool rtl = false;
};

// DAW'a sürüklenebilen MIDI kutusu; Enter/Boşluk ile dosyaya kaydeder
class DragTile : public juce::Component, public juce::SettableTooltipClient
{
public:
    DragTile();
    std::function<juce::File()> makeFile;
    std::function<void (const juce::File&)> onSaved;
    juce::String label, hint;
    juce::Colour colour;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }
    void saveAs();
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    std::function<const Palette&()> palette; std::function<juce::Font (float, bool)> font;
private:
    bool dragging = false; std::unique_ptr<juce::FileChooser> chooser;
};
}
