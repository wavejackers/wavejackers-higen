// Co-Producer: kullanıcının MIDI'sini (melodi, akor ya da bas) okur, tonunu bulur (Krumhansl-Kessler, süre ağırlıklı),
// HitGen'in fikirleri arasından kullanıcının notalarına en uyan akor yolunu arar ve eksik izleri tamamlar.
// Kullanıcının notaları kilitlidir: sonuç izine olduğu gibi konur, yeniden üretimde de korunur.
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include "Generator.h"

namespace hg
{
class Data;

struct UserMidi
{
    juce::String name;
    std::vector<NoteEv> notes;   // 16'lık adım cinsinden, 0'dan başlar
    int bars = 8;                // 4, 8 ya da 16 (HitGen döngü uzunlukları)
    double bpm = 0;              // dosyada tempo varsa
    int role = kMelody;          // tahmin: kMelody, kChords ya da kBass
};
bool readUserMidi (const juce::File& f, UserMidi& out, juce::String& errKey);
bool userMidiFromFile (const juce::MidiFile& mf, UserMidi& out, juce::String& errKey);
int guessRole (const std::vector<NoteEv>& notes);

struct KeyGuess { int key = 9; bool minor = true; double score = 0, margin = 0, coverage = 0; };   // key: 0 = C
// Sıralama: önce notaların ne kadarı makamın içinde (kapsam), eşitlikte Krumhansl-Kessler korelasyonu.
// Kısa döngülerde göreli majör/minör ve beşli komşu tonlar ayırt edilemeyebilir; bu yüzden aday listesi döner.
std::vector<KeyGuess> detectKeys (const std::vector<NoteEv>& notes);
KeyGuess detectKey (const std::vector<NoteEv>& notes);
std::vector<KeyGuess> keyCandidates (const std::vector<NoteEv>& notes, int maxCount = 4);   // kapsamı en iyiye 0.03 yakın olanlar

// Uyum (0..1): melodi -> akor tonu oranı (vuruş başları ağır), akor -> perde sınıfı örtüşmesi, bas -> kök/akor tonu
double chordFit (const Result& r, const std::vector<NoteEv>& user, int role);

struct CoCandidate { GenParams P; double fit = 0; };
// Ton ve makam sabit; tohum ve varyasyon taranır. En iyi 'top' aday, uyuma göre sıralı.
// keys boşsa base.key/base.scale kullanılır; doluysa her aday ton denenir ve uyum karar verir.
std::vector<CoCandidate> coSearch (const Data& data, const GenParams& base, const UserMidi& um, int seeds = 12, int top = 5, const std::vector<KeyGuess>& keys = {});
// Kilitli notalar: kullanıcının izi verilen uzunluğa sığdırılır (kısa döngü tekrarlanır, uzunsa kırpılır)
std::vector<NoteEv> fitLocked (const std::vector<NoteEv>& notes, int srcBars, int dstBars);
Result coApply (const Data& data, const CoCandidate& c, const UserMidi& um);
}
