#pragma once
#include <atomic>
#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Dsp/Engine.h"
#include "ParamSpec.h"
#include "Presets.h"
#include "PresetFile.h"

class CSSynthProcessor : public juce::AudioProcessor
{
public:
    CSSynthProcessor();
    ~CSSynthProcessor() override = default;

    // --- AudioProcessor ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CSSynth 1506"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Parameter ---
    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter* getParam (int index) const { return params[(size_t) index]; }

    // --- MIDI-Learn / CC-Zuordnung (Oxygen 61 Pro oder beliebiger Controller) ---
    void startLearn (int paramIndex)  { learnIndex.store (paramIndex); }
    void cancelLearn()                { learnIndex.store (-1); }
    int  getLearnIndex() const        { return learnIndex.load(); }
    int  getCcForParam (int paramIndex) const;
    void clearMappingForParam (int paramIndex);
    void resetMidiMapToDefaults();
    void clearAllMappings();
    int  getLastCc() const            { return lastCc.load(); }
    int  getLastCcValue() const       { return lastCcValue.load(); }

    // --- Anwender-Presets (.cs1506): Import / Export. Nur im Message-Thread aufrufen. ---
    static juce::File getUserPresetDir();                              // Dokumente/CSSynth/1506 Presets
    static std::vector<juce::File> scanUserPresets();                  // alle .cs1506 im Preset-Ordner
    static juce::File importPresetFile (const juce::File& source);     // prüfen + in den Preset-Ordner kopieren
    bool exportPresetToFile (const juce::File& dest, const juce::String& presetName);
    bool loadPresetFile (const juce::File& file);                      // false = ungültige Datei
    juce::String getCurrentUserPreset() const { return currentUserPreset; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidiMessage (const juce::MidiMessage&);
    void handleController (int cc, int value);
    void applyNormalised (int paramIndex, float normalised);

    cs::Engine engine;
    std::array<juce::RangedAudioParameter*, cs::NUM_PARAMS> params {};
    std::array<std::atomic<float>*, cs::NUM_PARAMS> raw {};

    std::array<std::atomic<int>, 128> ccMap;   // CC-Nummer -> Parameter-Index (-1 = frei)
    std::atomic<int> learnIndex { -1 };
    std::atomic<int> lastCc { -1 }, lastCcValue { 0 };
    int currentProgram = 0;
    juce::String currentUserPreset;   // Dateiname (ohne Endung) des geladenen Anwender-Presets, sonst leer

    juce::AudioBuffer<float> scratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CSSynthProcessor)
};
