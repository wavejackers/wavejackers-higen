#include "MidiLearn.h"

namespace hg
{
// Kanal modu mesajları (120-127: all notes off, reset...) eşlenmez; bunlar notalar için ayrılmıştır.
static constexpr int kMaxCc = 119;

void MidiLearn::init (juce::AudioProcessor& p)
{
    params.clear();
    for (auto* ap : p.getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (ap)) params.push_back (r);
    st = std::make_unique<PState[]> (params.size());
    for (auto& s : slot) s.store (-1);
}

int MidiLearn::indexOf (const juce::String& id) const
{
    for (size_t i = 0; i < params.size(); ++i) if (params[i]->getParameterID() == id) return (int) i;
    return -1;
}
juce::String MidiLearn::idOf (int idx) const { return idx >= 0 && idx < (int) params.size() ? params[(size_t) idx]->getParameterID() : juce::String(); }
juce::String MidiLearn::nameOf (int idx) const { return idx >= 0 && idx < (int) params.size() ? params[(size_t) idx]->getName (64) : juce::String(); }

void MidiLearn::unmapParam (int idx) noexcept { for (auto& s : slot) { int v = idx; s.compare_exchange_strong (v, -1); } }

bool MidiLearn::setMapping (int idx, int channel, int cc)
{
    if (idx < 0 || idx >= (int) params.size() || channel < 1 || channel > kChannels || cc < 0 || cc > kMaxCc) return false;
    unmapParam (idx);
    slot[(size_t) ((channel - 1) * kCCs + cc)].store (idx);         // bu CC'yi kullanan başka parametre varsa eşlemesini kaybeder
    st[(size_t) idx].picked.store (false); st[(size_t) idx].lastCc.store (-1.0f); st[(size_t) idx].lastSet.store (-1.0f);
    bump(); return true;
}

void MidiLearn::removeMapping (int idx) { if (idx < 0 || idx >= (int) params.size()) return; unmapParam (idx); bump(); }
void MidiLearn::clear() { for (auto& s : slot) s.store (-1); learnIdx.store (-1); bump(); }

MidiLearn::Map MidiLearn::mappingFor (int idx) const
{
    for (int k = 0; k < kSlots; ++k) if (slot[(size_t) k].load() == idx && idx >= 0) return { idx, k / kCCs + 1, k % kCCs };
    return {};
}

std::vector<MidiLearn::Map> MidiLearn::all() const
{
    std::vector<Map> v;
    for (int k = 0; k < kSlots; ++k) { const int i = slot[(size_t) k].load(); if (i >= 0) v.push_back ({ i, k / kCCs + 1, k % kCCs }); }
    return v;
}

juce::ValueTree MidiLearn::toTree() const
{
    juce::ValueTree t ("midimap");
    for (auto& m : all())
    {
        juce::ValueTree c ("map"); c.setProperty ("param", idOf (m.param), nullptr); c.setProperty ("ch", m.channel, nullptr); c.setProperty ("cc", m.cc, nullptr);
        t.appendChild (c, nullptr);
    }
    return t;
}

void MidiLearn::fromTree (const juce::ValueTree& t)
{
    for (auto& s : slot) s.store (-1);
    if (t.isValid())
        for (int i = 0; i < t.getNumChildren() && i < kSlots; ++i)
        {
            const auto c = t.getChild (i); if (! c.hasType ("map")) continue;
            const int idx = indexOf (c.getProperty ("param").toString());
            if (idx >= 0) setMapping (idx, (int) c.getProperty ("ch", 0), (int) c.getProperty ("cc", -1));
        }
    bump();
}

bool MidiLearn::handle (const juce::MidiMessage& m) noexcept
{
    if (! m.isController() || params.empty()) return false;
    const int ch = m.getChannel() - 1, cc = m.getControllerNumber();
    if (ch < 0 || ch >= kChannels || cc < 0 || cc > kMaxCc) return false;
    const int key = ch * kCCs + cc;
    int li = learnIdx.load();
    if (li >= 0 && li < (int) params.size())
    {
        if (learnIdx.compare_exchange_strong (li, -1))
        {
            unmapParam (li); slot[(size_t) key].store (li);
            auto& s = st[(size_t) li]; s.picked.store (false); s.lastCc.store (-1.0f); s.lastSet.store (-1.0f);
            learned.store (li); bump();
        }
        return true;
    }
    const int idx = slot[(size_t) key].load();
    if (idx < 0 || idx >= (int) params.size()) return false;
    auto* p = params[(size_t) idx]; auto& s = st[(size_t) idx];
    const float v = (float) m.getControllerValue() / 127.0f, cur = p->getValue();
    // kademeli parametrelerde (seçim, tamsayı) bir adım kadar fark doğaldır
    const int steps = p->getNumSteps(); const float tol = steps > 1 && steps < 100000 ? 1.0f / (float) (steps - 1) + 0.001f : 0.01f;
    const float ls = s.lastSet.load();
    if (s.picked.load() && ls >= 0.0f && std::abs (cur - ls) > tol) s.picked.store (false);   // başkası (otomasyon, fare, preset) değiştirdi
    if (! s.picked.load())
    {
        const float prev = s.lastCc.load();
        const bool near = std::abs (v - cur) <= juce::jmax (0.02f, tol);
        const bool crossed = prev >= 0.0f && (prev - cur) * (v - cur) <= 0.0f;
        if (near || crossed) s.picked.store (true);
    }
    s.lastCc.store (v);
    if (s.picked.load())
    {
        p->setValueNotifyingHost (v);
        s.lastSet.store (p->getValue());
    }
    s.touchTick.fetch_add (1);
    return true;
}

void MidiLearn::timerTick()
{
    for (size_t i = 0; i < params.size(); ++i)
    {
        auto& s = st[i]; const int t = s.touchTick.load();
        if (t != s.seenTick)
        {
            s.seenTick = t; s.idle = 0;
            if (! s.inGesture && s.picked.load()) { params[i]->beginChangeGesture(); s.inGesture = true; }
        }
        else if (s.inGesture && ++s.idle >= 9) { params[i]->endChangeGesture(); s.inGesture = false; }   // ~300 ms sessizlik
    }
}

juce::File MidiLearn::defaultFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Wavejackers").getChildFile ("HitGen").getChildFile ("midi-map.xml");
}

bool MidiLearn::saveDefault() const
{
    auto f = defaultFile(); f.getParentDirectory().createDirectory();
    if (auto xml = toTree().createXml()) return xml->writeTo (f);
    return false;
}

bool MidiLearn::loadDefault()
{
    const auto f = defaultFile();
    if (! f.existsAsFile() || f.getSize() > 200000) return false;
    if (auto xml = juce::XmlDocument::parse (f)) { const auto t = juce::ValueTree::fromXml (*xml); if (t.hasType ("midimap")) { fromTree (t); return true; } }
    return false;
}
}
