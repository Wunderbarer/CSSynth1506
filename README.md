# Diplomat 1506 (Projekt „CSSynth 1506“)

Polyphoner Software-Synthesizer im Stil der großen Vintage-Synthesizer der späten 70er/80er
(Vorbild: Yamaha CS-80) – als **VST3-Plugin** (z. B. für FL Studio unter Windows 11) und als
**Standalone-Programm**.

```
DIPLOMAT                                                  Model 1506
```

Die Oberfläche hat ein **Cyberpunk-Design**: dunkles Gehäuse mit Neonfarben (Cyan, Magenta, Violett).
Drehregler und Fader leuchten **heller und kräftiger, je höher der Wert steht** (von dunklem Violett über
Cyan bis fast Weiß; LEVEL-Fader und Master-Regler in Magenta). Schriftarten: Orbitron (Name), Audiowide
(Modellnummer) und Rajdhani (Beschriftungen) – eingebettet, Lizenz siehe `Resources/Fonts/OFL-Licenses.txt`.
Plugin-Name, Dateiendung `.cs1506` und Preset-Ordner bleiben unverändert, damit bestehende Projekte und
Presets weiter funktionieren.

## Klang

* **16 Stimmen**, vier unabhängige **Layer** (I – IV; im Fenster per Taster „LAYER I + II“ / „LAYER III + IV“
  umschaltbar), jeder mit:
  * Oszillator: Saw / Square / Pulse (mit PWM), Oktavlage 32' – 2', Detune, Sub-Oszillator, Rauschen
  * Filter: Hochpass + Tiefpass (Resonanz), **Brilliance**, Key-Tracking, Filter-Hüllkurve
    (IL – A – AL – D – SL – R wie beim Original)
  * Verstärker-Hüllkurve (ADSR) und Layer-Level
  * **Layer IV ist ein Formant-Oszillator:** Vokal A / E / I / O / U oder stufenlos **Morph** (Regler MORPH),
    Formant-Verschiebung (F.SHIFT, ±12 Halbtöne) und BREATH (Hauch-Rauschen). Der LFO kann den Vokal
    über **LFO > VOWEL** (Bereich MODULATION) bewegen – für sprechende Klänge.
* **LFO** (Sinus, Dreieck, Saw, Sample&Hold) auf Tonhöhe, Filter und Pulsbreite
* Spielhilfen: Pitchbend (Bereich einstellbar), Modulationsrad (CC 1), **Aftertouch** → Filter und Vibrato,
  Anschlagdynamik → Lautstärke / Filter, Glide (Portamento), Sustain-Pedal
* **Effekte:** Chorus, Delay (mit Ping-Pong) und Reverb
* **39 Werks-Presets** (Dropdown oben im Fenster, Pfeiltasten daneben zum Durchschalten):
  * Brass: Blade Brass, Brass Stab, Fanfare Brass, Soft Horns, Synth Trumpet, Big Brass Section
  * Strings: Warm Strings, Slow Strings, Ensemble 80, Cello Layer
  * Pads: Glass Pad, Choir Pad, Dark Pad, Aurora Pad, Analog Air, Wind Swell
  * Bass: Sub Bass, Fat Bass, Rubber Bass, Pulse Bass, Deep Sub
  * Leads: Ribbon Lead, Solo Cry, Pulse Lead, Smooth Lead
  * Keys: Electric Pluck, Glass Keys, Clav Stab
  * FX: Sweep FX, Wobble Pad, Space Drone, Noise Rise
  * Formant / 4 Layer: Choir Ahh, Vowel Sweep, Talking Lead, Whisper Oh, Deep Voice Bass, Four Layer Stack
  * Init (Grundeinstellung)

## Presets importieren, speichern, weitergeben

Eigene und fremde Klänge liegen als kleine Textdateien mit der Endung **`.cs1506`** vor
(XML, mit jedem Texteditor lesbar). Die Bedienung sitzt oben rechts im Fenster:

* **IMPORT** – Dateiauswahl, auch mehrere Dateien gleichzeitig. Das erste Preset wird sofort geladen.
* **SAVE** – speichert den aktuellen Klang als `.cs1506` (Vorschlag: der Preset-Ordner, siehe unten).
* **Drag & Drop** – Preset-Dateien einfach ins Plugin-Fenster ziehen (je nach Host; im Standalone immer).
* Importierte und gespeicherte Presets erscheinen im Dropdown unter **USER PRESETS**; die Pfeiltasten
  blättern durch alle Presets (Werks- und Anwender-Presets).

Der Preset-Ordner ist `Dokumente\CSSynth\1506 Presets`. Dateien, die du dort von Hand hineinlegst,
erscheinen beim nächsten Öffnen des Plugin-Fensters. Beim Import wird nie etwas überschrieben:
gibt es den Namen schon, wird „Name 2“ angelegt. Ungültige Dateien werden übersprungen, Werte außerhalb
des erlaubten Bereichs automatisch begrenzt.

Die Werks-Presets liegen zusätzlich als Dateien im Paket `CSSynth-1506-Presets.zip` bei, als Vorlage und
zum Ausprobieren des Imports.

## Bauen lassen über GitHub (empfohlen – nichts muss lokal installiert werden)

1. Neues Repository auf GitHub anlegen und den **gesamten Inhalt dieses Ordners** hochladen
   (inklusive des versteckten Ordners `.github`).
2. Auf GitHub den Reiter **Actions** öffnen. Der Workflow *Build CSSynth 1506 (Windows)* startet
   bei jedem Push automatisch (oder manuell über **Run workflow**). Der erste Build dauert
   einige Minuten, weil JUCE heruntergeladen und mitgebaut wird.
3. Nach dem Durchlauf unter **Artifacts** die Datei `CSSynth-1506-Windows-x64` herunterladen.
   Wer ein dauerhaftes Release möchte, setzt einen Tag, z. B. `git tag v1.0.0 && git push --tags` –
   die ZIP-Datei hängt dann automatisch unter **Releases**.

Der Workflow baut außerdem den Offline-DSP-Test und rendert alle Presets; er bricht ab,
wenn dabei NaN-Werte, Stille oder Übersteuerung auftreten.

## Installation unter macOS

Der Workflow baut zusätzlich eine **macOS-Version** (Universal: Apple Silicon und Intel, ab macOS 11) mit
**VST3, Audio Unit (AU) und Standalone-App**. Unter *Actions* den Lauf öffnen und das Artefakt
`CSSynth-1506-macOS-Universal` herunterladen, entpacken und `Install_macOS.command` starten
(Rechtsklick → *Öffnen*). Es kopiert VST3 nach `~/Library/Audio/Plug-Ins/VST3` und AU nach
`~/Library/Audio/Plug-Ins/Components` und entfernt die Quarantäne-Markierung. Die Dateien sind nur
ad-hoc signiert, nicht notarisiert; ohne das Skript zeigt macOS eine Sicherheitswarnung. Die
Standalone-App beim ersten Start per Rechtsklick → *Öffnen* starten.

## Installation unter Windows 11

Das ZIP enthält `VST3\CSSynth 1506.vst3`, `Standalone\CSSynth 1506.exe` und `Install_VST3.bat`.

* **Standalone:** `CSSynth 1506.exe` einfach starten. Unter *Options* (Zahnrad-Menü oben links im
  Fenster) Audio-Gerät und MIDI-Eingang (Oxygen 61 Pro) wählen.
* **VST3:** `Install_VST3.bat` per Rechtsklick → *Als Administrator ausführen*
  (kopiert nach `C:\Program Files\Common Files\VST3`). Alternativ den Ordner `CSSynth 1506.vst3`
  von Hand dorthin kopieren.
* **FL Studio:** *Options → Manage plugins →* **Find installed plugins** (ggf. Plugin-Ordner
  `C:\Program Files\Common Files\VST3` prüfen lassen). „CSSynth 1506“ erscheint danach unter
  *Installed → Generators → VST3*.

Windows SmartScreen kann beim ersten Start der .exe warnen, weil die Datei nicht signiert ist –
*Weitere Informationen → Trotzdem ausführen*.

## Mit dem M-Audio Oxygen 61 Pro steuern

Es gibt zwei Wege, die auch kombinierbar sind.

### A) Eingebautes MIDI-Learn (funktioniert in FL Studio und im Standalone)

1. **Rechtsklick** auf einen Drehregler, Fader oder Schalter → **MIDI Learn**. Das Element wird gelb umrandet.
2. Am Oxygen den gewünschten Regler/Fader bewegen. Die Zuordnung ist sofort aktiv (die zuletzt
   empfangene CC-Nummer steht unten rechts in der Fußzeile).
3. Die Zuordnungen werden **im Projekt bzw. Preset-Zustand gespeichert**.
   „RESET MIDI MAP“ stellt die Standardbelegung wieder her.

Standardbelegung (ohne Learn): CC 7 Master · CC 74 Layer‑I Tiefpass · CC 71 Resonanz ·
CC 73 / 72 Attack / Release · CC 75 Filter‑Attack · CC 76 LFO‑Rate · CC 77 LFO→Pitch ·
CC 78 LFO→Cutoff · CC 91 Reverb · CC 93 Chorus · CC 94 Delay. CC 1 = Modulationsrad,
CC 64 = Sustain-Pedal (fest, nicht lernbar).

**Wichtig in FL Studio:** Die MIDI-Daten des Oxygen müssen an das Plugin gelangen. Im
*MIDI settings*-Bereich (Options → MIDI Settings) das Oxygen als Input aktivieren und bei
**Output port** dieselbe Portnummer eintragen, die im Plugin-Fenster (Zahnrad → *MIDI → Input port*)
eingestellt ist. Das Oxygen sollte außerdem auf ein Preset stehen, das normale Controller-Nachrichten
sendet (nicht ausschließlich den DAW-/DirectLink-Modus für den FL-Mixer).

### B) FL-Studio-eigenes „Link to controller“

Alle Klangparameter sind als **automatisierbare VST3-Parameter** angemeldet. Im Plugin-Fenster
von FL Studio (Zahnrad → *Browse parameters*) oder per Rechtsklick auf das Parameterfeld →
**Link to controller** lässt sich jeder Parameter direkt mit einem Regler des Oxygen verbinden
und anschließend auch in der Playlist automatisieren.

## Selbst bauen (optional)

Voraussetzungen: Windows 11, Visual Studio 2022 („Desktopentwicklung mit C++“), CMake ≥ 3.22, Git.

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Die Ergebnisse liegen unter `build\CSSynth1506_artefacts\Release\`.
Mit `-DCSSYNTH_COPY_PLUGIN=ON` wird das VST3 nach dem Bauen direkt installiert (Administratorrechte nötig).
Mit `-DCSSYNTH_BUILD_TESTS=ON` entsteht zusätzlich `render_test.exe` (Offline-Test aller Presets).

## Projektaufbau

| Pfad | Inhalt |
|---|---|
| `Source/Dsp/Engine.*`, `Effects.h` | Stimmen, Filter, Hüllkurven, LFO, Chorus/Delay/Reverb |
| `Source/ParamSpec.h` | Parameter-Tabelle (Namen, Bereiche, Standardwerte) |
| `Source/Presets.h` | Werks-Presets |
| `Source/PresetFile.h` | Dateiformat `.cs1506` (Import/Export) |
| `Source/PluginProcessor.*` | VST3/Standalone-Anbindung, MIDI, MIDI-Learn, Zustand speichern |
| `Source/PluginEditor.*`, `Look.h` | Cyberpunk-Oberfläche (Neon-Drehregler, Fader, Leuchttaster) |
| `tests/render_test.cpp` | Offline-DSP-Test |
| `.github/workflows/build.yml` | Automatischer Build (Windows und macOS) |

## Lizenzhinweis

Das Projekt nutzt das [JUCE](https://juce.com)-Framework (Version 8), das unter **AGPLv3** oder einer
kommerziellen Lizenz steht. Für den privaten Gebrauch ist das unproblematisch; wer das Plugin
weitergibt oder verkauft, muss entweder den Quellcode gemäß AGPLv3 veröffentlichen oder eine
JUCE-Lizenz erwerben. VST ist eine Marke der Steinberg Media Technologies GmbH.

„Yamaha“ und „CS-80“ sind Marken ihrer Inhaber; CSSynth 1506 ist ein eigenständiges Projekt
und steht in keiner Verbindung zu Yamaha.
