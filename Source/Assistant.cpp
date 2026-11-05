#include "Assistant.h"
#include <cmath>

using namespace juce;

namespace hgui
{
// ---------------------------------------------------------------- tek gözlü karakter
void AssistantOrb::step (Point<float> mouseInParent, bool animate)
{
    ++frame;
    const auto c = getBounds().toFloat().getCentre();
    auto d = mouseInParent - c; const float len = std::sqrt (d.x * d.x + d.y * d.y);
    lookTarget = len > 1.0f ? d / jmax (80.0f, len) : Point<float>();
    look += (lookTarget - look) * 0.25f;
    if (animate)
    {
        bob = std::sin ((float) frame * 0.06f) * 0.9f;
        if (--blinkWait <= 0) { blink = 1.0f; blinkWait = 100 + (int) (Random::getSystemRandom().nextFloat() * 140.0f); }
    }
    else bob = 0.0f;
    if (blink > 0.0f) blink = jmax (0.0f, blink - 0.2f);
    repaint();
}

void AssistantOrb::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    auto b = getLocalBounds().toFloat();
    const float R = jmin (b.getWidth(), b.getHeight()) * 0.5f - 5.0f, cx = b.getCentreX(), cy = b.getCentreY() + 2.0f + bob;
    // anten: konuşurken ucu parlar
    g.setColour (P.grid2); g.drawLine (cx, cy - R, cx + 2.5f, cy - R - 3.5f, 1.4f);
    g.setColour (talking ? P.success : P.accent.withAlpha (0.8f)); g.fillEllipse (cx + 0.5f, cy - R - 6.5f, 4.5f, 4.5f);
    // gövde: sol üstten ışık alan küre
    ColourGradient body (P.accent.brighter (0.75f), cx - R * 0.38f, cy - R * 0.48f, P.accent2.darker (1.1f), cx + R * 0.6f, cy + R * 0.9f, true);
    body.addColour (0.45, P.accent);
    g.setGradientFill (body); g.fillEllipse (cx - R, cy - R, 2 * R, 2 * R);
    // kenar ışığı (sağ alt) ve parlak nokta
    Path rim; rim.addCentredArc (cx, cy, R - 1.2f, R - 1.2f, 0, MathConstants<float>::pi * 0.55f, MathConstants<float>::pi * 1.05f, true);
    g.setColour (P.focus.withAlpha (0.55f)); g.strokePath (rim, PathStrokeType (1.3f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::white.withAlpha (0.55f)); g.fillEllipse (cx - R * 0.62f, cy - R * 0.68f, R * 0.32f, R * 0.2f);
    // göz yuvası, akı, iris ve göz bebeği
    const float ex = cx, ey = cy - R * 0.04f, sock = R * 0.6f;
    g.setColour (P.bg.withAlpha (0.92f)); g.fillEllipse (ex - sock, ey - sock * 0.9f, 2 * sock, 1.8f * sock);
    const float sw = R * 0.48f, sh = R * 0.44f * (1.0f - std::sin (blink * MathConstants<float>::pi * 0.5f) * 0.92f);
    ColourGradient sclera (Colours::white, ex - sw * 0.3f, ey - sh * 0.5f, Colour (0xffcfd2e6), ex + sw, ey + sh, true);
    g.setGradientFill (sclera); g.fillEllipse (ex - sw, ey - sh, 2 * sw, 2 * sh);
    if (sh > R * 0.08f)
    {
        const float ir = R * 0.25f, px = ex + look.x * R * 0.2f, py = ey + look.y * R * 0.14f;
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addEllipse (ex - sw, ey - sh, 2 * sw, 2 * sh); g.reduceClipRegion (clip);
        ColourGradient iris (P.success.brighter (0.4f), px - ir * 0.3f, py - ir * 0.3f, P.accent2.darker (0.6f), px + ir, py + ir, true);
        g.setGradientFill (iris); g.fillEllipse (px - ir, py - ir, 2 * ir, 2 * ir);
        g.setColour (Colour (0xff07060f)); g.fillEllipse (px - ir * 0.45f, py - ir * 0.45f, ir * 0.9f, ir * 0.9f);
        g.setColour (Colours::white.withAlpha (0.9f)); g.fillEllipse (px - ir * 0.55f, py - ir * 0.6f, ir * 0.42f, ir * 0.42f);
    }
    if (badge) { const auto d = Rectangle<float> (9, 9).withCentre ({ cx + R * 0.78f, cy - R * 0.72f }); g.setColour (P.bg); g.fillEllipse (d.expanded (1.5f)); g.setColour (P.warning); g.fillEllipse (d); }
    if (hasKeyboardFocus (false)) lnf.drawFocusRing (g, b, b.getHeight() * 0.5f);
}

std::unique_ptr<AccessibilityHandler> AssistantOrb::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::button, AccessibilityActions().addAction (AccessibilityActionType::press, [this] { if (onClick) onClick(); }));
}

// ---------------------------------------------------------------- öneri kartı
AssistantBubble::AssistantBubble (LookAndFeel& l) : lnf (l)
{
    for (auto* b : { &applyBtn, &nextBtn, &guideBtn, &closeBtn }) addAndMakeVisible (*b);
    applyBtn.setComponentID ("primary"); closeBtn.setButtonText (String (CharPointer_UTF8 ("\xc3\x97")));
}

void AssistantBubble::show (const String& t, const String& text, const String& counter, bool canApply, const String& applyText)
{
    heading = t; body = text; count = counter; applyBtn.setVisible (canApply); applyBtn.setButtonText (applyText); resized(); repaint();
}

int AssistantBubble::preferredHeight (int width) const
{
    auto h = [this] (const String& t, float size, float w) { AttributedString as; as.append (t, lnf.font (size), Colours::white); TextLayout tl; tl.createLayout (as, w); return (int) std::ceil (tl.getHeight()); };
    const float w = (float) width - 36;
    return 2 + 12 + 16 + 24 + 6 + h (body, 13.5f, w) + 10 + h (footer, 10.5f, w) + 6 + 38 + 14 + 4;
}

void AssistantBubble::resized()
{
    auto r = getLocalBounds().reduced (14);
    closeBtn.setBounds (rtl ? r.getX() : r.getRight() - 28, r.getY() + 6, 28, 26);
    auto row = r.removeFromBottom (30);
    const int w = (row.getWidth() - 16) / 3;
    Component* order[3] = { &applyBtn, &nextBtn, &guideBtn };
    for (int i = 0; i < 3; ++i) { const int k = rtl ? 2 - i : i; order[i]->setBounds (row.getX() + k * (w + 8), row.getY(), w, 30); }
}

void AssistantBubble::paint (Graphics& g)
{
    const auto& P = lnf.pal(); auto r = getLocalBounds().toFloat().reduced (2);
    g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (r.translated (0, 3), 16);
    g.setColour (P.panel2); g.fillRoundedRectangle (r, 16);
    g.setColour (P.accent.withAlpha (0.7f)); g.drawRoundedRectangle (r.reduced (0.5f), 16, 1.2f);
    auto t = r.reduced (16, 12);
    const auto jl = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.accent); g.setFont (lnf.font (12, true)); g.drawText (count, t.removeFromTop (16).withTrimmedRight (rtl ? 0 : 34).withTrimmedLeft (rtl ? 34 : 0), jl);
    g.setColour (P.ink); g.setFont (lnf.font (16, true)); g.drawText (heading, t.removeFromTop (24), jl, true);
    t.removeFromTop (6); t.removeFromBottom (38 + 6);
    auto draw = [&] (const String& s, float size, Colour c, Rectangle<float> area)
    {
        AttributedString as; as.append (s, lnf.font (size), c); as.setJustification (rtl ? Justification::topRight : Justification::topLeft);
        as.setReadingDirection (rtl ? AttributedString::rightToLeft : AttributedString::leftToRight);
        TextLayout tl; tl.createLayout (as, area.getWidth()); tl.draw (g, area); return tl.getHeight();
    };
    { AttributedString as; as.append (footer, lnf.font (10.5f), P.mute); TextLayout tl; tl.createLayout (as, t.getWidth()); const float fh = tl.getHeight();
      draw (footer, 10.5f, P.mute, t.removeFromBottom (fh)); }
    draw (body, 13.5f, P.ink, t);
}

// ---------------------------------------------------------------- kılavuz
GuideOverlay::GuideOverlay (LookAndFeel& l) : lnf (l)
{
    for (auto* b : { &backBtn, &nextBtn, &skipBtn }) addAndMakeVisible (*b);
    nextBtn.setComponentID ("primary");
    backBtn.onClick = [this] { go (idx - 1); };
    nextBtn.onClick = [this] { next(); };
    skipBtn.onClick = [this] { finish(); };
    setWantsKeyboardFocus (true);
}

void GuideOverlay::start (std::vector<Step> s)
{
    steps = std::move (s); setVisible (true); toFront (true); go (0); if (isShowing()) grabKeyboardFocus();
}

void GuideOverlay::go (int i)
{
    if (steps.empty()) return;
    idx = jlimit (0, (int) steps.size() - 1, i);
    if (steps[(size_t) idx].enter) steps[(size_t) idx].enter();
    toFront (true);
    hole = steps[(size_t) idx].target ? steps[(size_t) idx].target() : Rectangle<int>();
    backBtn.setButtonText (backText); skipBtn.setButtonText (skipText);
    nextBtn.setButtonText (idx + 1 >= (int) steps.size() ? doneText : nextText);
    backBtn.setEnabled (idx > 0);
    layoutCard(); repaint();
    setTitle (steps[(size_t) idx].title); setDescription (steps[(size_t) idx].body);
    if (auto* h = getAccessibilityHandler()) h->notifyAccessibilityEvent (AccessibilityEvent::titleChanged);
}

void GuideOverlay::resized() { layoutCard(); }
void GuideOverlay::finish() { if (! isVisible()) return; setVisible (false); if (onFinish) onFinish(); }
void GuideOverlay::next() { if (idx + 1 >= (int) steps.size()) finish(); else go (idx + 1); }

void GuideOverlay::layoutCard()
{
    if (steps.empty()) return;
    const int w = 440;
    const auto& st = steps[(size_t) idx];
    AttributedString as; as.append (st.body, lnf.font (14.5f), Colours::white); TextLayout tl; tl.createLayout (as, (float) w - 40);
    const int h = jlimit (170, 360, 112 + (int) std::ceil (tl.getHeight()) + 44);
    const auto area = getLocalBounds().reduced (16);
    Rectangle<int> c (w, h);
    if (hole.isEmpty()) c = c.withCentre (area.getCentre());
    else
    {
        const int cxp = jlimit (area.getX(), area.getRight() - w, hole.getCentreX() - w / 2);
        if (hole.getBottom() + 18 + h <= area.getBottom()) c.setPosition (cxp, hole.getBottom() + 18);
        else if (hole.getY() - 18 - h >= area.getY()) c.setPosition (cxp, hole.getY() - 18 - h);
        else
        {
            const int cy = jlimit (area.getY(), area.getBottom() - h, hole.getCentreY() - h / 2);
            c.setPosition (hole.getRight() + 18 + w <= area.getRight() ? hole.getRight() + 18 : jmax (area.getX(), hole.getX() - 18 - w), cy);
        }
    }
    card = c;
    auto row = card.reduced (20, 18).removeFromBottom (32);
    skipBtn.setBounds (rtl ? row.removeFromRight (110) : row.removeFromLeft (110));
    nextBtn.setBounds (rtl ? row.removeFromLeft (130) : row.removeFromRight (130));
    if (rtl) row.removeFromLeft (8); else row.removeFromRight (8);
    backBtn.setBounds (rtl ? row.removeFromLeft (110) : row.removeFromRight (110));
}

void GuideOverlay::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    Path dim; dim.addRectangle (getLocalBounds().toFloat());
    if (! hole.isEmpty()) dim.addRoundedRectangle (hole.toFloat().expanded (6), 12);
    dim.setUsingNonZeroWinding (false);
    g.setColour (Colours::black.withAlpha (0.66f)); g.fillPath (dim);
    if (! hole.isEmpty())
    {
        g.setColour (P.accent.withAlpha (0.25f)); g.drawRoundedRectangle (hole.toFloat().expanded (9), 14, 4.0f);
        g.setColour (P.accent); g.drawRoundedRectangle (hole.toFloat().expanded (6), 12, 2.0f);
    }
    if (steps.empty()) return;
    const auto& st = steps[(size_t) idx]; auto r = card.toFloat();
    g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (r.translated (0, 4), 18);
    g.setColour (P.panel2); g.fillRoundedRectangle (r, 18);
    g.setColour (P.accent); g.fillRoundedRectangle (r.withHeight (4).reduced (18, 0), 2);
    g.setColour (P.line); g.drawRoundedRectangle (r.reduced (0.5f), 18, 1.0f);
    auto t = r.reduced (20, 18);
    const auto jl = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.accent); g.setFont (lnf.font (12, true)); g.drawText (stepWord.replace ("{i}", String (idx + 1)).replace ("{n}", String ((int) steps.size())), t.removeFromTop (18), jl);
    g.setColour (P.ink); g.setFont (lnf.font (19, true)); g.drawText (st.title, t.removeFromTop (30), jl, true);
    t.removeFromBottom (44);
    AttributedString as; as.append (st.body, lnf.font (14.5f), P.ink.withAlpha (0.9f)); as.setJustification (rtl ? Justification::topRight : Justification::topLeft);
    as.setReadingDirection (rtl ? AttributedString::rightToLeft : AttributedString::leftToRight);
    TextLayout tl; tl.createLayout (as, t.getWidth()); tl.draw (g, t);
}

bool GuideOverlay::keyPressed (const KeyPress& k)
{
    if (k == KeyPress::escapeKey) { finish(); return true; }
    if (k == KeyPress::returnKey || k == (rtl ? KeyPress::leftKey : KeyPress::rightKey)) { next(); return true; }
    if (k == (rtl ? KeyPress::rightKey : KeyPress::leftKey)) { if (idx > 0) go (idx - 1); return true; }
    return true;   // kılavuz açıkken kısayollar arkaya geçmesin
}
}
