#include "DescribePanel.h"

using namespace juce;

namespace
{
const char* const kNotes[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const int kBars[4] = { 1, 2, 4, 8 };
}

String DescribePanel::T (const char* tr, const char* en) const { return lang == "tr" ? String::fromUTF8 (tr) : String (en); }

DescribePanel::DescribePanel (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l)
{
    setWantsKeyboardFocus (false);
    for (auto* lb : { &promptLbl, &bpmLbl, &keyLbl, &lenLbl }) { addAndMakeVisible (*lb); lb->setFont (FontOptions (13.0f, Font::bold)); lb->setColour (Label::textColourId, lnf.pal().ink); }
    promptLbl.attachToComponent (&prompt, false); bpmLbl.attachToComponent (&bpm, false); keyLbl.attachToComponent (&keyBox, false); lenLbl.attachToComponent (&lenBox, false);
    addAndMakeVisible (prompt); prompt.setMultiLine (true, true); prompt.setReturnKeyStartsNewLine (false); prompt.setFont (FontOptions (16.0f));
    prompt.setIndents (10, 8); prompt.onReturnKey = [this] { generate (prompt.getText()); };
    prompt.setColour (TextEditor::backgroundColourId, lnf.pal().panel2); prompt.setColour (TextEditor::textColourId, lnf.pal().ink);
    prompt.setColour (TextEditor::outlineColourId, lnf.pal().line); prompt.setColour (TextEditor::focusedOutlineColourId, lnf.pal().focus);
    addAndMakeVisible (bpm); bpm.setRange (60, 190, 1); bpm.setTextBoxStyle (Slider::TextBoxLeft, false, 56, 32); bpm.setIncDecButtonsMode (Slider::incDecButtonsDraggable_Vertical);
    bpm.onValueChange = [this] { if (! building && haveDesign) { D.bpm = bpm.getValue(); dirtyAt = Time::currentTimeMillis(); } };
    addAndMakeVisible (keyBox); for (int m = 0; m < 2; ++m) for (int k = 0; k < 12; ++k) keyBox.addItem (String (kNotes[k]) + (m == 0 ? " m" : ""), 1 + m * 12 + k);
    keyBox.onChange = [this] { if (! building && haveDesign) { const int id = keyBox.getSelectedId() - 1; D.key = id % 12; D.minor = id < 12; dirtyAt = Time::currentTimeMillis(); } };
    addAndMakeVisible (lenBox);
    lenBox.onChange = [this] { if (! building && haveDesign) { D.bars = kBars[jlimit (0, 3, lenBox.getSelectedItemIndex())]; dirtyAt = Time::currentTimeMillis(); } };
    for (auto* b : { &genBtn, &regenBtn, &playBtn, &exportBtn, &editorBtn }) addAndMakeVisible (*b);
    genBtn.setComponentID ("primary");
    genBtn.onClick = [this] { generate (prompt.getText()); };
    regenBtn.onClick = [this] { regenerate(); };
    playBtn.onClick = [this] { togglePlay(); };
    exportBtn.onClick = [this] { startExport(); };
    editorBtn.onClick = [this] { if (onOpenEditor) onOpenEditor(); };
    for (auto* b : { &lfoBtn, &arpBtn, &rateBtn, &advBtn }) addAndMakeVisible (*b);
    lfoBtn.onClick = [this] { setLfo (! D.lfo); };
    arpBtn.onClick = [this] { setArp (! D.arp); };
    rateBtn.onClick = [this] { cycleRate (ModifierKeys::currentModifiers.isShiftDown() || ModifierKeys::currentModifiers.isRightButtonDown() ? -1 : 1); };
    advBtn.onClick = [this] { setAdvancedOpen (! advOpen); };
    advBtn.showDot = false;
    addAndMakeVisible (wave); wave.setAccessible (true);
    addAndMakeVisible (chips); chips.setAccessible (true);
    addAndMakeVisible (status); status.setColour (Label::textColourId, lnf.pal().mute); status.setFont (FontOptions (13.0f));
    for (int i = 0; i < (int) adv.size(); ++i)
    {
        auto& k = adv[(size_t) i]; k = std::make_unique<hgui::Knob>(); addChildComponent (*k);
        auto& s = k->slider; s.setColour (Slider::rotarySliderFillColourId, lnf.pal().accent);
        if (i == 0) { s.setRange (60, 16000, 1); s.setSkewFactorFromMidPoint (900); s.textFromValueFunction = [] (double v) { return v >= 1000 ? String (v / 1000, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }; }
        else if (i == 2) { s.setRange (.0005, 2, 0); s.setSkewFactorFromMidPoint (.08); s.textFromValueFunction = [] (double v) { return v < 1 ? String (roundToInt (v * 1000)) + " ms" : String (v, 2) + " s"; }; }
        else if (i == 3) { s.setRange (.02, 4, 0); s.setSkewFactorFromMidPoint (.4); s.textFromValueFunction = [] (double v) { return v < 1 ? String (roundToInt (v * 1000)) + " ms" : String (v, 2) + " s"; }; }
        else { s.setRange (0, i == 1 ? .9 : 1, 0); s.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; }; }
        s.updateText();
        s.onValueChange = [this, i] { if (! building) setAdvanced (i, adv[(size_t) i]->slider.getValue()); };
    }
    // sekme sırası: tarif, tempo, ton, süre, üret, yeniden üret, dinle, WAV, LFO, arpej, hız, gelişmiş, düğmeler
    int order = 1;
    for (Component* c : std::initializer_list<Component*> { &prompt, &bpm, &keyBox, &lenBox, &genBtn, &regenBtn, &playBtn, &exportBtn, &lfoBtn, &arpBtn, &rateBtn, &advBtn })
        c->setExplicitFocusOrder (order++);
    for (auto& k : adv) k->slider.setExplicitFocusOrder (order++);
    editorBtn.setExplicitFocusOrder (order++);
    setFocusContainerType (FocusContainerType::keyboardFocusContainer);
    refreshTexts(); syncControls();
    startTimerHz (20);
}

DescribePanel::~DescribePanel() { proc.clip.stop(); }

void DescribePanel::setLanguage (const String& l) { lang = l; refreshTexts(); if (haveDesign) { auto d = sp::design (lastText, context(), D.seed); D.heard = d.heard; D.filled = d.filled; } syncControls(); repaint(); }

double DescribePanel::sampleRate() const { const double r = proc.getSampleRate(); return r > 0 ? r : 48000.0; }

sp::Context DescribePanel::context() const
{
    sp::Context c; c.lang = lang; c.part = curPart;
    const double hb = proc.hostBpm.load(); c.bpm = hb > 0 ? hb : (double) proc.apvts.getRawParameterValue ("bpm")->load();
    c.key = jlimit (0, 11, (int) proc.apvts.getRawParameterValue ("key")->load());
    const int sc = (int) proc.apvts.getRawParameterValue ("scale")->load();
    String nm = sc > 0 && sc <= proc.data.scaleOrder.size() ? proc.data.scaleOrder[sc - 1] : String();
    if (nm.isEmpty()) { const auto st = proc.data.styleById (proc.data.styleId (jlimit (0, proc.data.numStyles() - 1, (int) proc.apvts.getRawParameterValue ("style")->load()))); if (auto* a = st["scales"].getArray()) if (a->size() > 0) nm = (*a)[0].toString(); }
    c.minor = ! (nm == "major" || nm == "mixolydian");
    return c;
}

void DescribePanel::refreshTexts()
{
    promptLbl.setText (T ("Sesi tarif et", "Describe the sound"), dontSendNotification);
    prompt.setTextToShowWhenEmpty (T ("Örn. sıcak, derin Afro House bas; vurucu, hafif distortion, ritmik LFO. Ya da sadece: karanlık bas",
                                      "E.g. warm, deep Afro House bass; punchy, light distortion, rhythmic LFO. Or just: dark bass"), lnf.pal().mute);
    prompt.setTitle (T ("Ses tarifi", "Sound description"));
    prompt.setDescription (T ("Enter: üret. Shift+Enter: yeni satır. Yazmadığınız ayrıntılar otomatik tamamlanır.", "Enter: generate. Shift+Enter: new line. Anything you leave out is filled in automatically."));
    bpmLbl.setText ("Tempo (BPM)", dontSendNotification); bpm.setTitle ("Tempo (BPM)");
    keyLbl.setText (T ("Ton", "Key"), dontSendNotification); keyBox.setTitle (T ("Ton", "Key"));
    lenLbl.setText (T ("Süre", "Length"), dontSendNotification); lenBox.setTitle (T ("Süre", "Length"));
    const int li = lenBox.getSelectedItemIndex(); lenBox.clear (dontSendNotification);
    for (int i = 0; i < 4; ++i) lenBox.addItem (String (kBars[i]) + (lang == "tr" ? String::fromUTF8 (" ölçü") : String (kBars[i] == 1 ? " bar" : " bars")), i + 1);
    lenBox.setSelectedItemIndex (li >= 0 ? li : 2, dontSendNotification);
    for (int m = 0; m < 2; ++m) for (int k = 0; k < 12; ++k) keyBox.changeItemText (1 + m * 12 + k, String (kNotes[k]) + " " + (m == 0 ? T ("minör", "minor") : T ("majör", "major")));
    genBtn.setButtonText (T ("Üret", "Generate")); genBtn.setTooltip (T ("Tarife göre sesi üret (Enter ya da Ctrl+Enter)", "Generate the sound from the description (Enter or Ctrl+Enter)"));
    regenBtn.setButtonText (T ("Yeniden üret", "Regenerate")); regenBtn.setTooltip (T ("Aynı tariften yeni bir çeşit (Ctrl+R)", "A new variation of the same description (Ctrl+R)"));
    exportBtn.setButtonText (T ("WAV indir", "Download WAV")); exportBtn.setTooltip (T ("Ön dinlemeyi 24 bit WAV olarak kaydet (Ctrl+S)", "Save the preview as a 24-bit WAV (Ctrl+S)"));
    editorBtn.setButtonText (T ("Tam düzenleyici (Synth X)", "Full editor (Synth X)")); editorBtn.setTooltip (T ("Tüm osilatör, filtre, zarf ve modülasyon ayarları", "Every oscillator, filter, envelope and modulation setting"));
    lfoBtn.name = "LFO"; lfoBtn.setTooltip (T ("Ritmik hareket: açınca türe uygun LFO ayarlanır (Alt+L)", "Rhythmic movement: turning it on sets an LFO that suits the sound (Alt+L)"));
    arpBtn.name = T ("Arpej", "Arpeggio"); arpBtn.setTooltip (T ("Notaları sırayla çalar; türe uygun kip ve oktav (Alt+A)", "Plays notes one after another with a mode and range that suit the sound (Alt+A)"));
    rateBtn.name = T ("Hız", "Rate"); rateBtn.setTooltip (T ("Tıklayın: BPM'e senkron bir sonraki hız. Shift+tık ya da sol ok: önceki (Alt+R)", "Click: next tempo-synced rate. Shift+click or Left arrow: previous (Alt+R)"));
    advBtn.name = T ("Gelişmiş ayarlar", "Advanced settings");
    const char* const names[7][2] = { { "Parlaklık", "Brightness" }, { "Rezonans", "Resonance" }, { "Atak", "Attack" }, { "Bırakma", "Release" }, { "Distortion", "Distortion" }, { "Reverb", "Reverb" }, { "Sidechain", "Sidechain" } };
    const char* const helps[7][2] = { { "Filtre kesimi: koyu - parlak", "Filter cutoff: dark to bright" }, { "Filtrenin kesim noktasındaki vurgu", "Emphasis at the filter cutoff" },
                                      { "Sesin ne kadar hızlı başladığı", "How fast the sound starts" }, { "Tuş bırakıldıktan sonra sönme süresi", "Fade time after the key is released" },
                                      { "Analog sürüş miktarı", "Amount of analog drive" }, { "Alan (yankı) miktarı", "Amount of space (reverb)" }, { "Kick ile kısılma (kick'e yer açar)", "Ducking with the kick (makes room for it)" } };
    for (int i = 0; i < 7; ++i) adv[(size_t) i]->setText (T (names[i][0], names[i][1]), T (helps[i][0], helps[i][1]));
    wave.setTitle (T ("Ön dinleme dalga biçimi", "Preview waveform")); wave.setDescription (T ("Tıklayın: dinle / durdur", "Click to play or stop"));
    syncControls();
}

void DescribePanel::syncControls()
{
    const ScopedValueSetter<bool> b (building, true);
    const auto c = context();
    bpm.setValue (haveDesign ? D.bpm : c.bpm, dontSendNotification);
    keyBox.setSelectedId (1 + ((haveDesign ? D.minor : c.minor) ? 0 : 12) + (haveDesign ? D.key : c.key), dontSendNotification);
    if (haveDesign) for (int i = 0; i < 4; ++i) if (kBars[i] == D.bars) lenBox.setSelectedItemIndex (i, dontSendNotification);
    const String onT = T ("Açık", "On"), offT = T ("Kapalı", "Off");
    lfoBtn.on = D.lfo; lfoBtn.state = D.lfo ? onT : offT; arpBtn.on = D.arp; arpBtn.state = D.arp ? onT : offT;
    rateBtn.on = D.lfo || D.arp; rateBtn.state = sp::rateName (D.rate);
    advBtn.on = advOpen; advBtn.state = advOpen ? String::fromUTF8 ("▾") : String::fromUTF8 ("▸");
    for (auto* x : { &lfoBtn, &arpBtn, &rateBtn, &advBtn })
    {
        x->setButtonText (x->name + ": " + x->state);
        x->setToggleable (x != &rateBtn); x->setToggleState (x->on, dontSendNotification);
        x->setTitle (x->name); x->setDescription (x->state);
    }
    rateBtn.setDescription (sp::rateName (D.rate) + (rateBtn.on ? String() : T (" (LFO ya da arpej açıkken etkili)", " (applies when LFO or arpeggio is on)")));
    playBtn.setButtonText (proc.clip.isPlaying() ? T ("Durdur", "Stop") : T ("Dinle", "Play")); playBtn.setTooltip (T ("Ön dinleme (Boşluk)", "Preview (Space)"));
    for (auto* x : { &regenBtn, &playBtn, &exportBtn }) x->setEnabled (haveDesign && (x == &regenBtn || clipBuf != nullptr));
    for (auto* x : { &lfoBtn, &arpBtn, &rateBtn }) x->setEnabled (haveDesign);
    if (haveDesign && ! program.isVoid())
    {
        auto num = [&] (const var& v, double d) { return v.isDouble() || v.isInt() ? (double) v : d; };
        adv[0]->slider.setValue (num (program["o"][0]["f"]["c"], 1000), dontSendNotification);
        adv[1]->slider.setValue (num (program["o"][0]["f"]["r"], .15), dontSendNotification);
        adv[2]->slider.setValue (num (program["env"][0][0], .003), dontSendNotification);
        adv[3]->slider.setValue (num (program["env"][0][3], .2), dontSendNotification);
        adv[4]->slider.setValue (D.dirt, dontSendNotification); adv[5]->slider.setValue (D.space, dontSendNotification); adv[6]->slider.setValue (D.side, dontSendNotification);
    }
    for (auto& k : adv) k->setVisible (advOpen); editorBtn.setVisible (advOpen);
    String said;
    if (haveDesign) said << T ("Anlaşılan: ", "Understood: ") << D.heard.joinIntoString (", ") << (D.filled.isEmpty() ? String() : ". " + T ("Otomatik: ", "Filled in: ") + D.filled.joinIntoString (", "));
    chips.setTitle (haveDesign ? said : T ("Henüz ses yok", "No sound yet"));
    chips.repaint(); repaint();
}

void DescribePanel::generate (const String& text)
{
    lastText = text.trim();
    if (prompt.getText() != text) prompt.setText (text, false);
    const auto c = context();
    D = sp::design (lastText, c, 1); haveDesign = true;
    applyDesign (true);
}

void DescribePanel::regenerate()
{
    if (! haveDesign) { generate (prompt.getText()); return; }
    const auto keep = D; const uint32_t seed = D.seed + 1;
    D = sp::design (lastText, context(), seed);
    // kullanıcının seçtikleri kalır: tempo, ton, süre, LFO, arpej, hız
    D.bpm = keep.bpm; D.key = keep.key; D.minor = keep.minor; D.bars = keep.bars; D.lfo = keep.lfo; D.arp = keep.arp; D.rate = keep.rate;
    if (keep.lfo && ! D.lfo) D.lfo = true;
    applyDesign (true);
}

void DescribePanel::applyDesign (bool announce)
{
    program = sp::build (D, sampleRate());
    proc.setXsProgram (D.part, program);
    if (D.part != curPart) { curPart = D.part; if (onPartChange) onPartChange (D.part); }
    rerender();
    status.setText (T ("Yüklendi: ", "Loaded to: ") + sp::kindName (D.kind, lang) + " " + T ("partisi", "part") + " - " + sp::keyName (D.key, D.minor, lang) + ", " + String (roundToInt (D.bpm)) + " BPM. "
                    + T ("Boşluk: dinle, Ctrl+R: yeniden üret, Ctrl+S: WAV.", "Space: play, Ctrl+R: regenerate, Ctrl+S: WAV."), dontSendNotification);
    syncControls();
    if (announce)
        if (auto* h = getAccessibilityHandler()) h->postAnnouncement (status.getText(), AccessibilityHandler::AnnouncementPriority::medium);
}

void DescribePanel::rerender()
{
    const bool wasPlaying = proc.clip.isPlaying();
    clipRate = sampleRate();
    clipBuf = sp::render (D, program, proc.data, clipRate);
    peaks.assign (240, 0.0f);
    if (clipBuf != nullptr)
    {
        const int n = clipBuf->getNumSamples();
        for (int i = 0; i < 240; ++i) { const int a = (int) ((int64) n * i / 240), z = (int) ((int64) n * (i + 1) / 240); if (z > a) peaks[(size_t) i] = jmax (clipBuf->getMagnitude (0, a, z - a), clipBuf->getMagnitude (1, a, z - a)); }
        if (wasPlaying) proc.clip.play (std::make_unique<AudioBuffer<float>> (*clipBuf), true);
    }
    dirtyAt = 0; wave.repaint();
}

void DescribePanel::togglePlay()
{
    if (clipBuf == nullptr) return;
    if (proc.clip.isPlaying()) proc.clip.stop();
    else { if (proc.demoPlaying()) proc.stopDemo(); proc.clip.play (std::make_unique<AudioBuffer<float>> (*clipBuf), true); }
    syncControls();
}

void DescribePanel::setLfo (bool on)
{
    if (! haveDesign) return;
    D.lfo = on; if (on && ! D.arp) D.rate = D.wobble ? 3 : sp::defaultRate (D.kind); if (! on) D.wobble = false;
    applyDesign (false);
}
void DescribePanel::setArp (bool on)
{
    if (! haveDesign) return;
    D.arp = on; if (on && ! D.lfo) D.rate = sp::defaultRate (D.kind);
    applyDesign (false);
}
void DescribePanel::cycleRate (int dir)
{
    if (! haveDesign) return;
    D.rate = (D.rate + dir + sp::kNumRates) % sp::kNumRates;
    applyDesign (false);
    if (auto* h = rateBtn.getAccessibilityHandler()) h->postAnnouncement (rateBtn.name + " " + sp::rateName (D.rate), AccessibilityHandler::AnnouncementPriority::medium);
}

void DescribePanel::setAdvancedOpen (bool open) { advOpen = open; syncControls(); resized(); }

void DescribePanel::setAdvanced (int which, double v)
{
    if (! haveDesign) return;
    switch (which)
    {
        case 0: D.advCut = (float) v; break; case 1: D.advRes = (float) v; break; case 2: D.advAtk = (float) v; break; case 3: D.advRel = (float) v; break;
        case 4: D.dirt = (float) v; break; case 5: D.space = (float) v; break; default: D.side = (float) v; break;
    }
    program = sp::build (D, sampleRate()); proc.setXsProgram (D.part, program);
    dirtyAt = Time::currentTimeMillis();
}

void DescribePanel::timerCallback()
{
    if (dirtyAt != 0 && Time::currentTimeMillis() - dirtyAt > 220)
    {
        dirtyAt = 0;
        if (haveDesign) { program = sp::build (D, sampleRate()); proc.setXsProgram (D.part, program); rerender(); syncControls(); }
    }
    const bool pl = proc.clip.isPlaying();
    if (playBtn.getButtonText() != (pl ? T ("Durdur", "Stop") : T ("Dinle", "Play"))) syncControls();
    if (pl) wave.repaint();
}

void DescribePanel::visibilityChanged() { if (! isShowing()) proc.clip.stop(); }

File DescribePanel::exportTo (const File& f)
{
    if (clipBuf == nullptr) return {};
    return sp::writeWav (*clipBuf, clipRate, f) ? f : File();
}

void DescribePanel::startExport()
{
    if (clipBuf == nullptr) return;
    String nm = "HitGen " + D.title + " " + String (kNotes[D.key]).replace ("#", "s") + (D.minor ? "m" : "") + " " + String (roundToInt (D.bpm)) + "bpm.wav";
    const auto dir = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Wavejackers HitGen").getChildFile ("Sounds");
    dir.createDirectory();
    chooser = std::make_unique<FileChooser> (T ("WAV olarak kaydet", "Save as WAV"), dir.getChildFile (File::createLegalFileName (nm)), "*.wav");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult(); if (f == File()) return;
        if (! f.hasFileExtension ("wav")) f = f.withFileExtension ("wav");
        const bool ok = exportTo (f) != File();
        status.setText (ok ? T ("Kaydedildi: ", "Saved: ") + f.getFullPathName() : T ("Kaydedilemedi: klasöre yazma izni yok ya da disk dolu.", "Could not save: no write permission or the disk is full."), dontSendNotification);
        if (auto* h = getAccessibilityHandler()) h->postAnnouncement (status.getText(), AccessibilityHandler::AnnouncementPriority::medium);
    });
}

bool DescribePanel::keyPressed (const KeyPress& k)
{
    const auto m = k.getModifiers(); const bool cmd = m.isCommandDown() || m.isCtrlDown();
    if (cmd && k.getKeyCode() == KeyPress::returnKey) { generate (prompt.getText()); return true; }
    if (cmd && (k.getKeyCode() == 'R' || k.getKeyCode() == 'r')) { regenerate(); return true; }
    if (cmd && (k.getKeyCode() == 'S' || k.getKeyCode() == 's')) { startExport(); return true; }
    if (m.isAltDown() && (k.getKeyCode() == 'L' || k.getKeyCode() == 'l')) { setLfo (! D.lfo); return true; }
    if (m.isAltDown() && (k.getKeyCode() == 'A' || k.getKeyCode() == 'a')) { setArp (! D.arp); return true; }
    if (m.isAltDown() && (k.getKeyCode() == 'R' || k.getKeyCode() == 'r')) { cycleRate (m.isShiftDown() ? -1 : 1); return true; }
    if (k.getKeyCode() == KeyPress::spaceKey && ! prompt.hasKeyboardFocus (true)) { togglePlay(); return true; }
    return false;
}

bool DescribePanel::StateButton::keyPressed (const KeyPress& k)
{
    if (this == &o.rateBtn && (k.getKeyCode() == KeyPress::rightKey || k.getKeyCode() == KeyPress::upKey)) { o.cycleRate (1); return true; }
    if (this == &o.rateBtn && (k.getKeyCode() == KeyPress::leftKey || k.getKeyCode() == KeyPress::downKey)) { o.cycleRate (-1); return true; }
    return TextButton::keyPressed (k);
}

void DescribePanel::StateButton::paintButton (Graphics& g, bool over, bool down)
{
    const auto& P = o.lnf.pal(); auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool en = isEnabled();
    g.setColour ((on ? P.accent.withAlpha (.18f) : P.panel2).withMultipliedAlpha (en ? 1.0f : .5f)); g.fillRoundedRectangle (r, 10);
    g.setColour ((on ? P.accent : over ? P.ink.withAlpha (.6f) : P.line).withMultipliedAlpha (en ? 1.0f : .5f)); g.drawRoundedRectangle (r, 10, on ? 2.0f : 1.2f);
    if (down) { g.setColour (P.ink.withAlpha (.08f)); g.fillRoundedRectangle (r, 10); }
    auto t = r.reduced (14, 0);
    if (showDot)
    {   // durum noktası: açıkken dolu, kapalıyken boş halka (renge ek olarak biçim farkı)
        auto d = t.removeFromLeft (14).withSizeKeepingCentre (12, 12);
        if (on) { g.setColour (P.accent); g.fillEllipse (d); } else { g.setColour (P.mute); g.drawEllipse (d.reduced (1), 1.6f); }
        t.removeFromLeft (10);
    }
    g.setColour (P.ink.withMultipliedAlpha (en ? 1.0f : .5f)); g.setFont (FontOptions (15.0f, Font::bold));
    g.drawText (name, t, Justification::centredLeft, true);
    g.setColour ((on ? P.ink : P.mute).withMultipliedAlpha (en ? 1.0f : .5f)); g.setFont (FontOptions (15.0f));
    g.drawText (state, t, Justification::centredRight, true);
}

void DescribePanel::Wave::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); auto r = getLocalBounds().toFloat();
    g.setColour (P.panel2); g.fillRoundedRectangle (r, 10); g.setColour (P.line); g.drawRoundedRectangle (r.reduced (.5f), 10, 1);
    if (o.peaks.empty() || o.clipBuf == nullptr)
    {
        g.setColour (P.mute); g.setFont (FontOptions (14.0f));
        g.drawText (o.T ("Tarifinizi yazıp Üret'e basın; ses burada görünür.", "Type a description and press Generate; the sound appears here."), r, Justification::centred);
        return;
    }
    const float mid = r.getCentreY(), h = r.getHeight() * .42f, w = r.getWidth() / (float) o.peaks.size();
    const float pos = o.proc.clip.isPlaying() ? o.proc.clip.position() : -1.0f;
    for (size_t i = 0; i < o.peaks.size(); ++i)
    {
        const float x = r.getX() + i * w, a = jmin (1.0f, o.peaks[i]) * h;
        g.setColour ((pos >= 0 && (float) i / o.peaks.size() <= pos ? P.accent : P.ink.withAlpha (.55f)));
        g.fillRect (x + .5f, mid - a, jmax (1.0f, w - 1.0f), jmax (1.0f, 2 * a));
    }
    if (pos >= 0) { g.setColour (P.focus); g.fillRect (r.getX() + pos * r.getWidth() - 1, r.getY() + 4, 2.0f, r.getHeight() - 8); }
}

void DescribePanel::Chips::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); auto r = getLocalBounds().toFloat();
    if (! o.haveDesign) return;
    g.setFont (FontOptions (13.0f, Font::bold));
    float x = r.getX(), y = r.getY() + 2; const float h = 22;
    auto chip = [&] (const String& s, bool auto_)
    {
        const float w = GlyphArrangement::getStringWidth (g.getCurrentFont(), s) + 20;
        if (x + w > r.getRight()) { x = r.getX(); y += h + 4; }
        if (y + h > r.getBottom() + 1) return;
        const Rectangle<float> c (x, y, w, h);
        g.setColour (auto_ ? P.panel2 : P.accent.withAlpha (.16f)); g.fillRoundedRectangle (c, h / 2);
        g.setColour (auto_ ? P.line : P.accent.withAlpha (.7f)); g.drawRoundedRectangle (c.reduced (.5f), h / 2, 1);
        g.setColour (auto_ ? P.mute : P.ink); g.drawText (s, c, Justification::centred);
        x += w + 6;
    };
    for (auto& s : o.D.heard) chip (s, false);
    if (! o.D.filled.isEmpty()) { x += 6; for (auto& s : o.D.filled) chip ("+ " + s, true); }
}

void DescribePanel::paint (Graphics& g)
{
    if (advOpen)
    {
        const auto& P = lnf.pal(); auto r = Rectangle<float> (8, 340, (float) getWidth() - 16, 112);
        g.setColour (P.panel2.withAlpha (.6f)); g.fillRoundedRectangle (r, 12); g.setColour (P.line); g.drawRoundedRectangle (r, 12, 1);
    }
}

void DescribePanel::paintOverChildren (Graphics& g)
{
    // belirgin odak halkası: klavyeyle gezilen denetim her zaman görünür
    if (auto* f = Component::getCurrentlyFocusedComponent())
        if (f != this && isParentOf (f) && f->isShowing())
        {
            auto* top = f; while (top->getParentComponent() != nullptr && top->getParentComponent() != this && dynamic_cast<hgui::Knob*> (top->getParentComponent()) == nullptr && dynamic_cast<Slider*> (top->getParentComponent()) == nullptr) top = top->getParentComponent();
            if (auto* k = dynamic_cast<hgui::Knob*> (top->getParentComponent())) top = k;
            if (auto* s = dynamic_cast<Slider*> (top->getParentComponent())) top = s;
            const auto r = getLocalArea (top->getParentComponent(), top->getBounds()).toFloat().expanded (3);
            g.setColour (lnf.pal().focus); g.drawRoundedRectangle (r, 12, 2.5f);
        }
}

void DescribePanel::resized()
{
    prompt.setBounds (16, 30, 700, 92);
    bpm.setBounds (732, 30, 124, 32); keyBox.setBounds (866, 30, 140, 32); lenBox.setBounds (1016, 30, 116, 32);
    genBtn.setBounds (732, 72, 196, 50); regenBtn.setBounds (936, 72, 196, 50);
    chips.setBounds (16, 130, 1116, 52);
    playBtn.setBounds (16, 190, 130, 56); wave.setBounds (156, 190, 760, 56); exportBtn.setBounds (926, 190, 206, 56);
    lfoBtn.setBounds (16, 258, 200, 48); arpBtn.setBounds (226, 258, 210, 48); rateBtn.setBounds (446, 258, 180, 48); advBtn.setBounds (636, 258, 280, 48);
    status.setBounds (16, 312, 1116, 24);
    for (int i = 0; i < (int) adv.size(); ++i) adv[(size_t) i]->setBounds (20 + i * 116, 346, 108, 100);
    editorBtn.setBounds (850, 376, 280, 44);
}
