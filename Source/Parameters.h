#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cmath>

// Identifiants et description de tous les parametres du plugin.
namespace Params
{
    inline juce::String id (int ch, const juce::String& name)
    {
        return "ch" + juce::String (ch + 1) + "_" + name;
    }

    inline const juce::StringArray colourNames { "Green", "Amber", "Cyan", "White", "Magenta", "Red" };
    inline const juce::StringArray genModeNames  { "DRAW (XY)", "TONE (NOTES)" };
    inline const juce::StringArray genSrcNames   { "INPUT ONLY", "INPUT + GEN", "GEN ONLY" };
    inline const juce::StringArray genScaleNames { "MAJOR", "MINOR", "PENTATONIC", "CHROMATIC" };

    inline juce::Colour colourFor (int index)
    {
        static const juce::uint32 table[] = { 0xff4dff7a, 0xffffb52e, 0xff3de6ff,
                                              0xffe8f4ff, 0xffff5ce1, 0xffff5050 };
        return juce::Colour (table[juce::jlimit (0, 5, index)]);
    }

    inline juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        return juce::NormalisableRange<float> (
            lo, hi,
            [] (float s, float e, float v) { return s * std::pow (e / s, v); },
            [] (float s, float e, float v) { return std::log (juce::jlimit (s, e, v) / s) / std::log (e / s); },
            [] (float s, float e, float v) { return juce::jlimit (s, e, v); });
    }

    inline juce::String mu() { return juce::String::fromUTF8 ("\xc2\xb5"); }

    inline juce::String voltsText (float v)
    {
        if (v < 1.0f)
            return juce::String (juce::roundToInt (v * 1000.0f)) + " mV";
        return juce::String (v, v < 10.0f ? 2 : 1) + " V";
    }

    inline float voltsFromText (const juce::String& t)
    {
        const float v = t.getFloatValue();
        return t.containsIgnoreCase ("mv") ? v / 1000.0f : v;
    }

    // la valeur du parametre est en millisecondes par division
    inline juce::String timeText (float ms)
    {
        if (ms < 1.0f)    return juce::String (juce::roundToInt (ms * 1000.0f)) + " " + mu() + "s";
        if (ms < 10.0f)   return juce::String (ms, 1) + " ms";
        if (ms < 1000.0f) return juce::String (juce::roundToInt (ms)) + " ms";
        return juce::String (ms / 1000.0f, 1) + " s";
    }

    inline float timeFromText (const juce::String& t)
    {
        const float v = t.getFloatValue();
        if (t.containsIgnoreCase ("us") || t.contains (mu()))  return v / 1000.0f;
        if (t.containsIgnoreCase ("ms"))                       return v;
        if (t.containsIgnoreCase ("s"))                        return v * 1000.0f;
        return v;
    }

    inline juce::String divText (float v, int) { return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " div"; }
    inline juce::String percentText (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        for (int ch = 0; ch < 2; ++ch)
        {
            const String n = "CH" + String (ch + 1) + " ";

            layout.add (std::make_unique<AudioParameterBool> (ParameterID { id (ch, "on"), 1 }, n + "On", true));

            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID { id (ch, "amp"), 1 }, n + "Volts/Div", logRange (0.005f, 5.0f), 0.5f,
                AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([] (float v, int) { return voltsText (v); })
                    .withValueFromStringFunction ([] (const String& t) { return voltsFromText (t); })));

            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID { id (ch, "time"), 1 }, n + "Time/Div", logRange (0.01f, 200.0f), 1.0f,
                AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([] (float v, int) { return timeText (v); })
                    .withValueFromStringFunction ([] (const String& t) { return timeFromText (t); })));

            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID { id (ch, "pos"), 1 }, n + "Y Position", NormalisableRange<float> (-4.0f, 4.0f, 0.01f),
                ch == 0 ? 2.0f : -2.0f,
                AudioParameterFloatAttributes().withStringFromValueFunction (divText)));

            layout.add (std::make_unique<AudioParameterFloat> (
                ParameterID { id (ch, "level"), 1 }, n + "Trigger Level", NormalisableRange<float> (-4.0f, 4.0f, 0.01f),
                0.0f, AudioParameterFloatAttributes().withStringFromValueFunction (divText)));

            layout.add (std::make_unique<AudioParameterBool> (ParameterID { id (ch, "slope"), 1 }, n + "Slope Falling", false));

            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id (ch, "color"), 1 }, n + "Colour",
                                                                colourNames, ch == 0 ? 0 : 1));
        }

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "xy", 1 }, "XY Mode", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "rot", 1 }, "XY Mid/Side Rotation", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "hold", 1 }, "Hold", false));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "auto", 1 }, "Trigger Auto", true));

        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { "xy_len", 1 }, "XY Trace Length", logRange (1.0f, 200.0f), 30.0f,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })
                .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));

        auto unit = NormalisableRange<float> (0.0f, 1.0f, 0.001f);
        auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (percentText);
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "intens", 1 }, "Intensity", unit, 0.7f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "focus", 1 }, "Focus", unit, 0.7f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "persist", 1 }, "Persistence", unit, 0.25f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "illum", 1 }, "Graticule Illumination", unit, 0.6f, pct));

        // generateur : texte -> forme d'onde (le texte lui-meme est stocke dans l'etat du plugin)
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "gen_src", 1 }, "Generator Source", genSrcNames, 0));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "gen_mode", 1 }, "Generator Mode", genModeNames, 0));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "gen_scale", 1 }, "Generator Scale", genScaleNames, 2));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "gen_level", 1 }, "Generator Level", unit, 0.8f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { "gen_freq", 1 }, "Generator Pitch", logRange (20.0f, 2000.0f), 110.0f,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; })
                .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { "gen_step", 1 }, "Generator Step", logRange (60.0f, 1000.0f), 220.0f,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; })
                .withValueFromStringFunction ([] (const String& t) { return t.getFloatValue(); })));

        // effets d'image (affichage seulement, n'affectent jamais le son)
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "fx_curve", 1 }, "FX Curvature", unit, 0.35f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "fx_scan", 1 }, "FX Scanlines", unit, 0.3f, pct));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "fx_glitch", 1 }, "FX Glitch", unit, 0.0f, pct));

        return layout;
    }
}
