#pragma once

#include <algorithm>
#include <cmath>
#include "GenTable.h"

// Generateur audio : produit un signal stereo a partir d'une GenTable.
//  mode 0 (DRAW) : le contour du texte est parcouru en boucle, gauche = X, droite = Y.
//                  A l'ecran en mode XY, on voit le texte ecrit.
//  mode 1 (TONE) : chaque caractere devient une note d'une gamme, jouee l'une apres l'autre.
//                  Gauche = sinus, droite = cosinus (decales d'un quart de periode) : figure de Lissajous arrondie.
// Pas d'allocation, pas de verrou : utilisable dans le thread audio.
class GeneratorEngine
{
public:
    struct Params
    {
        int mode = 0;
        float freq = 110.0f;     // Hz : cadence de repetition (DRAW) ou note fondamentale (TONE)
        float stepMs = 220.0f;   // duree d'une note (TONE)
        int scale = 2;           // 0 majeure, 1 mineure, 2 pentatonique, 3 chromatique
    };

    void prepare (double sr)
    {
        sampleRate = sr;
        invSr = 1.0 / sr;
        reset();
    }

    void reset()
    {
        drawPhase = 0.0;
        tonePhase = 0.0;
        stepPos = 0;
        stepIdx = -1;
        curFreq = 0.0f;
        resting = true;
    }

    void setTable (const GenTable* t) noexcept { table = t; }

    static int semitone (int code, int scale) noexcept
    {
        static const int major[]  = { 0, 2, 4, 5, 7, 9, 11 };
        static const int minor[]  = { 0, 2, 3, 5, 7, 8, 10 };
        static const int penta[]  = { 0, 2, 4, 7, 9 };
        static const int chroma[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

        const int* s = penta;
        int len = 5;
        switch (scale)
        {
            case 0: s = major;  len = 7;  break;
            case 1: s = minor;  len = 7;  break;
            case 3: s = chroma; len = 12; break;
            default: break;
        }

        int k;
        if (code >= 'a' && code <= 'z')      k = code - 'a';
        else if (code >= 'A' && code <= 'Z') k = code - 'A';
        else if (code >= '0' && code <= '9') k = 26 + (code - '0');
        else                                 k = (code & 0x7fff) % 26;

        return s[k % len] + 12 * ((k / len) % 3);
    }

    static bool isRest (int code) noexcept { return code == ' ' || code == '\t'; }

    void process (const Params& p, float& l, float& r) noexcept
    {
        l = r = 0.0f;
        const GenTable* t = table;
        if (t == nullptr)
            return;

        if (p.mode == 0)
        {
            if (t->x.empty())
                return;

            drawPhase += (double) p.freq * invSr;
            drawPhase -= std::floor (drawPhase);

            const float pos = (float) (drawPhase * GenTable::M);
            int i0 = (int) pos;
            if (i0 >= GenTable::M) i0 = GenTable::M - 1;
            const int i1 = (i0 + 1) % GenTable::M;
            const float f = pos - (float) i0;

            l = t->x[(size_t) i0] + (t->x[(size_t) i1] - t->x[(size_t) i0]) * f;
            r = t->y[(size_t) i0] + (t->y[(size_t) i1] - t->y[(size_t) i0]) * f;
            return;
        }

        const int n = (int) t->codes.size();
        if (n == 0)
            return;

        const int samplesPerStep = std::max (2, (int) (p.stepMs * 0.001 * sampleRate));

        if (stepIdx < 0 || stepPos >= samplesPerStep)
        {
            stepPos = 0;
            stepIdx = (stepIdx + 1) % n;
            const int code = t->codes[(size_t) stepIdx];
            resting = isRest (code);
            if (! resting)
            {
                const double semis = (double) semitone (code, p.scale);
                curFreq = (float) std::min (0.45 * sampleRate, (double) p.freq * std::pow (2.0, semis / 12.0));
            }
        }

        float env = 0.0f;
        if (! resting)
        {
            const int ramp = std::max (1, std::min ((int) (0.006 * sampleRate), samplesPerStep / 4));
            env = std::min ({ 1.0f, (float) stepPos / (float) ramp, (float) (samplesPerStep - stepPos) / (float) ramp });
            env = std::max (0.0f, env);

            tonePhase += (double) curFreq * invSr;
            tonePhase -= std::floor (tonePhase);
        }

        const float w = 6.2831853f * (float) tonePhase;
        l = env * 0.8f * (std::sin (w) + 0.25f * std::sin (3.0f * w));
        r = env * 0.8f * (std::cos (w) + 0.25f * std::cos (3.0f * w));
        ++stepPos;
    }

private:
    const GenTable* table = nullptr;
    double sampleRate = 44100.0, invSr = 1.0 / 44100.0;
    double drawPhase = 0.0, tonePhase = 0.0;
    int stepPos = 0, stepIdx = -1;
    float curFreq = 0.0f;
    bool resting = true;
};
