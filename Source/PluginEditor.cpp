#include "PluginEditor.h"

using namespace juce;

EditorContent::EditorContent (PhosphorProcessor& p)
    : screen (p),
      readout (p.apvts),
      ch1 (p.apvts, 0),
      ch2 (p.apvts, 1),
      display (p.apvts)
{
    // texture fine de la face avant (grain de peinture)
    noise = Image (Image::ARGB, W, H, true, SoftwareImageType());
    {
        Image::BitmapData bd (noise, Image::BitmapData::writeOnly);
        Random rnd (1234);
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                bd.setPixelColour (x, y, Colour::fromRGBA (255, 255, 255, (uint8) rnd.nextInt (12)));
    }

    addAndMakeVisible (screen);
    addAndMakeVisible (readout);
    addAndMakeVisible (ch1);
    addAndMakeVisible (ch2);
    addAndMakeVisible (display);

    setSize (W, H);
}

void EditorContent::resized()
{
    screen.setBounds (40, 40, 620, 496);
    readout.setBounds (40, 552, 620, 52);
    ch1.setBounds (716, 62, 210, 290);
    ch2.setBounds (934, 62, 210, 290);
    display.setBounds (716, 372, 428, 190);
}

void EditorContent::paint (Graphics& g)
{
    // face avant anthracite
    g.setGradientFill (ColourGradient (Colour (0xff46494c), 0.0f, 0.0f, Colour (0xff2b2d30), 0.0f, (float) H, false));
    g.fillAll();
    g.drawImageAt (noise, 0, 0);

    // cadre du tube cathodique
    const Rectangle<float> bezel (12.0f, 12.0f, 676.0f, 616.0f);
    g.setGradientFill (ColourGradient (Colour (0xff232527), bezel.getX(), bezel.getY(),
                                       Colour (0xff0d0e0f), bezel.getX(), bezel.getBottom(), false));
    g.fillRoundedRectangle (bezel, 14.0f);
    g.setColour (Colours::white.withAlpha (0.22f));
    g.drawRoundedRectangle (bezel.reduced (0.5f), 14.0f, 1.0f);

    const Rectangle<float> inset (34.0f, 34.0f, 632.0f, 508.0f);
    g.setColour (Colours::black);
    g.fillRoundedRectangle (inset, 30.0f);
    g.setColour (Colour (0xff5d6165));
    g.drawRoundedRectangle (inset, 30.0f, 2.0f);

    // fenetre de l'afficheur
    const Rectangle<float> strip (40.0f, 552.0f, 620.0f, 52.0f);
    g.setColour (Colour (0xff050605));
    g.fillRoundedRectangle (strip, 6.0f);
    g.setColour (Colour (0xff4a4d50));
    g.drawRoundedRectangle (strip, 6.0f, 1.5f);

    // en-tete serigraphie
    g.setColour (Colours::white);
    g.setFont (uiFont (22.0f, true));
    g.drawText ("PHOSPHOR XY", 716, 12, 260, 28, Justification::centredLeft);
    g.setColour (Colours::white.withAlpha (0.65f));
    g.setFont (uiFont (10.5f, true));
    g.drawText ("2 CHANNEL AUDIO OSCILLOSCOPE  -  XY MODE  -  VST3", 716, 32, 340, 14, Justification::centredLeft);

    // temoin d'alimentation
    g.setColour (Colour (0xffff3a2a).withAlpha (0.28f));
    g.fillEllipse (1118.0f, 18.0f, 20.0f, 20.0f);
    g.setColour (Colour (0xffff3a2a));
    g.fillEllipse (1122.0f, 22.0f, 12.0f, 12.0f);
    g.setColour (Colours::white.withAlpha (0.8f));
    g.setFont (uiFont (9.5f, true));
    g.drawText ("POWER", 1090, 38, 68, 12, Justification::centred);

    // plaque inferieure
    const Rectangle<float> plate (716.0f, 580.0f, 428.0f, 40.0f);
    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (plate, 5.0f);
    g.setColour (Colours::white.withAlpha (0.55f));
    g.setFont (uiFont (10.0f, true));
    g.drawText ("INPUT: CH1 = LEFT   CH2 = RIGHT   -  SIGNAL PASSES THROUGH UNCHANGED",
                plate.toNearestInt(), Justification::centred);
}

//==============================================================================
PhosphorEditor::PhosphorEditor (PhosphorProcessor& p)
    : AudioProcessorEditor (&p), content (p)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (content);

    setResizeLimits (580, 320, 2320, 1280);
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
