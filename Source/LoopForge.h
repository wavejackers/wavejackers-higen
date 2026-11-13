// LoopForge: tek vuruşluk sesten (ya da eklentinin son 30 saniyelik çıkışından) tempoya kilitli, dikişsiz döngü üretir.
// Döngü arka planda çevrimdışı işlenir (efekt kuyruğu başa sarılır); ses iş parçacığı yalnızca hazır tamponu okur:
// bellek ayırmaz, kilit almaz. Dışa aktarılan dosya ile duyulan döngü aynı tampondur.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <atomic>

namespace lf
{
constexpr int kMaxSteps = 32;
constexpr double kRingSeconds = 30.0;

struct Step
{
    bool on = false, rev = false;
    int pitch = 0;        // gam derecesi (-14..14): seçili gamın dışına çıkmaz
    int ratchet = 1;      // 1-4 alt tetik
    float vel = .85f, cut = 1.0f, pan = 0.0f, decay = .5f;   // cut 1 = filtre açık; pan -1..1
    float shift = 0.0f, attack = 0.0f;                         // shift -1..1 = ±50 ms; attack 0..1 = 2..80 ms
};

struct Settings
{
    int steps = 16, pulses = 5, rotation = 0, bars = 2, root = 9, scale = 1;   // scale: kScales dizini
    float swing = 0, human = 0, start = 0, end = 1, space = .25f, drive = .15f, motion = 0;
    int seed = 1;
    std::array<Step, kMaxSteps> step {};
    juce::ValueTree toTree() const;
    void fromTree (const juce::ValueTree&);
};

struct ScaleDef { const char* name; int iv[7]; };
extern const ScaleDef kScales[5];   // Minor, Phrygian, Dorian, Harmonic minor, Major
int degreeToSemis (int scale, int degree);

// Öklid ritmi: pulses vuruşu steps adıma en eşit biçimde dağıtır, rotation kadar döndürür
std::array<bool, kMaxSteps> euclid (int steps, int pulses, int rotation);
enum MutateMode { mSubtle, mEuclid, mArp };
void mutate (Settings&, MutateMode, uint32_t seed);

// Baş ve sondaki sessizliği atar, sınırları en yakın sıfır geçişine çeker (0..1 oranları döner)
void autoTrim (const juce::AudioBuffer<float>&, float& start, float& end);
// En güçlü vuruşu bulur: başlangıcı vuruş öncesine, sonu sönüme (en çok maxSeconds) koyar
void findHit (const juce::AudioBuffer<float>&, double sr, double maxSeconds, int& from, int& to);

struct Loop { juce::AudioBuffer<float> buf; double bpm = 120, sr = 48000; int bars = 2; };
// İki geçişli çevrimdışı işleme: kuru döngü efekt zincirinden iki kez geçer, ikinci tur tutulur (kuyruk başa sarılır)
std::unique_ptr<Loop> render (const Settings&, const juce::AudioBuffer<float>& sample, double sampleSr, double bpm, double sr);

class Engine : private juce::Thread
{
public:
    Engine();
    ~Engine() override;
    // ---- ses iş parçacığı
    void prepare (double sampleRate);
    void capture (const juce::AudioBuffer<float>&, int n) noexcept;                       // son 30 sn'yi halkaya yazar
    void process (juce::AudioBuffer<float>&, int n, bool hostPlay, double ppq, double bpm) noexcept;   // döngüyü çıkışa ekler
    // ---- mesaj iş parçacığı
    bool loadFile (const juce::File&);
    bool captureNow();                               // halkadaki en güçlü vuruşu örnek olarak yükler
    Settings get() const;
    void set (const Settings&);                      // değişiklik döngüyü yeniden işletir
    void setTempo (double bpm);
    void setPlaying (bool b) noexcept { playing.store (b); }
    bool isPlaying() const noexcept { return playing.load(); }
    bool hasSample() const;
    juce::String sampleName() const;
    void peaks (std::vector<float>& out, int n) const;                // dalga görünümü için tepe değerleri
    float playhead() const noexcept { return headPos.load(); }        // 0..1
    int version() const noexcept { return doneVersion.load(); }
    bool busy() const noexcept { return wantVersion.load() != doneVersion.load(); }   // işleme sürüyor       // her yeni döngüde artar
    juce::File exportWav();                          // 24 bit WAV (geçici klasör); döngü yoksa boş File
    void collect();                                  // eski döngü tamponlarını siler (zamanlayıcıdan)
    juce::ValueTree toTree() const;
    void fromTree (const juce::ValueTree&);
private:
    void run() override;
    void changed();
    void setSample (juce::AudioBuffer<float>&&, double sr, const juce::String& name, const juce::File& path);
    mutable juce::CriticalSection lock;              // ayarlar ve örnek: mesaj <-> işleme iş parçacığı (ses iş parçacığı almaz)
    Settings s; juce::AudioBuffer<float> sample; double sampleSr = 48000, tempo = 120; juce::String name; juce::File path;
    std::atomic<int> wantVersion { 0 }, doneVersion { 0 };
    std::atomic<Loop*> pending { nullptr }; Loop* live = nullptr; std::array<std::atomic<Loop*>, 8> trash {};
    std::unique_ptr<Loop> last;                      // dışa aktarım için son döngünün kopyası (kilit altında)
    std::atomic<bool> playing { false }; std::atomic<float> headPos { 0 };
    double sr = 48000, freePos = 0;
    juce::AudioBuffer<float> ring; std::atomic<int> ringPos { 0 }; int ringLen = 0;
    juce::AudioFormatManager formats;
};
}
