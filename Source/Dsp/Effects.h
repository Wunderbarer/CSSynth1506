// CSSynth 1506 - Digitale Effekte: Distortion, Phaser, Flanger, Chorus, Delay (Ping-Pong), Reverb (Freeverb-Topologie)
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace cs
{
constexpr float kPi = 3.14159265358979f;

inline float fastTanh (float x)
{
    x = std::clamp (x, -3.f, 3.f);
    return x * (27.f + x * x) / (27.f + 9.f * x * x);
}

// ---------------------------------------------------------------------------------------------
// Distortion: Soft (tanh) / Hard (Clip) / Fold (Wellenfalter), 2-fach überabgetastet, Klangblende (Tiefpass)
class Distortion
{
public:
    void prepare (float sampleRate) { sr = sampleRate; for (auto& c : ch) c = {}; }

    // type: 0 Soft, 1 Hard, 2 Fold | drive 0..1 | tone 0..1 | mix 0..1
    void process (float& l, float& r, int type, float drive, float tone, float mix)
    {
        const float pre  = 1.f + drive * drive * 39.f;
        const float comp = 1.f / std::pow (pre, 0.6f);                         // Pegelausgleich: mehr Drive = weniger Ausgangsverstärkung
        const float fc   = 800.f * std::pow (15.f, tone);                       // 800 Hz .. 12 kHz
        const float a    = 1.f - std::exp (-2.f * kPi * std::min (fc, sr * 0.45f) / sr);

        auto run = [&] (Chan& c, float x)
        {
            const float mid = 0.5f * (c.prev + x);                              // Zwischenwert für 2x-Rate
            c.prev = x;
            const float s0 = shape (mid * pre, type);
            const float s1 = shape (x * pre, type);
            const float y = (0.25f * c.s1 + 0.5f * s0 + 0.25f * s1) * comp;    // einfacher Tiefpass vor dem Dezimieren
            c.s1 = s1;
            c.lp += a * (y - c.lp);
            return c.lp * 0.9f;
        };

        const float wl = run (ch[0], l), wr = run (ch[1], r);
        l = l * (1.f - mix) + wl * mix;
        r = r * (1.f - mix) + wr * mix;
    }

private:
    struct Chan { float prev = 0.f, s1 = 0.f, lp = 0.f; };
    static float shape (float x, int type)
    {
        switch (type)
        {
            case 1:  return std::clamp (x, -1.f, 1.f);
            case 2:  return std::sin (0.5f * kPi * std::clamp (x, -8.f, 8.f));
            default: return fastTanh (x);
        }
    }
    Chan ch[2];
    float sr = 44100.f;
};

// ---------------------------------------------------------------------------------------------
// Phaser: 6 Allpass-Stufen, Rückkopplung, Stereo-Versatz des LFO
class Phaser
{
public:
    void prepare (float sampleRate) { sr = sampleRate; for (auto& c : ch) c = {}; phase = 0.f; }

    void process (float& l, float& r, float rate, float depth, float fb, float mix)
    {
        phase += rate / sr;
        if (phase >= 1.f) phase -= 1.f;
        const float fbk = std::clamp (fb, 0.f, 0.92f);

        auto run = [&] (Chan& c, float x, float ph)
        {
            const float lfo = 0.5f + 0.5f * std::sin (2.f * kPi * ph);
            const float f = 300.f * std::pow (2.f, lfo * (1.f + depth * 3.5f)); // 300 Hz .. ca. 6.8 kHz
            const float t = std::tan (kPi * std::min (f, sr * 0.45f) / sr);
            const float k = (t - 1.f) / (t + 1.f);
            float v = x + fbk * c.last;
            for (int i = 0; i < kStages; ++i)
            {
                const float y = k * v + c.z[i];
                c.z[i] = v - k * y;
                v = y;
            }
            c.last = fastTanh (v);
            return v;
        };

        const float wl = run (ch[0], l, phase), wr = run (ch[1], r, phase + 0.25f);
        l = l * (1.f - 0.5f * mix) + wl * 0.5f * mix;
        r = r * (1.f - 0.5f * mix) + wr * 0.5f * mix;
    }

private:
    static constexpr int kStages = 6;
    struct Chan { float z[kStages] = {}; float last = 0.f; };
    Chan ch[2];
    float sr = 44100.f, phase = 0.f;
};

// ---------------------------------------------------------------------------------------------
// Flanger: kurze, modulierte Verzögerung (0,3 - 7 ms) mit positiver oder negativer Rückkopplung
class Flanger
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        const size_t n = (size_t) (sr * 0.02f) + 8;
        bl.assign (n, 0.f); br.assign (n, 0.f);
        w = 0; phase = 0.f;
    }

    void process (float& l, float& r, float rate, float depth, float fb, float mix)
    {
        const int n = (int) bl.size();
        phase += rate / sr;
        if (phase >= 1.f) phase -= 1.f;
        const float fbk = std::clamp (fb, -0.92f, 0.92f);

        auto run = [&] (std::vector<float>& b, float x, float ph)
        {
            const float lfo = 0.5f + 0.5f * std::sin (2.f * kPi * ph);
            const float ms = 0.3f + lfo * (0.4f + depth * 6.3f);               // 0,3 .. 7 ms
            const float d = std::min (ms * 0.001f * sr, (float) n - 4.f);
            float rp = (float) w - d;
            while (rp < 0.f) rp += (float) n;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            const float wet = b[(size_t) i0 % (size_t) n] * (1.f - fr) + b[(size_t) (i0 + 1) % (size_t) n] * fr;
            b[(size_t) w] = x + fastTanh (wet * fbk);
            return wet;
        };

        const float wl = run (bl, l, phase), wr = run (br, r, phase + 0.25f);
        l = l * (1.f - 0.5f * mix) + wl * 0.5f * mix * 1.4f;
        r = r * (1.f - 0.5f * mix) + wr * 0.5f * mix * 1.4f;
        w = (w + 1) % n;
    }

private:
    std::vector<float> bl, br;
    float sr = 44100.f, phase = 0.f;
    int w = 0;
};

// ---------------------------------------------------------------------------------------------
class Chorus
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        buf.assign ((size_t) (sr * 0.06f) + 8, 0.f);
        w = 0; phase = 0.f;
    }

    void process (float& l, float& r, float rate, float depth, float mix)
    {
        const int n = (int) buf.size();
        buf[(size_t) w] = (l + r) * 0.5f;

        phase += rate / sr;
        if (phase >= 1.f) phase -= 1.f;

        const float base = 0.0085f * sr;
        const float dep  = (0.0005f + depth * 0.0045f) * sr;

        auto tap = [&] (float ph)
        {
            const float d = base + dep * std::sin (2.f * kPi * ph);
            float rp = (float) w - d;
            while (rp < 0.f) rp += (float) n;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            const int i1 = (i0 + 1) % n;
            return buf[(size_t) i0] * (1.f - fr) + buf[(size_t) i1] * fr;
        };

        const float wl = 0.5f * (tap (phase) + tap (phase + 0.5f));
        const float wr = 0.5f * (tap (phase + 0.25f) + tap (phase + 0.75f));

        l = l * (1.f - 0.4f * mix) + wl * mix * 0.85f;
        r = r * (1.f - 0.4f * mix) + wr * mix * 0.85f;
        w = (w + 1) % n;
    }

private:
    std::vector<float> buf;
    float sr = 44100.f, phase = 0.f;
    int w = 0;
};

// ---------------------------------------------------------------------------------------------
class Delay
{
public:
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        const size_t n = (size_t) (sr * 1.5f) + 8;
        bl.assign (n, 0.f); br.assign (n, 0.f);
        w = 0; cur = 0.38f * sr; lpl = lpr = 0.f;
    }

    void process (float& l, float& r, float timeMs, float fb, float mix, bool ping)
    {
        const int n = (int) bl.size();
        const float target = std::clamp (timeMs * 0.001f * sr, 8.f, (float) n - 4.f);
        cur += (target - cur) * 0.00015f; // weiches "Tape"-artiges Gleiten bei Zeitänderung

        auto read = [&] (const std::vector<float>& b)
        {
            float rp = (float) w - cur;
            while (rp < 0.f) rp += (float) n;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            return b[(size_t) i0 % (size_t) n] * (1.f - fr) + b[(size_t) (i0 + 1) % (size_t) n] * fr;
        };

        const float dl = read (bl), dr = read (br);
        lpl += 0.45f * (dl - lpl); // Höhendämpfung in der Rückkopplung
        lpr += 0.45f * (dr - lpr);

        if (ping)
        {
            bl[(size_t) w] = (l + r) * 0.5f + lpr * fb;
            br[(size_t) w] = lpl * fb;
        }
        else
        {
            bl[(size_t) w] = l + lpl * fb;
            br[(size_t) w] = r + lpr * fb;
        }

        l += dl * mix;
        r += dr * mix;
        w = (w + 1) % n;
    }

private:
    std::vector<float> bl, br;
    float sr = 44100.f, cur = 0.f, lpl = 0.f, lpr = 0.f;
    int w = 0;
};

// ---------------------------------------------------------------------------------------------
class Reverb
{
public:
    void prepare (float sampleRate)
    {
        static const int ct[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
        static const int at[4] = { 556, 441, 341, 225 };
        const float sc = sampleRate / 44100.f;
        for (int i = 0; i < 8; ++i)
        {
            combL[i].init ((int) (ct[i] * sc));
            combR[i].init ((int) ((ct[i] + 23) * sc));
        }
        for (int i = 0; i < 4; ++i)
        {
            apL[i].init ((int) (at[i] * sc));
            apR[i].init ((int) ((at[i] + 23) * sc));
        }
    }

    void process (float& l, float& r, float size, float damp, float mix)
    {
        const float fb = 0.70f + 0.28f * size;
        const float d1 = damp * 0.4f, d2 = 1.f - d1;
        const float in = (l + r) * 0.5f * 0.015f;

        float oL = 0.f, oR = 0.f;
        for (int i = 0; i < 8; ++i)
        {
            oL += combL[i].tick (in, fb, d1, d2);
            oR += combR[i].tick (in, fb, d1, d2);
        }
        for (int i = 0; i < 4; ++i)
        {
            oL = apL[i].tick (oL);
            oR = apR[i].tick (oR);
        }
        l += oL * 3.f * mix;
        r += oR * 3.f * mix;
    }

private:
    struct Comb
    {
        std::vector<float> b; int i = 0; float store = 0.f;
        void init (int n) { b.assign ((size_t) std::max (n, 1), 0.f); i = 0; store = 0.f; }
        float tick (float in, float fb, float d1, float d2)
        {
            const float out = b[(size_t) i];
            store = out * d2 + store * d1;
            b[(size_t) i] = in + store * fb;
            if (++i >= (int) b.size()) i = 0;
            return out;
        }
    };
    struct Allpass
    {
        std::vector<float> b; int i = 0;
        void init (int n) { b.assign ((size_t) std::max (n, 1), 0.f); i = 0; }
        float tick (float in)
        {
            const float bo = b[(size_t) i];
            const float out = -in + bo;
            b[(size_t) i] = in + bo * 0.5f;
            if (++i >= (int) b.size()) i = 0;
            return out;
        }
    };
    Comb combL[8], combR[8];
    Allpass apL[4], apR[4];
};
} // namespace cs
