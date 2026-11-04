#include "DrumPage.h"

using namespace juce;

namespace
{
const char* const kKnobKeys[HitGenProcessor::kDmCtl] = { "dmLevel", "dmTune", "dmDecay", "dmPunch", "dmSoft", "dmMid", "dmHigh" };
// düğme yerleşimi (2 sütun x 4 satır): Punch Soft / Mid High / Tune Decay / Level
const int kSlot[HitGenProcessor::kDmCtl][2] = { { 0, 3 }, { 0, 2 }, { 1, 2 }, { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
}

DrumPage::DrumPage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data)
{
    setTitle ("Drum machine");
    for (auto* lb : { &title, &hint, &kitLabel }) { addAndMakeVisible (*lb); lb->setInterceptsMouseClicks (false, false); }
    title.setFont (lnf.font (17, true)); hint.setFont (lnf.font (12.5f)); kitLabel.setFont (lnf.font (12.5f));
    addAndMakeVisible (creativeBtn); creativeBa = std::make_unique<BA> (proc.apvts, "drumCreative", creativeBtn); hgui::tagParam (creativeBtn, "drumCreative");
    addAndMakeVisible (kitBox);
    kitBox.onChange = [this] { const int i = kitBox.getSelectedId() - 1; if (i >= 0 && i < kitIds.size() && kitIds[i] != proc.soundId (hg::kDrums)) { proc.setSound (hg::kDrums, kitIds[i], true); shownKit = kitIds[i]; if (onKitChanged) onKitChanged(); } };
    for (auto* b : { &saveKitBtn, &loadKitBtn, &resetBtn }) addAndMakeVisible (*b);
    saveKitBtn.onClick = [this] { saveKit(); }; loadKitBtn.onClick = [this] { loadKitMenu(); };
    resetBtn.onClick = [this] { for (int i = 0; i < hg::kNumPads; ++i) proc.resetDrumPad (i); refreshPads(); say (T ("dmResetDone")); };
    for (int i = 0; i < hg::kNumPads; ++i)
    {
        auto& pd = pads[(size_t) i]; pd.owner = this; pd.index = i; addAndMakeVisible (pd); pd.setWantsKeyboardFocus (true);
        auto& lb = loadBtns[(size_t) i]; addAndMakeVisible (lb); lb.onClick = [this, i] { chooseSample (i); };
        auto& cb = clearBtns[(size_t) i]; addAndMakeVisible (cb); cb.setButtonText (String (CharPointer_UTF8 ("\xc3\x97")));
        cb.onClick = [this, i] { proc.clearDrumSample (i); refreshPads(); say (T ("dmCleared").replace ("{p}", T (String ("dmPad_") + hg::kPadIds[i]))); };
        for (int k = 0; k < HitGenProcessor::kDmCtl; ++k)
        {
            auto& kn = knobs[(size_t) i][(size_t) k]; addAndMakeVisible (kn);
            kn.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 64, 15);
            const auto id = HitGenProcessor::dmParamId (i, k);
            sa.push_back (std::make_unique<SA> (proc.apvts, id, kn.slider)); hgui::tagParam (kn.slider, id);
            kn.slider.setDoubleClickReturnValue (true, proc.apvts.getParameterRange (id).convertFrom0to1 (proc.apvts.getParameter (id)->getDefaultValue()));
        }
    }
    fillKits(); refreshPads();
}

DrumPage::~DrumPage() { sa.clear(); creativeBa.reset(); }

Component& DrumPage::padComponent (int pad) { return pads[(size_t) jlimit (0, hg::kNumPads - 1, pad)]; }

void DrumPage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r;
    const auto just = rtl ? Justification::centredRight : Justification::centredLeft;
    title.setText (T ("dmTitle"), dontSendNotification); title.setJustificationType (just);
    kitLabel.setText (T ("dmKit"), dontSendNotification); kitLabel.setJustificationType (just);
    creativeBtn.setButtonText (T ("dmCreative")); creativeBtn.setTooltip (T ("dmCreativeHint")); creativeBtn.getProperties().set ("rtl", rtl);
    saveKitBtn.setButtonText (T ("dmSaveKit")); loadKitBtn.setButtonText (T ("dmLoadKit")); resetBtn.setButtonText (T ("dmReset"));
    saveKitBtn.setTooltip (T ("dmSaveKitHint")); loadKitBtn.setTooltip (T ("dmLoadKitHint")); resetBtn.setTooltip (T ("dmResetHint"));
    kitBox.setTitle (T ("dmKit")); kitBox.setTooltip (T ("dmKitHint"));
    for (int i = 0; i < hg::kNumPads; ++i)
    {
        const String pn = T (String ("dmPad_") + hg::kPadIds[i]);
        pads[(size_t) i].name = pn; pads[(size_t) i].setTitle (pn); pads[(size_t) i].setTooltip (T ("dmPadHint"));
        loadBtns[(size_t) i].setButtonText (T ("dmLoad")); loadBtns[(size_t) i].setTitle (T ("dmLoad") + ": " + pn); loadBtns[(size_t) i].setTooltip (T ("dmLoadHint"));
        clearBtns[(size_t) i].setTitle (T ("dmClear") + ": " + pn); clearBtns[(size_t) i].setTooltip (T ("dmClear"));
        for (int k = 0; k < HitGenProcessor::kDmCtl; ++k)
        {
            auto& kn = knobs[(size_t) i][(size_t) k];
            kn.setText (T (kKnobKeys[k]), pn + ": " + T (String (kKnobKeys[k]) + "Hint"));
            kn.slider.setTitle (T (kKnobKeys[k]) + " (" + pn + ")");
            kn.caption.setColour (Label::textColourId, lnf.pal().mute); kn.caption.setFont (lnf.font (11.5f));
        }
    }
    fillKits(); refreshPads(); resized(); repaint();
}

void DrumPage::fillKits()
{
    kitBox.clear (dontSendNotification); kitIds.clear();
    for (int k = 0; k < data.numKits(); ++k) { kitIds.add (data.kit (k)["id"].toString()); kitBox.addItem (data.soundName (kitIds[k], lang), k + 1); }
    shownKit = proc.soundId (hg::kDrums); kitBox.setSelectedId (kitIds.indexOf (shownKit) + 1, dontSendNotification);
}

void DrumPage::refreshPads()
{
    const bool cr = proc.apvts.getRawParameterValue ("drumCreative")->load() > .5f; shownCreative = cr;
    for (int i = 0; i < hg::kNumPads; ++i)
    {
        const auto path = proc.drumSamplePath (i); shownPaths[(size_t) i] = path;
        pads[(size_t) i].source = cr && path.isNotEmpty() ? File (path).getFileNameWithoutExtension() : T ("dmAnalog");   // kapalı modda örnek çalmaz
        pads[(size_t) i].setDescription (pads[(size_t) i].source);
        clearBtns[(size_t) i].setEnabled (cr && path.isNotEmpty()); loadBtns[(size_t) i].setEnabled (cr);
        for (auto& kn : knobs[(size_t) i]) kn.setEnabled (cr);
        pads[(size_t) i].repaint();
    }
    hint.setText (T (cr ? "dmHintOn" : "dmHintOff"), dontSendNotification);
    hint.setColour (Label::textColourId, cr ? lnf.pal().ink : lnf.pal().mute);
    repaint();
}

void DrumPage::tick()
{
    const int h = proc.engine.drumMachine.lastHit.exchange (-1);
    if (h >= 0 && h < hg::kNumPads) pads[(size_t) h].flash = 1.0f;
    for (auto& p : pads) if (p.flash > 0.0f) { p.flash = jmax (0.0f, p.flash - 0.12f); p.repaint(); }
    const bool cr = proc.apvts.getRawParameterValue ("drumCreative")->load() > .5f;
    bool changed = cr != shownCreative;
    for (int i = 0; i < hg::kNumPads; ++i) if (proc.drumSamplePath (i) != shownPaths[(size_t) i]) changed = true;
    if (changed) refreshPads();
    if (proc.soundId (hg::kDrums) != shownKit) fillKits();
}

Rectangle<int> DrumPage::column (int pad) const
{
    const int w = 154, gap = 8; return { 16 + pad * (w + gap), 80, w, 566 };
}

void DrumPage::resized()
{
    auto put = [this] (Component& c, Rectangle<int> r) { c.setBounds (mir (r)); };
    put (title, { 16, 8, 240, 30 });
    put (creativeBtn, { 262, 8, 220, 30 });
    put (kitLabel, { 494, 8, 96, 30 }); put (kitBox, { 590, 9, 230, 28 });
    put (saveKitBtn, { 830, 9, 104, 28 }); put (loadKitBtn, { 940, 9, 104, 28 }); put (resetBtn, { 1050, 9, 104, 28 });
    put (hint, { 16, 46, 1132, 26 });
    for (int i = 0; i < hg::kNumPads; ++i)
    {
        const auto c = column (rtl ? hg::kNumPads - 1 - i : i);   // sağdan sola: ped sırası da aynalanır
        pads[(size_t) i].setBounds (c.getX() + 8, c.getY() + 8, c.getWidth() - 16, 120);
        loadBtns[(size_t) i].setBounds (c.getX() + 8, c.getY() + 134, c.getWidth() - 54, 26);
        clearBtns[(size_t) i].setBounds (c.getRight() - 42, c.getY() + 134, 34, 26);
        for (int k = 0; k < HitGenProcessor::kDmCtl; ++k)
        {
            const int col = rtl ? 1 - kSlot[k][0] : kSlot[k][0], row = kSlot[k][1];
            const bool single = row == 3;    // Level tek başına ortada
            const int kw = 69, x = single ? c.getCentreX() - kw / 2 : c.getX() + 8 + col * kw;
            knobs[(size_t) i][(size_t) k].setBounds (x, c.getY() + 170 + row * 98, kw, 92);
        }
    }
}

void DrumPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    g.setColour (P.panel); g.fillRoundedRectangle (Rectangle<float> (0, 0, (float) kW, 74), 14);
    for (int i = 0; i < hg::kNumPads; ++i)
    {
        const auto c = column (i).toFloat();
        g.setColour (P.panel); g.fillRoundedRectangle (c, 14);
        g.setColour (P.line); g.drawRoundedRectangle (c.reduced (0.5f), 14, 1.0f);
        // Punch/Soft satırı vurgulu: kullanıcının en sık çevireceği iki düğme
        g.setColour (P.accent.withAlpha (0.06f)); g.fillRoundedRectangle (Rectangle<float> (c.getX() + 6, c.getY() + 168, c.getWidth() - 12, 94), 10);
    }
}

// ---------------------------------------------------------------- ped
void DrumPage::Pad::paint (Graphics& g)
{
    auto& o = *owner; const auto& P = o.lnf.pal(); auto r = getLocalBounds().toFloat().reduced (1);
    const bool sample = source != o.T ("dmAnalog");
    // hafif "3B" yüzey: üstten aydınlık, alttan gölge
    ColourGradient bg (P.panel2.brighter (0.12f), r.getX(), r.getY(), P.panel2.darker (0.25f), r.getX(), r.getBottom(), false);
    g.setGradientFill (bg); g.fillRoundedRectangle (r, 12);
    if (flash > 0) { g.setColour (P.accent.withAlpha (0.35f * flash)); g.fillRoundedRectangle (r, 12); }
    g.setColour (dragOver ? P.success : hasKeyboardFocus (false) ? P.focus : flash > 0.3f ? P.accent : P.line);
    g.drawRoundedRectangle (r.reduced (0.5f), 12, dragOver || hasKeyboardFocus (false) ? 2.0f : 1.0f);
    // simge: analog için halka, örnek için küçük dalga
    auto icon = Rectangle<float> (26, 26).withCentre ({ r.getCentreX(), r.getY() + 34 });
    g.setColour (sample ? P.success : P.accent);
    if (sample)
    {
        Path w; for (int i = 0; i <= 24; ++i) { const float x = icon.getX() + i * icon.getWidth() / 24.0f, a = std::sin (i * 0.9f) * (1.0f - i / 26.0f) * 10.0f; if (i == 0) w.startNewSubPath (x, icon.getCentreY()); else w.lineTo (x, icon.getCentreY() - a); }
        g.strokePath (w, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    else { g.drawEllipse (icon.reduced (3), 2.0f); g.fillEllipse (icon.withSizeKeepingCentre (6, 6)); }
    g.setColour (P.ink); g.setFont (o.lnf.font (15, true)); g.drawText (name, r.withTrimmedTop (56).withHeight (22), Justification::centred, true);
    g.setColour (P.mute); g.setFont (o.lnf.font (11.5f)); g.drawText (dragOver ? o.T ("dmDrop") : source, r.withTrimmedTop (80).withHeight (18).reduced (6, 0), Justification::centred, true);
}

void DrumPage::Pad::mouseDown (const MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    grabKeyboardFocus(); owner->proc.engine.drumMachine.audition (index, 112); flash = 1.0f; repaint();
}

bool DrumPage::Pad::keyPressed (const KeyPress& k)
{
    if (k == KeyPress::spaceKey || k == KeyPress::returnKey) { owner->proc.engine.drumMachine.audition (index, 112); flash = 1.0f; repaint(); return true; }
    return false;
}

bool DrumPage::Pad::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files) if (HitGenProcessor::isDrumSampleFile (File (f))) return true;
    return false;
}

void DrumPage::Pad::filesDropped (const StringArray& files, int, int)
{
    dragOver = false; repaint();
    for (auto& f : files) if (HitGenProcessor::isDrumSampleFile (File (f))) { owner->loadSampleFile (index, File (f)); return; }
}

std::unique_ptr<AccessibilityHandler> DrumPage::Pad::createAccessibilityHandler()
{
    return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::button,
        AccessibilityActions().addAction (AccessibilityActionType::press, [this] { owner->proc.engine.drumMachine.audition (index, 112); }));
}

// ---------------------------------------------------------------- örnek ve kit işlemleri
void DrumPage::loadSampleFile (int pad, const File& f)
{
    String err;
    if (proc.loadDrumSample (pad, f, err))
    {
        refreshPads(); proc.engine.drumMachine.audition (pad, 112);
        say (T ("dmLoaded").replace ("{f}", f.getFileName()).replace ("{p}", T (String ("dmPad_") + hg::kPadIds[pad])));
    }
    else say (T (err.isNotEmpty() ? err : "dmBadFile"));
}

void DrumPage::chooseSample (int pad)
{
    chooser = std::make_unique<FileChooser> (T ("dmLoad"), File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, pad] (const FileChooser& fc)
    {
        const auto f = fc.getResult(); if (f.existsAsFile()) loadSampleFile (pad, f);
    });
}

void DrumPage::saveKit()
{
    auto* w = new AlertWindow (T ("dmSaveKit"), T ("dmSaveAsk"), MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.userArtist.isNotEmpty() ? proc.userArtist + " Kit" : String ("My Kit"), T ("dmKitName"));
    w->addButton (T ("hdSave"), 1, KeyPress (KeyPress::returnKey)); w->addButton (T ("libCancel"), 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([this, w] (int r)
    {
        if (r != 1) return;
        File out;
        say (proc.saveDrumKit (w->getTextEditorContents ("name"), out) ? T ("dmKitSaved").replace ("{f}", out.getFullPathName()) : T ("dmKitFail"));
    }), true);
}

void DrumPage::loadKitMenu()
{
    PopupMenu m; m.setLookAndFeel (&lnf); const auto files = HitGenProcessor::drumKitFiles();
    m.addSectionHeader (T ("dmLoadKit"));
    for (int i = 0; i < files.size() && i < 200; ++i) m.addItem (i + 1, files[i].getFileNameWithoutExtension());
    if (files.isEmpty()) m.addItem (-1, T ("dmNoKits"), false);
    m.addSeparator(); m.addItem (1000, T ("dmFromFile"));
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (loadKitBtn), [this, files] (int r)
    {
        auto doLoad = [this] (const File& f)
        {
            StringArray missing;
            if (! proc.loadDrumKit (f, missing)) { say (T ("dmKitFail")); return; }
            refreshPads();
            say (missing.isEmpty() ? T ("dmKitLoaded").replace ("{n}", f.getFileNameWithoutExtension()) : T ("dmKitMissing").replace ("{f}", missing.joinIntoString (", ")));
        };
        if (r >= 1 && r <= files.size()) doLoad (files[r - 1]);
        else if (r == 1000)
        {
            chooser = std::make_unique<FileChooser> (T ("dmLoadKit"), HitGenProcessor::drumKitFolder(), "*.hgkit");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [doLoad] (const FileChooser& fc) { const auto f = fc.getResult(); if (f.existsAsFile()) doLoad (f); });
        }
    });
}
