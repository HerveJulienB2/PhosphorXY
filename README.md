# Phosphor XY — oscilloscope audio VST3

Oscilloscope à deux traces avec mode XY, à l'esthétique du matériel de laboratoire des années 60
(face avant gris clair mat, boutons aluminium brossé ou noirs, un accent vert, écran cathodique à phosphore avec persistance).
Le plugin est transparent : le signal traverse sans modification. CH1 = canal gauche, CH2 = canal droit.

## Commandes

**Par voie (CHANNEL 1 = gauche, CHANNEL 2 = droite)**
- **V/DIV** : amplitude de la trace (5 mV à 5 V par division ; 1,0 = pleine échelle numérique = 1 V)
- **TIME/DIV** : base de temps propre à chaque trace (10 µs à 200 ms par division)
- **Y-POS** : position verticale
- **TRIG LEVEL** : niveau de déclenchement (en divisions) ; **RISE/FALL** : pente
- **ON** : active la trace ; menu de couleur de la trace

**Display**
- **INTENSITY**, **FOCUS** (finesse du faisceau), **PERSIST** (rémanence du phosphore), **GRID LIGHT** (éclairage du graticule)
- **XY MODE** : X = voie 1 avec son V/DIV, Y = voie 2 avec le sien ; **XY TRACE** : durée du tracé affiché
- **M/S 45** : en XY, pivote de 45° (goniomètre : un signal mono donne une ligne verticale)
- **HOLD** : fige l'image ; **TRIGGER AUTO/NORMAL** : balayage libre, ou trace seulement si déclenché

**Image effects** (affichage seulement, jamais sur le son)
- **CURVATURE** : bombé du tube cathodique ; **SCANLINES** : lignes de balayage ; **GLITCH** : glitchs numériques (décalages de bandes, séparation des couleurs, sauts d'image)

**Generator : texte vers forme d'onde**
- Tapez un mot ou une phrase (40 caractères max). Le texte est sauvegardé avec le projet.
- **MODE DRAW (XY)** : le contour des lettres est parcouru en boucle, gauche = X, droite = Y. En mode XY, le texte s'écrit à l'écran. **PITCH** = cadence de répétition du dessin.
- **MODE TONE (NOTES)** : chaque caractère devient une note (espace = silence), jouée à la suite. **PITCH** = note de base, **STEP** = durée d'une note, **SCALE** = gamme.
- **SOURCE** : *INPUT ONLY* (défaut, le plugin reste un simple oscilloscope), *INPUT + GEN* (mélange), *GEN ONLY* (générateur seul). **LEVEL** règle le volume du générateur.
- L'oscilloscope affiche ce que vous entendez : le signal de sortie.

Double-clic sur un bouton : retour à la valeur par défaut. La fenêtre est redimensionnable.

## Compiler

Prérequis : CMake ≥ 3.22 et un compilateur C++17 (Xcode, Visual Studio 2022, ou gcc/clang). JUCE 8.0.4 est téléchargé automatiquement.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Le `.vst3` est généré dans `build/PhosphorXY_artefacts/Release/VST3/` et copié automatiquement dans le dossier VST3
de l'utilisateur (désactivable avec `-DCOPY_AFTER_BUILD=OFF`).

- **macOS universel (Apple Silicon + Intel)** : ajouter `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
  Plugin non notarisé : `codesign --force --deep -s - "Phosphor XY.vst3"` puis, si besoin, `xattr -cr "Phosphor XY.vst3"`.
- **Windows** : ouvrir un « x64 Native Tools Command Prompt for VS 2022 » ; copier le dossier `.vst3` dans `C:\Program Files\Common Files\VST3`.
- **Sans machine de build** : poussez ce dossier sur GitHub ; le workflow `.github/workflows/build.yml` compile Windows, macOS et Linux et publie les `.vst3` en artefacts.

## Architecture

- `ScopeBuffer.h` : tampon circulaire sans verrou alimenté par le thread audio
- `ScopeScreen.cpp` : déclenchement par voie (interpolation sous-échantillon), tracé enveloppe min/max ou interpolation cubique, persistance du phosphore, graticule 10 × 8
- `BraunLookAndFeel.h`, `PluginEditor.*` : face avant (gris clair mat, accent vert)
- `GenTable.h`, `Generator.h`, `TextShape.cpp` : conversion du texte en forme d'onde et moteur audio du générateur
- `Parameters.h` : tous les paramètres (automatisables dans la DAW)
