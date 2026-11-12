#include "Ui.h"
#include "BinaryData.h"

namespace hgui
{
using namespace juce;

Palette Palette::make (bool hc, Colour)
{
    // Göksel tema; yüksek kontrast modu ayrı ve tamamen opak kalır.
    Palette p;
    if (hc)
    {
        p.bg = Colours::black; p.panel = Colours::black; p.panel2 = Colour (0xff101010); p.ink = Colours::white; p.mute = Colour (0xffe6e6e6);
        p.line = Colours::white; p.accent = Colour (0xffffd400); p.accent2 = p.accent; p.accentInk = Colours::black; p.grid = Colour (0xff3a3a3a); p.grid2 = Colour (0xff8a8a8a); p.focus = Colour (0xff00e5ff);
        p.success = Colour (0xff5cf2cf); p.warning = Colour (0xffffd45c);
        p.track[0] = Colour (0xffa8b2ff); p.track[1] = Colour (0xff5cf2cf); p.track[2] = Colour (0xffff8fbb); p.track[3] = Colour (0xffffd45c);
    }
    else
    {
        // göksel tema (bilge ve melekler): gece laciverdi zemin, fildişi yazı, tek altın vurgu.
        // paneller hafif saydam: arka plandaki figürler panellerin ardından sezilir, metin kontrastı korunur (fildişi/panel > 12:1)
        p.bg = Colour (0xff0d0f1a); p.panel = Colour (0xe0161827); p.panel2 = Colour (0xf0202235); p.ink = Colour (0xfff6f0e1); p.mute = Colour (0xffb3aa95);
        p.line = Colour (0xff3d3628); p.accent = Colour (0xffe3bb62); p.accent2 = Colour (0xffb8913e); p.accentInk = Colour (0xff1a1408);
        p.grid = Colour (0xff15172a); p.grid2 = Colour (0xff3d3829); p.focus = Colour (0xffffe6a6);
        p.success = Colour (0xff8fd1b0); p.warning = Colour (0xfff0a35e);
        p.track[0] = Colour (0xffe9b4c8); p.track[1] = Colour (0xffe3bb62); p.track[2] = Colour (0xff8fb7ea); p.track[3] = Colour (0xffa9e0cf);
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
    setPalette (Palette::make (false, Colour()));
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
    g.setColour (palette.line); g.strokePath (track, PathStrokeType (3, PathStrokeType::curved, PathStrokeType::rounded));
    Path val; val.addCentredArc (cx, cy, r - 3, r - 3, 0, a0, ang, true);
    g.setColour (s.isEnabled() ? palette.accent : palette.grid2); g.strokePath (val, PathStrokeType (3, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (palette.ink);
    g.drawLine (cx + (r * .3f) * std::sin (ang), cy - (r * .3f) * std::cos (ang), cx + (r - 6) * std::sin (ang), cy - (r - 6) * std::cos (ang), 2.4f);
    if (s.hasKeyboardFocus (false)) drawFocusRing (g, Rectangle<float> (cx - r - 2, cy - r - 2, 2 * r + 4, 2 * r + 4), r + 2);
    // MIDI Learn: öğrenme halkası (kesikli vurgu) ve eşlenmiş CC rozeti
    const auto& pr = s.getProperties();
    if ((bool) pr.getWithDefault ("hgLearn", false))
    {
        Path ring; ring.addEllipse (cx - r - 1, cy - r - 1, 2 * r + 2, 2 * r + 2); Path dashed; const float dl[] = { 4.0f, 3.0f };
        PathStrokeType (1.6f).createDashedStroke (dashed, ring, dl, 2); g.setColour (palette.accent); g.fillPath (dashed);
    }
    const auto cc = pr.getWithDefault ("hgCC", var()).toString();
    if (cc.isNotEmpty())
    {
        const float bw = jmax (26.0f, 6.0f * (float) cc.length() + 8.0f), bh = 13.0f;
        auto badge = Rectangle<float> (bw, bh).withCentre ({ cx, cy - r + 1 });
        g.setColour (palette.bg); g.fillRoundedRectangle (badge.expanded (1.5f), bh * 0.5f + 1.5f);
        g.setColour (palette.accent.withAlpha (0.22f)); g.fillRoundedRectangle (badge, bh * 0.5f);
        g.setColour (palette.accent); g.drawRoundedRectangle (badge, bh * 0.5f, 1.0f);
        g.setFont (font (9.5f, true)); g.drawText (cc, badge, Justification::centred, false);
    }
}

float LookAndFeel::hover (const Component& cm, bool over)
{
    auto& e = hov[&cm]; e.comp = const_cast<Component*> (&cm); e.target = over ? 1.0f : 0.0f; return e.amt;
}

void LookAndFeel::stepHover (bool animate)
{
    for (auto it = hov.begin(); it != hov.end();)
    {
        auto& e = it->second;
        if (e.comp == nullptr) { it = hov.erase (it); continue; }
        const float n = animate ? e.amt + (e.target - e.amt) * 0.35f : e.target;
        const float v = std::abs (n - e.target) < 0.02f ? e.target : n;
        if (v != e.amt) { e.amt = v; e.comp->repaint(); }
        ++it;
    }
}

void LookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1);
    const bool on = b.getToggleState(); const float hv = hover (b, over && b.isEnabled());
    if (b.getComponentID() == "nav")
    {
        // gezinme sekmesi: arka plan yok; etkin sekme ince bir lavanta alt çizgi ile gösterilir
        if (on) { g.setColour (palette.accent); g.fillRect (r.getX() + 6, r.getBottom() - 2.0f, r.getWidth() - 12, 2.0f); }
        else if (hv > 0) { g.setColour (palette.line.interpolatedWith (palette.mute, hv)); g.fillRect (r.getX() + 6, r.getBottom() - 1.0f, (r.getWidth() - 12) * hv, 1.0f); }
        if (b.hasKeyboardFocus (false)) drawFocusRing (g, r.expanded (1), 6);
        return;
    }
    const bool primary = b.getComponentID() == "primary";
    Colour fill = on || primary ? palette.accent : palette.panel2.interpolatedWith (palette.panel2.brighter (0.12f), hv);
    if (down) fill = fill.brighter (0.12f);
    if (! b.isEnabled()) fill = palette.panel;
    g.setColour (fill); g.fillRoundedRectangle (down ? r.reduced (0.5f) : r, 8);
    g.setColour (on || primary ? palette.accent : palette.line.interpolatedWith (palette.accent2, hv)); g.drawRoundedRectangle (r, 8, 1);
    if (b.hasKeyboardFocus (false)) drawFocusRing (g, r.expanded (1), 9);
}

void LookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    if (b.getComponentID() == "nav")
    {
        g.setFont (font (juce::jmin (14.0f, b.getHeight() * .46f), true));
        g.setColour (b.getToggleState() ? palette.ink : palette.mute.interpolatedWith (palette.ink, hover (b, b.isMouseOver())));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 2), Justification::centred, 1, .8f);
        return;
    }
    const bool primary = b.getComponentID() == "primary";
    g.setFont (font (jmin (15.0f, b.getHeight() * .45f), true));
    g.setColour (b.getToggleState() || primary ? palette.accentInk : (b.isEnabled() ? palette.ink : palette.mute.withAlpha (0.6f)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (b.getWidth() < 44 ? 2 : 8, 2), Justification::centred, 2, .85f);   // dar düğmelerde (+, -, x) kenar payı küçük
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
    hg::NoteEv keep; const bool hadSel = editing && work && sel >= 0 && sel < (int) work->tracks[(size_t) lane].size();
    if (hadSel) keep = work->tracks[(size_t) lane][(size_t) sel];
    res = std::move (r); names = trackNames; setTitle (summary); setDescription (summary);
    if (editing && res)
    {
        work = std::make_shared<hg::Result> (*res); sel = -1;
        if (hadSel) { const auto& v = work->tracks[(size_t) lane]; for (int i = 0; i < (int) v.size(); ++i) if (v[(size_t) i].s == keep.s && v[(size_t) i].n == keep.n) { sel = i; break; } }
        fitRange();
    }
    repaint();
    if (auto* h = getAccessibilityHandler()) h->notifyAccessibilityEvent (AccessibilityEvent::titleChanged);
}

// ---------------------------------------------------------------- düzenleme
namespace
{
const int kDrumRows[7] = { 46, 42, 64, 37, 39, 38, 36 };
const char* const kDrumRowNames[7] = { "Open hat", "Hat", "Perc", "Rim", "Clap", "Snare", "Kick" };
int drumRowOf (int note) { for (int k = 0; k < 7; ++k) if (kDrumRows[k] == note) return k; return -1; }
}

void PianoRoll::setEditing (bool on, int track)
{
    editing = on; lane = jlimit (0, 3, track); sel = -1; drag = Drag::none; view0 = 0;
    work = (on && res) ? std::make_shared<hg::Result> (*res) : nullptr;
    setWantsKeyboardFocus (on); setMouseCursor (on ? MouseCursor::CrosshairCursor : MouseCursor::NormalCursor);
    if (on) fitRange();
    repaint();
}

void PianoRoll::fitRange()
{
    if (lane == hg::kDrums || ! work) return;
    int a = 127, b = 0; for (auto& n : work->tracks[(size_t) lane]) { a = jmin (a, n.n); b = jmax (b, n.n); }
    if (a > b) { a = 55; b = 72; }
    a -= 5; b += 5; if (b - a < 23) { const int m = (23 - (b - a) + 1) / 2; a -= m; b += m; }
    lo = jlimit (0, 104, a); hi = jlimit (lo + 23, 127, b);
}

Rectangle<float> PianoRoll::editArea() const
{
    auto r = getLocalBounds().toFloat().reduced (4);
    return rtl ? r.withTrimmedRight (54) : r.withTrimmedLeft (54);
}
int PianoRoll::rows() const noexcept { return lane == hg::kDrums ? 7 : hi - lo + 1; }
float PianoRoll::rowH() const { return editArea().getHeight() / (float) rows(); }
float PianoRoll::xOf (double st) const
{
    const auto a = editArea(); const float f = (float) ((st - view0) / viewLen());
    return rtl ? a.getRight() - a.getWidth() * f : a.getX() + a.getWidth() * f;
}
double PianoRoll::stepAt (float x) const
{
    const auto a = editArea(); float f = (x - a.getX()) / a.getWidth(); if (rtl) f = 1.0f - f;
    return jlimit (0.0, (double) (work ? work->bars * 16 : 16), view0 + (double) f * viewLen());
}
float PianoRoll::yOf (int note) const
{
    const int row = lane == hg::kDrums ? drumRowOf (note) : hi - note;
    return editArea().getY() + row * rowH();
}
int PianoRoll::noteAtY (float y) const
{
    const int row = jlimit (0, rows() - 1, (int) ((y - editArea().getY()) / rowH()));
    return lane == hg::kDrums ? kDrumRows[row] : hi - row;
}
int PianoRoll::hitNote (Point<float> p, bool& onEdge) const
{
    onEdge = false; if (! work) return -1;
    const auto& v = work->tracks[(size_t) lane];
    for (int i = (int) v.size() - 1; i >= 0; --i)
    {
        const auto& n = v[(size_t) i]; if (lane == hg::kDrums && drumRowOf (n.n) < 0) continue;
        if (n.s + n.l <= view0 || n.s >= view0 + viewLen()) continue;
        float x1 = xOf (n.s), x2 = xOf (n.s + n.l); if (x1 > x2) std::swap (x1, x2);
        const Rectangle<float> r (x1, yOf (n.n), jmax (6.0f, x2 - x1), rowH());
        if (r.contains (p)) { onEdge = rtl ? p.x < x1 + 6 : p.x > r.getRight() - 6; return i; }
    }
    return -1;
}

void PianoRoll::beginChange()
{
    if (res) undoStack.push_back (res);
    if (undoStack.size() > 64) undoStack.erase (undoStack.begin());
    changed = false;
}
void PianoRoll::commit()
{
    if (! changed) { if (! undoStack.empty()) undoStack.pop_back(); return; }
    redoStack.clear(); changed = false;
    if (onEdit) onEdit (std::make_shared<hg::Result> (*work));
    if (onHistoryChanged) onHistoryChanged();
}
bool PianoRoll::undo()
{
    if (undoStack.empty() || ! res) return false;
    redoStack.push_back (res); auto prev = undoStack.back(); undoStack.pop_back();
    if (onEdit) onEdit (std::make_shared<hg::Result> (*prev));
    if (onHistoryChanged) onHistoryChanged();
    return true;
}
bool PianoRoll::redo()
{
    if (redoStack.empty() || ! res) return false;
    undoStack.push_back (res); auto nx = redoStack.back(); redoStack.pop_back();
    if (onEdit) onEdit (std::make_shared<hg::Result> (*nx));
    if (onHistoryChanged) onHistoryChanged();
    return true;
}

void PianoRoll::mouseMove (const MouseEvent& e)
{
    if (! editing) return;
    bool edge = false; const int i = hitNote (e.position, edge);
    setMouseCursor (i >= 0 ? (edge ? MouseCursor::LeftRightResizeCursor : MouseCursor::DraggingHandCursor) : MouseCursor::CrosshairCursor);
}
void PianoRoll::mouseDown (const MouseEvent& e)
{
    if (! editing || ! work || ! editArea().contains (e.position)) return;
    grabKeyboardFocus();
    auto& v = work->tracks[(size_t) lane];
    bool edge = false; int i = hitNote (e.position, edge);
    if (e.mods.isPopupMenu())   // sağ tık: sil; basılı tutup sürüklersen yol boyunca siler
    {
        beginChange(); sel = -1; drag = Drag::erase; lastPos = e.position;
        if (i >= 0) { v.erase (v.begin() + i); changed = true; repaint(); }
        return;
    }
    beginChange();
    if (i < 0)   // boş yere tıkla: nota ekle (davulda 1, diğerlerinde 2 adım); basılı sürüklersen sağa/sola boyar
    {
        hg::NoteEv n; n.s = std::floor (stepAt (e.position.x)); n.l = lane == hg::kDrums ? 1.0 : 2.0; n.n = noteAtY (e.position.y); n.v = 100;
        if (n.s >= work->bars * 16) n.s = work->bars * 16 - 1;
        v.push_back (n); i = (int) v.size() - 1; changed = true; drag = Drag::paint; lastPaint = n.s;
    }
    else drag = edge ? Drag::resize : Drag::move;
    sel = i; orig = v[(size_t) i]; grabOff = stepAt (e.position.x) - orig.s;
    repaint();
}
void PianoRoll::mouseDrag (const MouseEvent& e)
{
    if (! editing || ! work || drag == Drag::none) return;
    auto& v = work->tracks[(size_t) lane];
    if (drag == Drag::erase)
    {
        const float dist = lastPos.getDistanceFrom (e.position); const int stepsN = jmax (1, (int) std::ceil (dist / 3.0f));
        for (int k = 1; k <= stepsN; ++k)
        {
            const auto p = lastPos + (e.position - lastPos) * ((float) k / (float) stepsN);
            bool edge = false; int i;
            while ((i = hitNote (p, edge)) >= 0) { v.erase (v.begin() + i); changed = true; }
        }
        lastPos = e.position; if (changed) repaint();
        return;
    }
    if (drag == Drag::paint)
    {
        if (sel < 0 || sel >= (int) v.size()) return;
        const double L = orig.l, total = work->bars * 16.0;
        const double s = orig.s + std::floor ((stepAt (e.position.x) - orig.s) / L) * L;
        if (s == lastPaint || s < 0 || s + 1 > total) return;
        lastPaint = s;
        const int note = noteAtY (e.position.y);
        for (auto& o : v) if (o.n == note && o.s < s + L && o.s + o.l > s) return;   // o hücrede zaten nota var
        hg::NoteEv n; n.s = s; n.l = std::min (L, total - s); n.n = note; n.v = orig.v;
        v.push_back (n); changed = true; repaint();
        return;
    }
    if (sel < 0) return;
    auto& n = work->tracks[(size_t) lane][(size_t) sel]; const double total = work->bars * 16.0; const auto before = n;
    if (drag == Drag::move)
    {
        n.s = jlimit (0.0, total - 1.0, std::round (stepAt (e.position.x) - grabOff));
        n.n = noteAtY (e.position.y);
        if (n.s + n.l > total) n.l = total - n.s;
    }
    else n.l = jlimit (1.0, total - n.s, std::ceil (stepAt (e.position.x) - n.s));
    if (n.s != before.s || n.n != before.n || n.l != before.l) { changed = true; repaint(); }
}
void PianoRoll::mouseUp (const MouseEvent&)
{
    if (! editing || drag == Drag::none) { if (editing && changed) commit(); drag = Drag::none; return; }
    drag = Drag::none; commit();
}
void PianoRoll::mouseDoubleClick (const MouseEvent& e)
{
    if (! editing || ! work) return;
    bool edge = false; const int i = hitNote (e.position, edge); if (i < 0) return;
    beginChange(); auto& v = work->tracks[(size_t) lane]; v.erase (v.begin() + i); sel = -1; changed = true; commit(); repaint();
}
void PianoRoll::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (! editing || ! work) { Component::mouseWheelMove (e, w); return; }
    bool edge = false; const int i = hitNote (e.position, edge);
    if (i < 0 || e.mods.isShiftDown())   // boş yerde tekerlek: ölçüler arasında kaydır
    {
        const double total = work->bars * 16.0, d = (w.deltaY + w.deltaX) > 0 ? -16.0 : 16.0;
        view0 = jlimit (0.0, jmax (0.0, total - viewLen()), view0 + d); repaint(); return;
    }
    beginChange(); auto& n = work->tracks[(size_t) lane][(size_t) i];
    const int nv = jlimit (1, 127, n.v + (w.deltaY > 0 ? 8 : -8)); if (nv != n.v) { n.v = nv; changed = true; } sel = i;
    commit(); repaint();
}
bool PianoRoll::keyPressed (const KeyPress& k)
{
    if (! editing || ! work) return false;
    const auto mods = k.getModifiers(); const int code = k.getKeyCode();
    if (mods.isCommandDown() && (code == 'z' || code == 'Z')) return mods.isShiftDown() ? redo() || true : undo() || true;
    if (mods.isCommandDown() && (code == 'y' || code == 'Y')) { redo(); return true; }
    auto& v = work->tracks[(size_t) lane];
    if (v.empty()) return false;
    if (sel < 0 || sel >= (int) v.size()) { if (code == KeyPress::leftKey || code == KeyPress::rightKey) { sel = 0; repaint(); return true; } return false; }
    if (mods.isAltDown() && (code == KeyPress::leftKey || code == KeyPress::rightKey)) { sel = jlimit (0, (int) v.size() - 1, sel + (code == KeyPress::rightKey ? 1 : -1)); repaint(); return true; }
    const double total = work->bars * 16.0;
    beginChange(); auto& n = v[(size_t) sel]; const auto before = n;
    if (code == KeyPress::deleteKey || code == KeyPress::backspaceKey) { v.erase (v.begin() + sel); sel = -1; changed = true; }
    else if (code == KeyPress::leftKey || code == KeyPress::rightKey) { const double d = (code == KeyPress::rightKey) != rtl ? 1.0 : -1.0; n.s = jlimit (0.0, total - n.l, n.s + d); }
    else if (code == KeyPress::upKey || code == KeyPress::downKey)
    {
        const int d = code == KeyPress::upKey ? 1 : -1;
        if (lane == hg::kDrums) { const int r = drumRowOf (n.n); if (r >= 0) n.n = kDrumRows[jlimit (0, 6, r - d)]; }
        else n.n = jlimit (0, 127, n.n + d * (mods.isShiftDown() ? 12 : 1));
    }
    else { commit(); return false; }
    if (sel >= 0 && (n.s != before.s || n.n != before.n)) changed = true;
    if (sel >= 0 && sel < (int) v.size()) { const double s = v[(size_t) sel].s; if (s < view0 || s >= view0 + viewLen()) view0 = jlimit (0.0, jmax (0.0, work->bars * 16.0 - viewLen()), std::floor (s / 16.0) * 16.0); }
    commit(); repaint(); return true;
}

void PianoRoll::paintEditor (Graphics& g)
{
    const auto& P = palette(); const auto a = editArea(); const float rh = rowH();
    g.setColour (P.bg); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8);
    const auto gut = rtl ? Rectangle<float> (a.getRight() + 2, a.getY(), 50, a.getHeight()) : Rectangle<float> (a.getX() - 52, a.getY(), 50, a.getHeight());
    g.setFont (FontOptions (jlimit (8.0f, 11.0f, rh * 0.8f)));
    for (int r = 0; r < rows(); ++r)
    {
        const float y = a.getY() + r * rh; const int note = lane == hg::kDrums ? kDrumRows[r] : hi - r;
        const bool black = lane != hg::kDrums && (note % 12 == 1 || note % 12 == 3 || note % 12 == 6 || note % 12 == 8 || note % 12 == 10);
        g.setColour (black ? P.grid : P.panel); g.fillRect (a.getX(), y, a.getWidth(), rh);
        g.setColour (P.grid); g.drawHorizontalLine ((int) (y + rh), a.getX(), a.getRight());
        const bool label = lane == hg::kDrums || note % 12 == 0 || rh >= 9.0f;
        if (label)
        {
            g.setColour (note % 12 == 0 || lane == hg::kDrums ? P.ink : P.mute);
            const String t = lane == hg::kDrums ? String (kDrumRowNames[r]) : (noteName ? noteName (note) : String (note));
            g.drawText (t, gut.withY (y).withHeight (rh).reduced (3, 0).toNearestInt(), rtl ? Justification::centredLeft : Justification::centredRight);
        }
    }
    const int s0 = (int) view0, s1 = (int) (view0 + viewLen());
    for (int st = s0; st <= s1; ++st)
    {
        const float x = xOf (st); g.setColour (st % 16 == 0 ? P.grid2 : st % 4 == 0 ? P.line : P.grid);
        g.drawVerticalLine ((int) x, a.getY(), a.getBottom());
        if (st % 16 == 0 && st < s1) { g.setColour (P.mute); g.setFont (FontOptions (10.0f)); g.drawText (String (st / 16 + 1), Rectangle<float> (rtl ? x - 24 : x + 3, a.getY() + 1, 20, 12).toNearestInt(), rtl ? Justification::right : Justification::left); }
    }
    if (work->bars * 16 > s1 - s0)   // kaydırma ipucu: hangi ölçüler görünüyor
    {
        g.setColour (P.mute); g.setFont (FontOptions (10.0f));
        g.drawText (String (s0 / 16 + 1) + "-" + String (s1 / 16) + " / " + String (work->bars), gut.withHeight (14).withY (a.getBottom() - 14).toNearestInt(), Justification::centred);
    }
    g.saveState(); g.reduceClipRegion (a.toNearestInt());
    const auto col = P.track[lane]; const auto& v = work->tracks[(size_t) lane];
    for (int i = 0; i < (int) v.size(); ++i)
    {
        const auto& n = v[(size_t) i]; if (lane == hg::kDrums ? drumRowOf (n.n) < 0 : (n.n < lo || n.n > hi)) continue;
        if (n.s + n.l <= view0 || n.s >= view0 + viewLen()) continue;
        float x1 = xOf (n.s), x2 = xOf (n.s + n.l); if (x1 > x2) std::swap (x1, x2);
        const auto r = Rectangle<float> (x1 + 1, yOf (n.n) + 1, jmax (4.0f, x2 - x1 - 2), jmax (3.0f, rh - 2));
        g.setColour (col.withAlpha (0.45f + 0.55f * n.v / 127.0f)); g.fillRoundedRectangle (r, 2.5f);
        if (i == sel) { g.setColour (P.focus); g.drawRoundedRectangle (r.expanded (1), 3.0f, 2.0f); }
    }
    if (ph >= 0 && ph * 4.0 >= view0 && ph * 4.0 <= view0 + viewLen()) { const float x = xOf (ph * 4.0); g.setColour (P.accent); g.fillRect (x - 1, a.getY(), 2.0f, a.getHeight()); }
    g.restoreState();
    if (hasKeyboardFocus (false)) { g.setColour (P.focus); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1), 9, 1.5f); }
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
    if (editing && work) { paintEditor (g); return; }
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
