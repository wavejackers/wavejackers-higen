// Sample kütüphanesi: kullanıcının sample klasörlerini tarar, dosya adından türü (kick, riser, impact...) ve
// BPM'i çıkarır; proje iskeletindeki her ipucu (cue) için uygun bir sample önerir.
// Tarama ve proje dışa aktarımı arka plan iş parçacığında çalışır; sonuçlar mesaj iş parçacığında alınır.
#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "Generator.h"
#include <map>
#include <array>

namespace hg
{
enum SmpCat { kCatKick = 0, kCatClap, kCatSnare, kCatHat, kCatOpenHat, kCatShaker, kCatPerc, kCatRide, kCatCrash,
              kCatRiser, kCatDownlifter, kCatImpact, kCatSubDrop, kCatNoise, kCatFill, kCatVocal, kCatAtmos, kCatOther, kCatMidi, kNumCats };
extern const char* const kCatIds[kNumCats];

struct SampleEntry { juce::File file; int cat = kCatOther; int bpm = 0; bool loop = false; juce::String lname; juce::int64 size = 0; };

// Dosya adından tür, BPM ("124 BPM", "_119_") ve döngü bilgisi. Klasör adı da ipucu olarak kullanılır.
int classifySample (const juce::String& fileName, const juce::String& folderHint, int* bpmOut, bool* loopOut);

// İpucunun ses tarafı: hangi türden sample, tercih edilen anahtar sözcük, sona hizalı mı, ters mi, FX kılavuzuna girer mi
struct CueAudio { int cat = -1, fallback = -1; const char* pref = ""; bool endAligned = false, reversed = false, render = false; };
CueAudio cueAudio (const juce::String& cueType);

struct SampleLib
{
    juce::StringArray folders;
    std::vector<SampleEntry> items;
    int count (int cat) const;
    // Belirlenimci seçim: aynı tohum, ölçü ve tür her zaman aynı dosyayı verir
    const SampleEntry* pick (const juce::String& cueType, int bpm, juce::uint32 seed, int bar) const;
    const SampleEntry* pickCat (int cat, const char* pref, int bpm, juce::uint32 seed, int salt, bool oneShot) const;
    static SampleLib scan (const juce::StringArray& folders, const std::function<bool()>& shouldExit, std::atomic<int>* progress, int maxFiles = 40000);
};

// Proje klasörü: tam parça MIDI'si (işaretçilerle), iz başına tam boy MIDI, FX kılavuz WAV'ı, önerilen sample'ların
// kopyaları ve okunur bir plan dosyası. Ağır iş: arka plan iş parçacığında çağrılır.
struct ExportJobInput
{
    juce::File dir;                         // oluşturulacak klasör
    juce::String baseName, planText;        // plan metni çağıran tarafta (dil seçimine göre) hazırlanır
    Skeleton sk;                            // song + secs + cues
    std::vector<juce::MemoryBlock> midis;   // [tam parça, akor, bas, davul, melodi]
    std::vector<std::pair<SkCue, juce::File>> picks;   // ipucu başına seçilen sample (boş dosya = HitGen sesi)
    int bpm = 120; juce::uint32 seed = 7; double sampleRate = 44100;
    // FL Studio projesi için: HitGen'in kendi sesleri (davul vuruşları GM notasına göre, akor/bas/melodi C5'te tek nota)
    // ve kütüphaneden önerilen davul one-shot'ları (varsa render yerine kullanılır)
    bool writeFlp = true;
    std::map<juce::String, juce::AudioBuffer<float>> laneRenders;   // şerit rolü -> HitGen sesi (melodik: C5'te tek nota)
    std::map<juce::String, juce::File> laneFiles;                   // şerit rolü -> kütüphaneden önerilen one-shot (davullar)
    double renderRate = 44100;
    // HitGen 3.0: tam şarkı stem'leri (iz başına, diğer izler sessiz; pump ve efektler karışımdakiyle aynı) ve DAW köprüleri
    bool writeStems = true, writeBridges = true;
    std::shared_ptr<class Engine> stemEngine;      // mesaj iş parçacığında kurulur (yamalar, kit), işte yalnızca bu iş kullanır
};
struct ExportResult { bool ok = false; juce::File dir, guide, flp, midi, manifest; juce::String error; int copied = 0, placed = 0, flpClips = 0, flpPatterns = 0, stems = 0, scripts = 0; };
bool writeFlpProject (const ExportJobInput& in, ExportResult& R, juce::String& err);
ExportResult exportProject (const ExportJobInput& in, const std::function<bool()>& shouldExit, std::atomic<int>* progress);

// Tam şarkı stem'leri: Stems/NN <iz>.wav (24 bit). files'a göreli yolları ekler.
bool renderStems (const struct Arrangement& a, Engine& E, double sr, const juce::File& dir, const std::function<bool()>& shouldExit,
                  std::atomic<int>* progress, int progressFrom, int progressTo, juce::StringPairArray& files, int* countOut);

// FX kılavuzu: ipuçlarına göre riser/impact/downlifter... yerleştirilmiş tam boy stereo ses (sample yoksa sentez)
bool renderFxGuide (const juce::File& out, const Skeleton& sk, const std::vector<std::pair<SkCue, juce::File>>& picks,
                    int bpm, double sr, juce::uint32 seed, const std::function<bool()>& shouldExit, std::atomic<int>* progress, int* placedOut);

// Tam parçanın [bar, bar+bars) dilimi (bölüm MIDI'si)
Result sliceResult (const Result& song, int bar, int bars);
}
