// Koro sahnesi: yazılımla çizilen 3B figürler (bilge + dört melek). OpenGL kullanmaz: derinlik tamponlu üçgen
// tarayıcı doğrudan bir Image'a yazar, bu yüzden her DAW'da ve her işletim sisteminde aynı çalışır.
// Web sürümündeki web/stage3d.js ile aynı modeller ve aynı hareket.
#pragma once
#include <juce_graphics/juce_graphics.h>
#include <array>
#include <vector>

namespace s3d
{
struct Pose
{
    float t = 0, beat = 0, level = 0;                    // saniye, vuruş fazı 0..1, çıkış seviyesi 0..1
    std::array<float, 4> voice { { 0, 0, 0, 0 } };        // partilerin anlık seviyesi (bas, tenor, alto, soprano)
    std::array<bool, 4> on { { true, true, true, true } };
};
struct Glow { float x, y, r, a; };                        // 0..1 ekran konumu, yarıçap (yükseklik biriminde), opaklık

class Stage
{
public:
    Stage();
    void render (juce::Image& argb, const Pose&, int supersample = 2);   // arka plan saydam kalır
    const std::vector<Glow>& glows() const noexcept { return glow; }
    int hit (float x01, float y01) const;                 // tıklanan melek (parti dizini) ya da -1
    juce::Rectangle<float> bounds (int figure) const { return box[(size_t) juce::jlimit (0, 4, figure)]; }   // 0-3 melekler, 4 bilge (0..1)
    int triangles() const;
private:
    struct V3 { float x, y, z; };
    struct Mat { float m[12]; };
    struct Part { std::vector<V3> v, n; std::vector<int> tri; std::vector<juce::uint32> col; Mat local; float glowAmt = 0; bool wing = false; };
    struct Figure { std::vector<Part> parts; Mat world; float dim = 1; };
    enum { pBody, pHead, pArmL, pArmR, pHalo, pWingL, pWingR, pMouth, pStaff };
    static Mat ident(); static Mat mul (const Mat&, const Mat&); static Mat rx (float); static Mat ry (float); static Mat rz (float); static Mat tr (float, float, float); static Mat sc (float, float, float);
    static void lathe (Part&, const std::vector<std::array<float, 2>>& prof, int segs, const std::vector<juce::uint32>& cols, const Mat& xf);
    static void ball (Part&, float r, int segs, juce::uint32 col, const Mat& xf);
    static void wing (Part&, float side);
    static void normals (Part&);
    Figure makeFigure (bool elder, int variant);
    void pose (const Pose&);
    void draw (const Figure&, int W, int H, juce::Rectangle<float>& bb);
    std::array<Figure, 5> fig;                            // 0-3 melekler, 4 bilge
    std::array<juce::Rectangle<float>, 5> box;
    std::vector<Glow> glow; std::vector<juce::uint32> px; std::vector<float> zb; std::vector<float> tv;
};
}
