#include "Stage3D.h"
#include <cmath>

namespace s3d
{
namespace
{
constexpr float kPi = 3.14159265f, kCamY = .95f, kCamZ = 9.0f;
constexpr juce::uint32 cRobe = 0xF4EEDF, cGold = 0xD9B14A, cSkin = 0xE8B894, cWhite = 0xECECEC, cDark = 0x3A2A22, cHalo = 0xFFD86B, cWing = 0xFAF7F0, cWing2 = 0xE6DDCB, cWood = 0x8A6A3A;
const juce::uint32 kHair[4] = { 0x7A5230, 0xD9A441, 0xB5651D, 0xE8C877 };
const juce::uint32 kRobe[4] = { 0xE9EEF6, 0xF4EEDF, 0xF6EDE6, 0xEEF4EA };
const float kAngelX[4] = { -4.5f, -2.35f, 2.35f, 4.5f }, kAngelZ[4] = { .5f, 0, 0, .5f };
}

Stage::Mat Stage::ident() { return { { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 } }; }
Stage::Mat Stage::mul (const Mat& a, const Mat& b)
{
    Mat r;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 4; ++j)
            r.m[i * 4 + j] = a.m[i * 4] * b.m[j] + a.m[i * 4 + 1] * b.m[4 + j] + a.m[i * 4 + 2] * b.m[8 + j] + (j == 3 ? a.m[i * 4 + 3] : 0.0f);
    return r;
}
Stage::Mat Stage::rx (float a) { const float c = std::cos (a), s = std::sin (a); return { { 1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0 } }; }
Stage::Mat Stage::ry (float a) { const float c = std::cos (a), s = std::sin (a); return { { c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0 } }; }
Stage::Mat Stage::rz (float a) { const float c = std::cos (a), s = std::sin (a); return { { c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0 } }; }
Stage::Mat Stage::tr (float x, float y, float z) { return { { 1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z } }; }
Stage::Mat Stage::sc (float x, float y, float z) { return { { x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0 } }; }

// Döner yüzey: (yarıçap, yükseklik) profili Y ekseni çevresinde döndürülür; cols profil dilimi başına renk
void Stage::lathe (Part& p, const std::vector<std::array<float, 2>>& prof, int segs, const std::vector<juce::uint32>& cols, const Mat& xf)
{
    const int base = (int) p.v.size(), R = (int) prof.size();
    for (int i = 0; i < R; ++i)
        for (int s = 0; s < segs; ++s)
        {
            const float a = 2 * kPi * (float) s / (float) segs, x = prof[(size_t) i][0] * std::cos (a), y = prof[(size_t) i][1], z = prof[(size_t) i][0] * std::sin (a);
            p.v.push_back ({ xf.m[0] * x + xf.m[1] * y + xf.m[2] * z + xf.m[3], xf.m[4] * x + xf.m[5] * y + xf.m[6] * z + xf.m[7], xf.m[8] * x + xf.m[9] * y + xf.m[10] * z + xf.m[11] });
        }
    for (int i = 0; i + 1 < R; ++i)
        for (int s = 0; s < segs; ++s)
        {
            const int a = base + i * segs + s, b = base + i * segs + (s + 1) % segs, c = a + segs, d = b + segs; const auto col = cols[(size_t) juce::jmin (i, (int) cols.size() - 1)];
            p.tri.insert (p.tri.end(), { a, b, c, b, d, c }); p.col.push_back (col); p.col.push_back (col);
        }
}

void Stage::ball (Part& p, float r, int segs, juce::uint32 col, const Mat& xf)
{
    std::vector<std::array<float, 2>> prof; const int rings = juce::jmax (4, segs / 2);
    for (int i = 0; i <= rings; ++i) { const float a = kPi * (float) i / (float) rings; prof.push_back ({ r * std::sin (a), r * std::cos (a) }); }
    lathe (p, prof, segs, { col }, xf);
}

// Kanat: her tüy ayrı bir şerit (uçları sivri); altta uzun uçuş tüyleri, üstte kısa örtü tüyleri
void Stage::wing (Part& p, float side)
{
    const int NU = 11, NV = 4;
    for (int layer = 0; layer < 2; ++layer)
        for (int k = 0; k < NU; ++k)
        {
            const int base = (int) p.v.size();
            for (int j = 0; j <= NV; ++j)
                for (int e = 0; e < 2; ++e)
                {
                    const float v = (float) j / NV, taper = j == NV ? .3f : j == NV - 1 ? .06f : -.08f;   // tüyler birbirinin üstüne biner, uçları sivrilir
                    const float u = ((float) k + (e == 0 ? taper : 1 - taper)) / NU, um = ((float) k + .5f) / NU;
                    const float top = .55f * std::sin (kPi * u * .8f) - .25f * u * u, len = (.35f + .75f * std::sin (kPi * (um * .85f + .1f)) * (1 - .3f * um)) * (layer == 0 ? 1.0f : .5f);
                    p.v.push_back ({ side * (.05f + 1.05f * u + (layer == 0 ? .12f * v * um : 0.0f)), top - v * len, -.05f - .28f * u * u - .1f * v * v + (layer == 1 ? .04f : 0.0f) });
                }
            const auto col = layer == 1 ? cWing : (k % 2 == 0 ? cWing : cWing2);
            for (int j = 0; j < NV; ++j) { const int a = base + j * 2; p.tri.insert (p.tri.end(), { a, a + 1, a + 2, a + 1, a + 3, a + 2 }); p.col.push_back (col); p.col.push_back (col); }
        }
    p.wing = true;
}

void Stage::normals (Part& p)
{
    p.n.assign (p.v.size(), { 0, 0, 0 });
    for (size_t t = 0; t + 2 < p.tri.size(); t += 3)
    {
        const auto& a = p.v[(size_t) p.tri[t]]; const auto& b = p.v[(size_t) p.tri[t + 1]]; const auto& c = p.v[(size_t) p.tri[t + 2]];
        const V3 e1 { b.x - a.x, b.y - a.y, b.z - a.z }, e2 { c.x - a.x, c.y - a.y, c.z - a.z }, n { e1.y * e2.z - e1.z * e2.y, e1.z * e2.x - e1.x * e2.z, e1.x * e2.y - e1.y * e2.x };
        for (int k = 0; k < 3; ++k) { auto& d = p.n[(size_t) p.tri[t + (size_t) k]]; d.x += n.x; d.y += n.y; d.z += n.z; }
    }
    for (auto& n : p.n) { const float l = std::sqrt (n.x * n.x + n.y * n.y + n.z * n.z); if (l > 1e-9f) { n.x /= l; n.y /= l; n.z /= l; } else n = { 0, 1, 0 }; }
}

Stage::Figure Stage::makeFigure (bool elder, int variant)
{
    Figure f; f.parts.resize (9); f.world = ident();
    for (auto& p : f.parts) p.local = ident();
    const auto I = ident(); const juce::uint32 robe = elder ? cRobe : kRobe[variant & 3], hair = elder ? cWhite : kHair[variant & 3];
    // gövde: cübbe (altın yaka, kemer ve etek şeridi)
    if (elder) lathe (f.parts[pBody], { { 0, 1.48f }, { .09f, 1.47f }, { .27f, 1.38f }, { .31f, 1.12f }, { .26f, .86f }, { .27f, .78f }, { .34f, .35f }, { .5f, -.28f }, { .53f, -.4f }, { 0, -.4f } }, 18,
                      { cGold, robe, robe, robe, cGold, robe, robe, cGold, robe }, I);
    else lathe (f.parts[pBody], { { 0, 1.46f }, { .07f, 1.45f }, { .2f, 1.37f }, { .22f, 1.15f }, { .17f, .9f }, { .18f, .82f }, { .26f, .4f }, { .4f, -.28f }, { .43f, -.4f }, { 0, -.4f } }, 16,
                { cGold, robe, robe, robe, cGold, robe, robe, cGold, robe }, I);
    // baş: yüz, saç, gözler; bilgede sakal
    auto& h = f.parts[pHead];
    ball (h, .16f, 12, cSkin, I);
    ball (h, .178f, 12, hair, mul (tr (0, .03f, -.05f), sc (1.02f, 1, 1)));
    lathe (h, { { .14f, .08f }, { .16f, -.1f }, { .12f, elder ? -.34f : -.44f }, { 0, elder ? -.38f : -.5f } }, 10, { hair }, tr (0, 0, -.135f));
    for (float sx : { -.058f, .058f }) ball (h, .014f, 6, cDark, tr (sx, .025f, .148f));
    if (elder) lathe (h, { { .125f, -.02f }, { .13f, -.1f }, { .08f, -.26f }, { 0, -.38f } }, 10, { cWhite }, mul (tr (0, -.02f, .075f), sc (1, 1, .7f)));
    // kollar: geniş yen ve el (omuzdan aşağı doğru kurulur, omuz çevresinde döner)
    for (int a : { pArmL, pArmR })
    {
        const float w = elder ? 1.15f : 1.0f;
        lathe (f.parts[(size_t) a], { { 0, .02f }, { .085f * w, 0 }, { .1f * w, -.3f }, { .14f * w, -.56f }, { .05f, -.57f } }, 10, { robe, robe, robe, cGold }, I);
        ball (f.parts[(size_t) a], .058f, 8, cSkin, tr (0, -.63f, 0));
    }
    // hale: yatay altın halka
    {
        std::vector<std::array<float, 2>> ring; const float R = elder ? .27f : .22f;
        for (int i = 0; i <= 8; ++i) { const float a = 2 * kPi * (float) i / 8; ring.push_back ({ R + .022f * std::cos (a), -.022f * std::sin (a) }); }
        lathe (f.parts[pHalo], ring, 20, { cHalo }, I); f.parts[pHalo].glowAmt = 1;
    }
    if (elder)
    {   // asa: tepesinde ışık küresi ve altın halka
        auto& s = f.parts[pStaff];
        lathe (s, { { .03f, 1.72f }, { .028f, -.4f } }, 6, { cWood }, I);
        ball (s, .11f, 10, 0xFFF1B8, tr (0, 1.9f, 0));
        std::vector<std::array<float, 2>> ring; for (int i = 0; i <= 6; ++i) { const float a = 2 * kPi * (float) i / 6; ring.push_back ({ .2f + .018f * std::cos (a), -.018f * std::sin (a) }); }
        lathe (s, ring, 14, { cGold }, mul (tr (0, 1.9f, 0), rx (kPi * .5f)));
    }
    else
    {
        wing (f.parts[pWingL], -1); wing (f.parts[pWingR], 1);
        ball (f.parts[pMouth], .022f, 6, 0x7A3A30, I);
    }
    for (auto& p : f.parts) normals (p);
    return f;
}

Stage::Stage()
{
    for (int i = 0; i < 4; ++i) fig[(size_t) i] = makeFigure (false, i);
    fig[4] = makeFigure (true, 0);
    pose ({});
}

int Stage::triangles() const { int n = 0; for (auto& f : fig) for (auto& p : f.parts) n += (int) p.tri.size() / 3; return n; }

void Stage::pose (const Pose& q)
{
    const float t = q.t, lv = juce::jlimit (0.0f, 1.0f, q.level), pulse = std::exp (-5.0f * q.beat) * lv, sw = std::sin (2 * kPi * q.beat), k = juce::jlimit (0.0f, 1.0f, lv * 3);
    glow.clear();
    for (int i = 0; i < 4; ++i)
    {
        auto& f = fig[(size_t) i]; const bool on = q.on[(size_t) i]; const float v = on ? juce::jlimit (0.0f, 1.0f, q.voice[(size_t) i]) : 0.0f, side = i < 2 ? 1.0f : -1.0f;
        const float y = .44f + (on ? .12f * std::sin (t * 1.3f + (float) i * 1.7f) + .1f * v : -.12f);
        f.dim = on ? 1.0f : .45f;
        f.world = mul (tr (kAngelX[i], y, kAngelZ[i]), mul (ry (side * .32f + .05f * std::sin (t * .7f + (float) i)), mul (rz (.05f * sw * k), sc (.82f, .82f, .82f))));
        const float idle = .18f * std::sin (t * 1.6f + (float) i * .9f), flap = on ? .22f + idle * (1 - k) + (.3f + .45f * lv) * std::sin (2 * kPi * q.beat - (float) i * .4f) * k : -.95f;
        f.parts[pWingL].local = mul (tr (-.1f, 1.24f, -.15f), mul (ry (.38f), rz (-flap)));
        f.parts[pWingR].local = mul (tr (.1f, 1.24f, -.15f), mul (ry (-.38f), rz (flap)));
        const float lift = .25f + .55f * v, fwd = .95f - .45f * v;     // susarken eller önde, söylerken kollar açılır
        f.parts[pArmL].local = mul (tr (-.19f, 1.33f, 0), mul (rz (-lift), rx (-fwd)));
        f.parts[pArmR].local = mul (tr (.19f, 1.33f, 0), mul (rz (lift), rx (-fwd)));
        f.parts[pHead].local = mul (tr (0, 1.6f, 0), mul (rx (-.28f * v), rz (.06f * std::sin (t * 1.1f + (float) i))));
        f.parts[pMouth].local = mul (f.parts[pHead].local, mul (tr (0, -.075f, .15f), sc (1, .3f + 1.5f * v, .6f)));
        f.parts[pHalo].local = mul (tr (0, 1.9f, 0), mul (rx (.22f), ry (t * .8f)));
        f.parts[pHalo].glowAmt = on ? .55f + .45f * v : 0.0f;
    }
    {
        auto& f = fig[4];
        f.world = mul (tr (0, -.06f + .03f * std::sin (t * .9f), -.4f), mul (ry (.07f * std::sin (t * .5f)), sc (1.2f, 1.2f, 1.2f)));
        f.parts[pArmL].local = mul (tr (-.27f, 1.34f, 0), mul (rz (-.62f), rx (-.55f)));                       // asayı tutan kol
        const float lift = .3f + .95f * lv + .3f * pulse;
        f.parts[pArmR].local = mul (tr (.27f, 1.34f, 0), mul (rz (lift), rx (-.35f - .25f * pulse)));            // yöneten kol: seviyeyle kalkar, vuruşta iner
        f.parts[pHead].local = mul (tr (0, 1.63f, 0), mul (rx (-.1f * lv), ry (.12f * std::sin (t * .6f))));
        f.parts[pHalo].local = mul (tr (0, 1.95f, 0), mul (rx (.22f), ry (t * .6f)));
        f.parts[pHalo].glowAmt = .7f + .3f * lv;
        f.parts[pStaff].local = tr (-.66f, 0, .3f);
        f.parts[pStaff].glowAmt = .25f + .75f * juce::jmax (lv, pulse);
    }
}

void Stage::draw (const Figure& f, int W, int H, juce::Rectangle<float>& bb)
{
    const float foc = 2.45f * juce::jmin ((float) H, (float) W / 3.3f), cx = W * .5f, cy = H * .5f;
    const Mat view = tr (0, -kCamY, -kCamZ);
    const float Lx = .33f, Ly = .78f, Lz = .53f;
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (const auto& p : f.parts)
    {
        if (p.tri.empty()) continue;
        const Mat M = mul (view, mul (f.world, p.local));
        const float sN = 1.0f / juce::jmax (1e-6f, std::sqrt (M.m[0] * M.m[0] + M.m[4] * M.m[4] + M.m[8] * M.m[8]));
        tv.resize (p.v.size() * 5);
        for (size_t i = 0; i < p.v.size(); ++i)
        {
            const auto& v = p.v[i]; const auto& n = p.n[i];
            const float x = M.m[0] * v.x + M.m[1] * v.y + M.m[2] * v.z + M.m[3], y = M.m[4] * v.x + M.m[5] * v.y + M.m[6] * v.z + M.m[7], z = M.m[8] * v.x + M.m[9] * v.y + M.m[10] * v.z + M.m[11];
            float nx = (M.m[0] * n.x + M.m[1] * n.y + M.m[2] * n.z) * sN, ny = (M.m[4] * n.x + M.m[5] * n.y + M.m[6] * n.z) * sN, nz = (M.m[8] * n.x + M.m[9] * n.y + M.m[10] * n.z) * sN;
            const float d = juce::jmax (.1f, -z), il = 1.0f / std::sqrt (x * x + y * y + z * z), vx = -x * il, vy = -y * il, vz = -z * il;
            float nv = nx * vx + ny * vy + nz * vz; if (nv < 0) { nx = -nx; ny = -ny; nz = -nz; nv = -nv; }   // normal kameraya baksın: kanatlar iki yüzden aydınlanır
            const float dif = juce::jmax (0.0f, nx * Lx + ny * Ly + nz * Lz), rim = (1 - nv) * (1 - nv) * (1 - nv);
            float* o = &tv[i * 5]; o[0] = cx + x * foc / d; o[1] = cy - y * foc / d; o[2] = d;
            o[3] = (.44f + .6f * dif) * (1 - p.glowAmt) + 1.12f * p.glowAmt; o[4] = rim * .55f * f.dim;
            x0 = juce::jmin (x0, o[0]); x1 = juce::jmax (x1, o[0]); y0 = juce::jmin (y0, o[1]); y1 = juce::jmax (y1, o[1]);
        }
        for (size_t t = 0; t + 2 < p.tri.size(); t += 3)
        {
            const float* a = &tv[(size_t) p.tri[t] * 5]; const float* b = &tv[(size_t) p.tri[t + 1] * 5]; const float* c = &tv[(size_t) p.tri[t + 2] * 5];
            const float area = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]); if (std::abs (area) < 1e-4f || (area > 0 && ! p.wing)) continue;   // kapalı gövdelerde arka yüzler atlanır; kanatlar iki yüzlüdür
            const int minX = juce::jmax (0, (int) std::floor (juce::jmin (a[0], b[0], c[0]))), maxX = juce::jmin (W - 1, (int) std::ceil (juce::jmax (a[0], b[0], c[0])));
            const int minY = juce::jmax (0, (int) std::floor (juce::jmin (a[1], b[1], c[1]))), maxY = juce::jmin (H - 1, (int) std::ceil (juce::jmax (a[1], b[1], c[1])));
            if (minX > maxX || minY > maxY) continue;
            const juce::uint32 col = p.col[t / 3]; float cr = (float) ((col >> 16) & 255), cg = (float) ((col >> 8) & 255), cb = (float) (col & 255);
            if (f.dim < 1) { const float g = (cr + cg + cb) / 3; cr = (cr * .3f + g * .7f) * f.dim; cg = (cg * .3f + g * .7f) * f.dim; cb = (cb * .3f + g * .7f) * f.dim; }
            const float inv = 1.0f / area, d0 = (b[1] - c[1]) * inv, d1 = (c[1] - a[1]) * inv, sx = (float) minX + .5f;
            for (int y = minY; y <= maxY; ++y)
            {
                const float py = (float) y + .5f; juce::uint32* row = &px[(size_t) y * (size_t) W]; float* zr = &zb[(size_t) y * (size_t) W];
                float w0 = ((b[0] - sx) * (c[1] - py) - (b[1] - py) * (c[0] - sx)) * inv, w1 = ((c[0] - sx) * (a[1] - py) - (c[1] - py) * (a[0] - sx)) * inv;
                for (int x = minX; x <= maxX; ++x, w0 += d0, w1 += d1)
                {
                    const float w2 = 1 - w0 - w1;
                    if (w0 < -1e-5f || w1 < -1e-5f || w2 < -1e-5f) continue;
                    const float z = w0 * a[2] + w1 * b[2] + w2 * c[2]; if (z >= zr[x]) continue;
                    zr[x] = z;
                    const float lit = w0 * a[3] + w1 * b[3] + w2 * c[3], rim = (w0 * a[4] + w1 * b[4] + w2 * c[4]) * 255.0f;
                    const int r = juce::jmin (255, (int) (cr * lit + rim)), g = juce::jmin (255, (int) (cg * lit + rim * .94f)), bl = juce::jmin (255, (int) (cb * lit + rim * .78f));
                    row[x] = 0xff000000u | ((juce::uint32) r << 16) | ((juce::uint32) g << 8) | (juce::uint32) bl;
                }
            }
        }
    }
    bb = x1 > x0 ? juce::Rectangle<float> (x0 / W, y0 / H, (x1 - x0) / W, (y1 - y0) / H) : juce::Rectangle<float>();
}

void Stage::render (juce::Image& img, const Pose& q, int ss)
{
    if (! img.isValid() || img.getFormat() != juce::Image::ARGB) return;
    ss = juce::jlimit (1, 3, ss);
    const int w = img.getWidth(), h = img.getHeight(), W = w * ss, H = h * ss;
    px.assign ((size_t) W * (size_t) H, 0u); zb.assign ((size_t) W * (size_t) H, 1e9f);
    pose (q);
    for (int i = 0; i < 5; ++i) draw (fig[(size_t) i], W, H, box[(size_t) i]);
    // ışıltılar: haleler, asa küresi (konumlar figür kutularından)
    const float foc = 2.45f * juce::jmin ((float) H, (float) W / 3.3f);
    auto point = [&] (const Figure& f, const Mat& local, float x, float y, float z, float r, float a)
    {
        const Mat M = mul (tr (0, -kCamY, -kCamZ), mul (f.world, local));
        const float vx = M.m[0] * x + M.m[1] * y + M.m[2] * z + M.m[3], vy = M.m[4] * x + M.m[5] * y + M.m[6] * z + M.m[7], d = juce::jmax (.1f, -(M.m[8] * x + M.m[9] * y + M.m[10] * z + M.m[11]));
        glow.push_back ({ (W * .5f + vx * foc / d) / W, (H * .5f - vy * foc / d) / H, r * foc / d / H, a });
    };
    for (int i = 0; i < 4; ++i) if (q.on[(size_t) i]) point (fig[(size_t) i], fig[(size_t) i].parts[pHalo].local, 0, 0, 0, .34f + .3f * q.voice[(size_t) i], .22f + .5f * q.voice[(size_t) i]);
    point (fig[4], fig[4].parts[pHalo].local, 0, 0, 0, .5f + .3f * q.level, .3f + .4f * q.level);
    point (fig[4], fig[4].parts[pStaff].local, 0, 1.9f, 0, .3f + .5f * q.level, .35f + .55f * q.level);
    // süper örnekleme: kenarlar yumuşar, alfa kapsama oranıdır (önceden çarpılmış ARGB)
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
    const int n2 = ss * ss;
    for (int y = 0; y < h; ++y)
    {
        auto* dst = (juce::PixelARGB*) bd.getLinePointer (y);
        for (int x = 0; x < w; ++x)
        {
            int r = 0, g = 0, b = 0, cnt = 0;
            for (int j = 0; j < ss; ++j) { const juce::uint32* s = &px[(size_t) (y * ss + j) * (size_t) W + (size_t) (x * ss)]; for (int i = 0; i < ss; ++i) { const auto c = s[i]; if (c == 0) continue; ++cnt; r += (int) ((c >> 16) & 255); g += (int) ((c >> 8) & 255); b += (int) (c & 255); } }
            dst[x].setARGB ((juce::uint8) (cnt * 255 / n2), (juce::uint8) (r / n2), (juce::uint8) (g / n2), (juce::uint8) (b / n2));
        }
    }
}

int Stage::hit (float x, float y) const
{
    for (int i = 0; i < 4; ++i) if (box[(size_t) i].contains (x, y)) return i;
    return -1;
}
}
