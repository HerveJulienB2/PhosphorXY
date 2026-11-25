#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

// Resultat de la conversion d'un texte :
//  - x, y : le contour du texte, reechantillonne a vitesse constante sur M points (mode DRAW)
//  - codes : les caracteres, utilises pour fabriquer des notes (mode TONE)
// Aucune dependance a JUCE : le code est testable seul.
struct GenTable
{
    static constexpr int M = 4096;

    std::vector<float> x, y;
    std::vector<int> codes;

    static std::unique_ptr<GenTable> fromPolyline (const std::vector<std::array<float, 2>>& pts, std::vector<int> codes)
    {
        auto t = std::make_unique<GenTable>();
        t->codes = std::move (codes);

        if (pts.size() < 2)
            return t;

        // boucle fermee : le trace revient a son point de depart
        std::vector<std::array<float, 2>> p = pts;
        p.push_back (pts.front());

        std::vector<double> cum (p.size(), 0.0);
        float minX = p[0][0], maxX = minX, minY = p[0][1], maxY = minY;
        for (size_t i = 1; i < p.size(); ++i)
        {
            const double dx = (double) p[i][0] - p[i - 1][0];
            const double dy = (double) p[i][1] - p[i - 1][1];
            cum[i] = cum[i - 1] + std::sqrt (dx * dx + dy * dy);
            minX = std::min (minX, p[i][0]);  maxX = std::max (maxX, p[i][0]);
            minY = std::min (minY, p[i][1]);  maxY = std::max (maxY, p[i][1]);
        }

        const double total = cum.back();
        const float half = std::max (0.5f * (maxX - minX), 0.5f * (maxY - minY));
        if (total < 1e-6 || half < 1e-6f)
            return t;

        const float cx = 0.5f * (minX + maxX), cy = 0.5f * (minY + maxY);
        const float scale = 0.9f / half;   // forme centree, proportions conservees, +-0,9 au maximum

        t->x.resize (M);
        t->y.resize (M);

        size_t seg = 1;
        for (int i = 0; i < M; ++i)
        {
            const double s = total * (double) i / (double) M;
            while (seg + 1 < p.size() && cum[seg] < s)
                ++seg;

            const double segLen = cum[seg] - cum[seg - 1];
            const double f = segLen > 1e-12 ? std::min (1.0, std::max (0.0, (s - cum[seg - 1]) / segLen)) : 0.0;
            const double px = p[seg - 1][0] + (p[seg][0] - p[seg - 1][0]) * f;
            const double py = p[seg - 1][1] + (p[seg][1] - p[seg - 1][1]) * f;

            t->x[(size_t) i] = (float) (px - cx) * scale;
            t->y[(size_t) i] = -(float) (py - cy) * scale;   // l'axe vertical de l'ecran est inverse
        }

        return t;
    }
};
