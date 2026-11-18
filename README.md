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

* **16 Stimmen**, fünf unabhängige **Layer** (I – V; im Fenster per Taster „LAYER I + II“ / „LAYER III + IV“ /
  „LAYER V - SAMPLE“ umschaltbar), jeder mit:
  * Oszillator (Layer I–III): Saw / Square / Pulse (mit PWM) / **Sine** / **Triangle** (bandbegrenzt), Oktavlage 32' – 2', Detune, Sub-Oszillator, Rauschen
  * Filter: Hochpass + Tiefpass (Resonanz), **Brilliance**, Key-Tracking, Filter-Hüllkurve
    (IL – A – AL – D – SL – R wie beim Original)
  * Verstärker-Hüllkurve (ADSR) und Layer-Level
  * **Layer IV ist ein Formant-Oszillator:** Vokal A / E / I / O / U oder stufenlos **Morph** (Regler MORPH),
    Formant-Verschiebung (F.SHIFT, ±12 Halbtöne) und BREATH (Hauch-Rauschen). Der LFO kann den Vokal
    über **LFO > VOWEL** (Bereich MODULATION) bewegen – für sprechende Klänge.
* **Layer V ist ein Sample-Oszillator (fünfter Oszillator):** Eine Audiodatei (WAV, AIFF, FLAC, OGG, MP3)
  wird per **Drag & Drop** irgendwo ins Fenster gezogen oder über **LOAD FILE ...** geladen und klingt
  wie ein Oszillator – mit denselben Reglern/Fadern wie die anderen Layer (Oktave, Detune, Sub, Rauschen,
  Filter, Filter-/Verstärker-Hüllkurve, LEVEL). Zusätzlich:
  * PLAYBACK: **Loop**, **One-Shot**, **Ping-Pong**; START (Startpunkt); **ROOT NOTE** (Taste, bei der die
    Datei im Originalton klingt, Standard C5 = MIDI 60); **LOOP START / LOOP END** (auch per Marker in der
    Wellenform ziehbar)
  * Beim ersten Laden wird LEVEL von Layer V automatisch auf 0,8 gesetzt, falls es auf 0 stand.
  * Die Datei wird mono gemischt, normalisiert und auf max. **20 Sekunden** gekürzt. Sie wird im
    Projekt/DAW-Zustand mitgespeichert (als 16-Bit-WAV), ist aber **nicht** Teil der `.cs1506`-Presets
    (diese enthalten nur die Layer-V-Reglerwerte).
* **LFO** (Sinus, Dreieck, Saw, Sample&Hold) auf Tonhöhe, Filter und Pulsbreite
* Spielhilfen: Pitchbend (Bereich einstellbar), Modulationsrad (CC 1), **Aftertouch** → Filter und Vibrato,
  Anschlagdynamik → Lautstärke / Filter, Glide (Portamento), Sustain-Pedal
* **Effekte** (eigene Zeile unten im Fenster, Reihenfolge: Distortion → Phaser → Flanger → Chorus → Delay → Reverb;
  alle MIX-Regler stehen standardmäßig auf 0, ältere Presets klingen also unverändert):
  * **Distortion:** TYPE (Soft = tanh, Hard = Clipping, Fold = Wellenfalter), DRIVE, TONE (Klangblende), MIX
  * **Phaser:** 6 Allpass-Stufen mit RATE, DEPTH, FDBK (Rückkopplung), MIX, Stereo-Versatz
  * **Flanger:** RATE, DEPTH, FDBK (positiv oder negativ), MIX, Stereo-Versatz
  * Chorus, Delay (mit Ping-Pong) und Reverb wie bisher
* **44 Werks-Sounds** (Bank FACTORY, Dropdown oben im Fenster, Pfeiltasten daneben zum Durchschalten):
  * Brass: Blade Brass, Brass Stab, Fanfare Brass, Soft Horns, Synth Trumpet, Big Brass Section
  * Strings: Warm Strings, Slow Strings, Ensemble 80, Cello Layer
  * Pads: Glass Pad, Choir Pad, Dark Pad, Aurora Pad, Analog Air, Wind Swell
  * Bass: Sub Bass, Fat Bass, Rubber Bass, Pulse Bass, Deep Sub
  * Leads: Ribbon Lead, Solo Cry, Pulse Lead, Smooth Lead
  * Keys: Electric Pluck, Glass Keys, Clav Stab
  * FX: Sweep FX, Wobble Pad, Space Drone, Noise Rise
  * Formant / 4 Layer: Choir Ahh, Vowel Sweep, Talking Lead, Whisper Oh, Deep Voice Bass, Four Layer Stack
  * Effekte: Phase Pad, Jet Flanger Keys, Gritty Bass
  * Wellenformen: Pure Sine Sub, Soft Triangle Keys
  * Init (Grundeinstellung)

## Sounds: zwei Bänke (SOUNDBANKS und USER)

Oben rechts im Fenster schaltest du zwischen zwei getrennten Bänken um:

**SOUNDBANKS** – fertige Sounds zum Durchblättern und Spielen (nur lesen):
* Die Auswahl rechts oben (Klappfeld) wählt die Soundbank: **FACTORY** (die 44 eingebauten Werks-Sounds)
  oder eine geladene Bank. Darunter steht die Sound-Liste dieser Bank, die Pfeiltasten blättern darin.
* **LOAD BANK** lädt eine Soundbank-Datei (**`.cs1506bank`**; mehrere Sounds in einer Datei). Du kannst die
  Datei auch ins Fenster ziehen. Die Bank wird in `Dokumente\CSSynth\1506 Banks` abgelegt und steht
  danach immer zur Verfügung. Eine bereits geladene, identische Bank wird nicht doppelt angelegt.
* **REMOVE** entfernt die gewählte geladene Bank (Papierkorb, nach Rückfrage). Die Werks-Bank bleibt immer.
* Die Datei **`Diplomat 99 Sounds.cs1506bank`** ist das 99-Sounds-Paket als fertige Soundbank.

**USER SOUNDS** – deine eigenen Sounds:
* **SAVE** fragt nach einem Namen und legt den aktuellen Klang in der USER-Bank ab (auch wenn er aus einer
  Soundbank stammt – so wird aus einem Bank-Sound dein eigener, veränderter Sound). Gibt es den Namen
  schon, kommt eine Rückfrage, ob er ersetzt werden soll.
* **IMPORT** übernimmt einzelne Sound-Dateien (`.cs1506`, auch mehrere gleichzeitig) in die USER-Bank.
  Drag & Drop von `.cs1506`-Dateien funktioniert ebenfalls. Es wird nie etwas überschrieben: gibt es den
  Namen schon, wird „Name 2“ angelegt.
* **DELETE** entfernt den gewählten eigenen Sound (Papierkorb, nach Rückfrage).
* **EXPORT BANK** speichert alle eigenen Sounds als eine `.cs1506bank`-Datei, die du weitergeben oder in
  einem anderen Rechner per **LOAD BANK** laden kannst.

Die USER-Bank ist der Ordner `Dokumente\CSSynth\1506 Presets` (ein Sound = eine `.cs1506`-Datei, XML, mit
jedem Texteditor lesbar). Dateien, die du dort von Hand hineinlegst, erscheinen beim Umschalten auf die
Bank. Ungültige Dateien werden übersprungen, Werte außerhalb des erlaubten Bereichs automatisch begrenzt.

**MIDI-Zuordnung und Sample im Sound speichern:** Beim **SAVE** wählst du in zwei Auswahlfeldern, ob die
aktuelle **MIDI-Zuordnung** (Learn) und – falls eines geladen ist – das **Layer-V-Sample** mit in den Sound
geschrieben werden (Standard: ja). Beim späteren Laden gilt:
* Enthält der Sound eine MIDI-Zuordnung, ersetzt sie die aktuelle; enthält er keine, bleibt die aktuelle bestehen.
* Enthält der Sound ein Sample, ersetzt es das aktuelle Layer-V-Sample; sonst bleibt das aktuelle geladen.
* Die Anzeige unten bestätigt es: „LOADED NAME + MIDI MAP + SAMPLE“.
* **EXPORT BANK** nimmt diese Zusätze mit, ebenso `.cs1506`-Dateien beim Weitergeben. Ein Sound mit Sample ist
  entsprechend größer (bis ca. 2 MB je 20 Sekunden Mono-Audio, als 16-Bit-WAV in Textform gespeichert).
* Werks-Sounds enthalten weder MIDI-Zuordnung noch Sample.

Das Projekt/der DAW-Zustand merkt sich außerdem, welche Bank und welcher Sound zuletzt gewählt waren, und
speichert MIDI-Zuordnung und Sample ohnehin immer mit.

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
   Alternative: Unten auf **MIDI LEARN: OFF** klicken (wird zu **ON**). Dann genügt ein **Linksklick** auf ein
   Bedienelement nach dem anderen (Klick auf dasselbe Element bricht ab); zum Beenden den Taster wieder ausschalten.
2. Am Oxygen den gewünschten Regler/Fader bewegen. Die Zuordnung ist sofort aktiv; die Fußzeile bestätigt sie mit
   „LEARNED: CC 21 > DISTORTION DRIVE“. Ohne Learn zeigt sie die zuletzt empfangene Nachricht („LAST CC 21 = 64“).
   Bleibt dort „NO MIDI CC RECEIVED YET“ stehen, kommen am Plugin keine Controller-Daten an (siehe „Wichtig in FL Studio“).
3. Die Zuordnungen werden **im Projekt bzw. Preset-Zustand gespeichert**.
   „RESET MIDI MAP“ stellt die Standardbelegung wieder her.

Standardbelegung (ohne Learn): CC 7 Master · CC 74 Layer‑I Tiefpass · CC 71 Resonanz ·
CC 73 / 72 Attack / Release · CC 75 Filter‑Attack · CC 76 LFO‑Rate · CC 77 LFO→Pitch ·
CC 78 LFO→Cutoff · CC 91 Reverb · CC 93 Chorus · CC 94 Delay · CC 80 Distortion‑Drive ·
CC 81 Phaser‑Mix · CC 82 Flanger‑Mix · CC 83 Distortion‑Mix. CC 1 = Modulationsrad,
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
Mit `-DCSSYNTH_BUILD_TESTS=ON` entsteht zusätzlich `render_test.exe` (Offline-Test aller Presets) und `sample_test.exe` (Test des Sample-Oszillators) und `fx_test.exe` (Test von Distortion/Phaser/Flanger).

## Projektaufbau

| Pfad | Inhalt |
|---|---|
| `Source/Dsp/Engine.*`, `Effects.h` | Stimmen, Filter, Hüllkurven, LFO, Chorus/Delay/Reverb |
| `Source/ParamSpec.h` | Parameter-Tabelle (Namen, Bereiche, Standardwerte) |
| `Source/Presets.h` | Werks-Presets |
| `Source/PresetFile.h` | Dateiformat `.cs1506` (Import/Export) |
| `Source/PluginProcessor.*` | VST3/Standalone-Anbindung, MIDI, MIDI-Learn, Zustand speichern |
| `Source/PluginEditor.*`, `Look.h` | Cyberpunk-Oberfläche (Neon-Drehregler, Fader, Leuchttaster) |
| `Source/SampleView.h` | Wellenform-Anzeige / Drop-Zone für Layer V |
| `tests/render_test.cpp`, `sample_test.cpp`, `fx_test.cpp` | Offline-DSP-Tests |
| `.github/workflows/build.yml` | Automatischer Build (Windows und macOS) |

## Lizenzhinweis

Das Projekt nutzt das [JUCE](https://juce.com)-Framework (Version 8), das unter **AGPLv3** oder einer
kommerziellen Lizenz steht. Für den privaten Gebrauch ist das unproblematisch; wer das Plugin
weitergibt oder verkauft, muss entweder den Quellcode gemäß AGPLv3 veröffentlichen oder eine
JUCE-Lizenz erwerben. VST ist eine Marke der Steinberg Media Technologies GmbH.

„Yamaha“ und „CS-80“ sind Marken ihrer Inhaber; CSSynth 1506 ist ein eigenständiges Projekt
und steht in keiner Verbindung zu Yamaha.

## Diagnose bei Störgeräuschen

Der Synth schreibt auffällige Ereignisse in die Datei `Dokumente\CSSynth\1506 Diagnose.log`
(Start mit Abtastrate/Puffergröße, Status des Denormal-Schutzes, Ausgabe obwohl seit 15 s keine Stimme klingt,
MIDI-Nachrichten im Leerlauf, Audio-Blöcke mit mehr als 60 % Rechenzeit). Die Datei wird maximal ca. 0,5 MB groß.
Bei einem Störgeräusch: Uhrzeit notieren und die Datei zur Auswertung weitergeben.

## Tempo-Sync (Delay und Sample-Loop)

- **Delay → HOST SYNC**: Ist der Schalter an, richtet sich die Delay-Zeit nach dem Tempo der DAW. Der Regler **NOTE** wählt den Notenwert (1/16, 1/8 Triole, 1/8, 1/8 punktiert, 1/4 Triole, 1/4, 1/4 punktiert, 1/2, 1 Takt). Der Regler TIME ist dann ohne Wirkung. Bei sehr langsamem Tempo wird der Wert halbiert, weil der Delay-Puffer 1,5 s fasst.
- **Layer V (Sample) → HOST SYNC**: Ist der Schalter an, dauert der Loop-Bereich (LOOP START bis LOOP END) genau so viele Beats, wie bei **LOOP LENGTH** gewählt ist (1/4 bis 16 Beats). Die Grundtonnote (ROOT NOTE) läuft exakt im Takt, andere Tasten transponieren.
- Ohne DAW-Tempo (Standalone) gilt 120 BPM. Tempoänderungen folgen sofort.
- Die Sync-Einstellungen werden mit Presets gespeichert; ältere Presets bleiben unverändert (Sync aus).
