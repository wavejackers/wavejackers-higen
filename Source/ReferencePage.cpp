#include "ReferencePage.h"
#include <cmath>

using namespace juce;
using namespace hg;

namespace
{
const char* const kCatKeys[4] = { "rfBass", "rfLead", "rfPad", "rfPerc" };
const char* const kPartKeys[3] = { "chords", "bass", "melody" };
int partOfCat (int cat) { return cat == catBass ? 1 : cat == catLead ? 2 : cat == catPad ? 0 : -1; }
const char* const kNote[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

String ReferencePage::fmtTime (double s) { const int m = (int) (s / 60); return String (m) + ":" + String (s - m * 60, 1).paddedLeft ('0', 4); }

Colour ReferencePage::catColour (int c) const { const auto& P = lnf.pal(); return c == catBass ? P.track[1] : c == catLead ? P.track[3] : c == catPad ? P.track[0] : P.track[2]; }

ReferencePage::ReferencePage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data)
{
    setFocusContainerType (FocusContainerType::focusContainer);
    wave.o = this; cands.o = this; candList.assign (kNumRefCats, {});
    for (auto* c : std::initializer_list<Component*> { &openBtn, &smartBtn, &playBtn, &applyBtn, &zoomIn, &zoomOut, &zoomFit, &genBtn, &prevBtn, &nextBtn, &saveDnaBtn, &loadDnaBtn, &seedNewBtn,
                                                       &loopBtn, &previewBtn, &quickBtn, &pinBtn, &abBtn, &stat, &posLabel, &regionLabel, &dnaHint, &summaryLabel, &regenBtn, &randBtn, &previewNowBtn, &savePresetBtn, &texK, &movK, &briK, &viewSynth, &viewSamp, &viewLib, &libList, &libSave, &libRename, &libDelete, &libImport, &libExport, &libFav, &libUseBtn, &searchEd, &artistEd, &nameEd, &filterBox, &sTrimBtn, &sLoopSelBtn, &sSaveBtn, &sLoadBtn, &sEnable, &sLoop, &rootBox, &trS, &voiceS, &rootL, &trL, &voiceL, &sInfo, &simK, &varK, &gainS, &gainL, &seed, &seedLabel, &status, &info, &dnaInfo, &banner, &histLabel, &wave, &cands, &bar })
        addAndMakeVisible (*c);
    for (int i = 0; i < 3; ++i) { auto& b = partBtns[(size_t) i]; addAndMakeVisible (b); b.setRadioGroupId (301); b.setClickingTogglesState (true); b.onClick = [this, i] { if (partBtns[(size_t) i].getToggleState()) { part = i; proc.auditionPart.store (part); } }; }
    partBtns[1].setToggleState (true, dontSendNotification);
    openBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("rfOpen"), File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { const auto f = fc.getResult(); if (f.existsAsFile()) openFile (f); });
    };
    smartBtn.onClick = [this] { if (proc.ref.clip() != nullptr) proc.ref.analyse(); };
    playBtn.onClick = [this] { togglePlay(); };
    applyBtn.onClick = [this] { if (auto a = proc.ref.analysis()) { proc.applyRefToProject (*a); status.setText (T ("rfApplied"), dontSendNotification); } };
    zoomIn.onClick = [this] { setView (viewStart + viewLen * 0.25, viewLen * 0.5); };
    zoomOut.onClick = [this] { setView (viewStart - viewLen * 0.5, viewLen * 2.0); };
    zoomFit.onClick = [this] { if (auto c = proc.ref.clip()) setView (0, c->seconds()); };
    loopBtn.setToggleState (true, dontSendNotification); loopBtn.onClick = [this] { proc.ref.loop = loopBtn.getToggleState(); };
    previewBtn.setToggleState (true, dontSendNotification);
    gainS.setSliderStyle (Slider::LinearHorizontal); gainS.setTextBoxStyle (Slider::NoTextBox, false, 0, 0); gainS.setRange (0.0, 1.0, 0.01); gainS.setValue (0.6, dontSendNotification);
    gainS.onValueChange = [this] { proc.ref.gain = (float) gainS.getValue(); };
    simK.slider.setRange (0, 100, 1); simK.slider.setValue (75, dontSendNotification); simK.slider.setTextValueSuffix ("");
    varK.slider.setRange (0, 100, 1); varK.slider.setValue (35, dontSendNotification);
    seed.setSliderStyle (Slider::IncDecButtons); seed.setTextBoxStyle (Slider::TextBoxLeft, false, 80, 28); seed.setRange (1, 9999999, 1); seed.setValue (1, dontSendNotification);
    seedNewBtn.onClick = [this] { seed.setValue (1 + Random::getSystemRandom().nextInt (999999)); };
    genBtn.onClick = [this] { generate(); };
    prevBtn.onClick = [this] { applyHistory (histPos - 1); }; nextBtn.onClick = [this] { applyHistory (histPos + 1); };
    saveDnaBtn.onClick = [this] { saveDna(); }; loadDnaBtn.onClick = [this] { loadDna(); };
    for (auto* lb : { &status, &info, &dnaInfo, &banner, &histLabel, &gainL, &seedLabel }) { lb->setFont (lnf.font (12.5f)); lb->setJustificationType (Justification::centredLeft); }
    dnaInfo.setJustificationType (Justification::topLeft); status.setJustificationType (Justification::centredLeft);
    bar.addListener (this); bar.setAutoHide (false);
    proc.ref.onChange = [this] { refreshAfterChange(); };
    // ---- örnekleyici denetimleri
    auto& SP = proc.ref.sampler;
    for (auto& k : sKnobs) addAndMakeVisible (k);
    viewSynth.setRadioGroupId (302); viewSamp.setRadioGroupId (302); viewSynth.setClickingTogglesState (true); viewSamp.setClickingTogglesState (true); viewSynth.setToggleState (true, dontSendNotification);
    viewSynth.onClick = [this] { if (viewSynth.getToggleState()) setView (0); }; viewSamp.onClick = [this] { if (viewSamp.getToggleState()) setView (1); };
    for (int n = 0; n < 128; ++n) rootBox.addItem (MidiMessage::getMidiNoteName (n, true, true, 3) + "  (" + String (n) + ")", n + 1);
    rootBox.setSelectedId (SP.root.load() + 1, dontSendNotification); rootBox.onChange = [this] { proc.ref.sampler.root = rootBox.getSelectedId() - 1; };
    trS.setSliderStyle (Slider::IncDecButtons); trS.setTextBoxStyle (Slider::TextBoxLeft, false, 50, 28); trS.setRange (-24, 24, 1); trS.setValue (SP.transpose.load(), dontSendNotification); trS.onValueChange = [this] { proc.ref.sampler.transpose = (int) trS.getValue(); };
    voiceS.setSliderStyle (Slider::IncDecButtons); voiceS.setTextBoxStyle (Slider::TextBoxLeft, false, 40, 28); voiceS.setRange (1, 16, 1); voiceS.setValue (SP.maxVoices.load(), dontSendNotification); voiceS.onValueChange = [this] { proc.ref.sampler.maxVoices = (int) voiceS.getValue(); };
    sEnable.onClick = [this] { proc.ref.sampler.enabled = sEnable.getToggleState(); if (! sEnable.getToggleState()) proc.keyboardState.allNotesOff (1); sInfo.setText (T (sEnable.getToggleState() ? "rfSampOn" : "rfSampInfo"), dontSendNotification); };
    sLoop.onClick = [this] { proc.ref.sampler.loop = sLoop.getToggleState(); };
    sTrimBtn.onClick = [this] { trimFromSelection(); }; sLoopSelBtn.onClick = [this] { if (selB > selA + 0.02) { proc.ref.sampler.loopStart = selA; proc.ref.sampler.loopEnd = selB; sLoop.setToggleState (true, dontSendNotification); proc.ref.sampler.loop = true; status.setText (T ("rfLoopSet"), dontSendNotification); } };
    sSaveBtn.onClick = [this] { saveSampler(); }; sLoadBtn.onClick = [this] { loadSampler(); };
    struct KS { double lo, hi, mid; };
    const KS ks[6] = { { 0.001, 2.0, 0.05 }, { 0.001, 3.0, 0.3 }, { 0.0, 1.0, 0.5 }, { 0.003, 6.0, 0.4 }, { 0.0, 1.5, 0.8 }, { 0.0, 1.0, 0.5 } };
    for (int i = 0; i < 6; ++i)
    {
        auto& s = sKnobs[(size_t) i].slider; s.setRange (ks[i].lo, ks[i].hi, 0.001); if (i == 0 || i == 1 || i == 3) s.setSkewFactorFromMidPoint (ks[i].mid);
        s.setValue (i == 0 ? SP.a.load() : i == 1 ? SP.d.load() : i == 2 ? SP.s.load() : i == 3 ? SP.r.load() : i == 4 ? SP.gain.load() : SP.velSens.load(), dontSendNotification);
        s.onValueChange = [this, i] { auto& P = proc.ref.sampler; const float v = (float) sKnobs[(size_t) i].slider.getValue(); (i == 0 ? P.a : i == 1 ? P.d : i == 2 ? P.s : i == 3 ? P.r : i == 4 ? P.gain : P.velSens) = v; };
    }
    // ---- kütüphane
    libModel.o = this; libList.setModel (&libModel); libList.setRowHeight (22); libList.setColour (ListBox::backgroundColourId, lnf.pal().panel); libList.setColour (ListBox::outlineColourId, lnf.pal().line);
    viewLib.setRadioGroupId (302); viewLib.setClickingTogglesState (true); viewLib.onClick = [this] { if (viewLib.getToggleState()) setView (2); };
    filterBox.addItem ("-", 1); filterBox.addItem ("-", 2); filterBox.addItem ("-", 3); filterBox.setSelectedId (1, dontSendNotification); filterBox.onChange = [this] { libRefresh(); };
    searchEd.onTextChange = [this] { libRefresh(); }; searchEd.setFont (lnf.font (13.0f)); artistEd.setFont (lnf.font (13.0f)); nameEd.setFont (lnf.font (13.0f)); artistEd.setTextToShowWhenEmpty ("...", lnf.pal().mute); nameEd.setTextToShowWhenEmpty ("...", lnf.pal().mute); searchEd.setTextToShowWhenEmpty ("...", lnf.pal().mute);
    libSave.onClick = [this]
    {
        if (! haveDna) { status.setText (T ("rfNoDna"), dontSendNotification); return; }
        const auto nm = nameEd.getText().trim().isNotEmpty() ? nameEd.getText() : (dna.name.isNotEmpty() ? dna.name : dna.sourceFile + " " + dna.role);
        status.setText (lib.save (dna, artistEd.getText(), nm) ? T ("libSaved").replace ("{n}", nm) : T ("rfSaveFail"), dontSendNotification); libRefresh();
    };
    libRename.onClick = [this] { if (auto* e = libSel()) { const auto nm = nameEd.getText(); const auto e2 = *e; status.setText (nm.trim().isNotEmpty() && lib.rename (e2, nm) ? T ("libRenamed") : T ("libNeedName"), dontSendNotification); libRefresh(); } else status.setText (T ("libPick"), dontSendNotification); };
    libDelete.onClick = [this]
    {
        auto* e = libSel(); if (e == nullptr) { status.setText (T ("libPick"), dontSendNotification); return; }
        const auto e2 = *e; const auto opts = MessageBoxOptions::makeOptionsOkCancel (MessageBoxIconType::WarningIcon, T ("libDelete"), T ("libDeleteAsk").replace ("{n}", e2.name), T ("libDelete"), T ("libCancel"), this);
        AlertWindow::showAsync (opts, [this, e2] (int r) { if (r == 1) { status.setText (lib.remove (e2) ? T ("libDeleted") : T ("rfSaveFail"), dontSendNotification); libRefresh(); } });
    };
    libImport.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("libImport"), File::getSpecialLocation (File::userDocumentsDirectory), "*.hgdna");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::canSelectMultipleItems, [this] (const FileChooser& fc)
        { int ok = 0, bad = 0; for (auto& f : fc.getResults()) (lib.importFile (f) ? ok : bad)++; status.setText (T ("libImported").replace ("{ok}", String (ok)).replace ("{bad}", String (bad)), dontSendNotification); libRefresh(); });
    };
    libExport.onClick = [this]
    {
        auto* e = libSel(); if (e == nullptr) { status.setText (T ("libPick"), dontSendNotification); return; }
        const auto e2 = *e; chooser = std::make_unique<FileChooser> (T ("libExport"), File::getSpecialLocation (File::userDocumentsDirectory).getChildFile (e2.file.getFileName()), "*.hgdna");
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this, e2] (const FileChooser& fc) { if (fc.getResult() != File()) status.setText (lib.exportTo (e2, fc.getResult()) ? T ("rfSaved").replace ("{f}", fc.getResult().getFullPathName()) : T ("rfSaveFail"), dontSendNotification); });
    };
    libFav.onClick = [this] { if (auto* e = libSel()) { const auto e2 = *e; lib.setFavorite (e2, ! e2.favorite); libRefresh(); } else status.setText (T ("libPick"), dontSendNotification); };
    libUseBtn.onClick = [this] { libUse (libList.getSelectedRow()); };
    // ---- AI Minimal: ana eylemler vurgulu; DNA makroları yalnızca gerçek DSP parametrelerine bağlı
    for (auto* b : { &smartBtn, &playBtn, &genBtn, &quickBtn }) b->setComponentID ("primary");
    for (auto* l : { &posLabel, &regionLabel, &dnaHint, &summaryLabel }) { l->setFont (lnf.font (12.5f)); l->setColour (Label::textColourId, lnf.pal().mute); }
    posLabel.setJustificationType (Justification::centredRight); posLabel.setFont (lnf.font (14.0f, true)); posLabel.setColour (Label::textColourId, lnf.pal().ink);
    dnaHint.setJustificationType (Justification::topLeft); summaryLabel.setColour (Label::textColourId, lnf.pal().ink);
    for (auto* k : { &texK, &movK, &briK })
    {
        k->slider.setRange (0, 100, 1); k->slider.setValue (50, dontSendNotification); k->slider.setDoubleClickReturnValue (true, 50); k->slider.setEnabled (false);
        k->slider.onValueChange = [this] { applyMacros(); };
    }
    regenBtn.onClick = [this] { regenerateSame(); }; randBtn.onClick = [this] { randomizeGen(); };
    previewNowBtn.onClick = [this] { startPhrase(); }; savePresetBtn.onClick = [this] { if (onSavePreset) onSavePreset(); };
    quickBtn.onClick = [this] { quickStart(); }; pinBtn.onClick = [this] { pinA(); }; abBtn.onClick = [this] { swapAB(); }; abBtn.setEnabled (false);
    // DNA Orb stüdyosu
    addAndMakeVisible (orb); addAndMakeVisible (layers);
    orb.catColour = [this] (int k) { return catColour (k); }; layers.colour = [this] (int k) { return catColour (k); };
    orb.onOpen = [this] { openBtn.onClick(); };
    orb.onSeek = [this] (double t) { proc.ref.seekTo = t; if (! proc.ref.playing.load()) { proc.ref.regionStart = 0; proc.ref.regionEnd = 0; proc.ref.loop = loopBtn.getToggleState(); proc.ref.playing = true; relabel(); } };
    layers.cands = &candList;
    layers.onPick = [this] (int k)
    {
        const int n = (int) candList[(size_t) k].size(); if (n == 0) return;
        const int row = (k == selCat && selRow >= 0) ? (selRow + 1) % n : 0; selectCandidate (k, row);
    };
    layers.onStop = [this] { proc.ref.playing = false; relabel(); };
    {
        hgui::Knob* ks[5] = { &simK, &varK, &texK, &movK, &briK };
        for (int i = 0; i < 5; ++i) { macroRows[(size_t) i] = std::make_unique<hgui::MacroRow> (lnf, ks[i]->slider); addAndMakeVisible (*macroRows[(size_t) i]); }
    }
    // ---- 2.5
    for (auto* c : std::initializer_list<Component*> { &snapBtn, &rhythmTile, &libBatch, &gridBtn, &undoBtn, &redoBtn, &grid }) addAndMakeVisible (*c);
    snapBtn.setToggleState (true, dontSendNotification);
    rhythmTile.makeFile = [this] { return makeRhythmFile(); };
    rhythmTile.palette = [this]() -> const hgui::Palette& { return lnf.pal(); }; rhythmTile.font = [this] (float h, bool b) { return lnf.font (h, b); };
    libBatch.onClick = [this] { if (batch.running()) batch.cancel(); else startBatch(); };
    gridBtn.onClick = [this] { openGrid(); }; gridBtn.setComponentID ("primary");
    undoBtn.onClick = [this] { undo(); }; redoBtn.onClick = [this] { redo(); };
    grid.onAudition = [this] (int i)
    {
        if (i < 0 || i >= (int) grid.items.size()) return;
        proc.applySoundParams (part, grid.items[(size_t) i].v.data()); curVals = grid.items[(size_t) i].v; startPhrase();
    };
    grid.onKeep = [this] (int i) { keepVariation (i); };
    grid.onClose = [this] { closeGrid(); };
    grid.spName = [this] (int sp) { return T (spDef (sp).label); };
    batch.onFile = [this] (int i, int n, const File& f, const std::vector<TrackDna>& dnas, const String& err)
    {
        const String artist = f.getParentDirectory().getFileName();
        for (auto& d : dnas) if (lib.save (d, artist, d.name)) ++batchAdded;
        batchDoneFiles = i + 1;
        status.setText (err.isNotEmpty() ? T ("batchFail").replace ("{f}", f.getFileName())
                                         : T ("batchProg").replace ("{i}", String (i + 1)).replace ("{n}", String (n)).replace ("{f}", f.getFileName()), dontSendNotification);
        libRefresh();
    };
    batch.onDone = [this] (bool cancelled)
    {
        batchBusy = false; libRefresh(); relabel();
        status.setText (T (cancelled ? "batchCancelled" : "batchDone").replace ("{d}", String (batchAdded)).replace ("{n}", String (batchDoneFiles)), dontSendNotification);
        if (! cancelled) markCompleted();
    };
    updateUndoButtons();
    setWantsKeyboardFocus (true);
    libRefresh();
    setView (0); setWorkspace (0);
    smartBtn.setEnabled (false); playBtn.setEnabled (false); genBtn.setEnabled (false); applyBtn.setEnabled (false); prevBtn.setEnabled (false); nextBtn.setEnabled (false);
    setLanguage (lang, rtl);
}

ReferencePage::~ReferencePage() { proc.ref.onChange = nullptr; proc.ref.playing = false; bar.removeListener (this); }

void ReferencePage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r; relabel(); refreshAfterChange();
}

void ReferencePage::relabel()
{
    openBtn.setButtonText (T ("rfOpen")); smartBtn.setButtonText (T ("rfSmart")); smartBtn.setTooltip (T ("rfSmartHint"));
    playBtn.setButtonText (proc.ref.playing.load() ? T ("rfStop") : T ("rfPlay")); playBtn.setTooltip (T ("rfPlayHint"));
    applyBtn.setButtonText (T ("rfApply")); loopBtn.setButtonText (T ("rfLoop")); previewBtn.setButtonText (T ("rfPreview"));
    gainL.setText (T ("rfGain"), dontSendNotification); zoomIn.setButtonText ("+"); zoomOut.setButtonText ("-"); zoomFit.setButtonText (T ("rfFit"));
    zoomIn.setTooltip (T ("rfZoomIn")); zoomOut.setTooltip (T ("rfZoomOut"));
    genBtn.setButtonText (T ("rfGen")); genBtn.setTooltip (T ("rfGenHint")); prevBtn.setButtonText ("<"); nextBtn.setButtonText (">"); prevBtn.setTooltip (T ("rfPrev")); nextBtn.setTooltip (T ("rfNext"));
    saveDnaBtn.setButtonText (T ("rfSaveDna")); loadDnaBtn.setButtonText (T ("rfLoadDna")); seedNewBtn.setButtonText ("*"); seedNewBtn.setTooltip (T ("rfNewSeed"));
    seedLabel.setText (T ("seed"), dontSendNotification); regionLabel.setText (selB > selA + 0.02 ? T ("rfRegion").replace ("{a}", fmtTime (selA)).replace ("{b}", fmtTime (selB)).replace ("{d}", String (selB - selA, 1) + " s") : T ("rfRegionNone"), dontSendNotification);
    viewSynth.setButtonText (T ("rfViewSynth")); viewSamp.setButtonText (T ("rfViewSamp")); sEnable.setButtonText (T ("rfSampEnable")); sLoop.setButtonText (T ("rfLoop"));
    rootL.setText (T ("rfRoot"), dontSendNotification); trL.setText (T ("rfTranspose"), dontSendNotification); voiceL.setText (T ("rfVoices"), dontSendNotification);
    sTrimBtn.setButtonText (T ("rfTrim")); sLoopSelBtn.setButtonText (T ("rfLoopSel")); sSaveBtn.setButtonText (T ("rfSampSave")); sLoadBtn.setButtonText (T ("rfSampLoad"));
    quickBtn.setButtonText (T ("rfQuick")); quickBtn.setTooltip (T ("rfQuickHint")); pinBtn.setButtonText (T ("rfPinA")); pinBtn.setTooltip (T ("rfABHint")); abBtn.setButtonText (abOnA ? "B" : "A/B"); abBtn.setTooltip (T ("rfABHint"));
    regenBtn.setButtonText (T ("gnRegen")); regenBtn.setTooltip (T ("gnRegenHint")); randBtn.setButtonText (T ("gnRandom")); randBtn.setTooltip (T ("gnRandomHint"));
    previewNowBtn.setButtonText (T ("gnPreview")); savePresetBtn.setButtonText (T ("gnSavePreset")); genBtn.setButtonText (T ("gnMain")); genBtn.setTooltip (T ("rfGenHint"));
    simK.setText (T ("dnaSim"), T ("rfSimHint")); varK.setText (T ("dnaVar"), T ("rfVarHint"));
    texK.setText (T ("dnaTex"), T ("dnaTexHint")); movK.setText (T ("dnaMov"), T ("dnaMovHint")); briK.setText (T ("dnaBri"), T ("dnaBriHint"));
    dnaHint.setText (haveBase ? T ("dnaHintOn") : T ("dnaHintOff"), dontSendNotification);
    snapBtn.setButtonText (T ("snapBars")); snapBtn.setTooltip (T ("snapHint"));
    rhythmTile.label = T ("rhythmTile"); rhythmTile.hint = T ("rhythmHint"); rhythmTile.setTooltip (T ("rhythmHint")); rhythmTile.setTitle (T ("rhythmTile")); rhythmTile.repaint();
    libBatch.setButtonText (T (batchBusy ? "libBatchCancel" : "libBatch")); libBatch.setTooltip (T ("libBatchHint"));
    gridBtn.setButtonText (T ("gridBtn")); gridBtn.setTooltip (T ("gridHint"));
    undoBtn.setButtonText (T ("undo")); redoBtn.setButtonText (T ("redo"));
    undoBtn.setTooltip ((SystemStats::getOperatingSystemType() & SystemStats::MacOSX) != 0 ? "Cmd+Z" : "Ctrl+Z"); redoBtn.setTooltip ((SystemStats::getOperatingSystemType() & SystemStats::MacOSX) != 0 ? "Cmd+Shift+Z" : "Ctrl+Shift+Z");
    grid.title = T ("gridTitle"); grid.hint = T ("gridHint"); grid.keepWord = T ("gridKeep"); grid.closeWord = T ("gridClose"); grid.seedWord = T ("seed"); grid.rtl = rtl;
    {
        const char* nk[5] = { "dnaSim", "dnaVar", "dnaTex", "dnaMov", "dnaBri" }; const char* hk[5] = { "rfSimHint", "rfVarHint", "dnaTexHint", "dnaMovHint", "dnaBriHint" };
        for (int i = 0; i < 5; ++i) if (macroRows[(size_t) i]) { macroRows[(size_t) i]->setName (T (nk[i]), T (hk[i])); macroRows[(size_t) i]->rtl = rtl; }
        layers.name = [this] (int k) { return T (kCatKeys[k]); }; layers.noneText = T ("rfNone"); layers.ofWord = T ("scCands"); layers.rtl = rtl; orb.rtl = rtl;
        orb.emptyText = T ("rfDropHint"); orb.topText = T (kPartKeys[part]); orb.bpmWord = "BPM";
    }
    viewLib.setButtonText (T ("rfViewLib")); libSave.setButtonText (T ("libSave")); libRename.setButtonText (T ("libRename")); libDelete.setButtonText (T ("libDelete")); libImport.setButtonText (T ("libImport")); libExport.setButtonText (T ("libExport")); libFav.setButtonText (T ("libFav")); libUseBtn.setButtonText (T ("libUse"));
    searchEd.setTextToShowWhenEmpty (T ("libSearch"), lnf.pal().mute); artistEd.setTextToShowWhenEmpty (T ("libArtist"), lnf.pal().mute); nameEd.setTextToShowWhenEmpty (T ("libName"), lnf.pal().mute);
    { const int id = filterBox.getSelectedId(); filterBox.clear (dontSendNotification); filterBox.addItem (T ("libAll"), 1); filterBox.addItem (T ("libFavs"), 2); filterBox.addItem (T ("libRecent"), 3); filterBox.setSelectedId (id > 0 ? id : 1, dontSendNotification); }
    sTrimBtn.setTooltip (T ("rfTrimHint")); rootBox.setTooltip (T ("rfRootHint"));
    { const char* nm[6] = { "rfA", "rfD", "rfS", "rfR", "rfGain", "rfVel" }; for (int i = 0; i < 6; ++i) sKnobs[(size_t) i].setText (T (nm[i]), T (nm[i] + String ("H"))); }
    sInfo.setText (T (proc.ref.sampler.enabled.load() ? "rfSampOn" : "rfSampInfo"), dontSendNotification); sInfo.setFont (lnf.font (12.0f)); sInfo.setJustificationType (Justification::topLeft);
    for (int i = 0; i < 3; ++i) partBtns[(size_t) i].setButtonText (T (kPartKeys[i]));
    simK.setText (T ("rfSim"), T ("rfSimHint")); varK.setText (T ("rfVar"), T ("rfVarHint"));
    for (auto* lb : { &status, &info, &dnaInfo, &banner, &histLabel, &gainL, &seedLabel }) lb->setJustificationType (rtl ? Justification::centredRight : Justification::centredLeft);
    repaint();
}

void ReferencePage::openFile (const File& f)
{
    proc.ref.playing = false; selA = selB = 0; selCat = -1; candList.assign (kNumRefCats, {}); haveDna = false; dnaA = dnaB = -1; dnaInfo.setText ({}, dontSendNotification);
    status.setText (T ("rfLoading"), dontSendNotification); proc.ref.load (f);
}

void ReferencePage::refreshAfterChange()
{
    const int st = proc.ref.state.load(); if (quickPending) quickStep(); auto c = proc.ref.clip(); auto a = proc.ref.analysis();
    smartBtn.setEnabled (c != nullptr && st != RefEngine::Loading && st != RefEngine::Analysing);
    playBtn.setEnabled (c != nullptr); genBtn.setEnabled (c != nullptr || haveDna); applyBtn.setEnabled (a != nullptr && a->ok && a->bpm > 0);
    const int cid = c ? (int) (c->numSamples() ^ (int) c->seconds()) : 0;
    if (cid != shownClipId) { shownClipId = cid; if (c) setView (0, c->seconds()); else setView (0, 10); }
    candList.assign (kNumRefCats, {});
    if (a && a->ok) for (int k = 0; k < kNumRefCats; ++k) candList[(size_t) k] = a->top (k, 4);
    String line;
    if (c) line << c->name << " · " << fmtTime (c->seconds()) << " · " << (int) c->sampleRate << " Hz · " << (c->audio.getNumChannels() > 1 ? "stereo" : "mono");
    if (a && a->ok)
    {
        line << "\n";
        if (a->bpm > 0) line << "BPM " << String (a->bpm, 1) << " (" << T ("rfConf") << " " << roundToInt (a->bpmConf * 100) << "%)  ·  "; 
        line << T ("rfKey") << ": " << kNote[a->key] << (a->minor ? " m" : " M") << " (" << T ("rfConf") << " " << roundToInt (a->keyConf * 100) << "%, " << T ("rfAlt") << " " << kNote[a->keyAlt] << (a->keyAltMinor ? " m" : " M") << ")  ·  " << a->onsets.size() << " " << T ("rfOnsets");
        if (a->notes.contains ("bpmLow")) line << "  ·  " << T ("rfBpmLow");
        if (a->notes.contains ("keyLow")) line << "  ·  " << T ("rfKeyLow");
        if (a->notes.contains ("truncated")) line << "  ·  " << T ("rfTrunc");
        if (a->notes.contains ("tooShort")) line << "  ·  " << T ("rfTooShort");
    }
    info.setText (line, dontSendNotification);
    if (st == RefEngine::Failed) status.setText (T (proc.ref.error.isEmpty() ? "fileBad" : proc.ref.error), dontSendNotification);
    else if (st == RefEngine::Loading) status.setText (T ("rfLoading"), dontSendNotification);
    else if (st == RefEngine::Analysing) status.setText (T ("rfAnalysing"), dontSendNotification);
    else if (st == RefEngine::Ready && a && a->ok && candList[0].size() + candList[1].size() + candList[2].size() + candList[3].size() == 0) status.setText (T ("rfNoRegions"), dontSendNotification);
    else if (st == RefEngine::Ready) status.setText (T ("rfDone"), dontSendNotification);
    else if (st == RefEngine::Loaded) status.setText (T ("rfLoaded"), dontSendNotification);
    else if (proc.ref.missingPath.isNotEmpty()) status.setText (T ("fileMissing") + " " + proc.ref.missingPath, dontSendNotification);
    else status.setText (T ("rfDropHint"), dontSendNotification);
    banner.setText (T ("rfBanner"), dontSendNotification);
    syncSampler(); lastState = st; wave.repaint(); cands.repaint(); syncOrb(); repaint();
}

void ReferencePage::setView (double start, double len)
{
    auto c = proc.ref.clip(); const double dur = c ? c->seconds() : 10.0;
    viewLen = jlimit (0.05, std::max (0.05, dur), len); viewStart = jlimit (0.0, std::max (0.0, dur - viewLen), start);
    bar.setRangeLimits (0.0, std::max (dur, 0.1)); bar.setCurrentRange (viewStart, viewLen, dontSendNotification); wave.repaint();
}
void ReferencePage::scrollBarMoved (ScrollBar*, double newStart) { viewStart = newStart; wave.repaint(); }

void ReferencePage::setSelection (double a, double b, int cat)
{
    selA = std::min (a, b); selB = std::max (a, b); selCat = cat;
    proc.ref.regionStart = selA; proc.ref.regionEnd = selB > selA + 0.05 ? selB : 0.0;
    const int pc = partOfCat (cat); if (pc >= 0) { part = pc; partBtns[(size_t) pc].setToggleState (true, dontSendNotification); proc.auditionPart.store (part); }
    haveDna = haveDna && dnaA == -2; wave.repaint(); cands.repaint(); if (cat < 0) selRow = -1; syncOrb();
    regionLabel.setText (selB > selA + 0.02 ? T ("rfRegion").replace ("{a}", fmtTime (selA)).replace ("{b}", fmtTime (selB)).replace ("{d}", String (selB - selA, 1) + " s") : T ("rfRegionNone"), dontSendNotification); repaint();
}

void ReferencePage::selectCandidate (int cat, int row)
{
    if (cat < 0 || cat >= (int) candList.size() || row < 0 || row >= (int) candList[(size_t) cat].size()) return;
    const auto& g = candList[(size_t) cat][(size_t) row];
    setSelection (g.t0, g.t1, cat); selRow = row; syncOrb(); proc.ref.seekTo = g.t0; proc.ref.loop = true; loopBtn.setToggleState (true, dontSendNotification); proc.ref.playing = true; relabel();
    setView (std::max (0.0, g.t0 - (g.t1 - g.t0) * 0.5), (g.t1 - g.t0) * 2.0);
}

void ReferencePage::togglePlay()
{
    if (proc.ref.clip() == nullptr) return;
    if (proc.ref.playing.load()) { proc.ref.playing = false; relabel(); return; }
    proc.ref.regionStart = selA; proc.ref.regionEnd = selB > selA + 0.05 ? selB : 0.0; proc.ref.seekTo = selB > selA + 0.05 ? selA : proc.ref.posSec.load();
    proc.ref.loop = loopBtn.getToggleState(); proc.ref.playing = true; relabel();
}

void ReferencePage::tick()
{
    const int st = proc.ref.state.load();
    if (st != lastState || st == RefEngine::Loading || st == RefEngine::Analysing) { if (st == RefEngine::Analysing) status.setText (T ("rfAnalysing") + " " + String (roundToInt (proc.ref.progress.load() * 100)) + "%", dontSendNotification); if (st != lastState) refreshAfterChange(); }
    const bool pl = proc.ref.playing.load(); if ((playBtn.getButtonText() == T ("rfStop")) != pl) relabel();
    if (pl) wave.repaint();
    updateStat(); stat.advance();
    if (macroDirtyAt > 0 && Time::currentTimeMillis() - macroDirtyAt > 450) { macroDirtyAt = 0; pushUndo(); }
    if (gridPending && errorFlag) gridPending = false;
    if (ws == 3)
    {
        orb.setPlayhead (proc.ref.posSec.load(), pl); orb.topText = T (kPartKeys[part]);
        if (layers.playing != pl) { layers.playing = pl; layers.repaint(); }
    }
    if (ws == 0) { auto cl = proc.ref.clip(); const double pos = proc.ref.posSec.load(); const String t = cl ? fmtTime (pos) + " / " + fmtTime (cl->seconds()) : String ("0:00.0 / 0:00.0"); if (posLabel.getText() != t) posLabel.setText (t, dontSendNotification); }
    if (phraseRun)
    {
        const double el = Time::getMillisecondCounterHiRes() - phraseT0; bool pending = false;
        for (auto& e : phrase) { if (e.at < 0) continue; if (e.at <= el) { if (e.on) proc.keyboardState.noteOn (1, e.note, 0.8f); else proc.keyboardState.noteOff (1, e.note, 0.0f); e.at = -1; } else pending = true; }
        if (! pending) phraseRun = false;
    }
}

void ReferencePage::startPhrase()
{
    for (auto& e : phrase) if (e.at < 0 && e.on == false) {}   // önceki cümlenin açık notaları zaten kapandı
    proc.keyboardState.allNotesOff (1);
    auto a = proc.ref.analysis(); const int root = a && a->ok ? a->key : (int) proc.apvts.getRawParameterValue ("key")->load(); const bool minor = a && a->ok ? a->minor : true;
    phrase.clear(); const int third = minor ? 3 : 4; double t = 0;
    if (part == 0) { const int b = 60 + root; for (int iv : { 0, third, 7 }) { phrase.push_back ({ t, b + iv, true }); phrase.push_back ({ t + 1100, b + iv, false }); } }
    else if (part == 1) { const int b = 36 + root; for (int iv : { 0, 0, 7, third }) { phrase.push_back ({ t, b + iv, true }); phrase.push_back ({ t + 240, b + iv, false }); t += 280; } }
    else { const int b = 72 + root; for (int iv : { 0, third, 7, 10, 7, third }) { phrase.push_back ({ t, b + iv, true }); phrase.push_back ({ t + 200, b + iv, false }); t += 230; } }
    phraseT0 = Time::getMillisecondCounterHiRes(); phraseRun = true;
}

void ReferencePage::generate()
{
    proc.auditionPart.store (part);
    if (haveDna && dnaA == -2) { doGenerate (dna); return; }          // yüklenmiş DNA profili: ses dosyası gerekmez
    auto c = proc.ref.clip(); if (c == nullptr) { errorFlag = true; status.setText (T ("rfNeedClip"), dontSendNotification); return; }
    double a = selA, b = selB; int role = selCat >= 0 ? selCat : catLead;
    if (b <= a + 0.15) { if (! candList[(size_t) catLead].empty()) { a = candList[(size_t) catLead][0].t0; b = candList[(size_t) catLead][0].t1; } else { a = 0; b = std::min (c->seconds(), 8.0); } status.setText (T ("rfAutoSel"), dontSendNotification); }
    if (haveDna && std::abs (dnaA - a) < 1e-6 && std::abs (dnaB - b) < 1e-6 && dnaRole == role && dnaClip == c->name) { doGenerate (dna); return; }
    status.setText (T ("rfExtracting"), dontSendNotification); dnaBusy = true; errorFlag = false;
    proc.ref.extractDna (a, b, role, [this, a, b, role, c] (bool ok, const TrackDna& d)
    {
        dnaBusy = false;
        if (! ok) { errorFlag = true; status.setText (T ("rfDnaFail"), dontSendNotification); return; }
        dna = d; haveDna = true; dnaA = a; dnaB = b; dnaRole = role; dnaClip = c->name; doGenerate (dna);
    });
}

void ReferencePage::doGenerate (const TrackDna& d)
{
    const juce::uint32 sd = (juce::uint32) seed.getValue(); float vals[kNumSp];
    dnaToSoundParams (d, part == 1 ? 'b' : part == 0 ? 'c' : 'm', (float) simK.slider.getValue(), (float) varK.slider.getValue(), sd, vals);
    proc.applySoundParams (part, vals); pushHistory (vals, sd); lastSeed = sd; abOnA = false;
    std::copy (vals, vals + kNumSp, baseVals.begin()); haveBase = true; for (auto* k : { &texK, &movK, &briK }) { k->slider.setValue (50, dontSendNotification); k->slider.setEnabled (true); }
    errorFlag = false; markCompleted(); dnaHint.setText (T ("dnaHintOn"), dontSendNotification);
    summaryLabel.setText (T ("gnSummary").replace ("{sim}", String ((int) simK.slider.getValue())).replace ("{var}", String ((int) varK.slider.getValue())).replace ("{seed}", String ((int) sd)).replace ("{p}", T (kPartKeys[part])), dontSendNotification); abBtn.setEnabled (haveA); abBtn.setButtonText ("A/B");
    seed.setValue (sd + 1, dontSendNotification); seed.setValue (sd + 1);       // sonraki tıklama farklı bir varyasyon verir; kullanılan tohum geçmişte saklı
    auto F = [] (float v, int dp) { return dp == 0 ? String (roundToInt (v)) : String (v, dp); };
    dnaInfo.setText (T ("rfDnaInfo").replace ("{f0}", d.pitched ? F (d.f0Hz, 1) + " Hz" : T ("rfUnpitched")).replace ("{cent}", F (d.centroidHz, 0)).replace ("{atk}", F (d.attackMs, 0)).replace ("{sus}", F (d.sustainLevel, 2))
                         .replace ("{wid}", F (d.width, 2)).replace ("{noi}", F (d.noise, 2)).replace ("{dur}", fmtTime (d.regionEnd - d.regionStart)).replace ("{src}", d.sourceFile), dontSendNotification);
    status.setText (T ("rfGenerated").replace ("{p}", T (kPartKeys[part])).replace ("{s}", String ((int) sd)), dontSendNotification);
    curVals = baseVals; pushUndo();
    if (gridPending) { gridPending = false; buildGrid(); }
    else if (previewBtn.getToggleState()) startPhrase();
    repaint();
}

void ReferencePage::pushHistory (const float* vals, juce::uint32 sd)
{
    if (histPos + 1 < (int) history.size()) history.resize ((size_t) histPos + 1);
    Hist h; std::copy (vals, vals + kNumSp, h.v.begin()); h.seed = sd; history.push_back (h);
    if (history.size() > 40) history.erase (history.begin());
    histPos = (int) history.size() - 1; prevBtn.setEnabled (histPos > 0); nextBtn.setEnabled (false);
    histLabel.setText (T ("rfHist") + " " + String (histPos + 1) + "/" + String ((int) history.size()) + "  (seed " + String ((int) sd) + ")", dontSendNotification);
}

void ReferencePage::applyHistory (int pos)
{
    if (pos < 0 || pos >= (int) history.size()) return;
    histPos = pos; proc.applySoundParams (part, history[(size_t) pos].v.data());
    baseVals = history[(size_t) pos].v; haveBase = true; for (auto* k : { &texK, &movK, &briK }) { k->slider.setValue (50, dontSendNotification); k->slider.setEnabled (true); }
    prevBtn.setEnabled (histPos > 0); nextBtn.setEnabled (histPos + 1 < (int) history.size());
    histLabel.setText (T ("rfHist") + " " + String (histPos + 1) + "/" + String ((int) history.size()) + "  (seed " + String ((int) history[(size_t) pos].seed) + ")", dontSendNotification);
    curVals = baseVals; pushUndo();
    if (previewBtn.getToggleState()) startPhrase();
}

static File dnaFolder() { auto d = File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("DNA"); d.createDirectory(); return d; }

void ReferencePage::saveDna()
{
    if (! haveDna) { status.setText (T ("rfNoDna"), dontSendNotification); return; }
    chooser = std::make_unique<FileChooser> (T ("rfSaveDna"), dnaFolder().getChildFile (File::createLegalFileName (dna.name.isNotEmpty() ? dna.name : dna.sourceFile + "_" + dna.role) + ".hgdna"), "*.hgdna");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult(); if (f == File()) return; f = f.withFileExtension ("hgdna"); dna.name = f.getFileNameWithoutExtension();
        status.setText (f.replaceWithText (JSON::toString (dna.toVar())) ? T ("rfSaved").replace ("{f}", f.getFullPathName()) : T ("rfSaveFail"), dontSendNotification);
    });
}

void ReferencePage::loadDna()
{
    chooser = std::make_unique<FileChooser> (T ("rfLoadDna"), dnaFolder(), "*.hgdna");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    {
        const auto f = fc.getResult(); if (! f.existsAsFile()) return;
        TrackDna d; if (f.getSize() > 1000000 || ! TrackDna::fromVar (JSON::parse (f.loadFileAsString()), d)) { status.setText (T ("rfDnaBad"), dontSendNotification); return; }
        dna = d; haveDna = true; dnaA = dnaB = -2; genBtn.setEnabled (true);
        const int pc = d.role == "bass" ? 1 : d.role == "pad" ? 0 : d.role == "lead" ? 2 : part; part = pc; partBtns[(size_t) pc].setToggleState (true, dontSendNotification);
        status.setText (T ("rfDnaLoaded").replace ("{n}", d.name), dontSendNotification);
    });
}

// ---------------------------------------------------------------- yerleşim ve çizim
void ReferencePage::resized()
{
    auto P = [this] (Component& cm, Rectangle<int> r) { cm.setBounds (rtl ? r.withX (kW - r.getRight()) : r); };
    // ortak: durum göstergesi (sol) ve ileti satırı
    auto statusRow = [&] (int y) { P (stat, { 16, y, 120, 24 }); P (status, { 140, y, kW - 156, 22 }); };
    if (ws == 0)
    {
        P (openBtn, { 16, 8, 150, 36 }); P (playBtn, { 174, 8, 150, 36 }); P (loopBtn, { 334, 8, 88, 36 }); P (gainL, { 430, 8, 46, 36 }); P (gainS, { 478, 8, 104, 36 });
        P (posLabel, { 590, 8, 140, 36 }); P (applyBtn, { 1004, 8, 144, 36 }); P (quickBtn, { 1004, 560, 144, 36 });
        P (info, { 16, 52, kW - 32, 22 }); statusRow (76);
        P (wave, { 16, 104, kW - 32, 420 }); P (bar, { 16, 528, kW - 32, 12 });
        P (zoomOut, { 16, 548, 40, 28 }); P (zoomIn, { 60, 548, 40, 28 }); P (zoomFit, { 104, 548, 84, 28 }); P (snapBtn, { 200, 548, 190, 28 }); P (regionLabel, { 398, 548, 390, 28 }); P (rhythmTile, { 796, 560, 200, 36 });
    }
    else if (ws == 1)
    {
        P (smartBtn, { 16, 8, 260, 44 }); P (playBtn, { 284, 8, 140, 44 }); P (banner, { 440, 6, kW - 456, 48 }); P (stat, { 16, 58, 120, 24 }); P (status, { 140, 58, 790, 22 }); P (rhythmTile, { 948, 52, 200, 32 });
        P (wave, { 16, 90, kW - 32, 130 });
        P (cands, { 16, 232, kW - 32, 400 });
    }
    else if (ws == 2)
    {
        for (int i = 0; i < 3; ++i) P (partBtns[(size_t) i], { 16 + i * 112, 8, 108, 34 });
        P (viewSynth, { 700, 8, 140, 34 }); P (viewSamp, { 846, 8, 140, 34 }); P (viewLib, { 992, 8, 156, 34 });
        statusRow (48); P (wave, { 16, 76, kW - 32, 110 });
        const int y0 = 198;
        if (view == 0)
        {
            const int cw = (kW - 32) / 5;
            hgui::Knob* ks[5] = { &simK, &varK, &texK, &movK, &briK };
            for (int i = 0; i < 5; ++i) P (*ks[i], { 16 + i * cw + 20, y0 + 6, cw - 40, 150 });
            P (dnaHint, { 16, y0 + 168, kW - 32, 40 }); P (dnaInfo, { 16, y0 + 214, kW - 32, 50 });
            P (saveDnaBtn, { 16, y0 + 280, 160, 34 }); P (loadDnaBtn, { 184, y0 + 280, 160, 34 });
        }
        else if (view == 1)
        {
            P (sEnable, { 16, y0 + 6, 220, 34 }); P (rootL, { 252, y0 + 6, 70, 34 }); P (rootBox, { 322, y0 + 6, 140, 34 }); P (trL, { 478, y0 + 6, 100, 34 }); P (trS, { 578, y0 + 6, 130, 34 });
            P (voiceL, { 724, y0 + 6, 80, 34 }); P (voiceS, { 804, y0 + 6, 120, 34 }); P (sLoop, { 940, y0 + 6, 100, 34 }); P (sTrimBtn, { 1048, y0 + 6, 100, 34 });
            for (int i = 0; i < 6; ++i) P (sKnobs[(size_t) i], { 16 + i * 130, y0 + 56, 120, 130 });
            P (sLoopSelBtn, { 812, y0 + 60, 200, 34 }); P (sSaveBtn, { 812, y0 + 102, 200, 34 }); P (sLoadBtn, { 812, y0 + 144, 200, 34 }); P (sInfo, { 16, y0 + 200, kW - 32, 70 });
        }
        else
        {
            P (searchEd, { 16, y0 + 6, 340, 32 }); P (filterBox, { 364, y0 + 6, 150, 32 }); P (libList, { 16, y0 + 48, 498, 330 });
            P (artistEd, { 534, y0 + 6, 300, 32 }); P (nameEd, { 842, y0 + 6, 306, 32 });
            P (libSave, { 534, y0 + 48, 614, 34 });
            P (libUseBtn, { 534, y0 + 94, 148, 34 }); P (libFav, { 690, y0 + 94, 148, 34 }); P (libRename, { 846, y0 + 94, 148, 34 }); P (libDelete, { 1002, y0 + 94, 146, 34 });
            P (libImport, { 534, y0 + 138, 148, 34 }); P (libExport, { 690, y0 + 138, 148, 34 }); P (libBatch, { 846, y0 + 138, 302, 34 });
        }
    }
    else
    {
        // DNA Orb stüdyosu: sol makrolar, ortada halka, sağda katmanlar, altta üretim yuvası
        for (int i = 0; i < 3; ++i) P (partBtns[(size_t) i], { 16 + i * 100, 10, 96, 32 });
        P (stat, { 340, 14, 120, 24 }); P (status, { 462, 12, 420, 22 }); P (gridBtn, { 890, 10, 134, 32 }); P (quickBtn, { 1032, 10, 116, 32 });
        for (int i = 0; i < 5; ++i) if (macroRows[(size_t) i]) P (*macroRows[(size_t) i], { 16, 84 + i * 56, 300, 48 });
        P (seedLabel, { 16, 372, 80, 22 }); P (seed, { 16, 396, 200, 32 }); P (seedNewBtn, { 222, 396, 40, 32 }); P (previewBtn, { 16, 436, 300, 30 });
        P (orb, { 342, 50, 480, 480 }); P (grid, { 342, 50, 480, 480 });
        P (layers, { 848, 150, 300, 224 });
        P (prevBtn, { 848, 392, 40, 32 }); P (nextBtn, { 892, 392, 40, 32 }); P (pinBtn, { 940, 392, 96, 32 }); P (abBtn, { 1044, 392, 64, 32 });
        P (histLabel, { 848, 430, 300, 22 }); P (summaryLabel, { 848, 454, 300, 40 }); P (undoBtn, { 848, 500, 146, 32 }); P (redoBtn, { 1002, 500, 146, 32 });
        P (regenBtn, { 36, 572, 170, 46 }); P (randBtn, { 214, 572, 170, 46 }); P (genBtn, { 400, 564, 364, 62 });
        P (previewNowBtn, { 780, 572, 170, 46 }); P (savePresetBtn, { 958, 572, 170, 46 });
    }
}

void ReferencePage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    g.setColour (P.panel.withAlpha (ws == 3 ? 0.55f : 0.82f)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 12); g.setColour (P.line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 12, 1);
    auto an = proc.ref.analysis();
    if (ws == 0)
    {
        // BPM ve ton: yalnızca gerçek analiz sonucu; yoksa tire
        auto chip = [&] (Rectangle<int> r, const String& k, const String& v, bool ok)
        {
            auto rr = (rtl ? r.withX (kW - r.getRight()) : r).toFloat(); g.setColour (P.panel2); g.fillRoundedRectangle (rr, 8); g.setColour (P.line); g.drawRoundedRectangle (rr.reduced (0.5f), 8, 1);
            g.setColour (P.mute); g.setFont (lnf.font (10.5f)); g.drawText (k, rr.reduced (10, 3).removeFromTop (12), Justification::centredLeft);
            g.setColour (ok ? P.ink : P.mute); g.setFont (lnf.font (15.0f, true)); g.drawText (v, rr.reduced (10, 0).withTrimmedTop (12), Justification::centredLeft);
        };
        const bool ok = an && an->ok;
        chip ({ 738, 4, 116, 44 }, "BPM", ok && an->bpm > 0 ? String (an->bpm, 1) : String (juce::CharPointer_UTF8 ("\xe2\x80\x94")), ok && an->bpm > 0);
        static const char* const kn[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        chip ({ 862, 4, 134, 44 }, T ("rfKey").toUpperCase(), ok ? String (kn[an->key]) + (an->minor ? " m" : " M") + "  " + String (roundToInt (an->keyConf * 100)) + "%" : String (juce::CharPointer_UTF8 ("\xe2\x80\x94")), ok);
    }
    if (ws == 3)
    {
        const auto jl = rtl ? Justification::centredRight : Justification::centredLeft;
        auto R = [this] (Rectangle<int> r) { return rtl ? r.withX (kW - r.getRight()) : r; };
        g.setColour (P.mute); g.setFont (lnf.font (11.0f, true)); g.drawText ("SOUND DNA", R ({ 16, 60, 300, 16 }), jl);
        g.drawText (T ("rfSmart").toUpperCase(), R ({ 848, 60, 300, 16 }), jl);
        // sağ başlık: seçili katman ya da üretilecek parça, büyük yazı; altında kaynak
        const String big = selCat >= 0 ? T (kCatKeys[selCat]) : T (kPartKeys[part]);
        g.setColour (P.ink); g.setFont (lnf.font (30.0f, true)); g.drawText (big, R ({ 848, 78, 300, 38 }), jl);
        g.setColour (P.mute); g.setFont (lnf.font (12.0f));
        auto c = proc.ref.clip(); g.drawText (c ? c->name + "  " + (selB > selA + 0.02 ? fmtTime (selA) + " - " + fmtTime (selB) : String()) : T ("rfNeedClip"), R ({ 848, 116, 300, 20 }), jl);
        // alt yuva
        auto dock = Rectangle<float> (16, 552, (float) kW - 32, 86);
        ColourGradient dg (P.panel2, 0, dock.getY(), P.panel, 0, dock.getBottom(), false); g.setGradientFill (dg); g.fillRoundedRectangle (dock, 14);
        g.setColour (P.line); g.drawRoundedRectangle (dock.reduced (0.5f), 14, 1);
        g.setColour (P.line); g.fillRect (R ({ 332, 84, 1, 380 })); g.fillRect (R ({ 834, 84, 1, 380 }));
    }
    if (ws != 3 && proc.ref.clip() == nullptr && proc.ref.state.load() != RefEngine::Loading)
    {
        g.setColour (P.accent2.withAlpha (0.6f)); g.drawRoundedRectangle (wave.getBounds().toFloat().reduced (1), 12, 1.2f);
        g.setColour (P.mute); g.setFont (lnf.font (15.0f)); g.drawText (T ("rfDropHint"), wave.getBounds().reduced (24, 0), Justification::centred);
    }
    if (ws == 2 && view == 0)
    {
        g.setColour (P.mute); g.setFont (lnf.font (12.0f)); g.drawText (T ("dnaCoreGroup"), Rectangle<int> (rtl ? kW - 600 : 16, 190, 584, 14), rtl ? Justification::centredRight : Justification::centredLeft);
    }
    if (view == 2 && ws == 2 && libRows.empty()) { g.setColour (P.mute); g.setFont (lnf.font (12.5f)); g.drawText (T ("libEmpty"), libList.getBounds().reduced (10), Justification::centred); }
}

double ReferencePage::Wave::xToSec (float x) const { return o->viewStart + (double) (x / std::max (1, getWidth())) * o->viewLen; }
float ReferencePage::Wave::secToX (double s) const { return (float) ((s - o->viewStart) / o->viewLen * getWidth()); }

void ReferencePage::Wave::paint (Graphics& g)
{
    const auto& P = o->lnf.pal(); auto c = o->proc.ref.clip(); const auto r = getLocalBounds().toFloat();
    g.setColour (P.bg); g.fillRoundedRectangle (r, 10); g.setColour (P.line); g.drawRoundedRectangle (r.reduced (0.5f), 10, 1);
    if (c == nullptr) return;
    const int W = getWidth(), H = getHeight(); const double sr = c->sampleRate; const int n = c->numSamples();
    // bölge adayları (arka plan)
    for (int k = 0; k < (int) o->candList.size(); ++k) for (auto& gg : o->candList[(size_t) k])
    { const float x0 = secToX (gg.t0), x1 = secToX (gg.t1); if (x1 < 0 || x0 > W) continue; g.setColour (o->catColour (k).withAlpha (0.13f)); g.fillRect (Rectangle<float> (x0, 0, x1 - x0, (float) H));
      g.setColour (o->catColour (k).withAlpha (0.7f)); g.fillRect (Rectangle<float> (x0, 0, x1 - x0, 3.0f)); }
    if (o->selB > o->selA + 0.01) { const float x0 = secToX (o->selA), x1 = secToX (o->selB); g.setColour (P.accent.withAlpha (0.22f)); g.fillRect (Rectangle<float> (x0, 0, x1 - x0, (float) H)); g.setColour (P.accent); g.drawLine (x0, 0, x0, (float) H, 1.4f); g.drawLine (x1, 0, x1, (float) H, 1.4f); }
    // dalga formu: piksel başına min/maks (kaydırma/yakınlaştırma için doğrudan tampondan)
    const float* L = c->audio.getReadPointer (0); const float* R = c->audio.getNumChannels() > 1 ? c->audio.getReadPointer (1) : L;
    const double spp = o->viewLen * sr / W; const int stride = std::max (1, (int) (spp / 24)); const float mid = H * 0.5f;
    Path pth; g.setColour (P.accent.withAlpha (0.85f));
    for (int x = 0; x < W; ++x)
    {
        const int a = (int) ((o->viewStart * sr) + x * spp), b = std::min (n, (int) ((o->viewStart * sr) + (x + 1) * spp) + 1); if (a >= n) break;
        float mn = 0, mx = 0; for (int i = std::max (0, a); i < b; i += stride) { const float v = 0.5f * (L[i] + R[i]); mn = std::min (mn, v); mx = std::max (mx, v); }
        const float y0 = mid - std::min (1.0f, mx) * mid * 0.94f, y1 = mid - std::max (-1.0f, mn) * mid * 0.94f; g.fillRect ((float) x, y0, 1.0f, std::max (1.0f, y1 - y0));
    }
    // onset işaretleri
    if (auto a = o->proc.ref.analysis()) { g.setColour (P.ink.withAlpha (0.35f)); for (float t : a->onsets) { const float x = secToX (t); if (x >= 0 && x < W && spp < sr * 0.12) g.fillRect (x, (float) H - 6, 1.0f, 6.0f); } }
    // zaman cetveli
    g.setColour (P.mute); g.setFont (o->lnf.font (10.5f)); const double steps[] = { 0.1, 0.25, 0.5, 1, 2, 5, 10, 15, 30, 60, 120 }; double st = 120; for (double s : steps) if (o->viewLen / s <= 12) { st = s; break; }
    for (double t = std::ceil (o->viewStart / st) * st; t < o->viewStart + o->viewLen; t += st) { const float x = secToX (t); g.fillRect (x, 3.0f, 1.0f, 6.0f); g.drawText (fmtTime (t), Rectangle<float> (x + 3, 4, 60, 12), Justification::centredLeft); }
    // çalma konumu
    const double pos = o->proc.ref.posSec.load(); if (o->proc.ref.playing.load() || pos > 0) { const float x = secToX (pos); if (x >= 0 && x <= W) { g.setColour (P.ink); g.drawLine (x, 0, x, (float) H, 1.6f); } }
}

void ReferencePage::Wave::mouseDown (const MouseEvent& e) { if (o->isShowing()) o->grabKeyboardFocus(); if (o->proc.ref.clip() == nullptr) return; dragging = false; dragA = xToSec ((float) e.x); }
void ReferencePage::Wave::mouseDrag (const MouseEvent& e)
{
    if (o->proc.ref.clip() == nullptr) return;
    if (std::abs (e.getDistanceFromDragStartX()) > 4) { dragging = true; const double d = o->proc.ref.clip()->seconds(); o->setSelection (jlimit (0.0, d, dragA), jlimit (0.0, d, xToSec ((float) e.x)), -1); }
}
void ReferencePage::Wave::mouseUp (const MouseEvent& e)
{
    if (o->proc.ref.clip() == nullptr) return;
    if (! dragging) { o->proc.ref.seekTo = jlimit (0.0, o->proc.ref.clip()->seconds(), xToSec ((float) e.x)); if (e.mods.isShiftDown()) o->setSelection (0, 0, -1); }
    else if (! e.mods.isAltDown()) o->snapSelection();            // Alt: hizalamadan serbest seçim
    dragging = false; o->repaint();
}
void ReferencePage::Wave::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (o->proc.ref.clip() == nullptr) return;
    if (std::abs (w.deltaX) > std::abs (w.deltaY)) { o->setView (o->viewStart - w.deltaX * o->viewLen * 0.5, o->viewLen); return; }
    const double at = xToSec ((float) e.x), f = std::exp (-w.deltaY * 0.8), nl = o->viewLen * f; const double frac = (at - o->viewStart) / o->viewLen;
    o->setView (at - frac * nl, nl);
}
std::unique_ptr<AccessibilityHandler> ReferencePage::Wave::createAccessibilityHandler() { return std::make_unique<AccessibilityHandler> (*this, AccessibilityRole::image); }

Rectangle<int> ReferencePage::Cands::rowRect (int cat, int row) const { const int cw = getWidth() / 4; return { cat * cw + 14, 84 + row * 52, cw - 28, 46 }; }

void ReferencePage::Cands::paint (Graphics& g)
{
    const auto& P = o->lnf.pal(); const int cw = getWidth() / 4; const bool rtl = o->rtl;
    const auto just = rtl ? Justification::centredRight : Justification::centredLeft;
    for (int k = 0; k < 4; ++k)
    {
        const auto col = o->catColour (k); const auto& L = o->candList[(size_t) k]; const bool sel = o->selCat == k;
        const auto card = Rectangle<float> ((float) (k * cw + 4), 0.0f, (float) (cw - 8), (float) getHeight());
        g.setColour (P.bg); g.fillRoundedRectangle (card, 12); g.setColour (sel ? col : P.line); g.drawRoundedRectangle (card.reduced (0.5f), 12, sel ? 1.6f : 1.0f);
        drawCategoryIcon (g, k, Rectangle<float> (card.getX() + 16, 18, 26, 22), col);
        g.setColour (P.ink); g.setFont (o->lnf.font (14.5f, true)); g.drawText (o->T (kCatKeys[k]).toUpperCase(), Rectangle<float> (card.getX() + 52, 12, card.getWidth() - 130, 20), just);
        g.setColour (sel ? col : P.mute); g.setFont (o->lnf.font (11.0f));
        g.drawText (sel ? o->T ("scSelected") : (L.empty() ? o->T ("rfNone") : String (L.size()) + " " + o->T ("scCands")), Rectangle<float> (card.getX() + 52, 32, card.getWidth() - 130, 16), just);
        // en iyi adayın güveni (sezgisel puan; olasılık değil): yalnızca gerçek analiz varsa
        if (! L.empty()) { g.setColour (col); g.setFont (o->lnf.font (16.0f, true)); g.drawText (String (roundToInt (L[0].confidence * 100)) + "%", Rectangle<float> (card.getRight() - 76, 12, 62, 24), rtl ? Justification::centredLeft : Justification::centredRight); }
        g.setColour (P.line); g.fillRect (card.getX() + 14, 62.0f, card.getWidth() - 28, 1.0f);
        for (int r = 0; r < (int) L.size(); ++r)
        {
            const auto rr = rowRect (k, r).toFloat(); const bool rs = sel && std::abs (o->selA - L[(size_t) r].t0) < 1e-6;
                        g.setColour (rs ? col.withAlpha (0.16f) : P.panel); g.fillRoundedRectangle (rr, 8); g.setColour (rs ? col : (r == hoverRow && k == hoverCat ? P.accent2 : P.line)); g.drawRoundedRectangle (rr.reduced (0.5f), 8, 1.0f);
            // önizleme simgesi
            const auto pc = Rectangle<float> (rtl ? rr.getRight() - 30 : rr.getX() + 10, rr.getCentreY() - 9, 18, 18);
            g.setColour (col.withAlpha (rs ? 1.0f : 0.8f)); g.fillEllipse (pc); g.setColour (P.accentInk);
            Path tri; tri.addTriangle (pc.getX() + 7, pc.getY() + 5, pc.getX() + 7, pc.getBottom() - 5, pc.getRight() - 5, pc.getCentreY()); g.fillPath (tri);
            g.setColour (P.ink); g.setFont (o->lnf.font (12.5f)); g.drawText (fmtTime (L[(size_t) r].t0) + " - " + fmtTime (L[(size_t) r].t1), rr.reduced (rtl ? 40 : 36, 0).withTrimmedRight (rtl ? 0 : 52).withTrimmedLeft (rtl ? 52 : 36), just);
            g.setColour (col); g.setFont (o->lnf.font (12.5f, true)); g.drawText (String (roundToInt (L[(size_t) r].confidence * 100)) + "%", rr.reduced (10, 0), rtl ? Justification::centredLeft : Justification::centredRight);
            const float bw = rr.getWidth() - 20; g.setColour (col.withAlpha (0.55f)); g.fillRect (rr.getX() + 10, rr.getBottom() - 6, bw * L[(size_t) r].confidence, 2.0f);
        }
        if (L.empty() && o->proc.ref.analysis()) { g.setColour (P.mute); g.setFont (o->lnf.font (11.5f)); g.drawText (o->T ("scNoneHint"), Rectangle<float> (card.getX() + 16, 76, card.getWidth() - 32, 60), Justification::topLeft); }
    }
}
void ReferencePage::Cands::mouseMove (const MouseEvent& e)
{
    int hc = -1, hr = -1;
    for (int k = 0; k < 4 && hc < 0; ++k) for (int r = 0; r < (int) o->candList[(size_t) k].size(); ++r) if (rowRect (k, r).contains (e.getPosition())) { hc = k; hr = r; break; }
    if (hc != hoverCat || hr != hoverRow) { hoverCat = hc; hoverRow = hr; setMouseCursor (hc >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint(); }
}
void ReferencePage::Cands::mouseExit (const MouseEvent&) { if (hoverCat >= 0) { hoverCat = hoverRow = -1; repaint(); } }
void ReferencePage::Cands::mouseDown (const MouseEvent& e)
{
    for (int k = 0; k < 4; ++k) for (int r = 0; r < (int) o->candList[(size_t) k].size(); ++r) if (rowRect (k, r).contains (e.getPosition())) { o->selectCandidate (k, r); return; }
}

void ReferencePage::setView (int v) { view = v; viewSynth.setToggleState (v == 0, dontSendNotification); viewSamp.setToggleState (v == 1, dontSendNotification); viewLib.setToggleState (v == 2, dontSendNotification); if (v == 2) libRefresh(); applyWorkspace(); }

void ReferencePage::setWorkspace (int w) { ws = jlimit (0, 3, w); applyWorkspace(); }

void ReferencePage::syncOrb()
{
    hgui::OrbView::Model m; m.clip = proc.ref.clip(); m.cands = candList; m.selA = selA; m.selB = selB; m.selCat = selCat;
    if (auto a = proc.ref.analysis(); a != nullptr && a->ok) { m.bpm = a->bpm; m.key = a->key; m.minor = a->minor; }
    orb.setModel (m); layers.selCat = selCat; layers.selRow = selCat >= 0 ? selRow : -1; layers.repaint();
    if (auto c = proc.ref.clip()) orb.bottomText = c->name; else orb.bottomText = {};
}

void ReferencePage::applyWorkspace()
{
    using CL = std::initializer_list<Component*>;
    const bool r0 = ws == 0, r1 = ws == 1, r2 = ws == 2, r3 = ws == 3, d = r2 && view == 0, s = r2 && view == 1, lb = r2 && view == 2;
    auto vis = [] (CL l, bool on) { for (auto* x : l) x->setVisible (on); };
    // her çalışma alanında ortak: dalga formu, durum göstergesi
    vis ({ &stat, &status }, true); vis ({ &wave }, ! r3); vis ({ &info }, r0);
    vis ({ &orb }, r3 && ! gridOpen); vis ({ &grid }, r3 && gridOpen); vis ({ &layers, &gridBtn, &undoBtn, &redoBtn }, r3); for (auto& m : macroRows) if (m) m->setVisible (r3);
    vis ({ &snapBtn }, r0); vis ({ &rhythmTile }, r0 || r1); vis ({ &libBatch }, lb);
    vis ({ &openBtn, &loopBtn, &gainL, &gainS, &posLabel, &applyBtn, &bar, &zoomIn, &zoomOut, &zoomFit, &regionLabel }, r0);
    vis ({ &playBtn }, r0 || r1); vis ({ &quickBtn }, r0 || r3);
    vis ({ &smartBtn, &banner, &cands }, r1);
    vis ({ &partBtns[0], &partBtns[1], &partBtns[2] }, r2 || r3); vis ({ &viewSynth, &viewSamp, &viewLib }, r2);
    vis ({ &simK, &varK, &texK, &movK, &briK, &saveDnaBtn, &loadDnaBtn, &dnaHint }, d); vis ({ &dnaInfo }, d);
    vis ({ &sEnable, &sLoop, &rootBox, &trS, &voiceS, &rootL, &trL, &voiceL, &sTrimBtn, &sLoopSelBtn, &sSaveBtn, &sLoadBtn, &sInfo, &sKnobs[0], &sKnobs[1], &sKnobs[2], &sKnobs[3], &sKnobs[4], &sKnobs[5] }, s);
    vis ({ &libList, &libSave, &libRename, &libDelete, &libImport, &libExport, &libFav, &libUseBtn, &searchEd, &artistEd, &nameEd, &filterBox }, lb);
    vis ({ &genBtn, &regenBtn, &randBtn, &previewNowBtn, &savePresetBtn, &previewBtn, &seed, &seedLabel, &seedNewBtn, &prevBtn, &nextBtn, &pinBtn, &abBtn, &histLabel, &summaryLabel }, r3);
    if (r3) syncOrb();
    resized(); repaint();
}

void ReferencePage::syncSampler()
{
    auto& P = proc.ref.sampler; sEnable.setToggleState (P.enabled.load(), dontSendNotification); sLoop.setToggleState (P.loop.load(), dontSendNotification);
    rootBox.setSelectedId (P.root.load() + 1, dontSendNotification); trS.setValue (P.transpose.load(), dontSendNotification); voiceS.setValue (P.maxVoices.load(), dontSendNotification);
    const float v[6] = { P.a.load(), P.d.load(), P.s.load(), P.r.load(), P.gain.load(), P.velSens.load() }; for (int i = 0; i < 6; ++i) sKnobs[(size_t) i].slider.setValue (v[i], dontSendNotification);
}

void ReferencePage::trimFromSelection()
{
    auto c = proc.ref.clip(); if (c == nullptr) { status.setText (T ("rfNeedClip"), dontSendNotification); return; }
    if (selB <= selA + 0.02) { status.setText (T ("rfNeedSel"), dontSendNotification); return; }
    auto& P = proc.ref.sampler; P.trimStart = selA; P.trimEnd = selB; P.loopStart = selA; P.loopEnd = selB;
    if (haveDna && dna.pitched && dna.f0Hz > 20.0f) { const int m = jlimit (0, 127, roundToInt (69.0 + 12.0 * std::log2 (dna.f0Hz / 440.0))); P.root = m; rootBox.setSelectedId (m + 1, dontSendNotification); }
    status.setText (T ("rfTrimmed").replace ("{t}", fmtTime (selB - selA)).replace ("{n}", MidiMessage::getMidiNoteName (P.root.load(), true, true, 3)), dontSendNotification);
}

static File sampFolder() { auto d = File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("Samplers"); d.createDirectory(); return d; }

void ReferencePage::saveSampler()
{
    chooser = std::make_unique<FileChooser> (T ("rfSampSave"), sampFolder().getChildFile ("sampler.hgsamp"), "*.hgsamp");
    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::warnAboutOverwriting, [this] (const FileChooser& fc)
    {
        auto f = fc.getResult(); if (f == File()) return; f = f.withFileExtension ("hgsamp");
        status.setText (f.replaceWithText (JSON::toString (proc.ref.samplerToVar())) ? T ("rfSaved").replace ("{f}", f.getFullPathName()) : T ("rfSaveFail"), dontSendNotification);
    });
}

void ReferencePage::loadSampler()
{
    chooser = std::make_unique<FileChooser> (T ("rfSampLoad"), sampFolder(), "*.hgsamp");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
    {
        const auto f = fc.getResult(); if (! f.existsAsFile() || f.getSize() > 200000) return;
        const auto v = JSON::parse (f.loadFileAsString()); if (v.getProperty ("format", {}).toString() != "wavejackers-sampler") { status.setText (T ("rfDnaBad"), dontSendNotification); return; }
        proc.ref.samplerFromVar (v); syncSampler(); const File src (v.getProperty ("sourcePath", {}).toString());
        auto cur = proc.ref.clip();
        if (src.existsAsFile() && (cur == nullptr || cur->path != src.getFullPathName())) openFile (src);
        else if (! src.existsAsFile() && v.getProperty ("sourcePath", {}).toString().isNotEmpty() && cur == nullptr) status.setText (T ("fileMissing") + " " + v.getProperty ("sourcePath", {}).toString(), dontSendNotification);
        else status.setText (T ("rfSampLoaded"), dontSendNotification);
    });
}

void ReferencePage::LibModel::paintListBoxItem (int row, Graphics& g, int w, int h, bool sel)
{
    if (row < 0 || row >= (int) o->libRows.size()) return; const auto& e = o->libRows[(size_t) row]; const auto& P = o->lnf.pal();
    if (sel) { g.setColour (P.accent.withAlpha (0.25f)); g.fillRect (0, 0, w, h); }
    g.setColour (e.favorite ? P.accent : P.mute); g.setFont (o->lnf.font (13.0f, true)); g.drawText (e.favorite ? "*" : "-", 4, 0, 14, h, Justification::centred);
    g.setColour (P.ink); g.setFont (o->lnf.font (12.5f)); g.drawText ((e.artist.isNotEmpty() ? e.artist + "  /  " : String()) + e.name, 22, 0, w / 2 - 22, h, Justification::centredLeft, true);
    String tg; for (int i = 0; i < jmin (3, e.tags.size()); ++i) tg << (i ? "  " : "") << o->T (e.tags[i]).toLowerCase();
    g.setColour (P.accent.withAlpha (0.85f)); g.setFont (o->lnf.font (11.0f)); g.drawText (tg, w / 2 + 4, 0, w / 2 - 110, h, Justification::centredLeft, true);
    g.setColour (P.mute); g.drawText (e.role, w - 100, 0, 94, h, Justification::centredRight);
}
void ReferencePage::libRefresh() { libRows = lib.query (searchEd.getText(), (hg::DnaLibrary::Filter) (filterBox.getSelectedId() - 1), [this] (const String& t) { return T (t); }); libList.updateContent(); libList.repaint(); }
const hg::DnaEntry* ReferencePage::libSel() const { const int r = libList.getSelectedRow(); return r >= 0 && r < (int) libRows.size() ? &libRows[(size_t) r] : nullptr; }
void ReferencePage::libSelChanged() { if (auto* e = libSel()) { nameEd.setText (e->name, dontSendNotification); artistEd.setText (e->artist, dontSendNotification); } }
void ReferencePage::libUse (int row)
{
    if (row < 0 || row >= (int) libRows.size()) { status.setText (T ("libPick"), dontSendNotification); return; }
    const auto e = libRows[(size_t) row]; TrackDna d;
    if (! lib.load (e, d)) { status.setText (T ("rfDnaBad"), dontSendNotification); return; }
    dna = d; haveDna = true; dnaA = dnaB = -2; genBtn.setEnabled (true);
    const int pc = d.role == "bass" ? 1 : d.role == "pad" ? 0 : d.role == "lead" ? 2 : part; part = pc; partBtns[(size_t) pc].setToggleState (true, dontSendNotification); proc.auditionPart.store (part);
    status.setText (T ("rfDnaLoaded").replace ("{n}", d.name), dontSendNotification); libRefresh();
}

void ReferencePage::quickStart()
{
    quickPending = true; setView (0);
    const int st = proc.ref.state.load();
    if (proc.ref.clip() == nullptr) { status.setText (T ("rfQuickOpen"), dontSendNotification); openBtn.triggerClick(); return; }
    if (st == RefEngine::Loaded) proc.ref.analyse(); else quickStep();
}

void ReferencePage::quickStep()
{
    const int st = proc.ref.state.load();
    if (st == RefEngine::Failed) { quickPending = false; return; }
    if (st == RefEngine::Loaded) { quickPending = false; quickPending = true; proc.ref.analyse(); return; }
    if (st != RefEngine::Ready) return;
    quickPending = false;
    auto a = proc.ref.analysis(); if (! a || ! a->ok) return;
    int bc = -1; float bs = 0.0f;                                   // en yüksek güvenli lead/bas/pad adayı
    for (int k : { catLead, catBass, catPad }) { auto t = a->top (k, 1); if (! t.empty() && t[0].confidence > bs) { bs = t[0].confidence; bc = k; } }
    if (bc < 0) { status.setText (T ("rfNoRegions"), dontSendNotification); return; }
    candList.assign (kNumRefCats, {}); for (int k = 0; k < kNumRefCats; ++k) candList[(size_t) k] = a->top (k, 4);
    selectCandidate (bc, 0); proc.ref.playing = false; relabel(); generate();
}

void ReferencePage::pinA() { if (histPos < 0) { status.setText (T ("rfNoDna"), dontSendNotification); return; } proc.readSoundParams (part, slotA.data()); haveA = true; abOnA = false; abBtn.setEnabled (true); abBtn.setButtonText ("A/B"); status.setText (T ("rfPinned"), dontSendNotification); }

void ReferencePage::swapAB()
{
    if (! haveA) return;
    if (! abOnA) { proc.readSoundParams (part, slotB.data()); proc.applySoundParams (part, slotA.data()); abOnA = true; }
    else { proc.applySoundParams (part, slotB.data()); abOnA = false; }
    abBtn.setButtonText (abOnA ? "A" : "B"); status.setText (T (abOnA ? "rfShowA" : "rfShowB"), dontSendNotification);
    if (previewBtn.getToggleState()) startPhrase();
}

bool ReferencePage::keyPressed (const KeyPress& k)
{
    if (k.getModifiers().isAnyModifierKeyDown()) return false;
    if (k == KeyPress::spaceKey && proc.ref.clip() != nullptr) { togglePlay(); return true; }
    if (k.getTextCharacter() == 'g' || k.getTextCharacter() == 'G') { generate(); return true; }
    if (k.getTextCharacter() == '[') { applyHistory (histPos - 1); return true; }
    if (k.getTextCharacter() == ']') { applyHistory (histPos + 1); return true; }
    if (k.getTextCharacter() == 'a' || k.getTextCharacter() == 'A') { swapAB(); return true; }
    return false;
}

void ReferencePage::markCompleted() { completedAt = Time::currentTimeMillis(); }

// Durum yalnızca gerçek iş durumundan türetilir: yükleme/analiz (RefEngine), DNA çıkarımı (arka plan işi), son işlemin sonucu.
void ReferencePage::updateStat()
{
    using S = hgui::StatusIndicator;
    const int st = proc.ref.state.load();
    if (batchBusy) stat.set (S::Processing, batchFiles > 0 ? (float) batchDoneFiles / (float) batchFiles : -1.0f, T ("stProc"));
    else if (dnaBusy || st == RefEngine::Loading || st == RefEngine::Analysing)
        stat.set (S::Processing, st == RefEngine::Analysing ? proc.ref.progress.load() : -1.0f, T (st == RefEngine::Analysing ? "rfAnalysing" : "stProc"));
    else if (errorFlag || st == RefEngine::Failed) stat.set (S::Error, -1.0f, T ("stErr"));
    else if (Time::currentTimeMillis() - completedAt < 4000 && completedAt > 0) stat.set (S::Completed, -1.0f, T ("stDone"));
    else stat.set (S::Ready, -1.0f, T ("stReady"));
}

// DNA makroları: yalnızca üretilmiş sesin gerçek parametrelerini değiştirir (Brightness = filtre kesimi, Texture = gürültü + sürücü, Movement = LFO1 derinliği)
void ReferencePage::applyMacros()
{
    if (! haveBase) return;
    float v[kNumSp]; std::copy (baseVals.begin(), baseVals.end(), v);
    auto clampTo = [] (int i, float x) { const auto& d = spDef (i); return jlimit (d.lo, d.hi, x); };
    const float b = ((float) briK.slider.getValue() - 50.0f) / 50.0f, t = ((float) texK.slider.getValue() - 50.0f) / 50.0f, m = ((float) movK.slider.getValue() - 50.0f) / 50.0f;
    v[cut] = clampTo (cut, baseVals[(size_t) cut] * std::pow (2.0f, b * 1.5f));
    v[noise] = clampTo (noise, baseVals[(size_t) noise] + t * 0.30f); v[fxDrive] = clampTo (fxDrive, baseVals[(size_t) fxDrive] + t * 0.25f);
    v[l1depth] = clampTo (l1depth, baseVals[(size_t) l1depth] + m * 0.5f);
    if (m > 0 && baseVals[(size_t) l1depth] < 0.05f) { v[l1target] = 0; v[l1rate] = clampTo (l1rate, std::max (baseVals[(size_t) l1rate], 0.4f)); }   // kullanılmayan LFO'yu filtreye bağla
    proc.applySoundParams (part, v); std::copy (v, v + kNumSp, curVals.begin()); macroDirtyAt = Time::currentTimeMillis();
}

void ReferencePage::regenerateSame() { if (history.empty()) { generate(); return; } seed.setValue (lastSeed, dontSendNotification); generate(); }

void ReferencePage::randomizeGen()
{
    seed.setValue (1 + Random::getSystemRandom().nextInt (999999), dontSendNotification);
    varK.slider.setValue (25 + Random::getSystemRandom().nextInt (46), dontSendNotification);
    generate();
}

// ---------------------------------------------------------------- 2.5: iş akışı yardımcıları
void ReferencePage::snapSelection()
{
    if (! snapBtn.getToggleState()) return;
    auto c = proc.ref.clip(); auto a = proc.ref.analysis(); if (c == nullptr || a == nullptr || ! a->ok || a->bpm <= 0 || selB <= selA + 0.01) return;
    double na = 0, nb = 0; const double anchor = a->onsets.empty() ? 0.0 : (double) a->onsets.front();
    if (snapToBars (selA, selB, a->bpm, anchor, c->seconds(), na, nb)) setSelection (na, nb, -1);
}

File ReferencePage::makeRhythmFile()
{
    auto c = proc.ref.clip(); auto a = proc.ref.analysis();
    if (c == nullptr || a == nullptr || ! a->ok) { status.setText (T ("rhythmNone"), dontSendNotification); return {}; }
    RhythmSketch rs; if (! onsetsToMidi (*c, *a, selA, selB, rs)) { status.setText (T ("rhythmNone"), dontSendNotification); return {}; }
    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("HitGen"); dir.createDirectory();
    auto f = dir.getChildFile (File::createLegalFileName (c->name + " rhythm " + String (roundToInt (rs.bpm)) + "bpm") + ".mid"); f.deleteFile();
    { FileOutputStream os (f); if (! os.openedOk() || ! rs.midi.writeTo (os)) { status.setText (T ("rfSaveFail"), dontSendNotification); return {}; } }
    status.setText (T ("rhythmDone").replace ("{n}", String (rs.notes)).replace ("{k}", String (rs.kicks)).replace ("{s}", String (rs.snares)).replace ("{h}", String (rs.hats)), dontSendNotification);
    return f;
}

void ReferencePage::startBatch()
{
    chooser = std::make_unique<FileChooser> (T ("libBatch"), File::getSpecialLocation (File::userMusicDirectory), "*");
    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories, [this] (const FileChooser& fc)
    {
        const auto dir = fc.getResult(); if (! dir.isDirectory()) return;
        const auto files = BatchAnalyser::audioFilesIn (dir);
        if (files.isEmpty()) { status.setText (T ("batchNone"), dontSendNotification); return; }
        batchFiles = files.size(); batchAdded = 0; batchDoneFiles = 0;
        if (batch.start (files)) { batchBusy = true; errorFlag = false; status.setText (T ("batchProg").replace ("{i}", "0").replace ("{n}", String (batchFiles)).replace ("{f}", dir.getFileName()), dontSendNotification); relabel(); }
    });
}

void ReferencePage::openGrid()
{
    if (gridOpen) { closeGrid(); return; }
    gridPending = true; generate();                    // önce DNA ve temel ses; ızgara doGenerate sonunda kurulur
}

void ReferencePage::buildGrid()
{
    if (! haveDna) return;
    grid.items.clear(); const char role = part == 1 ? 'b' : part == 0 ? 'c' : 'm';
    for (int i = 0; i < 6; ++i)
    {
        hgui::VariationGrid::Item it; it.seed = lastSeed + (juce::uint32) i;
        dnaToSoundParams (dna, role, (float) simK.slider.getValue(), (float) varK.slider.getValue(), it.seed, it.v.data());
        grid.items.push_back (it);
    }
    grid.playing = 0; gridOpen = true; applyWorkspace(); grid.toFront (true);
    if (previewBtn.getToggleState()) startPhrase();
}

void ReferencePage::closeGrid() { gridOpen = false; applyWorkspace(); }

void ReferencePage::keepVariation (int i)
{
    if (i < 0 || i >= (int) grid.items.size()) return;
    const auto it = grid.items[(size_t) i];
    proc.applySoundParams (part, it.v.data()); pushHistory (it.v.data(), it.seed); lastSeed = it.seed;
    baseVals = it.v; haveBase = true; curVals = it.v;
    for (auto* k : { &texK, &movK, &briK }) { k->slider.setValue (50, dontSendNotification); k->slider.setEnabled (true); }
    pushUndo(); markCompleted(); dnaHint.setText (T ("dnaHintOn"), dontSendNotification);
    status.setText (T ("rfGenerated").replace ("{p}", T (kPartKeys[part])).replace ("{s}", String ((int) it.seed)), dontSendNotification);
    closeGrid(); repaint();
}

void ReferencePage::pushUndo()
{
    UndoState u; u.base = baseVals; u.cur = curVals; u.haveBase = haveBase;
    u.tex = (float) texK.slider.getValue(); u.mov = (float) movK.slider.getValue(); u.bri = (float) briK.slider.getValue();
    if (undoPos >= 0 && undoPos < (int) undoStack.size())
    {
        const auto& t = undoStack[(size_t) undoPos];
        if (t.cur == u.cur && t.base == u.base && t.tex == u.tex && t.mov == u.mov && t.bri == u.bri && t.haveBase == u.haveBase) return;   // değişiklik yok
    }
    if (undoPos + 1 < (int) undoStack.size()) undoStack.resize ((size_t) undoPos + 1);
    undoStack.push_back (u); if (undoStack.size() > 60) undoStack.erase (undoStack.begin());
    undoPos = (int) undoStack.size() - 1; updateUndoButtons();
}

void ReferencePage::restoreUndo (const UndoState& u)
{
    macroDirtyAt = 0; baseVals = u.base; curVals = u.cur; haveBase = u.haveBase;
    texK.slider.setValue (u.tex, dontSendNotification); movK.slider.setValue (u.mov, dontSendNotification); briK.slider.setValue (u.bri, dontSendNotification);
    for (auto* k : { &texK, &movK, &briK }) k->slider.setEnabled (haveBase);
    if (haveBase) proc.applySoundParams (part, curVals.data());
    if (previewBtn.getToggleState()) startPhrase();
    updateUndoButtons(); repaint();
}

bool ReferencePage::undo()
{
    if (macroDirtyAt > 0) { macroDirtyAt = 0; pushUndo(); }        // bekleyen makro değişikliği önce kayda geçer
    if (undoPos <= 0) { status.setText (T ("undoNone"), dontSendNotification); return false; }
    restoreUndo (undoStack[(size_t) --undoPos]); status.setText (T ("undo") + "  " + String (undoPos + 1) + "/" + String ((int) undoStack.size()), dontSendNotification); return true;
}

bool ReferencePage::redo()
{
    if (undoPos + 1 >= (int) undoStack.size()) { status.setText (T ("redoNone"), dontSendNotification); return false; }
    restoreUndo (undoStack[(size_t) ++undoPos]); status.setText (T ("redo") + "  " + String (undoPos + 1) + "/" + String ((int) undoStack.size()), dontSendNotification); return true;
}

void ReferencePage::updateUndoButtons() { undoBtn.setEnabled (undoPos > 0); redoBtn.setEnabled (undoPos + 1 < (int) undoStack.size()); }
