#include "VariationGrid.h"
#include <cmath>

using namespace juce;

namespace hgui
{
const std::array<int, 8>& VariationGrid::prints()
{
    static const std::array<int, 8> p { hg::cut, hg::reso, hg::envA, hg::envD, hg::envS, hg::noise, hg::fxDrive, hg::l1depth };
    return p;
}

static float norm01 (int i, float v)
{
    const auto& d = hg::spDef (i);
    if (d.hi <= d.lo) return 0.0f;
    if (d.centre > 0 && d.lo > 0) return jlimit (0.0f, 1.0f, (float) (std::log (v / d.lo) / std::log (d.hi / d.lo)));
    return jlimit (0.0f, 1.0f, (v - d.lo) / (d.hi - d.lo));
}

Rectangle<int> VariationGrid::tile (int i) const
{
    const int cols = 3, top = 64, gap = 10; const int tw = (getWidth() - gap * (cols + 1)) / cols, th = (getHeight() - top - gap * 3) / 2;
    int c = i % cols; if (rtl) c = cols - 1 - c;
    return { gap + c * (tw + gap), top + (i / cols) * (th + gap), tw, th };
}

void VariationGrid::paint (Graphics& g)
{
    const auto& P = lnf.pal(); auto r = getLocalBounds().toFloat();
    g.setColour (P.bg.withAlpha (0.96f)); g.fillRoundedRectangle (r, 16); g.setColour (P.line); g.drawRoundedRectangle (r.reduced (0.5f), 16, 1);
    const auto jl = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.ink); g.setFont (lnf.font (17.0f, true)); g.drawText (title, Rectangle<int> (16, 8, getWidth() - 120, 26).withX (rtl ? 104 : 16), jl);
    g.setColour (P.mute); g.setFont (lnf.font (11.5f)); g.drawText (hint, Rectangle<int> (16, 34, getWidth() - 32, 20), jl, true);
    { const auto cr = closeRect().toFloat(); g.setColour (P.panel2); g.fillRoundedRectangle (cr, 8); g.setColour (P.line); g.drawRoundedRectangle (cr.reduced (0.5f), 8, 1); g.setColour (P.ink); g.setFont (lnf.font (12.5f, true)); g.drawText (closeWord, cr, Justification::centred); }
    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto t = tile (i).toFloat(); const bool pl = playing == i, hv = hover == i;
        g.setColour (pl ? P.accent.withAlpha (0.14f) : (hv ? P.panel2 : P.panel)); g.fillRoundedRectangle (t, 12);
        g.setColour (pl ? P.accent : (hv ? P.grid2 : P.line)); g.drawRoundedRectangle (t.reduced (0.5f), 12, pl ? 1.6f : 1.0f);
        g.setColour (P.ink); g.setFont (lnf.font (13.0f, true));
        g.drawText (String (i + 1) + "   " + seedWord + " " + String ((int) items[(size_t) i].seed), t.reduced (12, 0).removeFromTop (30).translated (0, 4), jl);
        // parmak izi: her satır bir parametre, çubuk = aralık içindeki konum
        const auto& pr = prints(); auto area = t.reduced (12, 0).withTrimmedTop (36).withTrimmedBottom (40); const float rh = area.getHeight() / (float) pr.size();
        for (size_t k = 0; k < pr.size(); ++k)
        {
            auto row = Rectangle<float> (area.getX(), area.getY() + rh * k, area.getWidth(), rh);
            const float nv = norm01 (pr[k], items[(size_t) i].v[(size_t) pr[k]]);
            auto lab = row.withWidth (row.getWidth() * 0.42f); auto bar = row.withTrimmedLeft (row.getWidth() * 0.46f).withSizeKeepingCentre (row.getWidth() * 0.54f, 4.0f);
            if (rtl) { lab = lab.withX (row.getRight() - lab.getWidth()); bar = bar.withX (row.getX()); }
            g.setColour (P.mute); g.setFont (lnf.font (10.5f)); g.drawText (spName ? spName (pr[k]) : String (pr[k]), lab, jl, true);
            g.setColour (P.line); g.fillRoundedRectangle (bar, 2);
            g.setColour (pl ? P.accent : P.accent.withAlpha (0.7f)); g.fillRoundedRectangle (rtl ? bar.withLeft (bar.getRight() - jmax (2.0f, bar.getWidth() * nv)) : bar.withWidth (jmax (2.0f, bar.getWidth() * nv)), 2);
        }
        const auto kr = keepRect (i).toFloat(); g.setColour (pl ? P.accent : P.panel2); g.fillRoundedRectangle (kr, 8);
        g.setColour (pl ? P.accentInk : P.ink); g.setFont (lnf.font (12.5f, true)); g.drawText (keepWord, kr, Justification::centred);
    }
    if (hasKeyboardFocus (false) && playing >= 0) { g.setColour (P.focus); g.drawRoundedRectangle (tile (playing).toFloat().reduced (1), 12, 2); }
}

void VariationGrid::mouseDown (const MouseEvent& e)
{
    if (isShowing()) grabKeyboardFocus();
    if (closeRect().contains (e.getPosition())) { if (onClose) onClose(); return; }
    const int i = hit (e.getPosition()); if (i < 0) return;
    if (keepRect (i).contains (e.getPosition())) { if (onKeep) onKeep (i); return; }
    playing = i; repaint(); if (onAudition) onAudition (i);
}
void VariationGrid::mouseDoubleClick (const MouseEvent& e) { const int i = hit (e.getPosition()); if (i >= 0 && ! keepRect (i).contains (e.getPosition()) && onKeep) onKeep (i); }
void VariationGrid::mouseMove (const MouseEvent& e) { const int h = hit (e.getPosition()); if (h != hover) { hover = h; setMouseCursor (h >= 0 || closeRect().contains (e.getPosition()) ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); } }
bool VariationGrid::keyPressed (const KeyPress& k)
{
    const int n = (int) items.size(); if (n == 0) return false;
    if (k == KeyPress::escapeKey) { if (onClose) onClose(); return true; }
    if (k == KeyPress::returnKey && playing >= 0) { if (onKeep) onKeep (playing); return true; }
    int d = 0; if (k == KeyPress::rightKey) d = rtl ? -1 : 1; else if (k == KeyPress::leftKey) d = rtl ? 1 : -1; else if (k == KeyPress::downKey) d = 3; else if (k == KeyPress::upKey) d = -3;
    else if (k.getTextCharacter() >= '1' && k.getTextCharacter() <= '6') { playing = jmin (n - 1, (int) (k.getTextCharacter() - '1')); repaint(); if (onAudition) onAudition (playing); return true; }
    if (d == 0) return false;
    playing = jlimit (0, n - 1, (playing < 0 ? 0 : playing + d)); repaint(); if (onAudition) onAudition (playing); return true;
}
} // namespace hgui
