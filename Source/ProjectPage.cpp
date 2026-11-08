#include "ProjectPage.h"

using namespace juce;

// Şablon listesi: her stil bir satır. Projelerim: proc.presets öğeleri.
struct ProjectPage::Model : ListBoxModel
{
    Model (ProjectPage& o, bool t) : owner (o), templates (t) {}
    ProjectPage& owner; bool templates;
    int getNumRows() override { return templates ? owner.data.numStyles() : owner.proc.presets.items().size(); }
    String textFor (int row) const
    {
        if (templates)
            return row >= 0 && row < owner.data.numStyles() ? owner.data.style (row)["artist"].toString() : String();
        const auto& it = owner.proc.presets.items();
        return row >= 0 && row < it.size() ? it[row].display() : String();
    }
    String getNameForRow (int row) override { return textFor (row); }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool selected) override
    {
        if (row < 0 || row >= getNumRows()) return;
        const auto& P = owner.lnf.pal();
        auto r = Rectangle<int> (0, 0, w, h).reduced (2).toFloat();
        g.setColour (selected ? P.accent.withAlpha (.16f) : P.panel2); g.fillRoundedRectangle (r, 8);
        if (selected) { g.setColour (P.accent); g.drawRoundedRectangle (r, 8, 1.2f); }
        g.setColour (P.accent); g.fillRect (owner.rtl ? r.getRight() - 4 : r.getX(), r.getY() + 6, 4.0f, r.getHeight() - 12);
        g.setColour (P.ink); g.setFont (owner.lnf.font (15, true));
        g.drawText (textFor (row), r.reduced (14, 2).toNearestInt(), owner.rtl ? Justification::centredRight : Justification::centredLeft);
    }
    void activate (int row) { if (templates) owner.startTemplate (row); else owner.openRow (row); }
    void listBoxItemDoubleClicked (int row, const MouseEvent&) override { activate (row); }
    void returnKeyPressed (int row) override { activate (row); }
    void selectedRowsChanged (int) override { owner.pendingDelete = -1; owner.updateButtons(); owner.repaint(); }
};

// ---------------------------------------------------------------- iskelet listesi: bölüm satırları + ipuçları
// Bölüm satırındaki çip o bölümün MIDI'sini, ipucu satırındaki çip önerilen sample'ı DAW'a sürükler.
struct ProjectPage::CueModel : ListBoxModel
{
    explicit CueModel (ProjectPage& o) : owner (o) {}
    ProjectPage& owner;
    struct RowComp : Component, SettableTooltipClient
    {
        RowComp (ProjectPage& o) : owner (o) {}
        ProjectPage& owner; int row = -1; bool selected = false, dragging = false;
        const Row* r() const { return row >= 0 && row < (int) owner.rows.size() ? &owner.rows[(size_t) row] : nullptr; }
        Rectangle<int> chip() const { auto b = getLocalBounds().reduced (4, 3); return owner.rtl ? b.removeFromLeft (196) : b.removeFromRight (196); }
        bool hasChip() const { auto* x = r(); return x != nullptr && (x->section || x->file != File()); }
        void paint (Graphics& g) override
        {
            auto* x = r(); if (x == nullptr) return;
            const auto& P = owner.lnf.pal(); const bool rt = owner.rtl;
            auto b = getLocalBounds().reduced (2, 1).toFloat();
            const auto j = rt ? Justification::centredRight : Justification::centredLeft;
            if (x->section)
            {
                g.setColour (selected ? P.accent.withAlpha (.22f) : P.panel2.brighter (.06f)); g.fillRoundedRectangle (b, 6);
                const auto& s = owner.sk.secs[(size_t) x->sec];
                g.setColour (P.accent); g.setFont (owner.lnf.font (14, true));
                auto t = getLocalBounds().reduced (10, 0); if (rt) t.removeFromLeft (200); else t.removeFromRight (200);
                g.drawText (s.name + "   " + owner.T ("skBarsRange").replace ("{a}", String (s.bar + 1)).replace ("{b}", String (s.bar + s.bars))
                            + "   " + owner.timeOf (s.bar) + " - " + owner.timeOf (s.bar + s.bars), t, j, true);
            }
            else
            {
                if (selected) { g.setColour (P.accent.withAlpha (.14f)); g.fillRoundedRectangle (b, 6); }
                auto t = getLocalBounds().reduced (10, 0); if (rt) t.removeFromLeft (200); else t.removeFromRight (200);
                auto tm = rt ? t.removeFromRight (100) : t.removeFromLeft (100);
                g.setColour (P.mute); g.setFont (owner.lnf.font (12));
                g.drawText (owner.T ("skBarN").replace ("{n}", String (x->cue.bar + 1)) + "  " + owner.timeOf (x->cue.bar), tm, j, true);
                g.setColour (P.ink); g.setFont (owner.lnf.font (13));
                g.drawFittedText (owner.T ("cu_" + x->cue.type), t, j, 2, .85f);
            }
            if (hasChip())
            {
                auto c = chip().toFloat();
                g.setColour (x->section ? P.accent.withAlpha (.85f) : P.panel2); g.fillRoundedRectangle (c, 6);
                g.setColour (x->section ? P.accent : P.line); g.drawRoundedRectangle (c, 6, 1);
                g.setColour (x->section ? P.panel : P.ink); g.setFont (owner.lnf.font (12, x->section));
                g.drawFittedText (x->section ? owner.T ("skDragMidi") : "\xe2\x99\xab " + x->file.getFileName(), c.reduced (8, 2).toNearestInt(), Justification::centred, 1, .7f);
            }
            else if (x->hasAudio)
            {
                g.setColour (P.mute); g.setFont (owner.lnf.font (11.5f));
                g.drawFittedText (owner.T ("skHitgenSound"), chip().reduced (8, 2), Justification::centred, 1, .8f);
            }
        }
        void mouseDown (const MouseEvent& e) override { dragging = false; owner.cueList.selectRow (row); if (auto* x = r(); x != nullptr && e.mods.isPopupMenu()) owner.showSectionMenu (x->sec); }
        void mouseDrag (const MouseEvent& e) override
        {
            auto* x = r();
            if (dragging || x == nullptr || e.getDistanceFromDragStart() < 5 || ! chip().contains (e.getMouseDownPosition()) || ! hasChip()) return;
            dragging = true;
            const auto f = x->section ? owner.sectionMidiFile (x->sec) : x->file;
            if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
        }
        void mouseUp (const MouseEvent& e) override
        {
            auto* x = r();
            if (x != nullptr && ! dragging && e.mouseWasClicked() && ! e.mods.isPopupMenu())
            {
                if (chip().contains (e.getPosition()) && ! x->section && x->file != File()) x->file.revealToUser();
                else if (x->section) owner.showSectionMenu (x->sec);
            }
            dragging = false;
        }
    };
    int getNumRows() override { return (int) owner.rows.size(); }
    void paintListBoxItem (int, Graphics&, int, int, bool) override {}
    Component* refreshComponentForRow (int row, bool selected, Component* existing) override
    {
        auto* c = dynamic_cast<RowComp*> (existing);
        if (c == nullptr) { delete existing; c = new RowComp (owner); }
        c->row = row; c->selected = selected;
        if (auto* x = c->r())
        {
            c->setTooltip (x->section ? owner.T ("skDragMidiHint") : x->file != File() ? owner.T ("skDragSampleHint") + "\n" + x->file.getFullPathName() : String());
            c->setTitle (getNameForRow (row));
        }
        c->repaint();
        return c;
    }
    String getNameForRow (int row) override
    {
        if (row < 0 || row >= (int) owner.rows.size()) return {};
        const auto& x = owner.rows[(size_t) row];
        if (x.section) { const auto& s = owner.sk.secs[(size_t) x.sec]; return s.name + ", " + owner.T ("skBarsRange").replace ("{a}", String (s.bar + 1)).replace ("{b}", String (s.bar + s.bars)); }
        return owner.T ("skBarN").replace ("{n}", String (x.cue.bar + 1)) + ", " + owner.T ("cu_" + x.cue.type) + (x.file != File() ? ", " + x.file.getFileName() : String());
    }
    void returnKeyPressed (int row) override { if (row >= 0 && row < (int) owner.rows.size()) owner.showSectionMenu (owner.rows[(size_t) row].sec); }
};

// ---------------------------------------------------------------- bölüm şeridi
// Kutular uzunluklarıyla orantılı; altında ipucu işaretleri ve dakika cetveli. Tıkla: bölüm menüsü.
struct ProjectPage::SkView : Component
{
    explicit SkView (ProjectPage& o) : owner (o) { setWantsKeyboardFocus (true); setMouseCursor (MouseCursor::PointingHandCursor); }
    ProjectPage& owner;
    int total() const { return owner.sk.secs.empty() ? 0 : owner.sk.secs.back().bar + owner.sk.secs.back().bars; }
    float xOf (int bar) const
    {
        const int T = total(); const float w = (float) getWidth();
        const float x = T > 0 ? w * (float) bar / (float) T : 0;
        return owner.rtl ? w - x : x;
    }
    int sectionAt (Point<int> p) const
    {
        for (int i = 0; i < (int) owner.sk.secs.size(); ++i)
        {
            const auto& s = owner.sk.secs[(size_t) i];
            const float a = xOf (s.bar), b = xOf (s.bar + s.bars);
            if ((float) p.x >= std::min (a, b) && (float) p.x < std::max (a, b)) return i;
        }
        return -1;
    }
    void paint (Graphics& g) override
    {
        const auto& P = owner.lnf.pal();
        const int T = total(); if (T <= 0) return;
        const float H = 58;
        for (int i = 0; i < (int) owner.sk.secs.size(); ++i)
        {
            const auto& s = owner.sk.secs[(size_t) i];
            const float a = xOf (s.bar), b = xOf (s.bar + s.bars);
            Rectangle<float> r (std::min (a, b) + 1.5f, 0, std::abs (b - a) - 3, H);
            const bool drop = s.kind == "drop" || s.kind == "groove", build = s.kind == "build", brk = s.kind == "break";
            auto fill = drop ? P.accent : build ? P.accent.withAlpha (.45f) : brk ? P.panel2 : P.panel2.brighter (.08f);
            g.setColour (owner.proc.skel.active ? fill : fill.withMultipliedAlpha (.55f));
            g.fillRoundedRectangle (r, 6);
            const bool sel = i == owner.selSec;
            g.setColour (sel ? P.focus : brk ? P.accent.withAlpha (.5f) : P.line); g.drawRoundedRectangle (r, 6, sel ? 2.2f : 1.0f);
            g.setColour (drop ? P.panel : P.ink); g.setFont (owner.lnf.font (r.getWidth() > 64 ? 12.5f : 10.5f, true));
            g.drawFittedText (s.name, r.reduced (3, 5).withHeight (20).toNearestInt(), Justification::centred, 1, .6f);
            g.setFont (owner.lnf.font (11)); g.setColour (drop ? P.panel : P.mute);
            g.drawFittedText (owner.T ("pjBars").replace ("{n}", String (s.bars)), r.reduced (3, 4).withTrimmedTop (26).toNearestInt(), Justification::centred, 1, .6f);
        }
        // ipucu işaretleri: sesli ipuçları vurgulu
        for (auto& c : owner.sk.cues)
        {
            if (c.type == "pad") continue;
            const float x = xOf (c.bar), x2 = xOf (c.bar + c.len);
            const bool audio = hg::cueAudio (c.type).render;
            g.setColour (audio ? P.accent : P.mute.withAlpha (.7f));
            g.fillRect (Rectangle<float> (std::min (x, x2), H + (audio ? 4.0f : 13.0f), std::max (2.0f, std::abs (x2 - x)), audio ? 6.0f : 4.0f));
        }
        // dakika cetveli
        const int bpm = juce::jmax (1, owner.sk.song.P.bpm);
        const double secTotal = T * 240.0 / bpm;
        g.setFont (owner.lnf.font (10.5f)); g.setColour (P.mute);
        for (int m = 0; m * 60 <= (int) secTotal; ++m)
        {
            const float x = xOf ((int) std::lround (m * 60.0 * bpm / 240.0));
            g.fillRect (x, H + 20, 1.0f, 5.0f);
            const float lx = jlimit (0.0f, (float) getWidth() - 44.0f, x - 22.0f);
            g.drawText (String (m) + ":00", Rectangle<float> (lx, H + 24, 44, 14), x - 22.0f < 0 ? Justification::centredLeft : x + 22.0f > (float) getWidth() ? Justification::centredRight : Justification::centred);
        }
        if (hasKeyboardFocus (false)) { g.setColour (P.focus); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (.5f), 6, 1.5f); }
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (! e.mouseWasClicked()) return;
        const int i = sectionAt (e.getPosition());
        if (i >= 0) { owner.selSec = i; repaint(); owner.showSectionMenu (i); }
    }
    bool keyPressed (const KeyPress& k) override
    {
        const int n = (int) owner.sk.secs.size(); if (n == 0) return false;
        const int dir = owner.rtl ? -1 : 1;
        if (k == KeyPress::rightKey) { owner.selSec = jlimit (0, n - 1, owner.selSec + dir); repaint(); announce(); return true; }
        if (k == KeyPress::leftKey) { owner.selSec = jlimit (0, n - 1, owner.selSec - dir); repaint(); announce(); return true; }
        if (k == KeyPress::returnKey || k == KeyPress::spaceKey) { owner.showSectionMenu (jlimit (0, n - 1, owner.selSec)); return true; }
        if (k == KeyPress::deleteKey || k == KeyPress::backspaceKey) { owner.sectionCommand (owner.selSec, cmdDelete); return true; }
        if (k.getTextCharacter() == '+') { owner.sectionCommand (owner.selSec, cmdLonger); return true; }
        if (k.getTextCharacter() == '-') { owner.sectionCommand (owner.selSec, cmdShorter); return true; }
        return false;
    }
    void announce()
    {
        if (owner.selSec < 0 || owner.selSec >= (int) owner.sk.secs.size()) return;
        const auto& s = owner.sk.secs[(size_t) owner.selSec];
        if (auto* h = getAccessibilityHandler()) h->postAnnouncement (s.name + ", " + owner.T ("pjBars").replace ("{n}", String (s.bars)), AccessibilityHandler::AnnouncementPriority::medium);
    }
    void focusGained (FocusChangeType) override { if (owner.selSec < 0) owner.selSec = 0; repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }
};

juce::Component* ProjectPage::skeletonView() const { return skView.get(); }

// ---------------------------------------------------------------- sayfa
ProjectPage::ProjectPage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data)
{
    setFocusContainerType (FocusContainerType::focusContainer);
    for (auto* lb : { &help, &status, &skHint, &libInfo, &exportInfo }) { addAndMakeVisible (*lb); lb->setFont (lnf.font (13)); lb->setMinimumHorizontalScale (.8f); }
    help.setJustificationType (Justification::topLeft); status.setJustificationType (Justification::topLeft);
    addAndMakeVisible (nameEd); nameEd.setFont (lnf.font (16)); nameEd.setInputRestrictions (80); nameEd.setIndents (8, 6);
    nameEd.onReturnKey = [this] { save(); };
    for (auto* b : { &startBtn, &saveBtn, &openBtn, &deleteBtn, &genBtn, &addSecBtn, &libBtn, &drumsBtn, &exportBtn }) addAndMakeVisible (*b);
    for (auto* c : { &structBox, &lenBox }) addAndMakeVisible (*c);
    const int which[3] = { 5, 6, 4 };
    hgui::DragTile* tiles[3] = { &fullTile, &full0Tile, &loopTile };
    for (int i = 0; i < 3; ++i)
    {
        auto* t = tiles[i]; addAndMakeVisible (*t);
        t->palette = [this]() -> const hgui::Palette& { return lnf.pal(); };
        t->font = [this] (float h, bool b) { return lnf.font (h, b); };
        t->makeFile = [this, w = which[i]] { return proc.writeMidi (w); };
        t->onSaved = [this] (const File& f) { say (T ("pSavedTo").replace ("{f}", f.getFullPathName())); };
    }
    tmplModel = std::make_unique<Model> (*this, true); mineModel = std::make_unique<Model> (*this, false);
    for (auto* lb : { &tmplList, &mineList }) { addAndMakeVisible (*lb); lb->setRowHeight (40); lb->setWantsKeyboardFocus (true); }
    tmplList.setModel (tmplModel.get()); mineList.setModel (mineModel.get());
    cueModel = std::make_unique<CueModel> (*this);
    addAndMakeVisible (cueList); cueList.setRowHeight (30); cueList.setWantsKeyboardFocus (true); cueList.setModel (cueModel.get());
    skView = std::make_unique<SkView> (*this); addAndMakeVisible (*skView);
    tmplList.selectRow (juce::jlimit (0, juce::jmax (0, data.numStyles() - 1), (int) proc.apvts.getRawParameterValue ("style")->load()));
    startBtn.onClick = [this] { startTemplate (tmplList.getSelectedRow()); };
    saveBtn.onClick = [this] { save(); };
    openBtn.onClick = [this] { openRow (mineList.getSelectedRow()); };
    deleteBtn.onClick = [this]
    {
        const int row = mineList.getSelectedRow(); const auto& it = proc.presets.items();
        if (row < 0 || row >= it.size()) return;
        if (pendingDelete != row) { pendingDelete = row; say (T ("prConfirm")); return; }
        const auto name = it[row].display();
        proc.presets.remove (it[row].file); pendingDelete = -1;
        mineList.updateContent(); say (T ("prDeleted").replace ("{n}", name)); updateButtons();
    };
    genBtn.onClick = [this] { generateProject(); };
    addSecBtn.onClick = [this]
    {
        PopupMenu m;
        for (int i = 0; i < hg::kNumSkKinds; ++i) m.addItem (i + 1, kindName (hg::kSkKinds[i]));
        Component::SafePointer<ProjectPage> sp (this);   // MSVC: iç içe lambda'da "this" ile ilklendirme derlenmiyor
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (addSecBtn), [this, sp] (int r)
        {
            if (sp == nullptr || r <= 0) return;
            const int at = selSec >= 0 ? selSec : (int) sk.secs.size() - 1;
            sectionCommand (at, cmdInsert, r - 1);
        });
    };
    structBox.onChange = [this] { if (structBox.getSelectedItemIndex() >= 0) setStructure (structBox.getSelectedItemIndex()); };
    lenBox.onChange = [this] { if (lenBox.getSelectedItemIndex() >= 0 && lenBox.getSelectedId() < 100) setDurationIndex (lenBox.getSelectedItemIndex()); };
    libBtn.onClick = [this] { showLibraryMenu(); };
    drumsBtn.onClick = [this]
    {
        const int n = proc.loadSuggestedDrums();
        say (n > 0 ? T ("skDrumsDone").replace ("{n}", String (n)) : T ("skDrumsNone"));
        if (n > 0 && onLoaded) onLoaded();
    };
    exportBtn.onClick = [this] { exportProject(); };
    nameEd.setText (proc.userPreset, false);
    proc.ensureLibrary();
    refreshSkeleton();
    updateButtons();
}

ProjectPage::~ProjectPage() { tmplList.setModel (nullptr); mineList.setModel (nullptr); cueList.setModel (nullptr); }

void ProjectPage::say (const String& s) { status.setText (s, dontSendNotification); status.setColour (Label::textColourId, lnf.pal().ink); }

void ProjectPage::updateButtons()
{
    const int row = mineList.getSelectedRow(); const bool has = row >= 0 && row < proc.presets.items().size();
    for (auto* b : { &openBtn, &deleteBtn }) b->setEnabled (has);
    startBtn.setEnabled (tmplList.getSelectedRow() >= 0);
    auto L = proc.library();
    drumsBtn.setEnabled (L != nullptr && ! L->items.empty() && ! proc.scanning());
    exportBtn.setEnabled (! proc.exporting() && ! sk.secs.empty());
    genBtn.setButtonText (proc.skel.active ? T ("skRegen") : T ("skGen"));
}

void ProjectPage::startTemplate (int style)
{
    if (style < 0 || style >= data.numStyles()) return;
    if (auto* pr = proc.apvts.getParameter ("style")) pr->setValueNotifyingHost (pr->convertTo0to1 ((float) style));
    proc.applyStyleDefaults();   // stile uygun ses, tempo ve pump
    proc.newIdea();              // stile uygun ton, makam, enerji, özgünlük, swing + yeni tohum
    proc.userArtist = {}; proc.userPreset = {}; nameEd.setText ({}, false);   // yeni, henüz adsız proje
    if (proc.skel.structIdx == HitGenProcessor::kNumSkStructs - 1) proc.skSetStructure (proc.skel.structIdx);   // stilin yapısı yeni stile uysun
    say (T ("pjStarted").replace ("{s}", data.style (style)["artist"].toString()));
    if (onLoaded) onLoaded();
    shownRes = nullptr; refreshSkeleton();
}

void ProjectPage::save()
{
    const auto n = nameEd.getText().trim();
    if (n.isEmpty()) { say (T ("prNeed")); if (nameEd.isShowing()) nameEd.grabKeyboardFocus(); return; }
    const String a = proc.userArtist.isNotEmpty() ? proc.userArtist : String ("Wavejackers");
    String path;
    if (! proc.savePreset (a, n, path)) { say (T ("saveFail")); return; }
    mineList.updateContent();
    const int k = proc.presets.indexOf (a, n); if (k >= 0) mineList.selectRow (k);
    say (T ("prSaved").replace ("{n}", a + " - " + n));
    if (onLoaded) onLoaded();
}

void ProjectPage::openRow (int row)
{
    const auto& it = proc.presets.items();
    if (row < 0 || row >= it.size()) return;
    const auto info = it[row];
    if (! proc.loadPreset (info.file)) { say (T ("prBad")); return; }
    nameEd.setText (info.name, false);
    say (T ("prLoaded").replace ("{n}", info.display()));
    if (onLoaded) onLoaded();
    shownRes = nullptr; refreshSkeleton();
}

// ---------------------------------------------------------------- iskelet
String ProjectPage::timeOf (int bar) const
{
    const int bpm = jmax (1, sk.song.P.bpm);
    const int s = (int) std::lround (bar * 240.0 / bpm);
    return String (s / 60) + ":" + String (s % 60).paddedLeft ('0', 2);
}

void ProjectPage::refreshSkeleton()
{
    auto r = proc.result();
    shownRes = r.get(); shownLib = proc.libRevision;
    sk = proc.skeletonNow(); shownBpm = sk.song.P.bpm;
    if (selSec >= (int) sk.secs.size()) selSec = (int) sk.secs.size() - 1;
    rows.clear();
    const auto picks = proc.skPicks (sk);
    for (int i = 0; i < (int) sk.secs.size(); ++i)
    {
        Row h; h.section = true; h.sec = i; rows.push_back (h);
        for (auto& c : sk.cues)
        {
            if (c.sec != i || c.type == "pad") continue;
            Row x; x.sec = i; x.cue = c; x.hasAudio = hg::cueAudio (c.type).render;
            for (auto& pk : picks) if (pk.first.bar == c.bar && pk.first.type == c.type) { x.file = pk.second; break; }
            rows.push_back (x);
        }
    }
    cueList.updateContent(); cueList.repaint();
    syncCombos();
    if (skView) skView->repaint();
    updateButtons();
    repaint();
}

void ProjectPage::syncCombos()
{
    structBox.setSelectedItemIndex (proc.skel.kinds != proc.skStructKinds (proc.skel.structIdx) ? -1 : proc.skel.structIdx, dontSendNotification);
    int li = -1;
    if (! proc.skel.custom) for (int i = 0; i < 8; ++i) if (HitGenProcessor::kSkDurations[i] == proc.skel.durSec) li = i;
    lenBox.setSelectedItemIndex (li, dontSendNotification);
}

void ProjectPage::generateProject()
{
    if (proc.skel.kinds.isEmpty()) proc.skSetStructure (proc.skel.structIdx);
    const bool again = proc.skel.active;
    if (again) proc.newIdea();   // ikinci basış: aynı iskeletle yeni bir fikir
    proc.skel.active = true;
    refreshSkeleton();
    say (T ("skGenerated").replace ("{n}", String ((int) sk.secs.size())).replace ("{t}", timeOf (sk.song.bars)).replace ("{b}", String (sk.song.bars)));
    if (onLoaded) onLoaded();
}

void ProjectPage::setStructure (int idx)
{
    proc.skSetStructure (idx); selSec = -1;
    refreshSkeleton();
}

void ProjectPage::setDurationIndex (int idx)
{
    proc.skSetDuration (HitGenProcessor::kSkDurations[jlimit (0, 7, idx)]);
    refreshSkeleton();
}

void ProjectPage::sectionCommand (int section, int cmd, int arg)
{
    auto kinds = proc.skel.kinds;
    const int n = kinds.size();
    if (n == 0) return;
    section = jlimit (0, n - 1, section);
    const auto keepBars = [this] { std::vector<int> b; for (auto& x : sk.secs) b.push_back (x.bars); return b; };
    switch (cmd)
    {
        case cmdType: if (arg >= 0 && arg < hg::kNumSkKinds) { kinds.set (section, hg::kSkKinds[arg]); proc.skSetKinds (kinds, true); } break;
        case cmdLonger: proc.skSetBars (section, sk.secs[(size_t) section].bars + 8); break;
        case cmdShorter: proc.skSetBars (section, jmax (8, sk.secs[(size_t) section].bars - 8)); break;
        case cmdLeft: case cmdRight:
        {
            const int to = section + (cmd == cmdLeft ? -1 : 1);
            if (to < 0 || to >= n) return;
            auto b = keepBars(); std::swap (b[(size_t) section], b[(size_t) to]);
            const bool custom = proc.skel.custom;
            kinds.getReference (section).swapWith (kinds.getReference (to)); proc.skSetKinds (kinds, true);
            if (custom) proc.skel.bars = b;
            selSec = to; break;
        }
        case cmdDup: case cmdInsert:
        {
            if (n >= 24) { say (T ("skTooMany")); return; }
            kinds.insert (section + 1, cmd == cmdDup ? kinds[section] : String (hg::kSkKinds[jlimit (0, hg::kNumSkKinds - 1, arg)]));
            proc.skSetKinds (kinds); selSec = section + 1; break;
        }
        case cmdDelete:
            if (n <= 1) return;
            kinds.remove (section); proc.skSetKinds (kinds); selSec = jmin (section, n - 2); break;
        default: return;
    }
    refreshSkeleton();
    if (skView && selSec >= 0 && selSec < (int) sk.secs.size())
        if (auto* h = skView->getAccessibilityHandler())
            h->postAnnouncement (sk.secs[(size_t) selSec].name + ", " + T ("pjBars").replace ("{n}", String (sk.secs[(size_t) selSec].bars)) + ", " + timeOf (sk.song.bars), AccessibilityHandler::AnnouncementPriority::medium);
}

void ProjectPage::showSectionMenu (int section)
{
    if (section < 0 || section >= (int) sk.secs.size()) return;
    selSec = section; if (skView) skView->repaint();
    const int n = (int) sk.secs.size();
    PopupMenu types, ins;
    for (int i = 0; i < hg::kNumSkKinds; ++i)
    {
        types.addItem (100 + i, kindName (hg::kSkKinds[i]), true, sk.secs[(size_t) section].kind == hg::kSkKinds[i]);
        ins.addItem (200 + i, kindName (hg::kSkKinds[i]));
    }
    PopupMenu m;
    m.addSectionHeader (sk.secs[(size_t) section].name + " (" + T ("pjBars").replace ("{n}", String (sk.secs[(size_t) section].bars)) + ")");
    m.addSubMenu (T ("skType"), types);
    m.addItem (2, T ("skLonger")); m.addItem (3, T ("skShorter"), sk.secs[(size_t) section].bars > 8);
    m.addItem (4, T ("skLeft"), section > 0); m.addItem (5, T ("skRight"), section < n - 1);
    m.addItem (6, T ("skDup")); m.addSubMenu (T ("skInsert"), ins);
    m.addSeparator(); m.addItem (8, T ("skDel"), n > 1);
    Component* target = skView.get();
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (target), [this, section, sp = Component::SafePointer<ProjectPage> (this)] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        if (r >= 200) sectionCommand (section, cmdInsert, r - 200);
        else if (r >= 100) sectionCommand (section, cmdType, r - 100);
        else sectionCommand (section, r == 2 ? cmdLonger : r == 3 ? cmdShorter : r == 4 ? cmdLeft : r == 5 ? cmdRight : r == 6 ? cmdDup : cmdDelete);
    });
}

void ProjectPage::showLibraryMenu()
{
    PopupMenu m;
    m.addItem (1, T ("skLibAdd"));
    const auto f = proc.sampleFolders();
    if (! f.isEmpty())
    {
        m.addItem (2, T ("skLibRescan"));
        m.addSeparator();
        for (int i = 0; i < f.size(); ++i) m.addItem (100 + i, T ("skLibRemove").replace ("{f}", File (f[i]).getFileName()));
    }
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (libBtn), [this, f, sp = Component::SafePointer<ProjectPage> (this)] (int r)
    {
        if (sp == nullptr || r <= 0) return;
        if (r == 2) { proc.setSampleFolders (f); return; }
        if (r >= 100) { auto g = f; g.remove (r - 100); proc.setSampleFolders (g); refreshSkeleton(); return; }
        chooser = std::make_unique<FileChooser> (T ("skLibAdd"), File::getSpecialLocation (File::userMusicDirectory));
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories, [this, sp] (const FileChooser& fc)
        {
            if (sp == nullptr) return;
            const auto d = fc.getResult();
            if (d == File() || ! d.isDirectory()) return;
            auto g = proc.sampleFolders(); g.addIfNotAlreadyThere (d.getFullPathName());
            proc.setSampleFolders (g);
            say (T ("skLibScanning").replace ("{n}", "0"));
        });
    });
}

File ProjectPage::sectionMidiFile (int section) const
{
    if (section < 0 || section >= (int) sk.secs.size()) return {};
    auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("WavejackersHitGen"); dir.createDirectory();
    auto f = dir.getChildFile (File::createLegalFileName (proc.baseName() + "_" + String (section + 1).paddedLeft ('0', 2) + "-" + sk.secs[(size_t) section].name + ".mid"));
    const auto bytes = proc.sectionMidi (section);
    if (bytes.getSize() > 0) f.replaceWithData (bytes.getData(), bytes.getSize());
    return f;
}

String ProjectPage::planText() const
{
    String t;
    const auto& P = sk.song.P;
    const auto styleName = data.styleById (P.style)["artist"].toString();
    t << "Wavejackers HitGen - " << (proc.userPreset.isNotEmpty() ? proc.userPreset : proc.baseName()) << "\n";
    t << styleName << " | " << data.noteNames[jlimit (0, 11, P.key)] << " " << data.scale (P.scale)["n"].toString() << " | " << P.bpm << " BPM | "
      << T ("pjBars").replace ("{n}", String (sk.song.bars)) << " | " << timeOf (sk.song.bars) << "\n";
    t << T ("skP_" + sk.profile) << "\n\n";
    t << T ("skPlanSections") << "\n";
    for (int i = 0; i < (int) sk.secs.size(); ++i)
    {
        const auto& s = sk.secs[(size_t) i];
        t << String (i + 1).paddedLeft ('0', 2) << "  " << s.name.paddedRight (' ', 14) << T ("skBarsRange").replace ("{a}", String (s.bar + 1)).replace ("{b}", String (s.bar + s.bars)).paddedRight (' ', 18)
          << timeOf (s.bar) << " - " << timeOf (s.bar + s.bars) << "   " << T ("pjBars").replace ("{n}", String (s.bars)) << "\n";
    }
    t << "\n" << T ("skPlanCues") << "\n";
    const auto picks = proc.skPicks (sk);
    for (auto& c : sk.cues)
    {
        t << T ("skBarN").replace ("{n}", String (c.bar + 1)).paddedRight (' ', 10) << timeOf (c.bar).paddedRight (' ', 7) << T ("cu_" + c.type);
        for (auto& pk : picks)
            if (pk.first.bar == c.bar && pk.first.type == c.type)
            {
                if (pk.second != File()) t << "  ->  Samples/Bar " << String (c.bar + 1).paddedLeft ('0', 3) << " - " << c.type << " - " << pk.second.getFileName();
                else if (hg::cueAudio (c.type).render) t << "  ->  Audio/FX Guide.wav (" << T ("skHitgenSound") << ")";
                break;
            }
        t << "\n";
    }
    t << "\n" << T ("skPlanFlp") << "\n\n" << T ("skPlanHow") << "\n";
    return t;
}

void ProjectPage::exportProject (const File& dirIn)
{
    if (proc.exporting() || sk.secs.empty()) return;
    if (! proc.skel.active) { proc.skel.active = true; refreshSkeleton(); }
    File dir = dirIn;
    if (dir == File())
    {
        auto base = proc.projectsFolder();
        const auto name = File::createLegalFileName ((proc.userPreset.isNotEmpty() ? proc.userPreset : proc.baseName()) + " " + Time::getCurrentTime().formatted ("%Y-%m-%d %H-%M"));
        dir = base.getNonexistentChildFile (name, {}, false);
    }
    if (! proc.startExport (dir, planText())) return;
    exportInfo.setText (T ("skExporting").replace ("{p}", "0"), dontSendNotification);
    updateButtons();
}

void ProjectPage::tick()
{
    if (proc.presets.revision() != shownRev) { shownRev = proc.presets.revision(); mineList.updateContent(); mineList.repaint(); updateButtons(); }
    const bool libChanged = proc.pollLibrary();
    auto r = proc.result();
    const int bpm = (int) std::lround (proc.hostBpm.load() > 0 ? proc.hostBpm.load() : (double) proc.apvts.getRawParameterValue ("bpm")->load());
    if (libChanged || r.get() != shownRes || bpm != shownBpm || proc.libRevision != shownLib) refreshSkeleton();
    // kütüphane durumu
    {
        String s;
        if (proc.scanning()) s = T ("skLibScanning").replace ("{n}", String (proc.scanProgress.load()));
        else if (auto L = proc.library(); L != nullptr && ! proc.sampleFolders().isEmpty())
            s = T ("skLibInfo").replace ("{n}", String ((int) L->items.size())).replace ("{f}", String (proc.sampleFolders().size()));
        else s = T ("skLibEmpty");
        if (libInfo.getText() != s) { libInfo.setText (s, dontSendNotification); updateButtons(); }
    }
    hg::ExportResult er;
    if (proc.pollExport (er))
    {
        lastExport = er.dir;
        if (er.ok)
        {
            exportInfo.setText (T ("skExported").replace ("{f}", er.dir.getFullPathName()), dontSendNotification);
            say (T ("skExportedLong").replace ("{c}", String (er.copied)).replace ("{p}", String (er.placed)));
            if (revealAfterExport) er.dir.revealToUser();
        }
        else { exportInfo.setText (T ("skExportFail"), dontSendNotification); say (T ("skExportFail") + "\n" + er.error); }
        updateButtons();
    }
    else if (proc.exporting())
    {
        const auto s = T ("skExporting").replace ("{p}", String (proc.exportProgress.load()));
        if (exportInfo.getText() != s) exportInfo.setText (s, dontSendNotification);
    }
}

void ProjectPage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r;
    nameEd.setTitle (T ("pjName"));
    nameEd.setTextToShowWhenEmpty (T ("pjName"), lnf.pal().mute);
    help.setText (T ("pjHelp"), dontSendNotification);
    fullTile.label = T ("pjFull"); full0Tile.label = T ("pjFull0"); loopTile.label = T ("pjLoop");
    for (auto* t : { &fullTile, &full0Tile, &loopTile }) { t->setTitle (t->label); t->setTooltip (T ("pDragHint")); t->setHelpText (T ("pDragHint")); }
    startBtn.setButtonText (T ("pjStart")); saveBtn.setButtonText (T ("pjSave")); openBtn.setButtonText (T ("prLoad")); deleteBtn.setButtonText (T ("prDelete"));
    addSecBtn.setButtonText (T ("skAdd")); libBtn.setButtonText (T ("skLib")); drumsBtn.setButtonText (T ("skDrums")); exportBtn.setButtonText (T ("skExport"));
    genBtn.setTooltip (T ("skGenHint")); exportBtn.setTooltip (T ("skExportHint")); drumsBtn.setTooltip (T ("skDrumsHint")); libBtn.setTooltip (T ("skLibHint"));
    skHint.setText (T ("skHint"), dontSendNotification);
    tmplList.setTitle (T ("pjTemplates")); mineList.setTitle (T ("pjMine")); cueList.setTitle (T ("skList"));
    structBox.setTitle (T ("skStruct")); lenBox.setTitle (T ("skLen"));
    structBox.clear (dontSendNotification);
    for (int i = 0; i < HitGenProcessor::kNumSkStructs; ++i) structBox.addItem (T ("skS" + String (i)), i + 1);
    structBox.setTextWhenNothingSelected (T ("skCustomStruct"));
    lenBox.clear (dontSendNotification);
    for (int i = 0; i < 8; ++i)
    {
        const int s = HitGenProcessor::kSkDurations[i];
        lenBox.addItem (s == 0 ? T ("skLenStyle") : String (s / 60) + ":" + String (s % 60).paddedLeft ('0', 2), i + 1);
    }
    lenBox.setTextWhenNothingSelected (T ("skLenCustom"));
    if (skView) { skView->setTitle (T ("skTitle")); skView->setHelpText (T ("skHint")); skView->setDescription (T ("skHint")); }
    const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    help.setJustificationType (rtl ? Justification::topRight : Justification::topLeft); status.setJustificationType (help.getJustificationType());
    for (auto* lb : { &skHint, &libInfo, &exportInfo }) { lb->setJustificationType (j); lb->setColour (Label::textColourId, lnf.pal().mute); }
    help.setColour (Label::textColourId, lnf.pal().mute);
    nameEd.setJustification (j);
    nameEd.setColour (TextEditor::backgroundColourId, lnf.pal().panel2); nameEd.setColour (TextEditor::textColourId, lnf.pal().ink);
    nameEd.setColour (TextEditor::outlineColourId, lnf.pal().line); nameEd.setColour (TextEditor::focusedOutlineColourId, lnf.pal().focus);
    nameEd.applyFontToAllText (lnf.font (16)); nameEd.applyColourToAllText (lnf.pal().ink);
    for (auto* lb : { &tmplList, &mineList, &cueList }) lb->setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    refreshSkeleton();
    resized(); repaint();
}

void ProjectPage::resized()
{
    std::vector<std::pair<Component*, Rectangle<int>>> L {
        // sol: şablonlar ve projelerim
        { &help, { 20, 42, 410, 54 } },
        { &tmplList, { 20, 98, 410, 200 } }, { &startBtn, { 20, 304, 410, 34 } },
        { &nameEd, { 20, 378, 300, 34 } }, { &saveBtn, { 328, 378, 102, 34 } },
        { &mineList, { 20, 418, 410, 128 } },
        { &openBtn, { 20, 552, 128, 32 } }, { &deleteBtn, { 156, 552, 128, 32 } },
        { &status, { 20, 588, 410, 62 } },
        // sağ: proje iskeleti
        { &structBox, { 468, 44, 330, 32 } }, { &lenBox, { 806, 44, 118, 32 } }, { &genBtn, { 932, 44, 212, 32 } },
        { skView.get(), { 468, 84, 676, 98 } },
        { &addSecBtn, { 468, 188, 120, 28 } }, { &skHint, { 596, 188, 548, 28 } },
        { &cueList, { 468, 222, 676, 236 } },
        { &libBtn, { 468, 464, 200, 30 } }, { &libInfo, { 676, 464, 240, 30 } }, { &drumsBtn, { 924, 464, 220, 30 } },
        { &fullTile, { 468, 502, 216, 44 } }, { &full0Tile, { 692, 502, 216, 44 } }, { &loopTile, { 916, 502, 228, 44 } },
        { &exportBtn, { 468, 556, 250, 38 } }, { &exportInfo, { 726, 552, 418, 46 } } };
    for (auto& [c, r] : L) if (c != nullptr) c->setBounds (mir (r));
}

void ProjectPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    for (auto r : { Rectangle<int> (8, 0, 440, 654), Rectangle<int> (456, 0, 700, 654) }) { g.setColour (P.panel); g.fillRoundedRectangle (mir (r).toFloat(), 14); }
    const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.ink); g.setFont (lnf.font (17, true));
    g.drawText (T ("pjTemplates"), mir ({ 20, 10, 410, 30 }), j);
    g.drawText (T ("pjMine"), mir ({ 20, 344, 410, 30 }), j);
    g.drawText (T ("skTitle"), mir ({ 468, 10, 300, 30 }), j);
    // durum: profil, uzunluk, etkin mi
    g.setFont (lnf.font (12.5f)); g.setColour (proc.skel.active ? P.accent : P.mute);
    const String info = T ("skInfo").replace ("{b}", String (sk.song.bars)).replace ("{t}", timeOf (sk.song.bars)).replace ("{bpm}", String (sk.song.P.bpm))
                        + "  |  " + T ("skP_" + sk.profile) + (proc.skel.active ? String() : "  |  " + T ("skPreview"));
    g.drawFittedText (info, mir ({ 700, 10, 444, 30 }), rtl ? Justification::centredLeft : Justification::centredRight, 1, .7f);
    if (proc.presets.items().isEmpty())
    {
        g.setColour (P.mute); g.setFont (lnf.font (14));
        g.drawFittedText (T ("pjEmpty"), mir ({ 30, 426, 390, 80 }), rtl ? Justification::topRight : Justification::topLeft, 3);
    }
}
