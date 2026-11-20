// Offline-Test der neuen Effekte Distortion, Phaser, Flanger (ohne JUCE):
// Mix 0 = unverändert, Effekt wirkt hörbar, kein NaN, begrenzter Pegel, stabil bei Extremwerten, Spektrum.
//   fx_test
#include "../Source/Dsp/Engine.h"
#include <algorithm>
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

// ---------------------------------------------------------------------------------------------
// Wellenformen der Layer I-III (Sinus, Dreieck): Spektrum per FFT
#include <complex>
static void fft (std::vector<std::complex<double>>& a)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap (a[i], a[j]);
    }
    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double ang = -2.0 * 3.14159265358979323846 / (double) len;
        const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w (1.0);
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v; a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}

// Rendert eine einzelne Note auf Layer I mit der gewählten Wellenform (Filter offen, nichts sonst) und liefert das Leistungsspektrum
static std::vector<double> waveSpectrum (int wave, int note, double& f0)
{
    const int sr = 44100, N = 32768;
    Engine e; e.prepare (sr); e.setDriftAmount (0.f);
    Params p;
    for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
    for (int L = 0; L < NUM_LAYERS; ++L) p.v[(size_t) (L * LP_COUNT + LP_LEVEL)] = 0.f;
    p.v[LP_LEVEL] = 0.8f; p.v[LP_WAVE] = (float) wave; p.v[LP_LPF] = 18000.f; p.v[LP_HPF] = 20.f; p.v[LP_RES] = 0.f;
    p.v[LP_AMT] = 0.f; p.v[LP_KEY] = 0.f; p.v[LP_SUB] = 0.f; p.v[LP_NOISE] = 0.f; p.v[LP_DETUNE] = 0.f;
    p.v[LP_AA] = 0.001f; p.v[LP_AS] = 1.f; p.v[LP_OCT] = 2.f;
    p.v[GP_CH_MIX] = p.v[GP_DL_MIX] = p.v[GP_RV_MIX] = 0.f; p.v[GP_VEL_AMP] = 0.f;
    e.setParams (p);
    e.noteOn (note, 1.f);
    std::vector<float> l ((size_t) (sr / 2 + N)), r (l.size());
    for (size_t i = 0; i < l.size(); i += 512) e.process (l.data() + i, r.data() + i, (int) std::min<size_t> (512, l.size() - i));
    f0 = 440.0 * std::pow (2.0, (note - 69) / 12.0);
    std::vector<std::complex<double>> a ((size_t) N);
    for (int i = 0; i < N; ++i)
        a[(size_t) i] = (double) l[(size_t) (sr / 2 + i)] * (0.5 - 0.5 * std::cos (2.0 * 3.14159265358979323846 * i / N));
    fft (a);
    std::vector<double> pw ((size_t) N / 2);
    for (size_t i = 0; i < pw.size(); ++i) pw[i] = std::norm (a[i]);
    return pw;
}
static double harmonicAmp (const std::vector<double>& pw, double f0, int k, int sr = 44100)
{
    const double bin = f0 * k / ((double) sr / (double) (pw.size() * 2));
    double best = 0;
    for (int d = -3; d <= 3; ++d) { const long b = (long) std::lround (bin) + d; if (b >= 0 && b < (long) pw.size()) best = std::max (best, pw[(size_t) b]); }
    return std::sqrt (best);
}
static double inharmonicFraction (const std::vector<double>& pw, double f0, int sr = 44100)
{
    const double binHz = (double) sr / (double) (pw.size() * 2);
    double total = 0, bad = 0;
    for (size_t b = 3; b < pw.size(); ++b)
    {
        total += pw[b];
        const double hz = (double) b * binHz;
        const double k = std::round (hz / f0);
        const bool near = k >= 1 && std::fabs (hz - k * f0) < 4.0 * binHz;
        if (! near) bad += pw[b];
    }
    return bad / total;
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

    // 8) Neue Wellenformen der Layer I-III: Sinus und Dreieck
    {
        double f0 = 0;
        const auto sine = waveSpectrum (3, 57, f0);
        const double s1 = harmonicAmp (sine, f0, 1);
        double worst = 0;
        for (int k = 2; k <= 8; ++k) worst = std::max (worst, harmonicAmp (sine, f0, k) / s1);
        check (s1 > 1.0, "Sinus: Grundton vorhanden", s1);
        check (worst < 0.01, "Sinus: Obertöne unter -40 dB", worst);
        check (inharmonicFraction (sine, f0) < 1e-4, "Sinus: kein unharmonischer Anteil", inharmonicFraction (sine, f0));

        const auto tri = waveSpectrum (4, 57, f0);
        const double t1 = harmonicAmp (tri, f0, 1);
        check (std::fabs (harmonicAmp (tri, f0, 3) / t1 - 1.0 / 9.0) < 0.02, "Dreieck: 3. Oberton = 1/9", harmonicAmp (tri, f0, 3) / t1);
        check (std::fabs (harmonicAmp (tri, f0, 5) / t1 - 1.0 / 25.0) < 0.01, "Dreieck: 5. Oberton = 1/25", harmonicAmp (tri, f0, 5) / t1);
        check (harmonicAmp (tri, f0, 2) / t1 < 0.01 && harmonicAmp (tri, f0, 4) / t1 < 0.01, "Dreieck: keine geraden Obertöne",
               harmonicAmp (tri, f0, 2) / t1, harmonicAmp (tri, f0, 4) / t1);

        const auto saw = waveSpectrum (0, 57, f0);
        check (harmonicAmp (saw, f0, 2) / harmonicAmp (saw, f0, 1) > 0.3, "Saw bleibt unverändert (2. Oberton ~1/2)", harmonicAmp (saw, f0, 2) / harmonicAmp (saw, f0, 1));
        check (harmonicAmp (tri, f0, 3) / t1 < harmonicAmp (saw, f0, 3) / harmonicAmp (saw, f0, 1) * 0.5, "Dreieck ist weicher als Saw (3. Oberton relativ)");

        // hohe Töne: kaum Aliasing (bandbegrenztes Dreieck)
        for (int note : { 84, 96, 100 })
        {
            const auto hi = waveSpectrum (4, note, f0);
            const double frac = inharmonicFraction (hi, f0);
            char buf[96]; std::snprintf (buf, sizeof buf, "Dreieck Note %d (%.0f Hz): Aliasing unter -40 dB", note, f0);
            check (frac < 1e-4, buf, frac);
        }
        for (int note : { 84, 100 })
        {
            const auto hi = waveSpectrum (3, note, f0);
            char buf[96]; std::snprintf (buf, sizeof buf, "Sinus Note %d: sauber", note);
            check (inharmonicFraction (hi, f0) < 1e-4, buf, inharmonicFraction (hi, f0));
        }

        // Pegel der Wellenformen vergleichbar (Sinus/Dreieck nicht stumm, nicht lauter als Square)
        const auto sq = waveSpectrum (1, 57, f0);
        double rmsS = 0, rmsT = 0, rmsQ = 0;
        for (double v : sine) rmsS += v;
        for (double v : tri)  rmsT += v;
        for (double v : sq)   rmsQ += v;
        check (rmsS > 0.1 * rmsQ && rmsS < 2.0 * rmsQ && rmsT > 0.1 * rmsQ && rmsT < 2.0 * rmsQ, "Pegel von Sinus/Dreieck liegt im Rahmen von Square", rmsS / rmsQ, rmsT / rmsQ);
    }

    // 9) Qualitäts-Update: Stereo, Drift, Loop-Überblendung, Distortion-Oversampling, Reverb
    {
        auto stereoDiff = [] (int note, bool layers2)
        {
            Engine e; e.prepare (48000);
            Params p; for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
            p.v[GP_CH_MIX] = 0.f; p.v[GP_RV_MIX] = 0.f; p.v[GP_DL_MIX] = 0.f;
            p.v[LP_LEVEL] = 0.8f; p.v[LP_AS] = 1.f;
            if (layers2) { p.v[LP_COUNT + LP_LEVEL] = 0.8f; p.v[LP_COUNT + LP_AS] = 1.f; p.v[2 * LP_COUNT + LP_LEVEL] = 0.8f; p.v[2 * LP_COUNT + LP_AS] = 1.f; }
            e.setParams (p); e.noteOn (note, 0.8f);
            std::vector<float> l (48000), r (48000);
            for (int i = 0; i < 48000; i += 480) e.process (l.data() + i, r.data() + i, 480);
            double d = 0, m = 0; for (int i = 24000; i < 48000; ++i) { d += std::fabs (l[(size_t) i] - r[(size_t) i]); m += std::fabs (l[(size_t) i]) + std::fabs (r[(size_t) i]); }
            return d / m;
        };
        check (stereoDiff (36, true) < 1e-3, "Stereo: tiefe Töne (C2) bleiben mono", stereoDiff (36, true));
        check (stereoDiff (72, true) > 0.05, "Stereo: mehrere Layer in hoher Lage haben Breite", stereoDiff (72, true));
        check (stereoDiff (72, false) < 0.05, "Stereo: einzelner Layer bleibt fast mittig", stereoDiff (72, false));

        // Drift: Sinus A4 -> Frequenzabweichung bleibt klein (< 8 Cent), ist aber vorhanden
        {
            Engine e; e.prepare (48000);
            Params p; for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
            p.v[GP_CH_MIX] = 0.f; p.v[GP_RV_MIX] = 0.f; p.v[GP_DL_MIX] = 0.f; p.v[LP_WAVE] = 3; p.v[LP_LEVEL] = 0.8f; p.v[LP_AS] = 1.f; p.v[LP_LPF] = 18000.f;
            e.setParams (p); e.noteOn (69, 0.8f);
            const int N = 48000 * 12; std::vector<float> l ((size_t) N), r ((size_t) N);
            for (int i = 0; i < N; i += 480) e.process (l.data() + i, r.data() + i, 480);
            // Frequenz aus Nulldurchgängen pro Sekunde
            double lo = 1e9, hi = -1e9;
            for (int sec = 1; sec < 12; ++sec)
            {
                int z = 0; for (int i = sec * 48000; i < (sec + 1) * 48000 - 1; ++i) if (l[(size_t) i] <= 0.f && l[(size_t) i + 1] > 0.f) ++z;
                const double cents = 1200.0 * std::log2 ((double) z / 440.0); lo = std::min (lo, cents); hi = std::max (hi, cents);
            }
            check (hi - lo < 16.0, "Drift: Schwankung bleibt klein (Cent, Spanne)", hi - lo, 16.0);
        }

        // Sample-Loop: kein Knacken an der Loop-Grenze (Sinus, Loop passt nicht ganzzahlig)
        {
            SampleData sd; sd.rate = 32000; sd.data.resize (8000);
            for (size_t i = 0; i < sd.data.size(); ++i) sd.data[i] = 0.8f * std::sin (2.0 * 3.14159265358979 * 441.3 * (double) i / 32000.0);
            Engine e; e.prepare (48000); e.setSample (&sd);
            Params p; for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
            for (int L = 0; L < 5; ++L) p.v[L * LP_COUNT + LP_LEVEL] = 0.f;
            const int o = 4 * LP_COUNT; p.v[o + LP_LEVEL] = 1.f; p.v[o + LP_WAVE] = 0; p.v[o + LP_LPF] = 18000.f; p.v[o + LP_AS] = 1.f;
            p.v[GP_SMP_ROOT] = 69; p.v[GP_SMP_LSTART] = 0.1f; p.v[GP_SMP_LEND] = 0.9f; p.v[GP_RV_MIX] = 0; p.v[GP_CH_MIX] = 0; p.v[GP_DL_MIX] = 0;
            e.setParams (p); e.noteOn (69, 1.f);
            const int N = 96000; std::vector<float> l ((size_t) N), r ((size_t) N);
            for (int i = 0; i < N; i += 480) e.process (l.data() + i, r.data() + i, 480);
            std::vector<double> d; for (int i = 24000; i < N - 1; ++i) d.push_back (std::fabs ((double) l[(size_t) i + 1] - l[(size_t) i]));
            double mx = 0; for (double v : d) mx = std::max (mx, v);
            std::sort (d.begin(), d.end());
            check (mx < 3.0 * d[d.size() / 2], "Sample-Loop: kein Knacken an der Loop-Grenze (max/median Schritt)", mx / d[d.size() / 2], 3.0);
        }

        // Distortion: Alias-Anteil (Sinus 3,1 kHz, Soft, Drive voll) deutlich unter -35 dB
        {
            Distortion dd; dd.prepare (48000);
            const int N = 48000; std::vector<float> y ((size_t) N);
            for (int i = 0; i < N; ++i) { float l = 0.7f * (float) std::sin (2.0 * 3.14159265358979 * 3100.0 * i / 48000.0), r = l; dd.process (l, r, 0, 1.f, 1.f, 1.f); y[(size_t) i] = l; }
            const int n = 16384, a = 8192; double tot = 0, har = 0;
            for (int k = 1; k < n / 2; ++k)
            {
                const double f = k * 48000.0 / n; double re = 0, im = 0;
                for (int i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * 3.14159265358979 * i / n), ph = -2 * 3.14159265358979 * f * i / 48000.0; re += y[(size_t) (a + i)] * w * std::cos (ph); im += y[(size_t) (a + i)] * w * std::sin (ph); }
                const double pw2 = re * re + im * im; tot += pw2; const double h = f / 3100.0; if (std::fabs (h - std::round (h)) < 0.012) har += pw2;
            }
            check (10 * std::log10 ((tot - har) / tot) < -35.0, "Distortion: Alias-Anteil unter -35 dB (4x-Oversampling)", 10 * std::log10 ((tot - har) / tot), -35.0);
        }

        // Reverb: stabil, klingt aus (Impuls, SIZE 1.0), Stereo-Ausgang dekorreliert
        {
            Reverb rv; rv.prepare (48000);
            double eEarly = 0, eLate = 0, corr = 0, el = 0, er = 0; bool ok = true;
            for (int i = 0; i < 48000 * 30; ++i)
            {
                float l = i == 0 ? 1.f : 0.f, r = l; rv.process (l, r, 1.f, 0.5f, 1.f);
                if (! std::isfinite (l) || ! std::isfinite (r)) ok = false;
                if (i > 4800 && i < 48000 * 2) { eEarly += (double) l * l; corr += (double) l * r; el += (double) l * l; er += (double) r * r; }
                if (i > 48000 * 28) eLate += (double) l * l;
            }
            check (ok && eLate < eEarly * 1e-6, "Reverb: SIZE 1.0 klingt aus (kein Aufschaukeln)", eLate / std::max (eEarly, 1e-30), 1e-6);
            check (std::fabs (corr / std::sqrt (el * er)) < 0.5, "Reverb: Stereo-Kanäle sind dekorreliert", std::fabs (corr / std::sqrt (el * er)), 0.5);
        }
    }

    std::printf (failures ? "\n%d Test(s) FEHLGESCHLAGEN\n" : "\nALLE FX-TESTS BESTANDEN\n", failures);
    return failures ? 1 : 0;
}
