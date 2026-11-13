#pragma once
#include "IntroSound.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include "Data.h"
#include "Generator.h"
#include "Synth.h"
#include "SoundParams.h"
#include "Presets.h"
#include "Import.h"
#include "RefEngine.h"
#include "LoopForge.h"
#include "MidiLearn.h"
#include "SampleLib.h"
#include "CoProducer.h"
#include "Demo.h"
#include <mutex>
#include <optional>

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
    // Kütüphaneden sürpriz ses: stilin türüne ve izin rolüne uyan Factory 26 seslerinden biri (şu ankinden farklı).
    // Yeni fikirde elle seçilmemiş izler için kullanılır; ses kutusundaki "Sürpriz ses" de bunu çağırır.
    juce::String librarySoundFor (int track, juce::Random& rnd) const;
    void surpriseSound (int track);
    void suggestAlternative (int track);   // yalnızca o izin MIDI'sini başka bir fikirle değiştir
    juce::String lang = "en";
    lf::Engine loop;                       // LoopForge: tek vuruştan dikişsiz döngü (kendi çıkışını da yakalar)
    hg::RefEngine ref;                     // referans ses: yükleme, SMART DETECT, orijinali çalma
    hg::MidiLearn midiLearn;               // MIDI CC -> parametre (sağ tık > MIDI Learn)
    float uiScale = 1.0f; bool highContrast = false; bool animations = true;
    hg::IntroSound intro;                  // açılış sesi (DAW çalarken ve dışa aktarımda sessiz)
    std::function<void()> onResultChanged;
    juce::File writeMidi (int which);
    // ---- ses tasarımı (part: 0 akor, 1 bas, 2 melodi)
    void resetSoundDesign (int part);          // seçili sesin ilk haline dön
    void randomizeSound (int part);            // role uygun rastgele ses
    juce::var currentSoundVar (int part) const;   // parametrelerden kurulan güncel ses
    void applySoundParams (int part, const float* vals);     // Benzerini üret / geçmiş: değerleri yaz ve yamayı hemen kur
    void readSoundParams (int part, float* vals) const;
    void applyRefToProject (const hg::RefAnalysis& a);       // tespit edilen tempo, ton ve makamı üretici ayarlarına uygula
    juce::MidiKeyboardState keyboardState;     // ekran klavyesi
    bool wasSamp = false;                      // yalnızca ses iş parçacığı
    std::atomic<int> auditionPart { -1 };      // ses tasarımı açıkken klavye bu izi çalar (-1: ayara göre)
    // ses ön dinlemesi: seçili sesin kategorisine uygun 2 ölçülük cümle (part: 0 akor, 1 bas, 2 melodi)
    void playDemo (int part);
    void stopDemo() { demoReq.store (-2); }
    bool demoPlaying() const noexcept { return demoOn.load(); }
    // ---- sanatçı presetleri
    hg::PresetStore presets;
    juce::String userArtist, userPreset;       // yüklü/kaydedilmiş presetin sanatçı ve adı
    bool savePreset (const juce::String& artist, const juce::String& name, juce::String& fileOut);
    bool loadPreset (const juce::File& f);
    // FXP/FXB/Serum 2 dosyasından ses al; part: 0 akor, 1 bas, 2 melodi, -1 = dosya adından tahmin et. Çeviri anahtarı döner.
    juce::String importSynthFile (const juce::File& f, int part, hg::ImportedSound* info = nullptr);
    juce::ValueTree stateTree();
    void idle() { timerCallback(); }   // testler: mesaj iş parçacığı işlerini hemen yap   // 0..3 tek iz, 4 tüm izler (döngü), 5 tam parça; geçici klasöre yazar
    juce::MemoryBlock midiBytes (int which) const;
    juce::String baseName() const;

    // ---- davul makinesi (2.7): ped örnekleri, kitler
    static constexpr int kDmCtl = 7;                           // level, tune, decay, punch, soft, mid, high
    static juce::String dmParamId (int pad, int k);
    static bool isDrumSampleFile (const juce::File& f);
    bool loadDrumSample (int pad, const juce::File& f, juce::String& errKey, bool enableCreative = true);
    void clearDrumSample (int pad);
    void resetDrumPad (int pad);
    // kullanıcının kendi sesi (akor/bas/melodi izine, part: 0 akor, 1 bas, 2 melodi): ses dosyası örnekleyicide çalar
    bool loadInstSample (int part, const juce::File& f, juce::String& err, int* rootOut = nullptr);
    static std::unique_ptr<hg::InstSample> makeInstSample (const juce::File& f, juce::String& err);   // ses dosyası + kök nota
    void clearInstSample (int part);
    juce::String instSamplePath (int part) const { const juce::ScopedLock sl (drumLock); return instSamples[(size_t) juce::jlimit (0, 2, part)]; }
    // MIDI düzenleme: düzenlenmiş sonucu çalmaya ve dışa aktarmaya uygula (ileti iş parçacığı)
    void applyEditedResult (std::shared_ptr<hg::Result> r);
    bool hasEdits() const noexcept { return editActive; }
    void revertEdits();
    juce::String drumSamplePath (int pad) const { const juce::ScopedLock sl (drumLock); return drumSamples[(size_t) juce::jlimit (0, hg::kNumPads - 1, pad)]; }
    static juce::File drumKitFolder();
    static juce::Array<juce::File> drumKitFiles();
    bool saveDrumKit (const juce::String& name, juce::File& written);
    bool loadDrumKit (const juce::File& f, juce::StringArray& missing);
    juce::StringArray missingSamples;                          // proje yüklenirken bulunamayan örnekler (arayüz bildirir)

    // ---- proje iskeleti (Proje sekmesi): yapı, hedef süre, bölüm başına MIDI ve sample önerileri
    struct SkelState { juce::StringArray kinds; std::vector<int> bars; int durSec = 360; int structIdx = 0; bool custom = false; bool active = false; };
    SkelState skel;
    static constexpr int kNumSkStructs = 5;                    // 0..3 hazır yapılar (web: HG.SK_STRUCT), 4 = stilin kendi yapısı
    static const int kSkDurations[8];                          // 0 = stil uzunluğu, sonra 4:00..10:00
    juce::StringArray skStructKinds (int idx) const;
    void skSetStructure (int idx);
    void skSetDuration (int sec);
    void skSetKinds (const juce::StringArray& kinds, bool keepBars = false);   // bölüm ekle/sil/taşı/tür değiştir
    void skSetBars (int section, int bars);                    // elle uzunluk: "özel" süre
    hg::Skeleton skeletonNow() const;                          // güncel fikir + iskelet ayarları (ileti iş parçacığı)
    hg::Skeleton skeletonFor (const hg::Result& loop) const;
    hg::Result fullSong() const;                               // iskelet etkinse onun aranjmanı (ipucu işaretçileriyle), değilse stilin
    std::vector<std::pair<hg::SkCue, juce::File>> skPicks (const hg::Skeleton& sk) const;   // ipucu başına önerilen sample (boş = HitGen sesi)
    juce::MemoryBlock sectionMidi (int section) const;
    // ---- sample kütüphanesi (klasörler tüm projeler için ortak saklanır)
    std::shared_ptr<const hg::SampleLib> library() const { return lib; }
    juce::StringArray sampleFolders() const { return folders; }
    void setSampleFolders (const juce::StringArray& f, bool persist = true);   // taramayı arka planda başlatır
    void ensureLibrary();                                      // ilk açılışta kayıtlı klasörleri tara
    bool scanning() const { return scanBusy.load(); }
    bool pollLibrary();                                        // ileti iş parçacığı: biten taramayı al (değiştiyse true)
    std::atomic<int> scanProgress { 0 };
    int libRevision = 0;
    int loadSuggestedDrums();                                  // kütüphaneden kick, clap, hat... Davul Makinesi pad'lerine
    // ---- proje klasörü dışa aktarımı (arka planda)
    bool startExport (const juce::File& dir, const juce::String& planText);
    bool exportStems = true;                                   // tam şarkı stem'leri (testlerin hızlı turunda kapatılır)
    bool exporting() const { return exportBusy.load(); }
    std::atomic<int> exportProgress { 0 };
    bool pollExport (hg::ExportResult& out);
    // ---- Co-Producer: kullanıcının MIDI'si kilitli iz olarak (yeniden üretimde korunur)
    void coLock (const hg::UserMidi& um, const hg::CoCandidate& c);
    void coUnlock();
    int coLockedRole() const noexcept { return coRole; }
    juce::String coLockedName() const { return coName; }
    const std::vector<hg::NoteEv>& coLockedNotes() const noexcept { return coNotes; }
    hg::GenParams generatorParams() const { return currentParams(); }
    static juce::File projectsFolder();
    void renderExportSounds (hg::ExportJobInput& in);       // FL projesi için HitGen sesleri (ayrı motor örneği)
    static juce::File sampleFoldersFile();

    std::atomic<bool> previewPlaying { false };
    std::atomic<double> playPpq { 0 }, hostBpm { 0 };
    std::atomic<bool> hostPlaying { false };
    hg::Engine engine;
    static juce::StringArray styleNames (const hg::Data& d);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout (const hg::Data& d);
    void parameterChanged (const juce::String& id, float) override;
    void timerCallback() override;
    void loadPatch (int track, bool live = false);
    void applySoundToParams (int part);
    void readSd (int part, float* out) const;
    void writeSd (int part, const float* vals);
    void restoreFromTree (const juce::ValueTree& tree, bool includeUi);
    std::array<std::array<std::atomic<float>*, hg::kNumSp>, 3> sdPtr {};
    std::array<std::atomic<bool>, 3> sdDirty {};
    bool suppressSd = false, loadingPreset = false;
    std::array<std::unique_ptr<hg::Patch>, 3> tableCache;
    juce::MidiBuffer kbBuf;
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
    hg::DemoTable demoTab; std::atomic<int> demoReq { -1 }; std::atomic<bool> demoOn { false };
    bool demoRun = false; int demoKind = 0, demoTrack = 0; double demoBeat = 0;   // yalnızca ses iş parçacığı

    std::atomic<float>* p (const char* id) const { return apvts.getRawParameterValue (id); }
    // ses iş parçacığı için önceden çözülmüş parametre işaretçileri (String ayırmadan okunur)
    struct PartPtrs { std::atomic<float>* vol = nullptr, *mute = nullptr, *cut = nullptr, *res = nullptr, *atk = nullptr, *rel = nullptr, *drv = nullptr, *rev = nullptr, *dly = nullptr, *mov = nullptr; };
    std::array<PartPtrs, 4> pp {};
    std::array<std::array<std::atomic<float>*, kDmCtl>, hg::kNumPads> dmPtr {};
    std::array<juce::String, 3> instSamples;
    bool editActive = false; juce::String editKey;
    std::unique_ptr<std::array<std::vector<hg::NoteEv>, hg::kNumTracks>> savedEdit; juce::String savedEditKey;   // projeden gelen düzenleme   // düzenleme hangi üretim ayarlarına ait (tempo hariç)
    juce::String paramsKey() const;
    std::array<juce::String, hg::kNumPads> drumSamples;      // arayüz ve durum yükleme (sunucu başka iş parçacığından çağırabilir); ses iş parçacığı dokunmaz
    juce::CriticalSection drumLock;
    std::atomic<float>* pLoud = nullptr, *pCreative = nullptr;
    std::atomic<float>* pQuality = nullptr, *pThick = nullptr, *pPump = nullptr, *pMaster = nullptr, *pBpm = nullptr, *pPattern = nullptr, *pMidiOut = nullptr, *pKeysTo = nullptr;
    int coRole = -1, coBars = 8; std::vector<hg::NoteEv> coNotes; juce::String coName;
    // iskelet arka plan işleri
    juce::StringArray folders; bool libRequested = false;
    std::shared_ptr<const hg::SampleLib> lib;
    std::mutex jobMx; std::optional<hg::SampleLib> doneLib; std::optional<hg::ExportResult> doneExport;
    std::atomic<bool> scanBusy { false }, exportBusy { false }, rescan { false };
    juce::ThreadPool pool { juce::ThreadPoolOptions{}.withThreadName ("HitGen jobs").withNumberOfThreads (1) };   // en son üye: önce o yok edilir, işler biter
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HitGenProcessor)
};
