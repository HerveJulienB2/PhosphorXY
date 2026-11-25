#include "TextShape.h"

namespace TextShape
{
    std::unique_ptr<GenTable> build (const juce::String& text)
    {
        const auto t = text.trim().substring (0, 40);

        std::vector<int> codes;
        for (auto c : t)
            codes.push_back ((int) c);

        std::vector<std::array<float, 2>> pts;

        if (t.isNotEmpty())
        {
            juce::GlyphArrangement glyphs;
            glyphs.addLineOfText (juce::Font (juce::FontOptions (100.0f, juce::Font::bold)), t.toUpperCase(), 0.0f, 0.0f);

            juce::Path path;
            glyphs.createPath (path);

            // contours aplatis en segments ; les contours successifs sont relies bout a bout
            juce::PathFlatteningIterator it (path, juce::AffineTransform(), 0.4f);
            while (it.next())
            {
                if (it.subPathIndex == 0)
                    pts.push_back ({ it.x1, it.y1 });
                pts.push_back ({ it.x2, it.y2 });
            }
        }

        return GenTable::fromPolyline (pts, std::move (codes));
    }
}
