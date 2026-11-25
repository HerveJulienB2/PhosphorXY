#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Palette : gris clair mat, encre sombre, un seul accent vert.
namespace Theme
{
    inline const juce::Colour panelTop    { 0xffdadcd9 };
    inline const juce::Colour panelBottom { 0xffc8cbc7 };
    inline const juce::Colour ink         { 0xff25292b };
    inline const juce::Colour inkSoft     { 0xff6b7174 };
    inline const juce::Colour line        { 0xff9a9f9b };
    inline const juce::Colour recess      { 0xffe8eae7 };
    inline const juce::Colour accent      { 0xff1f9d57 };
    inline const juce::Colour accentLit   { 0xff3fe07d };
    inline const juce::Colour crtGreen    { 0xff7dffa0 };
}

inline juce::Font uiFont (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font monoFont (float height)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
}

// Esprit appareils de laboratoire des annees 60 (Braun / Dieter Rams) :
// surfaces mates, boutons sobres (aluminium brosse ou noirs), touches plates, un seul accent de couleur.
class BraunLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BraunLookAndFeel()
    {
        using juce::Colour;
        setColour (juce::Slider::textBoxTextColourId, Theme::ink);
        setColour (juce::Slider::textBoxOutlineColourId, Theme::line);
        setColour (juce::Slider::textBoxBackgroundColourId, Theme::recess);
        setColour (juce::Slider::textBoxHighlightColourId, Theme::accent.withAlpha (0.4f));
        setColour (juce::Label::textColourId, Theme::ink);
        setColour (juce::ComboBox::backgroundColourId, Theme::recess);
        setColour (juce::ComboBox::textColourId, Theme::ink);
        setColour (juce::ComboBox::outlineColourId, Theme::line);
        setColour (juce::ComboBox::arrowColourId, Theme::ink);
        setColour (juce::PopupMenu::backgroundColourId, Theme::recess);
        setColour (juce::PopupMenu::textColourId, Theme::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, Theme::accent);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    juce::Font getLabelFont (juce::Label&) override { return uiFont (11.0f); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return uiFont (11.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float startAngle, float endAngle, juce::Slider& slider) override
    {
        using juce::Colour;
        const bool metal = (bool) slider.getProperties()["metal"];

        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto c = bounds.getCentre();
        const float outerR = size * 0.5f;

        // graduations imprimees
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const bool major = (i % 5) == 0;
            g.setColour (Theme::inkSoft.withAlpha (major ? 1.0f : 0.7f));
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (outerR * (major ? 0.80f : 0.86f), a),
                                           c.getPointOnCircumference (outerR * 0.97f, a)),
                        major ? 1.4f : 1.0f);
        }

        const float R = outerR * 0.70f;
        const float a = startAngle + sliderPos * (endAngle - startAngle);

        // ombre douce
        g.setColour (juce::Colours::black.withAlpha (0.14f));
        g.fillEllipse (c.x - R + 1.5f, c.y - R + 3.5f, R * 2.0f, R * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillEllipse (c.x - R + 0.5f, c.y - R + 1.5f, R * 2.0f, R * 2.0f);

        if (metal)
        {
            // anneau noir
            g.setGradientFill (juce::ColourGradient (Colour (0xff3b3e40), c.x - R * 0.6f, c.y - R * 0.8f,
                                                     Colour (0xff0c0d0e), c.x + R * 0.6f, c.y + R * 0.9f, false));
            g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);

            // disque d'aluminium brosse
            const float D = R * 0.84f;
            g.setGradientFill (juce::ColourGradient (Colour (0xfff3f4f2), c.x - D * 0.7f, c.y - D * 0.8f,
                                                     Colour (0xff9ca0a2), c.x + D * 0.8f, c.y + D * 0.9f, false));
            g.fillEllipse (c.x - D, c.y - D, D * 2.0f, D * 2.0f);

            for (int i = 1; i <= 14; ++i)
            {
                const float rr = D * (float) i / 15.0f;
                g.setColour ((i % 2 ? juce::Colours::white : juce::Colours::black).withAlpha (0.07f));
                g.drawEllipse (c.x - rr, c.y - rr, rr * 2.0f, rr * 2.0f, 1.0f);
            }
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.drawEllipse (c.x - D, c.y - D, D * 2.0f, D * 2.0f, 1.0f);

            g.setColour (Colour (0xff2a2d2f));
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (D * 0.45f, a),
                                           c.getPointOnCircumference (D * 0.92f, a)), 2.6f);
        }
        else
        {
            g.setGradientFill (juce::ColourGradient (Colour (0xff4a4d50), c.x - R * 0.5f, c.y - R * 0.7f,
                                                     Colour (0xff0d0e0f), c.x + R * 0.7f, c.y + R * 0.9f, false));
            g.fillEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);

            const float D = R * 0.80f;
            g.setGradientFill (juce::ColourGradient (Colour (0xff2f3133), c.x - D * 0.6f, c.y - D * 0.8f,
                                                     Colour (0xff0a0b0b), c.x + D * 0.6f, c.y + D, false));
            g.fillEllipse (c.x - D, c.y - D, D * 2.0f, D * 2.0f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawEllipse (c.x - D, c.y - D, D * 2.0f, D * 2.0f, 1.0f);

            g.setColour (Colour (0xfff2f2f0));
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (D * 0.30f, a),
                                           c.getPointOnCircumference (R * 0.95f, a)), 2.6f);
        }
    }

    // Touche plate avec temoin lumineux : le temoin vert indique l'etat actif.
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool down) override
    {
        using juce::Colour;
        const auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();

        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 4.0f);

        g.setGradientFill (juce::ColourGradient (on ? Colour (0xffb9bdb8) : Colour (0xffe4e6e3), r.getX(), r.getY(),
                                                 on ? Colour (0xffc7cac6) : Colour (0xffc6c9c5), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        if (highlighted || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.0f : 0.25f));
            g.fillRoundedRectangle (r, 4.0f);
        }
        g.setColour (Theme::line);
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        if (on)
        {
            g.setColour (juce::Colours::black.withAlpha (0.12f));
            g.drawLine (r.getX() + 3.0f, r.getY() + 1.5f, r.getRight() - 3.0f, r.getY() + 1.5f, 2.0f);
        }

        const auto led = juce::Rectangle<float> (r.getCentreX() - 3.0f, r.getY() + 6.0f, 6.0f, 6.0f);
        if (on)
        {
            g.setColour (Theme::accentLit.withAlpha (0.35f));
            g.fillEllipse (led.expanded (3.0f));
        }
        g.setColour (on ? Theme::accentLit : Colour (0xff8a8f8b));
        g.fillEllipse (led);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (led, 1.0f);

        juce::String text = b.getButtonText();
        if (on && b.getProperties().contains ("onText"))
            text = b.getProperties()["onText"].toString();
        if (! on && b.getProperties().contains ("offText"))
            text = b.getProperties()["offText"].toString();

        g.setColour (Theme::ink);
        g.setFont (uiFont (10.5f, true));
        g.drawText (text, r.withTrimmedTop (14.0f).toNearestInt(), juce::Justification::centred);
    }

    // Titre de section : repere colore, texte, filet en relief.
    static void drawHeader (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title,
                            juce::Colour swatch = Theme::accent)
    {
        g.setColour (swatch);
        g.fillRect (area.getX(), area.getY() + 4.0f, 8.0f, 8.0f);

        g.setColour (Theme::ink);
        g.setFont (uiFont (11.0f, true));
        g.drawText (title, juce::Rectangle<float> (area.getX() + 14.0f, area.getY(), area.getWidth() - 14.0f, 16.0f),
                    juce::Justification::centredLeft);

        g.setColour (Theme::line);
        g.fillRect (area.getX(), area.getY() + 19.0f, area.getWidth(), 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillRect (area.getX(), area.getY() + 20.0f, area.getWidth(), 1.0f);
    }
};
