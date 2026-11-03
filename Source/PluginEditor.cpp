#include "PluginEditor.h"

using namespace juce;

namespace
{
const char* const kPart[4] = { "chords", "bass", "drums", "melody" };
const char* const kGroupKeys[8] = { "gAll", "gTechno", "gHouse", "gMain", "gTrance", "gHard", "gAfro", "gMine" };
const char* const kTabKeys[7] = { "navRef", "navDetect", "navDna", "navGen", "navMidi", "tabSound", "tabPresets" };
const char* const kAwardKeys[][2] = { { "house", "awHouse" }, { "techno", "awTechno" }, { "trance", "awTrance" }, { "psy", "awPsy" }, { "hard", "awHard" }, { "group", "awGroup" } };
String fill (String s, const String& k, const String& v) { return s.replace ("{" + k + "}", v); }
String stripTags (String s) { while (s.contains ("<")) { auto a = s.indexOf ("<"), b = s.indexOf (a, ">"); if (b < 0) break; s = s.substring (0, a) + s.substring (b + 1); } return s; }
}

HitGenEditor::HitGenEditor (HitGenProcessor& p) : AudioProcessorEditor (p), proc (p), data (p.data)
{
    setLookAndFeel (&lnf);
    content.owner = this;
    addAndMakeVisible (content);
    content.setFocusContainerType (FocusContainerType::keyboardFocusContainer);
    setTitle ("Wavejackers HitGen");

    auto& A = proc.apvts;
    auto addC = [this] (Component& c) { content.addAndMakeVisible (c); };
    for (auto* l : { &tagline, &status, &disclaimer, &varLabel, &soundLabel, &hostLabel, &seedLabel, &langLabel, &qualityLabel, &sizeLabel, &keyLabel, &scaleLabel, &barsLabel, &keysToLabel }) { addC (*l); l->setFont (lnf.font (13)); }
    tagline.setColour (Label::textColourId, lnf.pal().mute); disclaimer.setColour (Label::textColourId, lnf.pal().mute);
    status.setFont (lnf.font (12)); disclaimer.setFont (lnf.font (11.5f)); status.setJustificationType (Justification::topLeft); disclaimer.setJustificationType (Justification::topLeft);
    status.setAccessible (true);

    addC (playBtn); playBtn.setClickingTogglesState (false);
    playBtn.onClick = [this] { proc.previewPlaying.store (! proc.previewPlaying.load()); playBtn.setToggleState (proc.previewPlaying.load(), dontSendNotification); relabel(); };
    addC (newBtn); newBtn.onClick = [this] { proc.newIdea(); };
    addC (seed); seed.setSliderStyle (Slider::IncDecButtons); seed.setTextBoxStyle (Slider::TextBoxLeft, false, 64, 28); seed.setWantsKeyboardFocus (true);
    sa.push_back (std::make_unique<SA> (A, "seed", seed)); hgui::tagParam (seed, "seed");

    for (int i = 0; i < data.langNames.size(); ++i) langBox.addItem (data.langNames[i], i + 1);
    addC (langBox); langBox.onChange = [this] { setLanguage (data.langCodes[langBox.getSelectedId() - 1]); };
    qualityBox.addItemList ({ "Eco", "Balanced", "High" }, 1); addC (qualityBox); ca.push_back (std::make_unique<CA> (A, "quality", qualityBox));
    sizeBox.addItem ("Compact 900", 76); sizeBox.addItem ("Default 1100", 93); sizeBox.addItem ("Expanded 1400", 119);
    addC (sizeBox); sizeBox.onChange = [this] { proc.uiScale = sizeBox.getSelectedId() / 100.0f; setSize (roundToInt (kW * proc.uiScale), roundToInt (kH * proc.uiScale)); };
    addC (contrastBtn); contrastBtn.onClick = [this] { proc.highContrast = contrastBtn.getToggleState(); applyPalette(); };
    addC (motionBtn); motionBtn.setToggleState (proc.animations, dontSendNotification); motionBtn.onClick = [this] { proc.animations = motionBtn.getToggleState(); content.repaint(); };

    for (int i = 0; i < 7; ++i)
    {
        auto& b = tabBtns[(size_t) i]; addC (b); b.setClickingTogglesState (true); b.setRadioGroupId (100); b.setComponentID ("nav");
        b.onClick = [this, i] { if (! tabBtns[(size_t) i].getToggleState()) return; if (i < 4) { if (tab != 3 || ws != i) setWorkspace (i); } else if (tab != i - 4) setTab (i - 4); };
    }
    tabBtns[0].setToggleState (true, dontSendNotification);
    settingsPanel.o = this; content.addChildComponent (settingsPanel);
    for (auto* b : { &presetBtn, &saveBtn, &settingsBtn, &helpBtn }) addC (*b);
    presetBtn.onClick = [this] { showPresetMenu(); }; saveBtn.onClick = [this] { showSaveDialog(); };
    settingsBtn.setClickingTogglesState (false); settingsBtn.onClick = [this] { toggleSettings (! settingsOpen); }; helpBtn.onClick = [this] { showHelp(); };
    for (int i = 0; i < 8; ++i)
    {
        auto& b = groupBtns[(size_t) i]; addC (b); b.setClickingTogglesState (true); b.setRadioGroupId (101);
        b.onClick = [this, i] { if (groupBtns[(size_t) i].getToggleState()) { group = i; refreshStyleList(); } };
    }
    groupBtns[0].setToggleState (true, dontSendNotification);
    addC (styleList); styleList.setModel (&styleModel); styleList.setRowHeight (44); styleList.setWantsKeyboardFocus (true);
    for (int i = 0; i < 4; ++i)
    {
        auto& b = varBtns[(size_t) i]; addC (b); b.setRadioGroupId (102); b.setClickingTogglesState (true);
        b.onClick = [this, i] { if (varBtns[(size_t) i].getToggleState()) setParam ("variation", (float) i); };
    }
    const char* genIds[6] = { "orig", "energy", "thick", "human", "pump", "swing" };
    for (int i = 0; i < 6; ++i) { addC (genKnobs[(size_t) i]); sa.push_back (std::make_unique<SA> (A, genIds[i], genKnobs[(size_t) i].slider)); hgui::tagParam (genKnobs[(size_t) i].slider, genIds[i]); }
    for (auto& n : data.noteNames) keyBox.addItem (n, keyBox.getNumItems() + 1);
    addC (keyBox); ca.push_back (std::make_unique<CA> (A, "key", keyBox));
    scaleBox.addItem ("auto", 1); for (auto& s : data.scaleOrder) scaleBox.addItem (s, scaleBox.getNumItems() + 1);
    addC (scaleBox); ca.push_back (std::make_unique<CA> (A, "scale", scaleBox));
    barsBox.addItemList ({ "4", "8", "16" }, 1); addC (barsBox); ca.push_back (std::make_unique<CA> (A, "bars", barsBox));
    keysToBox.addItemList ({ "Chords", "Bass", "Melody" }, 1); addC (keysToBox); ca.push_back (std::make_unique<CA> (A, "keysTo", keysToBox));
    addC (seventhsBtn); ba.push_back (std::make_unique<BA> (A, "sevenths", seventhsBtn));
    addC (patternBtn); ba.push_back (std::make_unique<BA> (A, "pattern", patternBtn));
    addC (midiOutBtn); ba.push_back (std::make_unique<BA> (A, "midiOut", midiOutBtn));

    auto palFn = [this]() -> const hgui::Palette& { return lnf.pal(); };
    auto fontFn = [this] (float h, bool b) { return lnf.font (h, b); };
    chords.palette = palFn; chords.font = fontFn; addC (chords); chords.setAccessible (false);
    roll.palette = palFn; addC (roll);
    for (int t = 0; t < 4; ++t)
    {
        addC (trackLabels[(size_t) t]); trackLabels[(size_t) t].setFont (lnf.font (14, true)); trackLabels[(size_t) t].setColour (Label::textColourId, lnf.pal().track[t]);
        auto& m = muteBtns[(size_t) t]; addC (m); m.setClickingTogglesState (true);
        muteBa.push_back (std::make_unique<BA> (A, String ("mute_") + kPart[t], m));
        auto& ab = altBtns[(size_t) t]; addC (ab); ab.setButtonText (juce::String (juce::CharPointer_UTF8 ("\xe2\x86\xbb"))); ab.onClick = [this, t] { proc.suggestAlternative (t); };
        auto& sb = soundBoxes[(size_t) t]; addC (sb);
        sb.onChange = [this, t] { const int i = soundBoxes[(size_t) t].getSelectedId() - 1; if (i >= 0 && i < soundIds[(size_t) t].size() && soundIds[(size_t) t][i] != proc.soundId (t)) { proc.setSound (t, soundIds[(size_t) t][i], true); shownSounds[(size_t) t] = soundIds[(size_t) t][i]; } };
        auto& tl = tiles[(size_t) t]; addC (tl); tl.palette = palFn; tl.font = fontFn; tl.colour = lnf.pal().track[t];
        tl.makeFile = [this, t] { return proc.writeMidi (t); };
        tl.onSaved = [this] (const File& f) { status.setText (fill (T ("pSavedTo"), "f", f.getFullPathName()), sendNotificationAsync); };
    }
    for (int i = 0; i < 3; ++i)
    {
        auto& b = partBtns[(size_t) i]; addC (b); b.setRadioGroupId (103); b.setClickingTogglesState (true);
        b.onClick = [this, i] { if (partBtns[(size_t) i].getToggleState()) bindPart (i); };
    }
    for (auto& k : partKnobs) addC (k);
    for (auto* tl : { &loopTile, &fullTile }) { addC (*tl); tl->palette = palFn; tl->font = fontFn; tl->onSaved = [this] (const File& f) { status.setText (fill (T ("pSavedTo"), "f", f.getFullPathName()), sendNotificationAsync); }; }
    loopTile.makeFile = [this] { return proc.writeMidi (4); };
    fullTile.makeFile = [this] { return proc.writeMidi (5); };

    soundPage = std::make_unique<SoundPage> (proc, lnf); content.addChildComponent (*soundPage);
    presetPage = std::make_unique<PresetPage> (proc, lnf); content.addChildComponent (*presetPage);
    refPage = std::make_unique<ReferencePage> (proc, lnf); content.addChildComponent (*refPage);
    refPage->onSavePreset = [this] { showSaveDialog(); };
    presetPage->onLoaded = [this] { shownStyle = -1; shownVar = -1; fillSoundBoxes(); refreshStyleList(); applyPalette(); };
    for (auto* c : std::initializer_list<Component*> { &styleList, &varLabel, &keyLabel, &keyBox, &scaleLabel, &scaleBox, &barsLabel, &barsBox, &chords, &roll, &soundLabel,
                                                     &patternBtn, &midiOutBtn, &keysToLabel, &keysToBox, &seventhsBtn, &loopTile, &fullTile, &status, &disclaimer })
        genComps.push_back (c);
    for (auto& b : groupBtns) genComps.push_back (&b);
    for (auto& b : varBtns) genComps.push_back (&b);
    for (auto& k : genKnobs) genComps.push_back (&k);
    for (int t = 0; t < 4; ++t) { genComps.push_back (&trackLabels[(size_t) t]); genComps.push_back (&muteBtns[(size_t) t]); genComps.push_back (&altBtns[(size_t) t]); genComps.push_back (&soundBoxes[(size_t) t]); genComps.push_back (&tiles[(size_t) t]); }
    for (auto& b : partBtns) genComps.push_back (&b);
    for (auto& k : partKnobs) genComps.push_back (&k);
    lang = proc.lang; rtl = data.isRtl (lang);
    langBox.setSelectedId (data.langCodes.indexOf (lang) + 1, dontSendNotification);
    { const int want = roundToInt (proc.uiScale * 100); sizeBox.setSelectedId (std::abs (want - 76) <= std::abs (want - 93) && std::abs (want - 76) <= std::abs (want - 119) ? 76 : std::abs (want - 93) <= std::abs (want - 119) ? 93 : 119, dontSendNotification); }
    contrastBtn.setToggleState (proc.highContrast, dontSendNotification);
    partBtns[0].setToggleState (true, dontSendNotification); bindPart (0);
    shownVar = (int) proc.apvts.getRawParameterValue ("variation")->load();
    varBtns[(size_t) jlimit (0, 3, shownVar)].setToggleState (true, dontSendNotification);
    lnf.setLanguage (lang);
    refreshStyleList(); fillSoundBoxes(); applyPalette(); relabel(); refreshResult();
    soundPage->setLanguage (lang, rtl); presetPage->setLanguage (lang, rtl); refPage->setLanguage (lang, rtl);
    proc.onResultChanged = [this] { refreshResult(); };

    setResizable (true, true);
    if (auto* c = getConstrainer()) { c->setFixedAspectRatio ((double) kW / kH); c->setSizeLimits (kW * 3 / 4, kH * 3 / 4, kW * 2, kH * 2); }
    setSize (roundToInt (kW * proc.uiScale), roundToInt (kH * proc.uiScale));
    setWantsKeyboardFocus (true);
    addC (midiBtn); midiBtn.onClick = [this] { showMidiMappings(); };
    soundPage->onRebind = [this] { midiShownVer = -1; };
    addMouseListener (this, true);              // sağ tık > MIDI Learn (tüm alt bileşenler)
    midiSeenLearned = proc.midiLearn.lastLearned(); startTimerHz (30);   // editör açılırken eski bildirimi gösterme
}

HitGenEditor::~HitGenEditor()
{
    stopTimer(); removeMouseListener (this);
    if (proc.midiLearn.learning() >= 0) proc.midiLearn.cancelLearn();
    proc.onResultChanged = nullptr; proc.auditionPart.store (-1);
    soundPage.reset(); presetPage.reset(); refPage.reset();
    partSa.clear(); sa.clear(); ca.clear(); ba.clear(); muteBa.clear();
    styleList.setModel (nullptr);
    setLookAndFeel (nullptr);
}

void HitGenEditor::setParam (const char* id, float plain)
{
    if (auto* pr = proc.apvts.getParameter (id)) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); pr->endChangeGesture(); }
}

Colour HitGenEditor::styleAccent() const
{
    const int si = jlimit (0, data.numStyles() - 1, (int) proc.apvts.getRawParameterValue ("style")->load());
    return Colour::fromString ("ff" + data.style (si)["accent"].toString().substring (1));
}

void HitGenEditor::applyPalette()
{
    lnf.setPalette (hgui::Palette::make (proc.highContrast, styleAccent()));
    for (int t = 0; t < 4; ++t) { trackLabels[(size_t) t].setColour (Label::textColourId, lnf.pal().track[t]); tiles[(size_t) t].colour = lnf.pal().track[t]; }
    tagline.setColour (Label::textColourId, lnf.pal().mute); disclaimer.setColour (Label::textColourId, lnf.pal().mute);
    for (auto* l : { &status, &varLabel, &soundLabel, &hostLabel, &seedLabel, &langLabel, &qualityLabel, &sizeLabel, &keyLabel, &scaleLabel, &barsLabel, &keysToLabel }) l->setColour (Label::textColourId, lnf.pal().ink);
    if (soundPage != nullptr) { soundPage->setLanguage (lang, rtl); presetPage->setLanguage (lang, rtl); refPage->setLanguage (lang, rtl); }
    sendLookAndFeelChange();
    repaint();
}

void HitGenEditor::setLanguage (const String& l)
{
    if (! data.langCodes.contains (l)) return;
    lang = l; proc.lang = l; rtl = data.isRtl (l);
    lnf.setLanguage (l);
    langBox.setSelectedId (data.langCodes.indexOf (l) + 1, dontSendNotification);
    relabel(); fillSoundBoxes(); refreshStyleList(); refreshResult();
    soundPage->setLanguage (lang, rtl); presetPage->setLanguage (lang, rtl); refPage->setLanguage (lang, rtl);
    sendLookAndFeelChange(); resized(); repaint();
}

void HitGenEditor::setTab (int t)
{
    tab = jlimit (0, 3, t);
    tabBtns[(size_t) (tab == 3 ? ws : 4 + tab)].setToggleState (true, dontSendNotification);
    for (auto* c : genComps) c->setVisible (tab == 0);
    soundPage->setVisible (tab == 1); presetPage->setVisible (tab == 2); refPage->setVisible (tab == 3);
    for (auto* c : std::initializer_list<Component*> { &seed, &newBtn }) c->setVisible (tab == 0);
    proc.auditionPart.store (tab == 1 ? soundPage->currentPart() : tab == 3 ? refPage->currentPart() : -1);
    if (tab == 1) soundPage->tick();
    toggleSettings (false); repaint();
}

void HitGenEditor::setWorkspace (int w)
{
    ws = jlimit (0, 3, w); refPage->setWorkspace (ws); setTab (3);
}

void HitGenEditor::toggleSettings (bool on)
{
    settingsOpen = on; settingsPanel.setVisible (on);
    if (on) settingsPanel.toFront (false);
    for (auto* c : std::initializer_list<Component*> { &langLabel, &langBox, &qualityLabel, &qualityBox, &sizeBox, &contrastBtn, &motionBtn, &midiBtn }) { c->setVisible (on); if (on) c->toFront (false); }
    sizeLabel.setVisible (false);
    settingsBtn.setToggleState (on, dontSendNotification); repaint();
}

void HitGenEditor::Backdrop::paint (Graphics& g)
{
    const auto& P = o->lnf.pal(); auto r = getLocalBounds().toFloat().reduced (1);
    g.setColour (P.panel2); g.fillRoundedRectangle (r, 12); g.setColour (P.line); g.drawRoundedRectangle (r, 12, 1);
}

void HitGenEditor::showHelp()
{
    const auto opts = MessageBoxOptions::makeOptionsOk (MessageBoxIconType::InfoIcon, T ("hdHelp"), T ("helpText"), "OK", this);
    AlertWindow::showAsync (opts, nullptr);
}

void HitGenEditor::showSaveDialog()
{
    auto* w = new AlertWindow (T ("hdSave"), T ("saveAsk"), MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("artist", proc.userArtist, T ("prArtist")); w->addTextEditor ("name", proc.userPreset, T ("prName"));
    w->addButton (T ("hdSave"), 1, KeyPress (KeyPress::returnKey)); w->addButton (T ("libCancel"), 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([this, w] (int r)
    {
        if (r == 1)
        {
            String f; const bool ok = proc.savePreset (w->getTextEditorContents ("artist"), w->getTextEditorContents ("name"), f);
            status.setText (ok ? fill (T ("pSavedTo"), "f", f) : T ("saveNeed"), sendNotificationAsync); presetBtn.setButtonText (proc.userArtist.isNotEmpty() ? proc.userArtist + " - " + proc.userPreset : T ("hdPreset"));
        }
    }), true);
}

void HitGenEditor::showPresetMenu()
{
    PopupMenu m; const auto items = proc.presets.items();
    for (int i = 0; i < items.size() && i < 200; ++i) m.addItem (i + 1, items[i].artist + " - " + items[i].name);
    if (items.isEmpty()) m.addItem (-1, T ("hdNoPresets"), false);
    m.addSeparator(); m.addItem (1000, T ("hdBrowse"));
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (presetBtn), [this, items] (int r)
    {
        if (r == 1000) { setTab (2); return; }
        if (r >= 1 && r <= items.size() && proc.loadPreset (items[r - 1].file)) { shownStyle = -1; shownVar = -1; fillSoundBoxes(); refreshStyleList(); applyPalette(); presetBtn.setButtonText (items[r - 1].artist + " - " + items[r - 1].name); }
    });
}

void HitGenEditor::relabel()
{
    const auto just = rtl ? Justification::centredRight : Justification::centredLeft;
    tagline.setText (T ("tagline"), dontSendNotification); tagline.setJustificationType (just);
    playBtn.setButtonText (proc.previewPlaying.load() ? T ("stop") : T ("play"));
    playBtn.setTooltip (T ("pFollow")); playBtn.setHelpText (T ("pFollow"));
    newBtn.setButtonText (T ("newIdea")); newBtn.setTooltip (T ("shortcuts"));
    seedLabel.setText (T ("seed"), dontSendNotification); seed.setTitle (T ("seed"));
    langLabel.setText (T ("lang"), dontSendNotification); langBox.setTitle (T ("lang"));
    qualityLabel.setText (T ("perf"), dontSendNotification); qualityBox.setTitle (T ("perf"));
    auto refill = [] (ComboBox& cb, const StringArray& items)   // çeviri: öğeleri yeniden kur, seçimi koru
    {
        const int id = cb.getSelectedId(); cb.clear (dontSendNotification);
        for (int i = 0; i < items.size(); ++i) cb.addItem (items[i], i + 1);
        cb.setSelectedId (id, dontSendNotification);
    };
    refill (qualityBox, { T ("pEco"), T ("pBal"), T ("pHigh") });
    sizeLabel.setText (T ("pUiSize"), dontSendNotification); sizeBox.setTitle (T ("pUiSize"));
    contrastBtn.setButtonText (T ("pContrast")); motionBtn.setButtonText (T ("pMotion")); midiBtn.setButtonText (T ("mlAll")); midiBtn.setTooltip (T ("mlHint"));
    for (int i = 0; i < 8; ++i) groupBtns[(size_t) i].setButtonText (T (kGroupKeys[i]));
    presetBtn.setButtonText (proc.userArtist.isNotEmpty() ? proc.userArtist + " - " + proc.userPreset : T ("hdPreset")); saveBtn.setButtonText (T ("hdSave")); settingsBtn.setButtonText (T ("hdSettings")); helpBtn.setButtonText (T ("hdHelp"));
    for (int i = 0; i < 7; ++i) { tabBtns[(size_t) i].setButtonText (T (kTabKeys[i]).toUpperCase()); tabBtns[(size_t) i].setTooltip ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0 ? "Cmd+" + String (i + 1) : "Ctrl+" + String (i + 1)); }
    styleList.setTitle (T ("pStyleList"));
    varLabel.setText (T ("variation"), dontSendNotification); varLabel.setJustificationType (just);
    const char* genKeys[6] = { "orig", "energy", "thick", "human", "pump", "swing" };
    for (int i = 0; i < 6; ++i) genKnobs[(size_t) i].setText (T (genKeys[i]), i == 2 ? T ("thickHint") : String());
    keyLabel.setText (T ("key"), dontSendNotification); keyBox.setTitle (T ("key"));
    scaleLabel.setText (T ("scale"), dontSendNotification); scaleBox.setTitle (T ("scale"));
    { StringArray sc { T ("byStyle") }; for (auto& k : data.scaleOrder) sc.add (T ("scale_" + k)); refill (scaleBox, sc); }
    barsLabel.setText (T ("length"), dontSendNotification); barsBox.setTitle (T ("length"));
    refill (barsBox, { fill (T ("bars"), "n", "4"), fill (T ("bars"), "n", "8"), fill (T ("bars"), "n", "16") });
    keysToLabel.setText (T ("pKeysTo"), dontSendNotification); keysToBox.setTitle (T ("pKeysTo"));
    refill (keysToBox, { T ("chords"), T ("bass"), T ("melody") });
    seventhsBtn.setButtonText (T ("sevenths")); patternBtn.setButtonText (T ("pPattern")); midiOutBtn.setButtonText (T ("pMidiOut"));
    patternBtn.setTooltip (T ("pFollow"));
    for (auto* tb : { &contrastBtn, &motionBtn, &seventhsBtn, &patternBtn, &midiOutBtn }) tb->getProperties().set ("rtl", rtl);
    for (int t = 0; t < 4; ++t)
    {
        trackLabels[(size_t) t].setText (T (kPart[t]), dontSendNotification); trackLabels[(size_t) t].setJustificationType (just);
        altBtns[(size_t) t].setTooltip (T ("pAlt") + ": " + T (kPart[t]) + ". " + T ("pAltHint")); altBtns[(size_t) t].setTitle (T ("pAlt") + ": " + T (kPart[t])); altBtns[(size_t) t].setHelpText (T ("pAltHint"));
        muteBtns[(size_t) t].setButtonText (T ("mute")); muteBtns[(size_t) t].setTitle (T ("mute") + ": " + T (kPart[t]));
        soundBoxes[(size_t) t].setTitle (fill (T ("soundOf"), "t", T (kPart[t])));
        tiles[(size_t) t].label = T (kPart[t]) + " .mid"; tiles[(size_t) t].setTitle (T ("dragMid") + ": " + T (kPart[t]));
        tiles[(size_t) t].setTooltip (T ("pDragHint")); tiles[(size_t) t].setHelpText (T ("pDragHint")); tiles[(size_t) t].repaint();
    }
    soundLabel.setText (T ("pSound"), dontSendNotification); soundLabel.setJustificationType (just);
    partBtns[0].setButtonText (T ("chords")); partBtns[1].setButtonText (T ("bass")); partBtns[2].setButtonText (T ("melody"));
    const char* pk[8] = { "cutoff", "reso", "attack", "release", "drive", "reverb", "delay", "volume" };
    for (int i = 0; i < 8; ++i) partKnobs[(size_t) i].setText (T (pk[i]), {});
    loopTile.label = T ("pLoopAll"); loopTile.setTitle (T ("pLoopAll")); loopTile.setTooltip (T ("pDragHint")); loopTile.setHelpText (T ("pDragHint"));
    fullTile.label = T ("pFull"); fullTile.setTitle (T ("pFull")); fullTile.setTooltip (T ("pDragHint")); fullTile.setHelpText (T ("pDragHint"));
    disclaimer.setText (stripTags (T ("know2")), dontSendNotification); disclaimer.setJustificationType (rtl ? Justification::topRight : Justification::topLeft);
    status.setJustificationType (rtl ? Justification::topRight : Justification::topLeft);
    for (auto* l : { &seedLabel, &langLabel, &qualityLabel, &sizeLabel, &keyLabel, &scaleLabel, &barsLabel, &keysToLabel, &hostLabel }) l->setJustificationType (rtl ? Justification::centredRight : Justification::centredLeft);
    repaint();
}

void HitGenEditor::refreshStyleList()
{
    filtered.clear();
    if (isMine())
    {
        for (int i = 0; i < proc.presets.items().size(); ++i) filtered.add (i);
        styleList.updateContent();
        const int row = proc.presets.indexOf (proc.userArtist, proc.userPreset);
        if (row >= 0) styleList.selectRow (row, false, true); else styleList.deselectAllRows();
        styleList.repaint();
        return;
    }
    const String g = group == 0 ? String() : data.root()["groups"][group].toString();
    for (int i = 0; i < data.numStyles(); ++i) if (g.isEmpty() || data.style (i)["grp"].toString() == g) filtered.add (i);
    styleList.updateContent();
    const int si = (int) proc.apvts.getRawParameterValue ("style")->load();
    const int row = filtered.indexOf (si);
    if (row >= 0) styleList.selectRow (row, false, true); else styleList.deselectAllRows();
    styleList.repaint();
}

void HitGenEditor::StyleModel::paintListBoxItem (int row, Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= ed.filtered.size()) return;
    if (ed.isMine())
    {
        const auto& it = ed.proc.presets.items(); if (row >= it.size()) return;
        const auto& P = ed.lnf.pal(); const auto& pr = it[row];
        const int si = ed.data.styleIndex (pr.style);
        const auto acc = P.accent;
        auto r = Rectangle<int> (0, 0, w, h).reduced (2, 2).toFloat();
        g.setColour (selected ? acc.withAlpha (.16f) : P.panel2); g.fillRoundedRectangle (r, 8);
        g.setColour (acc); g.fillRect (ed.rtl ? r.getRight() - 4 : r.getX(), r.getY() + 4, 4.0f, r.getHeight() - 8);
        if (selected) { g.setColour (acc); g.drawRoundedRectangle (r, 8, 1.2f); }
        auto tr = r.reduced (12, 3).toNearestInt(); const auto j = ed.rtl ? Justification::centredRight : Justification::centredLeft;
        g.setColour (P.ink); g.setFont (ed.lnf.font (14.5f, true)); g.drawText (pr.artist, tr.removeFromTop (20), j);
        g.setColour (P.mute); g.setFont (ed.lnf.font (11.5f));
        g.drawText (pr.name + (si >= 0 ? "  ·  " + ed.T ("prBase").replace ("{s}", ed.data.style (si)["artist"].toString()) : String()), tr, j);
        return;
    }
    const auto& P = ed.lnf.pal(); const auto& S = ed.data.style (ed.filtered[row]);
    const auto acc = P.accent;
    auto r = Rectangle<int> (0, 0, w, h).reduced (2, 2).toFloat();
    g.setColour (selected ? acc.withAlpha (.16f) : P.panel2); g.fillRoundedRectangle (r, 8);
    g.setColour (acc); g.fillRect (ed.rtl ? r.getRight() - 4 : r.getX(), r.getY() + 4, 4.0f, r.getHeight() - 8);
    if (selected) { g.setColour (acc); g.drawRoundedRectangle (r, 8, 1.2f); }
    auto tx = ed.data.styleText (ed.lang, S["id"].toString());
    auto tr = r.reduced (12, 3).toNearestInt(); const auto j = ed.rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.ink); g.setFont (ed.lnf.font (14.5f, true));
    String right; if ((int) S["rank"] > 0) right = "#" + S["rank"].toString();
    g.drawText (S["artist"].toString(), tr.removeFromTop (20), j);
    g.setColour (P.mute); g.setFont (ed.lnf.font (11.5f));
    g.drawText (tx[0].toString() + "  " + S["bpm"][0].toString() + "-" + S["bpm"][1].toString() + " BPM", tr, j);
    if (right.isNotEmpty()) { g.setColour (acc); g.setFont (ed.lnf.font (12, true)); g.drawText (right, r.reduced (12, 4).toNearestInt(), ed.rtl ? Justification::topLeft : Justification::topRight); }
}
void HitGenEditor::StyleModel::selectedRowsChanged (int row)
{
    if (row < 0 || row >= ed.filtered.size()) return;
    if (ed.isMine())
    {
        const auto& it = ed.proc.presets.items(); if (row >= it.size()) return;
        if (it[row].artist == ed.proc.userArtist && it[row].name == ed.proc.userPreset) return;
        if (ed.proc.loadPreset (it[row].file)) { ed.shownStyle = (int) ed.proc.apvts.getRawParameterValue ("style")->load(); ed.fillSoundBoxes(); ed.applyPalette(); ed.status.setText (ed.T ("prLoaded").replace ("{n}", it[row].display()), dontSendNotification); }
        return;
    }
    const int si = ed.filtered[row];
    if ((int) ed.proc.apvts.getRawParameterValue ("style")->load() != si) ed.setParam ("style", (float) si);
}
String HitGenEditor::StyleModel::getNameForRow (int row)
{
    if (row < 0 || row >= ed.filtered.size()) return {};
    if (ed.isMine()) { const auto& it = ed.proc.presets.items(); return row < it.size() ? it[row].display() + ", " + ed.T ("prBase").replace ("{s}", ed.data.styleById (it[row].style)["artist"].toString()) : String(); }
    const auto& S = ed.data.style (ed.filtered[row]); auto tx = ed.data.styleText (ed.lang, S["id"].toString());
    String s = S["artist"].toString() + ", " + tx[0].toString() + ", " + S["bpm"][0].toString() + "-" + S["bpm"][1].toString() + " BPM";
    if ((int) S["rank"] > 0) s << ", " << fill (ed.T ("rank"), "n", S["rank"].toString());
    for (auto& a : kAwardKeys) if (S["award"].toString() == a[0]) s << ", " << ed.T (a[1]);
    return s + ". " + tx[1].toString();
}

void HitGenEditor::fillSoundBoxes()
{
    const char roleOf[4] = { 'c', 'b', 0, 'm' };
    for (int t = 0; t < 4; ++t)
    {
        auto& sb = soundBoxes[(size_t) t]; auto& ids = soundIds[(size_t) t];
        sb.clear (dontSendNotification); ids.clear();
        if (t == hg::kDrums) { for (int k = 0; k < data.numKits(); ++k) { ids.add (data.kit (k)["id"].toString()); sb.addItem (data.soundName (ids[k], lang), k + 1); } }
        else
        {
            // önce imza sesler, sonra kategoriler
            for (int pass = 0; pass < 2; ++pass)
            {
                if (pass == 1) sb.addSeparator();
                for (int s = 0; s < data.numSounds(); ++s)
                {
                    const auto& v = data.sound (s); const auto id = v["id"].toString();
                    if (! v["r"].toString().containsChar ((juce_wchar) roleOf[t])) continue;
                    if (data.isSignature (id) != (pass == 0)) continue;
                    ids.add (id); sb.addItem (data.soundName (id, lang) + (pass == 1 ? "  (" + T ("cat_" + v["cat"].toString()) + ")" : String()), ids.size());
                }
            }
        }
        const int cur = ids.indexOf (proc.soundId (t));
        sb.setSelectedId (cur + 1, dontSendNotification);
        shownSounds[(size_t) t] = proc.soundId (t);
    }
}

void HitGenEditor::bindPart (int p)
{
    part = p; partSa.clear();
    const char* ids[8] = { "cut_", "res_", "atk_", "rel_", "drv_", "rev_", "dly_", "vol_" };
    const char* pn = p == 0 ? "chords" : p == 1 ? "bass" : "melody";
    for (int i = 0; i < 8; ++i) { partSa.push_back (std::make_unique<SA> (proc.apvts, String (ids[i]) + pn, partKnobs[(size_t) i].slider)); hgui::tagParam (partKnobs[(size_t) i].slider, String (ids[i]) + pn); }
    midiShownVer = -1;     // rozetleri yeni parametrelere göre yenile
}

void HitGenEditor::refreshResult()
{
    auto r = proc.result(); if (! r) return;
    StringArray names; for (int t : { 0, 1, 2, 3 }) names.add (T (kPart[t]));
    String summary;
    for (int t = 0; t < 4; ++t)
    {
        const auto& n = r->tracks[(size_t) t]; String range;
        if (t != hg::kDrums && ! n.empty()) { int lo = 127, hi = 0; for (auto& e : n) { lo = jmin (lo, e.n); hi = jmax (hi, e.n); } range = data.noteNames[lo % 12] + String (lo / 12 - 1) + "-" + data.noteNames[hi % 12] + String (hi / 12 - 1); }
        summary << fill (fill (fill (T ("rollLabel"), "t", T (kPart[t])), "n", String ((int) n.size())), "r", range.isNotEmpty() ? range : T ("gm10")) << ". ";
    }
    StringArray chordNames; for (auto& c : r->chords) chordNames.add (c.name);
    summary << T ("chords") << ": " << chordNames.joinIntoString (", ");
    roll.setResult (r, { T ("chords"), T ("bass"), T ("drums"), T ("melody") }, summary); roll.setRtl (rtl);
    chords.setResult (r, T ("bar")); chords.setRtl (rtl);
    auto ki = hg::keyInfo (data, r->P.key, r->P.scale);
    auto tx = data.styleText (lang, r->P.style); auto vv = tx[2 + jlimit (0, 3, r->P.vi)];
    status.setText (data.noteNames[r->P.key] + " " + T ("scale_" + r->P.scale) + " · " + ki.cam + " · " + vv[0].toString() + "\n" + vv[1].toString() + " " + T ("compat") + " " + ki.compat.joinIntoString (", "), dontSendNotification);
    for (int i = 0; i < 4; ++i) { varBtns[(size_t) i].setButtonText (tx[2 + i][0].toString()); varBtns[(size_t) i].setTooltip (tx[2 + i][1].toString()); varBtns[(size_t) i].setHelpText (tx[2 + i][1].toString()); }
    repaint();
}

void HitGenEditor::timerCallback()
{
    // animasyon: seviye göstergesi her zaman, arka plan hareketi kalite ayarına göre
    {
        const float pk = proc.engine.peakOut.exchange (0.0f);
        meter = pk > meter ? pk : meter * 0.86f;
        meterHold = pk > meterHold ? pk : jmax (0.0f, meterHold - 0.012f);
        if (proc.midiLearn.version() != midiShownVer) refreshMidiBadges();
        if (midiToastUntil > 0 && Time::currentTimeMillis() > midiToastUntil && proc.midiLearn.learning() < 0) { midiToastUntil = 0; midiToast.clear(); content.repaint(); }
        if (refPage != nullptr && tab == 3) refPage->setLevel (jlimit (0.0f, 1.0f, meter / .891f));
        const int q = (int) proc.apvts.getRawParameterValue ("quality")->load();
        juce::ignoreUnused (q);
        ++frame;
        if (frame % 3 == 0) content.repaint (548, 16, 140, 16);
    }
    lnf.stepHover (proc.animations && (int) proc.apvts.getRawParameterValue ("quality")->load() != 0);   // hover geçişleri yalnızca arayüz iş parçacığında
    const bool hostPlay = proc.hostPlaying.load(), prev = proc.previewPlaying.load();
    roll.setPlayhead (proc.playPpq.load(), hostPlay || prev);
    chords.setCurrentBar (hostPlay || prev ? (int) (proc.playPpq.load() / 4.0) : -1);
    const double hb = proc.hostBpm.load();
    const String bpmText = String (hb > 0 ? roundToInt (hb) : (int) proc.apvts.getRawParameterValue ("bpm")->load()) + " BPM";
    hostLabel.setText ((hostPlay ? T ("pHost") + " " + T ("nowPlaying") + " · " : String()) + bpmText, dontSendNotification);
    if (playBtn.getToggleState() != prev) { playBtn.setToggleState (prev, dontSendNotification); relabel(); }
    const int si = (int) proc.apvts.getRawParameterValue ("style")->load();
    if (si != shownStyle) { shownStyle = si; refreshStyleList(); applyPalette(); }
    const int vi = (int) proc.apvts.getRawParameterValue ("variation")->load();
    if (vi != shownVar && vi >= 0 && vi < 4) { shownVar = vi; varBtns[(size_t) vi].setToggleState (true, dontSendNotification); }
    bool soundsChanged = false; for (int t = 0; t < 4; ++t) if (shownSounds[(size_t) t] != proc.soundId (t)) soundsChanged = true;
    if (soundsChanged) fillSoundBoxes();
    if (tab == 1) soundPage->tick();
    presetPage->tick();
    if (tab == 3) refPage->tick();
    static int presetRev = -1;
    if (isMine() && proc.presets.revision() != presetRev) { presetRev = proc.presets.revision(); refreshStyleList(); }
    static int tick = 0;
    if (++tick % 15 == 0)
    {
        auto tip = T ("pCpu") + " " + String (roundToInt (proc.engine.cpuLoad.load() * 100)) + "% · " + String (proc.engine.activeVoices()) + " " + T ("pVoices");
        hostLabel.setTooltip (tip); qualityBox.setTooltip (tip);
    }
}

bool HitGenEditor::keyPressed (const KeyPress& k)
{
    if (dynamic_cast<TextEditor*> (getCurrentlyFocusedComponent()) != nullptr) return false;
    if (k == KeyPress::escapeKey && proc.midiLearn.learning() >= 0) { proc.midiLearn.cancelLearn(); toast (T ("mlCancelled")); return true; }
    if (k == KeyPress::spaceKey && dynamic_cast<Button*> (getCurrentlyFocusedComponent()) == nullptr) { playBtn.triggerClick(); return true; }
    if (k.getTextCharacter() == 'n' || k.getTextCharacter() == 'N') { proc.newIdea(); return true; }
    if (k.getModifiers().isCommandDown() && tab == 3 && refPage != nullptr && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z' || k.getKeyCode() == 'Y' || k.getKeyCode() == 'y'))
    { if (k.getModifiers().isShiftDown() || k.getKeyCode() == 'Y' || k.getKeyCode() == 'y') refPage->redo(); else refPage->undo(); return true; }
    if (k.getModifiers().isCommandDown() && (k.getKeyCode() == 'S' || k.getKeyCode() == 's')) { loopTile.saveAs(); return true; }
    if (k.getModifiers().isCommandDown() && k.getKeyCode() >= '1' && k.getKeyCode() <= '7') { const int i = k.getKeyCode() - '1'; if (i < 4) setWorkspace (i); else setTab (i - 4); return true; }
    if (k == KeyPress::escapeKey && settingsOpen) { toggleSettings (false); return true; }
    return false;
}

void HitGenEditor::paint (Graphics& g) { g.fillAll (lnf.pal().bg); }

void HitGenEditor::Content::paint (Graphics& g)
{
    auto& ed = *owner; const auto& P = ed.lnf.pal();
    g.fillAll (P.bg);
    // derin çivit atmosfer: üstte ve sağ altta çok hafif iki ışık havuzu (durağan; animasyon yok)
    { ColourGradient a (P.accent.withAlpha (0.10f), kW * 0.5f, 120.0f, P.accent.withAlpha (0.0f), kW * 0.5f + 620.0f, 120.0f, true); g.setGradientFill (a); g.fillRect (0, 0, kW, kH); }
    { ColourGradient b (P.track[3].withAlpha (0.05f), kW * 0.92f, (float) kH, P.track[3].withAlpha (0.0f), kW * 0.92f - 520.0f, (float) kH, true); g.setGradientFill (b); g.fillRect (0, 0, kW, kH); }
    // ince başlık ve gezinme çizgileri
    g.setColour (P.line); g.fillRect (0, 52, kW, 1); g.fillRect (0, 90, kW, 1);
    if (ed.tab == 0)
        for (auto r : { Rectangle<float> (8, 98, 340, 654), Rectangle<float> (352, 98, 820, 386), Rectangle<float> (352, 488, 820, 264) })
        {
            const auto rr = ed.rtl ? r.withX (kW - r.getRight()) : r;
            g.setColour (P.panel); g.fillRoundedRectangle (rr, 12); g.setColour (P.line); g.drawRoundedRectangle (rr.reduced (0.5f), 12, 1);
        }
    // marka: WAVEJACKERS / HITGEN, altında ürün tanımı
    const float bx = ed.rtl ? kW - 300.0f : 20.0f; const auto jl = ed.rtl ? Justification::centredRight : Justification::centredLeft;
    g.setFont (ed.lnf.font (19, true)); g.setColour (P.ink); g.drawText ("WAVEJACKERS", Rectangle<float> (bx, 6, 160, 26), jl);
    g.setColour (P.accent); g.drawText ("/ HITGEN", Rectangle<float> (ed.rtl ? bx - 100 : bx + 148, 6, 110, 26), jl);
    g.setFont (ed.lnf.font (10.5f)); g.setColour (P.mute); g.drawText ("ARTIST SOUND DNA ENGINE", Rectangle<float> (bx, 31, 280, 16), jl);
    // çıkış seviyesi: tavan -1 dBFS (0.891); tepe tutma çizgisi ile
    {
        Rectangle<float> m (556, 24, 120, 4); if (ed.rtl) m = m.withX (kW - 556 - m.getWidth());
        g.setColour (P.line); g.fillRoundedRectangle (m, 2.0f);
        const float lv = jlimit (0.0f, 1.0f, ed.meter / .891f), hv = jlimit (0.0f, 1.0f, ed.meterHold / .891f);
        g.setColour (lv > 0.95f ? P.warning : P.accent); g.fillRoundedRectangle (m.withWidth (jmax (2.0f, m.getWidth() * lv)), 2.0f);
        g.setColour (P.mute); g.fillRect (m.getX() + m.getWidth() * hv - 1, m.getY() - 1, 2.0f, m.getHeight() + 2);
    }
    // MIDI Learn bildirimi: altta ortada küçük bir hap (öğrenirken kalıcı)
    if (ed.midiToast.isNotEmpty())
    {
        const bool learning = ed.proc.midiLearn.learning() >= 0;
        const auto f = ed.lnf.font (12.5f, true); const float w = jmin ((float) kW - 80.0f, GlyphArrangement::getStringWidth (f, ed.midiToast) + 48.0f);
        auto pill = Rectangle<float> (w, 34.0f).withCentre ({ kW * 0.5f, (float) kH - 30.0f });
        g.setColour (P.panel2.withAlpha (0.97f)); g.fillRoundedRectangle (pill, 17.0f); g.setColour (learning ? P.accent : P.line); g.drawRoundedRectangle (pill.reduced (0.5f), 17.0f, learning ? 1.6f : 1.0f);
        g.setColour (learning ? P.accent : P.success); g.fillEllipse (Rectangle<float> (8, 8).withCentre ({ pill.getX() + 20, pill.getCentreY() }));
        g.setColour (P.ink); g.setFont (f); g.drawText (ed.midiToast, pill.withTrimmedLeft (34).withTrimmedRight (14), Justification::centredLeft, true);
    }
}

void HitGenEditor::layoutContent()
{
    layoutList.clear();
    // başlık (y 0-52): marka solda; çal / BPM / seviye ortada; ön ayar, kaydet, ayarlar, yardım sağda
    place (tagline, { 0, 0, 0, 0 });
    place (playBtn, { 330, 10, 100, 32 }); place (hostLabel, { 438, 10, 112, 32 });
    place (presetBtn, { 700, 10, 214, 32 }); place (saveBtn, { 920, 10, 64, 32 }); place (settingsBtn, { 990, 10, 94, 32 }); place (helpBtn, { 1090, 10, 66, 32 });
    // gezinme (y 56-90): dört çalışma alanı, ayraç, üç stüdyo sayfası
    for (int i = 0; i < 4; ++i) place (tabBtns[(size_t) i], { 16 + i * 132, 56, 128, 34 });
    for (int i = 0; i < 3; ++i) place (tabBtns[(size_t) (4 + i)], { 570 + i * 112, 56, 108, 34 });
    place (seed, { 930, 58, 110, 30 }); place (newBtn, { 1046, 58, 110, 30 }); place (seedLabel, { 0, 0, 0, 0 });
    // ayarlar paneli (yalnızca açıkken görünür)
    settingsPanelBounds = Rectangle<int> (760, 94, 396, 232);
    place (midiBtn, { 780, 254, 360, 28 });
    place (langLabel, { 780, 108, 110, 28 }); place (langBox, { 900, 108, 240, 28 });
    place (qualityLabel, { 780, 144, 110, 28 }); place (qualityBox, { 900, 144, 240, 28 });
    place (sizeLabel, { 0, 0, 0, 0 }); place (sizeBox, { 900, 180, 240, 28 }); place (contrastBtn, { 780, 218, 180, 28 }); place (motionBtn, { 968, 218, 172, 28 });
    for (int i = 0; i < 8; ++i) place (groupBtns[(size_t) i], { 16 + (i % 3) * 110, 106 + (i / 3) * 32, 104, 28 });
    place (styleList, { 16, 204, 324, 218 });
    place (varLabel, { 16, 428, 324, 18 });
    for (int i = 0; i < 4; ++i) place (varBtns[(size_t) i], { 16 + (i % 2) * 164, 448 + (i / 2) * 38, 160, 34 });
    for (int i = 0; i < 6; ++i) place (genKnobs[(size_t) i], { 16 + (i % 3) * 108, 528 + (i / 3) * 88, 108, 86 });
    place (keyLabel, { 16, 704, 100, 14 }); place (keyBox, { 16, 718, 96, 28 });
    place (scaleLabel, { 118, 704, 130, 14 }); place (scaleBox, { 118, 718, 132, 28 });
    place (barsLabel, { 256, 704, 84, 14 }); place (barsBox, { 256, 718, 84, 28 });
    place (chords, { 362, 106, 800, 44 });
    for (int t = 0; t < 4; ++t)
    {
        const int y0 = 158 + t * 80;
        place (trackLabels[(size_t) t], { 362, y0 + 2, 84, 22 }); place (altBtns[(size_t) t], { 452, y0 + 1, 32, 26 }); place (muteBtns[(size_t) t], { 486, y0 + 2, 102, 24 });
        place (soundBoxes[(size_t) t], { 362, y0 + 28, 226, 24 }); place (tiles[(size_t) t], { 362, y0 + 54, 226, 22 });
    }
    place (roll, { 596, 158, 566, 320 });
    place (soundLabel, { 362, 496, 180, 26 });
    for (int i = 0; i < 3; ++i) place (partBtns[(size_t) i], { 546 + i * 112, 494, 106, 28 });
    for (int i = 0; i < 8; ++i) place (partKnobs[(size_t) i], { 362 + i * 100, 528, 100, 86 });
    place (patternBtn, { 362, 620, 190, 26 }); place (midiOutBtn, { 556, 620, 150, 26 });
    place (keysToLabel, { 712, 620, 120, 26 }); place (keysToBox, { 834, 620, 150, 26 }); place (seventhsBtn, { 990, 620, 172, 26 });
    place (loopTile, { 362, 656, 250, 40 }); place (fullTile, { 618, 656, 250, 40 });
    place (status, { 876, 652, 290, 48 });
    place (disclaimer, { 362, 704, 800, 44 });
    for (auto& [c, r] : layoutList) c->setBounds (rtl ? r.withX (kW - r.getRight()) : r);   // sağdan sola dilde yerleşim aynalanır
    if (soundPage) soundPage->setBounds (8, 98, SoundPage::kW, SoundPage::kH);
    if (presetPage) presetPage->setBounds (8, 98, PresetPage::kW, PresetPage::kH);
    if (refPage) refPage->setBounds (8, 98, ReferencePage::kW, ReferencePage::kH);
    settingsPanel.setBounds (settingsPanelBounds);
    sizeLabel.setVisible (false);
}

void HitGenEditor::resized()
{
    const float s = getWidth() / (float) kW;
    content.setBounds (0, 0, kW, kH);
    content.setTransform (AffineTransform::scale (s));
    layoutContent();
}

static bool isAudioName (const String& n) { return n.endsWithIgnoreCase (".wav") || n.endsWithIgnoreCase (".aif") || n.endsWithIgnoreCase (".aiff") || n.endsWithIgnoreCase (".mp3") || n.endsWithIgnoreCase (".flac") || n.endsWithIgnoreCase (".ogg"); }
bool HitGenEditor::isInterestedInFileDrag (const StringArray& files) { for (auto& f : files) if (isAudioName (f)) return true; return false; }
void HitGenEditor::filesDropped (const StringArray& files, int, int)
{
    for (auto& f : files) if (isAudioName (f)) { setTab (3); refPage->openFile (File (f)); return; }
}

// ---------------------------------------------------------------- MIDI Learn (arayüz)
void HitGenEditor::toast (const String& s, int ms) { midiToast = s; midiToastUntil = Time::currentTimeMillis() + ms; content.repaint(); }

static String ccLabel (const hg::MidiLearn::Map& m) { return "CC " + String (m.cc) + " / ch " + String (m.channel); }

void HitGenEditor::mouseDown (const MouseEvent& e)
{
    if (! e.mods.isPopupMenu()) return;
    for (auto* c = e.eventComponent; c != nullptr && c != this; c = c->getParentComponent())
    {
        const auto id = c->getProperties().getWithDefault ("hgParam", var()).toString();
        if (id.isNotEmpty() && c->isEnabled()) { showMidiMenu (*c, id); return; }
    }
}

void HitGenEditor::showMidiMenu (Component& target, const String& paramId)
{
    auto& ml = proc.midiLearn; const int idx = ml.indexOf (paramId); if (idx < 0) return;
    const auto m = ml.mappingFor (idx); const bool learningThis = ml.learning() == idx;
    PopupMenu menu; menu.setLookAndFeel (&lnf);
    menu.addSectionHeader (ml.nameOf (idx));
    menu.addItem (1, learningThis ? T ("mlCancel") : T ("mlLearn"));
    if (m.param >= 0) menu.addItem (2, T ("mlRemove").replace ("{cc}", ccLabel (m)));
    menu.addSeparator(); menu.addItem (3, T ("mlAll"));
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&target), [this, idx, learningThis] (int r)
    {
        auto& ml2 = proc.midiLearn;
        if (r == 1)
        {
            if (learningThis) { ml2.cancelLearn(); toast (T ("mlCancelled")); }
            else { midiSeenLearned = ml2.lastLearned(); ml2.startLearn (idx); midiToast = T ("mlWaiting").replace ("{p}", ml2.nameOf (idx)); midiToastUntil = Time::currentTimeMillis() + 600000; content.repaint(); }
        }
        else if (r == 2) { ml2.removeMapping (idx); toast (T ("mlRemoved")); }
        else if (r == 3) showMidiMappings();
    });
}

void HitGenEditor::showMidiMappings()
{
    auto& ml = proc.midiLearn; const auto maps = ml.all();
    PopupMenu menu; menu.setLookAndFeel (&lnf);
    menu.addSectionHeader (T ("mlTitle"));
    if (maps.empty()) menu.addItem (-1, T ("mlNone"), false);
    for (size_t i = 0; i < maps.size(); ++i)
    {
        PopupMenu sub; sub.addItem (1000 + (int) i, T ("mlRemove").replace ("{cc}", ccLabel (maps[i])));
        sub.addItem (2000 + (int) i, T ("mlLearn"));
        menu.addSubMenu (ml.nameOf (maps[i].param) + "    " + ccLabel (maps[i]), sub);
    }
    menu.addSeparator();
    menu.addItem (1, T ("mlClear"), ! maps.empty());
    menu.addItem (2, T ("mlSaveDef"), ! maps.empty());
    menu.addItem (3, T ("mlLoadDef"), hg::MidiLearn::defaultFile().existsAsFile());
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&midiBtn), [this, maps] (int r)
    {
        auto& ml2 = proc.midiLearn;
        if (r == 1) { ml2.clear(); toast (T ("mlRemoved")); }
        else if (r == 2) toast (T (ml2.saveDefault() ? "mlSavedDef" : "rfSaveFail"));
        else if (r == 3) { const bool ok = ml2.loadDefault(); toast (ok ? T ("mlLoadedDef").replace ("{n}", String ((int) ml2.all().size())) : T ("rfSaveFail")); }
        else if (r >= 1000 && r < 1000 + (int) maps.size()) { ml2.removeMapping (maps[(size_t) (r - 1000)].param); toast (T ("mlRemoved")); }
        else if (r >= 2000 && r < 2000 + (int) maps.size()) { const int p = maps[(size_t) (r - 2000)].param; midiSeenLearned = ml2.lastLearned(); ml2.startLearn (p); midiToast = T ("mlWaiting").replace ("{p}", ml2.nameOf (p)); midiToastUntil = Time::currentTimeMillis() + 600000; content.repaint(); }
    });
}

// Etiketli kaydırıcılara CC rozeti / öğrenme halkası; yeni bir eşleme öğrenildiyse bildirim
void HitGenEditor::refreshMidiBadges()
{
    auto& ml = proc.midiLearn; midiShownVer = ml.version(); const int learning = ml.learning();
    std::function<void (Component&)> walk = [&] (Component& c)
    {
        const auto id = c.getProperties().getWithDefault ("hgParam", var()).toString();
        if (id.isNotEmpty())
        {
            const int idx = ml.indexOf (id); const auto m = ml.mappingFor (idx);
            const var cc = m.param >= 0 ? var ("CC" + String (m.cc)) : var(); const bool lrn = idx >= 0 && idx == learning;
            auto& pr = c.getProperties();
            if (pr.getWithDefault ("hgCC", var()).toString() != cc.toString() || (bool) pr.getWithDefault ("hgLearn", false) != lrn)
            {
                if (cc.isVoid()) pr.remove ("hgCC"); else pr.set ("hgCC", cc);
                pr.set ("hgLearn", lrn); c.repaint();
                if (m.param >= 0) c.setHelpText (ml.nameOf (idx) + ": " + ccLabel (m));
            }
        }
        for (auto* ch : c.getChildren()) walk (*ch);
    };
    walk (content);
    if (learning >= 0) { midiToast = T ("mlWaiting").replace ("{p}", ml.nameOf (learning)); midiToastUntil = Time::currentTimeMillis() + 600000; content.repaint(); return; }
    const int ll = ml.lastLearned();
    if (ll >= 0 && ll != midiSeenLearned && learning < 0)
    {
        midiSeenLearned = ll; const auto m = ml.mappingFor (ll);
        if (m.param >= 0) toast (T ("mlLearned").replace ("{p}", ml.nameOf (ll)).replace ("{cc}", String (m.cc)).replace ("{ch}", String (m.channel)));
    }
    else if (learning < 0 && midiToastUntil > Time::currentTimeMillis() + 60000) { midiToastUntil = Time::currentTimeMillis() + 1; }   // öğrenme başka yerden bitti
}
