#include "PresetPage.h"

using namespace juce;

PresetPage::PresetPage (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data)
{
    setFocusContainerType (FocusContainerType::focusContainer);
    for (auto* lb : { &artistLabel, &nameLabel, &help, &status, &current }) { addAndMakeVisible (*lb); lb->setFont (lnf.font (13)); }
    help.setJustificationType (Justification::topLeft); status.setJustificationType (Justification::topLeft);
    help.setFont (lnf.font (13)); current.setFont (lnf.font (14, true));
    for (auto* ed : { &artistEd, &nameEd })
    {
        addAndMakeVisible (*ed); ed->setFont (lnf.font (16)); ed->setInputRestrictions (80); ed->setIndents (8, 6);
        ed->onReturnKey = [this] { save(); };
    }
    for (auto* b : { &saveBtn, &loadBtn, &deleteBtn, &exportBtn, &importBtn, &folderBtn }) addAndMakeVisible (*b);
    saveBtn.onClick = [this] { save(); };
    loadBtn.onClick = [this] { loadRow (list.getSelectedRow()); };
    deleteBtn.onClick = [this]
    {
        const int row = list.getSelectedRow(); const auto& it = proc.presets.items();
        if (row < 0 || row >= it.size()) return;
        if (pendingDelete != row) { pendingDelete = row; say (T ("prConfirm")); return; }
        const auto name = it[row].display();
        proc.presets.remove (it[row].file); pendingDelete = -1;
        list.updateContent(); say (T ("prDeleted").replace ("{n}", name)); updateButtons();
    };
    exportBtn.onClick = [this]
    {
        const int row = list.getSelectedRow(); const auto& it = proc.presets.items();
        if (row < 0 || row >= it.size()) return;
        const auto src = it[row].file;
        chooser = std::make_unique<FileChooser> (T ("prExport"), File::getSpecialLocation (File::userDesktopDirectory).getChildFile (src.getFileName()), String ("*") + hg::PresetStore::kExt);
        chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting, [this, src] (const FileChooser& fc)
        {
            auto f = fc.getResult(); if (f == File()) return;
            if (! f.hasFileExtension (hg::PresetStore::kExt)) f = f.withFileExtension (hg::PresetStore::kExt);
            if (src.copyFileTo (f)) say (T ("prExported").replace ("{f}", f.getFullPathName()));
        });
    };
    importBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("prImport"), File::getSpecialLocation (File::userDesktopDirectory), String ("*") + hg::PresetStore::kExt);
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
        {
            const auto f = fc.getResult(); if (f == File()) return;
            hg::PresetInfo info;
            if (! proc.presets.importFile (f, info)) { say (T ("prBad")); return; }
            list.updateContent();
            const int k = proc.presets.indexOf (info.artist, info.name);
            if (k >= 0) list.selectRow (k);
            say (T ("prImported").replace ("{n}", info.display()));
        });
    };
    folderBtn.onClick = [this] { proc.presets.dir().revealToUser(); };
    addAndMakeVisible (list); list.setModel (this); list.setRowHeight (52); list.setWantsKeyboardFocus (true);
    artistEd.setText (proc.userArtist, false); nameEd.setText (proc.userPreset, false);
    updateButtons();
}

PresetPage::~PresetPage() { list.setModel (nullptr); }

void PresetPage::say (const String& s) { status.setText (s, dontSendNotification); status.setColour (Label::textColourId, lnf.pal().ink); }

void PresetPage::updateButtons()
{
    const int row = list.getSelectedRow(); const bool has = row >= 0 && row < proc.presets.items().size();
    for (auto* b : { &loadBtn, &deleteBtn, &exportBtn }) b->setEnabled (has);
}

void PresetPage::save()
{
    const auto a = artistEd.getText().trim(), n = nameEd.getText().trim();
    if (a.isEmpty() || n.isEmpty()) { say (T ("prNeed")); (a.isEmpty() ? artistEd : nameEd).grabKeyboardFocus(); return; }
    String path;
    if (! proc.savePreset (a, n, path)) { say (T ("saveFail")); return; }
    list.updateContent();
    const int k = proc.presets.indexOf (a, n); if (k >= 0) list.selectRow (k);
    say (T ("prSaved").replace ("{n}", a + " - " + n));
    if (onLoaded) onLoaded();
}

void PresetPage::loadRow (int row)
{
    const auto& it = proc.presets.items();
    if (row < 0 || row >= it.size()) return;
    const auto info = it[row];
    if (! proc.loadPreset (info.file)) { say (T ("prBad")); return; }
    artistEd.setText (info.artist, false); nameEd.setText (info.name, false);
    say (T ("prLoaded").replace ("{n}", info.display()));
    if (onLoaded) onLoaded();
}

void PresetPage::tick()
{
    if (proc.presets.revision() != shownRev) { shownRev = proc.presets.revision(); list.updateContent(); list.repaint(); updateButtons(); }
    const String cur = proc.userArtist.isNotEmpty() ? proc.userArtist + " - " + proc.userPreset : String();
    if (current.getText() != cur) current.setText (cur, dontSendNotification);
}

void PresetPage::setLanguage (const String& l, bool r)
{
    lang = l; rtl = r;
    artistLabel.setText (T ("prArtist"), dontSendNotification); nameLabel.setText (T ("prName"), dontSendNotification);
    artistEd.setTitle (T ("prArtist")); nameEd.setTitle (T ("prName"));
    artistEd.setTextToShowWhenEmpty ("Wavejackers", lnf.pal().mute); nameEd.setTextToShowWhenEmpty (T ("prDefault"), lnf.pal().mute);
    help.setText (T ("prHelp"), dontSendNotification);
    saveBtn.setButtonText (T ("prSave")); loadBtn.setButtonText (T ("prLoad")); deleteBtn.setButtonText (T ("prDelete"));
    exportBtn.setButtonText (T ("prExport")); importBtn.setButtonText (T ("prImport")); folderBtn.setButtonText (T ("prFolder"));
    list.setTitle (T ("prList"));
    const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    for (auto* lb : { &artistLabel, &nameLabel, &current }) { lb->setJustificationType (j); lb->setColour (Label::textColourId, lnf.pal().ink); }
    help.setJustificationType (rtl ? Justification::topRight : Justification::topLeft); status.setJustificationType (help.getJustificationType());
    help.setColour (Label::textColourId, lnf.pal().mute);
    for (auto* ed : { &artistEd, &nameEd })
    {
        ed->setJustification (j);
        ed->setColour (TextEditor::backgroundColourId, lnf.pal().panel2); ed->setColour (TextEditor::textColourId, lnf.pal().ink);
        ed->setColour (TextEditor::outlineColourId, lnf.pal().line); ed->setColour (TextEditor::focusedOutlineColourId, lnf.pal().focus);
        ed->applyFontToAllText (lnf.font (16)); ed->applyColourToAllText (lnf.pal().ink);
    }
    list.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    resized(); repaint();
}

void PresetPage::resized()
{
    std::vector<std::pair<Component*, Rectangle<int>>> L {
        { &current, { 20, 48, 400, 22 } },
        { &artistLabel, { 20, 84, 400, 20 } }, { &artistEd, { 20, 106, 410, 36 } },
        { &nameLabel, { 20, 152, 400, 20 } }, { &nameEd, { 20, 174, 410, 36 } },
        { &saveBtn, { 20, 224, 220, 38 } },
        { &help, { 20, 276, 410, 150 } }, { &status, { 20, 440, 410, 90 } },
        { &list, { 468, 52, 676, 476 } },
        { &loadBtn, { 468, 548, 128, 36 } }, { &deleteBtn, { 604, 548, 128, 36 } }, { &exportBtn, { 740, 548, 128, 36 } },
        { &importBtn, { 876, 548, 128, 36 } }, { &folderBtn, { 1012, 548, 132, 36 } } };
    for (auto& [c, r] : L) c->setBounds (mir (r));
}

void PresetPage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    for (auto r : { Rectangle<int> (8, 0, 440, 654), Rectangle<int> (456, 0, 700, 654) }) { g.setColour (P.panel); g.fillRoundedRectangle (mir (r).toFloat(), 14); }
    const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.ink); g.setFont (lnf.font (17, true));
    g.drawText (T ("prTitle"), mir ({ 20, 12, 410, 30 }), j);
    g.drawText (T ("prList"), mir ({ 468, 12, 676, 30 }), j);
    if (proc.presets.items().isEmpty())
    {
        g.setColour (P.mute); g.setFont (lnf.font (14));
        g.drawFittedText (T ("prEmpty"), mir ({ 488, 80, 636, 80 }), rtl ? Justification::topRight : Justification::topLeft, 3);
    }
}

int PresetPage::getNumRows() { return proc.presets.items().size(); }

void PresetPage::paintListBoxItem (int row, Graphics& g, int w, int h, bool selected)
{
    const auto& it = proc.presets.items(); if (row < 0 || row >= it.size()) return;
    const auto& P = lnf.pal(); const auto& i = it[row];
    auto r = Rectangle<int> (0, 0, w, h).reduced (2).toFloat();
    g.setColour (selected ? P.accent.withAlpha (.16f) : P.panel2); g.fillRoundedRectangle (r, 8);
    if (selected) { g.setColour (P.accent); g.drawRoundedRectangle (r, 8, 1.2f); }
    const bool isCur = i.artist == proc.userArtist && i.name == proc.userPreset;
    g.setColour (P.accent); g.fillRect (rtl ? r.getRight() - 4 : r.getX(), r.getY() + 6, 4.0f, r.getHeight() - 12);
    auto tr = r.reduced (14, 4).toNearestInt(); const auto j = rtl ? Justification::centredRight : Justification::centredLeft;
    g.setColour (P.ink); g.setFont (lnf.font (15, true)); g.drawText (i.artist + "  -  " + i.name, tr.removeFromTop (24), j);
    if (isCur) { g.setColour (P.accent); const float cx = rtl ? r.getX() + 14 : r.getRight() - 14; g.fillEllipse (cx - 5, r.getCentreY() - 5, 10, 10); }
    const auto si = data.styleIndex (i.style);
    String sub = si >= 0 ? T ("prBase").replace ("{s}", data.style (si)["artist"].toString()) : String();
    if (i.created.toMilliseconds() > 0) sub << "  ·  " << i.created.formatted ("%Y-%m-%d %H:%M");
    g.setColour (P.mute); g.setFont (lnf.font (12)); g.drawText (sub, tr, j);
}

String PresetPage::getNameForRow (int row)
{
    const auto& it = proc.presets.items(); if (row < 0 || row >= it.size()) return {};
    const auto si = data.styleIndex (it[row].style);
    return it[row].display() + (si >= 0 ? ", " + T ("prBase").replace ("{s}", data.style (si)["artist"].toString()) : String());
}
