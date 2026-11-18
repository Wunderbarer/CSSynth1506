#include "PluginProcessor.h"
#if JUCE_INTEL
 #include <xmmintrin.h>
#endif
#include "PluginEditor.h"
#include <cstring>

using namespace cs;

namespace
{
// Standard-CC-Zuordnung (jederzeit per MIDI-Learn änderbar). Die üblichen Controller-Nummern
// für Regler/Fader des Oxygen 61 Pro lassen sich im Gerät frei belegen - einfach per Learn zuweisen.
struct DefaultCc { int cc; const char* id; };
const DefaultCc kDefaultCc[] = {
    { 7,  "master"   },
    { 74, "l1_lpf"   }, { 71, "l1_res"  }, { 73, "l1_aa"   }, { 72, "l1_ar"   },
    { 75, "l1_fa"    }, { 76, "lfo_rate" }, { 77, "lfo_pitch" }, { 78, "lfo_cut" },
    { 91, "rv_mix"   }, { 93, "ch_mix"  }, { 94, "dl_mix"  },
    { 80, "ds_drive" }, { 81, "ph_mix"  }, { 82, "fl_mix"  }, { 83, "ds_mix"  },
};

juce::NormalisableRange<float> makeRange (const ParamSpec& s)
{
    juce::NormalisableRange<float> r (s.min, s.max, s.step);
    if (s.centre > s.min && s.centre < s.max)
        r.setSkewForCentre (s.centre);
    return r;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout CSSynthProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& s : specs())
    {
        const juce::ParameterID pid { s.id, 1 };
        const juce::String name = s.name;

        switch (s.kind)
        {
            case Kind::Float:
            {
                const ParamSpec copy = s;
                layout.add (std::make_unique<juce::AudioParameterFloat> (
                    pid, name, makeRange (s), s.def,
                    juce::AudioParameterFloatAttributes()
                        .withStringFromValueFunction ([copy] (float v, int) { return juce::String (formatValue (copy, v)); })
                        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));
                break;
            }
            case Kind::Choice:
            {
                juce::StringArray items;
                for (auto& c : s.choices) items.add (c);
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, items, (int) s.def));
                break;
            }
            case Kind::Toggle:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, s.def > 0.5f));
                break;
        }
    }
    return layout;
}

CSSynthProcessor::CSSynthProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CSSYNTH", createLayout())
{
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        const auto& id = specs()[(size_t) i].id;
        params[(size_t) i] = apvts.getParameter (id);
        raw[(size_t) i] = apvts.getRawParameterValue (id);
    }
    for (int i = 0; i < NUM_PARAMS; ++i) { ccValue[(size_t) i].store (0.f); ccDirty[(size_t) i].store (false); ccIdle[(size_t) i] = -1; }
    resetMidiMapToDefaults();
    formats.registerBasicFormats();
    startTimerHz (30);
}

bool CSSynthProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void CSSynthProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    scratch.setSize (2, juce::jmax (samplesPerBlock, 1), false, false, true);
    currentSampleRate = sampleRate;
    diagPush (1, (int) std::lround (sampleRate), (float) samplesPerBlock);
}

//==============================================================================
// Diagnose (Log-Datei: Dokumente/CSSynth/1506 Diagnose.log) - hilft, sporadische Störgeräusche einzugrenzen
void CSSynthProcessor::diagPush (int kind, int a, float f)
{
    const int slot = diagW.fetch_add (1) % kDiagRing;
    auto& e = diagRing[(size_t) slot];
    e.a = a; e.f = f;
    e.kind.store (kind, std::memory_order_release);
}

void CSSynthProcessor::flushDiag()
{
    if (diagFull) return;
    juce::String out;
    while (true)
    {
        auto& e = diagRing[(size_t) (diagR % kDiagRing)];
        const int k = e.kind.load (std::memory_order_acquire);
        if (k == 0) break;
        const int a = e.a; const float f = e.f;
        e.kind.store (0, std::memory_order_release);
        ++diagR;
        const auto t = juce::Time::getCurrentTime().toString (true, true, true, true);
        switch (k)
        {
            case 1: out << t << "  START            Abtastrate " << a << " Hz, Puffer " << (int) f << " Samples\n"; break;
            case 2: out << t << "  MIDI IM LEERLAUF Status 0x" << juce::String::toHexString (a & 0xff).toUpperCase()
                        << "  Daten " << ((a >> 8) & 0x7f) << " " << ((a >> 16) & 0x7f) << "\n"; break;
            case 3: out << t << "  AUSGABE IM LEERLAUF  Spitze " << juce::String (f, 6) << "  (keine Stimme aktiv seit " << a << " s)\n"; break;
            case 4: out << t << "  LANGSAMER BLOCK  " << juce::String (f * 100.f, 0) << " % der Pufferzeit (" << a << " Samples)\n"; break;
            case 5: out << t << "  PUFFERGROESSE    " << a << " Samples\n"; break;
            case 6: out << t << "  DENORMAL-SCHUTZ  MXCSR-Bits 0x" << juce::String::toHexString (a & 0x8040).toUpperCase() << "  (8040 = aktiv)\n"; break;
            default: break;
        }
    }
    if (out.isEmpty()) return;

    const auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("CSSynth");
    const auto file = dir.getChildFile ("1506 Diagnose.log");
    dir.createDirectory();
    if (file.getSize() > 512 * 1024) { diagFull = true; return; }
    if (! diagHeaderWritten)
    {
        diagHeaderWritten = true;
        file.appendText ("--- Diplomat 1506 Diagnose, gestartet " + juce::Time::getCurrentTime().toString (true, true) + " ---\n");
    }
    file.appendText (out);
}

//==============================================================================
void CSSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const double tStart = juce::Time::getMillisecondCounterHiRes();

    const int numSamples = buffer.getNumSamples();
    if (! csrLogged)
    {
        csrLogged = true;
       #if JUCE_INTEL
        diagPush (6, (int) _mm_getcsr(), 0.f);
       #endif
    }
    if (numSamples != lastBlockSize)
    {
        lastBlockSize = numSamples;
        if (blockSizeLogs++ < 12) diagPush (5, numSamples, 0.f);
    }
    if (scratch.getNumSamples() < numSamples)
        scratch.setSize (2, numSamples, false, false, true);

    for (int c = 0; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    if (samplePending.exchange (false))               // neues/entferntes Sample für Layer V übernehmen
        engine.setSample (pendingSample.load());

    Params p;
    for (int i = 0; i < NUM_PARAMS; ++i)
        p.v[(size_t) i] = raw[(size_t) i]->load();
    engine.setParams (p);
    {
        double bpm = 120.0;
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
                if (auto b = pos->getBpm()) bpm = *b;
        engine.setTempo (bpm);
    }

    float* l = scratch.getWritePointer (0);
    float* r = scratch.getWritePointer (1);

    // MIDI sample-genau abarbeiten: Block an jedem Ereignis teilen
    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = juce::jlimit (0, numSamples, meta.samplePosition);
        if (evPos > pos)
        {
            engine.process (l + pos, r + pos, evPos - pos);
            pos = evPos;
        }
        {
            const auto msg = meta.getMessage();
            if (idleSeconds > 10.0 && msg.getRawDataSize() >= 1)
            {
                const auto* d = msg.getRawData();
                diagPush (2, (int) d[0] | ((msg.getRawDataSize() > 1 ? d[1] : 0) << 8) | ((msg.getRawDataSize() > 2 ? d[2] : 0) << 16), 0.f);
            }
            handleMidiMessage (msg);
        }
    }
    if (pos < numSamples)
        engine.process (l + pos, r + pos, numSamples - pos);

    // Diagnose: Leerlauf-Ausgabe und Rechenzeit erfassen
    {
        const double blockSec = (double) numSamples / juce::jmax (currentSampleRate, 1.0);
        runSeconds += blockSec;
        if (engine.hasActiveVoices()) idleSeconds = 0.0; else idleSeconds += blockSec;
        if (idleSeconds > 15.0 && runSeconds - lastIdleOutLog > 2.0)
        {
            float pk = 0.f;
            for (int i = 0; i < numSamples; ++i) pk = juce::jmax (pk, std::fabs (l[i]), std::fabs (r[i]));
            if (pk > 1.0e-5f) { lastIdleOutLog = runSeconds; diagPush (3, (int) idleSeconds, pk); }
        }
        const double load = (juce::Time::getMillisecondCounterHiRes() - tStart) / (blockSec * 1000.0);
        if (load > 0.6 && runSeconds - lastSlowLog > 0.25) { lastSlowLog = runSeconds; diagPush (4, numSamples, (float) load); }
    }

    // Parameter-Änderungen durch CC wirken ab dem nächsten Block
    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, l, numSamples);
        buffer.copyFrom (1, 0, r, numSamples);
    }
    else if (buffer.getNumChannels() == 1)
    {
        float* o = buffer.getWritePointer (0);
        for (int i = 0; i < numSamples; ++i) o[i] = 0.5f * (l[i] + r[i]);
    }
}

void CSSynthProcessor::handleMidiMessage (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())           engine.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff())       engine.allNotesOff();
    else if (m.isAllSoundOff())       engine.allSoundOff();
    else if (m.isPitchWheel())        engine.setPitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.f);
    else if (m.isChannelPressure())   engine.setAftertouch ((float) m.getChannelPressureValue() / 127.f);
    else if (m.isController())        handleController (m.getControllerNumber(), m.getControllerValue());
}

void CSSynthProcessor::handleController (int cc, int value)
{
    lastCc.store (cc);
    lastCcValue.store (value);

    // Sustain-Pedal und Channel-Mode-Nachrichten haben feste Bedeutung und werden nie gelernt
    if (cc == 64)  { engine.setSustain (value >= 64); return; }
    if (cc == 120) { engine.allSoundOff(); return; }
    if (cc == 123) { engine.allNotesOff(); return; }
    if (cc == 0 || cc == 32 || cc >= 120) return;

    // MIDI-Learn: nächster eingehender CC wird dem gewählten Parameter zugewiesen
    const int learn = learnIndex.load();
    if (learn >= 0)
    {
        for (auto& e : ccMap)
            if (e.load() == learn) e.store (-1);
        ccMap[(size_t) cc].store (learn);
        learnedCc.store (cc);
        learnedParam.store (learn);
        learnEvent.fetch_add (1);
        learnIndex.store (-1);
    }

    const int target = ccMap[(size_t) cc].load();
    if (target >= 0)
    {
        applyNormalised (target, (float) value / 127.f);
        return;
    }
    if (cc == 1)
        engine.setModWheel ((float) value / 127.f);
}

void CSSynthProcessor::applyNormalised (int paramIndex, float normalised)
{
    // Audio-Thread: Wert sofort setzen (der Klang folgt unmittelbar), ohne den Host aufzurufen.
    if (auto* p = params[(size_t) paramIndex])
    {
        p->setValue (normalised);
        ccValue[(size_t) paramIndex].store (normalised);
        ccDirty[(size_t) paramIndex].store (true);
    }
}

void CSSynthProcessor::flushMidiParamChanges()
{
    // Message-Thread: Host (Automation / Parameteranzeige) und Oberfläche über CC-Änderungen informieren.
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        auto* p = params[(size_t) i];
        if (p == nullptr) continue;

        if (ccDirty[(size_t) i].exchange (false))
        {
            if (ccIdle[(size_t) i] < 0) p->beginChangeGesture();
            ccIdle[(size_t) i] = 0;
            p->setValueNotifyingHost (ccValue[(size_t) i].load());
        }
        else if (ccIdle[(size_t) i] >= 0 && ++ccIdle[(size_t) i] > 6)   // ca. 200 ms ohne Bewegung: Geste beenden
        {
            p->endChangeGesture();
            ccIdle[(size_t) i] = -1;
        }
    }
}

//==============================================================================
int CSSynthProcessor::getCcForParam (int paramIndex) const
{
    for (int cc = 0; cc < 128; ++cc)
        if (ccMap[(size_t) cc].load() == paramIndex) return cc;
    return -1;
}

void CSSynthProcessor::clearMappingForParam (int paramIndex)
{
    for (auto& e : ccMap)
        if (e.load() == paramIndex) e.store (-1);
}

void CSSynthProcessor::clearAllMappings()
{
    for (auto& e : ccMap) e.store (-1);
}

void CSSynthProcessor::resetMidiMapToDefaults()
{
    clearAllMappings();
    for (const auto& d : kDefaultCc)
    {
        const int idx = indexOf (d.id);
        if (idx >= 0) ccMap[(size_t) d.cc].store (idx);
    }
}

//==============================================================================
int CSSynthProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String CSSynthProcessor::getProgramName (int index)
{
    const auto& p = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? juce::String (p[(size_t) index].name) : juce::String();
}

void CSSynthProcessor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) presets.size())) return;
    currentProgram = index;
    currentUserPreset = {};
    bankMode = SoundbankMode;
    soundbankId = {};
    soundbankPreset = {};

    std::array<float, NUM_PARAMS> values;
    for (int i = 0; i < NUM_PARAMS; ++i) values[(size_t) i] = specs()[(size_t) i].def;
    for (const auto& kv : presets[(size_t) index].values)
    {
        const int idx = indexOf (kv.first);
        if (idx >= 0) values[(size_t) idx] = kv.second;
    }
    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = params[(size_t) i])
            p->setValueNotifyingHost (p->convertTo0to1 (values[(size_t) i]));
}

//==============================================================================
// Anwender-Presets
juce::File CSSynthProcessor::getUserPresetDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("CSSynth").getChildFile ("1506 Presets");
}

std::vector<juce::File> CSSynthProcessor::scanUserPresets()
{
    std::vector<juce::File> out;
    const auto dir = getUserPresetDir();
    if (! dir.isDirectory()) return out;

    for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*" + juce::String (presetfile::extension())))
        out.push_back (f);

    std::sort (out.begin(), out.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    return out;
}

juce::File CSSynthProcessor::importPresetFile (const juce::File& source)
{
    std::array<float, NUM_PARAMS> values {};
    juce::String text;
    if (! presetfile::readFile (source, values, &text))
        return {};

    const auto dir = getUserPresetDir();
    if (source.getParentDirectory() == dir)       // liegt schon im Preset-Ordner
        return source;
    if (! dir.createDirectory().wasOk())
        return {};

    auto base = juce::File::createLegalFileName (source.getFileNameWithoutExtension()).trim();
    if (base.isEmpty()) base = "Preset";

    auto dest = dir.getChildFile (base + presetfile::extension());
    for (int n = 2; dest.exists() && n < 1000; ++n)   // vorhandene Presets nie überschreiben
        dest = dir.getChildFile (base + " " + juce::String (n) + presetfile::extension());
    if (dest.exists()) return {};

    return dest.replaceWithText (text) ? dest : juce::File();
}

bool CSSynthProcessor::exportPresetToFile (const juce::File& dest, const juce::String& presetName, const presetfile::Extras* extras)
{
    std::array<float, NUM_PARAMS> values {};
    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = params[(size_t) i])
            values[(size_t) i] = p->convertFrom0to1 (p->getValue());

    if (! dest.getParentDirectory().createDirectory().wasOk()) return false;
    if (! dest.replaceWithText (presetfile::toXml (values, presetName, extras))) return false;

    if (dest.getParentDirectory() == getUserPresetDir())
    {
        currentUserPreset = dest.getFileNameWithoutExtension();
        bankMode = UserMode;
    }
    return true;
}

bool CSSynthProcessor::loadPresetFile (const juce::File& file, juce::String* info)
{
    std::array<float, NUM_PARAMS> values {};
    presetfile::Extras extras;
    if (! presetfile::readFile (file, values, nullptr, &extras))
        return false;

    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = params[(size_t) i])
            p->setValueNotifyingHost (p->convertTo0to1 (values[(size_t) i]));

    const auto note = applyExtras (extras);
    if (info != nullptr) *info = note;

    currentUserPreset = file.getFileNameWithoutExtension();
    bankMode = UserMode;
    return true;
}

//==============================================================================
// Bänke
juce::File CSSynthProcessor::getBankDir()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("CSSynth").getChildFile ("1506 Banks");
}

std::vector<juce::File> CSSynthProcessor::scanBankFiles()
{
    std::vector<juce::File> out;
    const auto dir = getBankDir();
    if (! dir.isDirectory()) return out;

    for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*" + juce::String (presetfile::bankExtension())))
        out.push_back (f);

    std::sort (out.begin(), out.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileName().compareNatural (b.getFileName()) < 0; });
    return out;
}

bool CSSynthProcessor::readBank (const juce::File& file, presetfile::Bank& out)
{
    return presetfile::readBankFile (file, out);
}

juce::File CSSynthProcessor::importBankFile (const juce::File& source, juce::String& message)
{
    presetfile::Bank bank;
    if (! presetfile::readBankFile (source, bank))
    {
        message = "NOT A VALID SOUNDBANK FILE";
        return {};
    }

    const auto dir = getBankDir();
    if (source.getParentDirectory() == dir)           // liegt schon im Bank-Ordner
        return source;
    if (! dir.createDirectory().wasOk())
    {
        message = "COULD NOT CREATE BANK FOLDER";
        return {};
    }

    const auto text = source.loadFileAsString();
    auto base = juce::File::createLegalFileName (source.getFileNameWithoutExtension()).trim();
    if (base.isEmpty()) base = "Soundbank";

    auto dest = dir.getChildFile (base + presetfile::bankExtension());
    for (int n = 2; dest.exists() && n < 1000; ++n)
    {
        if (dest.loadFileAsString() == text) return dest;       // dieselbe Bank ist schon geladen
        dest = dir.getChildFile (base + " " + juce::String (n) + presetfile::bankExtension());
    }
    if (dest.exists()) { message = "TOO MANY BANKS WITH THIS NAME"; return {}; }

    if (! dest.replaceWithText (text)) { message = "COULD NOT COPY BANK FILE"; return {}; }
    return dest;
}

void CSSynthProcessor::applyBankPreset (const juce::String& id, const presetfile::BankPreset& preset, juce::String* info)
{
    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = params[(size_t) i])
            p->setValueNotifyingHost (p->convertTo0to1 (preset.values[(size_t) i]));

    const auto note = applyExtras (preset.extras);
    if (info != nullptr) *info = note;

    bankMode = SoundbankMode;
    soundbankId = id;
    soundbankPreset = preset.name;
    currentUserPreset = {};
    if (id.isEmpty())
        for (int k = 0; k < getNumPrograms(); ++k)
            if (getProgramName (k) == preset.name) { currentProgram = k; break; }
}

presetfile::Extras CSSynthProcessor::captureExtras (bool includeMidi, bool includeSample) const
{
    presetfile::Extras ex;
    if (includeMidi)
    {
        ex.hasMidi = true;
        for (int cc = 0; cc < 128; ++cc)
        {
            const int idx = ccMap[(size_t) cc].load();
            if (idx >= 0) ex.midi.emplace_back (cc, juce::String (specs()[(size_t) idx].id));
        }
    }
    if (includeSample)
        if (auto s = getSample())
        {
            ex.hasSample = true;
            ex.sampleName = juce::String (s->name);
            ex.sampleData = encodeSample (*s);
        }
    return ex;
}

juce::String CSSynthProcessor::applyExtras (const presetfile::Extras& ex)
{
    juce::String note;
    if (ex.hasMidi)
    {
        learnIndex.store (-1);
        clearAllMappings();
        for (const auto& kv : ex.midi)
        {
            const int idx = indexOf (kv.second.toStdString());
            if (idx >= 0 && juce::isPositiveAndBelow (kv.first, 128)) ccMap[(size_t) kv.first].store (idx);
        }
        note += " + MIDI MAP";
    }
    if (ex.hasSample)
    {
        if (auto s = decodeSample (ex.sampleData, ex.sampleName))
        {
            publishSample (s);
            note += " + SAMPLE";
        }
        else
            note += " (SAMPLE UNREADABLE)";
    }
    return note.trim();
}

juce::File CSSynthProcessor::userPresetFileFor (const juce::String& name)
{
    auto base = juce::File::createLegalFileName (name.trim()).trim();
    if (base.isEmpty()) base = "My Sound";
    return getUserPresetDir().getChildFile (base + presetfile::extension());
}

bool CSSynthProcessor::saveUserPreset (const juce::String& name, bool overwrite, juce::File& savedFile, const presetfile::Extras* extras)
{
    savedFile = userPresetFileFor (name);
    if (savedFile.exists() && ! overwrite) return false;
    return exportPresetToFile (savedFile, savedFile.getFileNameWithoutExtension(), extras);
}

bool CSSynthProcessor::exportUserBank (const juce::File& dest, const juce::String& bankName, int& count)
{
    std::vector<presetfile::BankPreset> list;
    for (const auto& f : scanUserPresets())
    {
        presetfile::BankPreset bp;
        if (! presetfile::readFile (f, bp.values, nullptr, &bp.extras)) continue;
        bp.name = f.getFileNameWithoutExtension();
        list.push_back (std::move (bp));
    }
    count = (int) list.size();
    if (list.empty()) return false;
    if (! dest.getParentDirectory().createDirectory().wasOk()) return false;
    return dest.replaceWithText (presetfile::bankToXml (bankName, list));
}

bool CSSynthProcessor::trashFile (const juce::File& f)
{
    return f.existsAsFile() && (f.moveToTrash() || f.deleteFile());
}

//==============================================================================
void CSSynthProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();

    juce::ValueTree map ("MIDIMAP");
    for (int cc = 0; cc < 128; ++cc)
    {
        const int idx = ccMap[(size_t) cc].load();
        if (idx < 0) continue;
        juce::ValueTree e ("MAP");
        e.setProperty ("cc", cc, nullptr);
        e.setProperty ("param", juce::String (specs()[(size_t) idx].id), nullptr);
        map.appendChild (e, nullptr);
    }
    state.appendChild (map, nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("userPreset", currentUserPreset, nullptr);
    state.setProperty ("bankMode", bankMode, nullptr);
    state.setProperty ("soundbank", soundbankId, nullptr);
    state.setProperty ("soundbankPreset", soundbankPreset, nullptr);

    if (auto s = getSample())                       // geladene Audiodatei gehört zum Projektzustand
    {
        juce::ValueTree smp ("SAMPLE");
        smp.setProperty ("name", juce::String (s->name), nullptr);
        smp.setProperty ("data", encodeSample (*s), nullptr);
        state.appendChild (smp, nullptr);
    }

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void CSSynthProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;

    auto tree = juce::ValueTree::fromXml (*xml);

    auto map = tree.getChildWithName ("MIDIMAP");
    if (map.isValid())
    {
        clearAllMappings();
        for (auto e : map)
        {
            const int cc = (int) e.getProperty ("cc", -1);
            const int idx = indexOf (e.getProperty ("param").toString().toStdString());
            if (cc >= 0 && cc < 128 && idx >= 0) ccMap[(size_t) cc].store (idx);
        }
        tree.removeChild (map, nullptr);
    }
    auto smpTree = tree.getChildWithName ("SAMPLE");
    if (smpTree.isValid())
    {
        if (auto s = decodeSample (smpTree.getProperty ("data").toString(), smpTree.getProperty ("name").toString()))
            publishSample (s);
        tree.removeChild (smpTree, nullptr);
    }
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    currentUserPreset = tree.getProperty ("userPreset").toString();
    {
        const int fallback = currentUserPreset.isNotEmpty() ? (int) UserMode : (int) SoundbankMode;   // Zustand aus älterer Version
        bankMode = (int) tree.getProperty ("bankMode", fallback) == (int) UserMode ? UserMode : SoundbankMode;
    }
    soundbankId = tree.getProperty ("soundbank").toString();
    soundbankPreset = tree.getProperty ("soundbankPreset").toString();
    apvts.replaceState (tree);
}

//==============================================================================
// Sample für Layer V
bool CSSynthProcessor::isSampleFile (const juce::File& f) const
{
    return formats.findFormatForFileExtension (f.getFileExtension().trimCharactersAtStart (".")) != nullptr;
}

juce::String CSSynthProcessor::getSampleWildcard() const
{
    return formats.getWildcardForAllFormats();
}

std::shared_ptr<const cs::SampleData> CSSynthProcessor::getSample() const
{
    const juce::ScopedLock sl (sampleLock);
    return sample;
}

void CSSynthProcessor::publishSample (std::shared_ptr<const cs::SampleData> s)
{
    {
        const juce::ScopedLock sl (sampleLock);
        sample = s;
        if (s != nullptr) keepAlive.push_back (s);
        while (keepAlive.size() > 3) keepAlive.erase (keepAlive.begin());   // Vorgänger bleiben kurz am Leben
    }
    pendingSample.store (s != nullptr ? s.get() : nullptr);
    samplePending.store (true);
    sampleRevision.fetch_add (1);
}

void CSSynthProcessor::clearSample()
{
    publishSample (nullptr);
}

bool CSSynthProcessor::loadSampleFile (const juce::File& file, juce::String& message)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples < 2 || reader->numChannels < 1 || reader->sampleRate < 1000.0)
    {
        message = "COULD NOT READ AUDIO FILE";
        return false;
    }

    const juce::int64 maxFrames = (juce::int64) (kMaxSampleSeconds * reader->sampleRate);
    const bool truncated = reader->lengthInSamples > maxFrames;
    const int frames = (int) juce::jmin (reader->lengthInSamples, maxFrames);
    const int chans = (int) juce::jmin (reader->numChannels, 2u);

    juce::AudioBuffer<float> buf (chans, frames);
    if (! reader->read (&buf, 0, frames, 0, true, chans > 1))
    {
        message = "COULD NOT READ AUDIO FILE";
        return false;
    }

    auto s = std::make_shared<cs::SampleData>();
    s->rate = reader->sampleRate;
    s->name = file.getFileNameWithoutExtension().toStdString();
    s->data.resize ((size_t) frames);
    float peak = 0.f;
    for (int i = 0; i < frames; ++i)
    {
        float v = buf.getSample (0, i);
        if (chans > 1) v = 0.5f * (v + buf.getSample (1, i));     // Stereo -> Mono
        s->data[(size_t) i] = v;
        peak = juce::jmax (peak, std::fabs (v));
    }
    if (peak < 1.0e-5f)
    {
        message = "AUDIO FILE IS SILENT";
        return false;
    }
    const float g = 1.f / peak;                                    // auf Spitzenpegel 1 normalisieren
    for (auto& v : s->data) v *= g;

    publishSample (s);
    message = "LOADED: " + file.getFileName().toUpperCase()
              + (truncated ? " (CUT TO " + juce::String ((int) kMaxSampleSeconds) + " S)" : juce::String());
    return true;
}

// 16-Bit-Mono-WAV im Speicher, als Base64 in den Projektzustand eingebettet
juce::String CSSynthProcessor::encodeSample (const cs::SampleData& s)
{
    const juce::uint32 n = (juce::uint32) s.data.size();
    const juce::uint32 dataBytes = n * 2, rate = (juce::uint32) juce::roundToInt (s.rate);
    juce::MemoryOutputStream out;
    out.write ("RIFF", 4);                 out.writeInt ((int) (36 + dataBytes));
    out.write ("WAVEfmt ", 8);             out.writeInt (16);
    out.writeShort (1);                    out.writeShort (1);
    out.writeInt ((int) rate);             out.writeInt ((int) (rate * 2));
    out.writeShort (2);                    out.writeShort (16);
    out.write ("data", 4);                 out.writeInt ((int) dataBytes);
    for (float v : s.data)
        out.writeShort ((short) juce::jlimit (-32767, 32767, juce::roundToInt (v * 32767.f)));
    return juce::Base64::toBase64 (out.getData(), out.getDataSize());
}

std::shared_ptr<cs::SampleData> CSSynthProcessor::decodeSample (const juce::String& base64, const juce::String& name)
{
    if (base64.isEmpty()) return nullptr;
    juce::MemoryOutputStream decoded;
    if (! juce::Base64::convertFromBase64 (decoded, base64)) return nullptr;

    const auto* p = static_cast<const juce::uint8*> (decoded.getData());
    const size_t size = decoded.getDataSize();
    if (size < 46 || std::memcmp (p, "RIFF", 4) != 0 || std::memcmp (p + 8, "WAVEfmt ", 8) != 0) return nullptr;

    auto rd32 = [&] (size_t o) { return (juce::uint32) p[o] | ((juce::uint32) p[o + 1] << 8) | ((juce::uint32) p[o + 2] << 16) | ((juce::uint32) p[o + 3] << 24); };
    const juce::uint32 rate = rd32 (24);
    const juce::uint32 dataBytes = rd32 (40);
    if (rate < 1000 || rate > 400000 || 44 + (size_t) dataBytes > size || dataBytes < 4) return nullptr;

    auto s = std::make_shared<cs::SampleData>();
    s->rate = (double) rate;
    s->name = name.toStdString();
    const size_t n = std::min ((size_t) (dataBytes / 2), (size_t) (rate * kMaxSampleSeconds) + 16);   // Schutz vor überlangen Daten
    s->data.resize (n);
    for (size_t i = 0; i < n; ++i)
        s->data[i] = (float) (short) (p[44 + 2 * i] | (p[45 + 2 * i] << 8)) * (1.f / 32767.f);
    return s;
}

juce::AudioProcessorEditor* CSSynthProcessor::createEditor() { return new CSSynthEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CSSynthProcessor(); }
