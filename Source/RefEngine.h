// Referans sesin yaşam döngüsü: arka planda çözme/analiz, orijinali çalma (gerçek zamanlı güvenli), DNA çıkarımı.
#pragma once
#include "RefAnalysis.h"

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

private:
    void post (std::function<void()> f);
    void setClip (std::shared_ptr<RefClip> c);
    juce::ThreadPool pool { 1 };
    std::atomic<bool> stopFlag { false };
    std::shared_ptr<std::atomic<bool>> alive;
    std::shared_ptr<const RefClip> clipMsg;          // yalnızca mesaj iş parçacığı
    std::shared_ptr<const RefAnalysis> analysisMsg;
    juce::SpinLock lock; std::shared_ptr<const RefClip> clipAudio; const RefClip* rawAudio = nullptr;   // lock ile korunur
    double rpos = 0;                                 // yalnızca ses iş parçacığı (örnek cinsinden)
};
}
