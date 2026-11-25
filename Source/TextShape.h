#pragma once

#include <juce_graphics/juce_graphics.h>
#include "GenTable.h"

namespace TextShape
{
    // Convertit un texte en GenTable : contour des lettres (mode DRAW) et caracteres (mode TONE).
    // A appeler depuis le thread de l'interface (utilise les polices du systeme).
    std::unique_ptr<GenTable> build (const juce::String& text);
}
