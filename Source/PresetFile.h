// CSSynth 1506 - Preset-Dateiformat (.cs1506): schlichtes XML, benötigt nur juce_core.
//
//   <CSSynthPreset version="1" synth="CSSynth 1506" name="Mein Sound">
//     <P id="l1_lpf" v="1200"/>
//     <P id="master" v="0.7"/>
//     ...
//   </CSSynthPreset>
//
// Werte stehen in Klartext-Einheiten (Hz, Sekunden, Choice-Index). Fehlende Parameter erhalten
// ihren Standardwert, unbekannte IDs werden ignoriert, Werte werden auf den gültigen Bereich begrenzt.
#pragma once
#include <juce_core/juce_core.h>
#include "ParamSpec.h"
#include <array>
#include <cmath>

namespace cs
{
namespace presetfile
{
constexpr juce::int64 kMaxBytes = 1024 * 1024;   // größere Dateien werden nicht gelesen

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

inline juce::String toXml (const std::array<float, NUM_PARAMS>& values, const juce::String& name)
{
    juce::XmlElement root ("CSSynthPreset");
    root.setAttribute ("version", 1);
    root.setAttribute ("synth", "CSSynth 1506");
    root.setAttribute ("name", name);
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        auto* e = root.createNewChildElement ("P");
        e->setAttribute ("id", juce::String (specs()[(size_t) i].id));
        e->setAttribute ("v", (double) values[(size_t) i]);
    }
    return root.toString();
}

// Text -> Parameterwerte. false = keine gültige Preset-Datei.
inline bool fromXml (const juce::String& text, std::array<float, NUM_PARAMS>& values)
{
    auto xml = juce::XmlDocument::parse (text);
    if (xml == nullptr || ! xml->hasTagName ("CSSynthPreset"))
        return false;

    for (int i = 0; i < NUM_PARAMS; ++i)
        values[(size_t) i] = specs()[(size_t) i].def;

    int found = 0;
    for (auto* e : xml->getChildWithTagNameIterator ("P"))
    {
        const int idx = indexOf (e->getStringAttribute ("id").toStdString());
        if (idx < 0 || ! e->hasAttribute ("v")) continue;
        values[(size_t) idx] = legalise (specs()[(size_t) idx], (float) e->getDoubleAttribute ("v"));
        ++found;
    }
    return found > 0;
}

inline bool readFile (const juce::File& f, std::array<float, NUM_PARAMS>& values, juce::String* textOut = nullptr)
{
    if (! f.existsAsFile()) return false;
    const auto size = f.getSize();
    if (size <= 0 || size > kMaxBytes) return false;

    const auto text = f.loadFileAsString();
    if (! fromXml (text, values)) return false;
    if (textOut != nullptr) *textOut = text;
    return true;
}
} // namespace presetfile
} // namespace cs
