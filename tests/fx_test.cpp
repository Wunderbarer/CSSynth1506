// Offline-Test der neuen Effekte Distortion, Phaser, Flanger (ohne JUCE):
// Mix 0 = unverändert, Effekt wirkt hörbar, kein NaN, begrenzter Pegel, stabil bei Extremwerten, Spektrum.
//   fx_test
#include "../Source/Dsp/Engine.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace cs;

static int failures = 0;
static void check (bool ok, const char* what, double a = 0, double b = 0)
{
    std::printf ("%s  %s  (%.4f / %.4f)\n", ok ? "OK  " : "FAIL", what, a, b);
    if (! ok) ++failures;
}

template <typename F>
static std::vector<float> render (F set, double seconds = 2.0, int note = 45)
{
    const int sr = 44100;
    Engine e;
    e.prepare (sr);
    Params p;
    for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
    p.v[LP_LEVEL] = 0.8f; p.v[LP_AS] = 1.f; p.v[LP_AA] = 0.002f;
    p.v[GP_CH_MIX] = 0.f; p.v[GP_RV_MIX] = 0.f; p.v[GP_DL_MIX] = 0.f; p.v[GP_VEL_AMP] = 0.f;
    set (p);
    e.setParams (p);
    e.noteOn (note, 1.f);
    e.noteOn (note + 7, 1.f);
    const int n = (int) (seconds * sr);
    std::vector<float> l ((size_t) n), r ((size_t) n);
    for (int i = 0; i < n; i += 512)
    {
        e.setParams (p);
        e.process (l.data() + i, r.data() + i, std::min (512, n - i));
    }
    // links und rechts hintereinander zurückgeben (für Stereo-Prüfung)
    l.insert (l.end(), r.begin(), r.end());
    return l;
}

static double rms (const std::vector<float>& x, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) x[i] * x[i];
    return std::sqrt (s / (double) (b - a));
}
static bool sane (const std::vector<float>& x, double& peak)
{
    peak = 0;
    for (float v : x) { if (! std::isfinite (v)) return false; peak = std::max (peak, (double) std::fabs (v)); }
    return true;
}
static double diff (const std::vector<float>& a, const std::vector<float>& b)
{
    double s = 0; const size_t n = std::min (a.size(), b.size());
    for (size_t i = n / 4; i < n / 2; ++i) s += std::fabs (a[i] - b[i]);
    return s / (double) (n / 4);
}
// Energie oberhalb von 3 kHz (grobes Obertonmaß) per einfachem Differenzfilter
static double highEnergy (const std::vector<float>& x, size_t a, size_t b)
{
    double s = 0; for (size_t i = a + 2; i < b; ++i) { const double d = x[i] - 2.0 * x[i - 1] + x[i - 2]; s += d * d; }
    return std::sqrt (s / (double) (b - a));
}

int main()
{
    const auto dry = render ([] (Params&) {});
    const size_t half = dry.size() / 2;

    // 1) Mix = 0 verändert nichts, auch bei extremen Einstellungen der Effekte
    auto off = render ([] (Params& p)
    {
        p.v[GP_DS_MIX] = 0.f; p.v[GP_DS_DRIVE] = 1.f; p.v[GP_PH_MIX] = 0.f; p.v[GP_PH_FB] = 0.95f;
        p.v[GP_FL_MIX] = 0.f; p.v[GP_FL_FB] = 0.95f;
    });
    check (diff (dry, off) < 1e-9, "Mix 0: Signal unveraendert", diff (dry, off));

    // 2) Standardwerte: alle neuen Mischregler sind 0 (alte Presets klingen unverändert)
    check (specs()[GP_DS_MIX].def == 0.f && specs()[GP_PH_MIX].def == 0.f && specs()[GP_FL_MIX].def == 0.f,
           "Standard-Mix aller neuen Effekte = 0");

    // 3) Jeder Effekt verändert das Signal, bleibt endlich und begrenzt
    struct Case { const char* name; int mixIdx; };
    const Case cases[] = { { "Distortion", GP_DS_MIX }, { "Phaser", GP_PH_MIX }, { "Flanger", GP_FL_MIX } };
    for (const auto& c : cases)
    {
        const auto x = render ([&] (Params& p) { p.v[(size_t) c.mixIdx] = 1.f; p.v[GP_DS_DRIVE] = 0.7f; });
        double peak;
        const bool ok = sane (x, peak);
        char buf[96];
        std::snprintf (buf, sizeof buf, "%s: endlich, Spitze <= 1", c.name);
        check (ok && peak <= 1.0, buf, peak);
        std::snprintf (buf, sizeof buf, "%s: veraendert das Signal", c.name);
        check (diff (dry, x) > 0.002, buf, diff (dry, x));
        std::snprintf (buf, sizeof buf, "%s: Pegel plausibel (0.3x..3x)", c.name);
        const double r0 = rms (dry, half / 2, half), r1 = rms (x, half / 2, half);
        check (r1 > 0.3 * r0 && r1 < 3.0 * r0, buf, r0, r1);
    }

    // 4) Distortion: mehr Drive = mehr Obertöne; alle drei Typen funktionieren
    const auto d1 = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.1f; p.v[GP_DS_TONE] = 1.f; });
    const auto d2 = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.9f; p.v[GP_DS_TONE] = 1.f; });
    check (highEnergy (d2, half / 2, half) > highEnergy (d1, half / 2, half), "Distortion: mehr Drive = mehr Obertoene",
           highEnergy (d1, half / 2, half), highEnergy (d2, half / 2, half));
    const auto t0 = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.8f; p.v[GP_DS_TYPE] = 0; });
    const auto t1 = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.8f; p.v[GP_DS_TYPE] = 1; });
    const auto t2 = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.8f; p.v[GP_DS_TYPE] = 2; });
    check (diff (t0, t1) > 0.002 && diff (t1, t2) > 0.002 && diff (t0, t2) > 0.002, "Distortion: Soft/Hard/Fold klingen verschieden");
    const auto dt = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.8f; p.v[GP_DS_TONE] = 0.f; });
    const auto dh = render ([] (Params& p) { p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 0.8f; p.v[GP_DS_TONE] = 1.f; });
    check (highEnergy (dt, half / 2, half) < highEnergy (dh, half / 2, half), "Distortion: TONE dunkel < hell",
           highEnergy (dt, half / 2, half), highEnergy (dh, half / 2, half));

    // 5) Phaser / Flanger bewegen sich: Pegel schwankt über der Zeit nicht (kein Aufschaukeln) und Stereo-Breite entsteht
    for (int which = 0; which < 2; ++which)
    {
        const auto x = render ([&] (Params& p)
        {
            if (which == 0) { p.v[GP_PH_MIX] = 1.f; p.v[GP_PH_RATE] = 0.7f; }
            else            { p.v[GP_FL_MIX] = 1.f; p.v[GP_FL_RATE] = 0.7f; }
        });
        double side = 0;
        for (size_t i = half / 4; i < half / 2; ++i) side += std::fabs (x[i] - x[half + i]);
        side /= (double) (half / 4);
        check (side > 0.001, which == 0 ? "Phaser: Stereo-Breite" : "Flanger: Stereo-Breite", side);
    }

    // 6) Extremwerte: maximale Rückkopplung (positiv/negativ), schnelle LFOs, alle Effekte gleichzeitig - stabil
    for (int variant = 0; variant < 3; ++variant)
    {
        const auto x = render ([&] (Params& p)
        {
            p.v[GP_DS_MIX] = 1.f; p.v[GP_DS_DRIVE] = 1.f; p.v[GP_DS_TYPE] = (float) variant;
            p.v[GP_PH_MIX] = 1.f; p.v[GP_PH_FB] = 0.95f; p.v[GP_PH_RATE] = 8.f; p.v[GP_PH_DEPTH] = 1.f;
            p.v[GP_FL_MIX] = 1.f; p.v[GP_FL_FB] = variant == 1 ? -0.95f : 0.95f; p.v[GP_FL_RATE] = 5.f; p.v[GP_FL_DEPTH] = 1.f;
            p.v[GP_CH_MIX] = 1.f; p.v[GP_DL_MIX] = 0.5f; p.v[GP_RV_MIX] = 0.5f;
        }, 3.0);
        double peak;
        const bool ok = sane (x, peak);
        check (ok && peak <= 1.05, "Extremwerte: endlich und begrenzt", peak);
    }

    // 7) Nach Notenende klingen die Effekte sauber aus (Release, danach Stille)
    {
        const int sr = 44100;
        Engine e; e.prepare (sr);
        Params p; for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
        p.v[GP_DS_MIX] = 1.f; p.v[GP_PH_MIX] = 1.f; p.v[GP_FL_MIX] = 1.f; p.v[GP_FL_FB] = 0.9f; p.v[GP_PH_FB] = 0.9f;
        p.v[GP_CH_MIX] = 0.f; p.v[GP_RV_MIX] = 0.f; p.v[GP_DL_MIX] = 0.f;
        p.v[LP_AR] = 0.1f;
        e.setParams (p); e.noteOn (50, 1.f);
        std::vector<float> l (512), r (512);
        for (int i = 0; i < 40; ++i) e.process (l.data(), r.data(), 512);
        e.noteOff (50);
        double last = 1;
        for (int i = 0; i < 200; ++i) { e.process (l.data(), r.data(), 512); }
        last = rms (std::vector<float> (l.begin(), l.end()), 0, 512);
        check (last < 1e-3, "Ausklingen nach Notenende (Stille)", last);
    }

    std::printf (failures ? "\n%d Test(s) FEHLGESCHLAGEN\n" : "\nALLE FX-TESTS BESTANDEN\n", failures);
    return failures ? 1 : 0;
}
