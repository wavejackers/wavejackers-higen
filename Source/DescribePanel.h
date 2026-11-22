// Ses tasarımı > Tarif: kullanıcı istediği sesi metinle anlatır; sesi Synth X üretir (harici servis yok).
// Az düğme: Üret, Yeniden üret, Dinle, WAV indir; LFO ve Arpej açma/kapama; tek bir Hız düğmesi (her tıklamada BPM'e
// senkron bir sonraki değer). Ayrıntılar "Gelişmiş ayarlar" açılır panelinde. Tüm denetimler klavyeyle kullanılabilir.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "SoundPrompt.h"
#include "Ui.h"

class DescribePanel : public juce::Component, private juce::Timer
{
public:
    DescribePanel (HitGenProcessor& p, hgui::LookAndFeel& l);
    ~DescribePanel() override;
    void setLanguage (const juce::String& lang);
    void setPart (int part) { curPart = part; }
    std::function<void (int part)> onPartChange;      // üretilen ses başka partiye aitse sayfa o partiye geçer
    std::function<void()> onOpenEditor;               // tam Synth X düzenleyicisi
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void visibilityChanged() override;
    // testler ve kısayollar
    void generate (const juce::String& text);
    void regenerate();
    bool hasSound() const noexcept { return clipBuf != nullptr; }
    const sp::Design& design() const noexcept { return D; }
    const juce::AudioBuffer<float>* clip() const noexcept { return clipBuf.get(); }
    juce::File exportTo (const juce::File& f);
    void setLfo (bool on); void setArp (bool on); void cycleRate (int dir = 1);
    juce::String rateText() const { return rateBtn.state; }
    void togglePlay();
    void setAdvancedOpen (bool open);
    bool advancedOpen() const noexcept { return advOpen; }
    void setAdvanced (int which, double value);       // 0 parlaklık 1 rezonans 2 atak 3 bırakma 4 distortion 5 reverb 6 sidechain
    juce::String statusText() const { return status.getText(); }
    sp::Context context() const;

private:
    // durum göstergeli düğme: ad, durum yazısı ve renkli nokta (renk tek başına anlam taşımaz)
    struct StateButton : juce::TextButton
    {
        DescribePanel& o; juce::String name, state; bool on = false; bool showDot = true;
        explicit StateButton (DescribePanel& p) : o (p) {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        bool keyPressed (const juce::KeyPress&) override;
    };
    struct Wave : juce::Component { DescribePanel& o; explicit Wave (DescribePanel& p) : o (p) {} void paint (juce::Graphics&) override; void mouseDown (const juce::MouseEvent&) override { o.togglePlay(); } };
    struct Chips : juce::Component { DescribePanel& o; explicit Chips (DescribePanel& p) : o (p) {} void paint (juce::Graphics&) override; };

    HitGenProcessor& proc; hgui::LookAndFeel& lnf;
    juce::String lang = "en"; int curPart = 1;
    juce::Label promptLbl, bpmLbl, keyLbl, lenLbl, status;
    juce::TextEditor prompt;
    juce::Slider bpm { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    juce::ComboBox keyBox, lenBox;
    juce::TextButton genBtn, regenBtn, playBtn, exportBtn, editorBtn;
    StateButton lfoBtn { *this }, arpBtn { *this }, rateBtn { *this }, advBtn { *this };
    Wave wave { *this }; Chips chips { *this };
    std::array<std::unique_ptr<hgui::Knob>, 7> adv; bool advOpen = false;
    std::unique_ptr<juce::FileChooser> chooser;

    sp::Design D; juce::var program; bool haveDesign = false; juce::String lastText;
    std::unique_ptr<juce::AudioBuffer<float>> clipBuf; double clipRate = 48000;
    std::vector<float> peaks; juce::int64 dirtyAt = 0; bool building = false;

    juce::String T (const char* tr, const char* en) const;
    void refreshTexts();
    void applyDesign (bool announce);                 // program kur, partiye yükle, ön dinlemeyi işle
    void rerender();
    void syncControls();
    void timerCallback() override;
    double sampleRate() const;
    void startExport();
};
