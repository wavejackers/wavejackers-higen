// Referans sesin yaşam döngüsü: arka planda çözme/analiz, orijinali çalma (gerçek zamanlı güvenli), DNA çıkarımı.
#pragma once
#include "RefAnalysis.h"
#include <array>

namespace hg
{
class RefEngine
{
public:
    enum State { Idle, Loading, Loaded, Analysing, Ready, Failed };
    RefEngine();
    ~RefEngine();

    // ---- mesaj iş parçacığı
    void load (const juce::File& f);                          // arka planda çözer; bitince onChange (mesaj iş parçacığında)
    void analyse();                                           // SMART DETECT: arka planda analiz eder
    void cancel();                                            // süren yüklemeyi/analizi iptal eder
    void clear();
    void extractDna (double t0, double t1, int role, std::function<void (bool, const TrackDna&)> done);   // arka planda; done mesaj iş parçacığında
    std::shared_ptr<const RefClip> clip() const { return clipMsg; }
    std::shared_ptr<const RefAnalysis> analysis() const { return analysisMsg; }
    juce::String error;                                       // çeviri anahtarı ("fileBad", "fileSilent", ...)
    std::function<void()> onChange;                           // durum/sonuç değişti
    std::atomic<int> state { Idle };
    std::atomic<float> progress { 0 };

    // ---- çalma denetimleri (herhangi bir iş parçacığından)
    std::atomic<bool> playing { false }, loop { true };
    std::atomic<float> gain { 0.6f };
    std::atomic<double> regionStart { 0 }, regionEnd { 0 };   // saniye; end <= start ise tüm dosya
    std::atomic<double> seekTo { -1 };                        // >= 0 ise ses iş parçacığı bu konuma atlar
    std::atomic<double> posSec { 0 };                         // arayüz için okuma konumu

    // ---- ses iş parçacığı: çıkışa EKLER (bellek ayırmaz, dosya okumaz)
    void render (juce::AudioBuffer<float>& out, double hostSr) noexcept;

    // ---- MIDI ile çalınan örnekleyici (seçilen bölge). Not: klasik yeniden örnekleme; perde değişince SÜRE de değişir.
    //      Süreyi koruyan perde kaydırma (time-stretch) YOKTUR; bunu arayüz de belirtir.
    struct SamplerParams
    {
        std::atomic<bool> enabled { false }, loop { false };
        std::atomic<int> root { 60 }, transpose { 0 }, fineCents { 0 }, maxVoices { 8 };
        std::atomic<float> a { 0.005f }, d { 0.25f }, s { 0.8f }, r { 0.3f }, gain { 0.8f }, velSens { 0.7f };
        std::atomic<double> trimStart { 0 }, trimEnd { 0 }, loopStart { 0 }, loopEnd { 0 };   // saniye (klip zamanı); trimEnd <= trimStart ise tüm klip
    } sampler;
    // ses iş parçacığı: delay = blok içindeki örnek ofseti
    void samplerNoteOn (int note, int vel, int delay) noexcept;
    void samplerNoteOff (int note) noexcept;
    void samplerAllOff() noexcept;
    void renderSampler (juce::AudioBuffer<float>& out, double hostSr) noexcept;
    juce::var samplerToVar() const;
    // DAW durumu: kaydedilecek / geri yüklenecek referans bilgisi (yol + örnekleyici ayarları). Dosya yoksa missingPath dolar (kurtarma: kullanıcı yeniden bulur).
    juce::var stateToVar() const;
    void restoreFromVar (const juce::var& v);                 // herhangi bir iş parçacığından; yükleme mesaj iş parçacığında başlar
    juce::String missingPath;                                 // yalnızca mesaj iş parçacığı
    bool hasPath() const { const juce::ScopedLock sl (pathLock); return curPath.isNotEmpty(); }
    void samplerFromVar (const juce::var& v);

private:
    void post (std::function<void()> f);
    void setClip (std::shared_ptr<RefClip> c);
    mutable juce::CriticalSection pathLock; juce::String curPath, curName;   // DAW durum kaydı herhangi bir iş parçacığından okuyabilir
    void setPath (const juce::String& p, const juce::String& n) { const juce::ScopedLock sl (pathLock); curPath = p; curName = n; }
    juce::ThreadPool pool { 1 };
    std::atomic<bool> stopFlag { false };
    std::shared_ptr<std::atomic<bool>> alive;
    std::shared_ptr<const RefClip> clipMsg;          // yalnızca mesaj iş parçacığı
    std::shared_ptr<const RefAnalysis> analysisMsg;
    juce::SpinLock lock; std::shared_ptr<const RefClip> clipAudio; const RefClip* rawAudio = nullptr;   // lock ile korunur
    double rpos = 0;                                 // yalnızca ses iş parçacığı (örnek cinsinden)
    struct Voice { bool on = false, held = false; int note = 0, delay = 0; double pos = 0, inc = 1; float vel = 1, env = 0; int stage = 0; unsigned long long age = 0; };
    static constexpr int kMaxVoices = 16;
    std::array<Voice, kMaxVoices> voices;            // yalnızca ses iş parçacığı
    unsigned long long voiceClock = 0; const RefClip* voiceClip = nullptr;
};
}
