// CSSynth 1506 - Parameter-Tabelle (ohne JUCE-Abhängigkeit, damit auch die Tests sie nutzen können)
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace cs
{
// ---- Parameter pro Layer (jeder der zwei Layer hat denselben Satz) ----
enum LayerP
{
    LP_WAVE, LP_OCT, LP_DETUNE, LP_PW, LP_SUB, LP_NOISE,          // Oszillator
    LP_HPF, LP_LPF, LP_RES, LP_BRILL, LP_KEY, LP_AMT,             // Filter
    LP_FIL, LP_FA, LP_FAL, LP_FD, LP_FSL, LP_FR,                  // Filter-Hüllkurve (IL A AL D SL R)
    LP_AA, LP_AD, LP_AS, LP_AR,                                   // Verstärker-Hüllkurve
    LP_LEVEL,
    LP_COUNT
};

// ---- Globale Parameter ----
enum GlobalP
{
    GP_MASTER = LP_COUNT * 2,
    GP_GLIDE, GP_BEND,
    GP_LFO_RATE, GP_LFO_SHAPE, GP_LFO_PITCH, GP_LFO_CUT, GP_LFO_PWM,
    GP_AT_FILTER, GP_AT_VIB, GP_VEL_AMP, GP_VEL_FLT,
    GP_CH_RATE, GP_CH_DEPTH, GP_CH_MIX,
    GP_DL_TIME, GP_DL_FB, GP_DL_MIX, GP_DL_PING,
    GP_RV_SIZE, GP_RV_DAMP, GP_RV_MIX,
    NUM_PARAMS
};

enum class Kind { Float, Choice, Toggle };
enum class Fmt { Ten, Percent, Hz, HzLfo, Seconds, Ms, Cents, Semis, Plain };

struct ParamSpec
{
    std::string id, name;
    Kind kind = Kind::Float;
    float min = 0.f, max = 1.f, def = 0.f;
    float centre = 0.f; // >0: Skew so, dass dieser Wert in der Reglermitte liegt
    float step = 0.f;
    Fmt fmt = Fmt::Ten;
    std::vector<std::string> choices;
};

inline std::string formatValue (const ParamSpec& s, float v)
{
    char b[64];
    if (s.kind == Kind::Choice)
    {
        const int i = std::clamp ((int) std::lround (v), 0, (int) s.choices.size() - 1);
        return s.choices[(size_t) i];
    }
    if (s.kind == Kind::Toggle)
        return v > 0.5f ? "On" : "Off";

    switch (s.fmt)
    {
        case Fmt::Ten:     std::snprintf (b, sizeof b, "%.1f", v * 10.f); break;
        case Fmt::Percent: std::snprintf (b, sizeof b, "%.0f %%", v * 100.f); break;
        case Fmt::Hz:
            if (v >= 1000.f)    std::snprintf (b, sizeof b, "%.2f kHz", v / 1000.f);
            else if (v >= 100.f) std::snprintf (b, sizeof b, "%.0f Hz", v);
            else                std::snprintf (b, sizeof b, "%.1f Hz", v);
            break;
        case Fmt::HzLfo:   std::snprintf (b, sizeof b, "%.2f Hz", v); break;
        case Fmt::Seconds:
            if (v < 1.f) std::snprintf (b, sizeof b, "%.0f ms", v * 1000.f);
            else         std::snprintf (b, sizeof b, "%.2f s", v);
            break;
        case Fmt::Ms:      std::snprintf (b, sizeof b, "%.0f ms", v); break;
        case Fmt::Cents:   std::snprintf (b, sizeof b, "%+.0f ct", v); break;
        case Fmt::Semis:   std::snprintf (b, sizeof b, "%.0f st", v); break;
        default:           std::snprintf (b, sizeof b, "%.2f", v); break;
    }
    return b;
}

inline const std::array<ParamSpec, NUM_PARAMS>& specs()
{
    static const std::array<ParamSpec, NUM_PARAMS> table = []
    {
        std::array<ParamSpec, NUM_PARAMS> a;

        auto F = [&] (int i, const std::string& id, const std::string& name, float mn, float mx, float def,
                      Fmt fmt, float centre = 0.f, float step = 0.f)
        {
            ParamSpec s; s.id = id; s.name = name; s.kind = Kind::Float;
            s.min = mn; s.max = mx; s.def = def; s.fmt = fmt; s.centre = centre; s.step = step;
            a[(size_t) i] = s;
        };
        auto C = [&] (int i, const std::string& id, const std::string& name, std::vector<std::string> ch, int def)
        {
            ParamSpec s; s.id = id; s.name = name; s.kind = Kind::Choice;
            s.choices = std::move (ch); s.min = 0.f; s.max = (float) s.choices.size() - 1.f; s.def = (float) def; s.step = 1.f;
            a[(size_t) i] = s;
        };
        auto T = [&] (int i, const std::string& id, const std::string& name, bool def)
        {
            ParamSpec s; s.id = id; s.name = name; s.kind = Kind::Toggle;
            s.min = 0.f; s.max = 1.f; s.def = def ? 1.f : 0.f; s.step = 1.f;
            a[(size_t) i] = s;
        };

        for (int L = 0; L < 2; ++L)
        {
            const int o = L * LP_COUNT;
            const std::string p = L == 0 ? "l1_" : "l2_";
            const std::string n = L == 0 ? "L1 " : "L2 ";

            C (o + LP_WAVE,   p + "wave",   n + "Wave",   { "Saw", "Square", "Pulse" }, 0);
            C (o + LP_OCT,    p + "oct",    n + "Octave", { "32'", "16'", "8'", "4'", "2'" }, 2);
            F (o + LP_DETUNE, p + "detune", n + "Detune", -50.f, 50.f, 0.f, Fmt::Cents);
            F (o + LP_PW,     p + "pw",     n + "Pulse Width", 0.05f, 0.95f, 0.5f, Fmt::Percent);
            F (o + LP_SUB,    p + "sub",    n + "Sub",    0.f, 1.f, 0.f, Fmt::Ten);
            F (o + LP_NOISE,  p + "noise",  n + "Noise",  0.f, 1.f, 0.f, Fmt::Ten);

            F (o + LP_HPF,    p + "hpf",    n + "HP Cutoff", 20.f, 8000.f, 20.f, Fmt::Hz, 400.f);
            F (o + LP_LPF,    p + "lpf",    n + "LP Cutoff", 40.f, 18000.f, 3000.f, Fmt::Hz, 1500.f);
            F (o + LP_RES,    p + "res",    n + "Resonance", 0.f, 1.f, 0.15f, Fmt::Ten);
            F (o + LP_BRILL,  p + "brill",  n + "Brilliance", 0.f, 1.f, 0.5f, Fmt::Ten);
            F (o + LP_KEY,    p + "key",    n + "Key Track", 0.f, 1.f, 0.5f, Fmt::Ten);
            F (o + LP_AMT,    p + "amt",    n + "Filter Env Amount", 0.f, 1.f, 0.35f, Fmt::Ten);

            F (o + LP_FIL,    p + "fil",    n + "Filter IL",  0.f, 1.f, 0.f, Fmt::Ten);
            F (o + LP_FA,     p + "fa",     n + "Filter Attack", 0.001f, 10.f, 0.01f, Fmt::Seconds, 0.3f);
            F (o + LP_FAL,    p + "fal",    n + "Filter AL",  0.f, 1.f, 1.f, Fmt::Ten);
            F (o + LP_FD,     p + "fd",     n + "Filter Decay", 0.001f, 10.f, 0.6f, Fmt::Seconds, 0.3f);
            F (o + LP_FSL,    p + "fsl",    n + "Filter SL",  0.f, 1.f, 0.5f, Fmt::Ten);
            F (o + LP_FR,     p + "fr",     n + "Filter Release", 0.001f, 10.f, 0.3f, Fmt::Seconds, 0.3f);

            F (o + LP_AA,     p + "aa",     n + "Amp Attack", 0.001f, 10.f, 0.005f, Fmt::Seconds, 0.3f);
            F (o + LP_AD,     p + "ad",     n + "Amp Decay",  0.001f, 10.f, 0.4f, Fmt::Seconds, 0.3f);
            F (o + LP_AS,     p + "as",     n + "Amp Sustain", 0.f, 1.f, 0.8f, Fmt::Ten);
            F (o + LP_AR,     p + "ar",     n + "Amp Release", 0.001f, 10.f, 0.25f, Fmt::Seconds, 0.3f);

            F (o + LP_LEVEL,  p + "level",  n + "Level", 0.f, 1.f, L == 0 ? 0.8f : 0.f, Fmt::Ten);
        }

        F (GP_MASTER,    "master",    "Master Volume", 0.f, 1.f, 0.7f, Fmt::Ten);
        F (GP_GLIDE,     "glide",     "Glide",         0.f, 2.f, 0.f, Fmt::Seconds, 0.25f);
        F (GP_BEND,      "bend",      "Bend Range",    1.f, 12.f, 2.f, Fmt::Semis, 0.f, 1.f);
        F (GP_LFO_RATE,  "lfo_rate",  "LFO Rate",      0.1f, 20.f, 5.f, Fmt::HzLfo, 3.f);
        C (GP_LFO_SHAPE, "lfo_shape", "LFO Shape",     { "Sine", "Triangle", "Saw", "Sample&Hold" }, 0);
        F (GP_LFO_PITCH, "lfo_pitch", "LFO > Pitch",   0.f, 1.f, 0.f, Fmt::Ten);
        F (GP_LFO_CUT,   "lfo_cut",   "LFO > Cutoff",  0.f, 1.f, 0.f, Fmt::Ten);
        F (GP_LFO_PWM,   "lfo_pwm",   "LFO > PWM",     0.f, 1.f, 0.f, Fmt::Ten);
        F (GP_AT_FILTER, "at_filter", "Aftertouch > Cutoff", 0.f, 1.f, 0.3f, Fmt::Ten);
        F (GP_AT_VIB,    "at_vib",    "Aftertouch > Vibrato", 0.f, 1.f, 0.3f, Fmt::Ten);
        F (GP_VEL_AMP,   "vel_amp",   "Velocity > Volume", 0.f, 1.f, 0.5f, Fmt::Ten);
        F (GP_VEL_FLT,   "vel_flt",   "Velocity > Cutoff", 0.f, 1.f, 0.f, Fmt::Ten);

        F (GP_CH_RATE,   "ch_rate",   "Chorus Rate",   0.05f, 5.f, 0.6f, Fmt::HzLfo, 0.8f);
        F (GP_CH_DEPTH,  "ch_depth",  "Chorus Depth",  0.f, 1.f, 0.5f, Fmt::Ten);
        F (GP_CH_MIX,    "ch_mix",    "Chorus Mix",    0.f, 1.f, 0.3f, Fmt::Ten);
        F (GP_DL_TIME,   "dl_time",   "Delay Time",    20.f, 1200.f, 380.f, Fmt::Ms, 300.f);
        F (GP_DL_FB,     "dl_fb",     "Delay Feedback", 0.f, 0.95f, 0.35f, Fmt::Ten);
        F (GP_DL_MIX,    "dl_mix",    "Delay Mix",     0.f, 1.f, 0.f, Fmt::Ten);
        T (GP_DL_PING,   "dl_ping",   "Delay Ping-Pong", true);
        F (GP_RV_SIZE,   "rv_size",   "Reverb Size",   0.f, 1.f, 0.6f, Fmt::Ten);
        F (GP_RV_DAMP,   "rv_damp",   "Reverb Damping", 0.f, 1.f, 0.5f, Fmt::Ten);
        F (GP_RV_MIX,    "rv_mix",    "Reverb Mix",    0.f, 1.f, 0.2f, Fmt::Ten);
        return a;
    }();
    return table;
}

inline int indexOf (const std::string& id)
{
    const auto& s = specs();
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i].id == id) return (int) i;
    return -1;
}
} // namespace cs
