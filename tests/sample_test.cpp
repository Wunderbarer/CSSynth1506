// Offline-Test des Sample-Oszillators (Layer V, ohne JUCE):
// Tonhöhe (Grundton, Oktave, Samplerate-Umrechnung), Loop / One-Shot / Ping-Pong, Startposition, kein NaN.
//   sample_test
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

static SampleData makeSine (double freq, double rate, double seconds)
{
    SampleData s;
    s.rate = rate;
    s.name = "test";
    const int n = (int) (rate * seconds);
    for (int i = 0; i < n; ++i) s.data.push_back ((float) std::sin (2.0 * 3.14159265358979 * freq * i / rate));
    return s;
}

struct Result { std::vector<float> out; };

// rendert eine Note (Dauer in Sekunden) mit dem Sample-Layer; set() darf Parameter anpassen
template <typename F>
static std::vector<float> render (const SampleData* sd, int note, double seconds, F set)
{
    const int sr = 44100;
    Engine e;
    e.prepare (sr);
    Params p;
    for (int i = 0; i < NUM_PARAMS; ++i) p.v[(size_t) i] = specs()[(size_t) i].def;
    const int o5 = SAMPLE_LAYER * LP_COUNT;
    p.v[LP_LEVEL] = 0.f;                       // Layer I aus
    p.v[o5 + LP_LEVEL] = 1.f;
    p.v[o5 + LP_LPF] = 18000.f; p.v[o5 + LP_HPF] = 20.f; p.v[o5 + LP_RES] = 0.f; p.v[o5 + LP_AMT] = 0.f; p.v[o5 + LP_KEY] = 0.f;
    p.v[o5 + LP_AA] = 0.002f; p.v[o5 + LP_AS] = 1.f;
    p.v[GP_CH_MIX] = 0.f; p.v[GP_RV_MIX] = 0.f; p.v[GP_DL_MIX] = 0.f; p.v[GP_VEL_AMP] = 0.f;
    p.v[GP_SMP_ROOT] = 69.f;
    set (p);
    e.setParams (p);
    e.setSample (sd);
    e.noteOn (note, 1.f);

    const int n = (int) (seconds * sr);
    std::vector<float> l ((size_t) n), r ((size_t) n);
    for (int i = 0; i < n; i += 512)
    {
        const int k = std::min (512, n - i);
        e.setParams (p);
        e.process (l.data() + i, r.data() + i, k);
    }
    return l;
}

static double crossingsFreq (const std::vector<float>& x, double t0, double t1, int sr = 44100)
{
    int c = 0;
    for (size_t i = (size_t) (t0 * sr) + 1; i < (size_t) (t1 * sr); ++i)
        if (x[i - 1] < 0.f && x[i] >= 0.f) ++c;
    return c / (t1 - t0);
}
static double rms (const std::vector<float>& x, double t0, double t1, int sr = 44100)
{
    double s = 0; size_t n = 0;
    for (size_t i = (size_t) (t0 * sr); i < (size_t) (t1 * sr) && i < x.size(); ++i) { s += (double) x[i] * x[i]; ++n; }
    return n ? std::sqrt (s / (double) n) : 0.0;
}
static bool finite (const std::vector<float>& x) { for (float v : x) if (! std::isfinite (v)) return false; return true; }

int main()
{
    const auto sine = makeSine (440.0, 44100.0, 1.0);

    // Grundton: Note = Root -> Originaltonhöhe; eine Oktave höher -> doppelte Frequenz
    auto a = render (&sine, 69, 2.0, [] (Params&) {});
    check (std::fabs (crossingsFreq (a, 0.3, 1.8) - 440.0) < 3.0, "Note = Root spielt 440 Hz", crossingsFreq (a, 0.3, 1.8), 440);
    auto b = render (&sine, 81, 2.0, [] (Params&) {});
    check (std::fabs (crossingsFreq (b, 0.3, 1.8) - 880.0) < 6.0, "Note +12 spielt 880 Hz", crossingsFreq (b, 0.3, 1.8), 880);
    auto c = render (&sine, 57, 2.0, [] (Params&) {});
    check (std::fabs (crossingsFreq (c, 0.3, 1.8) - 220.0) < 3.0, "Note -12 spielt 220 Hz", crossingsFreq (c, 0.3, 1.8), 220);

    // Oktavregler (4' = eine Oktave höher)
    auto oc = render (&sine, 69, 2.0, [] (Params& p) { p.v[SAMPLE_LAYER * LP_COUNT + LP_OCT] = 3.f; });
    check (std::fabs (crossingsFreq (oc, 0.3, 1.8) - 880.0) < 6.0, "Oktave 4' verdoppelt die Frequenz", crossingsFreq (oc, 0.3, 1.8), 880);

    // Root-Note verschoben: Root 57 -> Note 69 klingt eine Oktave höher
    auto rt = render (&sine, 69, 2.0, [] (Params& p) { p.v[GP_SMP_ROOT] = 57.f; });
    check (std::fabs (crossingsFreq (rt, 0.3, 1.8) - 880.0) < 6.0, "Root-Note wird berücksichtigt", crossingsFreq (rt, 0.3, 1.8), 880);

    // Samplerate der Datei (22,05 kHz-Datei mit 440 Hz) -> bei Root trotzdem 440 Hz
    const auto sine22 = makeSine (440.0, 22050.0, 1.0);
    auto s22 = render (&sine22, 69, 2.0, [] (Params&) {});
    check (std::fabs (crossingsFreq (s22, 0.3, 1.8) - 440.0) < 3.0, "Samplerate-Umrechnung (22,05 kHz -> 44,1 kHz)", crossingsFreq (s22, 0.3, 1.8), 440);

    // Pegel (Sinus Spitzenpegel 1 -> ca. 0.22 * Master-Kurve; wichtig: hörbar und nicht übersteuert)
    check (rms (a, 0.5, 1.5) > 0.02 && rms (a, 0.5, 1.5) < 0.6, "Pegel plausibel", rms (a, 0.5, 1.5), 0);

    // Loop: Datei ist 1 s lang, Note klingt 3 s weiter
    auto lp = render (&sine, 69, 3.5, [] (Params&) {});
    check (rms (lp, 2.5, 3.4) > 0.02, "Loop klingt nach dem Dateiende weiter", rms (lp, 2.5, 3.4), 0);

    // One-Shot: nach dem Ende ist Stille
    auto os = render (&sine, 69, 3.5, [] (Params& p) { p.v[SAMPLE_LAYER * LP_COUNT + LP_WAVE] = 1.f; });
    check (rms (os, 0.2, 0.8) > 0.02, "One-Shot klingt am Anfang", rms (os, 0.2, 0.8), 0);
    check (rms (os, 1.3, 3.4) < 0.002, "One-Shot endet am Dateiende", rms (os, 1.3, 3.4), 0);

    // Ping-Pong klingt weiter und bleibt endlich
    auto pp = render (&sine, 69, 3.5, [] (Params& p) { p.v[SAMPLE_LAYER * LP_COUNT + LP_WAVE] = 2.f; });
    check (rms (pp, 2.5, 3.4) > 0.02 && finite (pp), "Ping-Pong klingt weiter", rms (pp, 2.5, 3.4), 0);

    // Startposition 50 %: One-Shot dauert nur noch ca. 0.5 s
    auto st = render (&sine, 69, 2.5, [] (Params& p) { p.v[SAMPLE_LAYER * LP_COUNT + LP_WAVE] = 1.f; p.v[SAMPLE_LAYER * LP_COUNT + LP_PW] = 0.5f; });
    check (rms (st, 0.1, 0.4) > 0.02 && rms (st, 0.8, 2.4) < 0.002, "Startposition 50 % kürzt One-Shot", rms (st, 0.1, 0.4), rms (st, 0.8, 2.4));

    // Loop-Bereich 0..10 %: Wiedergabe bleibt im kurzen Loop (hört man als weiter klingenden Ton)
    auto ls = render (&sine, 69, 3.0, [] (Params& p) { p.v[GP_SMP_LSTART] = 0.1f; p.v[GP_SMP_LEND] = 0.4f; });
    check (rms (ls, 2.0, 2.9) > 0.02 && finite (ls), "Loop-Bereich 10..40 % klingt weiter", rms (ls, 2.0, 2.9), 0);

    // Kein Sample geladen -> Stille, kein Absturz; extrem hohe Töne / extremes Detune bleiben endlich
    auto none = render (nullptr, 69, 1.0, [] (Params&) {});
    check (rms (none, 0.2, 0.9) < 1e-6, "ohne Sample bleibt Layer V still", rms (none, 0.2, 0.9), 0);
    auto hi = render (&sine, 127, 1.0, [] (Params& p) { p.v[SAMPLE_LAYER * LP_COUNT + LP_OCT] = 4.f; p.v[SAMPLE_LAYER * LP_COUNT + LP_DETUNE] = 50.f; });
    check (finite (hi), "extreme Tonhöhe bleibt endlich");

    // sehr kurzes Sample (2 Werte) und leeres Sample
    SampleData tiny; tiny.data = { 0.5f, -0.5f };
    auto ti = render (&tiny, 69, 0.5, [] (Params&) {});
    check (finite (ti), "winziges Sample bleibt endlich");
    SampleData empty;
    auto em = render (&empty, 69, 0.5, [] (Params&) {});
    check (finite (em) && rms (em, 0.1, 0.4) < 1e-6, "leeres Sample bleibt still");

    std::printf ("\n%s\n", failures == 0 ? "ALLE SAMPLE-TESTS BESTANDEN" : "FEHLER");
    return failures == 0 ? 0 : 1;
}
