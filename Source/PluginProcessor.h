#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Data.h"
#include "Generator.h"
#include "Synth.h"

// Ses iş parçacığının çaldığı desen: PPQ (çeyrek nota) cinsinden olaylar
struct SeqEvent { double ppq = 0, len = 0; int track = 0, note = 0, vel = 0; };
struct Pattern { std::vector<SeqEvent> ev; double lenPpq = 32; };

class HitGenProcessor : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener, private juce::Timer
{
public:
    HitGenProcessor();
    ~HitGenProcessor() override;

    const juce::String getName() const override { return "Wavejackers HitGen"; }
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Arayüz için (mesaj iş parçacığı)
    const hg::Data& data;
    juce::AudioProcessorValueTreeState apvts;
    std::shared_ptr<const hg::Result> result() const { return current; }
    juce::String soundId (int track) const { return sounds[(size_t) track]; }
    void setSound (int track, const juce::String& id, bool manual);
    void applyStyleDefaults();          // stil değişince ses, tempo ve pump ayarları
    void regenerateNow();
    void newIdea();
    juce::String lang = "en";
    float uiScale = 1.0f; bool highContrast = false;
    std::function<void()> onResultChanged;
    juce::File writeMidi (int which);   // 0..3 tek iz, 4 tüm izler (döngü), 5 tam parça; geçici klasöre yazar
    juce::MemoryBlock midiBytes (int which) const;
    juce::String baseName() const;

    std::atomic<bool> previewPlaying { false };
    std::atomic<double> playPpq { 0 }, hostBpm { 0 };
    std::atomic<bool> hostPlaying { false };
    hg::Engine engine;
    static juce::StringArray styleNames (const hg::Data& d);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const hg::Data& d);
    void parameterChanged (const juce::String& id, float) override;
    void timerCallback() override;
    void loadPatch (int track);
    hg::GenParams currentParams() const;
    void pushPattern (const hg::Result& r);

    std::array<juce::String, 4> sounds;
    std::array<bool, 4> manual {};
    std::shared_ptr<const hg::Result> current;
    std::atomic<bool> regenPending { true }, styleChanged { false };
    int lastStyle = -1;
    double sr = 44100; int blockSize = 512;

    // desen değişimi (kilitsiz)
    std::atomic<Pattern*> pendingPattern { nullptr };
    Pattern* livePattern = nullptr;
    juce::AbstractFifo patTrash { 16 }; std::array<Pattern*, 16> patTrashSlots {};
    // ses iş parçacığı durumu
    bool wasRunning = false; double previewPos = 0, lastPpqEnd = -1;
    struct BlockEv { int off; int kind; int track, note, vel; double len; };   // kind 0 nota, 1 tuş açık, 2 tuş kapalı
    std::array<BlockEv, 1024> blockEv {}; int numBlockEv = 0;
    struct PendingOff { long long at; int ch, note; };
    std::array<PendingOff, 512> offs {}; int numOffs = 0; long long sampleClock = 0;

    std::atomic<float>* p (const char* id) const { return apvts.getRawParameterValue (id); }
    // ses iş parçacığı için önceden çözülmüş parametre işaretçileri (String ayırmadan okunur)
    struct PartPtrs { std::atomic<float>* vol = nullptr, *mute = nullptr, *cut = nullptr, *res = nullptr, *atk = nullptr, *rel = nullptr, *drv = nullptr, *rev = nullptr, *dly = nullptr; };
    std::array<PartPtrs, 4> pp {};
    std::atomic<float>* pQuality = nullptr, *pThick = nullptr, *pPump = nullptr, *pMaster = nullptr, *pBpm = nullptr, *pPattern = nullptr, *pMidiOut = nullptr, *pKeysTo = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitGenProcessor)
};
