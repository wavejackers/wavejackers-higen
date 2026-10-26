#include "Ui.h"
#include "BinaryData.h"

namespace hgui
{
using namespace juce;

Palette Palette::make (bool hc, Colour accent)
{
    Palette p;
    if (hc)
    {
        p.bg = Colours::black; p.panel = Colours::black; p.panel2 = Colour (0xff101010); p.ink = Colours::white; p.mute = Colour (0xffe6e6e6);
        p.line = Colours::white; p.accent = Colour (0xffffd400); p.accentInk = Colours::black; p.grid = Colour (0xff3a3a3a); p.grid2 = Colour (0xff8a8a8a); p.focus = Colour (0xff00e5ff);
        p.track[0] = Colour (0xffa8b2ff); p.track[1] = Colour (0xff5cf2cf); p.track[2] = Colour (0xffff8fbb); p.track[3] = Colour (0xffffd45c);
    }
    else
    {
        p.bg = Colour (0xff090b0e); p.panel = Colour (0xff11151a); p.panel2 = Colour (0xff171c23); p.ink = Colour (0xffeaf0f5); p.mute = Colour (0xff9aa5b1);
        p.line = Colour (0xff2a3340); p.accent = accent; p.accentInk = accent.getPerceivedBrightness() > .55f ? Colour (0xff05090a) : Colours::white;
        p.grid = Colour (0xff1a2129); p.grid2 = Colour (0xff2e3946); p.focus = Colour (0xff9db4ff);
        p.track[0] = Colour (0xff8f9bff); p.track[1] = Colour (0xff3fd9b8); p.track[2] = Colour (0xffff72a4); p.track[3] = Colour (0xfff5c04b);
    }
    return p;
}

LookAndFeel::LookAndFeel()
{
    latR = Typeface::createSystemTypefaceFor (BinaryData::NotoSansRegular_ttf, BinaryData::NotoSansRegular_ttfSize);
    latB = Typeface::createSystemTypefaceFor (BinaryData::NotoSansBold_ttf, BinaryData::NotoSansBold_ttfSize);
    arR = Typeface::createSystemTypefaceFor (BinaryData::NotoSansArabicRegular_ttf, BinaryData::NotoSansArabicRegular_ttfSize);
    arB = Typeface::createSystemTypefaceFor (BinaryData::NotoSansArabicBold_ttf, BinaryData::NotoSansArabicBold_ttfSize);
    hiR = Typeface::createSystemTypefaceFor (BinaryData::NotoSansDevanagariRegular_ttf, BinaryData::NotoSansDevanagariRegular_ttfSize);
    hiB = Typeface::createSystemTypefaceFor (BinaryData::NotoSansDevanagariBold_ttf, BinaryData::NotoSansDevanagariBold_ttfSize);
    setLanguage ("en");
    setPalette (Palette::make (false, Colour (0xff2fd6c6)));
}

void LookAndFeel::setLanguage (const String& lang)
{
    if (lang == "ar") { curR = arR; curB = arB; }        // Arapça ve Devanagari yazı tipleri Latin harfleri de içerir
    else if (lang == "hi") { curR = hiR; curB = hiB; }
    else { curR = latR; curB = latB; }
    if (curR == nullptr) curR = latR;
    if (curB == nullptr) curB = curR;
}

Font LookAndFeel::font (float h, bool bold) const
{
    auto tf = bold ? curB : curR;
    return tf != nullptr ? Font (FontOptions (tf).withHeight (h)) : Font (FontOptions (h, bold ? Font::bold : Font::plain));
}

Typeface::Ptr LookAndFeel::getTypefaceForFont (const Font& f)
{
    auto tf = f.isBold() ? curB : curR;
    return tf != nullptr ? tf : LookAndFeel_V4::getTypefaceForFont (f);
}

void LookAndFeel::setPalette (const Palette& p)
{
    palette = p;
    setColour (ResizableWindow::backgroundColourId, p.bg);
    setColour (Label::textColourId, p.ink);
    setColour (TextButton::buttonColourId, p.panel2); setColour (TextButton::buttonOnColourId, p.accent);
    setColour (TextButton::textColourOffId, p.ink); setColour (TextButton::textColourOnId, p.accentInk);
    setColour (ComboBox::backgroundColourId, p.panel2); setColour (ComboBox::textColourId, p.ink); setColour (ComboBox::outlineColourId, p.line); setColour (ComboBox::arrowColourId, p.mute);
    setColour (PopupMenu::backgroundColourId, p.panel2); setColour (PopupMenu::textColourId, p.ink); setColour (PopupMenu::highlightedBackgroundColourId, p.accent); setColour (PopupMenu::highlightedTextColourId, p.accentInk);
    setColour (Slider::textBoxTextColourId, p.ink); setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack); setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (Slider::rotarySliderFillColourId, p.accent); setColour (Slider::rotarySliderOutlineColourId, p.line);
    setColour (TextEditor::backgroundColourId, p.panel2); setColour (TextEditor::textColourId, p.ink); setColour (TextEditor::outlineColourId, p.line); setColour (TextEditor::focusedOutlineColourId, p.focus);
    setColour (ToggleButton::textColourId, p.ink); setColour (ToggleButton::tickColourId, p.accent);
    setColour (ListBox::backgroundColourId, p.panel); setColour (ListBox::outlineColourId, p.line);
    setColour (TooltipWindow::backgroundColourId, p.panel2); setColour (TooltipWindow::textColourId, p.ink); setColour (TooltipWindow::outlineColourId, p.line);
    setColour (ScrollBar::thumbColourId, p.grid2);
    setColour (CaretComponent::caretColourId, p.accent);
}

void LookAndFeel::drawFocusRing (Graphics& g, Rectangle<float> r, float radius)
{
    g.setColour (palette.focus); g.drawRoundedRectangle (r.reduced (1), radius, 2.0f);
}

void LookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4);
    const float r = jmin (b.getWidth(), b.getHeight()) / 2, cx = b.getCentreX(), cy = b.getCentreY(), ang = a0 + pos * (a1 - a0);
    Path track; track.addCentredArc (cx, cy, r - 3, r - 3, 0, a0, a1, true);
    g.setColour (palette.line); g.strokePath (track, PathStrokeType (4, PathStrokeType::curved, PathStrokeType::rounded));
    Path val; val.addCentredArc (cx, cy, r - 3, r - 3, 0, a0, ang, true);
    g.setColour (s.isEnabled() ? palette.accent : palette.mute); g.strokePath (val, PathStrokeType (4, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (palette.ink);
    g.drawLine (cx + (r * .3f) * std::sin (ang), cy - (r * .3f) * std::cos (ang), cx + (r - 6) * std::sin (ang), cy - (r - 6) * std::cos (ang), 2.4f);
    if (s.hasKeyboardFocus (false)) drawFocusRing (g, Rectangle<float> (cx - r - 2, cy - r - 2, 2 * r + 4, 2 * r + 4), r + 2);
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1);
    const bool on = b.getToggleState();
    g.setColour (on ? palette.accent : palette.panel2.brighter (down ? .15f : 0));
    g.fillRoundedRectangle (r, 8);
    g.setColour (over || on ? palette.accent : palette.line); g.drawRoundedRectangle (r, 8, 1);
    if (b.hasKeyboardFocus (false)) drawFocusRing (g, r.expanded (1), 9);
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setFont (font (jmin (15.0f, b.getHeight() * .45f), true));
    g.setColour (b.getToggleState() ? palette.accentInk : (b.isEnabled() ? palette.ink : palette.mute));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (8, 2), Justification::centred, 2, .85f);
}

void LookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat();
    const bool rtl = b.getProperties()["rtl"];
    auto box = rtl ? Rectangle<float> (r.getRight() - 40, r.getCentreY() - 10, 36, 20) : Rectangle<float> (r.getX() + 2, r.getCentreY() - 10, 36, 20);
    g.setColour (b.getToggleState() ? palette.accent : palette.panel2); g.fillRoundedRectangle (box, 10);
    g.setColour (over ? palette.accent : palette.line); g.drawRoundedRectangle (box, 10, 1);
    g.setColour (b.getToggleState() ? palette.accentInk : palette.mute);
    const float kx = b.getToggleState() ? box.getRight() - 17 : box.getX() + 3; g.fillEllipse (kx, box.getY() + 3, 14, 14);
    g.setColour (palette.ink); g.setFont (font (14));
    auto tr = rtl ? r.withTrimmedRight (46) : r.withTrimmedLeft (46);
    g.drawFittedText (b.getButtonText(), tr.toNearestInt(), rtl ? Justification::centredRight : Justification::centredLeft, 2, .85f);
    if (b.hasKeyboardFocus (false)) drawFocusRing (g, r, 8);
}

void LookAndFeel::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox& c)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (1);
    g.setColour (palette.panel2); g.fillRoundedRectangle (r, 8);
    g.setColour (c.isMouseOver (true) ? palette.accent : palette.line); g.drawRoundedRectangle (r, 8, 1);
    Path arrow; const float ax = w - 16.0f, ay = h * .5f;
    arrow.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
    g.setColour (palette.mute); g.fillPath (arrow);
    if (c.hasKeyboardFocus (true)) drawFocusRing (g, r, 9);
}

Label* LookAndFeel::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (12)); l->setColour (Label::textColourId, palette.ink); l->setJustificationType (Justification::centred);
    return l;
}

// ---------------------------------------------------------------- Knob
Knob::Knob()
{
    slider.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (Slider::TextBoxBelow, false, 70, 16);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setWantsKeyboardFocus (true);
    caption.setJustificationType (Justification::centred);
    caption.setFont (FontOptions (12.0f));
    caption.setInterceptsMouseClicks (false, false);
    caption.setAccessible (false);   // başlık zaten kaydırıcının adında
    addAndMakeVisible (slider); addAndMakeVisible (caption);
}
void Knob::setText (const String& title, const String& help)
{
    caption.setText (title, dontSendNotification);
    slider.setTitle (title); slider.setHelpText (help); slider.setTooltip (help.isNotEmpty() ? help : title);
}
void Knob::resized()
{
    auto r = getLocalBounds();
    caption.setBounds (r.removeFromBottom (16));
    slider.setBounds (r);
}

// ---------------------------------------------------------------- PianoRoll
PianoRoll::PianoRoll() { setTitle ("Piano roll"); setWantsKeyboardFocus (false); }

void PianoRoll::setResult (std::shared_ptr<const hg::Result> r, const StringArray& trackNames, const String& summary)
{
    res = std::move (r); names = trackNames; setTitle (summary); setDescription (summary); repaint();
    if (auto* h = getAccessibilityHandler()) h->notifyAccessibilityEvent (AccessibilityEvent::titleChanged);
}
void PianoRoll::setPlayhead (double p, bool playing)
{
    const double np = playing ? p : -1;
    if (std::abs (np - ph) > 1e-3) { ph = np; repaint(); }
}
std::array<Rectangle<int>, 4> PianoRoll::laneBounds() const
{
    std::array<Rectangle<int>, 4> out; auto r = getLocalBounds(); const int h = r.getHeight() / 4;
    for (int i = 0; i < 4; ++i) out[(size_t) i] = r.removeFromTop (i == 3 ? r.getHeight() : h).reduced (0, 2);
    return out;
}
void PianoRoll::paint (Graphics& g)
{
    const auto& P = palette();
    g.setColour (P.panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 10);
    if (! res) return;
    const double total = res->bars * 16 * 120.0;
    static const int order[4] = { hg::kChords, hg::kBass, hg::kDrums, hg::kMelody };
    static const int lanes[7] = { 46, 42, 64, 37, 39, 38, 36 };
    auto lb = laneBounds();
    for (int li = 0; li < 4; ++li)
    {
        const int tr = order[li]; auto r = lb[(size_t) li].toFloat(); const auto& notes = res->tracks[(size_t) tr];
        g.setColour (P.bg); g.fillRoundedRectangle (r, 6);
        const float x0 = r.getX() + 4, w = r.getWidth() - 8;
        auto X = [&] (double s) { const float f = (float) (hg::tickOf (s, res->P.swing) / total); return rtl ? x0 + w * (1 - f) : x0 + w * f; };
        for (int s = 0; s <= res->bars * 16; s += 4)
        {
            const float gx = rtl ? x0 + w * (1 - s / (res->bars * 16.0f)) : x0 + w * (s / (res->bars * 16.0f));
            g.setColour (s % 16 == 0 ? P.grid2 : P.grid); g.drawVerticalLine ((int) gx, r.getY(), r.getBottom());
        }
        int lo = 127, hi = 0, rows = 1;
        if (tr == hg::kDrums) rows = 7;
        else { for (auto& n : notes) { lo = jmin (lo, n.n); hi = jmax (hi, n.n); } if (notes.empty()) { lo = 48; hi = 72; } lo -= 1; hi += 1; if (hi - lo < 11) { int m = (11 - (hi - lo) + 1) / 2; lo -= m; hi += m; } rows = hi - lo + 1; }
        const float rh = r.getHeight() / rows;
        g.setColour (P.track[tr]);
        for (auto& n : notes)
        {
            int row;
            if (tr == hg::kDrums) { row = -1; for (int k = 0; k < 7; ++k) if (lanes[k] == n.n) row = k; if (row < 0) continue; }
            else row = hi - n.n;
            float a = X (n.s), b = X (n.s + n.l); if (a > b) std::swap (a, b);
            g.setOpacity (.5f + .5f * n.v / 127.0f);
            g.fillRect (a, r.getY() + row * rh + 1, jmax (2.0f, b - a - 1), jmax (2.0f, rh - 2));
        }
        g.setOpacity (1);
        g.setColour (P.mute); g.setFont (FontOptions (11.0f));
        if (li < names.size()) g.drawText (names[li], r.reduced (8, 2).toNearestInt(), rtl ? Justification::topLeft : Justification::topRight);
    }
    if (ph >= 0)
    {
        const float f = (float) (ph / (res->bars * 4.0)), x0 = 4, w = getWidth() - 8.0f, x = rtl ? x0 + w * (1 - f) : x0 + w * f;
        g.setColour (P.accent); g.fillRect (x - 1, 0.0f, 2.0f, (float) getHeight());
    }
}
std::unique_ptr<AccessibilityHandler> PianoRoll::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image);
}

// ---------------------------------------------------------------- ChordStrip
void ChordStrip::paint (Graphics& g)
{
    if (! res) return;
    const auto& P = palette(); const int n = (int) res->chords.size(); if (n == 0) return;
    const float gap = 6, w = (getWidth() - gap * (n - 1)) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        const int vi = rtl ? n - 1 - i : i;
        Rectangle<float> r (vi * (w + gap), 0, w, (float) getHeight());
        const bool on = i == cur;
        g.setColour (on ? P.accent.withAlpha (.18f) : P.panel2); g.fillRoundedRectangle (r, 8);
        g.setColour (on ? P.accent : P.line); g.drawRoundedRectangle (r.reduced (.5f), 8, 1);
        g.setColour (P.ink); g.setFont (font (jmin (18.0f, getHeight() * .42f), true));
        g.drawFittedText (res->chords[(size_t) i].name, r.reduced (6, 2).removeFromTop (r.getHeight() * .6f).toNearestInt(), Justification::centred, 1, .7f);
        g.setColour (P.mute); g.setFont (font (11.0f, false));
        g.drawFittedText (res->chords[(size_t) i].roman + "  " + bar + " " + String (i + 1), r.reduced (4, 2).removeFromBottom (r.getHeight() * .4f).toNearestInt(), Justification::centred, 1, .7f);
    }
}

// ---------------------------------------------------------------- DragTile
DragTile::DragTile() { setWantsKeyboardFocus (true); setMouseCursor (MouseCursor::DraggingHandCursor); }

void DragTile::paint (Graphics& g)
{
    const auto& P = palette(); auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (P.panel2); g.fillRoundedRectangle (r, 8);
    g.setColour (isMouseOver() || hasKeyboardFocus (false) ? P.accent : P.grid2);
    const float dash[] = { 4, 3 }; Path p; p.addRoundedRectangle (r, 8); PathStrokeType st (1.2f); Path d; st.createDashedStroke (d, p, dash, 2); g.fillPath (d);
    g.setColour (colour.isTransparent() ? P.ink : colour); g.setFont (font (13.0f, true));
    g.drawFittedText (label, getLocalBounds().reduced (8, 2), Justification::centred, 2, .8f);
    if (hasKeyboardFocus (false)) { g.setColour (P.focus); g.drawRoundedRectangle (r, 8, 2); }
}
void DragTile::mouseDown (const MouseEvent&) { dragging = false; }
void DragTile::mouseDrag (const MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 5 || ! makeFile) return;
    dragging = true;
    auto f = makeFile();
    if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}
void DragTile::mouseUp (const MouseEvent& e) { if (! dragging && e.mouseWasClicked()) saveAs(); dragging = false; }
bool DragTile::keyPressed (const KeyPress& k)
{
    if (k == KeyPress::returnKey || k == KeyPress::spaceKey) { saveAs(); return true; }
    return false;
}
void DragTile::saveAs()
{
    if (! makeFile) return;
    auto src = makeFile(); if (! src.existsAsFile()) return;
    chooser = std::make_unique<FileChooser> (label, File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (src.getFileName()), "*.mid");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
        [this, src] (const FileChooser& fc)
        {
            auto dst = fc.getResult(); if (dst == File()) return;
            if (dst.getFileExtension().isEmpty()) dst = dst.withFileExtension ("mid");
            if (src.copyFileTo (dst) && onSaved) onSaved (dst);
        });
}
std::unique_ptr<AccessibilityHandler> DragTile::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::button, AccessibilityActions().addAction (AccessibilityActionType::press, [this] { saveAs(); }));
}
}
