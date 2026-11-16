// Ses tasarımı sayfasındaki Synth X düzenleyicisi: seçili partinin Synth X programını düzenler.
// Program JSON olarak işlemcide tutulur (proje ve preset ile kaydedilir); her değişiklik çalan notalara canlı geçer.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Ui.h"

class SynthXPanel : public juce::Component, private juce::ListBoxModel
{
public:
    SynthXPanel (HitGenProcessor& p, hgui::LookAndFeel& l);
    void setPart (int part);
    void setLanguage (const juce::String& lang);
    void refresh();                                   // programı işlemciden yeniden oku
    void tick() { if (proc.xsVersion != shownVer) refresh(); }
    void paint (juce::Graphics&) override;
    void resized() override;
    // testler
    void showTab (int t);
    int tab() const noexcept { return section; }
    void applyTemplate (int i);
    void xgen (uint32_t seed);
    bool loadSequence (const juce::File&);
    static juce::File userDir();                      // kullanıcı programları (.hgx)
    juce::File saveProgram (const juce::String& name); // programı kaydeder; şablon listesine eklenir
    bool loadProgram (const juce::File&);
    void fillTemplates();
    juce::File saveProgram (const juce::String& name, const juce::String& cat);
    struct Entry { juce::String name, cat; int tpl = -1; juce::File file; };
    const std::vector<Entry>& browserEntries() const { return shown; }
    void setBrowserFilter (const juce::String& cat, const juce::String& search) { catFilter = cat; searchText = search; refreshBrowser(); }
    void openEntry (int row);
    juce::String statsText() const;
    bool keyPressed (const juce::KeyPress&) override;     // kısayol tuşları
    void setAccent (juce::Colour c);                      // Picasso            // MIDI dosyasını sekans olarak yükle (tutulan tuşa göre aktarılır)                        // X-Gen: partiye uygun rastgele program
    static int numTemplates();
    void setValue (const juce::String& path, const juce::var& v);   // yol: "o.0.f.c", "env.1.2", "lfo.0.d.3.1"
    juce::var value (const juce::String& path) const;
    int numControls() const { return (int) ctls.size(); }

private:
    struct Ctl
    {
        juce::String path, label; int kind = 0;          // 0 düğme, 1 seçim, 2 aç/kapa, 3 yatay miktar
        double lo = 0, hi = 1, def = 0, skewMid = 0; bool integer = false; std::function<juce::String (double)> text;
        std::function<juce::StringArray()> items; int itemBase = 0;   // seçim: değer = itemBase + dizin
        std::unique_ptr<hgui::Knob> knob; std::unique_ptr<juce::ComboBox> combo; std::unique_ptr<juce::ToggleButton> toggle; std::unique_ptr<juce::Slider> amount; std::unique_ptr<juce::Label> cap;
        juce::Rectangle<int> r;
    };
    struct Steps : juce::Component
    {
        SynthXPanel& o; explicit Steps (SynthXPanel& p) : o (p) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
        void mouseDrag (const juce::MouseEvent&) override;
    };
    struct XY : juce::Component
    {
        SynthXPanel& o; explicit XY (SynthXPanel& p) : o (p) {}
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
        void mouseDrag (const juce::MouseEvent&) override;
    };
    struct Scope : juce::Component { SynthXPanel& o; explicit Scope (SynthXPanel& p) : o (p) {} void paint (juce::Graphics&) override; };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf; const hg::Data& data;
    juce::String lang = "en"; int part = 0, section = 0, shownVer = -1; juce::var prog; bool building = false;
    juce::TextButton convertBtn, initBtn, sfBtn, xgenBtn, seqBtn, seqClear, saveBtn;
    juce::Array<juce::File> userProgs; juce::ComboBox templBox; juce::Label sfLbl;
    std::array<juce::TextButton, 12> tabs;
    std::vector<std::unique_ptr<Ctl>> ctls; Steps steps { *this }; Scope scope { *this };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> atts;   // düğmelerden sonra yok edilir (önce bağlantılar)
    std::unique_ptr<juce::FileChooser> chooser;
    XY xy { *this }; juce::ListBox list; juce::TextEditor search; juce::TextButton auditionBtn, deleteBtn, linkBtn; juce::ComboBox saveCat, kbMode; juce::Label stats, hint;
    struct Swatch : juce::Button
    {
        juce::Colour col; SynthXPanel* o = nullptr; Swatch() : juce::Button ({}) {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
    };
    std::array<juce::TextButton, 9> catBtns; std::array<Swatch, 9> swatches;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> kbAtt;
    std::vector<Entry> shown; juce::String catFilter = "All", searchText;
    void refreshBrowser();
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { openEntry (row); }
    void returnKeyPressed (int row) override { openEntry (row); }
    juce::String partCode() const { return part == 0 ? "c" : part == 1 ? "b" : "m"; }

    juce::String T (const juce::String& key, const juce::String& en) const;
    juce::String N (const juce::String& kind, int i, const juce::String& en) const;   // liste adlarının çevirisi
    juce::String destLabel (int d) const;
    void rebuild();                                   // seçili bölümün denetimlerini kur
    void syncAll();                                   // değerleri programdan denetimlere yaz
    void commit();                                    // programı işlemciye gönder
    bool active() const { return prog.isObject(); }
    Ctl& knob (const juce::String& path, const juce::String& label, double lo, double hi, juce::Rectangle<int> r, double skewMid = 0, bool integer = false, std::function<juce::String (double)> text = {});
    Ctl& choice (const juce::String& path, const juce::String& label, std::function<juce::StringArray()> items, juce::Rectangle<int> r, int base = 0);
    Ctl& toggle (const juce::String& path, const juce::String& label, juce::Rectangle<int> r);
    void paramKnob (const char* id, const juce::String& label, juce::Rectangle<int> r);   // DAW parametresine bağlı düğme (master, Alpha)
    void routes (const juce::String& base, int x, int y, int w);   // 5 hedef satırı
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SynthXPanel)
};
