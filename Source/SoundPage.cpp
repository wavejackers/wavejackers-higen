#include "SoundPage.h"
#include <cmath>
#include <complex>

using namespace juce;
using namespace hg;

namespace
{
const char* const kPartKeys[3] = { "chords", "bass", "melody" };
bool isToggle (int i) { return i == l1sync || i == l2sync; }
}

SoundPage::SoundPage (HitGenProcessor& p, hgui::LookAndFeel& l)
    : proc (p), lnf (l), data (p.data), keyboard (p.keyboardState, MidiKeyboardComponent::horizontalKeyboard)
{
    setFocusContainerType (FocusContainerType::focusContainer);
    for (int i = 0; i < 3; ++i)
    {
        auto& b = partBtns[(size_t) i]; addAndMakeVisible (b); b.setRadioGroupId (201); b.setClickingTogglesState (true);
        b.onClick = [this, i] { if (partBtns[(size_t) i].getToggleState() && part != i) setPart (i); };
    }
    partBtns[0].setToggleState (true, dontSendNotification);
    addAndMakeVisible (baseBox);
    baseBox.onChange = [this]
    {
        const int i = baseBox.getSelectedId() - 1;
        if (i >= 0 && i < baseIds.size() && baseIds[i] != proc.soundId (trackOfPart())) { proc.setSound (trackOfPart(), baseIds[i], true); shownBase = baseIds[i]; }
    };
    addAndMakeVisible (randBtn); randBtn.onClick = [this] { proc.randomizeSound (part); };
    addAndMakeVisible (resetBtn); resetBtn.onClick = [this] { proc.resetSoundDesign (part); };
    for (auto* lb : { &hint, &kbdHint }) { addAndMakeVisible (*lb); lb->setFont (lnf.font (12)); lb->setJustificationType (Justification::centredLeft); }

    for (int i = 0; i < kNumSp; ++i)
    {
        const auto& d = spDef (i);
        if (isToggle (i)) { auto t = std::make_unique<ToggleButton>(); addAndMakeVisible (*t); toggles[i] = std::move (t); continue; }
        if (d.kind == spChoice) { auto c = std::make_unique<ComboBox>(); addAndMakeVisible (*c); combos[i] = std::move (c); continue; }
        auto k = std::make_unique<hgui::Knob>(); addAndMakeVisible (*k); knobs[i] = std::move (k);
    }
    scope.owner = this; addAndMakeVisible (scope);
    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (24, 108); keyboard.setLowestVisibleKey (48); keyboard.setKeyWidth (17.0f);
    keyboard.setScrollButtonsVisible (true); keyboard.setVelocity (.8f, true);
    keyboard.setWantsKeyboardFocus (true);
    fillCombos(); fillBase(); bind();
}

SoundPage::~SoundPage() { sa.clear(); ca.clear(); ba.clear(); }

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

void SoundPage::fillBase()
{
    const int t = trackOfPart(); const char role = part == 0 ? 'c' : part == 1 ? 'b' : 'm';
    baseBox.clear (dontSendNotification); baseIds.clear();
    for (int pass = 0; pass < 2; ++pass)
    {
        if (pass == 1) baseBox.addSeparator();
        for (int s = 0; s < data.numSounds(); ++s)
        {
            const auto& v = data.sound (s); const auto id = v["id"].toString();
            if (! v["r"].toString().containsChar ((juce_wchar) role) || data.isSignature (id) != (pass == 0)) continue;
            baseIds.add (id); baseBox.addItem (data.soundName (id, lang) + (pass == 1 ? "  (" + T ("cat_" + v["cat"].toString()) + ")" : String()), baseIds.size());
        }
    }
    shownBase = proc.soundId (t);
    baseBox.setSelectedId (baseIds.indexOf (shownBase) + 1, dontSendNotification);
}

void SoundPage::bind()
{
    sa.clear(); ca.clear(); ba.clear();
    for (auto& [i, k] : knobs) sa.push_back (std::make_unique<SA> (proc.apvts, spId (part, i), k->slider));
    for (auto& [i, c] : combos) ca.push_back (std::make_unique<CA> (proc.apvts, spId (part, i), *c));
    for (auto& [i, t] : toggles) ba.push_back (std::make_unique<BA> (proc.apvts, spId (part, i), *t));
}

void SoundPage::setPart (int p)
{
    part = jlimit (0, 2, p);
    partBtns[(size_t) part].setToggleState (true, dontSendNotification);
    bind(); fillBase(); setLanguage (lang, rtl);
    if (isShowing()) proc.auditionPart.store (part);
    scope.repaint();
}

void SoundPage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r;
    fillCombos();
    for (int i = 0; i < 3; ++i) partBtns[(size_t) i].setButtonText (T (kPartKeys[i]));
    const String trackName = T (kPartKeys[part]);
    baseBox.setTitle (T ("baseSound") + ": " + trackName); baseBox.setTooltip (T ("baseSound"));
    randBtn.setButtonText (T ("rand")); randBtn.setTitle (T ("rand") + ": " + trackName);
    resetBtn.setButtonText (T ("reset")); resetBtn.setTitle (T ("reset") + ": " + trackName);
    hint.setText (T ("sdHint2"), dontSendNotification); kbdHint.setText (T ("kbdHint"), dontSendNotification);
    for (auto* lb : { &hint, &kbdHint }) { lb->setJustificationType (rtl ? Justification::centredRight : Justification::centredLeft); lb->setColour (Label::textColourId, lnf.pal().mute); }
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
    resized(); repaint();
}

void SoundPage::tick()
{
    const auto cur = proc.soundId (trackOfPart());
    if (cur != shownBase) fillBase();
    std::array<float, kNumSp> v {};
    for (int i = 0; i < kNumSp; ++i) v[(size_t) i] = proc.apvts.getRawParameterValue (spId (part, i))->load();
    if (v != scope.v) { scope.v = v; scope.repaint(); }
    if (isShowing() && proc.auditionPart.load() != part) proc.auditionPart.store (part);
}

void SoundPage::resized()
{
    layoutList.clear(); titles.clear(); panels.clear();
    for (int i = 0; i < 3; ++i) place (partBtns[(size_t) i], { 16 + i * 104, 10, 100, 30 });
    place (baseBox, { 340, 10, 300, 30 }); place (randBtn, { 652, 10, 150, 30 }); place (resetBtn, { 810, 10, 150, 30 });
    place (hint, { 16, 44, 1132, 18 });
    auto K = [this] (int i, int x, int y, int w = 78, int h = 80) { if (knobs.count (i)) place (*knobs[i], { x, y, w, h }); };
    auto C = [this] (int i, int x, int y, int w, int h = 26) { if (combos.count (i)) place (*combos[i], { x, y, w, h }); };
    // osilatörler
    panels.push_back ({ 8, 66, 636, 290 }); titles.push_back ({ "osc", {}, { 20, 70, 300, 22 }, false });
    const int ob[3] = { o1w, o2w, o3w };
    for (int j = 0; j < 3; ++j)
    {
        const int y0 = 96 + j * 86;
        titles.push_back ({ "oscN", String (j + 1), { 20, y0 + 6, 132, 16 }, true });
        C (ob[j], 20, y0 + 26, 132);
        for (int k = 1; k < 5; ++k) K (ob[j] + k, 160 + (k - 1) * 80, y0);
    }
    K (uni, 488, 96); K (uniDet, 564, 96); K (sub, 488, 182); K (noise, 564, 182);
    // wavetable + önizleme + klavye
    panels.push_back ({ 652, 66, 504, 290 }); titles.push_back ({ "wt", {}, { 664, 70, 300, 22 }, false });
    titles.push_back ({ "tableA", {}, { 664, 94, 150, 14 }, true }); C (wtA, 664, 108, 150);
    titles.push_back ({ "tableB", {}, { 664, 138, 150, 14 }, true }); C (wtB, 664, 152, 150);
    K (wtMorph, 824, 96); K (wtBright, 904, 96);
    place (scope, { 664, 184, 482, 84 });
    place (keyboard, { 664, 272, 482, 64 }); place (kbdHint, { 664, 336, 482, 16 });
    // filtre
    panels.push_back ({ 8, 364, 420, 282 }); titles.push_back ({ "filter", {}, { 20, 368, 300, 22 }, false });
    C (ftype, 20, 396, 140); C (fslope, 168, 396, 100);
    K (cut, 20, 430, 98, 84); K (reso, 120, 430, 98, 84); K (fenv, 220, 430, 98, 84); K (fatk, 320, 430, 98, 84);
    K (fdec, 20, 518, 98, 84); K (ktrack, 120, 518, 98, 84); K (fdrive, 220, 518, 98, 84);
    // zarf
    panels.push_back ({ 436, 364, 220, 282 }); titles.push_back ({ "ampEnv", {}, { 448, 368, 200, 22 }, false });
    K (envA, 448, 396, 98, 84); K (envD, 548, 396, 98, 84); K (envS, 448, 486, 98, 84); K (envR, 548, 486, 98, 84);
    // LFO
    panels.push_back ({ 664, 364, 300, 282 });
    const int lb[2] = { l1rate, l2rate };
    for (int k = 0; k < 2; ++k)
    {
        const int y0 = 368 + k * 136;
        titles.push_back ({ "lfo", String (k + 1), { 676, y0, 200, 22 }, false });
        C (lb[k] + 2, 676, y0 + 26, 118); C (lb[k] + 3, 676, y0 + 58, 118);
        if (toggles.count (lb[k] + 4)) place (*toggles[lb[k] + 4], { 676, y0 + 90, 118, 24 });
        K (lb[k], 800, y0 + 24, 78, 84); K (lb[k] + 1, 880, y0 + 24, 78, 84);
    }
    // efektler
    panels.push_back ({ 972, 364, 184, 282 }); titles.push_back ({ "fx", {}, { 984, 368, 160, 22 }, false });
    K (fxDrive, 984, 392, 80, 82); K (fxChorus, 1068, 392, 80, 82); K (fxDelay, 984, 476, 80, 82); K (fxReverb, 1068, 476, 80, 82); K (level, 984, 560, 80, 82);
    for (auto& [c, r] : layoutList) c->setBounds (mir (r));
}

void SoundPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    for (auto r : panels) { g.setColour (P.panel); g.fillRoundedRectangle (mir (r).toFloat(), 14); }
    for (auto& t : titles)
    {
        String s = T (t.key); if (t.arg.isNotEmpty()) s = s.replace ("{x}", t.arg).replace ("{n}", t.arg);
        g.setColour (t.small ? P.mute : P.ink); g.setFont (lnf.font (t.small ? 12.0f : 15.0f, ! t.small));
        g.drawText (s, mir (t.r), rtl ? Justification::centredRight : Justification::centredLeft);
    }
    // seçili izin rengiyle ince vurgu çizgisi
    g.setColour (P.track[trackOfPart()]); g.fillRect (mir (Rectangle<int> (16, 40, 312, 2)));
}

// ---------------------------------------------------------------- önizleme: dalga, filtre eğrisi, zarf
std::unique_ptr<AccessibilityHandler> SoundPage::Scope::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image);
}

void SoundPage::Scope::paint (Graphics& g)
{
    auto& o = *owner; const auto& P = o.lnf.pal();
    const auto col = P.track[o.trackOfPart()];
    auto area = getLocalBounds().toFloat();
    g.setColour (P.panel2); g.fillRoundedRectangle (area, 8);
    const float w3 = area.getWidth() / 3;
    std::array<Rectangle<float>, 3> boxes;
    for (int i = 0; i < 3; ++i) boxes[(size_t) i] = Rectangle<float> (area.getX() + i * w3, area.getY(), w3, area.getHeight()).reduced (8, 8);
    if (o.rtl) std::swap (boxes[0], boxes[2]);
    const auto& x = v;
    // 1) tek periyot dalga biçimi (osilatör karışımı)
    {
        auto b = boxes[0]; Path path; const int N = 160;
        const auto& W = o.data.wavetables();
        const int A = jlimit (0, 11, (int) x[wtA]), B = jlimit (0, 11, (int) x[wtB]);
        std::array<float, 48> h {};
        for (int n = 1; n <= 48; ++n)
        {
            const double m = std::round (x[wtMorph] * 16) / 16, br = std::round (x[wtBright] * 16) / 16, cutH = 2 + br * 46;
            h[(size_t) n - 1] = (float) (((double) W[A][n - 1] * (1 - m) + (double) W[B][n - 1] * m) * (n <= cutH ? 1.0 : std::exp (-(n - cutH) / 3.0)));
        }
        double wtNorm = 0; std::vector<float> ys ((size_t) N + 1);
        auto wtAt = [&] (double ph) { double s = 0; for (int n = 1; n <= 48; ++n) s += h[(size_t) n - 1] * std::sin (MathConstants<double>::twoPi * n * ph); return s; };
        for (int i = 0; i <= 64; ++i) wtNorm = jmax (wtNorm, std::abs (wtAt (i / 64.0)));
        if (wtNorm < 1e-9) wtNorm = 1;
        float peak = 1e-6f;
        const int ob[3] = { o1w, o2w, o3w };
        for (int i = 0; i <= N; ++i)
        {
            const double t = i / (double) N; double y = 0;
            for (int j = 0; j < 3; ++j)
            {
                const int wv = (int) x[(size_t) ob[j]]; if (wv == 0) continue;
                const double mul = std::pow (2.0, x[(size_t) ob[j] + 1] + x[(size_t) ob[j] + 2] / 12.0);
                double ph = t * mul; ph -= std::floor (ph);
                double s = wv == 1 ? 1 - 2 * ph : wv == 2 ? (ph < .5 ? 1 : -1) : wv == 3 ? (ph < .5 ? 4 * ph - 1 : 3 - 4 * ph) : wv == 4 ? std::sin (MathConstants<double>::twoPi * ph) : wtAt (ph) / wtNorm;
                y += s * x[(size_t) ob[j] + 4];
            }
            y += std::sin (MathConstants<double>::pi * t) * x[sub] * .8;
            ys[(size_t) i] = (float) y; peak = jmax (peak, std::abs ((float) y));
        }
        for (int i = 0; i <= N; ++i)
        {
            const float px = b.getX() + b.getWidth() * i / N, py = b.getCentreY() - ys[(size_t) i] / peak * b.getHeight() * .45f;
            if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (P.grid); g.drawHorizontalLine ((int) b.getCentreY(), b.getX(), b.getRight());
        g.setColour (col); g.strokePath (path, PathStrokeType (1.8f));
    }
    // 2) filtre tepkisi (SVF, 20 Hz - 20 kHz, logaritmik)
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
        g.setColour (P.grid); g.drawHorizontalLine ((int) (b.getY() + b.getHeight() * 18 / 54.0f), b.getX(), b.getRight());
        g.setColour (col); g.strokePath (path, PathStrokeType (1.8f));
    }
    // 3) ses zarfı (ADSR)
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
