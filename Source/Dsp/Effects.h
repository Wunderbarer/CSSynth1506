// CSSynth 1506 - Digitale Effekte: Chorus, Delay (Ping-Pong), Reverb (Freeverb-Topologie)
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
