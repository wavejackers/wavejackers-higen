// Karakter sahnesi (isteğe bağlı katman): bilge = ana çıkış, melek 1 = akorlar, melek 2 = melodi.
// Yalnızca arayüz iş parçacığında çizilir; ses iş parçacığından sadece atomik seviyeleri okur.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Synth.h"
#include "Ui.h"

class CharacterStage : public juce::Component
{
public:
    CharacterStage (hg::Engine& e, hgui::LookAndFeel& l);
    void setLabels (const juce::String& elder, const juce::String& chords, const juce::String& melody) { names[0] = elder; names[1] = chords; names[2] = melody; repaint(); }
    // animMode: 0 kapalı (duran portre), 1 düşük (yalnızca ışık), 2 yüksek (ışık + hale + parıltı)
    void tick (int animMode);
    void paint (juce::Graphics&) override;
private:
    hg::Engine& eng; hgui::LookAndFeel& lnf;
    juce::Image img[3]; juce::String names[3];
    float lvl[3] { 0, 0, 0 }; int mode = 2;
};
