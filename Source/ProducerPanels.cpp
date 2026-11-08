#include "ProducerPanels.h"

using namespace juce;

// Tür -> HitGen stili (yoksa yedek), varsayılan tempo, iskelet yapısı (HitGenProcessor::skStructKinds) ve süre
const HgGenre kHgGenres[5] = {
    { "afro", "Afro House", "blackcoffee", "orhan", 120, 1, 420 },
    { "mtechno", "Melodic Techno", "anyma", "monique", 124, 2, 420 },
    { "prog", "Progressive House", "prog", "tiesto", 124, 0, 360 },
    { "indie", "Indie Dance", "indie", "fisher", 118, 0, 360 },
    { "deep", "Deep House", "solomun", "domdolla", 122, 3, 420 } };

namespace
{
void setParam (HitGenProcessor& p, const char* id, float plain)
{
    if (auto* pr = p.apvts.getParameter (id)) { pr->beginChangeGesture(); pr->setValueNotifyingHost (pr->convertTo0to1 (plain)); pr->endChangeGesture(); }
}
void styleLabel (Label& l, hgui::LookAndFeel& lnf, float size, bool bold, bool mute)
{
    l.setFont (lnf.font (size, bold)); l.setColour (Label::textColourId, mute ? lnf.pal().mute : lnf.pal().ink);
    l.setJustificationType (Justification::centredLeft); l.setMinimumHorizontalScale (.8f);
}
}

// ---------------------------------------------------------------- Auto Producer
AutoProducerPanel::AutoProducerPanel (HitGenProcessor& p, hgui::LookAndFeel& l, const String& lg, std::function<void (const String&)> done)
    : proc (p), lnf (l), lang (lg), onDone (std::move (done))
{
    setSize (440, 368);
    for (auto* lb : { &title, &hint, &lGenre, &lBpm, &lKey, &lLen, &lEnergy }) addAndMakeVisible (*lb);
    styleLabel (title, lnf, 17, true, false); styleLabel (hint, lnf, 12.5f, false, true);
    hint.setJustificationType (Justification::topLeft);
    for (auto* lb : { &lGenre, &lBpm, &lKey, &lLen, &lEnergy }) styleLabel (*lb, lnf, 13, false, false);
    title.setText (T ("apTitle"), dontSendNotification); hint.setText (T ("apHint"), dontSendNotification);
    lGenre.setText (T ("apGenre"), dontSendNotification); lBpm.setText ("BPM", dontSendNotification); lKey.setText (T ("apKey"), dontSendNotification);
    lLen.setText (T ("skLen"), dontSendNotification); lEnergy.setText (T ("energy"), dontSendNotification);
    for (auto* c : { &genre, &key, &scale, &length }) addAndMakeVisible (*c);
    for (int i = 0; i < 5; ++i) genre.addItem (kHgGenres[i].name, i + 1);
    for (int i = 0; i < proc.data.noteNames.size(); ++i) key.addItem (proc.data.noteNames[i], i + 1);
    for (int i = 0; i < proc.data.scaleOrder.size(); ++i) scale.addItem (T (proc.data.scaleOrder[i]), i + 1);
    for (int i = 1; i < 8; ++i) { const int s = HitGenProcessor::kSkDurations[i]; length.addItem (String (s / 60) + ":00", i + 1); }
    genre.setTitle (T ("apGenre")); key.setTitle (T ("apKey")); scale.setTitle (T ("scale")); length.setTitle (T ("skLen"));
    for (auto* s : { &bpm, &energy })
    {
        addAndMakeVisible (*s); s->setSliderStyle (Slider::LinearHorizontal); s->setTextBoxStyle (Slider::TextBoxRight, false, 52, 24);
    }
    bpm.setRange (60, 190, 1); energy.setRange (0, 100, 1); bpm.setTitle ("BPM"); energy.setTitle (T ("energy"));
    energy.setValue (proc.apvts.getRawParameterValue ("energy")->load(), dontSendNotification);
    addAndMakeVisible (go); go.setButtonText (T ("apGo")); go.onClick = [this] { produce(); };
    genre.onChange = [this] { genreChanged(); };
    // başlangıç: güncel stile en yakın tür
    int gi = 1; const auto cur = proc.data.styleId ((int) proc.apvts.getRawParameterValue ("style")->load());
    for (int i = 0; i < 5; ++i) if (cur == kHgGenres[i].style || cur == kHgGenres[i].fallback) gi = i;
    genre.setSelectedItemIndex (gi, sendNotificationSync);
    key.setSelectedItemIndex ((int) proc.apvts.getRawParameterValue ("key")->load(), dontSendNotification);
    const int sc = (int) proc.apvts.getRawParameterValue ("scale")->load();
    scale.setSelectedItemIndex (sc > 0 ? sc - 1 : jmax (0, proc.data.scaleOrder.indexOf ("minor")), dontSendNotification);
}

void AutoProducerPanel::genreChanged()
{
    const auto& g = kHgGenres[jlimit (0, 4, genre.getSelectedItemIndex())];
    bpm.setValue (g.bpm, dontSendNotification);
    for (int i = 1; i < 8; ++i) if (HitGenProcessor::kSkDurations[i] == g.seconds) length.setSelectedItemIndex (i - 1, dontSendNotification);
}

void AutoProducerPanel::produce()
{
    const auto& g = kHgGenres[jlimit (0, 4, genre.getSelectedItemIndex())];
    int si = proc.data.styleIndex (g.style); if (si < 0) si = proc.data.styleIndex (g.fallback);
    if (si < 0) si = 0;
    if (proc.coLockedRole() >= 0) proc.coUnlock();
    setParam (proc, "style", (float) si);
    proc.applyStyleDefaults();
    setParam (proc, "seed", (float) Random::getSystemRandom().nextInt ({ 1, 99999 }));
    setParam (proc, "variation", (float) Random::getSystemRandom().nextInt (4));
    for (auto* a : { "altC", "altB", "altD", "altM" }) setParam (proc, a, 0.0f);
    setParam (proc, "key", (float) jmax (0, key.getSelectedItemIndex()));
    setParam (proc, "scale", (float) (jmax (0, scale.getSelectedItemIndex()) + 1));
    setParam (proc, "bpm", (float) bpm.getValue());
    setParam (proc, "energy", (float) energy.getValue());
    proc.userArtist = {}; proc.userPreset = {};
    proc.skSetStructure (g.structure);
    proc.skSetDuration (HitGenProcessor::kSkDurations[jlimit (1, 7, length.getSelectedItemIndex() + 1)]);
    proc.skel.active = true;
    proc.regenerateNow();
    const auto sk = proc.skeletonNow();
    const int s = (int) std::lround (sk.song.bars * 240.0 / jmax (1, sk.song.P.bpm));
    if (onDone) onDone (T ("apDone").replace ("{g}", g.name).replace ("{n}", String ((int) sk.secs.size())).replace ("{t}", String (s / 60) + ":" + String (s % 60).paddedLeft ('0', 2)));
}

void AutoProducerPanel::resized()
{
    title.setBounds (16, 10, 408, 26); hint.setBounds (16, 38, 408, 44);
    const int lx = 16, cx = 120, w = 304; int y = 90;
    for (auto [lb, c] : std::initializer_list<std::pair<Label*, Component*>> { { &lGenre, &genre }, { &lBpm, &bpm }, { &lKey, &key }, { &lLen, &length }, { &lEnergy, &energy } })
    {
        lb->setBounds (lx, y, 100, 30);
        if (c == &key) { key.setBounds (cx, y, 110, 30); scale.setBounds (cx + 118, y, w - 118, 30); }
        else c->setBounds (cx, y, w, 30);
        y += 40;
    }
    go.setBounds (16, y + 8, 408, 40);
}

void AutoProducerPanel::paint (Graphics& g) { g.fillAll (lnf.pal().panel.withAlpha (1.0f)); }

// ---------------------------------------------------------------- Co-Producer
CoProducerPanel::CoProducerPanel (HitGenProcessor& p, hgui::LookAndFeel& l, const String& lg, std::function<void (const String&)> done)
    : proc (p), lnf (l), lang (lg), onDone (std::move (done))
{
    setSize (440, 352);
    for (auto* lb : { &title, &hint, &info, &status }) addAndMakeVisible (*lb);
    styleLabel (title, lnf, 17, true, false); styleLabel (hint, lnf, 12.5f, false, true); styleLabel (info, lnf, 13.5f, true, false); styleLabel (status, lnf, 12.5f, false, false);
    for (auto* lb : { &hint, &info, &status }) lb->setJustificationType (Justification::topLeft);
    title.setText (T ("coTitle"), dontSendNotification); hint.setText (T ("coHint"), dontSendNotification);
    for (auto* b : { &load, &apply, &alt, &unlock }) addAndMakeVisible (*b);
    load.setButtonText (T ("coLoad")); apply.setButtonText (T ("coApply")); alt.setButtonText (T ("coNext")); unlock.setButtonText (T ("coUnlock"));
    addAndMakeVisible (role); role.setTitle (T ("coRole"));
    role.addItem (T ("coRoleMel"), 1); role.addItem (T ("coRoleChords"), 2); role.addItem (T ("coRoleBass"), 3);
    role.onChange = [this] { um.role = role.getSelectedItemIndex() == 1 ? hg::kChords : role.getSelectedItemIndex() == 2 ? hg::kBass : hg::kMelody; cands.clear(); idx = 0; updateButtons(); };
    load.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("coLoad"), File::getSpecialLocation (File::userMusicDirectory), "*.mid;*.midi");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, sp = Component::SafePointer<CoProducerPanel> (this)] (const FileChooser& fc)
        { if (sp != nullptr && fc.getResult().existsAsFile()) loadFile (fc.getResult()); });
    };
    apply.onClick = [this] { complete(); };
    alt.onClick = [this] { next(); };
    unlock.onClick = [this] { proc.coUnlock(); status.setText (T ("coUnlocked"), dontSendNotification); updateButtons(); if (onDone) onDone (T ("coUnlocked")); };
    if (proc.coLockedRole() >= 0)
        status.setText (T ("coLocked").replace ("{n}", proc.coLockedName()).replace ("{r}", roleName (proc.coLockedRole())), dontSendNotification);
    info.setText (T ("coDrop"), dontSendNotification);
    updateButtons();
}

bool CoProducerPanel::isInterestedInFileDrag (const StringArray& files)
{
    for (auto& f : files) if (f.endsWithIgnoreCase (".mid") || f.endsWithIgnoreCase (".midi")) return true;
    return false;
}
void CoProducerPanel::filesDropped (const StringArray& files, int, int)
{
    for (auto& f : files) if (f.endsWithIgnoreCase (".mid") || f.endsWithIgnoreCase (".midi")) { loadFile (File (f)); return; }
}

bool CoProducerPanel::loadFile (const File& f)
{
    String err;
    hg::UserMidi m;
    if (! hg::readUserMidi (f, m, err)) { status.setText (T (err), dontSendNotification); return false; }
    um = std::move (m); kg = hg::detectKey (um.notes); loaded = true; cands.clear(); idx = 0;
    role.setSelectedItemIndex (um.role == hg::kChords ? 1 : um.role == hg::kBass ? 2 : 0, dontSendNotification);
    StringArray ks;   // kısa döngülerde göreli ve komşu tonlar ayırt edilemeyebilir: adayların hepsi gösterilir, uyum araması seçer
    for (auto& g : hg::keyCandidates (um.notes, 3)) ks.add (proc.data.noteNames[g.key] + " " + T (g.minor ? "minor" : "major"));
    info.setText (T ("coInfo").replace ("{k} {s}", ks.joinIntoString (" / ")).replace ("{k}", ks[0]).replace ("{s}", {}).replace ("{c}", String (jlimit (0, 100, roundToInt (kg.coverage * 100))))
                  .replace ("{r}", roleName (um.role)).replace ("{b}", String (um.bars)).replace ("{n}", String ((int) um.notes.size())), dontSendNotification);
    status.setText (T ("coReady"), dontSendNotification);
    updateButtons();
    return true;
}

void CoProducerPanel::complete()
{
    if (! loaded) return;
    if (cands.empty())
    {
        auto base = proc.generatorParams();
        base.key = kg.key; base.scale = kg.minor ? "minor" : "major";
        cands = hg::coSearch (proc.data, base, um, 12, 5, hg::keyCandidates (um.notes, 4)); idx = 0;
    }
    if (cands.empty()) return;
    const auto& c = cands[(size_t) idx];
    proc.coLock (um, c);
    const String msg = T ("coFit").replace ("{f}", String (roundToInt (c.fit * 100))).replace ("{k}", proc.data.noteNames[c.P.key])
                       .replace ("{s}", T (c.P.scale)).replace ("{i}", String (idx + 1)).replace ("{m}", String ((int) cands.size()));
    status.setText (msg, dontSendNotification);
    updateButtons();
    if (onDone) onDone (msg);
}

void CoProducerPanel::next()
{
    if (cands.empty()) { complete(); return; }
    idx = (idx + 1) % (int) cands.size();
    complete();
}

void CoProducerPanel::updateButtons()
{
    apply.setEnabled (loaded); alt.setEnabled (loaded && ! cands.empty()); unlock.setEnabled (proc.coLockedRole() >= 0); role.setEnabled (loaded);
}

void CoProducerPanel::resized()
{
    title.setBounds (16, 10, 408, 26); hint.setBounds (16, 38, 408, 58);
    load.setBounds (16, 102, 200, 34); role.setBounds (224, 102, 200, 34);
    info.setBounds (16, 144, 408, 46);
    apply.setBounds (16, 198, 200, 38); alt.setBounds (224, 198, 200, 38);
    status.setBounds (16, 244, 408, 58); unlock.setBounds (16, 306, 408, 32);
}

void CoProducerPanel::paint (Graphics& g)
{
    g.fillAll (lnf.pal().panel.withAlpha (1.0f));
}
