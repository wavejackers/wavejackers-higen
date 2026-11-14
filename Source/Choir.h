// Koro: tek bir sesten (dahili ünlü ya da kullanıcının kaydı) akor ilerlemesini izleyen dört partili koro üretir.
// Kullanıcının sesi perde tespiti + TD-PSOLA ile hedef notaya çekilir (formant korunur: ses "sincap" gibi olmaz).
// Döngü arka planda çevrimdışı işlenir; ses iş parçacığı yalnızca hazır tamponu okur (bellek ayırmaz, kilit almaz).
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <atomic>
#include <vector>

namespace ch
{
constexpr int kParts = 4;                       // 0 bas, 1 tenor, 2 alto, 3 soprano
constexpr double kMaxVoiceSeconds = 12.0;
enum Source { sAah, sOoh, sEeh, sMmm, sOwnHold, sOwnWords, kNumSources };   // sOwnHold: ünlüyü akor boyunca uzatır; sOwnWords: kaydı her notada baştan söyler
enum Rhythm { rHold, rHalf, rQuarter, rStab, kNumRhythms };

struct Bar { int pc = 9, third = 3, fifth = 7; };   // ölçü başına akor: kök perde sınıfı ve aralıklar

struct Settings
{
    int source = sAah, rhythm = rHold, size = 3, seed = 1;      // size: parti başına şarkıcı (1-4)
    std::array<bool, kParts> part { { true, true, true, true } };
    float level = .7f, space = .6f, detune = .35f, air = .4f, vibrato = .3f, attack = .4f, character = 0;   // character -1..1: koyu..parlak (formant)
    juce::ValueTree toTree() const;
    void fromTree (const juce::ValueTree&);
};

// Dört partinin ölçü başına notaları (MIDI): bas kökte, üst sesler en yakın akor sesine gider (ses yürüyüşü)
std::vector<std::array<int, kParts>> voicing (const std::vector<Bar>&);

// Kullanıcı sesinin çözümlemesi: perde işaretleri ve uzatma bölgesi
struct Voice
{
    std::vector<float> x; double sr = 48000, f0 = 0;      // f0: ortanca perde (Hz); 0 = perdesiz (fısıltı, gürültü)
    std::vector<int> mark; std::vector<float> period;      // perde işaretleri (örnek) ve yerel periyot
    int loopA = 0, loopB = 0;                               // uzatma bölgesi (işaret dizini)
    float voiced = 0;                                       // perdeli çerçeve oranı 0..1
};
Voice analyse (const juce::AudioBuffer<float>&, double sr);
double detectPitch (const float* x, int n, double sr);     // tek çerçeve (YIN); perde yoksa 0

struct Loop { juce::AudioBuffer<float> buf; double bpm = 120, sr = 48000; int bars = 1; std::array<std::vector<float>, kParts> act; };   // act: parti zarfı (ölçü başına 32 nokta)
std::unique_ptr<Loop> render (const Settings&, const std::vector<Bar>&, const Voice&, double bpm, double sr);

class Engine : private juce::Thread
{
public:
    Engine();
    ~Engine() override;
    // ---- ses iş parçacığı
    void prepare (double sampleRate);
    void process (juce::AudioBuffer<float>&, int n, bool hostPlay, double ppq, double bpm) noexcept;
    void feed (const float* l, const float* r, int n, double rate) noexcept;     // kayıt sürerken girişi yazar (ses ya da aygıt iş parçacığı)
    // ---- mesaj iş parçacığı
    bool loadFile (const juce::File&);
    void recordStart();
    bool recordStop();                                 // kaydı ses olarak yükler; ses yoksa false
    bool recording() const noexcept { return recState.load() == 1; }
    float recLevel() const noexcept { return recPeak.load(); }
    double recSeconds() const noexcept { return recRate > 0 ? recPos.load() / recRate : 0; }
    Settings get() const;
    void set (const Settings&);
    void setChords (const std::vector<Bar>&);
    void setTempo (double bpm);
    void setPlaying (bool b) noexcept { playing.store (b); }
    bool isPlaying() const noexcept { return playing.load(); }
    bool hasVoice() const;
    double voicePitch() const;                         // Hz; 0 = yok ya da perdesiz
    juce::String voiceName() const;
    void peaks (std::vector<float>& out, int n) const;
    float playhead() const noexcept { return headPos.load(); }
    float activity (int part) const noexcept { return act[(size_t) juce::jlimit (0, kParts - 1, part)].load(); }   // çalan partinin anlık seviyesi 0..1
    int version() const noexcept { return doneVersion.load(); }
    bool busy() const noexcept { return wantVersion.load() != doneVersion.load(); }
    juce::File exportWav();
    void collect();
    juce::ValueTree toTree() const;
    void fromTree (const juce::ValueTree&);
private:
    void run() override;
    void changed();
    bool setVoice (juce::AudioBuffer<float>&&, double sr, const juce::String& name, const juce::File& path);
    mutable juce::CriticalSection lock;
    Settings s; std::vector<Bar> bars; Voice voice; double tempo = 120; juce::String name; juce::File path;
    std::atomic<int> wantVersion { 0 }, doneVersion { 0 };
    std::atomic<Loop*> pending { nullptr }; Loop* live = nullptr; std::array<std::atomic<Loop*>, 8> trash {};
    std::unique_ptr<Loop> last;
    std::atomic<bool> playing { false }; std::atomic<float> headPos { 0 }; std::array<std::atomic<float>, kParts> act {};
    double sr = 48000, freePos = 0;
    std::vector<float> rec; std::atomic<int> recState { 0 }, recPos { 0 }, feeding { 0 }; std::atomic<float> recPeak { 0 }; double recRate = 0;
    juce::AudioFormatManager formats;
};
}
