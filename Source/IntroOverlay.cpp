#include "IntroOverlay.h"
using namespace juce;

namespace
{
float ease (float x) { x = jlimit (0.0f, 1.0f, x); return x * x * (3.0f - 2.0f * x); }
}

IntroOverlay::IntroOverlay (hgui::LookAndFeel& l) : lnf (l)
{
    setWantsKeyboardFocus (true);
    setTitle ("Welcome to Wavejackers Hit Generation");
    setDescription ("Click or press any key to skip");
}

std::unique_ptr<AccessibilityHandler> IntroOverlay::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::dialogWindow);
}

void IntroOverlay::begin (bool m, Rectangle<int> eyeHome)
{
    motion = m; home = eyeHome; t = 0.0; on = true;
    setVisible (true); toFront (false); setAlpha (1.0f);
}

Rectangle<int> IntroOverlay::advance (double ms)
{
    if (! on) return home;
    t += ms;
    const double total = motion ? kTotalMs : 1600.0, reveal = motion ? kRevealMs : 600.0;
    // arayüz açılışı: son bölümde örtü saydamlaşır
    setAlpha (t < reveal ? 1.0f : 1.0f - ease ((float) ((t - reveal) / (total - reveal))));
    repaint();
    if (t >= total) { finish(); return home; }
    if (! motion) return home;
    // göz: merkezden doğar, sekiz çizerek arayüzü dolaşır, açılıştan hemen önce yerine (sağ üst) süzülür
    const auto W = (float) getWidth(), H = (float) getHeight();
    const Point<float> c (W * 0.5f, H * 0.52f), h ((float) home.getCentreX(), (float) home.getCentreY());
    const float u = (float) (t / 1000.0);
    auto lissa = [&] (float s) { return Point<float> (c.x + W * 0.36f * std::sin (s * 1.15f), c.y + H * 0.30f * std::sin (s * 2.3f)); };
    Point<float> p;
    if (u < 0.5f) p = c;                                           // doğuş
    else if (u < 3.7f) { const float k = ease ((u - 0.5f) / 0.5f); p = c + (lissa (u - 0.5f) - c) * k; }
    else { const float k = ease ((u - 3.7f) / 0.8f); p = lissa (3.2f) + (h - lissa (3.2f)) * k; }
    const auto next = u < 3.7f ? lissa (u - 0.45f) : h;           // gidiş yönüne bakar
    look = next + (next - p) * 3.0f;
    const float size = (float) home.getWidth() * (u < 3.7f ? 1.6f : 1.6f - 0.6f * ease ((u - 3.7f) / 0.8f));
    return Rectangle<float> (size, size).withCentre (p).toNearestInt();
}

void IntroOverlay::finish()
{
    if (! on) return;
    on = false; setVisible (false);
    if (onFinished) onFinished();
}

void IntroOverlay::paint (Graphics& g)
{
    const auto& P = lnf.pal(); const auto r = getLocalBounds().toFloat();
    g.fillAll (P.bg);
    const float u = (float) (t / 1000.0);
    // asa ışığı: merkezde büyüyen altın hale
    const float glow = ease (u / 1.6f);
    ColourGradient halo (P.accent.withAlpha (0.30f * glow), r.getCentreX(), r.getHeight() * 0.45f, P.accent.withAlpha (0.0f), r.getCentreX() + r.getWidth() * 0.42f, r.getHeight() * 0.45f, true);
    g.setGradientFill (halo); g.fillRect (r);
    // marka yazısı: önce WAVEJACKERS, ardından HIT GENERATION (seslendirmeyle aynı anda)
    const float a1 = ease ((u - 0.5f) / 0.8f), a2 = ease ((u - 1.4f) / 0.8f);
    auto line = [&] (const String& s, float y, float h, bool bold, Colour col, float a, float spread)
    {
        if (a <= 0.0f) return;
        g.setColour (col.withAlpha (a)); g.setFont (lnf.font (h, bold).withExtraKerningFactor (spread));
        g.drawText (s, Rectangle<float> (0.0f, y - (1.0f - a) * 8.0f, r.getWidth(), h * 1.4f), Justification::centred);
    };
    line ("WELCOME TO", r.getHeight() * 0.33f, 15.0f, false, P.mute, a1, 0.35f);
    line ("WAVEJACKERS", r.getHeight() * 0.38f, 44.0f, true, P.ink, a1, 0.18f);
    line ("HIT GENERATION", r.getHeight() * 0.38f + 64.0f, 26.0f, true, P.accent, a2, 0.42f);
    // ince altın çizgi: yazının altında soldan sağa açılır
    const float lw = r.getWidth() * 0.28f * ease ((u - 1.6f) / 1.2f);
    g.setColour (P.accent.withAlpha (0.7f)); g.fillRect (Rectangle<float> (lw * 2.0f, 1.5f).withCentre ({ r.getCentreX(), r.getHeight() * 0.38f + 108.0f }));
    g.setColour (P.mute.withAlpha (0.8f * a1)); g.setFont (lnf.font (12.0f));
    g.drawText (skipText, r.withTop (r.getBottom() - 40.0f), Justification::centred);
}
