// İş akışı yardımcıları (2.5): vuruş ızgarasına hizalama, DNA etiketleri, ritim -> MIDI taslağı, klasörü toplu analiz.
// Hepsi gerçek analiz verisinden çalışır; tahmin olan yerler açıkça belirtilir.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>
#include "RefAnalysis.h"

namespace hg
{
// Seçimi algılanan BPM'in bar ızgarasına (4 vuruş) hizalar. Izgaranın başlangıcı = ilk onset (indirme vuruşu bilinmediği için yaklaşık).
// En az bir bar uzunluk; dosya sınırlarına kırpılır. BPM yoksa false döner ve çıktıya dokunmaz.
bool snapToBars (double a, double b, double bpm, double anchor, double duration, double& outA, double& outB);

// DNA'dan çıkarılan etiket anahtarları (ör. "tagDark", "tagPluck"). Ölçülen özelliklerin basit eşikleri; öznel bir değerlendirme değildir.
juce::StringArray dnaTags (const TrackDna& d);

// Bölgedeki onset'leri tek ölçülük bir davul MIDI taslağına çevirir (kanal 10): düşük bant ağırlıklı -> 36 (kick),
// yüksek bant ağırlıklı -> 42 (hi-hat), diğerleri -> 38 (snare). Hız, onset'teki gerçek tepe seviyesinden gelir.
// BPM varsa 1/16 ızgarasına oturtulur. Bu bir TASLAKTIR: tam miks üzerinde kaba bir bant sınıflaması yapılır.
struct RhythmSketch { juce::MidiFile midi; int notes = 0, kicks = 0, snares = 0, hats = 0; double bpm = 120; };
bool onsetsToMidi (const RefClip& clip, const RefAnalysis& an, double t0, double t1, RhythmSketch& out);

// Tek dosya: çöz, analiz et, her kategorinin en iyi bölgesinden (güven >= minConf) DNA çıkar. Dönen DNA'ların adı "<dosya> <rol>".
int analyseFileToDna (const juce::File& f, std::vector<TrackDna>& out, juce::String& error, const std::atomic<bool>* cancel = nullptr, float minConf = 0.25f);

// Klasörü arka planda işler (en çok maxFiles ses dosyası). Her dosyanın sonucu mesaj iş parçacığına iletilir.
class BatchAnalyser : private juce::Thread
{
public:
    BatchAnalyser() : juce::Thread ("HitGen batch") {}
    ~BatchAnalyser() override;
    static juce::Array<juce::File> audioFilesIn (const juce::File& folder, int maxFiles = 60);
    // onFile(index, total, dosya, DNA'lar, hata) ve onDone(iptal edildi mi) mesaj iş parçacığında çağrılır.
    std::function<void (int, int, const juce::File&, const std::vector<TrackDna>&, const juce::String&)> onFile;
    std::function<void (bool)> onDone;
    bool start (const juce::Array<juce::File>& files);
    void cancel() { stop = true; stopThread (8000); }
    bool running() const { return isThreadRunning(); }

private:
    juce::Array<juce::File> list; std::atomic<bool> stop { false };
    std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>> (true);
    void run() override;
};
}
