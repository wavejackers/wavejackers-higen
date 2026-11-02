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
                                                       &loopBtn, &previewBtn, &simK, &varK, &gainS, &gainL, &seed, &seedLabel, &status, &info, &dnaInfo, &banner, &histLabel, &wave, &cands, &bar })
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
    const int st = proc.ref.state.load(); auto c = proc.ref.clip(); auto a = proc.ref.analysis();
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
    else status.setText (T ("rfDropHint"), dontSendNotification);
    banner.setText (T ("rfBanner"), dontSendNotification);
    lastState = st; wave.repaint(); cands.repaint(); repaint();
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
    proc.applySoundParams (part, vals); pushHistory (vals, sd);
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
    P (gainL, { 700, 10, 60, 34 }); P (gainS, { 760, 10, 130, 34 }); P (applyBtn, { 900, 10, 248, 34 });
    P (info, { 16, 48, 1132, 36 }); P (status, { 16, 84, 1132, 18 });
    P (wave, { 16, 106, 1132, 170 }); P (bar, { 16, 278, 1132, 12 });
    P (zoomOut, { 16, 294, 40, 24 }); P (zoomIn, { 60, 294, 40, 24 }); P (zoomFit, { 104, 294, 80, 24 }); P (banner, { 196, 294, 952, 24 });
    P (cands, { 16, 324, 1132, 164 });
    for (int i = 0; i < 3; ++i) P (partBtns[(size_t) i], { 16 + i * 108, 500, 104, 28 });
    P (genBtn, { 16, 538, 250, 54 }); P (prevBtn, { 274, 548, 40, 34 }); P (nextBtn, { 318, 548, 40, 34 }); P (histLabel, { 366, 548, 220, 34 });
    P (simK, { 600, 496, 100, 92 }); P (varK, { 706, 496, 100, 92 });
    P (seedLabel, { 830, 496, 80, 20 }); P (seed, { 830, 518, 170, 30 }); P (seedNewBtn, { 1006, 518, 34, 30 }); P (previewBtn, { 830, 556, 230, 30 });
    P (saveDnaBtn, { 1056, 496, 92, 30 }); P (loadDnaBtn, { 1056, 532, 92, 30 });
    P (dnaInfo, { 16, 596, 1132, 52 });
}

void ReferencePage::paint (Graphics& g)
{
    const auto& P = lnf.pal(); g.setColour (P.panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 14);
    g.setColour (P.panel2); g.fillRoundedRectangle (8.0f, 488.0f, (float) kW - 16, 160.0f, 10);
    g.setColour (P.mute); g.setFont (lnf.font (12.0f));
    g.drawText (T ("rfTarget"), Rectangle<int> (rtl ? kW - 330 : 16, 480, 314, 18), rtl ? Justification::centredRight : Justification::centredLeft);
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

void ReferencePage::Wave::mouseDown (const MouseEvent& e) { if (o->proc.ref.clip() == nullptr) return; dragging = false; dragA = xToSec ((float) e.x); }
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
