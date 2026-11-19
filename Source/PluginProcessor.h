#pragma once
#include <atomic>
#include <array>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include "Dsp/Engine.h"
#include "ParamSpec.h"
#include "Presets.h"
#include "PresetFile.h"

class CSSynthProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    CSSynthProcessor();
    ~CSSynthProcessor() override { stopTimer(); }

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
    // Learn-Modus (Taster in der Fußzeile): solange er an ist, genügt ein Klick auf ein Bedienelement, um es zu "scharf" zu schalten
    void setLearnArmed (bool on)      { learnArmed.store (on); if (! on) learnIndex.store (-1); }
    bool isLearnArmed() const         { return learnArmed.load(); }
    int  getLearnIndex() const        { return learnIndex.load(); }
    int  getCcForParam (int paramIndex) const;
    void clearMappingForParam (int paramIndex);
    void resetMidiMapToDefaults();
    void clearAllMappings();
    int  getLastCc() const            { return lastCc.load(); }
    int  getLastCcValue() const       { return lastCcValue.load(); }
    // Zähler, der bei jeder erfolgreichen Zuweisung (Learn) hochzählt - die Oberfläche zeigt dann "LEARNED: CC n > Parameter"
    void flushMidiParamChanges();     // meldet CC-Änderungen an Host und Oberfläche (Message-Thread; läuft per Timer)
    int  getLearnEvent() const        { return learnEvent.load(); }
    int  getLearnedCc() const         { return learnedCc.load(); }
    int  getLearnedParam() const      { return learnedParam.load(); }

    // --- Anwender-Presets (.cs1506): Import / Export. Nur im Message-Thread aufrufen. ---
    static juce::File getUserPresetDir();                              // Dokumente/CSSynth/1506 Presets
    static std::vector<juce::File> scanUserPresets();                  // alle .cs1506 im Preset-Ordner
    static juce::File importPresetFile (const juce::File& source);     // prüfen + in den Preset-Ordner kopieren
    bool exportPresetToFile (const juce::File& dest, const juce::String& presetName, const cs::presetfile::Extras* extras = nullptr);
    bool loadPresetFile (const juce::File& file, juce::String* info = nullptr);   // false = ungültige Datei; info: z. B. "+ SAMPLE + MIDI MAP"
    juce::String getCurrentUserPreset() const { return currentUserPreset; }

    // --- Zwei Bänke: SOUNDBANK (Werks-Bank + geladene .cs1506bank-Dateien, nur lesen) und USER (eigene Sounds) ---
    enum BankMode { SoundbankMode = 0, UserMode = 1 };
    int  getBankMode() const                    { return bankMode; }
    void setBankMode (int m)                    { bankMode = m == UserMode ? UserMode : SoundbankMode; }
    juce::String getSoundbankId() const         { return soundbankId; }      // "" = Werks-Bank, sonst Dateiname der geladenen Bank
    juce::String getSoundbankPreset() const     { return soundbankPreset; }  // Name des gewählten Sounds (nur Anzeige / Wiederherstellung)

    static juce::File getBankDir();                                          // Dokumente/CSSynth/1506 Banks
    static std::vector<juce::File> scanBankFiles();                          // alle .cs1506bank dort
    // Bank-Datei prüfen und in den Bank-Ordner übernehmen. Ergebnis {} = ungültig (Fehlertext in message), sonst Zieldatei.
    static juce::File importBankFile (const juce::File& source, juce::String& message);
    static bool readBank (const juce::File& file, cs::presetfile::Bank& out);
    void applyBankPreset (const juce::String& soundbankId, const cs::presetfile::BankPreset& preset, juce::String* info = nullptr);   // Sound einer Soundbank laden

    // Eigene Sounds (USER-Bank): immer im Ordner "1506 Presets". Name -> Datei; vorhandene Datei nur mit overwrite = true ersetzen.
    static juce::File userPresetFileFor (const juce::String& name);
    bool saveUserPreset (const juce::String& name, bool overwrite, juce::File& savedFile, const cs::presetfile::Extras* extras = nullptr);
    bool exportUserBank (const juce::File& dest, const juce::String& bankName, int& count);   // alle eigenen Sounds als .cs1506bank
    // MIDI-Zuordnung und geladenes Sample als Teil eines Sounds (beides optional)
    cs::presetfile::Extras captureExtras (bool includeMidi, bool includeSample) const;
    juce::String applyExtras (const cs::presetfile::Extras& ex);          // Hinweistext fuer die Anzeige, leer = nichts angewendet
    static bool trashFile (const juce::File& f);                              // in den Papierkorb (sonst löschen)

    // --- Sample für Layer V (Audiodatei als fünfter Oszillator). Nur im Message-Thread aufrufen. ---
    static constexpr double kMaxSampleSeconds = 20.0;                  // längere Dateien werden gekürzt
    bool isSampleFile (const juce::File& f) const;                      // Endung gehört zu einem lesbaren Audioformat
    juce::String getSampleWildcard() const;                             // z. B. "*.wav;*.aiff;*.flac;*.ogg;*.mp3"
    bool loadSampleFile (const juce::File& file, juce::String& message); // false = Fehlertext in message, sonst Info
    void clearSample();
    std::shared_ptr<const cs::SampleData> getSample() const;           // nullptr = keins geladen
    int getSeqStep() const { return engine.getSeqStep(); }             // aktueller Schritt des Rhythm-Seq (-1 = keiner)
    int getSampleRevision() const { return sampleRevision.load(); }    // zählt hoch, sobald sich das Sample ändert

    // Zustands-Speicherung des Samples (16-Bit-WAV als Base64) - öffentlich, damit Tests sie nutzen können
    static juce::String encodeSample (const cs::SampleData&);
    static std::shared_ptr<cs::SampleData> decodeSample (const juce::String& base64, const juce::String& name);

private:
    void publishSample (std::shared_ptr<const cs::SampleData> s);
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidiMessage (const juce::MidiMessage&);
    void handleController (int cc, int value);
    void applyNormalised (int paramIndex, float normalised);   // Audio-Thread: wirkt sofort, Host/Oberfläche folgen im Message-Thread
    void timerCallback() override { flushMidiParamChanges(); flushDiag(); }  // 30 Hz

    // --- Diagnose: der Audio-Thread meldet auffällige Ereignisse lock-frei, der Message-Thread schreibt sie in eine Log-Datei ---
    struct DiagEvent { std::atomic<int> kind { 0 }; int a = 0; float f = 0.f; };
    static constexpr int kDiagRing = 256;
    std::array<DiagEvent, kDiagRing> diagRing;
    std::atomic<int> diagW { 0 };
    int diagR = 0;
    bool diagHeaderWritten = false, diagFull = false;
    void diagPush (int kind, int a, float f);   // von jedem Thread aufrufbar
    void flushDiag();                           // Message-Thread
    double idleSeconds = 0.0, lastIdleOutLog = -100.0, lastSlowLog = -100.0, runSeconds = 0.0;
    int lastBlockSize = -1, blockSizeLogs = 0;
    bool csrLogged = false;
    double currentSampleRate = 44100.0;

    cs::Engine engine;
    std::array<juce::RangedAudioParameter*, cs::NUM_PARAMS> params {};
    std::array<std::atomic<float>*, cs::NUM_PARAMS> raw {};

    std::array<std::atomic<int>, 128> ccMap;   // CC-Nummer -> Parameter-Index (-1 = frei)
    std::atomic<int> learnIndex { -1 };
    std::atomic<bool> learnArmed { false };
    std::atomic<int> lastCc { -1 }, lastCcValue { 0 };
    float lastWheelSeen = 0.f;                 // Audio-Thread
    std::atomic<int> learnEvent { 0 }, learnedCc { -1 }, learnedParam { -1 };

    // CC-Änderungen: der Audio-Thread setzt nur den Wert (lock-frei); die Benachrichtigung des Hosts
    // (Automation, Anzeige in der DAW) und der Oberfläche läuft im Message-Thread.
    std::array<std::atomic<float>, cs::NUM_PARAMS> ccValue;
    std::array<std::atomic<bool>, cs::NUM_PARAMS> ccDirty;
    std::array<int, cs::NUM_PARAMS> ccIdle {};       // Message-Thread: Ticks seit der letzten Änderung (-1 = keine offene Geste)
    int currentProgram = 0;
    int bankMode = SoundbankMode;
    juce::String soundbankId, soundbankPreset;
    juce::String currentUserPreset;   // Dateiname (ohne Endung) des geladenen Anwender-Presets, sonst leer

    juce::AudioBuffer<float> scratch;

    juce::AudioFormatManager formats;
    mutable juce::CriticalSection sampleLock;
    std::shared_ptr<const cs::SampleData> sample;                          // aktuelles Sample (Message-Thread)
    std::vector<std::shared_ptr<const cs::SampleData>> keepAlive;          // hält Vorgänger am Leben, bis der Audio-Thread umgeschaltet hat
    std::atomic<const cs::SampleData*> pendingSample { nullptr };
    std::atomic<bool> samplePending { false };
    std::atomic<int> sampleRevision { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CSSynthProcessor)
};
