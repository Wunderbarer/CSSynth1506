// CSSynth 1506 - Klangerzeugung (ohne JUCE-Abhängigkeit)
//
// Aufbau, angelehnt an den Yamaha CS-80: 16 Stimmen, jede Stimme besteht aus fünf unabhängigen
// Layern (I - V). Jeder Layer: Oszillator (+Sub, Rauschen) -> 12 dB Hochpass -> 12 dB Tiefpass
// mit Resonanz -> Brilliance -> VCA. Filter-Hüllkurve mit IL/A/AL/D/SL/R, Verstärker-Hüllkurve ADSR.
// Layer IV ist ein Formant-Oszillator (Vokale A E I O U, stufenlos überblendbar): Sägezahn-Quelle
// -> drei parallele Bandpass-Resonatoren (F1-F3) -> weiter im normalen Layer-Signalweg.
// Layer V ist ein Sample-Oszillator: eine geladene Audiodatei wird tonhöhenrichtig abgespielt (Loop,
// One-Shot oder Ping-Pong) und läuft durch denselben Signalweg wie die anderen Layer.
// Danach globale Effekte: Distortion -> Phaser -> Flanger -> Chorus -> Delay -> Reverb -> Master mit weichem Limiter.
#pragma once
#include "../ParamSpec.h"
#include "Effects.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

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

// Geladene Audiodatei für den Sample-Layer (mono, auf Spitzenpegel 1 normalisiert).
struct SampleData
{
    std::vector<float> data;
    double rate = 44100.0;
    std::string name;
    bool empty() const { return data.size() < 2; }
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
    static constexpr int kVoices = 16;
    static constexpr int kChunk = 16; // Kontrollrate: alle 16 Samples

    void prepare (double sampleRate);
    void setParams (const Params& p);

    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustain (bool down);
    void allNotesOff();
    void allSoundOff();
    void setTempo (double bpm) { tempoBpm = bpm > 20.0 && bpm < 400.0 ? bpm : 120.0; }
    void setHostPosition (double ppq, bool valid) { hostPpq = ppq; hostPpqValid = valid; }   // Taktposition in Vierteln (nur bei laufendem Host gültig)
    int getSeqStep() const { return seqStepShown.load (std::memory_order_relaxed); }   // -1 = Rhythm-Seq aus
    bool hasActiveVoices() const { for (const auto& v : voices) if (v.active) return true; return false; }

    void setPitchBend (float v)  { bend = v; }       // -1..1
    void setModWheel (float v)   { modWheel = v; }   // 0..1
    void setAftertouch (float v) { aftertouch = v; } // 0..1

    // Sample für Layer V. Der Zeiger muss gültig bleiben, bis ein anderer gesetzt wird (nullptr = kein Sample).
    // Nur zwischen zwei process()-Aufrufen setzen.
    void setSample (const SampleData* s) { smp = s; }

    void process (float* left, float* right, int numSamples);

private:
    struct LayerVoice
    {
        float ph = 0.f, subPh = 0.f, tone = 0.f;
        float sph = 0.f, dStep = 0.f;   // Osc-Sync: Slave-Phase, Sprunghöhe beim letzten Reset
        Svf hp, lp;
        Svf fm[3];            // Formant-Resonatoren (nur Layer IV)
        double sp = 0.0;      // Abspielposition im Sample (nur Layer V)
        bool back = false, done = false;
        Env fe, ae;
    };
    struct Voice
    {
        bool active = false, gate = false, keyDown = false, sustained = false;
        int note = -1;
        float vel = 0.f, cur = 60.f, tgt = 60.f;
        std::uint64_t age = 0;
        LayerVoice lv[NUM_LAYERS];
    };

    Voice* allocate();
    void releaseVoice (Voice& v);
    void renderChunk (float* l, float* r, int n);
    void renderVoice (Voice& v, float* mono, int n);
    float noise();

    Params P;
    std::array<Voice, kVoices> voices;
    EnvCfg fcfg[NUM_LAYERS], acfg[NUM_LAYERS];

    float sr = 44100.f;
    double tempoBpm = 120.0;
    float bend = 0.f, bendS = 0.f, modWheel = 0.f, modS = 0.f, aftertouch = 0.f, atS = 0.f;
    float glideCoef = 1.f;

    // Rhythm-Seq (Step-Modulator): Werte werden pro Chunk berechnet und geglättet
    void updateSeq (int n);
    double hostPpq = 0.0, freeBeats = 0.0; bool hostPpqValid = false;
    float sqLevel = 1.f, sqLevelPrev = 1.f, sqCut = 0.f, sqSync = 0.f, sqPitch = 0.f, sqPwm = 0.f, sqVowel = 0.f, sqDly = 0.f;
    bool sqActive = false;
    std::atomic<int> seqStepShown { -1 };
    float lfoPhase = 0.f, lfoVal = 0.f, lfoSH = 0.f;
    int lastNote = -1;
    bool sustain = false;
    std::uint64_t counter = 0;
    std::uint32_t rng = 22222u;

    // geglättete Filterwerte (pro Layer)
    float hpS[NUM_LAYERS] { 20.f, 20.f, 20.f, 20.f, 20.f }, lpS[NUM_LAYERS] { 3000.f, 3000.f, 3000.f, 3000.f, 3000.f },
          resS[NUM_LAYERS] { 0.f, 0.f, 0.f, 0.f, 0.f };
    const SampleData* smp = nullptr;
    float formPos = 0.f;      // geglättete Vokal-Position 0..4 (A E I O U)

    Distortion dist;
    Phaser phaser;
    Flanger flanger;
    Chorus chorus;
    Delay delay;
    Reverb reverb;
};
} // namespace cs
