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

ProjectPage::ProjectPage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data)
{
    setFocusContainerType (FocusContainerType::focusContainer);
    for (auto* lb : { &help, &status, &nameLabel }) { addAndMakeVisible (*lb); lb->setFont (lnf.font (13)); }
    help.setJustificationType (Justification::topLeft); status.setJustificationType (Justification::topLeft);
    addAndMakeVisible (nameEd); nameEd.setFont (lnf.font (16)); nameEd.setInputRestrictions (80); nameEd.setIndents (8, 6);
    nameEd.onReturnKey = [this] { save(); };
    for (auto* b : { &startBtn, &saveBtn, &openBtn, &deleteBtn }) addAndMakeVisible (*b);
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
    for (auto* lb : { &tmplList, &mineList }) { addAndMakeVisible (*lb); lb->setRowHeight (44); lb->setWantsKeyboardFocus (true); }
    tmplList.setModel (tmplModel.get()); mineList.setModel (mineModel.get());
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
    nameEd.setText (proc.userPreset, false);
    updateButtons();
}

ProjectPage::~ProjectPage() { tmplList.setModel (nullptr); mineList.setModel (nullptr); }

void ProjectPage::say (const String& s) { status.setText (s, dontSendNotification); status.setColour (Label::textColourId, lnf.pal().ink); }

void ProjectPage::updateButtons()
{
    const int row = mineList.getSelectedRow(); const bool has = row >= 0 && row < proc.presets.items().size();
    for (auto* b : { &openBtn, &deleteBtn }) b->setEnabled (has);
    startBtn.setEnabled (tmplList.getSelectedRow() >= 0);
}

void ProjectPage::startTemplate (int style)
{
    if (style < 0 || style >= data.numStyles()) return;
    if (auto* pr = proc.apvts.getParameter ("style")) pr->setValueNotifyingHost (pr->convertTo0to1 ((float) style));
    proc.applyStyleDefaults();   // stile uygun ses, tempo ve pump
    proc.newIdea();              // stile uygun ton, makam, enerji, özgünlük, swing + yeni tohum
    proc.userArtist = {}; proc.userPreset = {}; nameEd.setText ({}, false);   // yeni, henüz adsız proje
    say (T ("pjStarted").replace ("{s}", data.style (style)["artist"].toString()));
    if (onLoaded) onLoaded();
}

void ProjectPage::save()
{
    const auto n = nameEd.getText().trim();
    if (n.isEmpty()) { say (T ("prNeed")); nameEd.grabKeyboardFocus(); return; }
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
}

void ProjectPage::tick()
{
    if (proc.presets.revision() != shownRev) { shownRev = proc.presets.revision(); mineList.updateContent(); mineList.repaint(); updateButtons(); }
}

void ProjectPage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r;
    nameLabel.setText (T ("pjName"), dontSendNotification); nameEd.setTitle (T ("pjName"));
    nameEd.setTextToShowWhenEmpty (T ("prDefault"), lnf.pal().mute);
    help.setText (T ("pjHelp"), dontSendNotification);
    fullTile.label = T ("pjFull"); full0Tile.label = T ("pjFull0"); loopTile.label = T ("pjLoop");
    for (auto* t : { &fullTile, &full0Tile, &loopTile }) { t->setTitle (t->label); t->setTooltip (T ("pDragHint")); t->setHelpText (T ("pDragHint")); }
    startBtn.setButtonText (T ("pjStart")); saveBtn.setButtonText (T ("pjSave")); openBtn.setButtonText (T ("prLoad")); deleteBtn.setButtonText (T ("prDelete"));
    tmplList.setTitle (T ("pjTemplates")); mineList.setTitle (T ("pjMine"));
    const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    nameLabel.setJustificationType (j); nameLabel.setColour (Label::textColourId, lnf.pal().ink);
    help.setJustificationType (rtl ? Justification::topRight : Justification::topLeft); status.setJustificationType (help.getJustificationType());
    help.setColour (Label::textColourId, lnf.pal().mute);
    nameEd.setJustification (j);
    nameEd.setColour (TextEditor::backgroundColourId, lnf.pal().panel2); nameEd.setColour (TextEditor::textColourId, lnf.pal().ink);
    nameEd.setColour (TextEditor::outlineColourId, lnf.pal().line); nameEd.setColour (TextEditor::focusedOutlineColourId, lnf.pal().focus);
    nameEd.applyFontToAllText (lnf.font (16)); nameEd.applyColourToAllText (lnf.pal().ink);
    for (auto* lb : { &tmplList, &mineList }) lb->setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    resized(); repaint();
}

void ProjectPage::resized()
{
    std::vector<std::pair<Component*, Rectangle<int>>> L {
        { &help, { 20, 48, 410, 110 } },
        { &tmplList, { 20, 166, 410, 362 } }, { &startBtn, { 20, 548, 410, 38 } },
        { &nameLabel, { 468, 48, 400, 20 } }, { &nameEd, { 468, 70, 540, 36 } }, { &saveBtn, { 1018, 70, 126, 36 } },
        { &mineList, { 468, 122, 676, 250 } },
        { &fullTile, { 468, 480, 216, 44 } }, { &full0Tile, { 692, 480, 216, 44 } }, { &loopTile, { 916, 480, 228, 44 } }, { &openBtn, { 468, 548, 128, 36 } }, { &deleteBtn, { 604, 548, 128, 36 } },
        { &status, { 740, 540, 404, 60 } } };
    for (auto& [c, r] : L) c->setBounds (mir (r));
}

void ProjectPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    for (auto r : { Rectangle<int> (8, 0, 440, 654), Rectangle<int> (456, 0, 700, 654) }) { g.setColour (P.panel); g.fillRoundedRectangle (mir (r).toFloat(), 14); }
    const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.ink); g.setFont (lnf.font (17, true));
    g.drawText (T ("pjTemplates"), mir ({ 20, 12, 410, 30 }), j);
    g.drawText (T ("pjMine"), mir ({ 468, 12, 676, 30 }), j);
    g.setColour (P.ink); g.setFont (lnf.font (15, true));
    g.drawText (T ("pjSkeleton"), mir ({ 468, 380, 676, 24 }), j);
    paintSkeleton (g, mir ({ 468, 408, 676, 60 }), jmax (0, tmplList.getSelectedRow()));
    if (proc.presets.items().isEmpty())
    {
        g.setColour (P.mute); g.setFont (lnf.font (14));
        g.drawFittedText (T ("pjEmpty"), mir ({ 488, 140, 636, 80 }), rtl ? Justification::topRight : Justification::topLeft, 3);
    }
}

// Seçili stilin parça iskeleti: bölümler uzunluklarıyla orantılı kutular (Intro, Build, Drop, Breakdown, Outro...).
void ProjectPage::paintSkeleton (Graphics& g, Rectangle<int> area, int style) const
{
    if (style < 0 || style >= data.numStyles()) return;
    const auto* arr = data.style (style)["arr"].getArray(); if (arr == nullptr || arr->isEmpty()) return;
    int total = 0; for (auto& sec : *arr) total += (int) sec[1];
    if (total <= 0) return;
    const auto& P = lnf.pal(); const float gap = 3.0f; float x = (float) area.getX(); const float W = (float) area.getWidth() - gap * (float) (arr->size() - 1);
    for (int i = 0; i < arr->size(); ++i)
    {
        const auto name = (*arr)[i][0].toString(); const int B = (int) (*arr)[i][1];
        const float w = W * (float) B / (float) total;
        Rectangle<float> r (rtl ? (float) area.getRight() - (x - (float) area.getX()) - w : x, (float) area.getY(), w, (float) area.getHeight());
        const bool drop = name.equalsIgnoreCase ("Drop"), build = name.equalsIgnoreCase ("Build"), brk = name.equalsIgnoreCase ("Breakdown");
        g.setColour (drop ? P.accent : build ? P.accent.withAlpha (.45f) : brk ? P.panel2 : P.panel2.brighter (.08f)); g.fillRoundedRectangle (r, 6);
        g.setColour (brk ? P.accent.withAlpha (.5f) : P.line); g.drawRoundedRectangle (r, 6, 1.0f);
        g.setColour (drop ? P.panel : P.ink); g.setFont (lnf.font (w > 60 ? 12.5f : 10.5f, true));
        g.drawFittedText (name, r.reduced (3, 6).withHeight (22).toNearestInt(), Justification::centred, 1, .7f);
        g.setFont (lnf.font (11)); g.setColour (drop ? P.panel : P.mute);
        g.drawText (T ("pjBars").replace ("{n}", String (B)), r.reduced (3, 4).withTrimmedTop (28).toNearestInt(), Justification::centred);
        x += w + gap;
    }
}
