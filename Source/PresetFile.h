// CSSynth 1506 - Preset-Dateiformat (.cs1506): schlichtes XML, benötigt nur juce_core.
//
//   <CSSynthPreset version="1" synth="CSSynth 1506" name="Mein Sound">
//     <P id="l1_lpf" v="1200"/>
//     <P id="master" v="0.7"/>
//     ...
//   </CSSynthPreset>
//
// Soundbank-Datei (.cs1506bank): mehrere Sounds in einer Datei, zum Laden und Weitergeben.
//
//   <CSSynthBank version="1" synth="CSSynth 1506" name="Meine Bank">
//     <CSSynthPreset name="Sound 1"> <P id="l1_lpf" v="1200"/> ... </CSSynthPreset>
//     <CSSynthPreset name="Sound 2"> ... </CSSynthPreset>
//   </CSSynthBank>
//
// In Bank-Dateien stehen nur Werte, die vom Standardwert abweichen (kleinere Dateien).
//
// Optional (vom Anwender beim Speichern gewählt) enthält ein Sound zusätzlich
//   <MIDIMAP><M cc="21" id="ds_drive"/>...</MIDIMAP>     die MIDI-Zuordnung (Learn) und
//   <SAMPLE name="Datei.wav">Base64 einer 16-Bit-Mono-WAV</SAMPLE>   das geladene Layer-V-Sample.
// Beim Laden ersetzt eine vorhandene MIDIMAP die aktuelle Zuordnung, ein vorhandenes SAMPLE das aktuelle Sample;
// fehlen die Abschnitte, bleibt die aktuelle Zuordnung bzw. das aktuelle Sample unverändert.
//
// Werte stehen in Klartext-Einheiten (Hz, Sekunden, Choice-Index). Fehlende Parameter erhalten
// ihren Standardwert, unbekannte IDs werden ignoriert, Werte werden auf den gültigen Bereich begrenzt.
#pragma once
#include <juce_core/juce_core.h>
#include "ParamSpec.h"
#include <array>
#include <cmath>
#include <vector>

namespace cs
{
namespace presetfile
{
constexpr juce::int64 kMaxBytes = 12 * 1024 * 1024;   // größere Dateien werden nicht gelesen (Platz für ein Sample)

inline const char* extension() { return ".cs1506"; }

// Wert auf den gültigen Bereich des Parameters bringen (NaN/Inf -> Standardwert)
inline float legalise (const ParamSpec& s, float v)
{
    if (! std::isfinite (v)) return s.def;
    v = std::clamp (v, s.min, s.max);
    if (s.kind != Kind::Float) return (float) std::lround (v);
    if (s.step > 0.f)
        v = std::clamp (s.min + std::round ((v - s.min) / s.step) * s.step, s.min, s.max);
    return v;
}

// Optionale Zusätze eines Sounds: MIDI-Zuordnung und Sample
struct Extras
{
    bool hasMidi = false;
    std::vector<std::pair<int, juce::String>> midi;     // CC-Nummer -> Parameter-ID
    bool hasSample = false;
    juce::String sampleName, sampleData;                // Base64 einer 16-Bit-Mono-WAV
};

inline void addExtras (juce::XmlElement& parent, const Extras* ex)
{
    if (ex == nullptr) return;
    if (ex->hasMidi)
    {
        auto* m = parent.createNewChildElement ("MIDIMAP");
        for (const auto& kv : ex->midi)
        {
            auto* e = m->createNewChildElement ("M");
            e->setAttribute ("cc", kv.first);
            e->setAttribute ("id", kv.second);
        }
    }
    if (ex->hasSample && ex->sampleData.isNotEmpty())
    {
        auto* e = parent.createNewChildElement ("SAMPLE");
        e->setAttribute ("name", ex->sampleName);
        e->addTextElement (ex->sampleData);
    }
}

inline void readExtras (const juce::XmlElement& parent, Extras& ex)
{
    ex = {};
    if (auto* m = parent.getChildByName ("MIDIMAP"))
    {
        ex.hasMidi = true;
        for (auto* e : m->getChildWithTagNameIterator ("M"))
        {
            const int cc = e->getIntAttribute ("cc", -1);
            const auto id = e->getStringAttribute ("id");
            if (cc < 0 || cc > 127 || indexOf (id.toStdString()) < 0) continue;     // ungültige Einträge überspringen
            ex.midi.emplace_back (cc, id);
        }
    }
    if (auto* sm = parent.getChildByName ("SAMPLE"))
    {
        auto data = sm->getAllSubText().removeCharacters (" \t\r\n");
        if (data.isNotEmpty())
        {
            ex.hasSample = true;
            ex.sampleName = sm->getStringAttribute ("name");
            ex.sampleData = data;
        }
    }
}

inline juce::String toXml (const std::array<float, NUM_PARAMS>& values, const juce::String& name, const Extras* extras = nullptr)
{
    juce::XmlElement root ("CSSynthPreset");
    root.setAttribute ("version", 1);
    root.setAttribute ("synth", "CSSynth 1506");
    root.setAttribute ("name", name);
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        if (i == GP_PITCHWHEEL) continue;          // Spielhilfe, gehört nicht zum Klang
        auto* e = root.createNewChildElement ("P");
        e->setAttribute ("id", juce::String (specs()[(size_t) i].id));
        e->setAttribute ("v", (double) values[(size_t) i]);
    }
    addExtras (root, extras);
    return root.toString();
}

// <CSSynthPreset>-Element -> Parameterwerte. false = keine gültigen Parameter enthalten.
inline bool fromElement (const juce::XmlElement& xml, std::array<float, NUM_PARAMS>& values, Extras* extras = nullptr)
{
    if (extras != nullptr) readExtras (xml, *extras);
    for (int i = 0; i < NUM_PARAMS; ++i)
        values[(size_t) i] = specs()[(size_t) i].def;

    int found = 0;
    for (auto* e : xml.getChildWithTagNameIterator ("P"))
    {
        const int idx = indexOf (e->getStringAttribute ("id").toStdString());
        if (idx < 0 || ! e->hasAttribute ("v")) continue;
        values[(size_t) idx] = legalise (specs()[(size_t) idx], (float) e->getDoubleAttribute ("v"));
        ++found;
    }
    return found > 0;
}

// Text -> Parameterwerte. false = keine gültige Preset-Datei.
inline bool fromXml (const juce::String& text, std::array<float, NUM_PARAMS>& values, Extras* extras = nullptr)
{
    auto xml = juce::XmlDocument::parse (text);
    if (xml == nullptr || ! xml->hasTagName ("CSSynthPreset"))
        return false;
    return fromElement (*xml, values, extras);
}

inline bool readFile (const juce::File& f, std::array<float, NUM_PARAMS>& values, juce::String* textOut = nullptr, Extras* extras = nullptr)
{
    if (! f.existsAsFile()) return false;
    const auto size = f.getSize();
    if (size <= 0 || size > kMaxBytes) return false;

    const auto text = f.loadFileAsString();
    if (! fromXml (text, values, extras)) return false;
    if (textOut != nullptr) *textOut = text;
    return true;
}

// ---------------------------------------------------------------------------------------------
// Soundbanks
constexpr juce::int64 kMaxBankBytes = 96 * 1024 * 1024;
constexpr int kMaxBankPresets = 2000;

inline const char* bankExtension() { return ".cs1506bank"; }

struct BankPreset { juce::String name; std::array<float, NUM_PARAMS> values {}; Extras extras; };
struct Bank       { juce::String name; std::vector<BankPreset> presets; };

inline juce::String bankToXml (const juce::String& bankName, const std::vector<BankPreset>& presets)
{
    juce::XmlElement root ("CSSynthBank");
    root.setAttribute ("version", 1);
    root.setAttribute ("synth", "CSSynth 1506");
    root.setAttribute ("name", bankName);
    for (const auto& pr : presets)
    {
        auto* e = root.createNewChildElement ("CSSynthPreset");
        e->setAttribute ("name", pr.name);
        for (int i = 0; i < NUM_PARAMS; ++i)
        {
            const auto& sp = specs()[(size_t) i];
            if (std::fabs (pr.values[(size_t) i] - sp.def) < 1e-6f) continue;     // Standardwerte weglassen
            auto* p = e->createNewChildElement ("P");
            p->setAttribute ("id", juce::String (sp.id));
            p->setAttribute ("v", (double) pr.values[(size_t) i]);
        }
        addExtras (*e, &pr.extras);
    }
    return root.toString();
}

// Ungültige Einzelsounds werden übersprungen. false = keine Bank-Datei oder kein einziger gültiger Sound.
inline bool bankFromXml (const juce::String& text, Bank& out)
{
    auto xml = juce::XmlDocument::parse (text);
    if (xml == nullptr || ! xml->hasTagName ("CSSynthBank"))
        return false;

    out.name = xml->getStringAttribute ("name").trim();
    out.presets.clear();
    for (auto* e : xml->getChildWithTagNameIterator ("CSSynthPreset"))
    {
        if ((int) out.presets.size() >= kMaxBankPresets) break;
        BankPreset bp;
        bp.name = e->getStringAttribute ("name").trim();
        if (bp.name.isEmpty()) bp.name = "Sound " + juce::String ((int) out.presets.size() + 1);
        // ein Sound darf auch nur aus Standardwerten bestehen (z. B. "Init"): Element zählt trotzdem
        fromElement (*e, bp.values, &bp.extras);
        out.presets.push_back (std::move (bp));
    }
    return ! out.presets.empty();
}

inline bool readBankFile (const juce::File& f, Bank& out)
{
    if (! f.existsAsFile()) return false;
    const auto size = f.getSize();
    if (size <= 0 || size > kMaxBankBytes) return false;
    if (! bankFromXml (f.loadFileAsString(), out)) return false;
    if (out.name.isEmpty()) out.name = f.getFileNameWithoutExtension();
    return true;
}
} // namespace presetfile
} // namespace cs
