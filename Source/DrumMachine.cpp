#include "DrumMachine.h"
#include <cmath>

namespace hg
{
const char* const kPadIds[kNumPads] = { "kick", "snare", "clap", "hat", "ohat", "rim", "perc" };
const int kPadNotes[kNumPads] = { 36, 38, 39, 42, 46, 37, 64 };

int padForNote (int note) noexcept
{
    switch (note)
    {
        case 35: case 36: return kPadKick;
        case 38: case 40: return kPadSnare;
        case 39: return kPadClap;
        case 42: case 44: return kPadHat;
        case 46: return kPadOpenHat;
        case 37: return kPadRim;
        default: return kPadPerc;   // tom, konga ve diğerleri perküsyon pedine
    }
}

namespace
{
constexpr double kTwoPi = 6.283185307179586;
DrumSample clearSentinel;   // "örneği kaldır" isteği
inline float frand (uint32_t& s) noexcept { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 8388608.0f - 1.0f; }
inline float fastTanh (float x) noexcept { x = juce::jlimit (-4.5f, 4.5f, x); const float x2 = x * x; return x * (27.0f + x2) / (27.0f + 9.0f * x2); }
inline float sat (float x, float k) noexcept { const float a = 1.0f + k * 4.0f; return fastTanh (x * a) / fastTanh (a); }
inline void svfCoef (float* c, double fc, double q, double sr) noexcept
{
    const double g = std::tan (juce::MathConstants<double>::pi * juce::jlimit (20.0, sr * 0.49, fc) / sr), k = 1.0 / juce::jmax (0.05, q);
    const double a1 = 1.0 / (1.0 + g * (g + k));
    c[0] = (float) a1; c[1] = (float) (g * a1); c[2] = (float) (g * g * a1); c[3] = (float) k;
}
struct SvfOut { float lp, bp, hp; };
inline SvfOut svf (float x, const float* c, float* s) noexcept
{
    const float v3 = x - s[1], v1 = c[0] * s[0] + c[1] * v3, v2 = s[1] + c[1] * s[0] + c[2] * v3;
    s[0] = 2 * v1 - s[0]; s[1] = 2 * v2 - s[1];
    return { v2, v1, x - c[3] * v1 - v2 };
}
inline float expd (double t, double tau) noexcept { return (float) std::exp (-t / juce::jmax (1e-4, tau)); }
// ped başına EQ bölgeleri: Mid (tepe) ve High (raf) frekansları
const float kMidHz[kNumPads] = { 350, 900, 1200, 3000, 3000, 1500, 800 };
const float kHighHz[kNumPads] = { 4000, 6000, 6000, 9000, 9000, 6000, 5000 };
// 808 zil osilatörleri (Hz); hat'ler için oktav üstü kullanılır
const float kMetal[6] = { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
}

DrumMachine::DrumMachine()
{
    for (auto& p : pending) p.store (nullptr);
    for (auto& v : auditionVel) v.store (110);
}

DrumMachine::~DrumMachine()
{
    for (auto& p : pending) { auto* x = p.exchange (nullptr); if (x != nullptr && x != &clearSentinel) delete x; }
    for (auto*& c : cur) { delete c; c = nullptr; }
    collectGarbage();
}

void DrumMachine::prepare (double sampleRate) { sr = sampleRate; reset(); }
void DrumMachine::reset() { for (auto& v : voices) v.active = false; }

void DrumMachine::setSample (int pad, std::unique_ptr<DrumSample> s)
{
    if (pad < 0 || pad >= kNumPads) return;
    DrumSample* raw = s != nullptr ? s.release() : &clearSentinel;
    auto* old = pending[(size_t) pad].exchange (raw);
    if (old != nullptr && old != &clearSentinel) delete old;     // ses iş parçacığı henüz almamıştı
}

void DrumMachine::retire (const DrumSample* s) noexcept
{
    if (s == nullptr) return;
    for (auto& v : voices) if (v.active && v.smp == s) v.active = false;
    int s1, n1, s2, n2; trashFifo.prepareToWrite (1, s1, n1, s2, n2);
    if (n1 > 0) trash[(size_t) s1] = const_cast<DrumSample*> (s);
    trashFifo.finishedWrite (n1);
}

void DrumMachine::collectGarbage()
{
    int s1, n1, s2, n2; trashFifo.prepareToRead (trashFifo.getNumReady(), s1, n1, s2, n2);
    for (int i = 0; i < n1; ++i) delete trash[(size_t) (s1 + i)];
    for (int i = 0; i < n2; ++i) delete trash[(size_t) (s2 + i)];
    trashFifo.finishedRead (n1 + n2);
}

void DrumMachine::audition (int pad, int vel) noexcept
{
    if (pad < 0 || pad >= kNumPads) return;
    auditionVel[(size_t) pad].store (vel); auditionMask.fetch_or (1u << pad);
}

static void peaking (float* b, double f, double q, double gainDb, double sr)
{
    if (std::abs (gainDb) < 0.05) { b[0] = 1; b[1] = b[2] = b[3] = b[4] = 0; return; }
    const double A = std::pow (10.0, gainDb / 40.0), w = kTwoPi * f / sr, cw = std::cos (w), al = std::sin (w) / (2 * q);
    const double a0 = 1 + al / A;
    b[0] = (float) ((1 + al * A) / a0); b[1] = (float) (-2 * cw / a0); b[2] = (float) ((1 - al * A) / a0); b[3] = (float) (-2 * cw / a0); b[4] = (float) ((1 - al / A) / a0);
}
static void highShelf (float* b, double f, double gainDb, double sr)
{
    if (std::abs (gainDb) < 0.05) { b[0] = 1; b[1] = b[2] = b[3] = b[4] = 0; return; }
    const double A = std::pow (10.0, gainDb / 40.0), w = kTwoPi * juce::jmin (f, sr * 0.45) / sr, cw = std::cos (w), al = std::sin (w) / 2 * std::sqrt (2.0), sA = 2 * std::sqrt (A) * al;
    const double a0 = (A + 1) - (A - 1) * cw + sA;
    b[0] = (float) (A * ((A + 1) + (A - 1) * cw + sA) / a0); b[1] = (float) (-2 * A * ((A - 1) + (A + 1) * cw) / a0); b[2] = (float) (A * ((A + 1) + (A - 1) * cw - sA) / a0);
    b[3] = (float) (2 * ((A - 1) - (A + 1) * cw) / a0); b[4] = (float) (((A + 1) - (A - 1) * cw - sA) / a0);
}

void DrumMachine::startVoice (Voice& v, int pad, float vel) noexcept
{
    const PadControls c = creative ? ctl[(size_t) pad] : neutral();
    v = Voice(); v.active = true; v.pad = pad; v.vel = vel; v.c = c;
    v.rng = 0x1234567u + (uint32_t) pad * 977u + (clock++ * 2654435761u);
    v.punchAmt = juce::jlimit (-1.0f, 1.0f, (c.punch - 0.5f) * 2.0f);
    v.softAtk = c.soft * 0.006f;
    const double fc = 18000.0 * std::pow (2500.0 / 18000.0, (double) juce::jlimit (0.0f, 1.0f, c.soft));
    v.lpC = (float) (1.0 - std::exp (-kTwoPi * juce::jmin (fc, sr * 0.45) / sr));
    float bm[5], bh[5];
    peaking (bm, kMidHz[pad], 0.9, c.mid, sr); highShelf (bh, kHighHz[pad], c.high, sr);
    v.mid.b0 = bm[0]; v.mid.b1 = bm[1]; v.mid.b2 = bm[2]; v.mid.a1 = bm[3]; v.mid.a2 = bm[4];
    v.high.b0 = bh[0]; v.high.b1 = bh[1]; v.high.b2 = bh[2]; v.high.a1 = bh[3]; v.high.a2 = bh[4];
    const double dm = juce::jlimit (0.25, 2.0, (double) c.decay), tuneR = std::pow (2.0, c.tune / 12.0);
    v.smp = creative ? cur[(size_t) pad] : nullptr;
    if (v.smp != nullptr && v.smp->buf.getNumSamples() > 1)
    {
        v.srate = v.smp->rate / sr * tuneR;
        const double dur = v.smp->buf.getNumSamples() / v.smp->rate / tuneR;
        v.len = c.decay < 1.0f ? dur * (0.25 + 0.75 * c.decay) : dur;
        return;
    }
    v.smp = nullptr;
    const auto& K = k;
    switch (pad)
    {
        case kPadKick:    v.len = (K.kd * 1.6 + .1) * dm; svfCoef (v.cfA, 1800, .7, sr); break;
        case kPadSnare:   v.len = juce::jmax (K.std_, K.snd) * 1.6 * dm + .05; svfCoef (v.cfA, K.snf, .8, sr); svfCoef (v.cfB, 1200, .7, sr); break;
        case kPadClap:    v.len = (K.cd * 1.6 + .05) * dm + .04; svfCoef (v.cfA, K.cnf, 1.3, sr); break;
        case kPadHat:     v.len = K.hd * 1.8 * dm + .02; svfCoef (v.cfA, K.hhp, .707, sr); svfCoef (v.cfB, 10000, .9, sr); break;
        case kPadOpenHat: v.len = K.od * 1.8 * dm + .02; svfCoef (v.cfA, K.ohp, .707, sr); svfCoef (v.cfB, 10000, .9, sr); break;
        case kPadRim:     v.len = .08 * dm + .01; svfCoef (v.cfA, 3400, 1, sr); break;
        default:          v.len = (K.pd * 1.6 + .05) * dm; svfCoef (v.cfA, K.pf * 3 * tuneR, 1.2, sr); break;
    }
    v.srate = tuneR;   // analog seste ton çarpanı
}

int DrumMachine::hit (int note, int vel) noexcept
{
    const int pad = padForNote (note);
    if (pad < 0) return -1;
    if (pad == kPadOpenHat) for (auto& x : voices) if (x.active && x.pad == kPadOpenHat) x.len = juce::jmin (x.len, x.t + .01);   // açık hat'ı yeni hat kısar
    if (pad == kPadHat) for (auto& x : voices) if (x.active && x.pad == kPadOpenHat) x.len = juce::jmin (x.len, x.t + .02);
    Voice* d = nullptr; double oldest = -1;
    for (auto& x : voices) if (! x.active) { d = &x; break; }
    if (d == nullptr) for (auto& x : voices) if (x.t > oldest) { oldest = x.t; d = &x; }
    startVoice (*d, pad, juce::jlimit (0.0f, 1.0f, vel / 127.0f));
    lastHit.store (pad, std::memory_order_relaxed);
    return pad;
}

float DrumMachine::synth (Voice& v) noexcept
{
    const auto& K = k; const double t = v.t, dt = 1.0 / sr, dm = juce::jlimit (0.25, 2.0, (double) v.c.decay), tr = v.srate;
    const float vel = v.vel, soft = v.c.soft, pa = v.punchAmt;
    switch (v.pad)
    {
        case kPadKick:
        {
            // perde zarflı sinüs gövde + tokmak tıkırtısı; Punch süpürmeyi derinleştirir ve hızlandırır
            const double f0 = K.kf1 + (K.kf0 - K.kf1) * (1.0 + 0.5 * pa), pd = K.kpd * (1.0 - 0.3 * pa);
            const double f = (K.kf1 + (f0 - K.kf1) * std::exp (-t / juce::jmax (.004, pd))) * tr;
            v.ph += f * dt; v.ph -= std::floor (v.ph);
            const double hold = K.kd * .15 * dm;
            const float amp = (float) (t < .0005 ? t / .0005 : 1.0) * (t < hold ? .95f : .95f * expd (t - hold, K.kd * dm / 4));
            float s = (float) std::sin (kTwoPi * v.ph) * amp * vel;
            const float drv = K.kdrv * (1.0f - .6f * soft);
            if (drv > 0) s = sat (s, drv);
            const float click = K.kclick * (1.0f - .7f * soft);
            if (click > 0 && t < .05) s += svf (frand (v.rng), v.cfA, v.s[0]).hp * vel * click * .5f * expd (t, .005);
            return s;
        }
        case kPadSnare:
        {
            // 909 çizgisi: iki tonlu gövde (perde düşüşlü) + bant geçiren gürültü
            const double f = K.sf * tr * (1.0 + .4 * std::exp (-t / .02));
            v.ph += f * dt; v.ph -= std::floor (v.ph); v.ph2[0] += f * 1.62 * dt; v.ph2[0] -= std::floor (v.ph2[0]);
            const float tone = ((float) std::sin (kTwoPi * v.ph) * .65f + (float) std::sin (kTwoPi * v.ph2[0]) * .35f) * expd (t, K.std_ * dm / 4);
            const float nz = frand (v.rng); const float n1 = svf (nz, v.cfA, v.s[0]).bp, n2 = svf (nz, v.cfB, v.s[1]).hp;
            const float noise = (n1 * .8f + n2 * .35f) * expd (t, K.snd * dm / 4);
            return (tone * (1 - K.smix) * .85f + noise * K.smix * 1.1f) * vel;
        }
        case kPadClap:
        {
            // dört hızlı darbe ve kuyruk (analog clap devresi gibi)
            const float nz = svf (frand (v.rng), v.cfA, v.s[0]).bp;
            float e = 0; const double offs[4] = { 0, .010, .021, .032 };
            for (int b = 0; b < 4; ++b) if (t >= offs[b]) e += b < 3 ? expd (t - offs[b], .006) : expd (t - offs[b], K.cd * dm / 4);
            return nz * vel * .6f * e;
        }
        case kPadHat: case kPadOpenHat:
        {
            const bool open = v.pad == kPadOpenHat; const float m = open ? K.om : K.hm, dd = open ? K.od : K.hd;
            float x = frand (v.rng) * (1 - m * .6f);
            float sq = 0;
            for (int i = 0; i < 6; ++i) { v.ph2[i] += kMetal[i] * 1.7 * tr * dt; v.ph2[i] -= std::floor (v.ph2[i]); sq += v.ph2[i] < .5 ? 1.0f : -1.0f; }
            x += sq * (m * .18f + .03f);
            const float y = svf (svf (x, v.cfB, v.s[1]).bp * .7f + x * .3f, v.cfA, v.s[0]).hp;
            return y * vel * .45f * expd (t, dd * dm / 4);
        }
        case kPadRim:
        {
            v.ph += K.rf * tr * dt; v.ph -= std::floor (v.ph); v.ph2[0] += K.rf * 1.47 * tr * dt; v.ph2[0] -= std::floor (v.ph2[0]);
            return ((float) std::sin (kTwoPi * v.ph) * .45f * expd (t, .008 * dm) + (float) std::sin (kTwoPi * v.ph2[0]) * .2f * expd (t, .006 * dm)
                    + svf (frand (v.rng), v.cfA, v.s[0]).bp * .3f * (1.0f - .6f * soft) * expd (t, .0075)) * vel;
        }
        default:
        {
            // konga / tom: hafif perde düşüşü, gövde dalgası kitten, vuruş gürültüsü
            const double f = K.pf * tr * (1.0 + .25 * std::exp (-t / .02)); v.ph += f * dt; v.ph -= std::floor (v.ph);
            const float w = K.ptype == 1 ? (float) (v.ph < .5 ? 4 * v.ph - 1 : 3 - 4 * v.ph) : K.ptype == 2 ? (v.ph < .5 ? .7f : -.7f) : (float) std::sin (kTwoPi * v.ph);
            return (w * .7f * expd (t, K.pd * dm / 4) + svf (frand (v.rng), v.cfA, v.s[0]).bp * .25f * (1.0f - .7f * soft) * expd (t, .0075)) * vel;
        }
    }
}

void DrumMachine::render (float* L, float* R, int n) noexcept
{
    if (kitDirty.exchange (false, std::memory_order_acquire)) k = pendingKit;
    for (int p = 0; p < kNumPads; ++p)
        if (auto* s = pending[(size_t) p].exchange (nullptr))
        {
            retire (cur[(size_t) p]);
            cur[(size_t) p] = s == &clearSentinel ? nullptr : s;
        }
    if (const uint32_t m = auditionMask.exchange (0))
        for (int p = 0; p < kNumPads; ++p) if (m & (1u << p)) hit (kPadNotes[p], auditionVel[(size_t) p].load());

    const double dt = 1.0 / sr;
    for (auto& v : voices)
    {
        if (! v.active) continue;
        const float lvl = v.c.level;
        for (int i = 0; i < n; ++i)
        {
            float a, b;
            if (v.smp != nullptr)
            {
                const auto& B = v.smp->buf; const int ns = B.getNumSamples();
                const int i0 = (int) v.sp; if (i0 >= ns - 1) { v.active = false; break; }
                const float f = (float) (v.sp - i0);
                const float* c0 = B.getReadPointer (0); const float* c1 = B.getReadPointer (B.getNumChannels() > 1 ? 1 : 0);
                a = (c0[i0] + (c0[i0 + 1] - c0[i0]) * f) * v.vel; b = (c1[i0] + (c1[i0 + 1] - c1[i0]) * f) * v.vel;
                v.sp += v.srate;
                if (v.c.decay < 1.0f) { const double hold = v.len * .5; if (v.t > hold) { const float e = expd (v.t - hold, v.len * .18); a *= e; b *= e; } }
            }
            else a = b = synth (v);
            // ped kanalı: Punch (geçici), Soft (yumuşak atak ve tını), Mid/High EQ, seviye
            float g = lvl;
            if (std::abs (v.punchAmt) > 1e-4f) g *= juce::jmax (0.2f, 1.0f + v.punchAmt * 1.2f * expd (v.t, .015));
            if (v.softAtk > 0.0f && v.t < v.softAtk) g *= (float) (v.t / v.softAtk);
            a *= g; b *= g;
            if (v.lpC < 0.999f) { v.lpS[0] += (a - v.lpS[0]) * v.lpC; v.lpS[1] += (b - v.lpS[1]) * v.lpC; a = v.lpS[0]; b = v.lpS[1]; }
            for (auto* q : { &v.mid, &v.high })
            {
                if (std::abs (q->b1) + std::abs (q->b2) + std::abs (q->a1) < 1e-9f) continue;   // nötr EQ
                float* io[2] = { &a, &b };
                for (int ch = 0; ch < 2; ++ch) { const float x = *io[ch], y = q->b0 * x + q->z1[ch]; q->z1[ch] = q->b1 * x - q->a1 * y + q->z2[ch]; q->z2[ch] = q->b2 * x - q->a2 * y; *io[ch] = y; }
            }
            L[i] += a; R[i] += b;
            v.t += dt;
            if (v.t > v.len) { v.active = false; break; }
        }
    }
}
}
