// Ses tasarımı sekmesi (3.1): ortada canlı dalga küresi ve dört makro yayı (Parlaklık, Hareket, Sürüş, Alan),
// üstte iz seçimi, ses seçici, ADSR ve ses seviyesi; altta görünüm çubuğu, temel nota, ekran klavyesi ve ön dinleme.
// Görünümler: Makro (küre), Tüm parametreler (osilatör, filtre, zarf, LFO, efekt) ve Ses tarayıcısı (kategori, açıklama, kullanım).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Ui.h"

class SoundPage : public juce::Component
{
public:
    SoundPage (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~SoundPage() override;
    void setLanguage (const juce::String& lang, bool rtl);
    void setPart (int part);
    int currentPart() const noexcept { return part; }
    void startImport() { doImport(); }
    std::function<void()> onRebind;    // parametre bağları değişti (MIDI Learn rozetleri yenilenir)
    void tick();                       // editör zamanlayıcısından: küre, seviye, ses listesi eşitlemesi
    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    static constexpr int kW = 1164, kH = 654;
    enum View { vMacro, vEdit, vBrowse };
    void setView (View v);
    View currentView() const noexcept { return view; }
    juce::StringArray browserIds() const { return list; }   // testler: tarayıcıda görünen sesler (sırasıyla)

private:
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // ---- makro yayı: kürenin çevresinde kalın bir yay, üzerinde kayan tutamak; yayın dışında döndürülmüş etiket
    struct ArcSlider : public juce::Slider
    {
        explicit ArcSlider (SoundPage& o);
        SoundPage& owner; juce::Point<float> c; float R = 170, band = 26, a0 = 0, a1 = 1; juce::String label; bool flipText = false;
        void setGeometry (juce::Point<float> centreInParent, float radius, float bandW, float angMin, float angMax);
        void paint (juce::Graphics&) override;
        bool hitTest (int x, int y) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        bool keyPressed (const juce::KeyPress&) override;
        juce::Point<float> local() const { return c - getPosition().toFloat(); }
        float angleAt (juce::Point<float> p) const;
        void setFromAngle (float a);
    };
    // ---- küçük döner düğme (ADSR, seviye)
    struct Dial : public juce::Slider
    {
        explicit Dial (SoundPage& o);
        SoundPage& owner;
        void paint (juce::Graphics&) override;
    };
    // ---- canlı küre: parametrelerden hesaplanan dalga biçimi, iz renginde parıltı, çıkış seviyesiyle hareket
    struct Orb : public juce::Component
    {
        SoundPage* owner = nullptr; std::array<float, hg::kNumSp> v {}; float level = 0, phase = 0;
        void paint (juce::Graphics&) override;
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    };
    // ---- eski ayrıntılı önizleme (dalga, filtre eğrisi, zarf): Tüm parametreler görünümünde
    struct Scope : public juce::Component
    {
        SoundPage* owner = nullptr; std::array<float, hg::kNumSp> v {};
        void paint (juce::Graphics&) override;
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
    };
    struct EditPanel : public juce::Component { SoundPage* owner = nullptr; void paint (juce::Graphics&) override; };
    // ---- simge düğmesi (görünüm çubuğu, önceki/sonraki, rastgele, sıfırla, oktav)
    struct IconButton : public juce::Button
    {
        IconButton (SoundPage& o, int icon) : juce::Button ({}), owner (o), kind (icon) {}
        SoundPage& owner; int kind;
        void paintButton (juce::Graphics&, bool over, bool down) override;
    };
    // ---- ses tarayıcısı: kategori sütunu ve açıklamalı ses listesi
    struct CatModel : public juce::ListBoxModel
    {
        SoundPage& o; explicit CatModel (SoundPage& s) : o (s) {}
        int getNumRows() override;
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        void selectedRowsChanged (int row) override;
        juce::String getNameForRow (int row) override;
    };
    struct SoundModel : public juce::ListBoxModel
    {
        SoundPage& o; explicit SoundModel (SoundPage& s) : o (s) {}
        int getNumRows() override;
        void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override;
        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override;
        void returnKeyPressed (int row) override;
        juce::String getNameForRow (int row) override;
    };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; bool rtl = false; int part = 0; View view = vMacro;
    // üst çubuk
    IconButton partBtn0 { *this, 11 }, partBtn1 { *this, 11 }, partBtn2 { *this, 11 };
    std::array<IconButton*, 3> partBtns { &partBtn0, &partBtn1, &partBtn2 };
    IconButton catBtn { *this, 9 }, nameBtn { *this, 10 };
    IconButton prevBtn { *this, 0 }, nextBtn { *this, 1 }, randBtn { *this, 2 }, resetBtn { *this, 3 };
    std::array<std::unique_ptr<Dial>, 4> adsr; Dial volDial { *this };
    // küre ve makrolar
    Orb orb; std::array<std::unique_ptr<ArcSlider>, 4> arcs;
    juce::Label descLbl, useLbl, hint;
    // alt çubuk
    IconButton viewMacro { *this, 4 }, viewEdit { *this, 5 }, viewBrowse { *this, 6 }, octDown { *this, 7 }, octUp { *this, 8 };
    IconButton demoBtn { *this, 12 }, importBtn { *this, 13 };
    juce::MidiKeyboardComponent keyboard; int baseNote = 48;
    // tüm parametreler
    EditPanel editPanel; juce::Viewport editView;
    std::map<int, std::unique_ptr<hgui::Knob>> knobs;
    std::map<int, std::unique_ptr<juce::ComboBox>> combos;
    std::map<int, std::unique_ptr<juce::ToggleButton>> toggles;
    Scope scope;
    struct Title { juce::String key, arg; juce::Rectangle<int> r; bool small; };
    std::vector<Title> titles; std::vector<juce::Rectangle<int>> panels;
    // tarayıcı
    CatModel catModel { *this }; SoundModel soundModel { *this };
    juce::ListBox catList, soundList;
    juce::StringArray cats;            // "", "@wf", sonra kategori kodları
    juce::StringArray list;            // görünen ses kimlikleri (filtrelenmiş)
    juce::String catFilter;            // "" tümü, "@wf" fabrika 26, ya da kategori
    juce::String shownId; juce::int64 msgUntil = 0; float shownPeak = 0;

    std::vector<std::unique_ptr<SA>> sa; std::vector<std::unique_ptr<CA>> ca; std::vector<std::unique_ptr<BA>> ba;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::String T (const juce::String& k) const { return data.tr (lang, k); }
    juce::String partId() const { return part == 0 ? "chords" : part == 1 ? "bass" : "melody"; }
    int trackOfPart() const { return part == 2 ? hg::kMelody : part; }
    char role() const { return part == 0 ? 'c' : part == 1 ? 'b' : 'm'; }
    juce::Colour tint() const { return lnf.pal().track[trackOfPart()]; }
    juce::Rectangle<int> mir (juce::Rectangle<int> r) const { return rtl ? r.withX (kW - r.getRight()) : r; }
    void bind();
    void fillCombos();
    void rebuildList (bool keepFilter = true);
    void selectSound (const juce::String& id);
    void stepSound (int dir);
    void refreshInfo();
    void layoutEdit();
    void doImport();
    void showCatMenu();
    void setBaseNote (int n);
    juce::String noteLabel (int n) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoundPage)
};
