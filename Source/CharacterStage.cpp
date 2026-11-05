#include "CharacterStage.h"
#include <BinaryData.h>
using namespace juce;

CharacterStage::CharacterStage (hg::Engine& e, hgui::LookAndFeel& l) : eng (e), lnf (l)
{
    setInterceptsMouseClicks (false, false);
    setAccessible (false);   // süs katmanı: ekran okuyucuya gürültü katmaz
    img[0] = ImageFileFormat::loadFrom (BinaryData::elder_jpg, (size_t) BinaryData::elder_jpgSize);
    img[1] = ImageFileFormat::loadFrom (BinaryData::angel_f_jpg, (size_t) BinaryData::angel_f_jpgSize);
    img[2] = ImageFileFormat::loadFrom (BinaryData::angel_m_jpg, (size_t) BinaryData::angel_m_jpgSize);
}

void CharacterStage::tick (int animMode)
{
    mode = animMode;
    // 0 akor, 1 bas, 2 davul, 3 melodi (Engine::partAct). exchange: bu arayüz okuyucusu sıfırlar, ses iş parçacığı yalnızca yazar.
    const float a0 = eng.partAct[0].exchange (0.0f), a1 = eng.partAct[1].exchange (0.0f);
    const float a2 = eng.partAct[2].exchange (0.0f), a3 = eng.partAct[3].exchange (0.0f);
    const float in[3] { jmax (a0, a1, a2, a3), a0, a3 };   // bilge = tüm çıkış
    for (int i = 0; i < 3; ++i) lvl[i] = jmax (in[i], lvl[i] * 0.86f);
    if (animMode > 0) repaint();
}

void CharacterStage::paint (Graphics& g)
{
    const auto& P = lnf.pal();
    const auto area = getLocalBounds().toFloat();
    g.setColour (P.panel2); g.fillRoundedRectangle (area, 10);
    const float slotW = area.getWidth() / 3.0f;
    const int order[3] { 1, 0, 2 };   // sol: akor meleği, orta: bilge, sağ: melodi meleği
    for (int s = 0; s < 3; ++s)
    {
        const int k = order[s];
        const auto slot = Rectangle<float> (area.getX() + s * slotW, area.getY(), slotW, area.getHeight()).reduced (4);
        const auto fig = slot.withTrimmedBottom (16);
        const float L = mode > 0 ? jlimit (0.0f, 1.0f, lvl[k]) : 0.0f;
        if (L > 0.01f)   // hale: ışık çıkışa göre; dolgu degrade yalnızca yüksek modda
        {
            const auto c = Colour (0xffffd98a);
            if (mode >= 2) g.setGradientFill (ColourGradient (c.withAlpha (0.55f * L), fig.getCentreX(), fig.getY() + fig.getHeight() * 0.3f, c.withAlpha (0.0f), fig.getCentreX(), fig.getBottom(), true));
            else g.setColour (c.withAlpha (0.25f * L));
            g.fillEllipse (fig.expanded (10, 4));
        }
        if (img[k].isValid())
        {
            const float sc = fig.getHeight() / (float) img[k].getHeight();
            const auto dst = Rectangle<float> ((float) img[k].getWidth() * sc, fig.getHeight()).withCentre (fig.getCentre());
            g.setOpacity (0.78f + 0.22f * L);
            g.drawImage (img[k], dst, RectanglePlacement::stretchToFit);
            g.setGradientFill (ColourGradient (P.panel2.withAlpha (0.0f), dst.getCentreX(), dst.getCentreY(), P.panel2, dst.getX(), dst.getY(), true));   // kenarlar sahneyle karışsın
            g.fillRect (dst);
            if (k == 0 && mode >= 2 && L > 0.01f)   // asa ucu: nota çalınca parlar
            {
                const float x = dst.getX() + dst.getWidth() * 0.17f, y = dst.getY() + 3;
                g.setGradientFill (ColourGradient (Colour (0xffffe9b0).withAlpha (L), x, y, Colour (0x00ffe9b0), x + 14, y + 14, true));
                g.fillEllipse (x - 14, y - 14, 28, 28);
            }
        }
        g.setColour (P.mute); g.setFont (lnf.font (12));
        g.drawText (names[k], slot.withTop (slot.getBottom() - 16).toNearestInt(), Justification::centred);
    }
}
