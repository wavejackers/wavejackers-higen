#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Ui.h"
#include "SoundPage.h"
#include "PresetPage.h"
#include "IntroOverlay.h"
#include "ProjectPage.h"
#include "ReferencePage.h"
#include "DrumPage.h"
#include "Assistant.h"

class HitGenEditor : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    explicit HitGenEditor (HitGenProcessor&);
    ~HitGenEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void mouseDown (const juce::MouseEvent&) override;   // alt bileşenlerden de gelir: sağ tık > MIDI Learn
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    static constexpr int kW = 1180, kH = 760;

    // testler için
    juce::Component& contentComponent() { return content; }
    hgui::PianoRoll& pianoRoll() { return roll; }
    void setLanguage (const juce::String& l);
    void setTab (int t);
    void setWorkspace (int w);   // 0 Reference, 1 Smart Detect, 2 DNA Engine, 3 Generate
    static bool autoGuide, autoIntro;       // ilk açılışta kılavuzu göster (testlerde kapalı)
    int currentWorkspace() const noexcept { return ws; }   // 0 üret, 1 ses tasarımı, 2 presetler, 3 referans
    int currentTab() const noexcept { return tab; }
    SoundPage& sounds() { return *soundPage; }
    ReferencePage& reference() { return *refPage; }
    DrumPage& drums() { return *drumPage; }
    hgui::GuideOverlay& guideOverlay() { return guide; }
    hgui::AssistantBubble& assistantBubble() { return bubble; }
    void startGuide();
    void startIntro();                         // açılış ekranı + sesi (testler doğrudan çağırabilir)
    IntroOverlay& introOverlay() { return intro; }
    ProjectPage* project() { return projectPage.get(); }
    void advanceIntro (double ms);              // zamanlayıcı her karede çağırır; testler doğrudan
    juce::Rectangle<int> eyeHome() const { return orbHome; }
    juce::Rectangle<int> eyeBounds() const { return orb.getBounds(); }
    void openAssistant();
    juce::String assistantText() const { return sugg.empty() ? juce::String() : sugg[(size_t) juce::jlimit (0, (int) sugg.size() - 1, suggIdx)].text; }
    int assistantCount() const { return (int) sugg.size(); }

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Content : public juce::Component { HitGenEditor* owner = nullptr; void paint (juce::Graphics&) override; };
    juce::Image artElder, artAngelF, artAngelM, artHead;   // karakter teması: önceden ölçeklenmiş arka plan figürleri ve başlık portresi
    void loadArt();
    struct StyleModel : public juce::ListBoxModel
    {
        HitGenEditor& ed; explicit StyleModel (HitGenEditor& e) : ed (e) {}
        int getNumRows() override { return ed.filtered.size(); }
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        void selectedRowsChanged (int row) override;
        juce::String getNameForRow (int row) override;
    };

    HitGenProcessor& proc;
    const hg::Data& data;
    hgui::LookAndFeel lnf;
    Content content;
    juce::TooltipWindow tooltips { this, 600 };

    juce::Label tagline, status, disclaimer, varLabel, soundLabel, hostLabel;
    juce::TextButton playBtn, newBtn;
    juce::Slider seed; juce::Label seedLabel;
    juce::ComboBox langBox, qualityBox, sizeBox, keyBox, scaleBox, barsBox, keysToBox;
    juce::Label langLabel, qualityLabel, sizeLabel, keyLabel, scaleLabel, barsLabel, keysToLabel;
    juce::ToggleButton contrastBtn, motionBtn, seventhsBtn, patternBtn, midiOutBtn;
    std::array<juce::TextButton, 4> altBtns;
    float phase = 0.0f, meter = 0.0f, meterHold = 0.0f; int frame = 0;
    static constexpr int kNumGroups = 11;   // tümü, 9 tür grubu, kendi presetlerim
    std::array<juce::TextButton, kNumGroups> groupBtns; std::array<juce::TextButton, 9> tabBtns; int tab = 0; int ws = 0;   // tabBtns 0-3: çalışma alanları (Reference, Smart Detect, DNA Engine, Generate); 4-6: MIDI Studio, Sound Design, Presets; 7: Drum machine (tab 4); 8: Proje (tab 5)
    juce::TextButton presetBtn, saveBtn, settingsBtn, helpBtn; bool settingsOpen = false;
    struct Backdrop : public juce::Component { HitGenEditor* o = nullptr; void paint (juce::Graphics&) override; } settingsPanel;
    void toggleSettings (bool on); void showHelp(); void showSaveDialog(); void showPresetMenu();
    // MIDI Learn
    juce::TextButton midiBtn; int midiShownVer = -1, midiSeenLearned = -1; juce::String midiToast; juce::int64 midiToastUntil = 0;
    void showMidiMenu (juce::Component& target, const juce::String& paramId); void showMidiMappings(); void refreshMidiBadges(); void toast (const juce::String& s, int ms = 3500);
public:
    // testler için
    void midiLearnFor (const juce::String& paramId) { const int i = proc.midiLearn.indexOf (paramId); if (i >= 0) proc.midiLearn.startLearn (i); }
    juce::String midiToastText() const { return midiToast; }
    void refreshMidiNow() { refreshMidiBadges(); }
private:
    juce::ListBox styleList; StyleModel styleModel { *this }; juce::Array<int> filtered; int group = 0;
    std::array<juce::TextButton, 4> varBtns;
    std::array<hgui::Knob, 6> genKnobs;
    hgui::ChordStrip chords; hgui::PianoRoll roll;
    juce::TextButton editBtn, undoBtn, redoBtn, revertBtn; std::array<juce::TextButton, 4> laneBtns;   // MIDI düzenleme araç çubuğu
    std::unique_ptr<juce::FileChooser> ownChooser;
    void updateEditBar();
    void chooseOwnSound (int track);      // "Kendi sesini yükle": davulda davul makinesine, diğerlerinde dosya/preset seçimine yönlendirir
    static int partOfTrack (int t) noexcept { return t == 0 ? 0 : t == 1 ? 1 : t == 3 ? 2 : -1; }
    std::array<juce::TextButton, 4> muteBtns; std::array<juce::ComboBox, 4> soundBoxes; std::array<hgui::DragTile, 4> tiles; std::array<juce::Label, 4> trackLabels;
    std::array<juce::StringArray, 4> soundIds;
    std::array<juce::TextButton, 3> partBtns; int part = 0;
    std::array<hgui::Knob, 8> partKnobs;
    hgui::DragTile loopTile, fullTile;

    std::unique_ptr<SoundPage> soundPage; std::unique_ptr<PresetPage> presetPage; std::unique_ptr<ProjectPage> projectPage; std::unique_ptr<ReferencePage> refPage; std::unique_ptr<DrumPage> drumPage;
    // kılavuz ve asistan (2.7)
    hgui::AssistantOrb orb { lnf }; hgui::AssistantBubble bubble { lnf }; hgui::GuideOverlay guide { lnf };
    juce::ToggleButton loudBtn, introBtn;
    IntroOverlay intro { lnf }; juce::Rectangle<int> orbHome; double introLast = 0.0; bool guideAfterIntro = false;
    static bool introEnabled(); static void setIntroEnabled (bool);
    struct ToastView : public juce::Component { HitGenEditor* owner = nullptr; void paint (juce::Graphics&) override; } toastView;   // bildirim hapı her sayfanın üstünde
    juce::String toastShown;
    struct Suggestion { juce::String id, title, text, applyText; std::function<void()> apply; };
    std::vector<Suggestion> sugg; int suggIdx = 0; juce::StringArray seenSugg; int suggTick = 0; bool everPlayed = false;
    std::vector<Suggestion> collectSuggestions();
    void showSuggestion (int i);
    void showHelpMenu();
    static juce::File uiSettingsFile();
    static bool guideDone();
    static void setGuideDone();
    std::vector<juce::Component*> genComps;
    bool isMine() const noexcept { return group == kNumGroups - 1; }
    std::vector<std::unique_ptr<SA>> sa; std::vector<std::unique_ptr<CA>> ca; std::vector<std::unique_ptr<BA>> ba;
    std::vector<std::unique_ptr<SA>> partSa; std::vector<std::unique_ptr<BA>> muteBa;

    juce::String lang; bool rtl = false; int shownStyle = -1, shownVar = -1;
    std::array<juce::String, 4> shownSounds;
    std::vector<std::pair<juce::Component*, juce::Rectangle<int>>> layoutList; juce::Rectangle<int> settingsPanelBounds;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    void timerCallback() override;
    void relabel();
    void refreshResult();
    void refreshStyleList();
    void fillSoundBoxes();
    void bindPart (int p);
    void applyPalette();
    void setParam (const char* id, float plain);
    void place (juce::Component& c, juce::Rectangle<int> r) { layoutList.push_back ({ &c, r }); }
    void layoutContent();
    juce::Colour styleAccent() const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitGenEditor)
};
