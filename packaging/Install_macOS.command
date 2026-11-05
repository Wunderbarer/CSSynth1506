#!/bin/bash
# CSSynth 1506 - Installation unter macOS (Doppelklick, ggf. Rechtsklick > Öffnen)
cd "$(dirname "$0")"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"

echo "Kopiere VST3 nach $VST3_DIR ..."
rm -rf "$VST3_DIR/CSSynth 1506.vst3"
cp -R "VST3/CSSynth 1506.vst3" "$VST3_DIR/"

echo "Kopiere Audio Unit nach $AU_DIR ..."
rm -rf "$AU_DIR/CSSynth 1506.component"
cp -R "AU/CSSynth 1506.component" "$AU_DIR/"

# Die Dateien sind nicht notarisiert: Quarantäne-Markierung entfernen, damit macOS sie ohne Warnung lädt.
xattr -dr com.apple.quarantine "$VST3_DIR/CSSynth 1506.vst3" "$AU_DIR/CSSynth 1506.component" 2>/dev/null
xattr -dr com.apple.quarantine "Standalone/CSSynth 1506.app" 2>/dev/null

echo
echo "Fertig. Das Standalone-Programm liegt im Ordner Standalone (nach Wunsch in 'Programme' ziehen)."
echo "In der DAW die Plugin-Liste neu einlesen. Logic Pro: ggf. Neustart bzw. Einstellungen > Plug-in-Verwaltung > Erneut scannen."
read -n 1 -s -r -p "Taste drücken zum Beenden ..."
