#include "ChoirPage.h"

using namespace juce;

namespace
{
const char* const kKnobIds[7] = { "ch_level", "ch_space", "ch_detune", "ch_air", "ch_vibrato", "ch_attack", "ch_char" };
const char* const kKnobKeys[7] = { "chLevel", "chSpace", "chDetune", "chAir", "chVibrato", "chAttack", "chChar" };
const char* const kPartKeys[4] = { "chBass", "chTenor", "chAlto", "chSoprano" };
}

ChoirPage::ChoirPage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data), cur (p.choir.get())
{
    for (auto* b : std::initializer_list<Button*> { &recBtn, &loadBtn, &playBtn, &exportBtn }) addAndMakeVisible (*b);
    for (auto* c : { &sourceBox, &rhythmBox, &sizeBox, &presetBox }) addAndMakeVisible (*c);
    addAndMakeVisible (stage); addAndMakeVisible (status);
    status.setJustificationType (Justification::centredLeft); stage.setMouseCursor (MouseCursor::PointingHandCursor);
    sourceBox.onChange = [this]
    {
        cur.source = jmax (0, sourceBox.getSelectedId() - 1); commit();
        if (cur.source >= ch::sOwnHold && ! proc.choir.hasVoice()) status.setText (T ("chNeedVoice"), dontSendNotification);
    };
    rhythmBox.onChange = [this] { cur.rhythm = jmax (0, rhythmBox.getSelectedId() - 1); commit(); };
    sizeBox.onChange = [this] { cur.size = jlimit (1, 4, sizeBox.getSelectedId()); commit(); };
    presetBox.onChange = [this] { const int i = presetBox.getSelectedId() - 1; presetBox.setSelectedId (0, dontSendNotification); if (i >= 0) applyPreset (i); };
    recBtn.onClick = [this] { toggleRecord(); };
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("chLoad"), File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { if (fc.getResult().existsAsFile()) openFile (fc.getResult()); });
    };
    playBtn.setClickingTogglesState (true);
    playAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "ch_play", playBtn);
    for (int i = 0; i < 7; ++i)
    {
        auto& k = knobs[(size_t) i]; addAndMakeVisible (k);
        sa.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, kKnobIds[i], k.slider));
        hgui::tagParam (k.slider, kKnobIds[i]);
    }
    setLanguage ("en", false);
}

ChoirPage::~ChoirPage() { stopMic(); if (proc.choir.recording()) proc.choir.recordStop(); }

void ChoirPage::setLanguage (const String& l, bool)
{
    lang = l;
    recBtn.setButtonText (T (proc.choir.recording() ? "chStop" : "chRecord")); loadBtn.setButtonText (T ("chLoad")); playBtn.setButtonText (T ("chPlay")); exportBtn.setButtonText (T ("lfExport"));
    recBtn.setTooltip (T ("chRecordTip")); exportBtn.setTooltip (T ("lfExportTip")); stage.setTooltip (T ("chStageTip"));
    for (int i = 0; i < 7; ++i) knobs[(size_t) i].setText (T (kKnobKeys[i]), {});
    sourceBox.clear (dontSendNotification); int id = 1; for (auto* k : { "chAah", "chOoh", "chEeh", "chMmm", "chOwnHold", "chOwnWords" }) sourceBox.addItem (T (k), id++);
    rhythmBox.clear (dontSendNotification); id = 1; for (auto* k : { "chHold", "chHalf", "chQuarter", "chStab" }) rhythmBox.addItem (T (k), id++);
    sizeBox.clear (dontSendNotification); for (int n = 1; n <= 4; ++n) sizeBox.addItem (T ("chSize").replace ("{n}", String (n)), n);
    presetBox.clear (dontSendNotification); id = 1; for (auto* k : { "chPre0", "chPre1", "chPre2", "chPre3" }) presetBox.addItem (T (k), id++);
    presetBox.setTextWhenNothingSelected (T ("chPreset"));
    syncControls(); status.setText (voiceLine(), dontSendNotification); repaint();
}

void ChoirPage::syncControls()
{
    sourceBox.setSelectedId (cur.source + 1, dontSendNotification); rhythmBox.setSelectedId (cur.rhythm + 1, dontSendNotification); sizeBox.setSelectedId (cur.size, dontSendNotification);
    stage.dirty = true;
}

String ChoirPage::voiceLine() const
{
    if (! proc.choir.hasVoice()) return T ("chVoiceNone");
    const double f = proc.choir.voicePitch(); String s = proc.choir.voiceName();
    if (f > 0) { const int m = roundToInt (69 + 12 * std::log2 (f / 440.0)); s << "  ·  " << data.noteNames[((m % 12) + 12) % 12] << String (m / 12 - 1) << "  ·  " << String (roundToInt (f)) << " Hz"; }
    else s << "  ·  " << T ("chNoPitch");
    return s;
}

void ChoirPage::applyPreset (int i)
{
    const bool own = cur.source >= ch::sOwnHold && proc.choir.hasVoice();
    cur.part = { { true, true, true, true } }; cur.rhythm = ch::rHold; cur.character = 0;
    if (i == 0) { cur.size = 4; cur.space = .8f; cur.detune = .4f; cur.air = .35f; cur.vibrato = .3f; cur.attack = .55f; if (! own) cur.source = ch::sAah; }
    else if (i == 1) { cur.part = { { true, true, false, false } }; cur.size = 4; cur.space = .7f; cur.detune = .25f; cur.air = .2f; cur.vibrato = .1f; cur.attack = .5f; cur.character = -.3f; if (! own) cur.source = ch::sOoh; }
    else if (i == 2) { cur.part = { { false, false, true, true } }; cur.size = 3; cur.space = .75f; cur.detune = .35f; cur.air = .7f; cur.vibrato = .45f; cur.attack = .45f; cur.character = .35f; if (! own) cur.source = ch::sAah; }
    else { cur.rhythm = ch::rStab; cur.size = 3; cur.space = .3f; cur.detune = .3f; cur.air = .5f; cur.vibrato = .15f; cur.attack = 0; if (! own) cur.source = ch::sAah; }
    commit(); syncControls();
}

void ChoirPage::togglePart (int part)
{
    part = jlimit (0, ch::kParts - 1, part); cur.part[(size_t) part] = ! cur.part[(size_t) part]; commit();
}

void ChoirPage::stopMic() { if (mic != nullptr) { mic->removeAudioCallback (this); mic->closeAudioDevice(); mic.reset(); } }

void ChoirPage::audioDeviceIOCallbackWithContext (const float* const* in, int numIn, float* const* out, int numOut, int n, const AudioIODeviceCallbackContext&)
{
    for (int c = 0; c < numOut; ++c) if (out[c] != nullptr) FloatVectorOperations::clear (out[c], n);
    if (numIn > 0 && in[0] != nullptr) proc.choir.feed (in[0], numIn > 1 ? in[1] : nullptr, n, micRate.load());
}

bool ChoirPage::toggleRecord()
{
    if (proc.choir.recording())
    {
        stopMic(); const bool ok = proc.choir.recordStop();
        if (ok) { cur = proc.choir.get(); syncControls(); }
        recBtn.setButtonText (T ("chRecord")); status.setText (ok ? voiceLine() : T ("chRecFail"), dontSendNotification);
        return ok;
    }
    proc.choir.recordStart();
    if (! proc.captureFromInput.load())   // DAW girişi bağlı değil: bilgisayarın mikrofonunu doğrudan aç
    {
        mic = std::make_unique<AudioDeviceManager>();
        const auto err = mic->initialise (1, 0, nullptr, true);
        if (err.isNotEmpty() || mic->getCurrentAudioDevice() == nullptr) { mic.reset(); proc.choir.recordStop(); status.setText (T ("chNoMic"), dontSendNotification); return false; }
        mic->addAudioCallback (this);
    }
    recBtn.setButtonText (T ("chStop")); status.setText (T ("chRecording"), dontSendNotification);
    return true;
}

bool ChoirPage::openFile (const File& f)
{
    const bool ok = proc.choir.loadFile (f);
    if (ok) { cur = proc.choir.get(); syncControls(); }
    status.setText (ok ? voiceLine() : T ("lfLoadFail"), dontSendNotification);
    return ok;
}

void ChoirPage::tick (float level)
{
    const auto e = proc.choir.get();
    const bool combos = e.source != cur.source || e.rhythm != cur.rhythm || e.size != cur.size || e.part != cur.part;
    cur = e; if (combos) syncControls();
    if (proc.choir.recording())
    {
        const double sec = proc.choir.recSeconds();
        status.setText (String::charToString ((juce_wchar) 0x25CF) + "  " + String (sec, 1) + " s  " + String::repeatedString ("|", jlimit (0, 24, (int) (proc.choir.recLevel() * 30))), dontSendNotification);
        if (sec >= ch::kMaxVoiceSeconds - .1) toggleRecord();
    }
    const int v = proc.choir.version();
    if (v != shownVersion) { shownVersion = v; proc.choir.peaks (pk, 400); repaint(); }
    // ---- sahne: koro çalıyorsa melekler kendi partilerini söyler, çalmıyorsa müziğin seviyesine eşlik eder
    const bool motion = proc.animations, eco = (int) proc.apvts.getRawParameterValue ("quality")->load() == 0;
    ++frame; clock += 1.0 / 30.0; lvl += (jlimit (0.0f, 1.0f, level) - lvl) * .35f;
    auto& q = stage.pose; const bool playing = proc.choir.isPlaying();
    for (int i = 0; i < 4; ++i) { const float a = playing ? proc.choir.activity (i) : lvl * .55f; smooth[i] += (a - smooth[i]) * (a > smooth[i] ? .4f : .15f); }
    if (! motion) { if (! stage.dirty) return; q = {}; }
    else
    {
        if (eco && (frame & 1)) return;
        const double ppq = proc.playPpq.load(); q.t = (float) clock; q.beat = (float) (ppq - std::floor (ppq)); q.level = lvl;
        for (int i = 0; i < 4; ++i) q.voice[(size_t) i] = smooth[i];
    }
    q.on = cur.part; stage.dirty = false;
    const int w = stage.getWidth(), h = stage.getHeight(); if (w < 8 || h < 8) return;
    if (stage.img.getWidth() != w || stage.img.getHeight() != h) stage.img = Image (Image::ARGB, w, h, true);
    stage.stage.render (stage.img, q, eco ? 1 : 2);
    stage.repaint();
}

void ChoirPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    g.setColour (P.panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
    // sesin dalga biçimi (kayıt ya da yüklenen dosya)
    const Rectangle<float> w (16, 392, 420, 40);
    g.setColour (P.panel2); g.fillRoundedRectangle (w, 6.0f);
    if (proc.choir.hasVoice() && ! pk.empty())
    {
        float mx = 1e-4f; for (auto v : pk) mx = jmax (mx, v);
        g.setColour (cur.source >= ch::sOwnHold ? P.accent : P.mute);
        for (int i = 0; i < (int) pk.size(); ++i) { const float x = w.getX() + 4 + (float) i * (w.getWidth() - 8) / (float) pk.size(), y = pk[(size_t) i] / mx * (w.getHeight() * .5f - 4); g.drawVerticalLine ((int) x, w.getCentreY() - y, w.getCentreY() + y + 1); }
    }
    else { g.setColour (P.mute); g.setFont (lnf.font (12)); g.drawText (T ("chDrop"), w, Justification::centred); }
    g.setColour (P.mute); g.setFont (lnf.font (12.5f));
    g.drawFittedText (T ("chHint"), Rectangle<int> (16, 588, kW - 32, 54), Justification::topLeft, 3);
}

void ChoirPage::resized()
{
    int x = 16; auto put = [&] (Component& c, int w) { c.setBounds (x, 12, w, 30); x += w + 8; };
    put (recBtn, 110); put (loadBtn, 104); put (playBtn, 84); put (sourceBox, 180); put (rhythmBox, 140); put (sizeBox, 110); put (presetBox, 150);
    exportBtn.setBounds (kW - 16 - 190, 12, 190, 30);
    stage.setBounds (16, 52, kW - 32, 330);
    status.setBounds (448, 392, kW - 464, 40);
    for (int i = 0; i < 7; ++i) knobs[(size_t) i].setBounds (16 + i * 162, 446, 150, 128);
}

// ---- sahne
void ChoirPage::StageView::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); const auto r = getLocalBounds().toFloat(); const float w = r.getWidth(), h = r.getHeight(), lv = pose.level;
    Path clip; clip.addRoundedRectangle (r, 10.0f); g.reduceClipRegion (clip);
    g.setGradientFill (ColourGradient (P.panel2.interpolatedWith (P.accent, .16f), 0, 0, P.panel2.darker (.18f), 0, h, false)); g.fillRect (r);
    const auto eb = stage.bounds (4); const Point<float> top (eb.getCentreX() * w, 0);
    {   // gökten inen ışık: bilgenin arkasında, seviyeyle parlar
        const Point<float> c (eb.getCentreX() * w, (eb.getY() + eb.getHeight() * .25f) * h);
        g.setGradientFill (ColourGradient (P.accent.withAlpha (.2f + .3f * lv), c.x, c.y, P.accent.withAlpha (0.0f), c.x + h * 1.25f, c.y, true)); g.fillRect (r);
        for (int i = 0; i < 9; ++i)
        {
            const float a = ((float) i - 4) * .2f + .05f * std::sin (pose.t * .3f + (float) i), half = .035f + .012f * std::sin (pose.t * .7f + (float) i * 2);
            Path ray; ray.startNewSubPath (top); ray.lineTo (top.x + std::tan (a - half) * h * 1.2f, h * 1.2f); ray.lineTo (top.x + std::tan (a + half) * h * 1.2f, h * 1.2f); ray.closeSubPath();
            g.setColour (P.accent.withAlpha ((.035f + .06f * lv) * (i % 2 == 0 ? 1.0f : .6f))); g.fillPath (ray);
        }
    }
    for (int i = 0; i < 5; ++i)   // bulutlar: figürlerin ayak ucunda
    {
        const auto b = stage.bounds (i); if (b.isEmpty()) continue;
        const float cx = b.getCentreX() * w, cy = jmin (h - 10.0f, b.getBottom() * h - 4), cw = (i == 4 ? .3f : .2f) * h * 2.2f;
        for (int k = -1; k <= 1; ++k) { g.setColour (Colours::white.withAlpha (P.bg.getBrightness() > .5f ? .5f : .1f)); g.fillEllipse (cx - cw * .5f + (float) k * cw * .3f, cy - (k == 0 ? 16.0f : 9.0f), cw, k == 0 ? 34.0f : 24.0f); }
    }
    auto bloom = [&] (float k)
    {
        for (const auto& gl : stage.glows())
        {
            const float x = gl.x * w, y = gl.y * h, rad = gl.r * h;
            g.setGradientFill (ColourGradient (Colour (0xffffe9a8).withAlpha (jlimit (0.0f, 1.0f, gl.a * k)), x, y, Colour (0x00ffe9a8), x + rad, y, true)); g.fillEllipse (x - rad, y - rad, rad * 2, rad * 2);
        }
    };
    bloom (1.0f);
    if (img.isValid()) g.drawImageAt (img, 0, 0);
    bloom (.35f);
    g.setFont (o.lnf.font (12.5f, true));
    for (int i = 0; i < 4; ++i)
    {
        const auto b = stage.bounds (i); const bool on = o.cur.part[(size_t) i];
        const Rectangle<float> tag (b.getCentreX() * w - 46, h - 26, 92, 20);
        g.setColour ((on ? P.accent : P.panel).withAlpha (on ? .92f : .7f)); g.fillRoundedRectangle (tag, 10.0f);
        g.setColour (on ? P.accentInk : P.mute); g.drawText (o.T (kPartKeys[i]), tag, Justification::centred);
    }
}

void ChoirPage::StageView::mouseDown (const MouseEvent& e)
{
    int i = stage.hit ((float) e.x / (float) jmax (1, getWidth()), (float) e.y / (float) jmax (1, getHeight()));
    if (i < 0 && e.y > getHeight() - 30) for (int k = 0; k < 4; ++k) if (std::abs (stage.bounds (k).getCentreX() * (float) getWidth() - (float) e.x) < 46) i = k;   // etikete tıklama
    if (i >= 0) o.togglePart (i);
}

void ChoirPage::StageView::mouseMove (const MouseEvent& e)
{
    const int i = stage.hit ((float) e.x / (float) jmax (1, getWidth()), (float) e.y / (float) jmax (1, getHeight()));
    setMouseCursor (i >= 0 || e.y > getHeight() - 30 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}

void ChoirPage::DragHandle::mouseDrag (const MouseEvent& e)
{
    if (e.getDistanceFromDragStart() < 8 || busy) return;
    const ScopedValueSetter<bool> guard (busy, true);
    const auto f = o.proc.choir.exportWav();
    if (! f.existsAsFile()) return;
    DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}
