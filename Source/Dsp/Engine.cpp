#include "Engine.h"

namespace cs
{
namespace
{
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
} // namespace

void Engine::prepare (double sampleRate)
{
    sr = (float) sampleRate;
    chorus.prepare (sr);
    delay.prepare (sr);
    reverb.prepare (sr);
    allSoundOff();
    lastNote = -1;
}

void Engine::setParams (const Params& p)
{
    P = p;
    for (int L = 0; L < 2; ++L)
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

    for (int L = 0; L < 2; ++L)
    {
        auto& lv = v->lv[L];
        if (! wasActive)
        {
            lv.hp = {}; lv.lp = {}; lv.tone = 0.f;
            lv.ph = (noise() * 0.5f + 0.5f);
            lv.subPh = 0.f;
            lv.fe = {}; lv.ae = {};
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

void Engine::renderChunk (float* oL, float* oR, int n)
{
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

    for (int L = 0; L < 2; ++L) // Filter-Grundwerte glätten (einmal pro Chunk, nicht pro Stimme)
    {
        const int o = L * LP_COUNT;
        hpS[L] += (P[o + LP_HPF] - hpS[L]) * 0.2f;
        lpS[L] += (P[o + LP_LPF] - lpS[L]) * 0.2f;
        resS[L] += (P[o + LP_RES] - resS[L]) * 0.2f;
    }

    float mono[kChunk] = {};
    for (auto& v : voices)
        if (v.active) renderVoice (v, mono, n);

    // --- Effekte und Master ---
    const float chRate = P[GP_CH_RATE], chDepth = P[GP_CH_DEPTH], chMix = P[GP_CH_MIX];
    const float dlTime = P[GP_DL_TIME], dlFb = P[GP_DL_FB], dlMix = P[GP_DL_MIX];
    const bool ping = P[GP_DL_PING] > 0.5f;
    const float rvSize = P[GP_RV_SIZE], rvDamp = P[GP_RV_DAMP], rvMix = P[GP_RV_MIX];
    const float m = P[GP_MASTER];
    const float gain = m * m * 1.6f;

    for (int k = 0; k < n; ++k)
    {
        float l = mono[k], r = mono[k];
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

void Engine::renderVoice (Voice& v, float* mono, int n)
{
    // Portamento
    v.cur = glideCoef >= 1.f ? v.tgt : v.cur + (v.tgt - v.cur) * glideCoef;

    const float vibSemis = lfoVal * (2.f * P[GP_LFO_PITCH] + 1.f * modS + 1.f * P[GP_AT_VIB] * atS);
    const float bendSemis = bendS * P[GP_BEND];
    const float velGain = (1.f - P[GP_VEL_AMP]) + P[GP_VEL_AMP] * v.vel;
    const float tc = 1.f - std::exp (-2.f * kPi * 1500.f / sr);

    bool any = false;

    for (int L = 0; L < 2; ++L)
    {
        LayerVoice& lv = v.lv[L];
        if (! lv.ae.active()) continue;
        any = true;

        const int o = L * LP_COUNT;
        const int wave = (int) std::lround (P[o + LP_WAVE]);
        const int oct = (int) std::lround (P[o + LP_OCT]) - 2;
        const float level = P[o + LP_LEVEL] * 0.22f * velGain;
        const float sub = P[o + LP_SUB] * 0.6f, noiseAmt = P[o + LP_NOISE] * 0.6f;

        // Tonhöhe
        const float semis = v.cur + (float) oct * 12.f + P[o + LP_DETUNE] * 0.01f + bendSemis + vibSemis;
        const float freq = 440.f * std::exp2 ((semis - 69.f) / 12.f);
        const float dt = std::min (freq / sr, 0.45f);
        const float sdt = dt * 0.5f;

        // Pulsbreite
        float pw = 0.5f;
        if (wave == 2)
            pw = std::clamp (P[o + LP_PW] + P[GP_LFO_PWM] * 0.4f * lfoVal, 0.05f, 0.95f);

        // Filter (Kontrollrate)
        const float fv = lv.fe.level;
        const float keyOct = P[o + LP_KEY] * ((float) v.note - 60.f) / 12.f;
        const float modOct = P[o + LP_AMT] * 7.f * fv + keyOct
                           + P[GP_LFO_CUT] * lfoVal * 2.f
                           + P[GP_AT_FILTER] * atS * 3.f
                           + P[GP_VEL_FLT] * (v.vel - 0.7f) * 3.f;

        const float lpFc = std::clamp (lpS[L] * std::exp2 (modOct), 20.f, sr * 0.45f);
        const float q = 1.f - resS[L];
        const SvfCoef hpC = makeSvf (hpS[L], 1.41f, sr);
        const SvfCoef lpC = makeSvf (lpFc, 0.06f + 1.35f * q * q, sr);
        const float brill = std::exp2 ((P[o + LP_BRILL] - 0.5f) * 3.f);

        for (int s = 0; s < n; ++s)
        {
            // Oszillator (PolyBLEP gegen Aliasing)
            lv.ph += dt;
            if (lv.ph >= 1.f) lv.ph -= 1.f;
            const float t = lv.ph;

            float osc;
            if (wave == 0)
                osc = 2.f * t - 1.f - polyBlep (t, dt);
            else
            {
                osc = (t < pw ? 1.f : -1.f) + polyBlep (t, dt);
                float t2 = t - pw; if (t2 < 0.f) t2 += 1.f;
                osc -= polyBlep (t2, dt);
            }

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

            // Hochpass -> Tiefpass
            float lpOut, hpOut, dummy;
            svfTick (lv.hp, hpC, x, dummy, hpOut);
            svfTick (lv.lp, lpC, hpOut, lpOut, dummy);
            float y = fastTanh (lpOut);

            // Brilliance: Höhenanteil über ca. 1.5 kHz anheben/absenken
            lv.tone += tc * (y - lv.tone);
            y = lv.tone + (y - lv.tone) * brill;

            lv.fe.tick (fcfg[L]);
            const float a = lv.ae.tick (acfg[L]);
            mono[s] += y * a * level;
        }
    }

    if (! any) v.active = false;
}
} // namespace cs
