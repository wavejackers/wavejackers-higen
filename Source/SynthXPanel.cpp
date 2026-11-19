#include "SynthXPanel.h"
#include "SynthX.h"

using namespace juce;

namespace
{
var* resolve (var& root, const String& path)
{
    var* cur = &root;
    for (auto& k : StringArray::fromTokens (path, ".", ""))
    {
        if (auto* a = cur->getArray()) { const int i = k.getIntValue(); if (i < 0 || i >= a->size()) return nullptr; cur = &a->getReference (i); }
        else if (auto* o = cur->getDynamicObject()) { cur = o->getProperties().getVarPointer (Identifier (k)); if (cur == nullptr) return nullptr; }
        else return nullptr;
    }
    return cur;
}
const char* const kTabKeys[12] = { "sxOsc1", "sxOsc2", "sxOsc3", "sxLayer", "sxEnv", "sxLfo", "sxStep", "sxMatrix", "sxFx", "sxPlay", "sxMacro", "sxBrowse" };
const char* const kTabEn[12] = { "Osc 1", "Osc 2", "Osc 3", "Layer FX", "Envelopes", "LFO", "Step", "Matrix", "FX", "Play", "Macro", "Browse" };
const char* const kDiv[12] = { "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/4 T", "1/8", "1/8 T", "1/16", "1/16 T", "1/32" };
struct Tpl { const char* key; const char* en; std::vector<std::pair<const char*, var>> set; };
const char* const kTplCat[11] = { "Other", "Lead", "Pad", "Keys", "Bass", "Pad", "Bass", "Pluck", "Lead", "Bass", "FX" };
const char* const kCats[9] = { "All", "Bass", "Lead", "Pad", "Keys", "Pluck", "FX", "Other", "Mine" };
const char* const kCatKeys[9] = { "gAll", "sxCatBass", "sxCatLead", "sxCatPad", "sxCatKeys", "sxCatPluck", "sxCatFx", "sxCatOther", "sxMine" };
const uint32 kSwatch[9] = { 0, 0xffd9b14a, 0xff4fb3a8, 0xff6c8cff, 0xffe0607e, 0xff9b6cf0, 0xff5fc46b, 0xffff8a3d, 0xffeaeaea };
var arr (std::initializer_list<var> l) { Array<var> a; for (auto& x : l) a.add (x); return a; }
const std::vector<Tpl>& templates()
{
    static const std::vector<Tpl> t = {
        { "sxTplInit", "Init", {} },
        { "sxTplSaw", "Supersaw lead", { { "o.0.t", 3 }, { "o.0.n", 4 }, { "o.0.d", .45 }, { "o.0.mx", .6 }, { "o.0.f.t", 1 }, { "o.0.f.c", 7000 }, { "o.0.f.r", .2 }, { "o.0.f.e", .25 },
                                          { "env.0", arr ({ .003, .3, .85, .35, 0 }) }, { "env.1", arr ({ .001, .5, .2, .3, 0 }) }, { "lfo.0.r", 5.5 }, { "lfo.0.fa", .6 }, { "lfo.0.d.0", arr ({ 0, .006 }) }, { "gl", .04 } } },
        { "sxTplPwm", "PWM pad", { { "o.0.a", 0 }, { "o.0.pm", 2 }, { "o.0.pw", .3 }, { "o.0.u", 4 }, { "o.0.d", .3 }, { "o.1.t", 1 }, { "o.1.a", 0 }, { "o.1.pm", 2 }, { "o.1.pw", .6 }, { "o.1.oc", -1 }, { "o.1.u", 2 }, { "o.1.lv", .5 },
                                    { "o.0.f.t", 2 }, { "o.0.f.c", 2500 }, { "o.1.f.t", 2 }, { "o.1.f.c", 2500 }, { "env.0", arr ({ .8, 1, .8, 1.6, 0 }) }, { "lfo.0.r", .35 }, { "lfo.0.d.0", arr ({ 6, .35 }) }, { "lfo.0.d.1", arr ({ 22, -.3 }) } } },
        { "sxTplBell", "FM bell", { { "o.0.a", 3 }, { "o.0.sm", 1 }, { "o.0.fm", .1 }, { "o.0.fr", 3.5 }, { "o.0.f.t", 0 }, { "env.0", arr ({ .002, 2.5, 0, 2, 0 }) }, { "env.1", arr ({ .001, 1.2, 0, 1, 0 }) }, { "ed.0.0", arr ({ 9, .6 }) } } },
        { "sxTplPd", "PD bass", { { "o.0.a", 0 }, { "o.0.sm", 2 }, { "o.0.sh", .7 }, { "o.0.oc", -1 }, { "o.0.f.t", 3 }, { "o.0.f.c", 900 }, { "o.0.f.r", .35 }, { "o.0.f.e", .4 },
                                  { "env.0", arr ({ .002, .4, .7, .15, 0 }) }, { "env.1", arr ({ .001, .25, 0, .2, 0 }) }, { "ed.0.0", arr ({ 5, .3 }) }, { "gl", .05 } } },
        { "sxTplVowel", "Vowel pad", { { "o.0.t", 2 }, { "o.0.a", 120 }, { "o.0.b", 127 }, { "o.0.mm", 1 }, { "o.0.m", .5 }, { "o.0.u", 3 }, { "o.0.d", .25 }, { "o.0.f.c", 9000 },
                                       { "lfo.0.w", 1 }, { "lfo.0.r", .12 }, { "lfo.0.d.0", arr ({ 4, .5 }) }, { "env.0", arr ({ .6, 1, .8, 1.5, 0 }) } } },
        { "sxTplWob", "Wobble bass", { { "o.0.u", 2 }, { "o.0.d", .15 }, { "o.1.t", 1 }, { "o.1.a", 1 }, { "o.1.oc", -1 }, { "o.0.f.t", 2 }, { "o.0.f.c", 400 }, { "o.0.f.r", .5 }, { "o.1.f.t", 2 }, { "o.1.f.c", 400 }, { "o.1.f.r", .5 },
                                       { "lfo.0.sy", 1 }, { "lfo.0.r", 7 }, { "lfo.0.d.0", arr ({ 11, .6 }) }, { "lfo.0.d.1", arr ({ 27, .6 }) } } },
        { "sxTplPluck", "Pluck", { { "o.0.t", 2 }, { "o.0.a", 64 }, { "o.0.f.t", 2 }, { "o.0.f.c", 600 }, { "o.0.f.r", .3 }, { "o.0.f.e", .8 }, { "env.0", arr ({ .001, .45, 0, .3, 0 }) }, { "env.1", arr ({ .001, .18, 0, .2, 0 }) } } },
        { "sxTplHoover", "Hoover", { { "o.0.t", 3 }, { "o.0.n", 3 }, { "o.0.d", .7 }, { "o.0.mw", arr ({ 0, 1, 0, 2 }) }, { "env.2", arr ({ .001, .25, 0, .2, 0 }) }, { "ed.1.0", arr ({ 52, -.5 }) }, { "gl", .1 } } },
        { "sxTplSub", "Sub bass", { { "o.0.a", 3 }, { "o.0.f.t", 0 }, { "o.1.t", 1 }, { "o.1.a", 2 }, { "o.1.oc", 1 }, { "o.1.lv", .25 }, { "o.1.f.t", 0 }, { "env.0", arr ({ .002, .2, 1, .1, 0 }) } } },
        { "sxTplRiser", "Noise riser", { { "o.0.t", 0 }, { "lx.on", 1 }, { "lx.w", -1 }, { "lx.t", 2 }, { "lx.c", 1200 }, { "lx.r", .35 }, { "lx.lv", 1 }, { "env.0", arr ({ 2, .5, 1, 1, 0 }) },
                                         { "lfo.0.w", 2 }, { "lfo.0.r", .25 }, { "lfo.0.d.0", arr ({ 50, .8 }) } } },
    };
    return t;
}
}

int SynthXPanel::numTemplates() { return (int) templates().size(); }

SynthXPanel::SynthXPanel (HitGenProcessor& p, hgui::LookAndFeel& l) : proc (p), lnf (l), data (p.data)
{
    for (auto* c : std::initializer_list<Component*> { &convertBtn, &initBtn, &sfBtn, &xgenBtn, &templBox, &sfLbl }) addAndMakeVisible (*c);
    for (int i = 0; i < 12; ++i)
    {
        auto& b = tabs[(size_t) i]; addAndMakeVisible (b); b.setClickingTogglesState (true); b.setRadioGroupId (771); b.setComponentID ("nav");
        b.onClick = [this, i] { if (tabs[(size_t) i].getToggleState() && section != i) showTab (i); };
    }
    addChildComponent (steps); addChildComponent (scope);
    setWantsKeyboardFocus (true);
    for (auto* c : std::initializer_list<Component*> { &xy, &list, &search, &auditionBtn, &deleteBtn, &linkBtn, &saveCat, &kbMode, &stats, &hint }) addChildComponent (*c);
    list.setModel (this); list.setRowHeight (26); list.setColour (ListBox::backgroundColourId, lnf.pal().panel2);
    search.setTextToShowWhenEmpty ("Search", lnf.pal().mute); search.onTextChange = [this] { searchText = search.getText(); refreshBrowser(); };
    for (int i = 0; i < 9; ++i)
    {
        auto& b = catBtns[(size_t) i]; addChildComponent (b); b.setClickingTogglesState (true); b.setRadioGroupId (772); b.setComponentID ("nav");
        b.onClick = [this, i] { if (catBtns[(size_t) i].getToggleState()) { catFilter = kCats[i]; refreshBrowser(); } };
        auto& w = swatches[(size_t) i]; addChildComponent (w);
        w.col = i == 0 ? Colour() : Colour (kSwatch[i]); w.o = this; w.onClick = [this, i] { setAccent (i == 0 ? Colour() : Colour (kSwatch[i])); repaint(); };
    }
    catBtns[0].setToggleState (true, dontSendNotification);
    auditionBtn.setClickingTogglesState (true);
    auditionBtn.onClick = [this] { if (proc.demoPlaying()) proc.stopDemo(); else proc.playDemo (part); auditionBtn.setToggleState (! auditionBtn.getToggleState(), dontSendNotification); };
    deleteBtn.onClick = [this] { const int r = list.getSelectedRow(); if (r >= 0 && r < (int) shown.size() && shown[(size_t) r].file.existsAsFile()) { shown[(size_t) r].file.moveToTrash(); fillTemplates(); refreshBrowser(); } };
    linkBtn.onClick = [this]
    {   // Morph X/Y'yi hızlıca bağla: X -> Osc 1 geçiş ve tüm kesimler, Y -> Osc 1 biçim ve rezonans
        if (! active()) prog = proc.xsFromClassic (part);
        auto set = [this] (const String& path, var v) { if (auto* x = resolve (prog, path)) *x = v; };
        set ("mtx.0.s", (int) xs::sMorphX); set ("mtx.0.d.0", Array<var> { 4, .5 }); set ("mtx.0.d.1", Array<var> { 11, .25 }); set ("mtx.0.d.2", Array<var> { 27, .25 }); set ("mtx.0.d.3", Array<var> { 43, .25 });
        set ("mtx.1.s", (int) xs::sMorphY); set ("mtx.1.d.0", Array<var> { 5, .5 }); set ("mtx.1.d.1", Array<var> { 12, .3 });
        commit(); syncAll();
    };
    saveCat.addItemList ({ "Bass", "Lead", "Pad", "Keys", "Pluck", "FX", "Other" }, 1); saveCat.setSelectedId (1, dontSendNotification);
    kbAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "kb_mode", kbMode);
    convertBtn.onClick = [this] { proc.setXsProgram (part, var()); refresh(); };   // seçili sesten yeniden kur
    xgenBtn.onClick = [this] { xgen ((uint32_t) Random::getSystemRandom().nextInt()); };
    addChildComponent (seqBtn); addChildComponent (seqClear); seqClear.setButtonText ("x");
    seqBtn.setTooltip (T ("sxSeqTip", "A MIDI file plays as a pattern: hold a key to play it transposed (C4 = original)"));
    seqBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("sxSeqLoad", "Load MIDI sequence"), File(), "*.mid;*.midi");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { if (fc.getResult().existsAsFile()) loadSequence (fc.getResult()); });
    };
    seqClear.onClick = [this] { if (active() && prog.getDynamicObject() != nullptr) { prog.getDynamicObject()->removeProperty ("seq"); commit(); rebuild(); } };
    initBtn.onClick = [this] { applyTemplate (0); };
    templBox.onChange = [this]
    {
        const int id = templBox.getSelectedId(); templBox.setSelectedId (0, dontSendNotification);
        if (id >= 1000) { if (id - 1000 < userProgs.size()) loadProgram (userProgs[id - 1000]); } else if (id >= 1) applyTemplate (id - 1);
    };
    addAndMakeVisible (saveBtn);
    saveBtn.onClick = [this]
    {
        auto* w = new AlertWindow (T ("sxSave", "Save"), T ("sxSaveName", "Name"), MessageBoxIconType::NoIcon, this);
        w->addTextEditor ("n", data.soundName (proc.soundId (part == 2 ? hg::kMelody : part), lang)); w->addButton ("OK", 1, KeyPress (KeyPress::returnKey)); w->addButton (T ("cancel", "Cancel"), 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([this, w] (int r) { if (r == 1) { const auto f = saveProgram (w->getTextEditorContents ("n"), kCats[jlimit (1, 7, saveCat.getSelectedId())]); refreshBrowser(); sfLbl.setText (f.existsAsFile() ? f.getFileNameWithoutExtension() : String(), dontSendNotification); } }), true);
    };
    sfBtn.onClick = [this]
    {
        chooser = std::make_unique<FileChooser> (T ("sxLoadSf", "Load SoundFont"), File(), "*.sf2");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc)
        {
            const auto f = fc.getResult(); if (! f.existsAsFile()) return;
            String err; auto sf = xs::loadSoundFont (f, err);
            if (sf == nullptr) { sfLbl.setText (T ("sxSfFail", "This SoundFont could not be read"), dontSendNotification); return; }
            if (! active()) prog = xs::Program::toVar (xs::Program::init());
            prog.getDynamicObject()->setProperty ("sf", f.getFullPathName());
            const int k = jlimit (0, 2, section); setValue ("o." + String (k) + ".t", 2); setValue ("o." + String (k) + ".a", -1);
            if (section > 2) showTab (0); else rebuild();
        });
    };
    sfLbl.setJustificationType (Justification::centredLeft); sfLbl.setColour (Label::textColourId, lnf.pal().mute);
    showTab (0);
}

String SynthXPanel::T (const String& key, const String& en) const { const auto s = data.tr (lang, key); return s == key ? en : s; }
String SynthXPanel::N (const String& kind, int i, const String& en) const { return T ("sx" + kind + String (i), en); }
String SynthXPanel::destLabel (int d) const
{
    if (d < 48) { static const char* const en[16] = { "Pitch", "Fine", "Level", "Pan", "Morph", "Shape", "PW", "Detune", "Spread", "FM", "FM filter", "Cutoff", "Resonance", "Drive", "Layer level", "Phase" };
                  return T ("sxOscN", "Osc") + " " + String (d / 16 + 1) + " " + N ("Dst", d % 16, en[d % 16]); }
    return N ("Dg", d - 48, xs::destName (d));
}

void SynthXPanel::setLanguage (const String& l)
{
    lang = l;
    convertBtn.setButtonText (T ("sxConvert", "From sound")); convertBtn.setTooltip (T ("sxConvertTip", "Rebuild this program from the selected sound (your edits are replaced)"));
    xgenBtn.setButtonText ("X-Gen"); xgenBtn.setTooltip (T ("sxXgenTip", "Generate a new random sound that suits this part"));
    initBtn.setButtonText (T ("sxInit", "Init")); sfBtn.setButtonText (T ("sxLoadSf", "Load SoundFont"));
    fillTemplates();
    saveBtn.setButtonText (T ("sxSave", "Save")); saveBtn.setTooltip (T ("sxSaveTip", "Save this program; it appears in the list next to the templates"));
    for (int i = 0; i < 12; ++i) tabs[(size_t) i].setButtonText (T (kTabKeys[i], kTabEn[i]));
    for (int i = 0; i < 9; ++i) { catBtns[(size_t) i].setButtonText (T (kCatKeys[i], kCats[i])); swatches[(size_t) i].setButtonText (i == 0 ? T ("sxStyleCol", "Style") : String()); }
    auditionBtn.setButtonText (T ("demoPlay", "Audition")); deleteBtn.setButtonText (T ("sxDelete", "Delete")); linkBtn.setButtonText (T ("sxLinkMorph", "Link Morph X/Y"));
    linkBtn.setTooltip (T ("sxLinkTip", "Morph X: Osc 1 morph and all cutoffs. Morph Y: Osc 1 shape and resonance (Matrix tab)"));
    search.setTextToShowWhenEmpty (T ("sxSearch", "Search"), lnf.pal().mute);
    { const int sel = kbMode.getSelectedId(); kbMode.clear (dontSendNotification); kbMode.addItemList ({ T ("sxKbSingle", "Single part"), T ("sxKbLayer", "Layer all parts"), T ("sxKbSplit", "Split: bass below") }, 1); kbMode.setSelectedId (jmax (1, sel), dontSendNotification); kbAtt = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "kb_mode", kbMode); }
    saveCat.clear (dontSendNotification); { int k = 1; for (auto* c : { "sxCatBass", "sxCatLead", "sxCatPad", "sxCatKeys", "sxCatPluck", "sxCatFx", "sxCatOther" }) saveCat.addItem (T (c, kCats[k]), k), ++k; } saveCat.setSelectedId (1, dontSendNotification);
    rebuild();
}

void SynthXPanel::setPart (int p) { part = jlimit (0, 2, p); refresh(); }

void SynthXPanel::refresh()
{
    shownVer = proc.xsVersion;
    const auto v = proc.xsProgram (part);
    prog = v.isObject() ? JSON::parse (JSON::toString (v)) : var();
    const auto sf = active() ? prog["sf"].toString() : String();
    sfLbl.setText (sf.isNotEmpty() ? File (sf).getFileName() : String(), dontSendNotification);
    syncAll(); repaint();
}

void SynthXPanel::commit()
{
    if (building || ! active()) return;
    proc.setXsProgram (part, JSON::parse (JSON::toString (prog))); shownVer = proc.xsVersion;
    scope.repaint(); steps.repaint();
}

var SynthXPanel::value (const String& path) const { auto copy = prog; auto* v = resolve (copy, path); return v != nullptr ? *v : var(); }

void SynthXPanel::setValue (const String& path, const var& v)
{
    if (! active()) { prog = proc.xsFromClassic (part); }
    if (auto* x = resolve (prog, path)) { *x = v; commit(); }
}

void SynthXPanel::applyTemplate (int i)
{
    i = jlimit (0, numTemplates() - 1, i);
    const auto keepSf = active() ? prog["sf"].toString() : String();
    prog = xs::Program::toVar (xs::Program::init());
    if (keepSf.isNotEmpty()) prog.getDynamicObject()->setProperty ("sf", keepSf);
    for (auto& [path, v] : templates()[(size_t) i].set) if (auto* x = resolve (prog, path)) *x = v;
    prog.getDynamicObject()->setProperty ("vol", xs::calibratedVolume (prog, part, proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0));   // şablonlar da partinin seviyesinde
    commit(); syncAll();
}

void SynthXPanel::showTab (int t)
{
    section = jlimit (0, 11, t); tabs[(size_t) section].setToggleState (true, dontSendNotification);
    rebuild();
}

// ---- denetim kurucuları
SynthXPanel::Ctl& SynthXPanel::knob (const String& path, const String& label, double lo, double hi, Rectangle<int> r, double skewMid, bool integer, std::function<String (double)> text)
{
    auto c = std::make_unique<Ctl>(); c->path = path; c->label = label; c->kind = 0; c->lo = lo; c->hi = hi; c->skewMid = skewMid; c->integer = integer; c->text = text; c->r = r;
    c->knob = std::make_unique<hgui::Knob>(); addAndMakeVisible (*c->knob); c->knob->setText (label, {});
    auto& s = c->knob->slider; s.setRange (lo, hi, integer ? 1.0 : 0.0); if (skewMid > 0) s.setSkewFactorFromMidPoint (skewMid);
    if (text) s.textFromValueFunction = text; else if (! integer && hi - lo <= 2.01) s.textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    else if (! integer) s.textFromValueFunction = [] (double v) { return String (v, 2); };
    s.updateText();
    auto* cp = c.get();
    s.onValueChange = [this, cp] { if (! building) setValue (cp->path, cp->integer ? var (roundToInt (cp->knob->slider.getValue())) : var (cp->knob->slider.getValue())); };
    s.setTitle (label);
    ctls.push_back (std::move (c)); return *ctls.back();
}

SynthXPanel::Ctl& SynthXPanel::choice (const String& path, const String& label, std::function<StringArray()> items, Rectangle<int> r, int base)
{
    auto c = std::make_unique<Ctl>(); c->path = path; c->label = label; c->kind = 1; c->items = items; c->itemBase = base; c->r = r;
    c->combo = std::make_unique<ComboBox>(); addAndMakeVisible (*c->combo); c->combo->setTitle (label); c->combo->setTooltip (label);
    c->cap = std::make_unique<Label> (String(), label); addAndMakeVisible (*c->cap); c->cap->setFont (lnf.font (11)); c->cap->setColour (Label::textColourId, lnf.pal().mute);
    int id = 1; for (auto& s : items()) c->combo->addItem (s, id++);
    auto* cp = c.get();
    c->combo->onChange = [this, cp]
    {
        if (building) return;
        setValue (cp->path, cp->itemBase + cp->combo->getSelectedId() - 1);
        if (cp->path.startsWith ("o.") && cp->path.endsWith (".t") && ! cp->path.contains (".f.")) { prog = xs::Program::toVar (prog); commit(); }   // dalga dizinleri yeni türe uyar
        if (cp->path.endsWith (".t") || cp->path.endsWith (".sy") || (cp->path.startsWith ("fx.") && cp->path.endsWith (".0"))) rebuild();
    };
    ctls.push_back (std::move (c)); return *ctls.back();
}

bool SynthXPanel::loadSequence (const File& f)
{
    FileInputStream in (f); MidiFile mf; if (! in.openedOk() || ! mf.readFrom (in)) return false;
    const int tpq = mf.getTimeFormat(); if (tpq <= 0) return false;
    Array<var> ns; double end = 0;
    for (int t = 0; t < mf.getNumTracks() && ns.isEmpty(); ++t)
    {
        MidiMessageSequence seq (*mf.getTrack (t)); seq.updateMatchedPairs();
        for (int i = 0; i < seq.getNumEvents() && ns.size() < 256; ++i)
        {
            auto* ev = seq.getEventPointer (i); if (! ev->message.isNoteOn()) continue;
            const double b = ev->message.getTimeStamp() / tpq; if (b >= 64) break;
            const double off = ev->noteOffObject != nullptr ? ev->noteOffObject->message.getTimeStamp() / tpq : b + .25;
            ns.add (Array<var> { b, jmax (.02, off - b), ev->message.getNoteNumber() - 60, (int) ev->message.getVelocity() }); end = jmax (end, off);
        }
    }
    if (ns.isEmpty()) return false;
    if (! active()) prog = proc.xsFromClassic (part);
    auto* q = new DynamicObject(); q->setProperty ("n", ns); q->setProperty ("b", jmax (1.0, std::ceil (end / 4) * 4)); q->setProperty ("nm", f.getFileNameWithoutExtension());
    prog.getDynamicObject()->setProperty ("seq", var (q)); if (auto* a = resolve (prog, "arp.on")) *a = 0;
    commit(); rebuild(); return true;
}

File SynthXPanel::userDir() { return File::getSpecialLocation (File::userApplicationDataDirectory).getChildFile ("Wavejackers HitGen").getChildFile ("Synth X"); }

void SynthXPanel::fillTemplates()
{
    templBox.clear (dontSendNotification); int id = 1;
    for (auto& t : templates()) templBox.addItem (T (t.key, t.en), id++);
    userProgs = userDir().findChildFiles (File::findFiles, false, "*.hgx"); userProgs.sort();
    if (! userProgs.isEmpty()) { templBox.addSeparator(); templBox.addSectionHeading (T ("sxMine", "My programs")); for (int i = 0; i < userProgs.size(); ++i) templBox.addItem (userProgs[i].getFileNameWithoutExtension(), 1000 + i); }
    templBox.setTextWhenNothingSelected (T ("sxTemplate", "Templates"));
}

File SynthXPanel::saveProgram (const String& name)
{
    if (! active() || name.trim().isEmpty()) return {};
    userDir().createDirectory();
    auto f = userDir().getChildFile (File::createLegalFileName (name.trim()) + ".hgx");
    if (! f.replaceWithText (JSON::toString (prog, false))) return {};
    fillTemplates(); return f;
}

bool SynthXPanel::loadProgram (const File& f)
{
    const auto v = JSON::parse (f.loadFileAsString()); if (! v.isObject()) return false;
    prog = xs::Program::toVar (v); commit(); syncAll(); rebuild(); return true;
}

void SynthXPanel::paramKnob (const char* id, const String& label, Rectangle<int> r)
{
    auto c = std::make_unique<Ctl>(); c->path = {}; c->kind = 4; c->r = r;
    c->knob = std::make_unique<hgui::Knob>(); addAndMakeVisible (*c->knob); c->knob->setText (label, {});
    atts.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, c->knob->slider)); hgui::tagParam (c->knob->slider, id);
    if (String (id) == "kb_split") c->knob->slider.textFromValueFunction = [this] (double v) { const int n = roundToInt (v); return data.noteNames[n % 12] + String (n / 12 - 1); };   // bağlantıdan sonra: nota adı
    c->knob->slider.updateText();
    ctls.push_back (std::move (c));
}

void SynthXPanel::xgen (uint32_t seed)
{
    Random R ((int64) seed); auto f = [&] (double a, double b) { return a + (b - a) * R.nextDouble(); };
    prog = xs::Program::toVar (xs::Program::init()); const bool bass = part == 1, chords = part == 0;
    auto set = [this] (const String& path, var v) { if (auto* x = resolve (prog, path)) *x = v; };
    const int types[3] = { 1 + R.nextInt (3), R.nextInt (3) == 0 ? 0 : 1 + R.nextInt (3), R.nextInt (4) == 0 ? 1 : 0 };
    for (int k = 0; k < 3; ++k)
    {
        const String o = "o." + String (k) + "."; set (o + "t", types[k]); if (types[k] == 0) continue;
        set (o + "a", types[k] == 2 ? R.nextInt (xs::romCount()) : R.nextInt (xs::kBasic)); set (o + "b", types[k] == 2 ? R.nextInt (xs::romCount()) : R.nextInt (xs::kBasic));
        set (o + "m", f (0, 1)); set (o + "mm", R.nextInt (3) == 0 ? 1 : 0); set (o + "sm", R.nextInt (4) == 0 ? 1 + R.nextInt (2) : 0); set (o + "sh", f (0, .6)); set (o + "fm", f (0, .4)); set (o + "fr", (double) (1 + R.nextInt (4)));
        set (o + "u", bass ? 1 + R.nextInt (2) : 1 + R.nextInt (7)); set (o + "d", f (.05, .4)); set (o + "pm", R.nextInt (5) == 0 ? 1 + R.nextInt (3) : 0); set (o + "pw", f (.2, .8));
        set (o + "oc", k == 2 ? -1 : bass ? -1 + R.nextInt (2) : R.nextInt (2) - (k == 1 ? 1 : 0)); set (o + "se", k == 1 && R.nextBool() ? 7 : 0); set (o + "lv", k == 0 ? .8 : f (.3, .7));
        set (o + "n", 1 + R.nextInt (4));
        set (o + "f.t", 1 + R.nextInt (3)); set (o + "f.c", bass ? f (300, 1800) : f (900, 9000)); set (o + "f.r", f (.05, .55)); set (o + "f.e", f (0, .6)); set (o + "f.sa", R.nextInt (3) == 0 ? 1 + R.nextInt (5) : 0); set (o + "f.dr", f (0, .4));
    }
    const double atk = bass ? f (.001, .01) : chords ? f (.005, .6) : f (.001, .08);
    set ("env.0", Array<var> { atk, f (.15, 1.2), bass ? f (.5, 1) : f (.2, .9), bass ? f (.05, .3) : f (.2, 1.5), 0 });
    set ("env.1", Array<var> { .001, f (.1, .8), f (0, .4), f (.1, .6), 0 });
    if (R.nextBool()) { set ("lfo.0.w", R.nextInt (xs::kLfoShapes)); set ("lfo.0.sy", 1); set ("lfo.0.r", 3 + R.nextInt (7)); set ("lfo.0.d.0", Array<var> { 11, f (.1, .4) }); }
    if (R.nextInt (3) == 0) { set ("fx.0", Array<var> { 11, f (.2, .7), f (.2, .5), f (0, 1), .5, f (.1, .3) }); }
    if (! bass && R.nextBool()) { set ("fx.1", Array<var> { 12, f (.3, .9), .5, 1, .1, f (.1, .35) }); }
    set ("vol", .7); set ("vol", xs::calibratedVolume (prog, part, proc.getSampleRate() > 0 ? proc.getSampleRate() : 48000.0));
    commit(); syncAll(); rebuild();
}

SynthXPanel::Ctl& SynthXPanel::toggle (const String& path, const String& label, Rectangle<int> r)
{
    auto c = std::make_unique<Ctl>(); c->path = path; c->label = label; c->kind = 2; c->r = r;
    c->toggle = std::make_unique<ToggleButton> (label); addAndMakeVisible (*c->toggle);
    auto* cp = c.get();
    c->toggle->onClick = [this, cp] { if (! building) { setValue (cp->path, cp->toggle->getToggleState() ? 1 : 0); if (cp->path.endsWith (".sy")) rebuild(); } };
    ctls.push_back (std::move (c)); return *ctls.back();
}

void SynthXPanel::routes (const String& base, int x, int y, int w)
{
    const auto dests = [this] { StringArray s; s.add ("-"); for (int d = 0; d < xs::kNumDests; ++d) s.add (destLabel (d)); return s; };
    for (int i = 0; i < xs::kSlots; ++i)
    {
        const auto p = base + "." + String (i);
        auto& d = choice (p + ".0", T ("sxDest", "Target") + " " + String (i + 1), dests, { x, y + i * 34, w - 110, 26 }, -1); d.cap->setVisible (false);
        auto c = std::make_unique<Ctl>(); c->path = p + ".1"; c->kind = 3; c->r = { x + w - 104, y + i * 34, 104, 26 }; c->lo = -1; c->hi = 1;
        c->amount = std::make_unique<Slider> (Slider::LinearHorizontal, Slider::NoTextBox); c->amount->setPopupDisplayEnabled (true, true, this); c->amount->setColour (Slider::thumbColourId, lnf.pal().accent); c->amount->setColour (Slider::trackColourId, lnf.pal().accent.withAlpha (.5f)); c->amount->setColour (Slider::backgroundColourId, lnf.pal().line); addAndMakeVisible (*c->amount); c->amount->setRange (-1, 1, 0); c->amount->setDoubleClickReturnValue (true, 0);
        c->amount->setTitle (T ("sxAmount", "Amount") + " " + String (i + 1)); c->amount->textFromValueFunction = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
        c->amount->setTooltip (T ("sxAmountTip", "Modulation amount (double-click to reset)"));
        auto* cp = c.get(); c->amount->onValueChange = [this, cp] { if (! building) setValue (cp->path, cp->amount->getValue()); };
        ctls.push_back (std::move (c));
    }
}

void SynthXPanel::rebuild()
{
    const ScopedValueSetter<bool> b (building, true);
    for (auto& c : ctls) { for (Component* x : std::initializer_list<Component*> { c->knob.get(), c->combo.get(), c->toggle.get(), c->amount.get(), c->cap.get() }) if (x != nullptr) removeChildComponent (x); }
    atts.clear(); ctls.clear(); seqBtn.setVisible (false); seqClear.setVisible (false);
    for (auto* c : std::initializer_list<Component*> { &xy, &list, &search, &auditionBtn, &deleteBtn, &linkBtn, &saveCat, &kbMode, &stats, &hint }) c->setVisible (false);
    for (auto& c : catBtns) c.setVisible (false);
    for (auto& w : swatches) w.setVisible (false); steps.setVisible (false); scope.setVisible (false);
    const int Y = 76, KW = 82, KH = 92;
    auto grid = [&] (int col, int row) { return Rectangle<int> (372 + col * KW, Y + row * KH, KW - 4, KH - 4); };
    auto pct = [] (double v) { return String (roundToInt (v * 100)) + "%"; };
    auto bip = [] (double v) { const int x = roundToInt (v * 100); return (x > 0 ? "+" : "") + String (x) + "%"; };
    if (section <= 2)
    {
        const String o = "o." + String (section) + ".";
        const int type = active() ? (int) value (o + "t") : 0;
        const bool rom = type == 2, multi = type == 3;
        const auto basic = [] { StringArray s; for (int i = 0; i < xs::kBasic; ++i) s.add (xs::basicName (i)); return s; };
        const auto romL = [] { StringArray s; for (int i = 0; i < xs::romCount(); ++i) s.add (xs::romName (i)); return s; };
        const auto romSf = [romL] { StringArray s; s.add ("SoundFont"); s.addArray (romL()); return s; };
        auto col = [&] (int c, int r) { return Rectangle<int> (16 + c * 176, Y + 16 + r * 50, 166, 26); };
        choice (o + "t", T ("sxType", "Oscillator"), [this] { return StringArray { T ("sxOff", "Off"), "Mod", "ROM", "Multiwave" }; }, col (0, 0));
        if (multi)
        {
            for (int s = 0; s < 4; ++s) choice (o + "mw." + String (s), T ("sxStack", "Stack") + " " + String (s + 1), basic, col (s % 2, 1 + s / 2));
            choice (o + "n", T ("sxStacks", "Stacks"), [] { return StringArray { "1 x 8", "2 x 8", "3 x 8", "4 x 8" }; }, col (1, 0), 1);
        }
        else
        {
            choice (o + "a", T ("sxWaveA", "Wave A"), rom ? std::function<StringArray()> (romSf) : std::function<StringArray()> (basic), col (0, 1), rom ? -1 : 0);
            choice (o + "b", T ("sxWaveB", "Wave B"), rom ? std::function<StringArray()> (romL) : std::function<StringArray()> (basic), col (1, 1));
            choice (o + "mm", T ("sxMorphMode", "Morph mode"), [this] { return StringArray { T ("sxBlend", "Blend A-B"), T ("sxSurface", "Wave surface") }; }, col (0, 2));
            choice (o + "sm", T ("sxSynth", "Synthesis"), [this] { return StringArray { T ("sxSub", "Subtractive"), "FM", T ("sxPd", "Phase distortion") }; }, col (1, 2));
            choice (o + "u", T ("sxUnison", "Unison"), [] { StringArray s; for (int i = 1; i <= xs::kMaxUni; ++i) s.add (String (i) + (i == 1 ? " voice" : " voices")); return s; }, col (0, 3), 1);
            choice (o + "ly", T ("sxLayering", "Layering"), [] { return StringArray { "x1", "x2" }; }, col (1, 3), 1);
            choice (o + "ph", T ("sxPhase", "Phase mode"), [this] { return StringArray { T ("sxFree", "Free"), T ("sxReset", "Reset"), T ("sxSpreadPh", "Spread"), T ("sxFixed", "Fixed") }; }, col (0, 4));
        }
        choice (o + "pm", T ("sxPwMode", "PW mode"), [this] { return StringArray { T ("sxOff", "Off"), T ("sxWidth", "Width"), T ("sxDual", "Dual saw"), T ("sxSync", "Sync") }; }, col (1, 4));
        choice (o + "oc", T ("sxOctave", "Octave"), [] { return StringArray { "-3", "-2", "-1", "0", "+1", "+2", "+3" }; }, col (0, 5), -3);
        choice (o + "f.t", T ("sxFilter", "Filter"), [this] { StringArray s; for (int i = 0; i < xs::kFilterTypes; ++i) s.add (N ("Flt", i, xs::filterName (i))); return s; }, col (1, 5));
        choice (o + "f.sa", T ("sxSat", "Saturation"), [this] { StringArray s; for (int i = 0; i < xs::kSatModes; ++i) s.add (N ("Sat", i, xs::satName (i))); return s; }, col (0, 6));
        int k = 0; auto K = [&] (const String& path, const String& key, const String& en, double lo, double hi, double mid = 0, bool integer = false, std::function<String (double)> tx = {}) { knob (o + path, T (key, en), lo, hi, grid (k % 9, k / 9), mid, integer, tx); ++k; };
        K ("lv", "sxLevel", "Level", 0, 1); K ("pa", "sxPan", "Pan", -1, 1, 0, false, bip); K ("se", "sxSemi", "Semitone", -12, 12, 0, true); K ("fi", "sxFine", "Fine", -1, 1, 0, false, [] (double v) { return String (roundToInt (v * 100)) + " ct"; });
        if (multi) { K ("d", "sxDetune", "Detune", 0, 1); K ("mx", "sxMix", "Center/sides", 0, 1); for (int s = 0; s < 4; ++s) { knob (o + "ml." + String (s), T ("sxStack", "Stack") + " " + String (s + 1), 0, 1, grid (k % 9, k / 9)); ++k; } }
        else
        {
            K ("m", "sxMorph", "Morph", 0, 1); K ("sh", "sxShape", "Shape", 0, 1); K ("d", "sxDetune", "Detune", 0, 1); K ("sp", "sxSpread", "Spread", 0, 1); K ("pv", "sxPhaseV", "Phase", 0, 1);
            K ("fm", "sxFm", "FM", 0, 1); K ("fr", "sxFmRatio", "FM ratio", .25, 16, 2, false, [] (double v) { return String (v, 2); }); K ("ff", "sxFmFilter", "FM filter", 0, 1);
            K ("ls", "sxLayerSemi", "Layer pitch", -24, 24, 0, true, [] (double v) { return String (roundToInt (v)) + " st"; }); K ("ll", "sxLayerLv", "Layer level", 0, 1);
        }
        K ("pw", "sxPw", "PW", .02, .98, 0, false, pct);
        while (k % 9 != 0) ++k;
        K ("f.c", "sxCutoff", "Cutoff", 20, 20000, 1000, false, [] (double v) { return v >= 1000 ? String (v / 1000, 1) + " kHz" : String (roundToInt (v)) + " Hz"; });
        K ("f.r", "sxRes", "Resonance", 0, 1); K ("f.e", "sxFenv", "Env 2 amount", -1, 1, 0, false, bip); K ("f.k", "sxKeyTrack", "Key track", 0, 1); K ("f.dr", "sxDrive", "Drive", 0, 1);
        K ("f.o", "sxDuo", "Duo offset", -4, 4, 0, false, [] (double v) { return String (v, 1) + " oct"; });
        scope.setVisible (true);
    }
    else if (section == 3)
    {
        const auto waves = [this] { StringArray s; s.add (T ("sxCrackle", "Crackle")); s.add (T ("sxPink", "Pink noise")); s.add (T ("sxWhite", "White noise")); for (int i = 0; i < xs::romCount(); ++i) s.add (xs::romName (i)); return s; };
        toggle ("lx.on", T ("sxLayerOn", "Layer FX on"), { 16, Y + 16, 200, 26 });
        choice ("lx.w", T ("sxWave", "Wave"), waves, { 16, Y + 66, 260, 26 }, -3);
        choice ("lx.t", T ("sxFilter", "Filter"), [] { return StringArray { "Low pass", "High pass", "Band pass", "Notch" }; }, { 16, Y + 116, 260, 26 });
        int k = 0; auto K = [&] (const String& path, const String& key, const String& en, double lo, double hi, double mid = 0, bool integer = false, std::function<String (double)> tx = {}) { knob (path, T (key, en), lo, hi, grid (k % 9, k / 9), mid, integer, tx); ++k; };
        K ("lx.lv", "sxLevel", "Level", 0, 1); K ("lx.p", "sxPitch", "Pitch", -24, 24, 0, false, [] (double v) { return String (v, 1) + " st"; });
        K ("lx.c", "sxCutoff", "Cutoff", 20, 20000, 1000, false, [] (double v) { return v >= 1000 ? String (v / 1000, 1) + " kHz" : String (roundToInt (v)) + " Hz"; }); K ("lx.r", "sxRes", "Resonance", 0, 1);
        auto note = [this] (double v) { const int n = roundToInt (v); return data.noteNames[n % 12] + String (n / 12 - 1); };
        K ("lx.kl", "sxKeyLo", "Lowest key", 0, 127, 0, true, note); K ("lx.kh", "sxKeyHi", "Highest key", 0, 127, 0, true, note);
        K ("lx.vl", "sxVelLo", "Lowest velocity", 0, 127, 0, true); K ("lx.vh", "sxVelHi", "Highest velocity", 0, 127, 0, true);
    }
    else if (section == 4)
    {
        for (int e = 0; e < xs::kEnvs; ++e)
        {
            const int x = 16 + e * 284; const String p = "env." + String (e) + ".";
            static const char* const nk[5] = { "sxA", "sxD", "sxS", "sxR", "sxCurve" }; static const char* const ne[5] = { "Attack", "Decay", "Sustain", "Release", "Curve" };
            for (int j = 0; j < 5; ++j)
            {
                const bool time = j == 0 || j == 1 || j == 3;
                knob (p + String (j), T (nk[j], ne[j]), time ? .0005 : j == 4 ? -1 : 0, time ? 20 : 1, { x + j * 54, Y + 22, 54, 86 }, time ? .5 : 0, false,
                      time ? std::function<String (double)> ([] (double v) { return v < 1 ? String (roundToInt (v * 1000)) + " ms" : String (v, 2) + " s"; }) : std::function<String (double)>());
            }
            if (e > 0) routes ("ed." + String (e - 1), x, Y + 128, 270);
        }
    }
    else if (section == 5)
    {
        for (int l = 0; l < xs::kLfos; ++l)
        {
            const int x = 16 + l * 570; const String p = "lfo." + String (l) + ".";
            choice (p + "w", T ("sxShapeL", "Shape"), [this] { StringArray s; for (int i = 0; i < xs::kLfoShapes; ++i) s.add (N ("Lfo", i, xs::lfoShapeName (i))); return s; }, { x, Y + 16, 200, 26 });
            toggle (p + "sy", T ("sxSyncT", "Tempo sync"), { x, Y + 56, 140, 26 }); toggle (p + "rt", T ("sxRetrig", "Restart on note"), { x, Y + 90, 200, 26 });
            const bool sync = active() && (int) value (p + "sy") != 0;
            if (sync) knob (p + "r", T ("sxRate", "Rate"), 0, 11, { x + 220, Y + 10, 82, 92 }, 0, true, [] (double v) { return String (kDiv[jlimit (0, 11, roundToInt (v))]); });
            else knob (p + "r", T ("sxRate", "Rate"), .01, 40, { x + 220, Y + 10, 82, 92 }, 2, false, [] (double v) { return String (v, 2) + " Hz"; });
            knob (p + "fa", T ("sxFade", "Fade in"), 0, 10, { x + 306, Y + 10, 82, 92 }, 1, false, [] (double v) { return String (v, 2) + " s"; });
            routes (p + "d", x, Y + 128, 540);
        }
    }
    else if (section == 6)
    {
        steps.setVisible (true);
        knob ("stp.n", T ("sxStepsN", "Steps"), 1, 16, { 16, Y + 200, 82, 92 }, 0, true);
        knob ("stp.r", T ("sxRate", "Rate"), 0, 11, { 102, Y + 200, 82, 92 }, 0, true, [] (double v) { return String (kDiv[jlimit (0, 11, roundToInt (v))]); });
        knob ("stp.gl", T ("sxGlideS", "Smooth"), 0, 1, { 188, Y + 200, 82, 92 });
        routes ("stp.d", 300, Y + 200, 540);
    }
    else if (section == 8)
    {   // efekt rafı (5 yuva, sırayla işlenir) ve master
        for (int i = 0; i < xs::kFxSlots; ++i)
        {
            const String p = "fx." + String (i) + "."; const int y = Y + 4 + i * 76, t = active() ? (int) value (p + "0") : 0;
            choice (p + "0", T ("sxSlot", "Slot") + " " + String (i + 1), [this] { StringArray s; for (int k = 0; k < xs::kFxTypes; ++k) s.add (N ("Fx", k, xs::fxName (k))); return s; }, { 16, y + 22, 170, 26 });
            if (t == 0) continue;
            for (int k = 0; k < 4; ++k) if (xs::fxParamName (t, k) != "-") knob (p + String (k + 1), N ("FxP", t * 4 + k, xs::fxParamName (t, k)), 0, 1, { 196 + k * 74, y, 72, 72 });
            knob (p + "5", T ("sxMixFx", "Mix"), 0, 1, { 196 + 4 * 74, y, 72, 72 });
        }
        paramKnob ("mst_hp", T ("sxMHp", "Low cut"), { 700, Y + 24, 82, 92 }); paramKnob ("mst_lp", T ("sxMLp", "High cut"), { 786, Y + 24, 82, 92 });
        paramKnob ("mst_width", T ("sxMWidth", "Stereo width"), { 872, Y + 24, 82, 92 }); paramKnob ("mst_color", T ("sxMColor", "Output color"), { 958, Y + 24, 82, 92 });
        paramKnob ("mst_out", T ("sxMOut", "Output"), { 1044, Y + 24, 82, 92 });
    }
    else if (section == 9)
    {   // çalım: akıllı akor, strum, arpej, tuşa göre pan, Alpha düğmesi
        choice ("chd.m", T ("sxChord", "Chord mode"), [this] { return StringArray { T ("sxOff", "Off"), T ("sxTriad", "Triad (in key)"), T ("sxSeventh", "Seventh (in key)"), "Power", T ("sxOct", "Octave"), "Sus2", "Sus4" }; }, { 16, Y + 26, 200, 26 });
        knob ("chd.st", T ("sxStrum", "Strum"), 0, 1, { 226, Y + 10, 82, 92 }, 0, false, [] (double v) { return String (roundToInt (v * 120)) + " ms"; });
        toggle ("arp.on", T ("sxArpOn", "Arpeggiator"), { 16, Y + 130, 200, 26 });
        choice ("arp.md", T ("sxArpMode", "Arp mode"), [this] { return StringArray { T ("sxUp", "Up"), T ("sxDown", "Down"), T ("sxUpDown", "Up-down"), T ("sxRandom", "Random"), T ("sxPlayed", "As played") }; }, { 16, Y + 186, 200, 26 });
        choice ("arp.dv", T ("sxArpRate", "Arp rate"), [] { return StringArray { "1/4", "1/8", "1/16", "1/8 T", "1/16 T", "1/32" }; }, { 226, Y + 186, 120, 26 });
        choice ("arp.oc", T ("sxArpOct", "Octaves"), [] { return StringArray { "1", "2", "3" }; }, { 356, Y + 186, 90, 26 }, 1);
        knob ("arp.gt", T ("sxGate", "Gate"), .05, 1, { 456, Y + 160, 82, 92 });
        knob ("kp", T ("sxKeyPan", "Key panning"), -1, 1, { 16, Y + 250, 82, 92 }, 0, false, [] (double v) { const int x = roundToInt (v * 100); return (x > 0 ? "+" : "") + String (x) + "%"; });
        paramKnob ("xs_alpha", T ("sxAlpha", "Alpha knob"), { 640, Y + 10, 82, 92 });
        toggle ("chd.sl", T ("sxScaleLock", "Scale lock (remap to key)"), { 16, Y + 76, 204, 26 });
        choice ("chd.sd", T ("sxSplitSide", "Helpers play on"), [this] { return StringArray { T ("sxWhole", "Whole keyboard"), T ("sxBelow", "Below split"), T ("sxAbove", "Above split") }; }, { 640, Y + 150, 200, 26 });
        knob ("chd.sp", T ("sxSplit", "Split key"), 24, 96, { 850, Y + 124, 82, 92 }, 0, true, [this] (double v) { const int n = roundToInt (v); return data.noteNames[n % 12] + String (n / 12 - 1); });
        choice ("glm", T ("sxGlideMode", "Glide"), [this] { return StringArray { T ("sxGlideAll", "Every note"), T ("sxGlideLeg", "Auto (legato only)") }; }, { 640, Y + 250, 200, 26 });
        choice ("glq", T ("sxGlideQ", "Glide time"), [this] { return StringArray { T ("sxGlideFree", "Free (Matrix tab)"), "1/4", "1/8", "1/16", "1/8 T", "1/16 T", "1/32" }; }, { 850, Y + 250, 170, 26 });
        knob ("chd.h1", T ("sxHarm1", "Harmony 1"), -24, 24, { 330, Y + 10, 82, 92 }, 0, true, [] (double v) { const int x = roundToInt (v); return x == 0 ? String ("-") : (x > 0 ? "+" : "") + String (x); });
        knob ("chd.h2", T ("sxHarm2", "Harmony 2"), -24, 24, { 416, Y + 10, 82, 92 }, 0, true, [] (double v) { const int x = roundToInt (v); return x == 0 ? String ("-") : (x > 0 ? "+" : "") + String (x); });
        toggle ("chd.hk", T ("sxHarmKey", "Harmony in key (steps)"), { 330, Y + 104, 240, 24 });
        seqBtn.setVisible (true); seqClear.setVisible (true);
        const auto nm = active() ? prog["seq"]["nm"].toString() : String();
        seqBtn.setButtonText (nm.isNotEmpty() ? T ("sxSeqOn", "Sequence") + ": " + nm : T ("sxSeqLoad", "Load MIDI sequence")); seqClear.setEnabled (nm.isNotEmpty());
    }
    else if (section == 10)
    {   // makrolar, Morph X/Y, klavye (Multi) modu, Picasso
        const String b = "xs_" + partCode() + "_";
        for (int k = 0; k < 4; ++k) paramKnob ((b + "k" + String (k + 1)).toRawUTF8(), T ("sxMacroN", "Macro") + " " + String (k + 1), { 16 + k * 90, Y + 10, 86, 92 });
        xy.setVisible (true); linkBtn.setVisible (true); kbMode.setVisible (true); hint.setVisible (true);
        paramKnob ("kb_split", T ("sxKbSplitKey", "Split key"), { 640, Y + 210, 82, 92 });
        for (auto& w : swatches) w.setVisible (true);
    }
    else if (section == 11)
    {   // tarayıcı
        list.setVisible (true); search.setVisible (true); auditionBtn.setVisible (true); deleteBtn.setVisible (true); saveCat.setVisible (true); stats.setVisible (true);
        for (auto& c : catBtns) c.setVisible (true);
        saveBtn.setVisible (true);
        refreshBrowser();
    }
    else
    {
        for (int m = 0; m < xs::kMtx; ++m)
        {
            const int x = 16 + m * 440;
            choice ("mtx." + String (m) + ".s", T ("sxSource", "Source") + " " + String (m + 1), [this] { StringArray s; for (int i = 0; i < xs::kNumSources; ++i) s.add (N ("Src", i, xs::sourceName (i))); return s; }, { x, Y + 16, 200, 26 });
            routes ("mtx." + String (m) + ".d", x, Y + 60, 420);
        }
        knob ("vol", T ("sxVolume", "Volume"), 0, 2, { 900, Y + 10, 82, 92 });
        knob ("gl", T ("sxGlide", "Glide"), 0, 2, { 986, Y + 10, 82, 92 }, .3, false, [] (double v) { return v <= 0 ? String ("-") : String (roundToInt (v * 1000)) + " ms"; });
    }
    for (auto& c : ctls)
    {
        if (c->knob) c->knob->setBounds (c->r);
        if (c->combo) { c->combo->setBounds (c->r); if (c->cap) c->cap->setBounds (c->r.getX(), c->r.getY() - 15, c->r.getWidth(), 14); }
        if (c->toggle) c->toggle->setBounds (c->r);
        if (c->amount) c->amount->setBounds (c->r);
    }
    syncAll(); resized(); repaint();
}

void SynthXPanel::syncAll()
{
    const ScopedValueSetter<bool> b (building, true);
    const bool on = active();
    for (auto& c : ctls)
    {
        if (c->kind == 4) continue;   // DAW parametresi: bağlantı eşitler
        const auto v = on ? value (c->path) : var();
        if (c->knob) { c->knob->slider.setValue (v.isVoid() ? c->lo : (double) v, dontSendNotification); c->knob->slider.updateText(); c->knob->setEnabled (on); }
        if (c->combo) { c->combo->setSelectedId (v.isVoid() ? 0 : (int) v - c->itemBase + 1, dontSendNotification); c->combo->setEnabled (on); }
        if (c->toggle) { c->toggle->setToggleState (! v.isVoid() && (int) v != 0, dontSendNotification); c->toggle->setEnabled (on); }
        if (c->amount) { c->amount->setValue (v.isVoid() ? 0.0 : (double) v, dontSendNotification); c->amount->setEnabled (on); }
    }
    convertBtn.setEnabled (true); sfBtn.setEnabled (true);
    steps.repaint(); scope.repaint();
}

void SynthXPanel::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    g.setColour (P.panel.withAlpha (.92f)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 12);
    g.setColour (P.line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (.5f), 12, 1);
    if (! active())
    {
        g.setColour (P.mute); g.setFont (lnf.font (14));
        g.drawFittedText (T ("sxOffHint", "This part plays with the classic engine. Switch on Synth X, convert the sound or pick a template to edit it here."), getLocalBounds().withTrimmedTop (300).reduced (40, 0), Justification::centredTop, 2);
    }
    if (section == 4) for (int e = 0; e < xs::kEnvs; ++e) { g.setColour (P.mute); g.setFont (lnf.font (12, true)); g.drawText ((e == 0 ? T ("sxEnvAmp", "Env 1 (volume)") : "Env " + String (e + 1)), 16 + e * 284, 74, 270, 16, Justification::centredLeft); }
    if (section == 10)
    {
        g.setColour (P.mute); g.setFont (lnf.font (12, true));
        g.drawText (T ("sxPicasso", "Picasso: interface colour"), 640, 74 + 10, 400, 18, Justification::centredLeft);
        g.drawText (T ("sxKbMode", "Keyboard (Multi mode)"), 640, 74 + 154, 400, 18, Justification::centredLeft);
    }
    if (section == 5) for (int l = 0; l < 2; ++l) { g.setColour (P.mute); g.setFont (lnf.font (12, true)); g.drawText ("LFO " + String (l + 1), 16 + l * 570, 186, 200, 16, Justification::centredLeft); }
}

void SynthXPanel::resized()
{
    int x = 16; auto put = [&] (Component& c, int w) { c.setBounds (x, 8, w, 28); x += w + 8; };
    put (convertBtn, 110); put (initBtn, 64); put (templBox, 180); put (saveBtn, 64); put (xgenBtn, 70); put (sfBtn, 130); sfLbl.setBounds (x, 8, getWidth() - x - 16 - 190, 28);
    for (int i = 0; i < 12; ++i) tabs[(size_t) i].setBounds (16 + i * 92, 42, 88, 26);
    steps.setBounds (16, 92, getWidth() - 32, 170);
    seqBtn.setBounds (310, 76 + 250 + 26, 260, 28); seqClear.setBounds (576, 76 + 250 + 26, 28, 28);
    const int Y = 76;
    xy.setBounds (400, Y + 10, 200, 200); linkBtn.setBounds (400, Y + 220, 200, 28);
    hint.setBounds (16, Y + 120, 370, 120); hint.setJustificationType (Justification::topLeft);
    kbMode.setBounds (640, Y + 176, 220, 26);
    for (int i = 0; i < 9; ++i) swatches[(size_t) i].setBounds (640 + i * 52, Y + 40, i == 0 ? 48 : 40, 32);
    for (int i = 0; i < 9; ++i) catBtns[(size_t) i].setBounds (16 + i * 96, Y + 4, 92, 26);
    search.setBounds (16, Y + 36, 300, 26); list.setBounds (16, Y + 68, 760, 300);
    auditionBtn.setBounds (790, Y + 68, 160, 30); deleteBtn.setBounds (790, Y + 104, 160, 30); saveCat.setBounds (790, Y + 160, 160, 26);
    stats.setBounds (790, Y + 200, 340, 160); stats.setJustificationType (Justification::topLeft);
    scope.setBounds (getWidth() - 16 - 180, 8, 180, 28);
}

void SynthXPanel::Steps::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); const auto r = getLocalBounds().toFloat(); g.setColour (P.panel2); g.fillRoundedRectangle (r, 8);
    const int n = o.active() ? jlimit (1, xs::kSteps, (int) o.value ("stp.n")) : xs::kSteps; const float w = r.getWidth() / xs::kSteps, mid = r.getCentreY();
    g.setColour (P.line); g.fillRect (r.getX(), mid, r.getWidth(), 1.0f);
    for (int i = 0; i < xs::kSteps; ++i)
    {
        const float v = o.active() ? (float) (double) o.value ("stp.v." + String (i)) : 0.0f;
        g.setColour ((i < n ? P.accent : P.mute).withAlpha (i < n ? .9f : .3f));
        const float h = v * (r.getHeight() * .5f - 6);
        g.fillRoundedRectangle (r.getX() + i * w + 3, h > 0 ? mid - h : mid, w - 6, jmax (2.0f, std::abs (h)), 2);
    }
}
void SynthXPanel::Steps::mouseDrag (const MouseEvent& e)
{
    if (! o.active()) return;
    const int i = jlimit (0, xs::kSteps - 1, e.x * xs::kSteps / jmax (1, getWidth()));
    const float v = jlimit (-1.0f, 1.0f, ((float) getHeight() * .5f - (float) e.y) / ((float) getHeight() * .5f - 6));
    o.setValue ("stp.v." + String (i), std::round (v * 100) / 100); repaint();
}

void SynthXPanel::Scope::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); g.setColour (P.panel2); g.fillRoundedRectangle (getLocalBounds().toFloat(), 6);
    if (! o.active() || o.section > 2) return;
    const String b = "o." + String (o.section) + ".";
    const int t = (int) o.value (b + "t"); if (t == 0) return;
    const int a = (int) o.value (b + "a"), bb = (int) o.value (b + "b"); const float m = (float) (double) o.value (b + "m");
    if (t == 2 && a < 0) { g.setColour (P.mute); g.setFont (o.lnf.font (11)); g.drawText ("SoundFont", getLocalBounds(), Justification::centred); return; }
    std::vector<float> wa, wb; const int bank = t == 2 ? 1 : 0, n = getWidth();
    xs::waveShape (bank, t == 3 ? (int) o.value (b + "mw.0") : a, wa, n); xs::waveShape (bank, t == 3 ? (int) o.value (b + "mw.0") : bb, wb, n);
    Path p; const float h = (float) getHeight() * .5f - 3;
    for (int i = 0; i < n; ++i) { const float y = (float) getHeight() * .5f - (wa[(size_t) i] + (wb[(size_t) i] - wa[(size_t) i]) * m) * h; if (i == 0) p.startNewSubPath (0, y); else p.lineTo ((float) i, y); }
    g.setColour (P.accent); g.strokePath (p, PathStrokeType (1.5f));
}

// ---- tarayıcı, istatistik, kısayollar, Picasso, Morph pad
void SynthXPanel::refreshBrowser()
{
    shown.clear(); const auto q = searchText.trim().toLowerCase();
    auto add = [&] (Entry e) { if ((catFilter == "All" || e.cat == catFilter || (catFilter == "Mine" && e.file != File())) && (q.isEmpty() || e.name.toLowerCase().contains (q))) shown.push_back (e); };
    for (int i = 0; i < numTemplates(); ++i) add ({ T (templates()[(size_t) i].key, templates()[(size_t) i].en), kTplCat[i], i, {} });
    for (auto& f : userProgs) { const auto v = JSON::parse (f.loadFileAsString()); add ({ f.getFileNameWithoutExtension(), v["cat"].toString().isNotEmpty() ? v["cat"].toString() : String ("Other"), -1, f }); }
    list.updateContent(); list.repaint();
    stats.setText (statsText(), dontSendNotification); hint.setText (T ("sxMacroHint", "Macros and Morph X/Y are automatable from your DAW. In the Matrix tab pick Macro 1-4 or Morph X/Y as a source and send it to any targets."), dontSendNotification);
}
void SynthXPanel::paintListBoxItem (int row, Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) shown.size()) return; const auto& P = lnf.pal(); const auto& e = shown[(size_t) row];
    if (selected) { g.setColour (P.accent.withAlpha (.25f)); g.fillRect (0, 0, w, h); }
    g.setColour (P.ink); g.setFont (lnf.font (13, e.file != File())); g.drawText (e.name, 10, 0, w - 160, h, Justification::centredLeft);
    g.setColour (P.mute); g.setFont (lnf.font (11)); g.drawText (T ("sxCat" + e.cat, e.cat) + (e.file != File() ? "  ·  " + T ("sxMine", "My programs") : String()), w - 200, 0, 190, h, Justification::centredRight);
}
void SynthXPanel::openEntry (int row)
{
    if (row < 0 || row >= (int) shown.size()) return;
    const auto& e = shown[(size_t) row]; if (e.tpl >= 0) applyTemplate (e.tpl); else loadProgram (e.file);
    list.selectRow (row); if (proc.demoPlaying()) proc.playDemo (part);   // dinleme açıkken yeni sesle yeniden başlar
}
String SynthXPanel::statsText() const
{
    const int mins = (int) ((Time::currentTimeMillis() - proc.statStart) / 60000), play = (int) proc.statPlaySec.load();
    return T ("sxStats", "This session") + "\n" + T ("sxStatTime", "Open for") + ": " + String (mins) + " min\n" + T ("sxStatPlay", "Playing") + ": " + String (play / 60) + ":" + String (play % 60).paddedLeft ('0', 2)
         + "\n" + T ("sxStatNotes", "Notes played") + ": " + String (proc.statNotes.load()) + "\n" + T ("sxStatEdits", "Sound edits") + ": " + String (proc.statEdits);
}
File SynthXPanel::saveProgram (const String& name, const String& cat)
{
    if (! active()) return {};
    prog.getDynamicObject()->setProperty ("cat", cat);
    return saveProgram (name);
}
bool SynthXPanel::keyPressed (const KeyPress& k)
{
    const auto m = k.getModifiers(); const int c = k.getKeyCode();
    if (m.isCommandDown() && (c == 'G' || c == 'g')) { xgen ((uint32_t) Random::getSystemRandom().nextInt()); return true; }
    if (m.isCommandDown() && (c == 'B' || c == 'b')) { showTab (11); return true; }
    if (m.isCommandDown() && (c == 'S' || c == 's')) { saveBtn.triggerClick(); return true; }
    if (m.isAltDown() && c >= '1' && c <= '9') { showTab (c - '1'); return true; }
    if (m.isAltDown() && c == '0') { showTab (9); return true; }
    if (section == 11 && (c == KeyPress::upKey || c == KeyPress::downKey) && ! shown.empty()) { openEntry (jlimit (0, (int) shown.size() - 1, list.getSelectedRow() + (c == KeyPress::upKey ? -1 : 1))); return true; }
    if (section == 11 && c == KeyPress::spaceKey) { auditionBtn.triggerClick(); return true; }
    return false;
}
void SynthXPanel::setAccent (Colour col)
{
    proc.customAccent = col; if (proc.onAccentChanged) proc.onAccentChanged();
}
void SynthXPanel::Swatch::paintButton (Graphics& g, bool over, bool)
{
    const auto& P = o->lnf.pal(); auto r = getLocalBounds().toFloat().reduced (2);
    const bool sel = col.isTransparent() ? o->proc.customAccent.isTransparent() : o->proc.customAccent == col;
    if (col.isTransparent()) { g.setColour (P.panel2); g.fillRoundedRectangle (r, 8); g.setColour (P.ink); g.setFont (o->lnf.font (12, true)); g.drawText (getButtonText(), r, Justification::centred); }
    else { g.setColour (col); g.fillEllipse (r.withSizeKeepingCentre (r.getHeight(), r.getHeight())); }
    if (sel || over) { g.setColour (sel ? P.ink : P.mute); g.drawRoundedRectangle (r, 8, sel ? 2.0f : 1.0f); }
}
void SynthXPanel::XY::paint (Graphics& g)
{
    const auto& P = o.lnf.pal(); const auto r = getLocalBounds().toFloat(); g.setColour (P.panel2); g.fillRoundedRectangle (r, 10);
    g.setColour (P.line); g.drawLine (r.getCentreX(), r.getY(), r.getCentreX(), r.getBottom()); g.drawLine (r.getX(), r.getCentreY(), r.getRight(), r.getCentreY());
    const String b = "xs_" + o.partCode() + "_"; const float x = o.proc.apvts.getRawParameterValue (b + "mx")->load() / 100.0f, y = o.proc.apvts.getRawParameterValue (b + "my")->load() / 100.0f;
    g.setColour (P.accent); g.fillEllipse (r.getX() + x * r.getWidth() - 9, r.getBottom() - y * r.getHeight() - 9, 18, 18);
    g.setColour (P.mute); g.setFont (o.lnf.font (11)); g.drawText ("Morph X / Y", r.reduced (8), Justification::topLeft);
}
void SynthXPanel::XY::mouseDrag (const MouseEvent& e)
{
    const String b = "xs_" + o.partCode() + "_";
    const float x = jlimit (0.0f, 1.0f, (float) e.x / (float) jmax (1, getWidth())), y = jlimit (0.0f, 1.0f, 1 - (float) e.y / (float) jmax (1, getHeight()));
    if (auto* p = o.proc.apvts.getParameter (b + "mx")) p->setValueNotifyingHost (x);
    if (auto* p = o.proc.apvts.getParameter (b + "my")) p->setValueNotifyingHost (y);
    repaint();
}
