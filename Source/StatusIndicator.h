// İşlem durumu göstergesi: Hazır / İşleniyor / Tamamlandı / Hata. Değerler yalnızca gerçek iş durumundan beslenir;
// ilerleme çubuğu analiz ilerlemesini (0..1) gösterir, ilerleme bilinmiyorsa belirsiz kayan çubuk çizilir.
#pragma once
#include "Ui.h"

namespace hgui
{
class StatusIndicator : public juce::Component
{
public:
    enum State { Ready, Processing, Completed, Error };
    explicit StatusIndicator (const LookAndFeel& l) : lnf (l) { setInterceptsMouseClicks (false, false); setAccessible (false); }

    void set (State s, float progressOrNeg, const juce::String& word)
    {
        if (s != state || std::abs (progressOrNeg - prog) > 0.004f || word != label) { state = s; prog = progressOrNeg; label = word; repaint(); }
    }
    void advance() { if (state == Processing) { sweep += 0.045f; if (sweep > 1.0f) sweep -= 1.0f; repaint(); } }   // 30 Hz editör zamanlayıcısından, yalnızca işlem sürerken
    State getState() const noexcept { return state; }

    void paint (juce::Graphics& g) override
    {
        const auto& P = lnf.pal(); const auto r = getLocalBounds().toFloat();
        const auto col = state == Ready ? P.mute : state == Processing ? P.accent : state == Completed ? P.success : P.warning;
        g.setColour (col); g.fillEllipse (4.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.setFont (lnf.font (11.5f, true)); g.setColour (col.interpolatedWith (P.ink, 0.35f));
        g.drawText (label, juce::Rectangle<float> (18.0f, 0, r.getWidth() - 18.0f, r.getHeight() - 4.0f), juce::Justification::centredLeft);
        if (state == Processing)
        {
            const auto bar = juce::Rectangle<float> (0, r.getBottom() - 3.0f, r.getWidth(), 2.0f);
            g.setColour (P.line); g.fillRect (bar);
            g.setColour (P.accent);
            if (prog >= 0.0f) g.fillRect (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, prog)));
            else { const float w = bar.getWidth() * 0.25f, x = (bar.getWidth() + w) * sweep - w; g.fillRect (juce::Rectangle<float> (juce::jmax (0.0f, x), bar.getY(), juce::jmin (x + w, bar.getWidth()) - juce::jmax (0.0f, x), bar.getHeight())); }
        }
    }

private:
    const LookAndFeel& lnf; State state = Ready; float prog = -1.0f; float sweep = 0; juce::String label;
};
}
