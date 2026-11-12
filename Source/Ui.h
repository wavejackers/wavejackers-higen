// Eklenti arayüz parçaları: tema, knob, piyano rulosu, akor şeridi, sürüklenebilir MIDI kutusu
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Generator.h"
#include <map>

class HitGenProcessor;

namespace hgui
{
struct Palette
{
    juce::Colour bg, panel, panel2, ink, mute, line, accent, accent2, accentInk, grid, grid2, focus, success, warning;
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
    // Üzerine gelince yumuşak geçiş: çizim sırasında hedef kaydedilir, editörün zamanlayıcısı (arayüz iş parçacığı) değeri yaklaştırır.
    float hover (const juce::Component& c, bool over);
    void stepHover (bool animate);

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
    struct HoverState { juce::Component::SafePointer<juce::Component> comp; float amt = 0, target = 0; };
    std::map<const juce::Component*, HoverState> hov;
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

    // ---- düzenleme: bir izi tam yükseklikte gösterir; tıkla = nota ekle, boş yerde basılı sürükle = boya (FL'deki paint gibi),
    // notayı sürükle = taşı, sağ kenar = uzat, sağ tık / çift tık / Delete = sil, sağ tuş basılı sürükle = yol boyunca sil, tekerlek = şiddet, oklar = taşı, Ctrl+Z / Ctrl+Y = geri al / yinele
    void setEditing (bool on, int track);
    bool isEditing() const noexcept { return editing; }
    int editTrack() const noexcept { return lane; }
    bool undo(); bool redo();
    bool canUndo() const noexcept { return ! undoStack.empty(); }
    bool canRedo() const noexcept { return ! redoStack.empty(); }
    std::function<void (std::shared_ptr<hg::Result>)> onEdit;   // her tamamlanan düzenlemede yeni sonuç
    std::function<void()> onHistoryChanged;
    std::function<juce::String (int)> noteName;                 // nota adı (dil ve ton)
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    std::shared_ptr<const hg::Result> res; juce::StringArray names; double ph = -1; bool rtl = false;
    bool editing = false; int lane = 3, lo = 48, hi = 72; double view0 = 0;   // düzenlemede görünen pencere: 4 ölçü, tekerlekle kaydırılır
    double viewLen() const noexcept { return work ? std::min (64.0, work->bars * 16.0) : 64.0; }
    std::shared_ptr<hg::Result> work;               // düzenlenen kopya
    int sel = -1; enum class Drag { none, move, resize, paint, erase } drag = Drag::none; hg::NoteEv orig; bool changed = false; double grabOff = 0;
    double lastPaint = -1; juce::Point<float> lastPos;   // boyama: son boyanan adım; silme: son fare konumu (hızlı sürüklemede ara noktalar da silinir)
    std::vector<std::shared_ptr<const hg::Result>> undoStack, redoStack;
    juce::Rectangle<float> editArea() const;
    int rows() const noexcept;
    double stepAt (float x) const; int noteAtY (float y) const; float xOf (double s) const; float yOf (int note) const; float rowH() const;
    int hitNote (juce::Point<float> p, bool& onEdge) const;
    void fitRange();
    void beginChange(); void commit();
    void paintEditor (juce::Graphics&);
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
// MIDI Learn: bir kaydırıcıyı eklenti parametresine etiketler (sağ tık menüsü ve CC rozeti bu etiketi okur)
inline void tagParam (juce::Component& c, const juce::String& paramId) { c.getProperties().set ("hgParam", paramId); }

}
