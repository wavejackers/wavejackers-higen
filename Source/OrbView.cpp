#include "OrbView.h"
#include <cmath>

using namespace juce;

namespace hgui
{
static constexpr float kTau = MathConstants<float>::twoPi;
static constexpr int kBins = 360;

bool OrbView::Model::operator== (const Model& o) const
{
    if (clip != o.clip || selCat != o.selCat || std::abs (selA - o.selA) > 1e-9 || std::abs (selB - o.selB) > 1e-9 || std::abs (bpm - o.bpm) > 1e-9 || key != o.key || minor != o.minor) return false;
    if (cands.size() != o.cands.size()) return false;
    for (size_t k = 0; k < cands.size(); ++k)
    {
        if (cands[k].size() != o.cands[k].size()) return false;
        for (size_t i = 0; i < cands[k].size(); ++i) if (cands[k][i].t0 != o.cands[k][i].t0 || cands[k][i].t1 != o.cands[k][i].t1) return false;
    }
    return true;
}

OrbView::OrbView (LookAndFeel& l) : lnf (l)
{
    setOpaque (false); setRepaintsOnMouseActivity (false);
    setTitle ("DNA Orb"); setDescription ("Reference audio as a ring: waveform, detected regions, selection and playback position.");
}

void OrbView::setModel (const Model& m)
{
    if (m == model) return;
    model = m; if (model.clip.get() != peaksFor) computePeaks();
    dirty = true; repaint();
}

void OrbView::setPlayhead (double sec, bool pl)
{
    if (std::abs (sec - pos) < 1e-4 && pl == playing) return;
    pos = sec; playing = pl; repaint();
}

void OrbView::setLevel (float lv)
{
    level = jlimit (0.0f, 1.0f, lv);
    const float nx = shownLevel + (level - shownLevel) * 0.35f;      // yumuşatma yalnızca arayüz iş parçacığında
    if (std::abs (nx - shownLevel) > 0.004f || (nx < 0.004f && shownLevel > 0)) { shownLevel = nx < 0.004f ? 0.0f : nx; repaint(); }
}

// Halka için tepe/RMS: sesin tamamı 360 kutuya; çok uzun dosyalarda adımla örneklenir (en çok ~1.5 M okuma)
void OrbView::computePeaks()
{
    peak.assign (kBins, 0.0f); rms.assign (kBins, 0.0f); peaksFor = model.clip.get();
    auto* c = model.clip.get(); if (c == nullptr || c->numSamples() == 0) return;
    const int n = c->numSamples(); const float* L = c->audio.getReadPointer (0); const float* R = c->audio.getNumChannels() > 1 ? c->audio.getReadPointer (1) : L;
    const int stride = jmax (1, n / (kBins * 4000)); float top = 1e-6f;
    for (int b = 0; b < kBins; ++b)
    {
        const int a = (int) ((int64) n * b / kBins), e = (int) ((int64) n * (b + 1) / kBins); float pk = 0; double sq = 0; int cnt = 0;
        for (int i = a; i < e; i += stride) { const float v = 0.5f * (L[i] + R[i]); pk = jmax (pk, std::abs (v)); sq += (double) v * v; ++cnt; }
        peak[(size_t) b] = pk; rms[(size_t) b] = cnt > 0 ? (float) std::sqrt (sq / cnt) : 0.0f; top = jmax (top, pk);
    }
    for (int b = 0; b < kBins; ++b) { peak[(size_t) b] /= top; rms[(size_t) b] = jmin (1.0f, rms[(size_t) b] / top * 1.4f); }
}

double OrbView::secAt (Point<float> p) const
{
    auto* c = model.clip.get(); if (c == nullptr) return -1;
    const auto ct = centre(); const float R = radius(), d = p.getDistanceFrom (ct);
    if (d < R * 0.42f || d > R + 2) return -1;
    float a = std::atan2 (p.x - ct.x, ct.y - p.y); if (a < 0) a += kTau;
    return (double) (a / kTau) * c->seconds();
}

void OrbView::renderStatic (Graphics& g, float sc)
{
    const auto& P = lnf.pal(); const float W = (float) getWidth(), H = (float) getHeight();
    g.addTransform (AffineTransform::scale (sc));
    const Point<float> c (W * 0.5f, H * 0.5f); const float R = radius();
    const float rWave = R * 0.60f, waveLen = R * 0.27f, rRegion = R * 0.935f, rCore = R * 0.50f;
    auto* clip = model.clip.get();

    // zemin: halkanın içi koyu çivit, kenara doğru sönen ince bir hale
    {
        ColourGradient halo (P.accent.withAlpha (0.16f), c.x, c.y, P.accent.withAlpha (0.0f), c.x + R * 1.04f, c.y, true);
        halo.addColour (0.80, P.accent.withAlpha (0.10f));
        g.setGradientFill (halo); g.fillEllipse (Rectangle<float> (R * 2.08f, R * 2.08f).withCentre (c));
        ColourGradient disc (P.panel2, c.x, c.y - R * 0.3f, P.bg, c.x, c.y + R, true);
        g.setGradientFill (disc); g.fillEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c));
        g.setColour (P.line.brighter (0.15f)); g.drawEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c), 1.2f);
        g.setColour (P.line.withAlpha (0.6f)); g.drawEllipse (Rectangle<float> (rRegion * 2, rRegion * 2).withCentre (c), 1.0f);
    }

    if (clip != nullptr)
    {
        const double dur = jmax (0.01, clip->seconds());
        // vuruş çentikleri: yalnızca BPM gerçekten algılandıysa
        if (model.bpm > 0)
        {
            const int beats = (int) std::floor (dur * model.bpm / 60.0);
            if (beats > 0 && beats <= 720)
                for (int b = 0; b <= beats; ++b)
                {
                    const float a = (float) (b * 60.0 / model.bpm / dur) * kTau; const bool bar = b % 4 == 0;
                    g.setColour (bar ? P.mute.withAlpha (0.8f) : P.mute.withAlpha (0.28f));
                    g.drawLine (Line<float> (polar (c, R - (bar ? 9.0f : 5.0f), a), polar (c, R - 1.5f, a)), bar ? 1.4f : 1.0f);
                }
        }
        // seçimin dilimi
        if (model.selB > model.selA + 0.01)
        {
            const float a0 = (float) (model.selA / dur) * kTau, a1 = (float) (model.selB / dur) * kTau;
            Path wedge; wedge.addPieSegment (Rectangle<float> (rRegion * 2, rRegion * 2).withCentre (c), a0, a1, rCore / rRegion);
            g.setColour (P.accent.withAlpha (0.10f)); g.fillPath (wedge);
        }
        // dalga formu halkası: dışa tepe, içe RMS; renk zaman boyunca vurgu rengi ile ikinci renk arasında kayar
        const auto cA = P.accent, cB = catColour ? catColour (1) : P.accent2;
        const float bw = jmax (1.0f, kTau * rWave / kBins * 0.52f);
        for (int b = 0; b < kBins; ++b)
        {
            const float a = (b + 0.5f) / kBins * kTau, t = 0.5f - 0.5f * std::cos (a);
            const float pk = peak.empty() ? 0.0f : peak[(size_t) b], rm = rms.empty() ? 0.0f : rms[(size_t) b];
            const float out = 1.5f + pk * waveLen, in = 1.0f + rm * waveLen * 0.42f;
            g.setColour (cA.interpolatedWith (cB, t).withAlpha (0.55f + 0.45f * pk));
            g.drawLine (Line<float> (polar (c, rWave - in, a), polar (c, rWave + out, a)), bw);
        }
        // Smart Detect bölgeleri: kategori renginde yaylar (seçili olan kalın)
        for (int k = 0; k < (int) model.cands.size(); ++k)
            for (auto& rg : model.cands[(size_t) k])
            {
                const float a0 = (float) (rg.t0 / dur) * kTau, a1 = (float) (rg.t1 / dur) * kTau;
                const bool sel = model.selCat == k && std::abs (model.selA - rg.t0) < 1e-6;
                Path arc; arc.addCentredArc (c.x, c.y, rRegion, rRegion, 0, a0, a1, true);
                const auto col = catColour ? catColour (k) : P.accent;
                g.setColour (col.withAlpha (sel ? 1.0f : 0.55f)); g.strokePath (arc, PathStrokeType (sel ? 6.0f : 3.0f, PathStrokeType::curved, PathStrokeType::rounded));
            }
        if (model.selB > model.selA + 0.01 && model.selCat < 0)
        {
            Path arc; arc.addCentredArc (c.x, c.y, rRegion, rRegion, 0, (float) (model.selA / dur) * kTau, (float) (model.selB / dur) * kTau, true);
            g.setColour (P.accent); g.strokePath (arc, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        }
    }
    else
    {
        // boş durum: kesikli halka, bırakma ipucu
        Path ring; ring.addEllipse (Rectangle<float> (rWave * 2, rWave * 2).withCentre (c)); Path dashed; const float dl[] = { 4.0f, 7.0f };
        PathStrokeType (1.2f).createDashedStroke (dashed, ring, dl, 2); g.setColour (P.accent.withAlpha (0.55f)); g.fillPath (dashed);
    }

    // çekirdek: parlak, iki tonlu küre (dekoratif; üstündeki tüm sayılar gerçek)
    {
        const auto core = Rectangle<float> (rCore * 2, rCore * 2).withCentre (c);
        ColourGradient body (Colour (0xfffff1cc), c.x - rCore * 0.42f, c.y - rCore * 0.48f, Colour (0xff2a1c08), c.x + rCore * 0.55f, c.y + rCore * 0.80f, true);
        body.addColour (0.30, Colour (0xffe6c070)); body.addColour (0.62, Colour (0xffa87a28)); body.addColour (0.86, Colour (0xff4f3510));   // asa ucundaki altın küre
        g.setGradientFill (body); g.fillEllipse (core);
        const auto tint = catColour ? catColour (1) : P.accent;    // ikinci renk alt sağdan sızar
        ColourGradient rim (tint.withAlpha (0.0f), c.x - rCore * 0.1f, c.y - rCore * 0.1f, tint.withAlpha (0.55f), c.x + rCore * 0.78f, c.y + rCore * 0.70f, true);
        g.setGradientFill (rim); g.fillEllipse (core);
        const auto pink = catColour ? catColour (2) : P.accent2;
        ColourGradient bloom (pink.withAlpha (0.32f), c.x - rCore * 0.55f, c.y + rCore * 0.55f, pink.withAlpha (0.0f), c.x - rCore * 0.05f, c.y + rCore * 0.15f, true);
        g.setGradientFill (bloom); g.fillEllipse (core);
        ColourGradient centreDim (Colour (0xff140d03).withAlpha (0.55f), c.x, c.y + rCore * 0.05f, Colour (0xff140d03).withAlpha (0.0f), c.x + rCore * 0.62f, c.y, true);
        g.setGradientFill (centreDim); g.fillEllipse (core);   // ortadaki sayılar okunsun diye merkez hafif koyulaşır
        g.setColour (Colours::white.withAlpha (0.18f)); g.drawEllipse (core.reduced (0.5f), 1.0f);
    }
}

void OrbView::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    const float sc = jmax (1.0f, (float) g.getInternalContext().getPhysicalPixelScaleFactor());
    if (dirty || cache.isNull() || std::abs (sc - cacheScale) > 0.01f)
    {
        cache = Image (Image::ARGB, jmax (1, roundToInt (getWidth() * sc)), jmax (1, roundToInt (getHeight() * sc)), true);
        Graphics cg (cache); renderStatic (cg, sc); cacheScale = sc; dirty = false;
    }
    g.drawImage (cache, getLocalBounds().toFloat());

    const auto c = centre(); const float R = radius(), rCore = R * 0.50f; auto* clip = model.clip.get();
    // seviye: çekirdeğin çevresinde gerçek çıkış seviyesine göre büyüyen ince halka
    if (shownLevel > 0.0f)
    {
        const float rr = rCore + 3.0f + shownLevel * R * 0.06f;
        g.setColour (P.accent.withAlpha (0.20f + 0.55f * shownLevel)); g.drawEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c), 1.0f + 2.0f * shownLevel);
    }
    if (clip != nullptr)
    {
        const double dur = jmax (0.01, clip->seconds());
        const float a = (float) (jlimit (0.0, dur, pos) / dur) * kTau;
        if (playing || pos > 0)
        {
            Path prog; prog.addCentredArc (c.x, c.y, R + 0.5f, R + 0.5f, 0, 0, a, true);
            g.setColour (P.ink.withAlpha (0.55f)); g.strokePath (prog, PathStrokeType (1.6f));
            g.setColour (P.ink.withAlpha (playing ? 0.95f : 0.6f)); g.drawLine (Line<float> (polar (c, rCore + 6, a), polar (c, R, a)), 1.4f);
            g.fillEllipse (Rectangle<float> (7, 7).withCentre (polar (c, R, a)));
        }
        if (hoverSec >= 0)
        {
            const float ha = (float) (hoverSec / dur) * kTau; g.setColour (P.ink.withAlpha (0.3f)); g.drawLine (Line<float> (polar (c, rCore + 6, ha), polar (c, R, ha)), 1.0f);
        }
    }
    // merkez: üstte bağlam, ortada BPM, altta ton (yalnızca gerçek analiz; yoksa tire)
    {
        const String dash = String (CharPointer_UTF8 ("\xe2\x80\x94"));
        const bool hasBpm = clip != nullptr && model.bpm > 0;
        g.setColour (Colours::white.withAlpha (0.72f)); g.setFont (lnf.font (11.5f, true));
        g.drawText (topText.toUpperCase(), Rectangle<float> (rCore * 1.6f, 16).withCentre ({ c.x, c.y - rCore * 0.48f }), Justification::centred);
        g.setColour (Colours::white); g.setFont (lnf.font (jmax (20.0f, rCore * 0.36f), true));
        g.drawText (hasBpm ? String (model.bpm, model.bpm == std::floor (model.bpm) ? 0 : 1) : dash, Rectangle<float> (rCore * 1.6f, rCore * 0.42f).withCentre ({ c.x, c.y - rCore * 0.08f }), Justification::centred);
        g.setColour (Colours::white.withAlpha (0.72f)); g.setFont (lnf.font (11.0f));
        g.drawText (bpmWord, Rectangle<float> (rCore * 1.2f, 14).withCentre ({ c.x, c.y + rCore * 0.17f }), Justification::centred);
        static const char* const kn[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        g.setColour (Colours::white); g.setFont (lnf.font (16.0f, true));
        g.drawText (clip != nullptr && model.key >= 0 ? String (kn[model.key % 12]) + (model.minor ? " minor" : " major") : dash,
                    Rectangle<float> (rCore * 1.4f, 20).withCentre ({ c.x, c.y + rCore * 0.40f }), Justification::centred);
        if (bottomText.isNotEmpty()) { g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (lnf.font (10.5f)); g.drawText (bottomText, Rectangle<float> (rCore * 1.3f, 14).withCentre ({ c.x, c.y + rCore * 0.62f }), Justification::centred); }
    }
    if (clip == nullptr && emptyText.isNotEmpty())
    {
        g.setColour (P.mute); g.setFont (lnf.font (13.0f));
        g.drawFittedText (emptyText, Rectangle<int> ((int) (R * 1.3f), 40).withCentre ({ (int) c.x, (int) (c.y + R * 0.78f) }), Justification::centred, 2);
    }
}

void OrbView::mouseDown (const MouseEvent& e)
{
    if (model.clip == nullptr) { if (onOpen) onOpen(); return; }
    const double s = secAt (e.position); if (s >= 0 && onSeek) onSeek (s);
}

void OrbView::mouseMove (const MouseEvent& e)
{
    const double s = secAt (e.position);
    setMouseCursor (model.clip == nullptr || s >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    if (std::abs (s - hoverSec) > 1e-3) { hoverSec = s; repaint(); }
}

std::unique_ptr<AccessibilityHandler> OrbView::createAccessibilityHandler() { return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image); }
} // namespace hgui
