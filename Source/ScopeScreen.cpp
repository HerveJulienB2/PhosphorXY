#include "ScopeScreen.h"
#include <climits>
#include "BraunLookAndFeel.h"

using namespace juce;

ScopeScreen::ScopeScreen (PhosphorProcessor& p)
    : proc (p), vblank (this, [this] { update(); })
{
    auto& s = proc.apvts;
    for (int ch = 0; ch < 2; ++ch)
    {
        cp[ch].on     = s.getRawParameterValue (Params::id (ch, "on"));
        cp[ch].amp    = s.getRawParameterValue (Params::id (ch, "amp"));
        cp[ch].time   = s.getRawParameterValue (Params::id (ch, "time"));
        cp[ch].pos    = s.getRawParameterValue (Params::id (ch, "pos"));
        cp[ch].level  = s.getRawParameterValue (Params::id (ch, "level"));
        cp[ch].slope  = s.getRawParameterValue (Params::id (ch, "slope"));
        cp[ch].colour = s.getRawParameterValue (Params::id (ch, "color"));
    }
    xyMode   = s.getRawParameterValue ("xy");
    rot      = s.getRawParameterValue ("rot");
    hold     = s.getRawParameterValue ("hold");
    autoTrig = s.getRawParameterValue ("auto");
    xyLen    = s.getRawParameterValue ("xy_len");
    intens   = s.getRawParameterValue ("intens");
    focus    = s.getRawParameterValue ("focus");
    persist  = s.getRawParameterValue ("persist");
    illum    = s.getRawParameterValue ("illum");
    fxCurve  = s.getRawParameterValue ("fx_curve");
    fxScan   = s.getRawParameterValue ("fx_scan");
    fxGlitch = s.getRawParameterValue ("fx_glitch");

    setOpaque (true);
}

void ScopeScreen::resized()
{
    const int w = jmax (1, getWidth()), h = jmax (1, getHeight());
    persistImage = Image (Image::ARGB, w, h, true, SoftwareImageType());
    persistImage.clear (persistImage.getBounds(), Colours::black);
    frameImage = Image (Image::ARGB, w, h, true, SoftwareImageType());
    outImage = Image (Image::ARGB, w, h, true, SoftwareImageType());

    rowDx.assign ((size_t) h, 0);
    rowSplit.assign ((size_t) h, 0);
    bands.clear();
    mapW = mapH = 0;          // force le recalcul de la courbure
    scanBuiltFor = -1.0f;     // et des lignes de balayage

    rebuildGraticule();
}

void ScopeScreen::rebuildGraticule()
{
    const int w = getWidth(), h = getHeight();
    graticule = Image (Image::ARGB, jmax (1, w), jmax (1, h), true, SoftwareImageType());
    Graphics g (graticule);

    const Colour col (0xffbfe3c8);
    const float dw = (float) w / 10.0f, dh = (float) h / 8.0f;
    const float dash[] = { 2.0f, 4.0f };

    g.setColour (col.withAlpha (0.38f));
    for (int i = 1; i < 10; ++i)
        if (i != 5) g.drawDashedLine (Line<float> (std::round (i * dw) + 0.5f, 0.0f, std::round (i * dw) + 0.5f, (float) h), dash, 2, 1.0f);
    for (int j = 1; j < 8; ++j)
        if (j != 4) g.drawDashedLine (Line<float> (0.0f, std::round (j * dh) + 0.5f, (float) w, std::round (j * dh) + 0.5f), dash, 2, 1.0f);

    // axes centraux pleins avec graduations tous les 1/5 de division
    const float cx = std::round ((float) w * 0.5f) + 0.5f, cy = std::round ((float) h * 0.5f) + 0.5f;
    g.setColour (col.withAlpha (0.65f));
    g.drawLine (cx, 0.0f, cx, (float) h, 1.0f);
    g.drawLine (0.0f, cy, (float) w, cy, 1.0f);
    for (int i = 0; i <= 50; ++i)
    {
        const float x = (float) i * dw / 5.0f;
        g.drawLine (x, cy - 3.0f, x, cy + 3.0f, 1.0f);
    }
    for (int j = 0; j <= 40; ++j)
    {
        const float y = (float) j * dh / 5.0f;
        g.drawLine (cx - 3.0f, y, cx + 3.0f, y, 1.0f);
    }

    // reperes 0 % et 100 % (temps de montee)
    g.setColour (col.withAlpha (0.5f));
    for (float f : { 0.5f, 3.5f })
        g.drawLine (cx - 10.0f, std::round (f * dh) + 0.5f, cx + 10.0f, std::round (f * dh) + 0.5f, 1.0f);
}

void ScopeScreen::fadeImage (int k256)
{
    Image::BitmapData bd (persistImage, Image::BitmapData::readWrite);
    const int w = bd.width, h = bd.height, stride = bd.pixelStride;
    for (int y = 0; y < h; ++y)
    {
        auto* line = bd.getLinePointer (y);
        for (int x = 0; x < w; ++x)
        {
            auto* px = line + x * stride;
            px[0] = (uint8) ((px[0] * k256) >> 8);
            px[1] = (uint8) ((px[1] * k256) >> 8);
            px[2] = (uint8) ((px[2] * k256) >> 8);
        }
    }
}

//==============================================================================
// Effets d'image

void ScopeScreen::rebuildMap (float curve)
{
    const int w = getWidth(), h = getHeight();
    mapX.assign ((size_t) (w * h), 0);
    mapY.assign ((size_t) (w * h), 0);

    // distorsion en barillet : les bords se courbent, les coins s'arrondissent
    const float k = curve * 0.22f;
    for (int y = 0; y < h; ++y)
    {
        const float v = ((float) y + 0.5f) / (float) h * 2.0f - 1.0f;
        for (int x = 0; x < w; ++x)
        {
            const float u = ((float) x + 0.5f) / (float) w * 2.0f - 1.0f;
            const float sx = u * (1.0f + k * v * v);
            const float sy = v * (1.0f + k * u * u);
            const float px = (sx + 1.0f) * 0.5f * (float) w - 0.5f;
            const float py = (sy + 1.0f) * 0.5f * (float) h - 0.5f;

            const size_t i = (size_t) (y * w + x);
            if (px < -0.5f || py < -0.5f || px > (float) w - 0.5f || py > (float) h - 0.5f)
            {
                mapX[i] = INT_MIN;      // en dehors de l'image : noir
            }
            else
            {
                mapX[i] = (int) (px * 65536.0f);
                mapY[i] = (int) (py * 65536.0f);
            }
        }
    }

    mapCurve = curve;
    mapW = w;
    mapH = h;
}

static inline void bilinear (const uint8* base, int pixelStride, int lineStride, int w, int h,
                             int sx, int sy, uint8* out)
{
    int x0 = sx >> 16, y0 = sy >> 16;
    const int fx = (sx >> 8) & 255, fy = (sy >> 8) & 255;
    int x1 = x0 + 1, y1 = y0 + 1;
    x0 = jlimit (0, w - 1, x0);  x1 = jlimit (0, w - 1, x1);
    y0 = jlimit (0, h - 1, y0);  y1 = jlimit (0, h - 1, y1);

    const uint8* p00 = base + y0 * lineStride + x0 * pixelStride;
    const uint8* p01 = base + y0 * lineStride + x1 * pixelStride;
    const uint8* p10 = base + y1 * lineStride + x0 * pixelStride;
    const uint8* p11 = base + y1 * lineStride + x1 * pixelStride;

    for (int c = 0; c < 3; ++c)
    {
        const int top = p00[c] * (256 - fx) + p01[c] * fx;
        const int bot = p10[c] * (256 - fx) + p11[c] * fx;
        out[c] = (uint8) ((top * (256 - fy) + bot * fy) >> 16);
    }
}

void ScopeScreen::applyRemap()
{
    Image::BitmapData src (frameImage, Image::BitmapData::readOnly);
    Image::BitmapData dst (outImage, Image::BitmapData::writeOnly);
    const int w = src.width, h = src.height;
    const uint8* base = src.getLinePointer (0);
    const int ps = src.pixelStride, ls = src.lineStride;

    for (int y = 0; y < h; ++y)
    {
        auto* d = dst.getLinePointer (y);
        const int dx = rowDx[(size_t) y] * 65536;
        const int split = rowSplit[(size_t) y] * 65536;
        const int dy = vJump * 65536;

        for (int x = 0; x < w; ++x)
        {
            auto* px = d + x * dst.pixelStride;
            const size_t i = (size_t) (y * w + x);

            if (mapX[i] == INT_MIN)
            {
                px[0] = px[1] = px[2] = 0;
                px[3] = 255;
                continue;
            }

            const int sx = mapX[i] + dx, sy = mapY[i] + dy;
            if (split == 0)
            {
                bilinear (base, ps, ls, w, h, sx, sy, px);
            }
            else
            {
                // separation des couleurs : rouge et bleu decales de part et d'autre du vert
                uint8 t[3];
                bilinear (base, ps, ls, w, h, sx, sy, t);            px[1] = t[1];
                bilinear (base, ps, ls, w, h, sx + split, sy, t);    px[2] = t[2];
                bilinear (base, ps, ls, w, h, sx - split, sy, t);    px[0] = t[0];
            }
            px[3] = 255;
        }
    }
}

void ScopeScreen::rebuildScan (float amount)
{
    const int w = getWidth(), h = getHeight();
    scanImage = Image (Image::ARGB, jmax (1, w), jmax (1, h), true, SoftwareImageType());
    Graphics g (scanImage);
    g.setColour (Colours::black.withAlpha (amount * 0.40f));
    for (int y = 1; y < h; y += 3)
        g.fillRect (0, y, w, 1);
    scanBuiltFor = amount;
}

void ScopeScreen::updateGlitch (float amount)
{
    const int h = getHeight();
    if ((int) rowDx.size() != h)
        return;

    for (auto& b : bands)
        --b.left;
    bands.erase (std::remove_if (bands.begin(), bands.end(), [] (const GlitchBand& b) { return b.left <= 0; }), bands.end());

    if (vJumpLeft > 0 && --vJumpLeft == 0)
        vJump = 0;

    if (amount > 0.001f)
    {
        // probabilite par image : ~0,4 glitch/s a 10 %, ~3/s a 50 %, ~10/s a 100 %
        const float p = 0.005f + 0.17f * amount * amount;
        if (rng.nextFloat() < p)
        {
            const int count = 1 + (amount > 0.5f ? rng.nextInt (3) : 0);
            for (int i = 0; i < count; ++i)
            {
                GlitchBand b;
                b.h = 3 + rng.nextInt ((int) (10.0f + amount * 70.0f));
                b.y0 = rng.nextInt (jmax (1, h - b.h));
                b.dx = (rng.nextBool() ? 1 : -1) * (4 + rng.nextInt ((int) (8.0f + amount * 90.0f)));
                b.split = rng.nextFloat() < 0.6f ? 1 + rng.nextInt ((int) (2.0f + amount * 8.0f)) : 0;
                b.left = 2 + rng.nextInt ((int) (2.0f + amount * 5.0f));
                bands.push_back (b);
            }
        }

        if (vJumpLeft == 0 && rng.nextFloat() < p * 0.25f)
        {
            vJump = (rng.nextBool() ? 1 : -1) * (2 + rng.nextInt ((int) (4.0f + amount * 24.0f)));
            vJumpLeft = 2;
        }
    }
    else
    {
        bands.clear();
        vJump = 0;
        vJumpLeft = 0;
    }

    std::fill (rowDx.begin(), rowDx.end(), 0);
    std::fill (rowSplit.begin(), rowSplit.end(), 0);
    for (const auto& b : bands)
        for (int y = b.y0; y < jmin (h, b.y0 + b.h); ++y)
        {
            rowDx[(size_t) y] = b.dx;
            rowSplit[(size_t) y] = b.split;
        }
}

//==============================================================================
void ScopeScreen::update()
{
    const double now = Time::getMillisecondCounterHiRes();
    const double dt = jlimit (0.004, 0.1, (now - lastMs) * 0.001);
    lastMs = now;

    updateGlitch (fxGlitch->load());

    if (persistImage.isNull() || hold->load() > 0.5f)
    {
        repaint();
        return;
    }

    // decroissance du phosphore
    const float p = persist->load();
    if (p < 0.02f)
        persistImage.clear (persistImage.getBounds(), Colours::black);
    else
        fadeImage (jlimit (0, 255, (int) (std::exp (-dt / (0.004 + (double) p * p * 1.2)) * 256.0)));

    Graphics g (persistImage);
    const float intensity = intens->load() * 1.5f;
    const float fo = focus->load();

    if (xyMode->load() > 0.5f)
        drawXY (g, intensity, fo);
    else
        for (int ch = 0; ch < 2; ++ch)
            if (cp[ch].on->load() > 0.5f)
                drawYT (ch, g, intensity, fo);

    repaint();
}

void ScopeScreen::strokeBeam (Graphics& g, const Path& path, Colour c, float intensity, float focusV, bool heavy)
{
    const float wf = 1.55f - 0.85f * focusV;   // focus eleve = faisceau fin
    auto stroke = [&] (float width, float alpha, Colour colour)
    {
        g.setColour (colour.withAlpha (jlimit (0.0f, 1.0f, alpha * intensity)));
        g.strokePath (path, PathStrokeType (width * wf, PathStrokeType::curved, PathStrokeType::rounded));
    };

    if (! heavy)
    {
        stroke (6.0f, 0.07f, c);      // halo
        stroke (3.2f, 0.16f, c);
    }
    stroke (1.8f, 0.75f, c);                                                  // faisceau
    stroke (0.8f, 0.80f, c.brighter (0.6f).interpolatedWith (Colours::white, 0.5f));   // coeur chaud
}

void ScopeScreen::drawYT (int ch, Graphics& g, float intensity, float focusV)
{
    const double sr = proc.currentSampleRate.load();
    const float vdiv = cp[ch].amp->load();
    const double timeDiv = (double) cp[ch].time->load() * 0.001;     // s / division
    const double N = jlimit (4.0, (double) (ScopeBuffer::size / 4), timeDiv * 10.0 * sr);

    auto& buf = proc.getScope (ch);
    const int64_t w = buf.getWritePos();
    const int64_t need = (int64_t) std::ceil (N) + 8;
    if (w < need + 8)
        return;

    // --- declenchement : on cherche le franchissement de niveau le plus recent
    //     qui laisse assez d'echantillons pour remplir l'ecran.
    const float levelV = cp[ch].level->load() * vdiv;
    const bool falling = cp[ch].slope->load() > 0.5f;
    const int64_t searchLen = jmin (ScopeBuffer::size / 2, jmax ((int64_t) (2.0 * N), (int64_t) (0.06 * sr)));
    const int64_t start = w - need;

    bool found = false;
    double T = 0.0;
    for (int64_t i = start; i > start - searchLen; --i)
    {
        const float a = buf.at (i - 1), b = buf.at (i);
        const bool cross = falling ? (a > levelV && b <= levelV) : (a < levelV && b >= levelV);
        if (cross)
        {
            const double frac = (b != a) ? (double) (levelV - a) / (double) (b - a) : 0.0;
            T = (double) (i - 1) + frac;
            found = true;
            break;
        }
    }

    if (! found)
    {
        if (autoTrig->load() < 0.5f)
            return;                 // mode NORM : pas de declenchement, pas de trace
        T = (double) start;         // mode AUTO : balayage libre
    }

    const double startPos = T - N * 0.1;   // le point de declenchement est place a 1 division du bord gauche

    const int W = getWidth(), H = getHeight();
    const float cy = (float) H * 0.5f, divH = (float) H / 8.0f;
    const float pos = cp[ch].pos->load();
    auto yOf = [&] (float v) { return cy - jlimit (-12.0f, 12.0f, v / vdiv + pos) * divH; };

    Path path;
    const double spp = N / (double) W;

    if (spp > 1.5)
    {
        // beaucoup d'echantillons par pixel : enveloppe min/max pour ne rien perdre
        float prevY = 0.0f;
        bool first = true;
        for (int px = 0; px < W; ++px)
        {
            const int64_t i0 = (int64_t) std::ceil (startPos + px * spp);
            int64_t i1 = (int64_t) std::floor (startPos + (px + 1) * spp);
            if (i1 < i0) i1 = i0;

            float mn = buf.at (i0), mx = mn;
            for (int64_t i = i0 + 1; i <= i1; ++i)
            {
                const float v = buf.at (i);
                mn = jmin (mn, v);
                mx = jmax (mx, v);
            }
            const float yTop = yOf (mx), yBot = yOf (mn);
            const float x = (float) px + 0.5f;

            if (first)
            {
                path.startNewSubPath (x, yTop);
                path.lineTo (x, yBot);
                prevY = yBot;
                first = false;
            }
            else if (std::abs (prevY - yTop) < std::abs (prevY - yBot))
            {
                path.lineTo (x, yTop);
                path.lineTo (x, yBot);
                prevY = yBot;
            }
            else
            {
                path.lineTo (x, yBot);
                path.lineTo (x, yTop);
                prevY = yTop;
            }
        }
    }
    else
    {
        // zoom : interpolation cubique (Catmull-Rom) pour des courbes lisses
        for (int px = 0; px < W; ++px)
        {
            const double sp = startPos + px * spp;
            const int64_t i = (int64_t) std::floor (sp);
            const float t = (float) (sp - (double) i);
            const float y0 = buf.at (i - 1), y1 = buf.at (i), y2 = buf.at (i + 1), y3 = buf.at (i + 2);
            const float v = 0.5f * ((2.0f * y1) + (-y0 + y2) * t
                                    + (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t * t
                                    + (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t * t * t);
            if (px == 0) path.startNewSubPath (0.0f, yOf (v));
            else         path.lineTo ((float) px, yOf (v));
        }
    }

    strokeBeam (g, path, Params::colourFor ((int) cp[ch].colour->load()), intensity, focusV, false);
}

void ScopeScreen::drawXY (Graphics& g, float intensity, float focusV)
{
    const double sr = proc.currentSampleRate.load();
    const double n = jlimit (8.0, (double) (ScopeBuffer::size / 4), (double) xyLen->load() * 0.001 * sr);
    auto& b0 = proc.getScope (0);
    auto& b1 = proc.getScope (1);
    const int64_t w = b0.getWritePos();
    const int count = (int) n;
    if (w < count + 8)
        return;

    const float vx = cp[0].amp->load(), vy = cp[1].amp->load();
    const float px0 = cp[0].pos->load(), py0 = cp[1].pos->load();
    const float cx = (float) getWidth() * 0.5f, cy = (float) getHeight() * 0.5f;
    const float divW = (float) getWidth() / 10.0f, divH = (float) getHeight() / 8.0f;
    const bool ms = rot->load() > 0.5f;
    const int step = jmax (1, count / 20000);
    const float k = 0.70710678f;

    Path path;
    bool first = true;
    for (int j = 0; j < count; j += step)
    {
        const int64_t i = w - count + j;
        const float l = b0.at (i), r = b1.at (i);
        const float xs = ms ? (r - l) * k : l;     // mode M/S : goniometre (mono = ligne verticale)
        const float ys = ms ? (l + r) * k : r;
        const float x = cx + jlimit (-12.0f, 12.0f, xs / vx + px0) * divW;
        const float y = cy - jlimit (-12.0f, 12.0f, ys / vy + py0) * divH;
        if (first) { path.startNewSubPath (x, y); first = false; }
        else       path.lineTo (x, y);
    }

    strokeBeam (g, path, Params::colourFor ((int) cp[0].colour->load()), intensity, focusV, count > 6000);
}

//==============================================================================
void ScopeScreen::drawMarkers (Graphics& g)
{
    // reperes de position (gauche) et de niveau de declenchement (droite)
    if (xyMode->load() > 0.5f)
        return;

    const float w = (float) getWidth();
    const float cy = (float) getHeight() * 0.5f, divH = (float) getHeight() / 8.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        if (cp[ch].on->load() < 0.5f)
            continue;

        const auto c = Params::colourFor ((int) cp[ch].colour->load());
        const float pos = cp[ch].pos->load();

        const float y = cy - pos * divH;
        Rectangle<float> tag (3.0f, y - 7.0f, 14.0f, 14.0f);
        g.setColour (c.withAlpha (0.9f));
        g.fillRoundedRectangle (tag, 2.0f);
        g.setColour (Colours::black);
        g.setFont (uiFont (10.0f, true));
        g.drawText (String (ch + 1), tag.toNearestInt(), Justification::centred);

        const float ty = cy - (cp[ch].level->load() + pos) * divH;
        Path tri;
        tri.addTriangle (w - 3.0f, ty, w - 13.0f, ty - 6.0f, w - 13.0f, ty + 6.0f);
        g.setColour (c.withAlpha (0.85f));
        g.fillPath (tri);
    }
}

void ScopeScreen::paint (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    Path clip;
    clip.addRoundedRectangle (b, 24.0f);
    g.reduceClipRegion (clip, AffineTransform());

    g.fillAll (Colours::black);
    if (persistImage.isNull() || frameImage.isNull())
        return;

    const float curve = fxCurve->load(), scan = fxScan->load(), glitch = fxGlitch->load();
    const bool remap = curve > 0.01f || glitch > 0.001f || ! bands.empty() || vJump != 0;

    // image de l'ecran : phosphore + graticule + reperes
    {
        Graphics fg (frameImage);
        fg.drawImageAt (persistImage, 0, 0);
        fg.setOpacity (jmap (illum->load(), 0.15f, 0.9f));
        fg.drawImageAt (graticule, 0, 0);
        drawMarkers (fg);
    }

    if (remap)
    {
        if (mapW != getWidth() || mapH != getHeight() || std::abs (curve - mapCurve) > 0.002f)
            rebuildMap (curve);
        applyRemap();
        g.drawImageAt (outImage, 0, 0);
    }
    else
    {
        g.drawImageAt (frameImage, 0, 0);
    }

    // lignes de balayage
    if (scan > 0.01f)
    {
        if (std::abs (scan - scanBuiltFor) > 0.004f || scanImage.isNull())
            rebuildScan (scan);
        g.drawImageAt (scanImage, 0, 0);
    }

    // verre : vignettage et reflet
    ColourGradient vignette (Colours::transparentBlack, b.getCentre(), Colour (0xaa000000), { 0.0f, 0.0f }, true);
    vignette.addColour (0.62, Colours::transparentBlack);
    g.setGradientFill (vignette);
    g.fillRect (b);

    g.setGradientFill (ColourGradient (Colour (0x16ffffff), 0.0f, 0.0f, Colour (0x00ffffff), 0.0f, b.getHeight() * 0.45f, false));
    g.fillRect (b);
}
