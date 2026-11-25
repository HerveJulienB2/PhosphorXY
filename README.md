# Phosphor XY — oscilloscope audio VST3

Oscilloscope à deux traces avec mode XY, à l'esthétique des oscilloscopes analogiques HAMEG
(face avant anthracite, boutons noirs à index blanc, écran cathodique à phosphore avec persistance).
Le plugin est transparent : le signal traverse sans modification. CH1 = canal gauche, CH2 = canal droit.

## Commandes

**Par voie (CH1 / CH2)**
- **V/DIV** : amplitude de la trace (5 mV à 5 V par division ; 1,0 = pleine échelle numérique = 1 V)
- **TIME/DIV** : base de temps propre à chaque trace (10 µs à 200 ms par division)
- **Y-POS** : position verticale
- **TRIG LEVEL** : niveau de déclenchement (en divisions) ; **RISE/FALL** : pente
- **ON** : active la trace ; menu de couleur (vert, ambre, cyan, blanc, magenta, rouge)

**Affichage**
- **INTENS**, **FOCUS** (finesse du faisceau), **PERSIST** (rémanence du phosphore), **ILLUM** (éclairage du graticule)
- **XY** : bascule en mode XY (X = CH1 avec son V/DIV, Y = CH2 avec le sien) ; **XY LEN** : durée du tracé affichée
- **M/S ROT** : en XY, pivote de 45° (goniomètre : un signal mono donne une ligne verticale, l'élargissement stéréo ouvre la figure)
- **HOLD** : fige l'image ; **TRIG AUTO/NORM** : balayage libre ou trace seulement si déclenché

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
- `HamegLookAndFeel.h`, `PluginEditor.*` : face avant
- `Parameters.h` : tous les paramètres (automatisables dans la DAW)
