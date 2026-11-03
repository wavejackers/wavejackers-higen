// Kompakt makro satırı: var olan bir Slider'ın ikinci görünümü (değer ve geri çağrılar o kaydırıcıda kalır).
// Yatay/dikey sürükleme, tekerlek, çift tık (varsayılana dön) ve klavye ok tuşları desteklenir.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Ui.h"

namespace hgui
{
class MacroRow : public juce::Component, public juce::SettableTooltipClient, private juce::Slider::Listener
{
public:
    MacroRow (LookAndFeel& l, juce::Slider& s) : lnf (l), sl (s)
    {
        sl.addListener (this); setWantsKeyboardFocus (true); setRepaintsOnMouseActivity (true);
    }
    ~MacroRow() override { sl.removeListener (this); }      // kaydırıcı bu satırdan önce bildirilmiş olmalı
    void setName (const juce::String& n, const juce::String& help) { name = n; setTitle (n); setTooltip (help); setHelpText (help); repaint(); }
    bool rtl = false;

    void paint (juce::Graphics& g) override
    {
        const auto& P = lnf.pal(); const bool en = sl.isEnabled(); auto r = getLocalBounds().toFloat().reduced (1);
        const float h = lnf.hover (*this, isMouseOverOrDragging() && en);
        g.setColour (P.panel2.withAlpha (0.35f + 0.45f * h)); g.fillRoundedRectangle (r, 10);
        g.setColour ((hasKeyboardFocus (false) ? P.focus : P.line).withAlpha (en ? 1.0f : 0.5f)); g.drawRoundedRectangle (r, 10, 1.0f);
        const double lo = sl.getMinimum(), hi = sl.getMaximum(); const float t = hi > lo ? (float) ((sl.getValue() - lo) / (hi - lo)) : 0.0f;
        const float a = en ? 1.0f : 0.4f;
        // küçük halka: değer yayı
        auto ring = juce::Rectangle<float> (18, 18).withCentre ({ rtl ? r.getRight() - 22 : r.getX() + 22, r.getCentreY() - 3 });
        juce::Path bg; bg.addCentredArc (ring.getCentreX(), ring.getCentreY(), 8, 8, 0, -2.4f, 2.4f, true);
        g.setColour (P.line.withAlpha (a)); g.strokePath (bg, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        juce::Path fg; fg.addCentredArc (ring.getCentreX(), ring.getCentreY(), 8, 8, 0, -2.4f, -2.4f + 4.8f * t, true);
        g.setColour (P.accent.withAlpha (a)); g.strokePath (fg, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        auto txt = r.reduced (40, 0).withTrimmedBottom (6);
        g.setColour (P.ink.withAlpha (a)); g.setFont (lnf.font (14.0f, true));
        g.drawText (name, txt, rtl ? juce::Justification::centredRight : juce::Justification::centredLeft);
        g.setColour ((en ? P.accent : P.mute)); g.setFont (lnf.font (14.0f, true));
        g.drawText (juce::String (juce::roundToInt (sl.getValue())) + "%", txt.withTrimmedLeft (rtl ? 0 : -24).withTrimmedRight (rtl ? -24 : 0), rtl ? juce::Justification::centredLeft : juce::Justification::centredRight);
        // alt çizgi: değer çubuğu
        auto bar = juce::Rectangle<float> (r.getX() + 40, r.getBottom() - 9, r.getWidth() - 56, 2.0f);
        g.setColour (P.line.withAlpha (a)); g.fillRoundedRectangle (bar, 1);
        g.setColour (P.accent.withAlpha (a)); g.fillRoundedRectangle (rtl ? bar.withLeft (bar.getRight() - bar.getWidth() * t) : bar.withWidth (bar.getWidth() * t), 1);
    }

    void mouseDown (const juce::MouseEvent&) override { if (! sl.isEnabled()) return; grabKeyboardFocus(); startVal = sl.getValue(); sl.startedDragging(); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! sl.isEnabled()) return;
        const double range = sl.getMaximum() - sl.getMinimum(); const int dx = rtl ? -e.getDistanceFromDragStartX() : e.getDistanceFromDragStartX();
        const double fine = e.mods.isShiftDown() ? 0.2 : 1.0;
        sl.setValue (startVal + (dx - e.getDistanceFromDragStartY()) * range / 220.0 * fine);
    }
    void mouseUp (const juce::MouseEvent&) override { if (sl.isEnabled()) sl.stoppedDragging(); }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (sl.isEnabled() && sl.isDoubleClickReturnEnabled()) sl.setValue (sl.getDoubleClickReturnValue()); }
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w) override
    {
        if (! sl.isEnabled()) return; sl.setValue (sl.getValue() + (w.deltaY + w.deltaX) * (sl.getMaximum() - sl.getMinimum()) * 0.08);
    }
    bool keyPressed (const juce::KeyPress& k) override
    {
        if (! sl.isEnabled()) return false; const double st = (sl.getMaximum() - sl.getMinimum()) / 100.0 * (k.getModifiers().isShiftDown() ? 10 : 1);
        if (k == juce::KeyPress::rightKey || k == juce::KeyPress::upKey) { sl.setValue (sl.getValue() + st); return true; }
        if (k == juce::KeyPress::leftKey || k == juce::KeyPress::downKey) { sl.setValue (sl.getValue() - st); return true; }
        return false;
    }
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }
    void enablementChanged() override { repaint(); }

private:
    LookAndFeel& lnf; juce::Slider& sl; juce::String name; double startVal = 0;
    void sliderValueChanged (juce::Slider*) override { repaint(); }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacroRow)
};
} // namespace hgui
