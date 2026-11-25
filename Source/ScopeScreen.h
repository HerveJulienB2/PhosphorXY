#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "PluginProcessor.h"

// L'ecran cathodique : graticule 10 x 8 divisions, phosphore avec persistance,
// modes YT (deux traces, base de temps et declenchement propres a chaque voie) et XY.
// Effets d'image (affichage seulement) : courbure du tube, lignes de balayage, glitchs numeriques.
class ScopeScreen : public juce::Component
{
public:
    explicit ScopeScreen (PhosphorProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct ChannelParams
    {
        std::atomic<float> *on, *amp, *time, *pos, *level, *slope, *colour;
    };

    struct GlitchBand
    {
        int y0, h, dx, split, left;
    };

    void update();
    void fadeImage (int k256);
    void rebuildGraticule();
    void rebuildMap (float curve);
    void rebuildScan (float amount);
    void updateGlitch (float amount);
    void applyRemap();
    void drawMarkers (juce::Graphics&);
    void drawYT (int ch, juce::Graphics& g, float intensity, float focus);
    void drawXY (juce::Graphics& g, float intensity, float focus);
    static void strokeBeam (juce::Graphics&, const juce::Path&, juce::Colour, float intensity, float focus, bool heavy);

    PhosphorProcessor& proc;
    ChannelParams cp[2] {};
    std::atomic<float> *xyMode, *rot, *hold, *autoTrig, *xyLen, *intens, *focus, *persist, *illum,
                       *fxCurve, *fxScan, *fxGlitch;

    juce::Image persistImage, graticule, frameImage, outImage, scanImage;
    double lastMs = 0.0;

    // courbure : table de remappage des pixels (virgule fixe 16.16)
    std::vector<int> mapX, mapY;
    float mapCurve = -1.0f;
    int mapW = 0, mapH = 0;
    float scanBuiltFor = -1.0f;

    // glitchs
    juce::Random rng;
    std::vector<GlitchBand> bands;
    std::vector<int> rowDx, rowSplit;
    int vJump = 0, vJumpLeft = 0;

    juce::VBlankAttachment vblank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeScreen)
};
