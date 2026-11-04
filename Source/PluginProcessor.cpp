#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    resetMidiMapToDefaults();
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
}

//==============================================================================
void CSSynthProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    if (scratch.getNumSamples() < numSamples)
        scratch.setSize (2, numSamples, false, false, true);

    for (int c = 0; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);

    Params p;
    for (int i = 0; i < NUM_PARAMS; ++i)
        p.v[(size_t) i] = raw[(size_t) i]->load();
    engine.setParams (p);

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
        handleMidiMessage (meta.getMessage());
    }
    if (pos < numSamples)
        engine.process (l + pos, r + pos, numSamples - pos);

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
    if (auto* p = params[(size_t) paramIndex])
        p->setValueNotifyingHost (normalised);
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

bool CSSynthProcessor::exportPresetToFile (const juce::File& dest, const juce::String& presetName)
{
    std::array<float, NUM_PARAMS> values {};
    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = params[(size_t) i])
            values[(size_t) i] = p->convertFrom0to1 (p->getValue());

    if (! dest.getParentDirectory().createDirectory().wasOk()) return false;
    if (! dest.replaceWithText (presetfile::toXml (values, presetName))) return false;

    if (dest.getParentDirectory() == getUserPresetDir())
        currentUserPreset = dest.getFileNameWithoutExtension();
    return true;
}

bool CSSynthProcessor::loadPresetFile (const juce::File& file)
{
    std::array<float, NUM_PARAMS> values {};
    if (! presetfile::readFile (file, values))
        return false;

    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = params[(size_t) i])
            p->setValueNotifyingHost (p->convertTo0to1 (values[(size_t) i]));

    currentUserPreset = file.getFileNameWithoutExtension();
    return true;
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
    currentProgram = juce::jlimit (0, getNumPrograms() - 1, (int) tree.getProperty ("program", 0));
    currentUserPreset = tree.getProperty ("userPreset").toString();
    apvts.replaceState (tree);
}

juce::AudioProcessorEditor* CSSynthProcessor::createEditor() { return new CSSynthEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CSSynthProcessor(); }
