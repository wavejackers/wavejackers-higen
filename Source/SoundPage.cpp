#include "SoundPage.h"
#include <cmath>
#include <complex>

using namespace juce;
using namespace hg;

namespace
{
const char* const kPartKeys[3] = { "chords", "bass", "melody" };
bool isToggle (int i) { return i == l1sync || i == l2sync; }
constexpr float kPi = MathConstants<float>::pi;
float deg (float d) { return d * kPi / 180.0f; }

// grafit yüzeyler (sayfaya özgü); vurgu renkleri temadan (iz rengi ve altın)
const String kDot = String (CharPointer_UTF8 ("  \xc2\xb7  "));
const Colour kBgTop (0xff2a2a2e), kBgBottom (0xff1c1c1f), kPill (0xff17171a), kPillLine (0x14ffffff), kTrack (0xff141416), kKnob (0xff2d2d32);

// Seçili sesin tek periyot dalga biçimi (osilatör karışımı, alt osilatör, kesim frekansına göre yumuşatma), -1..1
void waveCycle (const Data& data, const std::array<float, kNumSp>& x, int N, std::vector<float>& ys, bool filtered)
{
    const auto& W = data.wavetables();
    const int A = jlimit (0, 11, (int) x[wtA]), B = jlimit (0, 11, (int) x[wtB]);
    std::array<float, 48> h {};
    const double m = std::round (x[wtMorph] * 16) / 16, br = std::round (x[wtBright] * 16) / 16, cutH = 2 + br * 46;
    for (int n = 1; n <= 48; ++n) h[(size_t) n - 1] = (float) (((double) W[A][n - 1] * (1 - m) + (double) W[B][n - 1] * m) * (n <= cutH ? 1.0 : std::exp (-(n - cutH) / 3.0)));
    auto wtAt = [&] (double ph) { double s = 0; for (int n = 1; n <= 48; ++n) s += h[(size_t) n - 1] * std::sin (MathConstants<double>::twoPi * n * ph); return s; };
    double wtNorm = 0; for (int i = 0; i <= 64; ++i) wtNorm = jmax (wtNorm, std::abs (wtAt (i / 64.0))); if (wtNorm < 1e-9) wtNorm = 1;
    ys.assign ((size_t) N + 1, 0.0f); float peak = 1e-6f;
    const int ob[3] = { o1w, o2w, o3w };
    for (int i = 0; i <= N; ++i)
    {
        const double t = i / (double) N; double y = 0;
        for (int j = 0; j < 3; ++j)
        {
            const int wv = (int) x[(size_t) ob[j]]; if (wv == 0) continue;
            const double mul = std::pow (2.0, x[(size_t) ob[j] + 1] + x[(size_t) ob[j] + 2] / 12.0);
            double ph = t * mul; ph -= std::floor (ph);
            const double s = wv == 1 ? 1 - 2 * ph : wv == 2 ? (ph < .5 ? 1 : -1) : wv == 3 ? (ph < .5 ? 4 * ph - 1 : 3 - 4 * ph) : wv == 4 ? std::sin (MathConstants<double>::twoPi * ph) : wtAt (ph) / wtNorm;
            y += s * x[(size_t) ob[j] + 4];
        }
        y += std::sin (MathConstants<double>::pi * t) * x[sub] * .8;
        ys[(size_t) i] = (float) y;
    }
    if (filtered)   // kesim düşükse köşeler yuvarlanır (görsel ipucu, gerçek filtre değil)
    {
        const int passes = jlimit (0, 14, (int) std::lround (14 - std::log2 (jmax (60.0f, x[cut]) / 60.0f) * 1.6));
        for (int p = 0; p < passes; ++p) { auto c = ys; for (int i = 1; i < N; ++i) ys[(size_t) i] = (c[(size_t) i - 1] + 2 * c[(size_t) i] + c[(size_t) i + 1]) * .25f; }
    }
    for (auto y : ys) peak = jmax (peak, std::abs (y));
    for (auto& y : ys) y /= peak;
}

void pill (Graphics& g, Rectangle<float> r)
{
    DropShadow (Colour (0x66000000), 14, { 0, 4 }).drawForRectangle (g, r.toNearestInt());
    g.setColour (kPill); g.fillRoundedRectangle (r, r.getHeight() * .5f);
    g.setColour (kPillLine); g.drawRoundedRectangle (r.reduced (.5f), r.getHeight() * .5f, 1.0f);
}
}

// ---------------------------------------------------------------- yay kaydırıcı
SoundPage::ArcSlider::ArcSlider (SoundPage& o) : owner (o)
{
    setSliderStyle (RotaryHorizontalVerticalDrag); setTextBoxStyle (NoTextBox, false, 0, 0);
    setWantsKeyboardFocus (true); setRepaintsOnMouseActivity (true); setDoubleClickReturnValue (true, 0);
}

void SoundPage::ArcSlider::setGeometry (Point<float> centreInParent, float radius, float bandW, float angMin, float angMax)
{
    c = centreInParent; R = radius; band = bandW; a0 = angMin; a1 = angMax; repaint();
}

float SoundPage::ArcSlider::angleAt (Point<float> p) const
{
    const auto lc = local(); float a = std::atan2 (-(p.y - lc.y), p.x - lc.x);
    const float mid = (a0 + a1) * .5f;
    while (a - mid > kPi) a -= 2 * kPi;
    while (a - mid < -kPi) a += 2 * kPi;
    return a;
}

void SoundPage::ArcSlider::setFromAngle (float a)
{
    const double t = jlimit (0.0, 1.0, (double) ((a - a0) / (a1 - a0)));
    setValue (getMinimum() + t * (getMaximum() - getMinimum()), sendNotificationSync);
}

bool SoundPage::ArcSlider::hitTest (int x, int y)
{
    const auto lc = local(); const Point<float> p ((float) x, (float) y);
    const float d = p.getDistanceFrom (lc);
    if (d < R - band * .5f - 6 || d > R + band * .5f + 6) return false;
    const float a = angleAt (p), lo = jmin (a0, a1) - .06f, hi = jmax (a0, a1) + .06f;
    return a >= lo && a <= hi;
}

void SoundPage::ArcSlider::mouseDown (const MouseEvent& e)
{
    Slider::mouseDown (e);   // sürükleme başlangıcı (DAW otomasyon jesti)
    if (! e.mods.isPopupMenu()) setFromAngle (angleAt (e.position));
}

void SoundPage::ArcSlider::mouseDrag (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    setFromAngle (angleAt (e.position));
}

bool SoundPage::ArcSlider::keyPressed (const KeyPress& k)
{
    const double range = getMaximum() - getMinimum(), step = range * (k.getModifiers().isShiftDown() ? .01 : .05);
    const int kc = k.getKeyCode();
    if (kc == KeyPress::upKey || kc == KeyPress::rightKey) { setValue (getValue() + step, sendNotificationSync); return true; }
    if (kc == KeyPress::downKey || kc == KeyPress::leftKey) { setValue (getValue() - step, sendNotificationSync); return true; }
    if (kc == KeyPress::homeKey) { setValue (getMinimum(), sendNotificationSync); return true; }
    if (kc == KeyPress::endKey) { setValue (getMaximum(), sendNotificationSync); return true; }
    return Slider::keyPressed (k);
}

void SoundPage::ArcSlider::paint (Graphics& g)
{
    const auto lc = local(); const auto& P = owner.lnf.pal();
    auto jang = [] (float a) { return kPi * .5f - a; };   // matematik açısı -> JUCE (saat 12'den saat yönü)
    const bool en = isEnabled(); const float hov = owner.lnf.hover (*this, isMouseOverOrDragging() && en);
    // yuva
    Path track; track.addCentredArc (lc.x, lc.y, R, R, 0, jang (a0), jang (a1), true);
    g.setColour (Colours::black.withAlpha (.35f)); g.strokePath (track, PathStrokeType (band + 4, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (kTrack); g.strokePath (track, PathStrokeType (band, PathStrokeType::curved, PathStrokeType::rounded));
    // iç kılavuz noktaları
    const float rg = R - band * .5f - 9; const int dots = 22;
    for (int i = 0; i <= dots; ++i)
    {
        const float a = a0 + (a1 - a0) * i / (float) dots;
        g.setColour (Colours::white.withAlpha (.10f)); g.fillEllipse (Rectangle<float> (2.2f, 2.2f).withCentre ({ lc.x + rg * std::cos (a), lc.y - rg * std::sin (a) }));
    }
    const double t = (getValue() - getMinimum()) / jmax (1e-9, getMaximum() - getMinimum());
    const float av = a0 + (a1 - a0) * (float) t;
    const auto col = owner.tint();
    // değer izi (ince)
    Path trail; trail.addCentredArc (lc.x, lc.y, rg, rg, 0, jang (a0), jang (av), true);
    g.setColour (col.withAlpha (en ? .85f : .3f)); g.strokePath (trail, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    // tutamak
    const Point<float> hp (lc.x + R * std::cos (av), lc.y - R * std::sin (av));
    const float hr = band * .42f + hov * 1.5f;
    g.setColour (Colours::black.withAlpha (.45f)); g.fillEllipse (Rectangle<float> (hr * 2 + 3, hr * 2 + 3).withCentre (hp.translated (0, 1.5f)));
    g.setGradientFill (ColourGradient (Colour (0xff3a3a40), hp.x, hp.y - hr, Colour (0xff222226), hp.x, hp.y + hr, false));
    g.fillEllipse (Rectangle<float> (hr * 2, hr * 2).withCentre (hp));
    g.setColour (Colours::white.withAlpha (en ? .95f : .4f)); g.fillEllipse (Rectangle<float> (6, 6).withCentre (hp));
    if (hasKeyboardFocus (false)) { g.setColour (P.focus); g.drawEllipse (Rectangle<float> (hr * 2 + 6, hr * 2 + 6).withCentre (hp), 2.0f); }
    // etiket: yayın dışında, teğete göre döndürülmüş
    const float am = (a0 + a1) * .5f, rl = R + band * .5f + 15;
    const Point<float> lp (lc.x + rl * std::cos (am), lc.y - rl * std::sin (am));
    float rot = kPi * .5f - am;   // metin yönü: açı - 90°
    if (std::sin (am) < 0) rot += kPi;   // alttaki yaylarda baş aşağı olmasın
    g.saveState();
    g.addTransform (AffineTransform::rotation (rot, lp.x, lp.y));
    g.setColour (Colours::white.withAlpha (.52f + .3f * hov)); g.setFont (owner.lnf.font (12.5f, true));
    String txt = label.toUpperCase();
    if (isMouseOverOrDragging() || hasKeyboardFocus (false)) txt = label + "  " + getTextFromValue (getValue());   // üzerindeyken değer
    g.drawText (txt, Rectangle<float> (lp.x - 90, lp.y - 9, 180, 18), Justification::centred);
    g.restoreState();
}

// ---------------------------------------------------------------- küçük döner düğme
SoundPage::Dial::Dial (SoundPage& o) : owner (o)
{
    setSliderStyle (RotaryHorizontalVerticalDrag); setTextBoxStyle (NoTextBox, false, 0, 0);
    setRotaryParameters (deg (-135) + 2 * kPi, deg (135) + 2 * kPi, true);
    setWantsKeyboardFocus (true); setRepaintsOnMouseActivity (true);
}

void SoundPage::Dial::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (2); const float r = jmin (b.getWidth(), b.getHeight()) * .5f; const auto cc = b.getCentre();
    const bool en = isEnabled(); const float hov = owner.lnf.hover (*this, isMouseOverOrDragging() && en);
    g.setColour (Colours::black.withAlpha (.45f)); g.fillEllipse (Rectangle<float> (r * 2 + 2, r * 2 + 2).withCentre (cc.translated (0, 1.5f)));
    g.setGradientFill (ColourGradient (Colour (0xff3b3b41).brighter (.15f * hov), cc.x, cc.y - r, kKnob.darker (.4f), cc.x, cc.y + r, false));
    g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (cc));
    const double t = (getValue() - getMinimum()) / jmax (1e-9, getMaximum() - getMinimum());
    const float a = deg (-135) + deg (270) * (float) t;   // saat 12'den saat yönü
    g.setColour (Colours::white.withAlpha (en ? .95f : .35f));
    g.drawLine (cc.x + r * .3f * std::sin (a), cc.y - r * .3f * std::cos (a), cc.x + r * .78f * std::sin (a), cc.y - r * .78f * std::cos (a), 2.2f);
    if (hasKeyboardFocus (false)) { g.setColour (owner.lnf.pal().focus); g.drawEllipse (Rectangle<float> (r * 2 + 4, r * 2 + 4).withCentre (cc), 2.0f); }
}

// ---------------------------------------------------------------- simge düğmeleri
void SoundPage::IconButton::paintButton (Graphics& g, bool over, bool down)
{
    const auto& P = owner.lnf.pal(); auto b = getLocalBounds().toFloat();
    const bool en = isEnabled(); const float hov = owner.lnf.hover (*this, (over || down) && en);
    const auto ink = Colours::white.withAlpha (en ? .62f + .38f * hov : .3f);
    const auto cc = b.getCentre(); const auto tint = owner.tint();
    auto stroke = [&] (Path& p, float w = 1.8f) { g.strokePath (p, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded)); };
    const bool sel = getToggleState();
    if (kind <= 8 && (sel || hov > .01f)) { g.setColour (Colours::white.withAlpha (sel ? .12f : .06f * hov)); g.fillRoundedRectangle (b.reduced (2), b.getHeight() * .5f - 2); }
    g.setColour (sel ? tint : ink);
    Path p;
    switch (kind)
    {
        case 0: case 1:   // önceki / sonraki (RTL'de yer değişir, ok yönü de)
        {
            const bool left = (kind == 0) != owner.rtl;
            p.startNewSubPath (cc.x + (left ? 3.0f : -3.0f), cc.y - 6); p.lineTo (cc.x + (left ? -3.0f : 3.0f), cc.y); p.lineTo (cc.x + (left ? 3.0f : -3.0f), cc.y + 6); stroke (p, 2.0f); break;
        }
        case 2:   // zar: rastgele
        {
            auto r = Rectangle<float> (16, 16).withCentre (cc); g.drawRoundedRectangle (r, 3.5f, 1.6f);
            for (auto d : { Point<float> (-3.5f, -3.5f), Point<float> (3.5f, 3.5f), Point<float> (0, 0) }) g.fillEllipse (Rectangle<float> (3, 3).withCentre (cc + d));
            break;
        }
        case 3:   // geri dön
        {
            p.addCentredArc (cc.x, cc.y, 7, 7, 0, deg (-40), deg (250), true); stroke (p);
            Path h; const Point<float> e (cc.x + 7 * std::sin (deg (-40)), cc.y - 7 * std::cos (deg (-40)));
            h.startNewSubPath (e.translated (-1, -5)); h.lineTo (e); h.lineTo (e.translated (5, -1)); stroke (h); break;
        }
        case 4:   // makro görünümü: küre ve yaylar
        {
            g.drawEllipse (Rectangle<float> (10, 10).withCentre (cc), 1.6f);
            for (int q = 0; q < 4; ++q) { Path a; const float m = deg (45 + 90 * q); a.addCentredArc (cc.x, cc.y, 9, 9, 0, m - deg (22), m + deg (22), true); stroke (a, 1.6f); }
            break;
        }
        case 5:   // tüm parametreler: kaydırıcılar
            for (int q = 0; q < 3; ++q)
            {
                const float x = cc.x - 6 + q * 6; Path l; l.startNewSubPath (x, cc.y - 8); l.lineTo (x, cc.y + 8); stroke (l, 1.4f);
                g.fillEllipse (Rectangle<float> (5, 5).withCentre ({ x, cc.y + (q == 1 ? 4.0f : -3.0f) }));
            }
            break;
        case 6:   // tarayıcı: liste
            for (int q = 0; q < 4; ++q) { Path l; l.startNewSubPath (cc.x - 8, cc.y - 6 + q * 4); l.lineTo (cc.x + 8, cc.y - 6 + q * 4); stroke (l, 1.6f); }
            break;
        case 7: case 8:   // eksi / artı
            p.startNewSubPath (cc.x - 5, cc.y); p.lineTo (cc.x + 5, cc.y); if (kind == 8) { p.startNewSubPath (cc.x, cc.y - 5); p.lineTo (cc.x, cc.y + 5); } stroke (p, 1.8f); break;
        case 9:   // kategori çipi: renkli nokta + yazı + açılır ok
        {
            g.setColour (Colour (0xff26262a).brighter (.12f * hov)); g.fillRoundedRectangle (b.reduced (1), b.getHeight() * .5f - 1);
            const float dx = owner.rtl ? b.getRight() - 16 : b.getX() + 16;
            g.setColour (tint); g.fillEllipse (Rectangle<float> (7, 7).withCentre ({ dx, cc.y }));
            g.setColour (Colours::white.withAlpha (.9f)); g.setFont (owner.lnf.font (14.0f, true));
            auto tr = b.reduced (26, 0); g.drawText (getButtonText(), owner.rtl ? tr.withTrimmedLeft (8) : tr.withTrimmedRight (8), owner.rtl ? Justification::centredRight : Justification::centredLeft, true);
            const float ax = owner.rtl ? b.getX() + 16 : b.getRight() - 16;
            Path car; car.addTriangle (ax - 4, cc.y - 2, ax + 4, cc.y - 2, ax, cc.y + 3); g.setColour (Colours::white.withAlpha (.7f)); g.fillPath (car);
            break;
        }
        case 10:   // ses adı
            g.setColour (Colours::white.withAlpha (.92f + .08f * hov)); g.setFont (owner.lnf.font (16.0f, false));
            g.drawText (getButtonText(), b.reduced (6, 0), owner.rtl ? Justification::centredRight : Justification::centredLeft, true);
            break;
        case 11:   // iz bölümü
            if (sel) { g.setColour (tint.withAlpha (.20f)); g.fillRoundedRectangle (b.reduced (2), b.getHeight() * .5f - 2); g.setColour (tint.withAlpha (.6f)); g.drawRoundedRectangle (b.reduced (2.5f), b.getHeight() * .5f - 2.5f, 1.0f); }
            else if (hov > .01f) { g.setColour (Colours::white.withAlpha (.05f * hov)); g.fillRoundedRectangle (b.reduced (2), b.getHeight() * .5f - 2); }
            g.setColour (sel ? Colours::white : Colours::white.withAlpha (.6f + .3f * hov)); g.setFont (owner.lnf.font (14.0f, sel));
            g.drawText (getButtonText(), b, Justification::centred, true);
            break;
        case 12: case 13:   // ön dinleme / içe aktar: yazı + simge
        {
            g.setColour (Colours::white.withAlpha (.05f + .07f * hov + (sel ? .08f : 0))); g.fillRoundedRectangle (b.reduced (2), b.getHeight() * .5f - 2);
            const float ix = owner.rtl ? b.getX() + 22 : b.getRight() - 22;
            g.setColour (sel ? tint : Colours::white.withAlpha (.85f));
            if (kind == 12)
            {
                if (sel) g.fillRect (Rectangle<float> (10, 10).withCentre ({ ix, cc.y }));
                else { Path t; t.addTriangle (ix - 4, cc.y - 6, ix - 4, cc.y + 6, ix + 6, cc.y); g.fillPath (t); }
            }
            else
            {
                Path a; a.startNewSubPath (ix, cc.y + 6); a.lineTo (ix, cc.y - 5); a.startNewSubPath (ix - 4.5f, cc.y - 1); a.lineTo (ix, cc.y - 5.5f); a.lineTo (ix + 4.5f, cc.y - 1);
                a.startNewSubPath (ix - 7, cc.y + 7); a.lineTo (ix + 7, cc.y + 7); stroke (a, 1.6f);
            }
            g.setColour (Colours::white.withAlpha (.88f)); g.setFont (owner.lnf.font (14.0f, true));
            auto tr = b.reduced (16, 0); g.drawText (getButtonText(), owner.rtl ? tr.withTrimmedLeft (22) : tr.withTrimmedRight (22), owner.rtl ? Justification::centredRight : Justification::centredLeft, true);
            break;
        }
        default: break;
    }
    if (hasKeyboardFocus (false)) { g.setColour (P.focus); g.drawRoundedRectangle (b.reduced (1), b.getHeight() * .5f - 1, 2.0f); }
}

// ---------------------------------------------------------------- küre
std::unique_ptr<AccessibilityHandler> SoundPage::Orb::createAccessibilityHandler() { return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image); }

void SoundPage::Orb::paint (Graphics& g)
{
    auto& o = *owner; const auto b = getLocalBounds().toFloat(); const auto cc = b.getCentre();
    const float r = jmin (b.getWidth(), b.getHeight()) * .5f - 34;
    const auto tint = o.tint(), gold = o.lnf.pal().accent;
    // dış parıltı (çıkış seviyesiyle güçlenir)
    {
        const float a = .20f + .35f * jlimit (0.0f, 1.0f, level * 2.2f);
        ColourGradient gl (tint.withAlpha (a), cc.x, cc.y, tint.withAlpha (0.0f), cc.x + r * 1.55f, cc.y, true);
        gl.addColour (.55, gold.withAlpha (a * .35f));
        g.setGradientFill (gl); g.fillEllipse (Rectangle<float> (r * 3.1f, r * 3.1f).withCentre (cc));
    }
    // küre gövdesi
    const auto sphere = Rectangle<float> (r * 2, r * 2).withCentre (cc);
    g.setColour (Colours::black.withAlpha (.55f)); g.fillEllipse (sphere.expanded (3).translated (0, 4));
    {
        ColourGradient body (Colour (0xff404047), cc.x - r * .3f, cc.y - r * .45f, Colour (0xff141417), cc.x + r * .55f, cc.y + r * .9f, true);
        body.addColour (.6, Colour (0xff232327));
        g.setGradientFill (body); g.fillEllipse (sphere);
    }
    // dalga şeridi (kürenin içinde, katmanlı)
    {
        g.saveState();
        Path clip; clip.addEllipse (sphere.reduced (2)); g.reduceClipRegion (clip);
        std::vector<float> ys; const int N = 220;
        waveCycle (o.data, v, N, ys, true);
        const int layers = 13;
        for (int k = 0; k < layers; ++k)
        {
            const float f = k / (float) (layers - 1), mid = 1 - std::abs (f - .5f) * 2;
            Path w; const float amp = r * (.30f - .012f * k) * (.85f + .3f * jlimit (0.0f, 1.0f, level * 2));
            for (int i = 0; i <= N; ++i)
            {
                const float t = i / (float) N, x = cc.x - r + 2 * r * t;
                const float edge = std::sin (kPi * t);   // kenarlara doğru incelir (derinlik)
                const float wob = std::sin (phase * 1.3f + k * .45f + t * 6.0f) * 2.2f;
                const float y = cc.y + r * .06f + (k - layers * .5f) * 1.5f - ys[(size_t) ((i + (int) (phase * 20 + k * 3)) % (N + 1))] * amp * edge + wob * edge;
                if (i == 0) w.startNewSubPath (x, y); else w.lineTo (x, y);
            }
            g.setColour (tint.interpolatedWith (gold, f).withAlpha (.12f + .55f * mid));
            g.strokePath (w, PathStrokeType (k == layers / 2 ? 2.0f : 1.1f, PathStrokeType::curved));
        }
        // alt gölge ve üst ışık
        ColourGradient sh (Colours::transparentBlack, cc.x, cc.y + r * .2f, Colours::black.withAlpha (.5f), cc.x, cc.y + r, false);
        g.setGradientFill (sh); g.fillEllipse (sphere);
        g.restoreState();
    }
    g.setColour (Colours::white.withAlpha (.06f)); Path hl; hl.addCentredArc (cc.x, cc.y, r - 2, r - 2, 0, deg (-60), deg (60), true); g.strokePath (hl, PathStrokeType (1.5f));
    g.setColour (Colours::black.withAlpha (.6f)); g.drawEllipse (sphere, 1.2f);
}

// ---------------------------------------------------------------- sayfa
SoundPage::SoundPage (HitGenProcessor& p, hgui::LookAndFeel& l)
    : proc (p), lnf (l), data (p.data), keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setFocusContainerType (FocusContainerType::focusContainer);
    for (int i = 0; i < 3; ++i)
    {
        auto& b = *partBtns[(size_t) i]; addAndMakeVisible (b); b.setRadioGroupId (201); b.setClickingTogglesState (true);
        b.onClick = [this, i] { if (partBtns[(size_t) i]->getToggleState() && part != i) setPart (i); };
    }
    partBtns[0]->setToggleState (true, dontSendNotification);
    for (auto* b : { (Button*) &catBtn, (Button*) &nameBtn, (Button*) &prevBtn, (Button*) &nextBtn, (Button*) &randBtn, (Button*) &resetBtn }) addAndMakeVisible (*b);
    catBtn.onClick = [this] { showCatMenu(); };
    nameBtn.onClick = [this] { setView (view == vBrowse ? vMacro : vBrowse); };
    prevBtn.onClick = [this] { stepSound (-1); }; nextBtn.onClick = [this] { stepSound (1); };
    randBtn.onClick = [this] { proc.randomizeSound (part); };
    resetBtn.onClick = [this] { proc.resetSoundDesign (part); };
    for (auto& d : adsr) { d = std::make_unique<Dial> (*this); addAndMakeVisible (*d); }
    addAndMakeVisible (volDial);
    orb.owner = this; addAndMakeVisible (orb);
    for (auto& a : arcs) { a = std::make_unique<ArcSlider> (*this); addAndMakeVisible (*a); }
    for (auto* lb : { &descLbl, &useLbl, &hint }) { addAndMakeVisible (*lb); lb->setJustificationType (Justification::centred); lb->setMinimumHorizontalScale (.8f); }
    descLbl.setFont (lnf.font (15.0f)); useLbl.setFont (lnf.font (13.0f)); hint.setFont (lnf.font (12.0f));
    descLbl.setColour (Label::textColourId, Colours::white.withAlpha (.86f)); useLbl.setColour (Label::textColourId, Colours::white.withAlpha (.55f)); hint.setColour (Label::textColourId, Colours::white.withAlpha (.45f));
    for (auto* b : { &viewMacro, &viewEdit, &viewBrowse }) { addAndMakeVisible (*b); b->setRadioGroupId (202); b->setClickingTogglesState (true); }
    viewMacro.setToggleState (true, dontSendNotification);
    viewMacro.onClick = [this] { if (viewMacro.getToggleState()) setView (vMacro); };
    viewEdit.onClick = [this] { if (viewEdit.getToggleState()) setView (vEdit); };
    viewBrowse.onClick = [this] { if (viewBrowse.getToggleState()) setView (vBrowse); };
    addAndMakeVisible (octDown); addAndMakeVisible (octUp);
    octDown.onClick = [this] { setBaseNote (baseNote - 12); }; octUp.onClick = [this] { setBaseNote (baseNote + 12); };
    addAndMakeVisible (demoBtn); demoBtn.onClick = [this] { if (proc.demoPlaying()) proc.stopDemo(); else proc.playDemo (part); demoBtn.setToggleState (! demoBtn.getToggleState(), dontSendNotification); };
    addAndMakeVisible (importBtn); importBtn.onClick = [this] { doImport(); };
    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (24, 108); keyboard.setKeyWidth (40.0f); keyboard.setScrollButtonsVisible (false); keyboard.setVelocity (.8f, true);
    keyboard.setWantsKeyboardFocus (true); keyboard.setOctaveForMiddleC (4); keyboard.setLowestVisibleKey (baseNote);
    keyboard.setColour (MidiKeyboardComponent::whiteNoteColourId, Colour (0xfff4f4f2)); keyboard.setColour (MidiKeyboardComponent::blackNoteColourId, Colour (0xff2c2c30));
    keyboard.setColour (MidiKeyboardComponent::keySeparatorLineColourId, Colour (0xffcfcfcf)); keyboard.setColour (MidiKeyboardComponent::shadowColourId, Colours::transparentBlack);
    keyboard.setColour (MidiKeyboardComponent::textLabelColourId, Colour (0xff8a8a90));
    // tüm parametreler
    editPanel.owner = this; editView.setViewedComponent (&editPanel, false); editView.setScrollBarsShown (true, false); editView.setScrollBarThickness (8);
    addChildComponent (editView);
    for (int i = 0; i < kNumSp; ++i)
    {
        const auto& d = spDef (i);
        if (isToggle (i)) { auto t = std::make_unique<ToggleButton>(); editPanel.addAndMakeVisible (*t); toggles[i] = std::move (t); continue; }
        if (d.kind == spChoice) { auto c = std::make_unique<ComboBox>(); editPanel.addAndMakeVisible (*c); combos[i] = std::move (c); continue; }
        auto k = std::make_unique<hgui::Knob>(); editPanel.addAndMakeVisible (*k); knobs[i] = std::move (k);
    }
    scope.owner = this; editPanel.addAndMakeVisible (scope);
    // tarayıcı
    catList.setModel (&catModel); soundList.setModel (&soundModel);
    catList.setRowHeight (38); soundList.setRowHeight (66);
    for (auto* lb : { &catList, &soundList }) { addChildComponent (*lb); lb->setColour (ListBox::backgroundColourId, Colours::transparentBlack); lb->setColour (ListBox::outlineColourId, Colours::transparentBlack); }
    fillCombos(); rebuildList (false); bind();
    setView (vMacro);
}

SoundPage::~SoundPage() { proc.stopDemo(); sa.clear(); ca.clear(); ba.clear(); catList.setModel (nullptr); soundList.setModel (nullptr); }

void SoundPage::fillCombos()
{
    auto fill = [] (ComboBox& cb, const StringArray& items)
    {
        const int id = cb.getSelectedId(); cb.clear (dontSendNotification);
        for (int i = 0; i < items.size(); ++i) cb.addItem (items[i], i + 1);
        if (id > 0) cb.setSelectedId (id, dontSendNotification);
    };
    StringArray wts; for (int i = 0; i < 12; ++i) wts.add (T ("wt" + String (i)));
    for (auto& [i, cb] : combos)
    {
        const String k (spDef (i).key);
        if (k.startsWith ("o") && k.endsWith ("w")) fill (*cb, { T ("oscOff"), T ("wt0"), T ("wt1"), T ("wt2"), T ("sSine"), T ("wavetableW") });
        else if (k == "wtA" || k == "wtB") fill (*cb, wts);
        else if (k == "ftype") fill (*cb, { T ("fLP"), T ("fHP"), T ("fBP") });
        else if (k == "fslope") fill (*cb, { "12 dB", "24 dB" });
        else if (k.endsWith ("target")) fill (*cb, { T ("tCut"), T ("tPit"), T ("tAmp") });
        else if (k.endsWith ("shape")) fill (*cb, { T ("sSine"), T ("sTri"), T ("sSq"), T ("sSaw") });
    }
}

// Tarayıcı sırası: önce Fabrika 26 (kategori sırasıyla), sonra imza sesleri, sonra klasik banka
void SoundPage::rebuildList (bool keepFilter)
{
    const auto catOrder = data.categories();
    cats.clear(); cats.add (""); cats.add ("@wf");
    struct E { String id; int group, cat, idx; };
    std::vector<E> all;
    for (int s = 0; s < data.numSounds(); ++s)
    {
        const auto& v = data.sound (s); const auto id = v["id"].toString();
        if (! v["r"].toString().containsChar ((juce_wchar) role())) continue;
        const auto c = v["cat"].toString();
        all.push_back ({ id, data.isFactory (id) ? 0 : data.isSignature (id) ? 1 : 2, catOrder.indexOf (c), s });
    }
    for (auto& c : catOrder) for (auto& e : all) if (data.soundById (e.id)["cat"].toString() == c) { cats.add (c); break; }
    std::stable_sort (all.begin(), all.end(), [] (const E& a, const E& b) { return a.group != b.group ? a.group < b.group : a.cat != b.cat ? a.cat < b.cat : a.idx < b.idx; });
    if (! keepFilter || ! cats.contains (catFilter)) catFilter = "";
    list.clear();
    for (auto& e : all)
    {
        if (catFilter == "@wf" && e.group != 0) continue;
        if (catFilter.isNotEmpty() && catFilter != "@wf" && data.soundById (e.id)["cat"].toString() != catFilter) continue;
        list.add (e.id);
    }
    catList.updateContent(); soundList.updateContent();
    catList.selectRow (jmax (0, cats.indexOf (catFilter)), true, true);
    shownId = proc.soundId (trackOfPart());
    const int row = list.indexOf (shownId);
    if (row >= 0) { soundList.selectRow (row, true, true); soundList.scrollToEnsureRowIsOnscreen (row); } else soundList.deselectAllRows();
    refreshInfo();
}

void SoundPage::selectSound (const String& id)
{
    if (id.isEmpty()) return;
    if (id != proc.soundId (trackOfPart())) proc.setSound (trackOfPart(), id, true);
    shownId = id; refreshInfo();
    const int row = list.indexOf (id); if (row >= 0) { soundList.selectRow (row, true, true); soundList.scrollToEnsureRowIsOnscreen (row); }
}

void SoundPage::stepSound (int dir)
{
    if (list.isEmpty()) return;
    const int i = list.indexOf (proc.soundId (trackOfPart()));
    selectSound (list[(i < 0 ? 0 : i + dir + list.size()) % list.size()]);
    if (proc.demoPlaying()) proc.playDemo (part);   // ön dinleme açıkken yeni sesle yeniden başlar
}

void SoundPage::refreshInfo()
{
    const auto id = proc.soundId (trackOfPart());
    const auto& v = data.soundById (id);
    const auto cat = v["cat"].toString();
    catBtn.setButtonText (cat.isNotEmpty() ? T ("cat_" + cat) : T ("catAll"));
    nameBtn.setButtonText (data.soundName (id, lang) + (data.isFactory (id) ? "" : String()));
    nameBtn.setTitle (T ("baseSound") + ": " + data.soundName (id, lang)); nameBtn.setTooltip (T ("vBrowse"));
    auto desc = data.soundInfo (id, lang, 0), use = data.soundInfo (id, lang, 1);
    if (desc.isEmpty()) desc = T ("cat_" + cat) + (data.isSignature (id) ? kDot + T ("sigTag") : String());
    descLbl.setText (desc, dontSendNotification);
    useLbl.setText (use.isNotEmpty() ? T ("useWord") + ": " + use : String(), dontSendNotification);
    orb.setDescription (desc);
    repaint();
}

void SoundPage::bind()
{
    sa.clear(); ca.clear(); ba.clear();
    for (auto& [i, k] : knobs) { sa.push_back (std::make_unique<SA> (proc.apvts, spId (part, i), k->slider)); hgui::tagParam (k->slider, spId (part, i)); }
    const int env[4] = { envA, envD, envS, envR };
    for (int k = 0; k < 4; ++k) { sa.push_back (std::make_unique<SA> (proc.apvts, spId (part, env[k]), *adsr[(size_t) k])); hgui::tagParam (*adsr[(size_t) k], spId (part, env[k])); }
    sa.push_back (std::make_unique<SA> (proc.apvts, "vol_" + partId(), volDial)); hgui::tagParam (volDial, "vol_" + partId());
    const String macroIds[4] = { "cut_", "drv_", "mov_", "rev_" };
    for (int k = 0; k < 4; ++k) { sa.push_back (std::make_unique<SA> (proc.apvts, macroIds[k] + partId(), *arcs[(size_t) k])); hgui::tagParam (*arcs[(size_t) k], macroIds[k] + partId()); }
    if (onRebind) onRebind();
    for (auto& [i, c] : combos) ca.push_back (std::make_unique<CA> (proc.apvts, spId (part, i), *c));
    for (auto& [i, t] : toggles) ba.push_back (std::make_unique<BA> (proc.apvts, spId (part, i), *t));
}

void SoundPage::setPart (int p)
{
    if (proc.demoPlaying()) proc.stopDemo();
    part = jlimit (0, 2, p);
    partBtns[(size_t) part]->setToggleState (true, dontSendNotification);
    bind(); rebuildList (false); setLanguage (lang, rtl);
    if (isShowing()) proc.auditionPart.store (part);
    orb.repaint(); scope.repaint();
}

void SoundPage::setView (View v)
{
    view = v;
    (v == vMacro ? viewMacro : v == vEdit ? viewEdit : viewBrowse).setToggleState (true, dontSendNotification);
    const bool m = v == vMacro, e = v == vEdit, b = v == vBrowse;
    orb.setVisible (m); for (auto& a : arcs) a->setVisible (m);
    descLbl.setVisible (m); useLbl.setVisible (m); hint.setVisible (m);
    editView.setVisible (e);
    catList.setVisible (b); soundList.setVisible (b);
    if (b) { const int row = list.indexOf (proc.soundId (trackOfPart())); if (row >= 0) soundList.scrollToEnsureRowIsOnscreen (row); }
    resized(); repaint();
}

void SoundPage::visibilityChanged()
{
    if (! isVisible() && proc.demoPlaying()) proc.stopDemo();
}

String SoundPage::noteLabel (int n) const
{
    static const char* const nm[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return String (nm[((n % 12) + 12) % 12]) + String (n / 12 - 1);
}

void SoundPage::setBaseNote (int n)
{
    baseNote = jlimit (24, 84, n); keyboard.setLowestVisibleKey (baseNote);
    octDown.setEnabled (baseNote > 24); octUp.setEnabled (baseNote < 84);
    repaint();
}

void SoundPage::showCatMenu()
{
    PopupMenu m;
    for (int i = 0; i < cats.size(); ++i)
    {
        const auto& c = cats[i];
        m.addItem (i + 1, c.isEmpty() ? T ("catAll") : c == "@wf" ? T ("bankWf") : T ("cat_" + c), true, c == catFilter);
        if (i == 1) m.addSeparator();
    }
    SafePointer<SoundPage> safe (this);
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&catBtn), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        safe->catFilter = safe->cats[r - 1]; safe->rebuildList (true); safe->setView (vBrowse);
    });
}

void SoundPage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r;
    fillCombos();
    for (int i = 0; i < 3; ++i) { partBtns[(size_t) i]->setButtonText (T (kPartKeys[i])); partBtns[(size_t) i]->setTitle (T ("editTrack") + ": " + T (kPartKeys[i])); }
    const String trackName = T (kPartKeys[part]);
    catBtn.setTitle (T ("sdCat") + ": " + trackName); catBtn.setTooltip (T ("sdCat"));
    prevBtn.setTitle (T ("prevSnd")); prevBtn.setTooltip (T ("prevSnd")); nextBtn.setTitle (T ("nextSnd")); nextBtn.setTooltip (T ("nextSnd"));
    randBtn.setTitle (T ("rand") + ": " + trackName); randBtn.setTooltip (T ("rand"));
    resetBtn.setTitle (T ("reset") + ": " + trackName); resetBtn.setTooltip (T ("reset"));
    importBtn.setButtonText (T ("pImportShort")); importBtn.setTooltip (T ("pImportHint")); importBtn.setTitle (T ("pImport") + ": " + trackName);
    demoBtn.setButtonText (T ("demoPlay")); demoBtn.setTitle (T ("demoPlay") + ": " + trackName); demoBtn.setTooltip (T ("demoHint"));
    viewMacro.setTitle (T ("vMacro")); viewMacro.setTooltip (T ("vMacro"));
    viewEdit.setTitle (T ("vEdit")); viewEdit.setTooltip (T ("vEdit"));
    viewBrowse.setTitle (T ("vBrowse")); viewBrowse.setTooltip (T ("vBrowse"));
    octDown.setTitle (T ("octDown")); octDown.setTooltip (T ("octDown")); octUp.setTitle (T ("octUp")); octUp.setTooltip (T ("octUp"));
    const char* const env[4] = { "attack", "decay", "sustain", "release" };
    for (int k = 0; k < 4; ++k) { adsr[(size_t) k]->setTitle (T (env[k]) + " (" + trackName + ")"); adsr[(size_t) k]->setTooltip (T (env[k])); }
    volDial.setTitle (T ("volume") + " (" + trackName + ")"); volDial.setTooltip (T ("volume"));
    const char* const mk[4] = { "mBright", "mDrive", "mMove", "mSpace" };
    for (int k = 0; k < 4; ++k)
    {
        auto& a = *arcs[(size_t) k]; a.label = T (mk[k]);
        a.setTitle (T (mk[k]) + " (" + trackName + ")"); a.setHelpText (T (String (mk[k]) + "Hint")); a.setTooltip (T (String (mk[k]) + "Hint"));
    }
    hint.setText (T ("sdHint2"), dontSendNotification);
    for (auto* lb : { &descLbl, &useLbl, &hint }) lb->setJustificationType (Justification::centred);
    for (auto& [i, k] : knobs)
    {
        const auto& d = spDef (i); String title = T (d.label);
        const String key (d.key);
        if (key.startsWith ("o") && key.length() > 2 && CharacterFunctions::isDigit (key[1])) title = T ("oscN").replace ("{x}", String::charToString (key[1])) + " " + title;
        if (key.startsWith ("l") && CharacterFunctions::isDigit (key[1])) title = T ("lfo").replace ("{n}", String::charToString (key[1])) + " " + title;
        if (key == "fdrive") title = T ("filter") + " " + T ("drive");
        k->setText (T (d.label), trackName + ", " + title);
        k->slider.setTitle (title + " (" + trackName + ")");
        k->caption.setColour (Label::textColourId, lnf.pal().mute);
    }
    for (auto& [i, c] : combos)
    {
        const auto& d = spDef (i); String title = T (d.label); const String key (d.key);
        if (key.startsWith ("o")) title = T ("oscN").replace ("{x}", String::charToString (key[1])) + " " + title;
        if (key.startsWith ("l")) title = T ("lfo").replace ("{n}", String::charToString (key[1])) + " " + title;
        c->setTitle (title + " (" + trackName + ")"); c->setTooltip (title);
    }
    for (auto& [i, t] : toggles)
    {
        const String key (spDef (i).key);
        t->setButtonText (T ("sync")); t->setTitle (T ("lfo").replace ("{n}", String::charToString (key[1])) + " " + T ("sync") + " (" + trackName + ")");
        t->getProperties().set ("rtl", rtl);
    }
    keyboard.setTitle (T ("kbd") + ": " + trackName); keyboard.setHelpText (T ("kbdHint"));
    scope.setTitle (T ("scope") + ", " + T ("filter") + ", " + T ("ampEnv"));
    orb.setTitle (T ("orbTitle") + ": " + trackName);
    catList.setTitle (T ("sdCat")); soundList.setTitle (T ("vBrowse") + ": " + trackName);
    refreshInfo(); catList.repaint(); soundList.repaint();
    resized(); repaint();
}

void SoundPage::tick()
{
    const auto cur = proc.soundId (trackOfPart());
    if (cur != shownId) { shownId = cur; const int row = list.indexOf (cur); if (row >= 0) soundList.selectRow (row, true, true); else rebuildList (true); refreshInfo(); }
    std::array<float, kNumSp> v {};
    for (int i = 0; i < kNumSp; ++i) v[(size_t) i] = proc.apvts.getRawParameterValue (spId (part, i))->load();
    if (v != scope.v) { scope.v = v; scope.repaint(); }
    const float pk = proc.engine.peakOut.load (std::memory_order_relaxed);
    const float lv = shownPeak = jmax (pk, shownPeak * .85f);
    bool orbDirty = v != orb.v || std::abs (lv - orb.level) > .004f;
    if (proc.animations && lv > .003f) { orb.phase += .06f + lv * .25f; orbDirty = true; }
    if (orbDirty && orb.isVisible()) { orb.v = v; orb.level = lv; orb.repaint(); }
    if (demoBtn.getToggleState() != proc.demoPlaying()) demoBtn.setToggleState (proc.demoPlaying(), dontSendNotification);
    if (msgUntil != 0 && Time::currentTimeMillis() > msgUntil) { msgUntil = 0; hint.setText (T ("sdHint2"), dontSendNotification); }
    if (isShowing() && proc.auditionPart.load() != part) proc.auditionPart.store (part);
    if (isShowing()) repaint (mir ({ 1028, 12, 124, 44 }));   // seviye göstergesi
}

void SoundPage::layoutEdit()
{
    titles.clear(); panels.clear();
    auto P = [this] (Component& c, Rectangle<int> r) { c.setBounds (rtl ? r.withX (1148 - r.getRight()) : r); };
    auto K = [&] (int i, int x, int y, int w = 78, int h = 80) { if (knobs.count (i)) P (*knobs[i], { x, y, w, h }); };
    auto C = [&] (int i, int x, int y, int w, int h = 26) { if (combos.count (i)) P (*combos[i], { x, y, w, h }); };
    // eski düzen, panel kökü (8, 66) -> (0, 0)
    panels.push_back ({ 0, 0, 636, 290 }); titles.push_back ({ "osc", {}, { 12, 4, 300, 22 }, false });
    const int ob[3] = { o1w, o2w, o3w };
    for (int j = 0; j < 3; ++j)
    {
        const int y0 = 30 + j * 86;
        titles.push_back ({ "oscN", String (j + 1), { 12, y0 + 6, 132, 16 }, true });
        C (ob[j], 12, y0 + 26, 132);
        for (int k = 1; k < 5; ++k) K (ob[j] + k, 152 + (k - 1) * 80, y0);
    }
    K (uni, 480, 30); K (uniDet, 556, 30); K (sub, 480, 116); K (noise, 556, 116);
    panels.push_back ({ 644, 0, 504, 290 }); titles.push_back ({ "wt", {}, { 656, 4, 300, 22 }, false });
    titles.push_back ({ "tableA", {}, { 656, 28, 150, 14 }, true }); C (wtA, 656, 42, 150);
    titles.push_back ({ "tableB", {}, { 656, 72, 150, 14 }, true }); C (wtB, 656, 86, 150);
    K (wtMorph, 816, 30); K (wtBright, 896, 30);
    P (scope, { 656, 118, 482, 160 });
    panels.push_back ({ 0, 298, 420, 282 }); titles.push_back ({ "filter", {}, { 12, 302, 300, 22 }, false });
    C (ftype, 12, 330, 140); C (fslope, 160, 330, 100);
    K (cut, 12, 364, 98, 84); K (reso, 112, 364, 98, 84); K (fenv, 212, 364, 98, 84); K (fatk, 312, 364, 98, 84);
    K (fdec, 12, 452, 98, 84); K (ktrack, 112, 452, 98, 84); K (fdrive, 212, 452, 98, 84);
    panels.push_back ({ 428, 298, 220, 282 }); titles.push_back ({ "ampEnv", {}, { 440, 302, 200, 22 }, false });
    K (envA, 440, 330, 98, 84); K (envD, 540, 330, 98, 84); K (envS, 440, 420, 98, 84); K (envR, 540, 420, 98, 84);
    panels.push_back ({ 656, 298, 300, 282 });
    const int lb[2] = { l1rate, l2rate };
    for (int k = 0; k < 2; ++k)
    {
        const int y0 = 302 + k * 136;
        titles.push_back ({ "lfo", String (k + 1), { 668, y0, 200, 22 }, false });
        C (lb[k] + 2, 668, y0 + 26, 118); C (lb[k] + 3, 668, y0 + 58, 118);
        if (toggles.count (lb[k] + 4)) P (*toggles[lb[k] + 4], { 668, y0 + 90, 118, 24 });
        K (lb[k], 792, y0 + 24, 78, 84); K (lb[k] + 1, 872, y0 + 24, 78, 84);
    }
    panels.push_back ({ 964, 298, 184, 282 }); titles.push_back ({ "fx", {}, { 976, 302, 160, 22 }, false });
    K (fxDrive, 976, 326, 80, 82); K (fxChorus, 1060, 326, 80, 82); K (fxDelay, 976, 410, 80, 82); K (fxReverb, 1060, 410, 80, 82); K (level, 976, 494, 80, 82);
    editPanel.setSize (1148, 582);
}

void SoundPage::resized()
{
    auto B = [this] (Component& c, Rectangle<int> r) { c.setBounds (mir (r)); };
    // üst çubuk
    for (int i = 0; i < 3; ++i) B (*partBtns[(size_t) i], { 22 + i * 80, 18, 80, 32 });
    B (catBtn, { 286, 18, 140, 32 }); B (prevBtn, { 432, 18, 30, 32 }); B (nameBtn, { 462, 18, 250, 32 }); B (nextBtn, { 712, 18, 30, 32 });
    B (randBtn, { 750, 18, 34, 32 }); B (resetBtn, { 788, 18, 34, 32 });
    for (int k = 0; k < 4; ++k) B (*adsr[(size_t) k], { 850 + k * 41, 16, 36, 36 });
    B (volDial, { 1036, 16, 36, 36 });
    // küre ve yaylar (merkez sayfa ortasında; RTL'de açılar aynalanır)
    const Point<float> c (582.0f, 256.0f); const float R = 172, band = 26;
    orb.setBounds (Rectangle<int> (0, 0, 2 * (118 + 34), 2 * (118 + 34)).withCentre (c.toInt()));
    const float am[4][2] = { { 172, 112 }, { 188, 248 }, { 8, 68 }, { -8, -68 } };   // parlaklık, sürüş, hareket, alan
    const Rectangle<int> quad[4] = { { 360, 46, 210, 214 }, { 360, 252, 210, 222 }, { 594, 46, 210, 214 }, { 594, 252, 210, 222 } };
    for (int k = 0; k < 4; ++k)
    {
        float a0 = am[k][0], a1 = am[k][1];
        if (rtl) { a0 = 180 - a0; a1 = 180 - a1; }
        auto q = mir (quad[k]);
        arcs[(size_t) k]->setBounds (q); arcs[(size_t) k]->setGeometry (c, R, band, deg (a0), deg (a1));
    }
    B (descLbl, { 232, 452, 700, 22 }); B (useLbl, { 232, 474, 700, 20 });
    B (hint, { 232, 494, 700, 18 });
    // görünüm çubuğu ve alt satır
    B (viewMacro, { 516, 522, 44, 36 }); B (viewEdit, { 560, 522, 44, 36 }); B (viewBrowse, { 604, 522, 44, 36 });
    B (keyboard, { 262, 572, 640, 76 });
    B (octDown, { 168, 598, 32, 32 }); B (octUp, { 204, 598, 32, 32 });
    B (demoBtn, { 922, 596, 112, 36 }); B (importBtn, { 1036, 596, 112, 36 });
    keyboard.setLowestVisibleKey (baseNote);
    // görünümler
    layoutEdit();
    editView.setBounds (mir ({ 8, 80, 1148, 432 }));
    B (catList, { 16, 84, 210, 424 }); B (soundList, { 236, 84, 912, 424 });
}

void SoundPage::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (kBgTop, 0, 0, kBgBottom, 0, b.getHeight(), false)); g.fillRoundedRectangle (b, 18);
    if (view == vMacro)
    {
        ColourGradient gl (tint().withAlpha (.10f), 582, 256, tint().withAlpha (0.0f), 582 + 420, 256, true);
        g.setGradientFill (gl); g.fillRoundedRectangle (b, 18);
    }
    g.setColour (Colours::white.withAlpha (.05f)); g.drawRoundedRectangle (b.reduced (.5f), 18, 1);
    // üst çubuk hapları
    pill (g, mir ({ 16, 12, 252, 44 }).toFloat()); pill (g, mir ({ 280, 12, 548, 44 }).toFloat());
    pill (g, mir ({ 840, 12, 176, 44 }).toFloat()); pill (g, mir ({ 1028, 12, 120, 44 }).toFloat());
    g.setColour (Colours::white.withAlpha (.45f)); g.setFont (lnf.font (11.5f, true));
    const char* const ad[4] = { "A", "D", "S", "R" };
    for (int k = 0; k < 4; ++k) g.drawText (ad[k], mir ({ 850 + k * 41, 60, 36, 14 }), Justification::centred);
    g.drawText (T ("volume"), mir ({ 1028, 60, 120, 14 }), Justification::centred);
    {   // seviye göstergesi
        const auto m = mir ({ 1082, 31, 56, 6 }).toFloat();
        g.setColour (Colour (0xff0e0e10)); g.fillRoundedRectangle (m, 3);
        const float lv = jlimit (0.0f, 1.0f, shownPeak / .891f), w = m.getWidth() * lv;
        g.setColour (tint().withAlpha (.9f)); g.fillRoundedRectangle (rtl ? m.withLeft (m.getRight() - w) : m.withWidth (w), 3);
        g.setColour (lnf.pal().accent); g.fillRect (rtl ? m.getX() + m.getWidth() * .1f : m.getRight() - m.getWidth() * .1f, m.getY() - 2, 1.5f, m.getHeight() + 4);
    }
    // alt satır hapları
    pill (g, mir ({ 510, 518, 144, 44 }).toFloat());
    pill (g, mir ({ 16, 592, 228, 44 }).toFloat()); pill (g, mir ({ 916, 592, 236, 44 }).toFloat());
    g.setColour (Colours::white.withAlpha (.75f)); g.setFont (lnf.font (14.0f));
    g.drawText (T ("baseNote") + ":  " + noteLabel (baseNote), mir ({ 30, 592, 136, 44 }), rtl ? Justification::centredRight : Justification::centredLeft, true);
    if (view == vBrowse)
    {
        g.setColour (Colour (0x55000000)); g.fillRoundedRectangle (mir ({ 12, 80, 218, 432 }).toFloat(), 14); g.fillRoundedRectangle (mir ({ 232, 80, 920, 432 }).toFloat(), 14);
    }
}

void SoundPage::EditPanel::paint (Graphics& g)
{
    auto& o = *owner; const auto& P = o.lnf.pal();
    auto mr = [&] (Rectangle<int> r) { return o.rtl ? r.withX (getWidth() - r.getRight()) : r; };
    for (auto r : o.panels) { g.setColour (Colour (0xff1a1a1d)); g.fillRoundedRectangle (mr (r).toFloat(), 14); g.setColour (Colours::white.withAlpha (.05f)); g.drawRoundedRectangle (mr (r).toFloat().reduced (.5f), 14, 1); }
    for (auto& t : o.titles)
    {
        String s = o.T (t.key); if (t.arg.isNotEmpty()) s = s.replace ("{x}", t.arg).replace ("{n}", t.arg);
        g.setColour (t.small ? P.mute : P.ink); g.setFont (o.lnf.font (t.small ? 12.0f : 15.0f, ! t.small));
        g.drawText (s, mr (t.r), o.rtl ? Justification::centredRight : Justification::centredLeft);
    }
}

// ---------------------------------------------------------------- tarayıcı
int SoundPage::CatModel::getNumRows() { return o.cats.size(); }
String SoundPage::CatModel::getNameForRow (int row)
{
    const auto c = o.cats[row]; return c.isEmpty() ? o.T ("catAll") : c == "@wf" ? o.T ("bankWf") : o.T ("cat_" + c);
}
void SoundPage::CatModel::paintListBoxItem (int row, Graphics& g, int w, int h, bool selected)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (6, 3);
    if (selected) { g.setColour (o.tint().withAlpha (.18f)); g.fillRoundedRectangle (r, 10); }
    g.setColour (Colours::white.withAlpha (selected ? .95f : .7f)); g.setFont (o.lnf.font (14.0f, selected));
    g.drawText (getNameForRow (row), r.reduced (12, 0), o.rtl ? Justification::centredRight : Justification::centredLeft, true);
}
void SoundPage::CatModel::selectedRowsChanged (int row)
{
    if (row < 0 || row >= o.cats.size() || o.cats[row] == o.catFilter) return;
    o.catFilter = o.cats[row];
    SafePointer<SoundPage> safe (&o);
    MessageManager::callAsync ([safe] { if (safe != nullptr) safe->rebuildList (true); });   // liste kutusunun kendi bildirimi içinde içerik değiştirilmez
}
int SoundPage::SoundModel::getNumRows() { return o.list.size(); }
String SoundPage::SoundModel::getNameForRow (int row)
{
    const auto id = o.list[row]; const auto& v = o.data.soundById (id);
    return o.data.soundName (id, o.lang) + ", " + o.T ("cat_" + v["cat"].toString()) + ". " + o.data.soundInfo (id, o.lang, 0);
}
void SoundPage::SoundModel::paintListBoxItem (int row, Graphics& g, int w, int h, bool selected)
{
    const auto id = o.list[row]; const auto& v = o.data.soundById (id);
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (8, 3);
    if (selected) { g.setColour (o.tint().withAlpha (.14f)); g.fillRoundedRectangle (r, 12); g.setColour (o.tint().withAlpha (.5f)); g.drawRoundedRectangle (r.reduced (.5f), 12, 1); }
    const auto just = o.rtl ? Justification::centredRight : Justification::centredLeft;
    auto tr = r.reduced (16, 6);
    const auto name = o.data.soundName (id, o.lang);
    g.setColour (Colours::white.withAlpha (.95f)); g.setFont (o.lnf.font (15.0f, true));
    g.drawText (name, tr.removeFromTop (20), just, true);
    // kategori çipi (sağ üst)
    {
        const auto cat = o.T ("cat_" + v["cat"].toString()) + (o.data.isFactory (id) ? kDot + o.T ("bankWf") : o.data.isSignature (id) ? kDot + o.T ("sigTag") : String());
        g.setFont (o.lnf.font (11.5f, true)); g.setColour (Colours::white.withAlpha (.45f));
        g.drawText (cat, Rectangle<float> (r.getX() + 16, r.getY() + 6, r.getWidth() - 32, 20), o.rtl ? Justification::centredLeft : Justification::centredRight, true);
    }
    const auto desc = o.data.soundInfo (id, o.lang, 0), use = o.data.soundInfo (id, o.lang, 1);
    g.setFont (o.lnf.font (13.0f)); g.setColour (Colours::white.withAlpha (.72f));
    g.drawText (desc.isNotEmpty() ? desc : o.T ("classicSound"), tr.removeFromTop (18), just, true);
    if (use.isNotEmpty()) { g.setFont (o.lnf.font (12.0f)); g.setColour (Colours::white.withAlpha (.48f)); g.drawText (o.T ("useWord") + ": " + use, tr.removeFromTop (16), just, true); }
}
void SoundPage::SoundModel::listBoxItemClicked (int row, const MouseEvent&) { o.selectSound (o.list[row]); }
void SoundPage::SoundModel::listBoxItemDoubleClicked (int row, const MouseEvent&) { o.selectSound (o.list[row]); o.proc.playDemo (o.part); }
void SoundPage::SoundModel::returnKeyPressed (int row) { o.selectSound (o.list[row]); o.proc.playDemo (o.part); }

// ---------------------------------------------------------------- ayrıntılı önizleme: dalga, filtre eğrisi, zarf
std::unique_ptr<AccessibilityHandler> SoundPage::Scope::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image);
}

void SoundPage::Scope::paint (Graphics& g)
{
    auto& o = *owner; const auto& P = o.lnf.pal();
    const auto col = o.tint();
    auto area = getLocalBounds().toFloat();
    g.setColour (Colour (0xff111113)); g.fillRoundedRectangle (area, 8);
    const float w3 = area.getWidth() / 3;
    std::array<Rectangle<float>, 3> boxes;
    for (int i = 0; i < 3; ++i) boxes[(size_t) i] = Rectangle<float> (area.getX() + i * w3, area.getY(), w3, area.getHeight()).reduced (8, 8);
    if (o.rtl) std::swap (boxes[0], boxes[2]);
    const auto& x = v;
    {
        auto b = boxes[0]; Path path; const int N = 160; std::vector<float> ys;
        waveCycle (o.data, x, N, ys, false);
        for (int i = 0; i <= N; ++i)
        {
            const float px = b.getX() + b.getWidth() * i / N, py = b.getCentreY() - ys[(size_t) i] * b.getHeight() * .45f;
            if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (P.grid2.withAlpha (.5f)); g.drawHorizontalLine ((int) b.getCentreY(), b.getX(), b.getRight());
        g.setColour (col); g.strokePath (path, PathStrokeType (1.8f));
    }
    {
        auto b = boxes[1]; Path path;
        const int ft = (int) x[ftype]; const bool s24 = x[fslope] > .5f;
        const double fc = jlimit (30.0, 18000.0, (double) x[cut]);
        double Q = ft == 2 ? jmax (.3, (double) x[reso]) : std::pow (10.0, (s24 ? x[reso] * .7 : x[reso]) / 20.0);
        Q = jmax (.1, Q);
        const int N = 120;
        for (int i = 0; i <= N; ++i)
        {
            const double f = 20 * std::pow (1000.0, i / (double) N), wr = f / fc;
            const std::complex<double> s (0, wr), den = s * s + s / Q + 1.0;
            std::complex<double> H = ft == 0 ? 1.0 / den : ft == 1 ? (s * s) / den : (s / Q) / den;
            double mag = std::abs (H); if (s24) mag *= mag;
            const double db = jlimit (-36.0, 18.0, 20 * std::log10 (jmax (1e-6, mag)));
            const float px = b.getX() + b.getWidth() * i / N, py = b.getY() + (float) ((18 - db) / 54.0) * b.getHeight();
            if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (P.grid2.withAlpha (.5f)); g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * 18 / 54.0f), b.getX(), b.getRight());
        g.setColour (col); g.strokePath (path, PathStrokeType (1.8f));
    }
    {
        auto b = boxes[2]; Path path;
        const float A = x[envA], D = x[envD], S = x[envS], R = x[envR];
        const float hold = jmax (.25f, (A + D + R) * .3f), total = A + D + hold + R;
        auto X = [&] (float t) { return b.getX() + b.getWidth() * t / total; };
        auto Y = [&] (float lv) { return b.getBottom() - lv * b.getHeight(); };
        path.startNewSubPath (X (0), Y (0)); path.lineTo (X (A), Y (1));
        for (int i = 1; i <= 20; ++i) { const float t = D * i / 20; path.lineTo (X (A + t), Y (S + (1 - S) * std::exp (-3 * t / jmax (.001f, D)))); }
        path.lineTo (X (A + D + hold), Y (S));
        for (int i = 1; i <= 20; ++i) { const float t = R * i / 20; path.lineTo (X (A + D + hold + t), Y (S * std::exp (-3 * t / jmax (.001f, R)))); }
        g.setColour (col.withAlpha (.18f)); Path fill (path); fill.lineTo (X (total), Y (0)); fill.closeSubPath(); g.fillPath (fill);
        g.setColour (col); g.strokePath (path, PathStrokeType (1.8f));
    }
}

void SoundPage::doImport()
{
    chooser = std::make_unique<FileChooser> (T ("pImport"), File::getSpecialLocation (File::userDocumentsDirectory), "*.fxp;*.fxb;*.SerumPreset;*.serumpreset");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    {
        const auto f = fc.getResult(); if (! f.existsAsFile()) return;
        hg::ImportedSound info; const auto key = proc.importSynthFile (f, part, &info);
        const int pt = info.ok ? jlimit (0, 2, part) : part;
        auto msg = T (key).replace ("{n}", info.name.isNotEmpty() ? info.name : f.getFileNameWithoutExtension()).replace ("{f}", info.format).replace ("{p}", T (kPartKeys[pt])).replace ("{e}", info.error);
        if (pt != part) setPart (pt);
        rebuildList (true); setView (vMacro); hint.setText (msg, dontSendNotification); msgUntil = Time::currentTimeMillis() + 9000; repaint();
    });
}
