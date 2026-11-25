#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

inline juce::Font uiFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font monoFont (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

// Face avant inspiree des oscilloscopes analogiques HAMEG :
// gris anthracite, serigraphie blanche, boutons noirs a index blanc, touches grises avec LED.
class HamegLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HamegLookAndFeel()
    {
        using juce::Colour;
        setColour (juce::Slider::textBoxTextColourId, Colour (0xffe8ecef));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, Colour (0x59000000));
        setColour (juce::Slider::textBoxHighlightColourId, Colour (0xff4d90ff));
        setColour (juce::Label::textColourId, Colour (0xffe8ecef));
        setColour (juce::ComboBox::backgroundColourId, Colour (0xff1c1e20));
        setColour (juce::ComboBox::textColourId, Colour (0xffe8ecef));
        setColour (juce::ComboBox::outlineColourId, Colour (0xff6b6f73));
        setColour (juce::ComboBox::arrowColourId, Colour (0xffe8ecef));
        setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1c1e20));
        setColour (juce::PopupMenu::textColourId, Colour (0xffe8ecef));
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Colour (0xff3a5f8f));
    }

    juce::Font getLabelFont (juce::Label&) override { return uiFont (11.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return uiFont (11.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        using juce::Colour;
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float outerR = size * 0.5f;

        // graduations serigraphiees
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const bool major = (i % 5) == 0;
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (outerR * (major ? 0.78f : 0.84f), a),
                                           c.getPointOnCircumference (outerR * 0.96f, a)),
                        major ? 1.5f : 1.0f);
        }

        const float knobR = outerR * 0.68f;

        // ombre portee
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - knobR + 1.5f, c.y - knobR + 3.0f, knobR * 2.0f, knobR * 2.0f);

        // jupe
        g.setGradientFill (juce::ColourGradient (Colour (0xff53565a), c.x - knobR * 0.5f, c.y - knobR * 0.7f,
                                                 Colour (0xff0b0c0d), c.x + knobR * 0.7f, c.y + knobR * 0.9f, false));
        g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);

        // moletage
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        for (int i = 0; i < 36; ++i)
        {
            const float a = (float) i / 36.0f * juce::MathConstants<float>::twoPi;
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (knobR * 0.86f, a),
                                           c.getPointOnCircumference (knobR * 0.98f, a)), 1.0f);
        }

        // capuchon
        const float capR = knobR * 0.78f;
        g.setGradientFill (juce::ColourGradient (Colour (0xff303235), c.x - capR * 0.6f, c.y - capR * 0.8f,
                                                 Colour (0xff060606), c.x + capR * 0.6f, c.y + capR, false));
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2.0f, capR * 2.0f, 1.0f);

        // index blanc
        const float a = startAngle + sliderPos * (endAngle - startAngle);
        g.setColour (juce::Colours::white);
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (capR * 0.30f, a),
                                       c.getPointOnCircumference (knobR * 0.96f, a)), 2.6f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool down) override
    {
        using juce::Colour;
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();

        g.setGradientFill (juce::ColourGradient (Colour (down ? 0xff4b4e51 : 0xff72767a), r.getX(), r.getY(),
                                                 Colour (0xff3a3d40), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (Colour (0xff101112));
        g.drawRoundedRectangle (r, 3.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.30f : 0.15f));
        g.drawLine (r.getX() + 3.0f, r.getY() + 1.5f, r.getRight() - 3.0f, r.getY() + 1.5f, 1.0f);

        // LED
        const bool green = (bool) b.getProperties()["ledGreen"];
        const Colour led = green ? Colour (0xff58ff72) : Colour (0xffff7d1f);
        auto ledRect = juce::Rectangle<float> (r.getCentreX() - 8.0f, r.getY() + 4.0f, 16.0f, 4.0f);
        if (on)
        {
            g.setColour (led.withAlpha (0.28f));
            g.fillRoundedRectangle (ledRect.expanded (3.0f, 2.0f), 3.0f);
        }
        g.setColour (on ? led : led.darker (0.85f).withAlpha (0.7f));
        g.fillRoundedRectangle (ledRect, 1.5f);

        juce::String text = b.getButtonText();
        if (on && b.getProperties().contains ("onText"))
            text = b.getProperties()["onText"].toString();
        if (! on && b.getProperties().contains ("offText"))
            text = b.getProperties()["offText"].toString();

        g.setColour (juce::Colours::white.withAlpha (0.93f));
        g.setFont (uiFont (11.0f, true));
        g.drawText (text, r.withTrimmedTop (9.0f).toNearestInt(), juce::Justification::centred);
    }

    // Cadre de section avec etiquette, comme la serigraphie d'une face avant.
    static void drawSection (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title,
                             juce::Colour accent = juce::Colours::white)
    {
        area = area.reduced (1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (area, 5.0f, 1.2f);

        const auto font = uiFont (12.0f, true);
        juce::GlyphArrangement ga;
        ga.addLineOfText (font, title, 0.0f, 0.0f);
        const float tw = ga.getBoundingBox (0, -1, true).getWidth() + 22.0f;

        juce::Rectangle<float> tag (area.getX() + 12.0f, area.getY() - 9.0f, tw, 18.0f);
        g.setColour (juce::Colour (0xff1e2022));
        g.fillRoundedRectangle (tag, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.drawRoundedRectangle (tag, 4.0f, 1.0f);
        g.setColour (accent);
        g.setFont (font);
        g.drawText (title, tag.toNearestInt(), juce::Justification::centred);
    }
};
