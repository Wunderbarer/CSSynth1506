#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Look.h"

// Oberfläche in festen Entwurfskoordinaten (1320 x 700), die beim Skalieren des Fensters
// als Ganzes vergrößert/verkleinert wird.
class MainPanel : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
    static constexpr int W = 1320, H = 700;

    explicit MainPanel (CSSynthProcessor&);
    ~MainPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // Preset-Dateien (.cs1506) per Drag & Drop ins Fenster importieren
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;

    // --- Preset-Auswahl, Import und Export ---
    void rebuildPresetBox();                 // Werks-Presets + Anwender-Presets neu einlesen
    void syncPresetBox();                    // Auswahl an den Prozessor-Zustand angleichen
    void stepPreset (int direction);         // vor / zurück durch die gesamte Liste
    void startImport();
    void startSave();
    void importFiles (const juce::Array<juce::File>& files);
    void showNotice (const juce::String& text);

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

    CSSynthProcessor& proc;
    VintageLook look;
    juce::OwnedArray<juce::Component> owned;
    std::vector<LearnWidget*> learners;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, resetMidiBtn { "RESET MIDI MAP" };
    juce::TextButton importBtn { "IMPORT" }, saveBtn { "SAVE" };
    juce::String midiText = "NO MIDI CC RECEIVED YET";

    std::vector<juce::File> userFiles;               // Anwender-Presets im Dropdown (Id = 1000 + Index)
    std::unique_ptr<juce::FileChooser> chooser;      // muss während des asynchronen Dialogs leben
    juce::String notice;                             // kurze Statusmeldung in der Fußzeile
    int noticeTicks = 0;

    struct Group { juce::String title; juce::Rectangle<int> r; };
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
