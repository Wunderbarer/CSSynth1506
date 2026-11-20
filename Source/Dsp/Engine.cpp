#include "Engine.h"

namespace cs
{
namespace
{
// Integral der polyBLEP-Korrektur: begradigt Knicke (Steigungssprünge) des Dreiecks. t = Phase seit dem Knick (0..1).
inline float polyBlamp (float t, float dt)
{
    if (t < dt)        { const float x = t / dt - 1.f;        return -(1.f / 3.f) * x * x * x * dt; }
    if (t > 1.f - dt)  { const float x = (t - 1.f) / dt + 1.f; return  (1.f / 3.f) * x * x * x * dt; }
    return 0.f;
}

inline float polyBlep (float t, float dt)
{
    if (t < dt)         { t /= dt;               return t + t - t * t - 1.f; }
    if (t > 1.f - dt)   { t = (t - 1.f) / dt;    return t * t + t + t + 1.f; }
    return 0.f;
}

inline float envCoef (float seconds, float sr)
{
    return 1.f - std::exp (-2.4f / (std::max (seconds, 0.0005f) * sr));
}

inline SvfCoef makeSvf (float fc, float k, float sr)
{
    const float g = std::tan (kPi * std::clamp (fc, 10.f, sr * 0.45f) / sr);
    SvfCoef c;
    c.k = k;
    c.a1 = 1.f / (1.f + g * (g + k));
    c.a2 = g * c.a1;
    c.a3 = g * c.a2;
    return c;
}

inline void svfTick (Svf& s, const SvfCoef& c, float x, float& lp, float& hp)
{
    const float v3 = x - s.ic2;
    const float v1 = c.a1 * s.ic1 + c.a2 * v3;
    const float v2 = s.ic2 + c.a2 * s.ic1 + c.a3 * v3;
    s.ic1 = 2.f * v1 - s.ic1;
    s.ic2 = 2.f * v2 - s.ic2;
    lp = v2;
    hp = x - c.k * v1 - v2;
}
// ---- Formant-Oszillator (Layer IV) ----
constexpr int kVowels = 5;   // A E I O U

// Formantfrequenzen F1..F3 (Hz) und relative Pegel; typische Mittelwerte einer männlichen Stimme
const float kFormF[kVowels][3]  = { { 800.f, 1150.f, 2900.f },   // A
                                    { 400.f, 1700.f, 2600.f },   // E
                                    { 270.f, 2300.f, 3000.f },   // I
                                    { 450.f,  800.f, 2830.f },   // O
                                    { 325.f,  700.f, 2530.f } }; // U
const float kFormA[kVowels][3]  = { { 1.f, 0.50f, 0.22f },
                                    { 1.f, 0.45f, 0.25f },
                                    { 1.f, 0.30f, 0.20f },
                                    { 1.f, 0.40f, 0.12f },
                                    { 1.f, 0.25f, 0.10f } };
const float kFormBw[3] = { 90.f, 110.f, 170.f };   // Bandbreiten in Hz
constexpr float kFormantGain = 6.8f;               // gleicht den Pegel an einen Sägezahn-Layer an

// Bandpass-Ausgang des gleichen TPT-Filters (Spitzenverstärkung 1/k, wird über die Amplitude normiert)
inline float svfBand (Svf& s, const SvfCoef& c, float x)
{
    const float v3 = x - s.ic2;
    const float v1 = c.a1 * s.ic1 + c.a2 * v3;
    const float v2 = s.ic2 + c.a2 * s.ic1 + c.a3 * v3;
    s.ic1 = 2.f * v1 - s.ic1;
    s.ic2 = 2.f * v2 - s.ic2;
    return v1;
}

// ---- Sample-Oszillator (Layer V) ----
constexpr double kLoopXfade = 0.0015;   // Loop-Überblendung in Sekunden
// 4-Punkt-Hermite-Interpolation an der (gebrochenen) Position pos
inline float sampleAt (const std::vector<float>& d, double pos)
{
    const int n = (int) d.size();
    const int i = (int) pos;
    const float f = (float) (pos - (double) i);
    auto at = [&] (int k) { return d[(size_t) std::clamp (k, 0, n - 1)]; };
    const float xm1 = at (i - 1), x0 = at (i), x1 = at (i + 1), x2 = at (i + 2);
    const float c = (x1 - xm1) * 0.5f;
    const float v = x0 - x1;
    const float w = c + v;
    const float a = w + v + (x2 - x0) * 0.5f;
    const float bNeg = w + a;
    return ((a * f - bNeg) * f + c) * f + x0;
}
} // namespace

void Engine::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    dist.prepare (sr);
    phaser.prepare (sr);
    flanger.prepare (sr);
    chorus.prepare (sr);
    delay.prepare (sr);
    reverb.prepare (sr);
    allSoundOff();
    lastNote = -1;
}

void Engine::setParams (const Params& p)
{
    P = p;
    for (int L = 0; L < NUM_LAYERS; ++L)
    {
        const int o = L * LP_COUNT;
        EnvCfg f;
        f.il = p[o + LP_FIL]; f.al = p[o + LP_FAL]; f.sl = p[o + LP_FSL]; f.rel = f.il;
        f.ca = envCoef (p[o + LP_FA], sr); f.cd = envCoef (p[o + LP_FD], sr); f.cr = envCoef (p[o + LP_FR], sr);
        fcfg[L] = f;

        EnvCfg a;
        a.il = 0.f; a.al = 1.f; a.sl = p[o + LP_AS]; a.rel = 0.f;
        a.ca = envCoef (p[o + LP_AA], sr); a.cd = envCoef (p[o + LP_AD], sr); a.cr = envCoef (p[o + LP_AR], sr);
        acfg[L] = a;
    }
    const float g = p[GP_GLIDE];
    glideCoef = g < 0.003f ? 1.f : 1.f - std::exp (-(float) kChunk * 3.f / (g * sr));
}

float Engine::noise()
{
    rng = rng * 1664525u + 1013904223u;
    return (float) (std::int32_t) rng * (1.f / 2147483648.f);
}

Engine::Voice* Engine::allocate()
{
    for (auto& v : voices)
        if (! v.active) return &v;

    Voice* best = nullptr;
    for (auto& v : voices) // bevorzugt Stimmen in der Ausklingphase, sonst die älteste
        if (! v.gate && (best == nullptr || v.age < best->age)) best = &v;
    if (best != nullptr) return best;

    best = &voices[0];
    for (auto& v : voices)
        if (v.age < best->age) best = &v;
    return best;
}

void Engine::noteOn (int note, float velocity)
{
    if (! hasActiveVoices()) freeBeats = 0.0;   // Rhythm-Seq im Modus "Free": Muster startet mit der ersten Note neu
    Voice* v = allocate();
    const bool wasActive = v->active;

    v->note = note;
    v->vel = std::clamp (velocity, 0.f, 1.f);
    v->gate = v->keyDown = true;
    v->sustained = false;
    v->age = ++counter;
    v->tgt = (float) note;
    v->cur = (P[GP_GLIDE] >= 0.003f && lastNote >= 0) ? (float) lastNote : (float) note;
    lastNote = note;

    for (int L = 0; L < NUM_LAYERS; ++L)
    {
        auto& lv = v->lv[L];
        if (! wasActive)
        {
            lv.hp = {}; lv.lp = {}; lv.tone = 0.f;
            for (auto& f : lv.fm) f = {};
            lv.ph = (noise() * 0.5f + 0.5f);
            lv.subPh = 0.f; lv.sph = 0.f; lv.dStep = 0.f;
            lv.fe = {}; lv.ae = {};
            lv.dr1 = noise() * 0.02f; lv.dr2 = noise() * 0.02f;
        }
        if (L == SAMPLE_LAYER)   // Sample startet bei jedem Anschlag neu an der Startposition
        {
            const double len = smp != nullptr ? (double) smp->data.size() : 0.0;
            lv.sp = std::clamp ((double) P[L * LP_COUNT + LP_PW], 0.0, 1.0) * std::max (len - 2.0, 0.0);
            lv.back = false;
            lv.done = false;
        }
        if (P[L * LP_COUNT + LP_LEVEL] > 0.f)
        {
            lv.fe.gateOn (fcfg[L]);
            lv.ae.gateOn (acfg[L]);
        }
    }
    v->active = true;
}

void Engine::releaseVoice (Voice& v)
{
    v.gate = false;
    v.sustained = false;
    for (auto& lv : v.lv) { lv.fe.gateOff(); lv.ae.gateOff(); }
}

void Engine::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.keyDown && v.note == note)
        {
            v.keyDown = false;
            if (sustain) v.sustained = true;
            else         releaseVoice (v);
        }
}

void Engine::setSustain (bool down)
{
    sustain = down;
    if (! down)
        for (auto& v : voices)
            if (v.active && v.gate && ! v.keyDown) releaseVoice (v);
}

void Engine::allNotesOff()
{
    for (auto& v : voices)
        if (v.active) { v.keyDown = false; releaseVoice (v); }
}

void Engine::allSoundOff()
{
    for (auto& v : voices) v = Voice {};
}

void Engine::process (float* left, float* right, int numSamples)
{
    int i = 0;
    while (i < numSamples)
    {
        const int n = std::min (kChunk, numSamples - i);
        renderChunk (left + i, right + i, n);
        i += n;
    }
}

void Engine::updateSeq (int n)
{
    const float aCut = P[GP_SQ_CUT], aLvl = P[GP_SQ_LEVEL], aSync = P[GP_SQ_SYNC], aPit = P[GP_SQ_PITCH],
                aPwm = P[GP_SQ_PWM], aVow = P[GP_SQ_VOWEL], aDly = P[GP_SQ_DLY];
    sqLevelPrev = sqLevel;
    sqActive = aCut > 0.001f || aLvl > 0.001f || aSync > 0.001f || aPit > 0.001f || aPwm > 0.001f || aVow > 0.001f || aDly > 0.001f;
    const double beatsPerChunk = (double) n * tempoBpm / (60.0 * (double) sr);
    const bool hostMode = P[GP_SQ_MODE] > 0.5f && hostPpqValid;
    double pos;
    if (hostMode) { pos = std::max (hostPpq, 0.0); hostPpq += beatsPerChunk; }
    else          { pos = freeBeats; freeBeats += beatsPerChunk; }
    if (! sqActive)
    {
        sqLevel = 1.f; sqCut = sqSync = sqPitch = sqPwm = sqVowel = sqDly = 0.f;
        seqStepShown.store (-1, std::memory_order_relaxed);
        return;
    }

    static const double kStepBeats[] = { 0.125, 1.0 / 6.0, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 2.0 };
    const double sb = kStepBeats[std::clamp ((int) std::lround (P[GP_SQ_RATE]), 0, 9)];
    const double t = pos / sb;
    const double pair = std::floor (t / 2.0);
    const double u = t - 2.0 * pair;
    const long long idx = (long long) pair * 2 + (u >= 1.0 + (double) P[GP_SQ_SWING] ? 1 : 0);
    const int len = std::clamp ((int) std::lround (P[GP_SQ_LEN]), 1, 16);
    const int step = (int) (idx % len);
    seqStepShown.store (hasActiveVoices() ? step : -1, std::memory_order_relaxed);

    const float v = P[GP_SQ_S1 + step];
    const float v01 = 0.5f * (v + 1.f);
    const float tLevel = 1.f - aLvl * (1.f - v01);
    const float tCut   = aCut * v * 3.f;
    const float tSync  = aSync * v01;
    const float tPitch = std::round (aPit * v * 12.f);
    const float tPwm   = aPwm * v * 0.4f;
    const float tVowel = aVow * v * 2.f;
    const float tDly   = aDly * v01 * 0.5f;

    // Slew: Anteil der Schrittdauer (mindestens 1,5 ms gegen Knackser)
    const double stepSec = sb * 60.0 / tempoBpm;
    const double tau = 0.0015 + (double) P[GP_SQ_SLEW] * (double) P[GP_SQ_SLEW] * 0.6 * stepSec;
    const float c = 1.f - (float) std::exp (-(double) n / ((double) sr * tau));
    sqLevel += (tLevel - sqLevel) * c;
    sqCut   += (tCut   - sqCut)   * c;
    sqSync  += (tSync  - sqSync)  * c;
    sqPitch += (tPitch - sqPitch) * c;
    sqPwm   += (tPwm   - sqPwm)   * c;
    sqVowel += (tVowel - sqVowel) * c;
    sqDly   += (tDly   - sqDly)   * c;
}

void Engine::renderChunk (float* oL, float* oR, int n)
{
    updateSeq (n);
    // --- LFO und geglättete Spielhilfen (Kontrollrate) ---
    const float rate = P[GP_LFO_RATE];
    lfoPhase += rate * (float) n / sr;
    if (lfoPhase >= 1.f)
    {
        lfoPhase -= 1.f;
        lfoSH = noise();
    }
    switch ((int) std::lround (P[GP_LFO_SHAPE]))
    {
        case 0:  lfoVal = std::sin (2.f * kPi * lfoPhase); break;
        case 1:  lfoVal = 4.f * std::fabs (lfoPhase - 0.5f) - 1.f; break;
        case 2:  lfoVal = 2.f * lfoPhase - 1.f; break;
        default: lfoVal = lfoSH; break;
    }
    bendS += (bend - bendS) * 0.3f;
    modS  += (modWheel - modS) * 0.15f;
    atS   += (aftertouch - atS) * 0.1f;

    for (int L = 0; L < NUM_LAYERS; ++L) // Filter-Grundwerte glätten (einmal pro Chunk, nicht pro Stimme)
    {
        const int o = L * LP_COUNT;
        hpS[L] += (P[o + LP_HPF] - hpS[L]) * 0.2f;
        lpS[L] += (P[o + LP_LPF] - lpS[L]) * 0.2f;
        resS[L] += (P[o + LP_RES] - resS[L]) * 0.2f;
    }

    {   // Vokal-Position des Formant-Layers: feste Vokale 0..4, "Morph" über den Regler; LFO moduliert
        const int o = FORMANT_LAYER * LP_COUNT;
        const int w = (int) std::lround (P[o + LP_WAVE]);
        const float base = w < kVowels ? (float) w : std::clamp (P[o + LP_PW], 0.f, 1.f) * (float) (kVowels - 1);
        const float target = std::clamp (base + lfoVal * P[GP_LFO_VOWEL] * 2.f + sqVowel, 0.f, (float) (kVowels - 1));
        formPos += (target - formPos) * 0.35f;
    }

    float monoL[kChunk] = {}, monoR[kChunk] = {};
    for (auto& v : voices)
        if (v.active) renderVoice (v, monoL, monoR, n);

    // --- Effekte und Master ---
    const int dsType = (int) std::lround (P[GP_DS_TYPE]);
    const float dsDrive = P[GP_DS_DRIVE], dsTone = P[GP_DS_TONE], dsMix = P[GP_DS_MIX];
    const float phRate = P[GP_PH_RATE], phDepth = P[GP_PH_DEPTH], phFb = P[GP_PH_FB], phMix = P[GP_PH_MIX];
    const float flRate = P[GP_FL_RATE], flDepth = P[GP_FL_DEPTH], flFb = P[GP_FL_FB], flMix = P[GP_FL_MIX];
    const float chRate = P[GP_CH_RATE], chDepth = P[GP_CH_DEPTH], chMix = P[GP_CH_MIX];
    float dlTime = P[GP_DL_TIME];
    if (P[GP_DL_SYNC] > 0.5f)
    {
        static const double kDivBeats[] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0, 4.0 };
        const int di = std::clamp ((int) std::lround (P[GP_DL_DIV]), 0, 8);
        double ms = kDivBeats[di] * 60000.0 / tempoBpm;
        while (ms > 1400.0) ms *= 0.5;           // Puffer fasst 1,5 s: bei langsamem Tempo eine Teilung kürzer
        dlTime = (float) std::max (ms, 1.0);
    }
    const float dlFb = P[GP_DL_FB], dlMix = std::clamp (P[GP_DL_MIX] + sqDly, 0.f, 1.f);
    const bool ping = P[GP_DL_PING] > 0.5f;
    const float rvSize = P[GP_RV_SIZE], rvDamp = P[GP_RV_DAMP], rvMix = P[GP_RV_MIX];
    const float m = P[GP_MASTER];
    const float gain = m * m * 1.6f;

    for (int k = 0; k < n; ++k)
    {
        float l = monoL[k], r = monoR[k];
        if (dsMix > 0.001f) dist.process (l, r, dsType, dsDrive, dsTone, dsMix);
        if (phMix > 0.001f) phaser.process (l, r, phRate, phDepth, phFb, phMix);
        if (flMix > 0.001f) flanger.process (l, r, flRate, flDepth, flFb, flMix);
        if (chMix > 0.001f) chorus.process (l, r, chRate, chDepth, chMix);
        if (dlMix > 0.001f) delay.process (l, r, dlTime, dlFb, dlMix, ping);
        if (rvMix > 0.001f) reverb.process (l, r, rvSize, rvDamp, rvMix);
        l *= gain; r *= gain;

        // weicher Limiter: unter 0.7 linear, darüber sanfte Sättigung
        auto soft = [] (float x)
        {
            const float a = std::fabs (x);
            if (a <= 0.7f) return x;
            const float y = 0.7f + 0.3f * fastTanh ((a - 0.7f) / 0.3f);
            return x < 0.f ? -y : y;
        };
        oL[k] = soft (l);
        oR[k] = soft (r);
    }
}

void Engine::renderVoice (Voice& v, float* monoL, float* monoR, int n)
{
    // Feste Stereoverteilung (ohne Regler): Layer I bleibt in der Mitte, die übrigen Layer sitzen leicht links/rechts,
    // dazu eine kleine tonhöhenabhängige Streuung. Tiefe Töne (unter C2) bleiben mono, nach oben öffnet sich das Bild.
    static const float kLayerPan[NUM_LAYERS] = { 0.f, 0.30f, -0.30f, 0.18f, -0.18f };
    const float width = std::clamp (((float) v.note - 36.f) / 24.f, 0.f, 1.f);
    const float noteSpread = ((float) ((v.note * 7) % 12) - 5.5f) / 5.5f * 0.12f;
    // Portamento
    v.cur = glideCoef >= 1.f ? v.tgt : v.cur + (v.tgt - v.cur) * glideCoef;

    const float vibSemis = lfoVal * (2.f * P[GP_LFO_PITCH] + 1.f * modS + 1.f * P[GP_AT_VIB] * atS);
    const float bendSemis = bendS * P[GP_BEND];
    const float velGain = (1.f - P[GP_VEL_AMP]) + P[GP_VEL_AMP] * v.vel;
    const float tc = 1.f - std::exp (-2.f * kPi * 1500.f / sr);

    bool any = false;

    for (int L = 0; L < NUM_LAYERS; ++L)
    {
        LayerVoice& lv = v.lv[L];
        if (! lv.ae.active()) continue;
        any = true;

        const bool formant = (L == FORMANT_LAYER);
        const bool sampler = (L == SAMPLE_LAYER);
        const int o = L * LP_COUNT;
        const float pan = std::clamp ((kLayerPan[L] + noteSpread) * width, -1.f, 1.f);
        const float gL = std::cos (std::max (pan, 0.f) * 0.5f * kPi), gR = std::cos (std::max (-pan, 0.f) * 0.5f * kPi);
        // analoge Drift: zwei langsame Zufallswerte pro Layer (ca. +-2 Cent bzw. +-3 % Filterfrequenz)
        lv.dr1 += 0.0005f * (noise() - lv.dr1);
        lv.dr2 += 0.0005f * (noise() - lv.dr2);
        const bool syncedLoop = sampler && P[GP_SMP_SYNC] > 0.5f;
        const float driftSemis = syncedLoop ? 0.f : lv.dr1 * 2.5f * driftAmt;
        const int wave = (int) std::lround (P[o + LP_WAVE]);
        const int oct = (int) std::lround (P[o + LP_OCT]) - 2;
        const float level = P[o + LP_LEVEL] * 0.22f * velGain;
        const float sub = formant ? 0.f : P[o + LP_SUB] * 0.6f;   // beim Formant-Layer ist LP_SUB die Formant-Verschiebung
        const float noiseAmt = P[o + LP_NOISE] * 0.6f;

        // Tonhöhe
        const float semis = v.cur + (float) oct * 12.f + P[o + LP_DETUNE] * 0.01f + bendSemis + vibSemis + sqPitch + driftSemis;
        const float freq = 440.f * std::exp2 ((semis - 69.f) / 12.f);
        const float dt = std::min (freq / sr, 0.45f);
        const float sdt = dt * 0.5f;

        // Sample-Wiedergabe: Schrittweite pro Ausgabe-Sample, Loop-Grenzen
        const bool haveSample = sampler && smp != nullptr && ! smp->empty();
        double sStep = 0.0, sLs = 0.0, sLe = 0.0, sLen = 0.0;
        if (haveSample)
        {
            sLen = (double) smp->data.size();
            const float rootFreq = 440.f * std::exp2 ((P[GP_SMP_ROOT] - 69.f) / 12.f);
            sStep = (double) (freq / rootFreq) * smp->rate / (double) sr;
            sLs = std::clamp ((double) P[GP_SMP_LSTART], 0.0, 1.0) * (sLen - 1.0);
            sLe = std::clamp ((double) P[GP_SMP_LEND], 0.0, 1.0) * (sLen - 1.0);
            if (sLe < sLs + 8.0) sLe = std::min (sLs + 8.0, sLen - 1.0);   // Mindestlänge des Loops
            if (P[GP_SMP_SYNC] > 0.5f)
            {
                // Loop-Länge = gewählte Beat-Zahl im Host-Tempo (Grundton spielt exakt synchron, andere Töne transponieren)
                static const double kBeats[] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };
                const double beats = kBeats[std::clamp ((int) std::lround (P[GP_SMP_BEATS]), 0, 6)];
                const double span0 = std::max (sLe - sLs, 1.0);
                sStep = (span0 / (beats * 60.0 / tempoBpm * (double) sr)) * (double) (freq / rootFreq);
            }
        }

        // Sinus und Dreieck: sanftere Sättigung, damit sie rein klingen (Saw/Square/Pulse behalten den kräftigen Charakter)
        const float satK = (! formant && ! sampler && (wave == 3 || wave == 4)) ? 0.25f : 1.f;

        // Pulsbreite
        float pw = 0.5f;
        if (! formant && ! sampler && wave == 2)
            pw = std::clamp (P[o + LP_PW] + P[GP_LFO_PWM] * 0.4f * lfoVal + sqPwm, 0.05f, 0.95f);

        // Filter (Kontrollrate)
        const float fv = lv.fe.level;
        const float keyOct = P[o + LP_KEY] * ((float) v.note - 60.f) / 12.f;
        const float modOct = P[o + LP_AMT] * 7.f * fv + keyOct
                           + P[GP_LFO_CUT] * lfoVal * 2.f
                           + P[GP_AT_FILTER] * atS * 3.f
                           + P[GP_VEL_FLT] * (v.vel - 0.7f) * 3.f
                           + sqCut + lv.dr2 * 4.f * driftAmt;

        const float lpFc = std::clamp (lpS[L] * std::exp2 (modOct), 20.f, sr * 0.45f);
        const float q = 1.f - resS[L];
        const SvfCoef hpC = makeSvf (hpS[L], 1.41f, sr);
        const SvfCoef lpC = makeSvf (lpFc, 0.06f + 1.35f * q * q, sr);
        const float brill = std::exp2 ((P[o + LP_BRILL] - 0.5f) * 3.f);

        // Formant-Resonatoren (Kontrollrate): Frequenzen/Pegel aus der Vokaltabelle, stufenlos interpoliert
        SvfCoef formC[3];
        float formAmp[3] = { 0.f, 0.f, 0.f };
        if (formant)
        {
            const int i0 = std::min ((int) formPos, kVowels - 2);
            const float fr = formPos - (float) i0;
            const float shift = std::exp2 (P[o + LP_SUB] / 12.f);
            for (int k = 0; k < 3; ++k)
            {
                const float f = std::clamp ((kFormF[i0][k] * (1.f - fr) + kFormF[i0 + 1][k] * fr) * shift, 80.f, sr * 0.4f);
                formC[k] = makeSvf (f, std::clamp (kFormBw[k] * shift / f, 0.03f, 0.6f), sr);
                formAmp[k] = (kFormA[i0][k] * (1.f - fr) + kFormA[i0 + 1][k] * fr) * formC[k].k;   // * k: Bandpass auf Spitzenpegel 1
            }
        }

        // Hard Sync (Layer I-III): Der Master läuft mit der Notenfrequenz, der Slave mit dem Vielfachen davon
        // und wird bei jedem Master-Durchlauf auf Phase 0 zurückgesetzt.
        const bool syncOn = ! formant && ! sampler && (P[o + LP_SYNC] + sqSync > 0.001f || P[o + LP_SYNCENV] > 0.001f);
        const float syncRatio = syncOn ? 1.f + (std::min (P[o + LP_SYNC] + sqSync, 1.f) + P[o + LP_SYNCENV] * fv) * 7.f : 1.f;
        const float dts = std::min (dt * syncRatio, 0.45f);
        const float syncRatioEff = dt > 1.0e-9f ? dts / dt : 1.f;
        const int wv = wave;
        auto naive = [wv, pw] (float p) -> float
        {
            if (wv == 0) return 2.f * p - 1.f;
            if (wv == 3) return std::sin (2.f * kPi * p);
            if (wv == 4) return 4.f * std::fabs (p - 0.5f) - 1.f;
            return p < pw ? 1.f : -1.f;
        };

        for (int s = 0; s < n; ++s)
        {
            // Oszillator (PolyBLEP gegen Aliasing)
            lv.ph += dt;
            bool wrapped = false;
            if (lv.ph >= 1.f) { lv.ph -= 1.f; wrapped = true; }
            const float tm = lv.ph;
            float t = tm, dtw = dt, syncCorr = 0.f;
            if (syncOn)
            {
                lv.sph += dts;
                if (wrapped)
                {
                    const float f = tm / std::max (dt, 1.0e-9f);                       // Anteil des Samples nach dem Master-Durchlauf
                    float pre = lv.sph - dts * f; pre -= std::floor (pre);
                    lv.dStep = naive (0.f) - naive (pre);
                    lv.sph = dts * f;
                    // Der Slave-Oszillator korrigiert seinen eigenen Phasen-0-Sprung ohnehin (Saw: -2, Square/Pulse: +2) -
                    // hier kommt nur die Differenz zum tatsächlichen Reset-Sprung dazu.
                    const float natural = wv == 0 ? -2.f : ((wv == 1 || wv == 2) ? 2.f : 0.f);
                    syncCorr = 0.5f * (lv.dStep - natural) * polyBlep (tm, dt);
                }
                else if (tm > 1.f - dt)                                                // Reset steht bevor: Vorwärts-Korrektur
                {
                    float pw2 = lv.sph + syncRatioEff * (1.f - tm); pw2 -= std::floor (pw2);
                    syncCorr = 0.5f * (naive (0.f) - naive (pw2)) * polyBlep (tm, dt);
                }
                lv.sph -= std::floor (lv.sph);
                t = lv.sph; dtw = dts;
            }

            float osc;
            if (sampler)
            {
                osc = 0.f;
                if (haveSample && ! lv.done)
                {
                    osc = sampleAt (smp->data, lv.sp);
                    if (wave == 0)          // Loop: kurze Überblendung am Loop-Ende (verhindert Knacken bei nicht perfekt geschnittenen Loops)
                    {
                        const double xf = std::min (kLoopXfade * (double) smp->rate, (sLe - sLs) * 0.25);
                        if (xf > 1.0 && lv.sp > sLe - xf)
                        {
                            const double wgt = (lv.sp - (sLe - xf)) / xf;
                            osc = (float) ((1.0 - wgt) * osc + wgt * sampleAt (smp->data, lv.sp - (sLe - sLs)));
                        }
                    }
                    else if (wave == 1)     // One-Shot: kurzes Ausblenden der letzten Samples, damit das Ende nicht knackt
                        osc *= (float) std::min (1.0, (sLen - 1.0 - lv.sp) / 48.0);
                    if (wave == 1)          // One-Shot: einmal bis zum Ende
                    {
                        lv.sp += sStep;
                        if (lv.sp >= sLen - 1.0) lv.done = true;
                    }
                    else if (wave == 2)     // Ping-Pong zwischen Loop-Start und -Ende
                    {
                        lv.sp += lv.back ? -sStep : sStep;
                        if (lv.sp >= sLe)      { lv.sp = 2.0 * sLe - lv.sp; lv.back = true; }
                        else if (lv.sp <= sLs) { lv.sp = 2.0 * sLs - lv.sp; lv.back = false; }
                        lv.sp = std::clamp (lv.sp, 0.0, sLen - 1.0);
                    }
                    else                    // Loop (Vorwärts)
                    {
                        lv.sp += sStep;
                        if (lv.sp >= sLe)
                        {
                            const double span = sLe - sLs;
                            lv.sp = sLs + std::fmod (lv.sp - sLs, span);
                        }
                    }
                }
            }
            else if (formant || wave == 0)  // Formant-Layer: immer Sägezahn als Quelle
                osc = 2.f * t - 1.f - polyBlep (t, dtw);
            else if (wave == 3)             // Sinus
                osc = std::sin (2.f * kPi * t);
            else if (wave == 4)             // Dreieck (Spitzen bei Phase 0 und Tal bei 0.5), bandbegrenzt per polyBLAMP
            {
                osc = 4.f * std::fabs (t - 0.5f) - 1.f;
                float t2 = t - 0.5f; if (t2 < 0.f) t2 += 1.f;
                osc += 4.f * (polyBlamp (t2, dtw) - polyBlamp (t, dtw));
            }
            else
            {
                osc = (t < pw ? 1.f : -1.f) + polyBlep (t, dtw);
                float t2 = t - pw; if (t2 < 0.f) t2 += 1.f;
                osc -= polyBlep (t2, dtw);
            }

            osc += syncCorr;

            float x = osc;
            if (sub > 0.f)
            {
                lv.subPh += sdt;
                if (lv.subPh >= 1.f) lv.subPh -= 1.f;
                float so = (lv.subPh < 0.5f ? 1.f : -1.f) + polyBlep (lv.subPh, sdt);
                float t3 = lv.subPh + 0.5f; if (t3 >= 1.f) t3 -= 1.f;
                so -= polyBlep (t3, sdt);
                x += sub * so;
            }
            if (noiseAmt > 0.f) x += noiseAmt * noise();

            if (formant)   // Quelle (+ Hauch) durch drei parallele Formant-Resonatoren
            {
                float f = 0.f;
                for (int k = 0; k < 3; ++k)
                    f += formAmp[k] * svfBand (lv.fm[k], formC[k], x);
                x = f * kFormantGain;
            }

            // Hochpass -> Tiefpass
            float lpOut, hpOut, dummy;
            svfTick (lv.hp, hpC, x, dummy, hpOut);
            svfTick (lv.lp, lpC, hpOut, lpOut, dummy);
            float y = fastTanh (lpOut * satK) / satK;

            // Brilliance: Höhenanteil über ca. 1.5 kHz anheben/absenken
            lv.tone += tc * (y - lv.tone);
            y = lv.tone + (y - lv.tone) * brill;

            lv.fe.tick (fcfg[L]);
            const float a = lv.ae.tick (acfg[L]);
            const float o2 = y * a * level * (sqLevelPrev + (sqLevel - sqLevelPrev) * ((float) (s + 1) / (float) n));
            monoL[s] += o2 * gL;
            monoR[s] += o2 * gR;
        }
    }

    if (! any) v.active = false;
}
} // namespace cs
