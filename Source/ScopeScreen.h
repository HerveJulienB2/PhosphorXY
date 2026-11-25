#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// L'ecran cathodique : graticule 10 x 8 divisions, phosphore avec persistance,
// modes YT (deux traces, base de temps et declenchement propres a chaque voie) et XY.
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

    void update();
    void fadeImage (int k256);
    void rebuildGraticule();
    void drawYT (int ch, juce::Graphics& g, float intensity, float focus);
    void drawXY (juce::Graphics& g, float intensity, float focus);
    static void strokeBeam (juce::Graphics&, const juce::Path&, juce::Colour, float intensity, float focus, bool heavy);

    PhosphorProcessor& proc;
    ChannelParams cp[2] {};
    std::atomic<float> *xyMode, *rot, *hold, *autoTrig, *xyLen, *intens, *focus, *persist, *illum;

    juce::Image persistImage, graticule;
    double lastMs = 0.0;

    juce::VBlankAttachment vblank;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScopeScreen)
};
