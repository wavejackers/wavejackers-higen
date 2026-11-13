#include "LoopPage.h"

using namespace juce;

LoopPage::LoopPage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data), cur (p.loop.get())
{
    for (auto* b : std::initializer_list<Button*> { &loadBtn, &captureBtn, &playBtn, &mutateBtn, &exportBtn }) addAndMakeVisible (*b);
    for (auto* c : { &barsBox, &stepsBox, &rootBox, &scaleBox, &laneBox, &modeBox }) addAndMakeVisible (*c);
    addAndMakeVisible (wave); addAndMakeVisible (grid); addAndMakeVisible (status);
    status.setJustificationType (Justification::centredLeft);
    for (int b : { 1, 2, 4, 8 }) barsBox.addItem (String (b), b);
    stepsBox.addItem ("16", 16); stepsBox.addItem ("32", 32);
    for (int i = 0; i < 12; ++i) rootBox.addItem (data.noteNames[i], i + 1);
    for (int i = 0; i < 5; ++i) scaleBox.addItem (lf::kScales[i].name, i + 1);
    barsBox.onChange = [this] { cur.bars = barsBox.getSelectedId(); commit(); };
    stepsBox.onChange = [this] { cur.steps = stepsBox.getSelectedId(); syncControls(); applyEuclid(); };
    rootBox.onChange = [this] { cur.root = rootBox.getSelectedId() - 1; commit(); };
    scaleBox.onChange = [this] { cur.scale = scaleBox.getSelectedId() - 1; commit(); };
    laneBox.onChange = [this] { valueLane = jmax (0, laneBox.getSelectedId() - 1); grid.repaint(); };
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("lfLoad"), File(), "*.wav;*.aif;*.aiff;*.flac");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { if (fc.getResult().existsAsFile()) openFile (fc.getResult()); });
    };
    captureBtn.onClick = [this]
    {
        const bool ok = proc.loop.captureNow();
        if (ok) { cur = proc.loop.get(); syncControls(); }
        status.setText (T (ok ? "lfCaptured" : "lfCaptureFail"), dontSendNotification);
    };
    playBtn.setClickingTogglesState (true);
    playBtn.onClick = [this] { proc.loop.setPlaying (playBtn.getToggleState()); };
    mutateBtn.onClick = [this] { mutateNow ((lf::MutateMode) jmax (0, modeBox.getSelectedId() - 1)); };
    static const double lim[7][3] = { { 0, 32, 1 }, { 0, 31, 1 }, { 0, 100, 1 }, { 0, 100, 1 }, { 0, 100, 1 }, { 0, 100, 1 }, { 0, 100, 1 } };
    for (int i = 0; i < 7; ++i)
    {
        auto& k = knobs[(size_t) i]; addAndMakeVisible (k); k.slider.setRange (lim[i][0], lim[i][1], lim[i][2]);
        k.slider.onValueChange = [this, i]
        {
            const float v = (float) knobs[(size_t) i].slider.getValue();
            if (i == 0) { cur.pulses = (int) v; applyEuclid(); return; }
            if (i == 1) { cur.rotation = (int) v; applyEuclid(); return; }
            (i == 2 ? cur.swing : i == 3 ? cur.human : i == 4 ? cur.space : i == 5 ? cur.drive : cur.motion) = v / 100.0f;
            commit();
        };
    }
    syncControls(); setLanguage ("en", false);
}

void LoopPage::setLanguage (const String& l, bool)
{
    lang = l;
    loadBtn.setButtonText (T ("lfLoad")); captureBtn.setButtonText (T ("lfCapture")); playBtn.setButtonText (T ("lfPlay")); mutateBtn.setButtonText (T ("lfMutate")); exportBtn.setButtonText (T ("lfExport"));
    captureBtn.setTooltip (T ("lfCaptureTip")); exportBtn.setTooltip (T ("lfExportTip"));
    static const char* kn[7] = { "lfPulses", "lfRot", "lfSwing", "lfHuman", "lfSpace", "lfDrive", "lfMotion" };
    for (int i = 0; i < 7; ++i) knobs[(size_t) i].setText (T (kn[i]), {});
    const int ln = laneBox.getSelectedId(), md = modeBox.getSelectedId();
    laneBox.clear (dontSendNotification); int id = 1; for (auto* k : { "lfVel", "lfCut", "lfPan", "lfDecay" }) laneBox.addItem (T (k), id++);
    modeBox.clear (dontSendNotification); id = 1; for (auto* k : { "lfSubtle", "lfEuclid", "lfArp" }) modeBox.addItem (T (k), id++);
    laneBox.setSelectedId (jmax (1, ln), dontSendNotification); modeBox.setSelectedId (md > 0 ? md : 2, dontSendNotification);
    barsBox.setTooltip (T ("lfBars")); stepsBox.setTooltip (T ("lfSteps"));
    if (! proc.loop.hasSample()) status.setText (T ("lfDrop"), dontSendNotification);
    repaint();
}

void LoopPage::syncControls()
{
    barsBox.setSelectedId (cur.bars, dontSendNotification); stepsBox.setSelectedId (cur.steps, dontSendNotification);
    rootBox.setSelectedId (cur.root + 1, dontSendNotification); scaleBox.setSelectedId (cur.scale + 1, dontSendNotification);
    knobs[0].slider.setRange (0, cur.steps, 1); knobs[1].slider.setRange (0, cur.steps - 1, 1);
    const float v[7] = { (float) cur.pulses, (float) cur.rotation, cur.swing * 100, cur.human * 100, cur.space * 100, cur.drive * 100, cur.motion * 100 };
    for (int i = 0; i < 7; ++i) knobs[(size_t) i].slider.setValue (v[i], dontSendNotification);
    playBtn.setToggleState (proc.loop.isPlaying(), dontSendNotification);
    grid.repaint(); wave.repaint();
}

void LoopPage::applyEuclid()   // yalnızca tetikleri değiştirir; perde ve diğer adım ayarları kalır
{
    cur.pulses = jlimit (0, cur.steps, cur.pulses); cur.rotation = jlimit (0, cur.steps - 1, cur.rotation);
    const auto e = lf::euclid (cur.steps, cur.pulses, cur.rotation);
    for (int i = 0; i < cur.steps; ++i) cur.step[(size_t) i].on = e[(size_t) i];
    commit();
}

void LoopPage::mutateNow (lf::MutateMode m)
{
    lf::mutate (cur, m, (uint32_t) Random::getSystemRandom().nextInt (0x7fffffff));
    commit(); syncControls();
}

bool LoopPage::openFile (const File& f)
{
    const bool ok = proc.loop.loadFile (f);
    if (ok) { cur = proc.loop.get(); syncControls(); }
    status.setText (ok ? f.getFileName() : T ("lfLoadFail"), dontSendNotification);
    return ok;
}

void LoopPage::tick()
{
    const int v = proc.loop.version();
    if (v != shownVersion) { shownVersion = v; proc.loop.peaks (wave.pk, jmax (64, wave.getWidth())); wave.repaint(); }
    if (proc.loop.isPlaying()) grid.repaint();
}

void LoopPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    g.setColour (P.panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
    g.setColour (P.mute); g.setFont (lnf.font (12));
    static const char* lanes[5] = { "lfTrig", "lfPitch", "", "lfRev", "lfRatchet" };
    for (int i = 0; i < 5; ++i) if (i != 2) g.drawText (T (lanes[i]), grid.lane (i).withX (16).withWidth (70).translated (0, grid.getY()), Justification::centredLeft);
}

void LoopPage::resized()
{
    int x = 16; auto put = [&] (Component& c, int w) { c.setBounds (x, 12, w, 30); x += w + 8; };
    put (loadBtn, 110); put (captureBtn, 110); put (playBtn, 90); put (barsBox, 64); put (stepsBox, 64); put (rootBox, 64); put (scaleBox, 150);
    exportBtn.setBounds (kW - 16 - 190, 12, 190, 30); status.setBounds (x, 12, kW - 16 - 198 - x, 30);
    wave.setBounds (16, 52, kW - 32, 150);
    grid.setBounds (90, 212, kW - 106, 300); laneBox.setBounds (8, grid.getY() + grid.lane (2).getY() + 22, 78, 26);
    for (int i = 0; i < 4; ++i) knobs[(size_t) i].setBounds (16 + i * 96, 524, 90, 112);
    modeBox.setBounds (430, 548, 190, 30); mutateBtn.setBounds (430, 586, 190, 40);
    for (int i = 4; i < 7; ++i) knobs[(size_t) i].setBounds (700 + (i - 4) * 150, 516, 130, 128);
}

// ---- dalga: kırpma tutamakları
void LoopPage::Wave::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); auto r = getLocalBounds().toFloat();
    g.setColour (P.panel2); g.fillRoundedRectangle (r, 8.0f);
    const float w = r.getWidth(), h = r.getHeight(), mid = h * .5f;
    if (pk.empty() || ! o.proc.loop.hasSample()) { g.setColour (P.mute); g.setFont (o.lnf.font (15)); g.drawText (o.T ("lfDrop"), getLocalBounds(), Justification::centred); return; }
    float mx = 1e-4f; for (auto v : pk) mx = jmax (mx, v);
    const float a = o.cur.start * w, z = o.cur.end * w;
    for (int i = 0; i < (int) pk.size(); ++i)
    {
        const float x = (float) i * w / (float) pk.size(), y = pk[(size_t) i] / mx * (mid - 6);
        g.setColour (x >= a && x <= z ? P.accent : P.mute.withAlpha (.5f));
        g.drawVerticalLine ((int) x, mid - y, mid + y + 1);
    }
    g.setColour (P.ink);
    for (float hx : { a, z }) { g.fillRect (hx - 1, 0.0f, 2.0f, h); g.fillRoundedRectangle (hx - 5, 0, 10, 14, 3.0f); }
}
void LoopPage::Wave::mouseDown (const MouseEvent& e)
{
    const float x = (float) e.x / (float) jmax (1, getWidth());
    drag = std::abs (x - o.cur.start) <= std::abs (x - o.cur.end) ? 0 : 1; mouseDrag (e);
}
void LoopPage::Wave::mouseDrag (const MouseEvent& e)
{
    const float x = jlimit (0.0f, 1.0f, (float) e.x / (float) jmax (1, getWidth()));
    if (drag == 0) o.cur.start = jmin (x, o.cur.end - .005f); else o.cur.end = jmax (x, o.cur.start + .005f);
    o.commit();
}

// ---- adım ızgarası: tetik, perde, değer (hız / filtre / pan / sönüm), ters, ratchet
Rectangle<int> LoopPage::Grid::lane (int i) const
{
    static const int y[5] = { 0, 40, 156, 236, 268 }, h[5] = { 34, 110, 74, 26, 26 };
    return { 0, y[i], getWidth(), h[i] };
}
void LoopPage::Grid::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); const int n = o.cur.steps; const float cw = (float) getWidth() / (float) n;
    const int head = o.proc.loop.isPlaying() ? (int) (o.proc.loop.playhead() * 16 * o.cur.bars) % n : -1;
    for (int i = 0; i < n; ++i)
    {
        const auto& s = o.cur.step[(size_t) i]; const float x = (float) i * cw + 1, w = cw - 2;
        const Colour base = P.panel2.brighter (i % 4 == 0 ? .14f : .03f), hot = i == head ? P.ink : P.accent;
        auto cell = [&] (int ln) { auto r = lane (ln).toFloat(); return Rectangle<float> (x, r.getY(), w, r.getHeight()); };
        auto t = cell (0); g.setColour (s.on ? hot : base); g.fillRoundedRectangle (t, 4.0f);
        auto p = cell (1); g.setColour (base); g.fillRoundedRectangle (p, 3.0f);
        if (s.on && s.pitch != 0) { const float u = p.getHeight() / 15.0f, c = p.getCentreY(); g.setColour (hot.withAlpha (.85f)); g.fillRect (Rectangle<float> (x + 1, s.pitch > 0 ? c - s.pitch * u : c, w - 2, std::abs (s.pitch) * u)); }
        g.setColour (P.line); g.fillRect (x, p.getCentreY(), w, 1.0f);
        auto v = cell (2); g.setColour (base); g.fillRoundedRectangle (v, 3.0f);
        if (s.on)
        {
            const float val = o.value (const_cast<lf::Step&> (s)); g.setColour (P.accent2.withAlpha (.85f));
            if (o.valueLane == 2) g.fillRect (Rectangle<float> (x + 1, val > 0 ? v.getCentreY() - val * v.getHeight() * .5f : v.getCentreY(), w - 2, jmax (1.0f, std::abs (val) * v.getHeight() * .5f)));
            else g.fillRect (Rectangle<float> (x + 1, v.getBottom() - val * v.getHeight(), w - 2, val * v.getHeight()));
        }
        auto rv = cell (3); g.setColour (s.on && s.rev ? P.warning : base); g.fillRoundedRectangle (rv, 3.0f);
        auto rt = cell (4); g.setColour (base); g.fillRoundedRectangle (rt, 3.0f);
        if (s.on && s.ratchet > 1) { g.setColour (P.ink); g.setFont (o.lnf.font (12, true)); g.drawText (String (s.ratchet), rt, Justification::centred); }
    }
}
void LoopPage::Grid::hit (const MouseEvent& e, bool down)
{
    const int n = o.cur.steps, i = jlimit (0, n - 1, e.x * n / jmax (1, getWidth()));
    for (int ln = 0; ln < 5; ++ln)
    {
        const auto r = lane (ln); if (e.y < r.getY() || e.y >= r.getBottom()) continue;
        auto& s = o.cur.step[(size_t) i];
        if (ln == 0) { if (down) paintOn = s.on ? 0 : 1; s.on = paintOn == 1; }       // sürükleyince aynı durumu boyar
        else if (ln == 3 || ln == 4) { if (! down) return; if (ln == 3) s.rev = ! s.rev; else s.ratchet = s.ratchet % 4 + 1; }
        else o.clickStep (ln, i, (float) (e.y - r.getY()) / (float) r.getHeight());
        o.commit(); return;
    }
}

void LoopPage::clickStep (int ln, int i, float y01)
{
    auto& s = cur.step[(size_t) jlimit (0, lf::kMaxSteps - 1, i)]; y01 = jlimit (0.0f, 1.0f, y01);
    if (ln == 0) s.on = ! s.on;
    else if (ln == 1) { s.pitch = jlimit (-7, 7, roundToInt ((.5f - y01) * 15.0f)); s.on = true; }
    else if (ln == 2) value (s) = valueLane == 2 ? jlimit (-1.0f, 1.0f, (.5f - y01) * 2.0f) : 1.0f - y01;
    else if (ln == 3) s.rev = ! s.rev;
    else s.ratchet = s.ratchet % 4 + 1;
    commit();
}

void LoopPage::DragHandle::mouseDrag (const MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 8 || busy) return;
    const ScopedValueSetter<bool> guard (busy, true);
    const auto f = o.proc.loop.exportWav();
    if (! f.existsAsFile()) { o.status.setText (o.T ("lfNoSample"), dontSendNotification); return; }
    DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}
