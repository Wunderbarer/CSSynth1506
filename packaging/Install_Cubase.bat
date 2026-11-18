@echo off
setlocal
rem CSSynth 1506 / Diplomat - automatische Installation fuer Cubase (VST3)
rem Fordert Administratorrechte selbst an, kopiert das Plugin in den VST3-Ordner,
rem den Cubase automatisch durchsucht, und prueft, welche Cubase-Versionen vorhanden sind.

rem --- Administratorrechte pruefen, sonst neu starten (UAC-Abfrage) ---
net session >nul 2>&1
if errorlevel 1 (
  echo Administratorrechte werden angefordert ...
  powershell -NoProfile -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
  exit /b
)

set "SRC=%~dp0VST3\CSSynth 1506.vst3"
set "TARGET=%CommonProgramFiles%\VST3"

if not exist "%SRC%" (
  echo FEHLER: "%SRC%" wurde nicht gefunden.
  echo Bitte das ZIP zuerst vollstaendig entpacken und diese Datei im entpackten Ordner starten.
  goto :ende
)

rem --- laufendes Cubase kann die Datei sperren ---
tasklist /FI "IMAGENAME eq Cubase*" 2>nul | find /I "Cubase" >nul
if not errorlevel 1 (
  echo.
  echo ACHTUNG: Cubase laeuft noch. Bitte Cubase schliessen und danach eine Taste druecken.
  pause
)

echo.
echo Installiere nach "%TARGET%" ...
if exist "%TARGET%\CSSynth 1506.vst3" rmdir /S /Q "%TARGET%\CSSynth 1506.vst3" >nul 2>&1
xcopy /E /I /Y /Q "%SRC%" "%TARGET%\CSSynth 1506.vst3" >nul
if errorlevel 1 (
  echo FEHLER beim Kopieren. Ist Cubase oder ein anderes Programm noch geoeffnet?
  goto :ende
)
echo Plugin kopiert.

rem --- Cubase-Installationen anzeigen ---
echo.
echo Gefundene Cubase-Installationen:
set "FOUND=0"
for /d %%D in ("%ProgramFiles%\Steinberg\Cubase*") do (
  echo    %%~nxD
  set "FOUND=1"
)
if "%FOUND%"=="0" echo    keine im Standardordner gefunden ^(das Plugin ist trotzdem installiert^)

echo.
echo FERTIG. Naechste Schritte in Cubase:
echo    1. Cubase starten.
echo    2. Studio ^> VST-Plugin-Verwaltung ^(VST Plug-in Manager^) oeffnen.
echo    3. Unten links auf das Aktualisieren-Symbol ^(Refresh^) klicken.
echo    4. "CSSynth 1506" erscheint unter Instrumente. Spur: Rechtsklick ^> Instrumentenspur hinzufuegen ^> CSSynth 1506.
echo    Falls es in der Liste auf der Sperrliste ^(Blocklist^) steht: Eintrag markieren und "Entfernen" waehlen.

:ende
echo.
pause
