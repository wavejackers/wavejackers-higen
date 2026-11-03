#include "LayerList.h"

using namespace juce;
using namespace hg;

// Kategori simgesi: küçük, çizgisel
void drawCategoryIcon (Graphics& g, int cat, Rectangle<float> r, Colour col)
{
    g.setColour (col); Path p; const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight(), cy = y + h * 0.5f;
    if (cat == catBass) { for (int i = 0; i <= 24; ++i) { const float t = i / 24.0f; const float px = x + t * w, py = cy - std::sin (t * 6.2832f) * h * 0.30f; if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py); } }
    else if (cat == catLead) { p.startNewSubPath (x, cy + h * 0.28f); p.lineTo (x + w * 0.5f, cy - h * 0.28f); p.lineTo (x + w * 0.5f, cy + h * 0.28f); p.lineTo (x + w, cy - h * 0.28f); }
    else if (cat == catPad) { for (int k = 0; k < 3; ++k) for (int i = 0; i <= 20; ++i) { const float t = i / 20.0f; const float px = x + t * w, py = y + h * (0.28f + 0.22f * k) + std::sin (t * 6.2832f + k) * h * 0.07f; if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py); } }
    else { const float bw = w / 7.0f; for (int i = 0; i < 4; ++i) { const float bh = h * (i % 2 ? 0.35f : 0.8f); g.fillRoundedRectangle (x + i * bw * 2, cy - bh * 0.5f, bw, bh, 1.5f); } return; }
    g.strokePath (p, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
}

namespace hgui
{
void LayerList::paint (Graphics& g)
{
    const auto& P = lnf.pal(); const auto just = rtl ? Justification::centredRight : Justification::centredLeft;
    for (int k = 0; k < 4; ++k)
    {
        const auto r = pill (k).toFloat().reduced (0.5f); const int n = count (k); const bool sel = selCat == k, on = n > 0;
        const auto col = colour ? colour (k) : P.accent; const float rad = r.getHeight() * 0.5f;
        g.setColour (sel ? col.withAlpha (0.14f) : (hover == k ? P.panel2 : P.panel.withAlpha (0.6f))); g.fillRoundedRectangle (r, rad);
        g.setColour (sel ? col : (hover == k ? P.grid2 : P.line)); g.drawRoundedRectangle (r, rad, sel ? 1.5f : 1.0f);
        const float a = on ? 1.0f : 0.38f;
        // durum noktası (seçili = dolu)
        const auto dot = Rectangle<float> (10, 10).withCentre ({ rtl ? r.getRight() - 20 : r.getX() + 20, r.getCentreY() });
        g.setColour (col.withAlpha (a)); if (sel) g.fillEllipse (dot); else g.drawEllipse (dot.reduced (0.5f), 1.2f);
        drawCategoryIcon (g, k, Rectangle<float> (22, 16).withCentre ({ rtl ? r.getRight() - 52 : r.getX() + 52, r.getCentreY() }), col.withAlpha (a));
        auto txt = r.withTrimmedLeft (rtl ? 70.0f : 74.0f).withTrimmedRight (rtl ? 74.0f : 70.0f);
        g.setColour (P.ink.withAlpha (a)); g.setFont (lnf.font (13.5f, true));
        g.drawText (name ? name (k).toUpperCase() : String (k), txt.withTrimmedBottom (r.getHeight() * 0.42f).withTrimmedTop (4), just);
        g.setColour (P.mute.withAlpha (on ? 1.0f : 0.6f)); g.setFont (lnf.font (11.0f));
        const String sub = ! on ? noneText : sel && selRow >= 0 ? String (selRow + 1) + "/" + String (n) + "  " + ofWord : String (n) + " " + ofWord;
        g.drawText (sub, txt.withTrimmedTop (r.getHeight() * 0.52f).withTrimmedBottom (4), just);
        if (on)
        {
            const float conf = (*cands)[(size_t) k][(size_t) (sel && selRow >= 0 && selRow < n ? selRow : 0)].confidence;
            g.setColour (col); g.setFont (lnf.font (13.0f, true));
            g.drawText (String (roundToInt (conf * 100)) + "%", txt, rtl ? Justification::centredLeft : Justification::centredRight);
            // önizleme / durdur
            const auto pr = playRect (k); g.setColour (sel ? col : P.panel2); g.fillEllipse (pr); g.setColour (sel ? P.accentInk : P.ink);
            if (sel && playing) g.fillRect (Rectangle<float> (9, 9).withCentre (pr.getCentre()));
            else { Path tri; tri.addTriangle (pr.getCentreX() - 3.5f, pr.getCentreY() - 5.5f, pr.getCentreX() - 3.5f, pr.getCentreY() + 5.5f, pr.getCentreX() + 5.5f, pr.getCentreY()); g.fillPath (tri); }
        }
    }
}

void LayerList::mouseDown (const MouseEvent& e)
{
    const int k = hit (e.getPosition()); if (k < 0) return;
    if (k == selCat && playing && playRect (k).expanded (4).contains (e.position)) { if (onStop) onStop(); return; }
    if (onPick) onPick (k);
}
} // namespace hgui
