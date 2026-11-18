#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include "PluginProcessor.h"
#include "Look.h"
#include "SampleView.h"

// Oberfläche in festen Entwurfskoordinaten (1320 x 700), die beim Skalieren des Fensters
// als Ganzes vergrößert/verkleinert wird.
class MainPanel : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    static constexpr int W = 1320, H = 794;

    explicit MainPanel (CSSynthProcessor&);
    ~MainPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void paintOverChildren (juce::Graphics&) override;

    // Per Drag & Drop ins Fenster: Preset-Dateien (.cs1506) werden importiert, Audiodateien in Layer V geladen
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;

    // --- Zwei Bänke: SOUNDBANKS (Werks-Bank + geladene Bank-Dateien) und USER (eigene Sounds) ---
    void applyMode (int mode);               // Tabs, Schalter und Listen auf die gewählte Bank umstellen
    void rebuildBankBox();                   // Auswahl der Soundbanks (Werks-Bank + Dateien im Bank-Ordner)
    void rebuildPresetBox();                 // Sound-Liste der gewählten Bank neu aufbauen
    void syncPresetBox();                    // Auswahl an den Prozessor-Zustand angleichen
    void updateBankControls();               // Beschriftung/Aktivierung der Schalter und die Zeile unter der Auswahl
    void stepPreset (int direction);         // vor / zurück durch die Liste der gewählten Bank
    void startImport();                      // USER: einzelne Sounds (.cs1506) importieren
    void startSave();                        // USER: aktuellen Klang unter einem Namen ablegen
    void saveUserSound (const juce::String& name, bool overwrite, bool withMidi, bool withSample);
    void startDeleteUserSound();
    void startExportBank();                  // USER: alle eigenen Sounds als Soundbank-Datei
    void startLoadBank();                    // SOUNDBANKS: Soundbank-Datei laden
    void loadBankFile (const juce::File& f);
    void startRemoveBank();
    void importFiles (const juce::Array<juce::File>& files);
    void showNotice (const juce::String& text);
    void askText (const juce::String& title, const juce::String& message, const juce::String& initial,
                  const juce::String& okText, std::function<void (const juce::String&)> onOk);
    void askSave (const juce::String& initial, bool sampleLoaded,
                  std::function<void (const juce::String&, bool, bool)> onOk);   // Name + Optionen MIDI-Zuordnung / Sample
    void askConfirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                     std::function<void()> onOk);

    template <typename T, typename... Args>
    T* add (Args&&... args)
    {
        auto* c = new T (std::forward<Args> (args)...);
        owned.add (c);
        addAndMakeVisible (c);
        return c;
    }
    KnobCell* knob (int paramIdx, const juce::String& label, juce::Rectangle<int> bounds, int knobSize, int labelH = 16);
    FaderCell* fader (int paramIdx, const juce::String& label, juce::Rectangle<int> bounds, bool accent = false);

    void buildLayer (int layer);
    void buildGlobal();
    void buildSamplePanel();                 // rechte Spalte der Seite "Layer V / Sample"
    void setPage (int page);                 // 0 = Layer I+II, 1 = Layer III+IV, 2 = Layer V (Sample)
    void startLoadSample();                  // Dateidialog
    void loadSampleFromFile (const juce::File& f);
    bool anyAudioFile (const juce::StringArray& files) const;

    CSSynthProcessor& proc;
    CyberLook look;
    juce::OwnedArray<juce::Component> owned;
    std::vector<LearnWidget*> learners;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, resetMidiBtn { "RESET MIDI MAP" }, learnBtn { "MIDI LEARN: OFF" };
    juce::TextButton btnA { "LOAD BANK" }, btnB { "REMOVE" };                 // je nach Bank: LOAD BANK/REMOVE bzw. IMPORT/SAVE
    juce::TextButton tabBtn[2] { juce::TextButton ("SOUNDBANKS"), juce::TextButton ("USER SOUNDS") };
    juce::TextButton deleteBtn { "DELETE" }, exportBtn { "EXPORT BANK" };
    juce::ComboBox bankBox;                                                    // Auswahl der Soundbank (nur im Modus SOUNDBANKS)
    int shownMode = -1;                                                        // zuletzt dargestellte Bank (0 Soundbanks, 1 User)
    juce::String viewBankId;                                                   // angezeigte Soundbank ("" = Werks-Bank, sonst Dateiname)
    std::vector<juce::File> bankFiles;                                         // Dateien im Bank-Ordner (Bank-Auswahl: Id 2 + Index)
    std::unique_ptr<cs::presetfile::Bank> viewBank;                            // eingelesene angezeigte Soundbank (nullptr = Werks-Bank)
    juce::String bankCaption;                                                  // Zeile unter der Sound-Auswahl
    static constexpr int kPages = 3;
    juce::TextButton pageBtn[kPages] { juce::TextButton ("LAYER I + II"), juce::TextButton ("LAYER III + IV"), juce::TextButton ("LAYER V  -  SAMPLE") };
    SampleView* sampleView = nullptr;
    bool dropHint = false;
    int curPage = 0;
    int titleRight = 330;     // rechter Rand des Schriftzugs (wird in paint() gemessen)
    std::vector<juce::Component*> pageComps[kPages];
    juce::String midiText = "NO MIDI CC RECEIVED YET";

    std::vector<juce::File> userFiles;               // Anwender-Presets im Dropdown (Id = 1000 + Index)
    std::unique_ptr<juce::FileChooser> chooser;      // muss während des asynchronen Dialogs leben
    juce::String notice;                             // kurze Statusmeldung in der Fußzeile
    int noticeTicks = 0;
    int seenLearnEvent = 0;                          // zuletzt angezeigte erfolgreiche MIDI-Learn-Zuweisung

    struct Group { juce::String title; juce::Rectangle<int> r; int page = -1; };
    std::vector<Group> groups;       // beschriftete Bereiche (gezeichnet in paint)
    std::vector<juce::Rectangle<int>> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainPanel)
};

class CSSynthEditor : public juce::AudioProcessorEditor
{
public:
    explicit CSSynthEditor (CSSynthProcessor&);
    void resized() override;

private:
    MainPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CSSynthEditor)
};
