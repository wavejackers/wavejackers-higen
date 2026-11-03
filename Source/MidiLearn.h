// MIDI Learn: MIDI CC -> eklenti parametresi eşlemesi.
//  - Ses iş parçacığı yalnızca atomik değerler okur/yazar: bellek ayırmaz, kilit almaz, dosyaya dokunmaz.
//  - Değer, parametreye ses iş parçacığında uygulanır (DSP hemen duyar) ve sunucuya setValueNotifyingHost ile bildirilir;
//    sunucu jestleri (begin/endChangeGesture) mesaj iş parçacığındaki zamanlayıcıda açılıp kapanır.
//  - Otomasyon çakışması: "yakalama" (soft takeover). Parametre başka bir yerden (sunucu otomasyonu, fare, preset) değiştiyse
//    CC, parametrenin o anki değerinden geçene kadar etkisizdir; böylece değer zıplamaz ve otomasyonla kavga etmez.
//  - Eşlemeler proje durumunda saklanır (preset dosyalarına girmez); istenirse varsayılan olarak da kaydedilir.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>

namespace hg
{
class MidiLearn
{
public:
    static constexpr int kChannels = 16, kCCs = 128, kSlots = kChannels * kCCs;

    // mesaj iş parçacığı, işlemci kurucusunda bir kez: eşlenebilir parametreleri listeler (sonra liste değişmez)
    void init (juce::AudioProcessor& p);

    // ---- mesaj iş parçacığı
    int indexOf (const juce::String& paramId) const;
    juce::String idOf (int idx) const;
    juce::String nameOf (int idx) const;
    int numParams() const noexcept { return (int) params.size(); }
    void startLearn (int idx) noexcept { learnIdx.store (idx); bump(); }
    void cancelLearn() noexcept { learnIdx.store (-1); bump(); }
    int learning() const noexcept { return learnIdx.load(); }
    bool setMapping (int idx, int channel /*1..16*/, int cc /*0..127*/);
    void removeMapping (int idx);
    void clear();
    struct Map { int param = -1, channel = 0, cc = -1; };
    Map mappingFor (int idx) const;                     // param == -1: eşleme yok
    std::vector<Map> all() const;                       // kanal/CC sırasıyla
    juce::ValueTree toTree() const;                     // <midimap><map param=".." ch=".." cc=".."/></midimap>
    void fromTree (const juce::ValueTree& t);           // bilinmeyen parametreler ve geçersiz değerler atlanır
    void timerTick();                                   // jestleri kapatır/açar (30 Hz)
    int version() const noexcept { return ver.load(); } // değişince arayüz yeniden çizer
    int lastLearned() const noexcept { return learned.load(); }

    // ---- ses iş parçacığı. true: mesaj tüketildi (eşlenmiş CC ya da öğrenme)
    bool handle (const juce::MidiMessage& m) noexcept;

    static juce::File defaultFile();
    bool saveDefault() const;
    bool loadDefault();

private:
    std::vector<juce::RangedAudioParameter*> params;    // init'ten sonra sabit
    std::array<std::atomic<int>, kSlots> slot {};      // (kanal*128 + cc) -> parametre sırası, -1 yok
    struct PState
    {
        std::atomic<bool> picked { false };             // yakalandı mı
        std::atomic<float> lastSet { -1.0f };           // CC'nin en son yazdığı değer (0..1)
        std::atomic<float> lastCc { -1.0f };            // önceki CC değeri (yakalama için)
        std::atomic<int> touchTick { 0 };               // ses iş parçacığı her CC'de artırır
        int seenTick = 0; bool inGesture = false; int idle = 0;   // yalnızca mesaj iş parçacığı
    };
    std::unique_ptr<PState[]> st;
    std::atomic<int> learnIdx { -1 }, ver { 0 }, learned { -1 };
    void bump() noexcept { ver.fetch_add (1); }
    void unmapParam (int idx) noexcept;
};
}
