#include "PluginEditor.h"

using namespace juce;

EditorContent::EditorContent (PhosphorProcessor& p)
    : screen (p),
      readout (p.apvts),
      ch1 (p.apvts, 0),
      ch2 (p.apvts, 1),
      display (p.apvts),
      fx (p.apvts),
      generator (p)
{
    // grain fin de la peinture (poudre mate)
    noise = Image (Image::ARGB, W, H, true, SoftwareImageType());
    {
        Image::BitmapData bd (noise, Image::BitmapData::writeOnly);
        Random rnd (1234);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                bd.setPixelColour (x, y, rnd.nextBool() ? Colour::fromRGBA (0, 0, 0, (uint8) rnd.nextInt (9))
                                                        : Colour::fromRGBA (255, 255, 255, (uint8) rnd.nextInt (11)));
    }

    addAndMakeVisible (screen);
    addAndMakeVisible (readout);
    addAndMakeVisible (ch1);
    addAndMakeVisible (ch2);
    addAndMakeVisible (display);
    addAndMakeVisible (fx);
    addAndMakeVisible (generator);

    setSize (W, H);
}

void EditorContent::resized()
{
    screen.setBounds (50, 50, 640, 512);
    readout.setBounds (50, 578, 640, 52);

    ch1.setBounds (748, 72, 268, 262);
    ch2.setBounds (1028, 72, 268, 262);
    display.setBounds (748, 344, 548, 162);
    fx.setBounds (748, 516, 336, 134);
    generator.setBounds (24, 664, 1272, 138);
}

static void drawGrille (Graphics& g, Point<float> centre, float radius)
{
    // grille perforee ronde, clin d'oeil aux haut-parleurs des postes portables des annees 60
    const float pitch = 9.0f;
    for (float y = -radius; y <= radius; y += pitch)
        for (float x = -radius; x <= radius; x += pitch)
            if (x * x + y * y <= radius * radius)
            {
                const float px = centre.x + x, py = centre.y + y;
                g.setColour (Colours::white.withAlpha (0.55f));
                g.fillEllipse (px - 2.2f, py - 1.2f, 4.4f, 4.4f);
                g.setColour (Colour (0xff4b5052));
                g.fillEllipse (px - 2.2f, py - 2.2f, 4.4f, 4.4f);
            }
}

void EditorContent::paint (Graphics& g)
{
    // face avant gris clair mat
    g.setGradientFill (ColourGradient (Theme::panelTop, 0.0f, 0.0f, Theme::panelBottom, 0.0f, (float) H, false));
    g.fillAll();
    g.drawImageAt (noise, 0, 0);

    // bloc de l'ecran cathodique
    const Rectangle<float> bezel (24.0f, 24.0f, 700.0f, 626.0f);
    g.setColour (Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (bezel.translated (0.0f, 5.0f).expanded (1.0f), 20.0f);
    g.setGradientFill (ColourGradient (Colour (0xff35393b), bezel.getX(), bezel.getY(),
                                       Colour (0xff1a1c1d), bezel.getX(), bezel.getBottom(), false));
    g.fillRoundedRectangle (bezel, 18.0f);
    g.setColour (Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (bezel.reduced (0.5f), 18.0f, 1.0f);

    const Rectangle<float> inset (44.0f, 44.0f, 652.0f, 524.0f);
    g.setColour (Colours::black);
    g.fillRoundedRectangle (inset, 28.0f);
    g.setColour (Colour (0xff55595c));
    g.drawRoundedRectangle (inset, 28.0f, 2.0f);

    const Rectangle<float> strip (50.0f, 578.0f, 640.0f, 52.0f);
    g.setColour (Colour (0xff050605));
    g.fillRoundedRectangle (strip, 6.0f);
    g.setColour (Colour (0xff4a4d50));
    g.drawRoundedRectangle (strip, 6.0f, 1.5f);

    // titre : minuscules, sobre
    g.setColour (Theme::ink);
    g.setFont (uiFont (27.0f, true));
    g.drawText ("phosphor xy", 748, 22, 300, 34, Justification::centredLeft);
    g.setColour (Theme::inkSoft);
    g.setFont (uiFont (11.0f));
    g.drawText ("audio oscilloscope   -   channel 1 = left, channel 2 = right", 750, 50, 420, 14, Justification::centredLeft);

    // temoin de fonctionnement
    g.setColour (Theme::accentLit.withAlpha (0.30f));
    g.fillEllipse (1268.0f, 30.0f, 22.0f, 22.0f);
    g.setColour (Theme::accentLit);
    g.fillEllipse (1273.0f, 35.0f, 12.0f, 12.0f);
    g.setColour (Colours::black.withAlpha (0.35f));
    g.drawEllipse (1273.0f, 35.0f, 12.0f, 12.0f, 1.0f);
    g.setColour (Theme::inkSoft);
    g.setFont (uiFont (9.5f, true));
    g.drawText ("VST3", 1196, 33, 66, 16, Justification::centredRight);

    // grille perforee
    drawGrille (g, { 1194.0f, 585.0f }, 58.0f);

    // filet de separation avec le generateur
    g.setColour (Theme::line);
    g.fillRect (24, 657, 1272, 1);
    g.setColour (Colours::white.withAlpha (0.55f));
    g.fillRect (24, 658, 1272, 1);
}

//==============================================================================
PhosphorEditor::PhosphorEditor (PhosphorProcessor& p)
    : AudioProcessorEditor (&p), content (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (content);

    setResizeLimits (660, 412, 2640, 1648);
    getConstrainer()->setFixedAspectRatio ((double) EditorContent::W / (double) EditorContent::H);
    setResizable (true, true);
    setSize (EditorContent::W, EditorContent::H);
}

PhosphorEditor::~PhosphorEditor()
{
    setLookAndFeel (nullptr);
}

void PhosphorEditor::resized()
{
    content.setBounds (0, 0, EditorContent::W, EditorContent::H);
    content.setTransform (AffineTransform::scale ((float) getWidth() / (float) EditorContent::W));
}
