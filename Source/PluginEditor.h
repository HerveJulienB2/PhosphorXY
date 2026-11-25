#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ScopeScreen.h"
#include "BraunLookAndFeel.h"

using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

//==============================================================================
// Bouton rotatif avec son etiquette et sa valeur. "metal" : aluminium brosse pour les reglages principaux.
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& text,
          bool metal = false)
    {
        title.setText (text, juce::dontSendNotification);
        title.setJustificationType (juce::Justification::centred);
        title.setFont (uiFont (10.5f, true));
        addAndMakeVisible (title);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 16);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        slider.getProperties().set ("metal", metal);
        addAndMakeVisible (slider);

        if (auto* p = state.getParameter (paramId))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));

        attachment = std::make_unique<SliderAttachment> (state, paramId, slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        title.setBounds (r.removeFromTop (14));
        slider.setBounds (r);
    }

private:
    juce::Label title;
    juce::Slider slider;
    std::unique_ptr<SliderAttachment> attachment;
};

// Touche a temoin lumineux liee a un parametre booleen.
class Key
{
public:
    Key (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& text,
         const juce::String& onText = {}, const juce::String& offText = {})
    {
        button.setButtonText (text);
        if (onText.isNotEmpty())  button.getProperties().set ("onText", onText);
        if (offText.isNotEmpty()) button.getProperties().set ("offText", offText);
        attachment = std::make_unique<ButtonAttachment> (state, paramId, button);
    }

    juce::ToggleButton button;

private:
    std::unique_ptr<ButtonAttachment> attachment;
};

// Liste deroulante liee a un parametre a choix.
class Choice
{
public:
    Choice (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::StringArray& items)
    {
        box.addItemList (items, 1);
        attachment = std::make_unique<ComboBoxAttachment> (state, paramId, box);
    }

    juce::ComboBox box;

private:
    std::unique_ptr<ComboBoxAttachment> attachment;
};

//==============================================================================
// Une voie : amplitude, base de temps, position, niveau de declenchement.
class ChannelPanel : public juce::Component
{
public:
    ChannelPanel (juce::AudioProcessorValueTreeState& state, int ch)
        : channel (ch),
          amp   (state, Params::id (ch, "amp"),   "V/DIV", true),
          time  (state, Params::id (ch, "time"),  "TIME/DIV", true),
          pos   (state, Params::id (ch, "pos"),   "Y-POS"),
          level (state, Params::id (ch, "level"), "TRIG LEVEL"),
          onKey    (state, Params::id (ch, "on"),    "ON"),
          slopeKey (state, Params::id (ch, "slope"), "RISE", "FALL", "RISE"),
          colour   (state, Params::id (ch, "color"), Params::colourNames)
    {
        for (auto* k : { &amp, &time, &pos, &level })
            addAndMakeVisible (*k);

        addAndMakeVisible (onKey.button);
        addAndMakeVisible (slopeKey.button);
        colour.box.onChange = [this] { repaint(); };
        addAndMakeVisible (colour.box);
    }

    void paint (juce::Graphics& g) override
    {
        const auto swatch = Params::colourFor (juce::jmax (0, colour.box.getSelectedItemIndex()));
        BraunLookAndFeel::drawHeader (g, getLocalBounds().toFloat(), "CHANNEL " + juce::String (channel + 1)
                                                                      + (channel == 0 ? "  (LEFT)" : "  (RIGHT)"), swatch);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 0);
        r.removeFromTop (24);

        auto row1 = r.removeFromTop (100);
        amp.setBounds (row1.removeFromLeft (row1.getWidth() / 2));
        time.setBounds (row1);

        auto row2 = r.removeFromTop (100);
        pos.setBounds (row2.removeFromLeft (row2.getWidth() / 2));
        level.setBounds (row2);

        r.removeFromTop (6);
        auto btn = r.removeFromTop (28);
        onKey.button.setBounds (btn.removeFromLeft (58));
        btn.removeFromLeft (6);
        slopeKey.button.setBounds (btn.removeFromLeft (58));
        btn.removeFromLeft (6);
        colour.box.setBounds (btn.reduced (0, 2));
    }

private:
    int channel;
    Knob amp, time, pos, level;
    Key onKey, slopeKey;
    Choice colour;
};

//==============================================================================
// Affichage : intensite, focus, persistance, eclairage, mode XY, declenchement.
class DisplayPanel : public juce::Component
{
public:
    explicit DisplayPanel (juce::AudioProcessorValueTreeState& state)
        : intens  (state, "intens",  "INTENSITY", true),
          focus   (state, "focus",   "FOCUS"),
          persist (state, "persist", "PERSIST"),
          illum   (state, "illum",   "GRID LIGHT"),
          xyLen   (state, "xy_len",  "XY TRACE"),
          xyKey   (state, "xy",   "XY MODE"),
          rotKey  (state, "rot",  "M/S 45"),
          holdKey (state, "hold", "HOLD"),
          trigKey (state, "auto", "TRIGGER", "AUTO", "NORMAL")
    {
        for (auto* k : { &intens, &focus, &persist, &illum, &xyLen })
            addAndMakeVisible (*k);
        for (auto* k : { &xyKey, &rotKey, &holdKey, &trigKey })
            addAndMakeVisible (k->button);
    }

    void paint (juce::Graphics& g) override
    {
        BraunLookAndFeel::drawHeader (g, getLocalBounds().toFloat(), "DISPLAY");
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 0);
        r.removeFromTop (24);

        auto row = r.removeFromTop (100);
        const int kw = row.getWidth() / 5;
        for (auto* k : { &intens, &focus, &persist, &illum, &xyLen })
            k->setBounds (row.removeFromLeft (kw));

        r.removeFromTop (6);
        auto btn = r.removeFromTop (28);
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
// Effets d'image : n'agissent que sur l'affichage, jamais sur le son.
class FxPanel : public juce::Component
{
public:
    explicit FxPanel (juce::AudioProcessorValueTreeState& state)
        : curve  (state, "fx_curve",  "CURVATURE"),
          scan   (state, "fx_scan",   "SCANLINES"),
          glitch (state, "fx_glitch", "GLITCH")
    {
        for (auto* k : { &curve, &scan, &glitch })
            addAndMakeVisible (*k);
    }

    void paint (juce::Graphics& g) override
    {
        BraunLookAndFeel::drawHeader (g, getLocalBounds().toFloat(), "IMAGE EFFECTS");
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6, 0);
        r.removeFromTop (24);
        auto row = r.removeFromTop (100);
        const int kw = row.getWidth() / 3;
        for (auto* k : { &curve, &scan, &glitch })
            k->setBounds (row.removeFromLeft (kw));
    }

private:
    Knob curve, scan, glitch;
};

//==============================================================================
// Apercu de la forme generee : le contour du texte (DRAW) ou la suite de notes (TONE).
class ShapePreview : public juce::Component, private juce::Timer
{
public:
    explicit ShapePreview (PhosphorProcessor& p) : proc (p) { startTimerHz (15); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0b0d0c));
        g.fillRoundedRectangle (b, 6.0f);
        g.setColour (Theme::line);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

        const auto inner = b.reduced (10.0f);
        g.setColour (juce::Colour (0xffbfe3c8).withAlpha (0.18f));
        g.drawHorizontalLine ((int) inner.getCentreY(), inner.getX(), inner.getRight());
        g.drawVerticalLine ((int) inner.getCentreX(), inner.getY(), inner.getBottom());

        const auto* t = proc.getTable();
        if (t == nullptr)
            return;

        if (lastMode == 0)
        {
            if (t->x.empty())
                return;

            float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
            for (size_t i = 0; i < t->x.size(); ++i)
            {
                minX = juce::jmin (minX, t->x[i]);  maxX = juce::jmax (maxX, t->x[i]);
                minY = juce::jmin (minY, t->y[i]);  maxY = juce::jmax (maxY, t->y[i]);
            }

            const float sw = maxX - minX, sh = maxY - minY;
            const float s = juce::jmin (inner.getWidth() / juce::jmax (0.001f, sw), inner.getHeight() / juce::jmax (0.001f, sh));
            const float cx = inner.getCentreX() - 0.5f * (minX + maxX) * s;
            const float cy = inner.getCentreY() + 0.5f * (minY + maxY) * s;

            juce::Path path;
            for (size_t i = 0; i < t->x.size(); i += 4)
            {
                const float x = cx + t->x[i] * s, y = cy - t->y[i] * s;
                if (i == 0) path.startNewSubPath (x, y);
                else        path.lineTo (x, y);
            }
            path.closeSubPath();

            g.setColour (Theme::crtGreen.withAlpha (0.25f));
            g.strokePath (path, juce::PathStrokeType (3.5f));
            g.setColour (Theme::crtGreen);
            g.strokePath (path, juce::PathStrokeType (1.2f));
        }
        else
        {
            const int n = (int) t->codes.size();
            if (n == 0)
                return;

            const float bw = inner.getWidth() / (float) n;
            for (int i = 0; i < n; ++i)
            {
                const int code = t->codes[(size_t) i];
                auto bar = juce::Rectangle<float> (inner.getX() + (float) i * bw + 1.0f, 0.0f, juce::jmax (2.0f, bw - 2.0f), 0.0f);
                if (GeneratorEngine::isRest (code))
                {
                    g.setColour (Theme::crtGreen.withAlpha (0.3f));
                    g.fillRect (bar.getX(), inner.getBottom() - 1.0f, bar.getWidth(), 1.0f);
                    continue;
                }

                const float h = inner.getHeight() * (float) (GeneratorEngine::semitone (code, lastScale) + 3) / 40.0f;
                bar.setY (inner.getBottom() - h);
                bar.setHeight (h);
                g.setColour (Theme::crtGreen.withAlpha (0.85f));
                g.fillRect (bar);
            }
        }
    }

private:
    void timerCallback() override
    {
        const auto* t = proc.getTable();
        const int mode = juce::roundToInt (proc.apvts.getRawParameterValue ("gen_mode")->load());
        const int scale = juce::roundToInt (proc.apvts.getRawParameterValue ("gen_scale")->load());
        if (t != lastTable || mode != lastMode || scale != lastScale)
        {
            lastTable = t;
            lastMode = mode;
            lastScale = scale;
            repaint();
        }
    }

    PhosphorProcessor& proc;
    const GenTable* lastTable = nullptr;
    int lastMode = 0, lastScale = 2;
};

//==============================================================================
// Generateur : texte -> forme d'onde. Le plugin reste d'abord un oscilloscope ; la source par defaut est l'entree.
class GeneratorPanel : public juce::Component, private juce::Timer
{
public:
    explicit GeneratorPanel (PhosphorProcessor& p)
        : proc (p),
          pitch (p.apvts, "gen_freq",  "PITCH", true),
          step  (p.apvts, "gen_step",  "STEP"),
          level (p.apvts, "gen_level", "LEVEL"),
          mode  (p.apvts, "gen_mode",  Params::genModeNames),
          source (p.apvts, "gen_src",  Params::genSrcNames),
          scale (p.apvts, "gen_scale", Params::genScaleNames),
          preview (p)
    {
        using juce::Colour;
        text.setMultiLine (false);
        text.setReturnKeyStartsNewLine (false);
        text.setInputRestrictions (40);
        text.setFont (monoFont (17.0f));
        text.setColour (juce::TextEditor::backgroundColourId, Colour (0xff0b0d0c));
        text.setColour (juce::TextEditor::textColourId, Theme::crtGreen);
        text.setColour (juce::TextEditor::outlineColourId, Theme::line);
        text.setColour (juce::TextEditor::focusedOutlineColourId, Theme::accent);
        text.setColour (juce::TextEditor::highlightColourId, Theme::accent.withAlpha (0.45f));
        text.setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        text.setColour (juce::CaretComponent::caretColourId, Theme::crtGreen);
        text.setIndents (10, 6);
        text.setTextToShowWhenEmpty ("type a word or a sentence", Theme::inkSoft);
        text.setText (proc.getGeneratorText(), false);
        text.onTextChange = [this] { proc.setGeneratorText (text.getText()); };

        addAndMakeVisible (text);
        addAndMakeVisible (mode.box);
        addAndMakeVisible (source.box);
        addAndMakeVisible (scale.box);
        addAndMakeVisible (pitch);
        addAndMakeVisible (step);
        addAndMakeVisible (level);
        addAndMakeVisible (preview);

        startTimerHz (8);
    }

    void paint (juce::Graphics& g) override
    {
        BraunLookAndFeel::drawHeader (g, getLocalBounds().toFloat(), "GENERATOR   -   TEXT TO WAVEFORM");

        g.setColour (Theme::inkSoft);
        g.setFont (uiFont (9.5f, true));
        g.drawText ("TEXT", textArea.withHeight (12).translated (0, -13), juce::Justification::bottomLeft);
        g.drawText ("MODE",   mode.box.getBounds().withHeight (12).translated (0, -13), juce::Justification::bottomLeft);
        g.drawText ("SOURCE", source.box.getBounds().withHeight (12).translated (0, -13), juce::Justification::bottomLeft);
        g.drawText ("SCALE (TONE)", scale.box.getBounds().withHeight (12).translated (0, -13), juce::Justification::bottomLeft);
        g.drawText ("PREVIEW", preview.getBounds().withHeight (12).translated (0, -13), juce::Justification::bottomLeft);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (14, 0);
        r.removeFromTop (38);

        auto colA = r.removeFromLeft (440);
        textArea = colA.removeFromTop (32);
        text.setBounds (textArea);
        colA.removeFromTop (28);
        auto combos = colA.removeFromTop (26);
        mode.box.setBounds (combos.removeFromLeft (140));
        combos.removeFromLeft (10);
        source.box.setBounds (combos.removeFromLeft (140));
        combos.removeFromLeft (10);
        scale.box.setBounds (combos.removeFromLeft (140));

        r.removeFromLeft (16);
        auto knobs = r.removeFromLeft (300);
        knobs.setTop (getHeight() - 112);
        knobs.setHeight (100);
        pitch.setBounds (knobs.removeFromLeft (100));
        step.setBounds (knobs.removeFromLeft (100));
        level.setBounds (knobs.removeFromLeft (100));

        r.removeFromLeft (18);
        auto pv = r;
        pv.setTop (getHeight() - 112 + 14);
        pv.setHeight (86);
        preview.setBounds (pv);
    }

private:
    void timerCallback() override
    {
        // la duree des notes et la gamme ne servent qu'en mode TONE
        const bool tone = proc.apvts.getRawParameterValue ("gen_mode")->load() > 0.5f;
        step.setEnabled (tone);
        step.setAlpha (tone ? 1.0f : 0.4f);
        scale.box.setEnabled (tone);
        scale.box.setAlpha (tone ? 1.0f : 0.4f);

        // suit le texte si le projet vient d'etre recharge
        if (! text.hasKeyboardFocus (true))
        {
            const auto t = proc.getGeneratorText();
            if (t != text.getText())
                text.setText (t, false);
        }
    }

    PhosphorProcessor& proc;
    juce::TextEditor text;
    juce::Rectangle<int> textArea;
    Knob pitch, step, level;
    Choice mode, source, scale;
    ShapePreview preview;
};

//==============================================================================
// Afficheur sous l'ecran (lecture des reglages, facon "readout").
class Readout : public juce::Component, private juce::Timer
{
public:
    explicit Readout (juce::AudioProcessorValueTreeState& s) : state (s) { startTimerHz (10); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::crtGreen);
        g.setFont (monoFont (15.0f));
        g.drawText (line1, 12, 6, getWidth() - 24, 20, juce::Justification::centredLeft);
        g.drawText (line2, 12, 28, getWidth() - 24, 20, juce::Justification::centredLeft);
        if (flag ("hold"))
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

        if (state.getRawParameterValue ("gen_src")->load() > 0.5f)
            b += "  GEN " + text ("gen_mode").upToFirstOccurrenceOf (" ", false, false);

        line1 = a;
        line2 = b;
        repaint();
    }

    juce::AudioProcessorValueTreeState& state;
    juce::String line1, line2;
};

//==============================================================================
// Contenu de taille logique fixe : l'editeur le met simplement a l'echelle.
class EditorContent : public juce::Component
{
public:
    static constexpr int W = 1320, H = 824;

    explicit EditorContent (PhosphorProcessor& p);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Image noise;
    ScopeScreen screen;
    Readout readout;
    ChannelPanel ch1, ch2;
    DisplayPanel display;
    FxPanel fx;
    GeneratorPanel generator;
};

class PhosphorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PhosphorEditor (PhosphorProcessor&);
    ~PhosphorEditor() override;

    void paint (juce::Graphics&) override {}
    void resized() override;

private:
    BraunLookAndFeel laf;
    EditorContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhosphorEditor)
};
