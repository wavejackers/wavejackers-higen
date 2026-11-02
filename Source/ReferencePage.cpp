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
                                                       &loopBtn, &previewBtn, &quickBtn, &pinBtn, &abBtn, &viewSynth, &viewSamp, &viewLib, &libList, &libSave, &libRename, &libDelete, &libImport, &libExport, &libFav, &libUseBtn, &searchEd, &artistEd, &nameEd, &filterBox, &sTrimBtn, &sLoopSelBtn, &sSaveBtn, &sLoadBtn, &sEnable, &sLoop, &rootBox, &trS, &voiceS, &rootL, &trL, &voiceL, &sInfo, &simK, &varK, &gainS, &gainL, &seed, &seedLabel, &status, &info, &dnaInfo, &banner, &histLabel, &wave, &cands, &bar })
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
    quickBtn.onClick = [this] { quickStart(); }; pinBtn.onClick = [this] { pinA(); }; abBtn.onClick = [this] { swapAB(); }; abBtn.setEnabled (false);
    setWantsKeyboardFocus (true);
    libRefresh();
    setView (0);
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
    seedLabel.setText (T ("seed"), dontSendNotification);
    viewSynth.setButtonText (T ("rfViewSynth")); viewSamp.setButtonText (T ("rfViewSamp")); sEnable.setButtonText (T ("rfSampEnable")); sLoop.setButtonText (T ("rfLoop"));
    rootL.setText (T ("rfRoot"), dontSendNotification); trL.setText (T ("rfTranspose"), dontSendNotification); voiceL.setText (T ("rfVoices"), dontSendNotification);
    sTrimBtn.setButtonText (T ("rfTrim")); sLoopSelBtn.setButtonText (T ("rfLoopSel")); sSaveBtn.setButtonText (T ("rfSampSave")); sLoadBtn.setButtonText (T ("rfSampLoad"));
    quickBtn.setButtonText (T ("rfQuick")); quickBtn.setTooltip (T ("rfQuickHint")); pinBtn.setButtonText (T ("rfPinA")); pinBtn.setTooltip (T ("rfABHint")); abBtn.setButtonText (abOnA ? "B" : "A/B"); abBtn.setTooltip (T ("rfABHint"));
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
    syncSampler(); lastState = st; wave.repaint(); cands.repaint(); repaint();
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
    haveDna = haveDna && dnaA == -2; wave.repaint(); cands.repaint(); repaint();
}

void ReferencePage::selectCandidate (int cat, int row)
{
    if (cat < 0 || cat >= (int) candList.size() || row < 0 || row >= (int) candList[(size_t) cat].size()) return;
    const auto& g = candList[(size_t) cat][(size_t) row];
    setSelection (g.t0, g.t1, cat); proc.ref.seekTo = g.t0; proc.ref.loop = true; loopBtn.setToggleState (true, dontSendNotification); proc.ref.playing = true; relabel();
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
    auto c = proc.ref.clip(); if (c == nullptr) { status.setText (T ("rfNeedClip"), dontSendNotification); return; }
    double a = selA, b = selB; int role = selCat >= 0 ? selCat : catLead;
    if (b <= a + 0.15) { if (! candList[(size_t) catLead].empty()) { a = candList[(size_t) catLead][0].t0; b = candList[(size_t) catLead][0].t1; } else { a = 0; b = std::min (c->seconds(), 8.0); } status.setText (T ("rfAutoSel"), dontSendNotification); }
    if (haveDna && std::abs (dnaA - a) < 1e-6 && std::abs (dnaB - b) < 1e-6 && dnaRole == role && dnaClip == c->name) { doGenerate (dna); return; }
    status.setText (T ("rfExtracting"), dontSendNotification);
    proc.ref.extractDna (a, b, role, [this, a, b, role, c] (bool ok, const TrackDna& d)
    {
        if (! ok) { status.setText (T ("rfDnaFail"), dontSendNotification); return; }
        dna = d; haveDna = true; dnaA = a; dnaB = b; dnaRole = role; dnaClip = c->name; doGenerate (dna);
    });
}

void ReferencePage::doGenerate (const TrackDna& d)
{
    const juce::uint32 sd = (juce::uint32) seed.getValue(); float vals[kNumSp];
    dnaToSoundParams (d, part == 1 ? 'b' : part == 0 ? 'c' : 'm', (float) simK.slider.getValue(), (float) varK.slider.getValue(), sd, vals);
    proc.applySoundParams (part, vals); pushHistory (vals, sd); abOnA = false; abBtn.setEnabled (haveA); abBtn.setButtonText ("A/B");
    seed.setValue (sd + 1, dontSendNotification); seed.setValue (sd + 1);       // sonraki tıklama farklı bir varyasyon verir; kullanılan tohum geçmişte saklı
    auto F = [] (float v, int dp) { return dp == 0 ? String (roundToInt (v)) : String (v, dp); };
    dnaInfo.setText (T ("rfDnaInfo").replace ("{f0}", d.pitched ? F (d.f0Hz, 1) + " Hz" : T ("rfUnpitched")).replace ("{cent}", F (d.centroidHz, 0)).replace ("{atk}", F (d.attackMs, 0)).replace ("{sus}", F (d.sustainLevel, 2))
                         .replace ("{wid}", F (d.width, 2)).replace ("{noi}", F (d.noise, 2)).replace ("{dur}", fmtTime (d.regionEnd - d.regionStart)).replace ("{src}", d.sourceFile), dontSendNotification);
    status.setText (T ("rfGenerated").replace ("{p}", T (kPartKeys[part])).replace ("{s}", String ((int) sd)), dontSendNotification);
    if (previewBtn.getToggleState()) startPhrase();
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
    prevBtn.setEnabled (histPos > 0); nextBtn.setEnabled (histPos + 1 < (int) history.size());
    histLabel.setText (T ("rfHist") + " " + String (histPos + 1) + "/" + String ((int) history.size()) + "  (seed " + String ((int) history[(size_t) pos].seed) + ")", dontSendNotification);
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
    auto P = [this] (Component& c, Rectangle<int> r) { c.setBounds (rtl ? r.withX (kW - r.getRight()) : r); };
    P (openBtn, { 16, 10, 150, 34 }); P (smartBtn, { 174, 10, 210, 34 }); P (playBtn, { 392, 10, 200, 34 }); P (loopBtn, { 600, 10, 90, 34 });
    P (gainL, { 698, 10, 52, 34 }); P (gainS, { 750, 10, 110, 34 }); P (applyBtn, { 866, 10, 134, 34 }); P (quickBtn, { 1008, 10, 140, 34 });
    P (info, { 16, 48, 1132, 36 }); P (status, { 16, 84, 1132, 18 });
    P (wave, { 16, 106, 1132, 170 }); P (bar, { 16, 278, 1132, 12 });
    P (zoomOut, { 16, 294, 40, 24 }); P (zoomIn, { 60, 294, 40, 24 }); P (zoomFit, { 104, 294, 80, 24 }); P (banner, { 196, 294, 952, 24 });
    P (cands, { 16, 324, 1132, 164 });
    for (int i = 0; i < 3; ++i) P (partBtns[(size_t) i], { 16 + i * 108, 500, 104, 28 });
    P (genBtn, { 16, 538, 250, 54 }); P (prevBtn, { 274, 548, 40, 34 }); P (nextBtn, { 318, 548, 40, 34 }); P (pinBtn, { 366, 548, 78, 34 }); P (abBtn, { 450, 548, 60, 34 }); P (histLabel, { 274, 592, 320, 18 });
    P (simK, { 600, 496, 100, 92 }); P (varK, { 706, 496, 100, 92 });
    P (seedLabel, { 830, 496, 80, 20 }); P (seed, { 830, 518, 170, 30 }); P (seedNewBtn, { 1006, 518, 34, 30 }); P (previewBtn, { 830, 556, 230, 30 });
    P (saveDnaBtn, { 1056, 496, 92, 30 }); P (loadDnaBtn, { 1056, 532, 92, 30 });
    P (dnaInfo, { 16, 612, 1132, 38 });
    P (viewSynth, { 340, 492, 120, 26 }); P (viewSamp, { 464, 492, 120, 26 }); P (viewLib, { 588, 492, 120, 26 });
    P (searchEd, { 16, 526, 300, 26 }); P (filterBox, { 324, 526, 140, 26 }); P (libList, { 16, 556, 448, 92 });
    P (artistEd, { 480, 526, 210, 26 }); P (nameEd, { 698, 526, 220, 26 }); P (libSave, { 930, 526, 218, 26 });
    P (libUseBtn, { 480, 560, 110, 26 }); P (libFav, { 596, 560, 110, 26 }); P (libRename, { 712, 560, 110, 26 }); P (libDelete, { 828, 560, 110, 26 });
    P (libImport, { 480, 594, 110, 26 }); P (libExport, { 596, 594, 110, 26 });
    P (sEnable, { 16, 528, 200, 30 }); P (rootL, { 230, 528, 60, 30 }); P (rootBox, { 290, 528, 130, 30 }); P (trL, { 436, 528, 90, 30 }); P (trS, { 526, 528, 120, 30 });
    P (voiceL, { 660, 528, 80, 30 }); P (voiceS, { 740, 528, 110, 30 }); P (sLoop, { 866, 528, 100, 30 }); P (sTrimBtn, { 976, 528, 172, 30 });
    for (int i = 0; i < 6; ++i) P (sKnobs[(size_t) i], { 16 + i * 104, 562, 100, 86 });
    P (sLoopSelBtn, { 650, 562, 170, 28 }); P (sSaveBtn, { 650, 594, 170, 28 }); P (sLoadBtn, { 650, 624, 170, 28 }); P (sInfo, { 832, 566, 316, 82 });
}

void ReferencePage::paint (Graphics& g)
{
    const auto& P = lnf.pal(); g.setColour (P.panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 14);
    g.setColour (P.panel2); g.fillRoundedRectangle (8.0f, 488.0f, (float) kW - 16, 160.0f, 10);
    g.setColour (P.mute); g.setFont (lnf.font (12.0f));
    if (view == 0) g.drawText (T ("rfTarget"), Rectangle<int> (rtl ? kW - 330 : 16, 480, 314, 18), rtl ? Justification::centredRight : Justification::centredLeft);
    if (view == 2 && libRows.empty()) { g.setColour (P.mute); g.setFont (lnf.font (12.5f)); g.drawText (T ("libEmpty"), libList.getBounds().reduced (10), Justification::centred); }
    if (proc.ref.clip() == nullptr && proc.ref.state.load() != RefEngine::Loading)
    {
        g.setColour (P.accent.withAlpha (0.55f)); g.drawRoundedRectangle (wave.getBounds().toFloat().reduced (1), 10, 1.5f);
        g.setColour (P.mute); g.setFont (lnf.font (15.0f)); g.drawText (T ("rfDropHint"), wave.getBounds(), Justification::centred);
    }
}

double ReferencePage::Wave::xToSec (float x) const { return o->viewStart + (double) (x / std::max (1, getWidth())) * o->viewLen; }
float ReferencePage::Wave::secToX (double s) const { return (float) ((s - o->viewStart) / o->viewLen * getWidth()); }

void ReferencePage::Wave::paint (Graphics& g)
{
    const auto& P = o->lnf.pal(); auto c = o->proc.ref.clip(); const auto r = getLocalBounds().toFloat();
    g.setColour (P.bg.darker (0.3f)); g.fillRoundedRectangle (r, 10);
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

void ReferencePage::Wave::mouseDown (const MouseEvent& e) { o->grabKeyboardFocus(); if (o->proc.ref.clip() == nullptr) return; dragging = false; dragA = xToSec ((float) e.x); }
void ReferencePage::Wave::mouseDrag (const MouseEvent& e)
{
    if (o->proc.ref.clip() == nullptr) return;
    if (std::abs (e.getDistanceFromDragStartX()) > 4) { dragging = true; const double d = o->proc.ref.clip()->seconds(); o->setSelection (jlimit (0.0, d, dragA), jlimit (0.0, d, xToSec ((float) e.x)), -1); }
}
void ReferencePage::Wave::mouseUp (const MouseEvent& e)
{
    if (o->proc.ref.clip() == nullptr) return;
    if (! dragging) { o->proc.ref.seekTo = jlimit (0.0, o->proc.ref.clip()->seconds(), xToSec ((float) e.x)); if (e.mods.isShiftDown()) o->setSelection (0, 0, -1); }
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

Rectangle<int> ReferencePage::Cands::rowRect (int cat, int row) const { const int cw = getWidth() / 4; return { cat * cw + 4, 30 + row * 32, cw - 8, 28 }; }

void ReferencePage::Cands::paint (Graphics& g)
{
    const auto& P = o->lnf.pal(); const int cw = getWidth() / 4;
    for (int k = 0; k < 4; ++k)
    {
        const auto col = o->catColour (k); g.setColour (col); g.setFont (o->lnf.font (13.5f, true));
        g.drawText (o->T (kCatKeys[k]), Rectangle<int> (k * cw + 8, 2, cw - 16, 24), o->rtl ? Justification::centredRight : Justification::centredLeft);
        const auto& L = o->candList[(size_t) k];
        if (L.empty()) { g.setColour (P.mute); g.setFont (o->lnf.font (11.5f)); g.drawText (o->proc.ref.analysis() ? o->T ("rfNone") : "—", Rectangle<int> (k * cw + 8, 34, cw - 16, 24), Justification::centredLeft); continue; }
        for (int r = 0; r < (int) L.size(); ++r)
        {
            const auto rr = rowRect (k, r).toFloat(); const bool sel = o->selCat == k && std::abs (o->selA - L[(size_t) r].t0) < 1e-6;
            g.setColour (sel ? col.withAlpha (0.28f) : P.panel); g.fillRoundedRectangle (rr, 7); g.setColour (sel ? col : P.line); g.drawRoundedRectangle (rr, 7, sel ? 1.4f : 1.0f);
            g.setColour (P.ink); g.setFont (o->lnf.font (12.5f)); g.drawText (fmtTime (L[(size_t) r].t0) + " - " + fmtTime (L[(size_t) r].t1), rr.reduced (8, 0).withTrimmedRight (60), Justification::centredLeft);
            g.setColour (col); g.setFont (o->lnf.font (12.5f, true)); g.drawText (String (roundToInt (L[(size_t) r].confidence * 100)) + "%", rr.reduced (8, 0), Justification::centredRight);
            const float bw = rr.getWidth() - 16; g.setColour (col.withAlpha (0.5f)); g.fillRect (rr.getX() + 8, rr.getBottom() - 4, bw * L[(size_t) r].confidence, 2.0f);
        }
    }
}
void ReferencePage::Cands::mouseDown (const MouseEvent& e)
{
    for (int k = 0; k < 4; ++k) for (int r = 0; r < (int) o->candList[(size_t) k].size(); ++r) if (rowRect (k, r).contains (e.getPosition())) { o->selectCandidate (k, r); return; }
}

void ReferencePage::setView (int v)
{
    view = v; const bool s = v == 1, synth = v == 0;
    for (auto* x : std::initializer_list<Component*> { &partBtns[0], &partBtns[1], &partBtns[2], &genBtn, &prevBtn, &nextBtn, &pinBtn, &abBtn, &histLabel, &simK, &varK, &seed, &seedLabel, &seedNewBtn, &previewBtn, &saveDnaBtn, &loadDnaBtn, &dnaInfo }) x->setVisible (synth);
    for (auto* x : std::initializer_list<Component*> { &sEnable, &sLoop, &rootBox, &trS, &voiceS, &rootL, &trL, &voiceL, &sTrimBtn, &sLoopSelBtn, &sSaveBtn, &sLoadBtn, &sInfo }) x->setVisible (s);
    for (auto& k : sKnobs) k.setVisible (s);
    const bool lb = v == 2;
    for (auto* x : std::initializer_list<Component*> { &libList, &libSave, &libRename, &libDelete, &libImport, &libExport, &libFav, &libUseBtn, &searchEd, &artistEd, &nameEd, &filterBox }) x->setVisible (lb);
    viewSynth.setToggleState (v == 0, dontSendNotification); viewSamp.setToggleState (s, dontSendNotification); viewLib.setToggleState (lb, dontSendNotification); if (lb) libRefresh(); repaint();
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
    g.setColour (P.ink); g.setFont (o->lnf.font (12.5f)); g.drawText ((e.artist.isNotEmpty() ? e.artist + "  /  " : String()) + e.name, 22, 0, w - 130, h, Justification::centredLeft, true);
    g.setColour (P.mute); g.drawText (e.role, w - 100, 0, 94, h, Justification::centredRight);
}
void ReferencePage::libRefresh() { libRows = lib.query (searchEd.getText(), (hg::DnaLibrary::Filter) (filterBox.getSelectedId() - 1)); libList.updateContent(); libList.repaint(); }
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
