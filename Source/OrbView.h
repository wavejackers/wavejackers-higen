// DNA Orb: referans sesin tamamını bir halka olarak gösterir. Her şey gerçek veriden çizilir:
// dış halka = dalga formu (tepe ve RMS), vuruş çentikleri = algılanan BPM, renkli yaylar = Smart Detect bölgeleri,
// ibre = gerçek çalma konumu, çekirdeğin parıltısı = eklentinin gerçek çıkış seviyesi.
// Durağan katmanlar bir görüntüde önbelleğe alınır; her karede yalnızca ibre, seviye ve metin çizilir.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "RefAnalysis.h"
#include "Ui.h"

namespace hgui
{
class OrbView : public juce::Component
{
public:
    struct Model
    {
        std::shared_ptr<const hg::RefClip> clip;
        std::vector<std::vector<hg::RefRegion>> cands;
        double selA = 0, selB = 0; int selCat = -1;
        double bpm = 0; int key = -1; bool minor = true;
        bool operator== (const Model& o) const;
    };

    explicit OrbView (LookAndFeel& l);
    std::function<juce::Colour (int)> catColour;
    std::function<void (double)> onSeek;          // halkaya tıklama: saniye
    std::function<void()> onOpen;                 // boşken tıklama: dosya aç
    juce::String emptyText, bpmWord = "BPM", topText, bottomText;
    bool rtl = false;

    void setModel (const Model& m);
    void setPlayhead (double sec, bool playing);
    void setLevel (float lv);                     // 0..1, editörün seviye göstergesinden
    void invalidate() { dirty = true; repaint(); }

    void paint (juce::Graphics&) override;
    void resized() override { dirty = true; }
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hoverSec = -1; repaint(); }
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    LookAndFeel& lnf; Model model;
    std::vector<float> peak, rms;                 // halka için 360 kutu, 0..1
    const hg::RefClip* peaksFor = nullptr;
    juce::Image cache; float cacheScale = 0; bool dirty = true;
    double pos = 0; bool playing = false; float level = 0, shownLevel = 0; double hoverSec = -1;

    void computePeaks();
    void renderStatic (juce::Graphics& g, float scale);
    juce::Point<float> centre() const { return getLocalBounds().toFloat().getCentre(); }
    float radius() const { return juce::jmin (getWidth(), getHeight()) * 0.5f - 4.0f; }
    double secAt (juce::Point<float> p) const;    // -1: halkanın dışında
    static juce::Point<float> polar (juce::Point<float> c, float r, float a) { return { c.x + r * std::sin (a), c.y - r * std::cos (a) }; }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrbView)
};
} // namespace hgui
