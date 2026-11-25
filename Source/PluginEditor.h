#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ScopeScreen.h"
#include "HamegLookAndFeel.h"

using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

//==============================================================================
// Bouton rotatif avec son etiquette serigraphiee et sa valeur numerique.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& text)
    {
        title.setText (text, juce::dontSendNotification);
        title.setJustificationType (juce::Justification::centred);
        title.setFont (uiFont (11.0f, true));
        addAndMakeVisible (title);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 16);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);

        if (auto* p = state.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

        attachment = std::make_unique<SliderAttachment> (state, paramId, slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        title.setBounds (r.removeFromTop (16));
        slider.setBounds (r);
    }

private:
    juce::Label title;
    juce::Slider slider;
    std::unique_ptr<SliderAttachment> attachment;
};

// Touche a LED liee a un parametre booleen.
class Key
{
public:
    Key (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& text,
         const juce::String& onText = {}, const juce::String& offText = {}, bool greenLed = false)
    {
        button.setButtonText (text);
        if (onText.isNotEmpty())  button.getProperties().set ("onText", onText);
        if (offText.isNotEmpty()) button.getProperties().set ("offText", offText);
        if (greenLed)             button.getProperties().set ("ledGreen", true);
        attachment = std::make_unique<ButtonAttachment> (state, paramId, button);
    }

    juce::ToggleButton button;

private:
    std::unique_ptr<ButtonAttachment> attachment;
};

//==============================================================================
// Section d'une voie : amplitude, base de temps, position, niveau de declenchement.
class ChannelPanel : public juce::Component
{
public:
    ChannelPanel (juce::AudioProcessorValueTreeState& state, int ch)
        : channel (ch),
          amp   (state, Params::id (ch, "amp"),   "V/DIV"),
          time  (state, Params::id (ch, "time"),  "TIME/DIV"),
          pos   (state, Params::id (ch, "pos"),   "Y-POS"),
          level (state, Params::id (ch, "level"), "TRIG LEVEL"),
          onKey    (state, Params::id (ch, "on"),    "ON", {}, {}, true),
          slopeKey (state, Params::id (ch, "slope"), "RISE", "FALL", "RISE")
    {
        for (auto* k : { &amp, &time, &pos, &level })
            addAndMakeVisible (*k);

        addAndMakeVisible (onKey.button);
        addAndMakeVisible (slopeKey.button);

        colour.addItemList (Params::colourNames, 1);
        colourAttachment = std::make_unique<ComboBoxAttachment> (state, Params::id (ch, "color"), colour);
        colour.onChange = [this] { repaint(); };
        addAndMakeVisible (colour);
    }

    void paint (juce::Graphics& g) override
    {
        const auto accent = Params::colourFor (juce::jmax (0, colour.getSelectedItemIndex()));
        HamegLookAndFeel::drawSection (g, getLocalBounds().toFloat().withTrimmedTop (9.0f),
                                       "CH" + juce::String (channel + 1), accent);
    }

    void resized() override
    {
        auto r = getLocalBounds().withTrimmedTop (9).reduced (5, 0);
        r.removeFromTop (24);

        auto row1 = r.removeFromTop (108);
        amp.setBounds (row1.removeFromLeft (100));
        time.setBounds (row1.removeFromLeft (100));

        auto row2 = r.removeFromTop (108);
        pos.setBounds (row2.removeFromLeft (100));
        level.setBounds (row2.removeFromLeft (100));

        r.removeFromTop (6);
        auto btn = r.removeFromTop (28);
        onKey.button.setBounds (btn.removeFromLeft (56));
        btn.removeFromLeft (4);
        slopeKey.button.setBounds (btn.removeFromLeft (56));
        btn.removeFromLeft (4);
        colour.setBounds (btn.reduced (0, 2));
    }

private:
    int channel;
    Knob amp, time, pos, level;
    Key onKey, slopeKey;
    juce::ComboBox colour;
    std::unique_ptr<ComboBoxAttachment> colourAttachment;
};

//==============================================================================
// Section affichage : intensite, focus, persistance, eclairage, mode XY, declenchement.
class DisplayPanel : public juce::Component
{
public:
    explicit DisplayPanel (juce::AudioProcessorValueTreeState& state)
        : intens  (state, "intens",  "INTENS"),
          focus   (state, "focus",   "FOCUS"),
          persist (state, "persist", "PERSIST"),
          illum   (state, "illum",   "ILLUM"),
          xyLen   (state, "xy_len",  "XY LEN"),
          xyKey   (state, "xy",   "XY"),
          rotKey  (state, "rot",  "M/S ROT"),
          holdKey (state, "hold", "HOLD"),
          trigKey (state, "auto", "TRIG", "AUTO", "NORM", true)
    {
        for (auto* k : { &intens, &focus, &persist, &illum, &xyLen })
            addAndMakeVisible (*k);
        for (auto* k : { &xyKey, &rotKey, &holdKey, &trigKey })
            addAndMakeVisible (k->button);
    }

    void paint (juce::Graphics& g) override
    {
        HamegLookAndFeel::drawSection (g, getLocalBounds().toFloat().withTrimmedTop (9.0f), "DISPLAY");
    }

    void resized() override
    {
        auto r = getLocalBounds().withTrimmedTop (9).reduced (6, 0);
        r.removeFromTop (24);

        auto row = r.removeFromTop (108);
        const int kw = row.getWidth() / 5;
        for (auto* k : { &intens, &focus, &persist, &illum, &xyLen })
            k->setBounds (row.removeFromLeft (kw));

        r.removeFromTop (8);
        auto btn = r.removeFromTop (30);
        const int bw = (btn.getWidth() - 3 * 8) / 4;
        for (auto* k : { &xyKey, &rotKey, &holdKey, &trigKey })
        {
            k->button.setBounds (btn.removeFromLeft (bw));
            btn.removeFromLeft (8);
        }
    }

private:
    Knob intens, focus, persist, illum, xyLen;
    Key xyKey, rotKey, holdKey, trigKey;
};

//==============================================================================
// Afficheur sous l'ecran (style "readout" des oscilloscopes numeriques HAMEG).
class Readout : public juce::Component, private juce::Timer
{
public:
    explicit Readout (juce::AudioProcessorValueTreeState& s) : state (s) { startTimerHz (10); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour (0xff6dff8a));
        g.setFont (monoFont (15.0f));
        g.drawText (line1, 12, 6, getWidth() - 24, 20, juce::Justification::centredLeft);
        g.drawText (line2, 12, 28, getWidth() - 24, 20, juce::Justification::centredLeft);
        if (state.getRawParameterValue ("hold")->load() > 0.5f)
            g.drawText ("HOLD", 12, 6, getWidth() - 24, 20, juce::Justification::centredRight);
    }

private:
    juce::String text (const juce::String& id) const { return state.getParameter (id)->getCurrentValueAsText(); }
    bool flag (const juce::String& id) const { return state.getRawParameterValue (id)->load() > 0.5f; }

    juce::String channelLine (int ch) const
    {
        if (! flag (Params::id (ch, "on")))
            return "CH" + juce::String (ch + 1) + "  OFF";

        return "CH" + juce::String (ch + 1) + "  " + text (Params::id (ch, "amp")) + "/DIV  "
               + text (Params::id (ch, "time")) + "/DIV  "
               + (flag (Params::id (ch, "slope")) ? "FALL" : "RISE");
    }

    void timerCallback() override
    {
        juce::String a, b;
        if (flag ("xy"))
        {
            a = "XY  X=CH1 " + text ("ch1_amp") + "/DIV  Y=CH2 " + text ("ch2_amp") + "/DIV";
            b = "TRACE " + text ("xy_len") + (flag ("rot") ? "  M/S 45deg" : "");
        }
        else
        {
            a = channelLine (0);
            b = channelLine (1) + "  TRIG " + (flag ("auto") ? "AUTO" : "NORM");
        }

        if (a != line1 || b != line2)
        {
            line1 = a;
            line2 = b;
        }
        repaint();
    }

    juce::AudioProcessorValueTreeState& state;
    juce::String line1, line2;
};

//==============================================================================
// Contenu de taille logique fixe (1160 x 640) : l'editeur le met simplement a l'echelle.
class EditorContent : public juce::Component
{
public:
    static constexpr int W = 1160, H = 640;

    explicit EditorContent (PhosphorProcessor& p);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Image noise;
    ScopeScreen screen;
    Readout readout;
    ChannelPanel ch1, ch2;
    DisplayPanel display;
};

class PhosphorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PhosphorEditor (PhosphorProcessor&);
    ~PhosphorEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

private:
    HamegLookAndFeel laf;
    EditorContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhosphorEditor)
};
