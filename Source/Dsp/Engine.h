// CSSynth 1506 - Klangerzeugung (ohne JUCE-Abhängigkeit)
//
// Aufbau, angelehnt an den Yamaha CS-80: 8 Stimmen, jede Stimme besteht aus zwei unabhängigen
// Layern (I und II). Jeder Layer: Oszillator (+Sub, Rauschen) -> 12 dB Hochpass -> 12 dB Tiefpass
// mit Resonanz -> Brilliance -> VCA. Filter-Hüllkurve mit IL/A/AL/D/SL/R, Verstärker-Hüllkurve ADSR.
// Danach globale Effekte: Chorus -> Delay -> Reverb -> Master mit weichem Limiter.
#pragma once
#include "../ParamSpec.h"
#include "Effects.h"
#include <array>
#include <cstdint>

namespace cs
{
struct Params
{
    std::array<float, NUM_PARAMS> v {};
    float operator[] (int i) const { return v[(size_t) i]; }
};

struct EnvCfg
{
    float il = 0.f, al = 1.f, sl = 1.f, rel = 0.f;
    float ca = 0.01f, cd = 0.01f, cr = 0.01f; // Koeffizienten pro Sample
};

// Analoge Hüllkurve: exponentielle Annäherung mit leichtem Überschwingziel, damit die Stufen
// in der eingestellten Zeit sauber ihr Ziel erreichen.
struct Env
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    float level = 0.f;
    Stage st = Idle;

    void gateOn (const EnvCfg& c) { if (st == Idle) level = c.il; st = Attack; }
    void gateOff()                { if (st != Idle) st = Release; }
    bool active() const           { return st != Idle; }

    float tick (const EnvCfg& c)
    {
        switch (st)
        {
            case Attack:  approach (c.al, c.ca, Decay); break;
            case Decay:   approach (c.sl, c.cd, Sustain); break;
            case Sustain: level += (c.sl - level) * 0.01f; break;
            case Release: approach (c.rel, c.cr, Idle); break;
            case Idle:    break;
        }
        return level;
    }

private:
    void approach (float target, float coef, Stage next)
    {
        if (level < target)
        {
            level += (target + 0.1f - level) * coef;
            if (level >= target) { level = target; st = next; }
        }
        else if (level > target)
        {
            level += (target - 0.1f - level) * coef;
            if (level <= target) { level = target; st = next; }
        }
        else
            st = next;
    }
};

struct Svf
{
    float ic1 = 0.f, ic2 = 0.f;
};

struct SvfCoef
{
    float a1 = 0.f, a2 = 0.f, a3 = 0.f, k = 1.4f;
};

class Engine
{
public:
    static constexpr int kVoices = 8;
    static constexpr int kChunk = 16; // Kontrollrate: alle 16 Samples

    void prepare (double sampleRate);
    void setParams (const Params& p);

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustain (bool down);
    void allNotesOff();
    void allSoundOff();

    void setPitchBend (float v)  { bend = v; }       // -1..1
    void setModWheel (float v)   { modWheel = v; }   // 0..1
    void setAftertouch (float v) { aftertouch = v; } // 0..1

    void process (float* left, float* right, int numSamples);

private:
    struct LayerVoice
    {
        float ph = 0.f, subPh = 0.f, tone = 0.f;
        Svf hp, lp;
        Env fe, ae;
    };
    struct Voice
    {
        bool active = false, gate = false, keyDown = false, sustained = false;
        int note = -1;
        float vel = 0.f, cur = 60.f, tgt = 60.f;
        std::uint64_t age = 0;
        LayerVoice lv[2];
    };

    Voice* allocate();
    void releaseVoice (Voice& v);
    void renderChunk (float* l, float* r, int n);
    void renderVoice (Voice& v, float* mono, int n);
    float noise();

    Params P;
    std::array<Voice, kVoices> voices;
    EnvCfg fcfg[2], acfg[2];

    float sr = 44100.f;
    float bend = 0.f, bendS = 0.f, modWheel = 0.f, modS = 0.f, aftertouch = 0.f, atS = 0.f;
    float glideCoef = 1.f;
    float lfoPhase = 0.f, lfoVal = 0.f, lfoSH = 0.f;
    int lastNote = -1;
    bool sustain = false;
    std::uint64_t counter = 0;
    std::uint32_t rng = 22222u;

    // geglättete Filterwerte (pro Layer)
    float hpS[2] { 20.f, 20.f }, lpS[2] { 3000.f, 3000.f }, resS[2] { 0.f, 0.f };

    Chorus chorus;
    Delay delay;
    Reverb reverb;
};
} // namespace cs
