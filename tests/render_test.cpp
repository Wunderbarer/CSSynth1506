// Offline-Test der Klangerzeugung (ohne JUCE): rendert jedes Preset als Akkord in eine WAV-Datei
// und prüft auf NaN/Inf, Stille, Übersteuerung und hängende Stimmen.
//   render_test [ausgabeverzeichnis]
#include "../Source/Dsp/Engine.h"
#include "../Source/Presets.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

static void writeWav (const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sr)
{
    FILE* f = std::fopen (path.c_str(), "wb");
    if (! f) return;
    const std::uint32_t n = (std::uint32_t) l.size(), dataBytes = n * 4, riff = 36 + dataBytes, fmtLen = 16, rate = (std::uint32_t) sr, byteRate = rate * 4;
    const std::uint16_t pcm = 1, ch = 2, align = 4, bits = 16;
    std::fwrite ("RIFF", 1, 4, f); std::fwrite (&riff, 4, 1, f); std::fwrite ("WAVEfmt ", 1, 8, f);
    std::fwrite (&fmtLen, 4, 1, f); std::fwrite (&pcm, 2, 1, f); std::fwrite (&ch, 2, 1, f);
    std::fwrite (&rate, 4, 1, f); std::fwrite (&byteRate, 4, 1, f); std::fwrite (&align, 2, 1, f); std::fwrite (&bits, 2, 1, f);
    std::fwrite ("data", 1, 4, f); std::fwrite (&dataBytes, 4, 1, f);
    for (std::uint32_t i = 0; i < n; ++i)
    {
        const std::int16_t a = (std::int16_t) std::clamp (l[i] * 32767.f, -32768.f, 32767.f);
        const std::int16_t b = (std::int16_t) std::clamp (r[i] * 32767.f, -32768.f, 32767.f);
        std::fwrite (&a, 2, 1, f); std::fwrite (&b, 2, 1, f);
    }
    std::fclose (f);
}

int main (int argc, char** argv)
{
    const std::string outDir = argc > 1 ? argv[1] : ".";
    const int sr = 48000;
    int failures = 0;

    for (const auto& preset : cs::factoryPresets())
    {
        cs::Params p;
        for (int i = 0; i < cs::NUM_PARAMS; ++i) p.v[(size_t) i] = cs::specs()[(size_t) i].def;
        for (const auto& kv : preset.values)
        {
            const int idx = cs::indexOf (kv.first);
            if (idx < 0) { std::printf ("FEHLER: unbekannte Parameter-ID %s in Preset %s\n", kv.first, preset.name); ++failures; continue; }
            p.v[(size_t) idx] = kv.second;
        }

        cs::Engine e;
        e.prepare (sr);
        e.setParams (p);

        const int hold = sr * 3, tail = sr * 4;
        std::vector<float> l ((size_t) (hold + tail)), r (l.size());
        const int chord[4] = { 48, 55, 60, 64 };
        int pos = 0;
        auto run = [&] (int n)
        {
            // in unregelmäßigen Blockgrößen rendern wie ein Host
            int done = 0; int bs = 37;
            while (done < n) { const int m = std::min (bs, n - done); e.process (l.data() + pos + done, r.data() + pos + done, m); done += m; bs = bs == 37 ? 512 : 37; }
            pos += n;
        };
        for (int note : chord) e.noteOn (note, 0.8f);
        run (hold);
        for (int note : chord) e.noteOff (note);
        run (tail);

        float peak = 0.f; double sum = 0.0, tailSum = 0.0; bool bad = false;
        for (size_t i = 0; i < l.size(); ++i)
        {
            const float a = std::max (std::fabs (l[i]), std::fabs (r[i]));
            if (! std::isfinite (a)) bad = true;
            peak = std::max (peak, a);
            if ((int) i < hold) sum += (double) l[i] * l[i];
            if ((int) i > hold + tail - sr / 2) tailSum += (double) l[i] * l[i];
        }
        const double rms = std::sqrt (sum / hold), tailRms = std::sqrt (tailSum / (sr / 2));
        const bool ok = ! bad && peak > 0.05f && peak < 1.0f && tailRms < 0.01 && rms > 0.01;
        std::printf ("%-14s peak %.3f  rms(hold) %.3f  rms(letzte 0.5s) %.5f  %s\n", preset.name, peak, rms, tailRms, ok ? "OK" : "CHECK");
        if (! ok) ++failures;

        std::string fn = outDir + "/" + preset.name + ".wav";
        for (auto& c : fn) if (c == ' ') c = '_';
        writeWav (fn, l, r, sr);
    }
    std::printf (failures ? "\n%d Problem(e)\n" : "\nAlle Presets OK\n", failures);
    return failures ? 1 : 0;
}
