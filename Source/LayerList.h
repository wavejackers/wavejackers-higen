// Smart Detect katmanları: BASS / SYNTH / PAD / PERCUSSION hapları. Her hap gerçek analiz sonucundan beslenir
// (aday sayısı ve en iyi adayın sezgisel güven puanı); aday yoksa soluk ve tıklanamaz.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RefAnalysis.h"
#include "Ui.h"

void drawCategoryIcon (juce::Graphics& g, int cat, juce::Rectangle<float> r, juce::Colour col);

namespace hgui
{
class LayerList : public juce::Component
{
public:
    explicit LayerList (LookAndFeel& l) : lnf (l) { setWantsKeyboardFocus (false); }
    const std::vector<std::vector<hg::RefRegion>>* cands = nullptr;
    int selCat = -1, selRow = -1; bool playing = false; bool rtl = false;
    std::function<juce::Colour (int)> colour;
    std::function<juce::String (int)> name;
    juce::String noneText = "-", ofWord = "/";
    std::function<void (int)> onPick;       // hap: seçili değilse en iyi aday, seçiliyse sıradaki aday
    std::function<void()> onStop;           // seçili hapın durdur düğmesi

    juce::Rectangle<int> pill (int k) const { const int h = 50, gap = 8; return { 0, k * (h + gap), getWidth(), h }; }
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent& e) override { const int h = hit (e.getPosition()); if (h != hover) { hover = h; setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor); repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override;

private:
    LookAndFeel& lnf; int hover = -1;
    int count (int k) const { return cands != nullptr && k < (int) cands->size() ? (int) (*cands)[(size_t) k].size() : 0; }
    int hit (juce::Point<int> p) const { for (int k = 0; k < 4; ++k) if (pill (k).contains (p) && count (k) > 0) return k; return -1; }
    juce::Rectangle<float> playRect (int k) const { auto r = pill (k).toFloat(); return juce::Rectangle<float> (30, 30).withCentre ({ rtl ? r.getX() + 26 : r.getRight() - 26, r.getCentreY() }); }
};
} // namespace hgui
