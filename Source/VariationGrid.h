// Varyasyon ızgarası: aynı DNA'nın altı farklı tohumu yan yana. Her kutu, o varyasyonun gerçek ses parametrelerinden
// (kesim, rezonans, atak, sönüm, sürdürme, gürültü, sürücü, LFO derinliği) bir parmak izi çizer.
// Tıklama dinletir (parametreleri uygular), "Tut" ya da çift tıklama varyasyonu geçmişe ekler.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "SoundParams.h"
#include "Ui.h"

namespace hgui
{
class VariationGrid : public juce::Component
{
public:
    explicit VariationGrid (LookAndFeel& l) : lnf (l) { setWantsKeyboardFocus (true); setTitle ("Variations"); }
    struct Item { std::array<float, hg::kNumSp> v {}; juce::uint32 seed = 0; };
    std::vector<Item> items; int playing = -1;
    juce::String title, hint, keepWord, closeWord, seedWord = "Seed"; bool rtl = false;
    std::function<juce::String (int)> spName;
    std::function<void (int)> onAudition, onKeep; std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    bool keyPressed (const juce::KeyPress&) override;
    static const std::array<int, 8>& prints();     // parmak izinde gösterilen parametreler

private:
    LookAndFeel& lnf; int hover = -1;
    juce::Rectangle<int> tile (int i) const;
    juce::Rectangle<int> keepRect (int i) const { auto t = tile (i); return t.removeFromBottom (36).reduced (10, 4); }
    juce::Rectangle<int> closeRect() const { return { rtl ? 8 : getWidth() - 88, 8, 80, 28 }; }
    int hit (juce::Point<int> p) const { for (int i = 0; i < (int) items.size(); ++i) if (tile (i).contains (p)) return i; return -1; }
};
} // namespace hgui
