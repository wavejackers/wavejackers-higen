// Kılavuz ve asistan (2.7)
//  - AssistantOrb: tek gözlü, gölgelendirilmiş "3B" küre karakter. Göz fareyi izler, ara sıra kırpar; yeni öneri varsa rozet gösterir.
//  - AssistantBubble: asistanın öneri kartı (metin, Uygula, Sonraki, Kılavuz, Kapat).
//  - GuideOverlay: ilk açılışta (ve Yardım menüsünden) adım adım kılavuz; hedef alanı vurgular, diğer yerleri karartır.
// Öneriler eklentinin kendi ayarlarına bakan kurallarla üretilir (PluginEditor); internet ya da bulut hizmeti kullanılmaz.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Ui.h"

namespace hgui
{
class AssistantOrb : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit AssistantOrb (LookAndFeel& l) : lnf (l) { setWantsKeyboardFocus (true); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override { if (e.mouseWasClicked() && onClick) onClick(); }
    bool keyPressed (const juce::KeyPress& k) override { if ((k == juce::KeyPress::returnKey || k == juce::KeyPress::spaceKey) && onClick) { onClick(); return true; } return false; }
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }
    void step (juce::Point<float> mouseInParent, bool animate);   // editör zamanlayıcısı: bakış, kırpma, süzülme
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    std::function<void()> onClick;
    bool badge = false, talking = false;

private:
    LookAndFeel& lnf;
    juce::Point<float> look, lookTarget; float blink = 0.0f, bob = 0.0f; int blinkWait = 90, frame = 0;
};

class AssistantBubble : public juce::Component
{
public:
    explicit AssistantBubble (LookAndFeel& l);
    void paint (juce::Graphics&) override;
    void resized() override;
    void show (const juce::String& title, const juce::String& text, const juce::String& counter, bool canApply, const juce::String& applyText);
    int preferredHeight (int width) const;                      // metne göre kart yüksekliği
    juce::TextButton applyBtn, nextBtn, guideBtn, closeBtn;
    juce::String footer; bool rtl = false;

private:
    LookAndFeel& lnf; juce::String heading, body, count;
};

class GuideOverlay : public juce::Component
{
public:
    struct Step { juce::String title, body; std::function<juce::Rectangle<int>()> target; std::function<void()> enter; };
    explicit GuideOverlay (LookAndFeel& l);
    void start (std::vector<Step> steps);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override {}         // arkadaki bileşenlere tıklanmasın
    int current() const noexcept { return idx; }
    int count() const noexcept { return (int) steps.size(); }
    void go (int i);
    void next();
    void finish();                                               // gizle ve onFinish
    std::function<void()> onFinish;                              // bitti ya da atlandı
    juce::String backText, nextText, doneText, skipText, stepWord; bool rtl = false;

private:
    LookAndFeel& lnf; std::vector<Step> steps; int idx = 0;
    juce::TextButton backBtn, nextBtn, skipBtn;
    juce::Rectangle<int> hole, card;
    void layoutCard();
};
}
