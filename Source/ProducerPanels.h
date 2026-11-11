// HitGen 3.0 panelleri (Proje sekmesinden açılır):
//  Auto Producer: tür, BPM, ton, makam, süre ve enerji seçilir; tek tıkla tam parça (iskelet + fikir + sesler).
//  Co-Producer: kullanıcının MIDI'si yüklenir; ton ve rol bulunur, en uyumlu fikir aranır, kullanıcının notaları kilitli kalır.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Ui.h"

struct HgGenre { const char* id; const char* name; const char* style; const char* fallback; int bpm; int structure; int seconds; };
extern const HgGenre kHgGenres[6];

class AutoProducerPanel : public juce::Component
{
public:
    AutoProducerPanel (HitGenProcessor& p, hgui::LookAndFeel& l, const juce::String& lang, std::function<void (const juce::String&)> done);
    void resized() override;
    void paint (juce::Graphics&) override;
    void produce();                       // testler de çağırır
    juce::ComboBox genre, key, scale, length;
    juce::Slider bpm, energy;
private:
    HitGenProcessor& proc; hgui::LookAndFeel& lnf; juce::String lang;
    std::function<void (const juce::String&)> onDone;
    juce::Label title, hint, lGenre, lBpm, lKey, lLen, lEnergy;
    juce::TextButton go;
    juce::String T (const juce::String& k) const { return proc.data.tr (lang, k); }
    void genreChanged();
};

class CoProducerPanel : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    CoProducerPanel (HitGenProcessor& p, hgui::LookAndFeel& l, const juce::String& lang, std::function<void (const juce::String&)> done);
    void resized() override;
    void paint (juce::Graphics&) override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int, int) override;
    bool loadFile (const juce::File& f);  // testler de çağırır
    void complete();                      // en uyumlu fikri uygula
    void next();                          // sıradaki aday
    juce::String infoText() const { return info.getText(); }
    const hg::UserMidi& userMidi() const { return um; }
    juce::ComboBox role;
private:
    HitGenProcessor& proc; hgui::LookAndFeel& lnf; juce::String lang;
    std::function<void (const juce::String&)> onDone;
    juce::Label title, hint, info, status;
    juce::TextButton load, apply, alt, unlock;
    std::unique_ptr<juce::FileChooser> chooser;
    hg::UserMidi um; hg::KeyGuess kg; std::vector<hg::CoCandidate> cands; int idx = 0; bool loaded = false;
    juce::String T (const juce::String& k) const { return proc.data.tr (lang, k); }
    juce::String roleName (int r) const { return T (r == hg::kChords ? "coRoleChords" : r == hg::kBass ? "coRoleBass" : "coRoleMel"); }
    void updateButtons();
};
