// CSSynth 1506 - Werks-Presets. Nicht aufgeführte Parameter bleiben auf ihrem Standardwert.
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace cs
{
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<Preset>& factoryPresets()
{
    // Wave: 0 Saw, 1 Square, 2 Pulse | Oct: 0=32' 1=16' 2=8' 3=4' 4=2' | LFO-Shape: 0 Sine 1 Tri 2 Saw 3 S&H
    static const std::vector<Preset> p = {
        { "Init", {} },

        { "Blade Brass", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -6 }, { "l1_lpf", 700 }, { "l1_res", 0.30f }, { "l1_amt", 0.55f },
            { "l1_fil", 0.15f }, { "l1_fa", 0.45f }, { "l1_fal", 1.0f }, { "l1_fd", 1.6f }, { "l1_fsl", 0.55f }, { "l1_fr", 0.5f },
            { "l1_aa", 0.10f }, { "l1_ad", 0.5f }, { "l1_as", 0.95f }, { "l1_ar", 0.45f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 7 }, { "l2_lpf", 900 }, { "l2_res", 0.25f }, { "l2_amt", 0.5f },
            { "l2_fil", 0.15f }, { "l2_fa", 0.5f }, { "l2_fal", 1.0f }, { "l2_fd", 1.8f }, { "l2_fsl", 0.6f }, { "l2_fr", 0.5f },
            { "l2_aa", 0.12f }, { "l2_ad", 0.5f }, { "l2_as", 0.95f }, { "l2_ar", 0.5f }, { "l2_level", 0.7f },
            { "at_filter", 0.35f }, { "at_vib", 0.3f }, { "ch_mix", 0.22f }, { "dl_mix", 0.10f }, { "dl_time", 480 }, { "rv_mix", 0.35f }, { "rv_size", 0.75f } } },

        { "Warm Strings", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -9 }, { "l1_lpf", 2600 }, { "l1_res", 0.1f }, { "l1_amt", 0.15f }, { "l1_brill", 0.45f },
            { "l1_aa", 0.55f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 0.9f }, { "l1_fa", 0.6f }, { "l1_fd", 1.0f }, { "l1_fsl", 0.8f }, { "l1_level", 0.75f },
            { "l2_wave", 0 }, { "l2_oct", 3 }, { "l2_detune", 11 }, { "l2_lpf", 3200 }, { "l2_res", 0.1f }, { "l2_amt", 0.15f },
            { "l2_aa", 0.7f }, { "l2_ad", 1.0f }, { "l2_as", 0.9f }, { "l2_ar", 1.0f }, { "l2_fa", 0.7f }, { "l2_fd", 1.0f }, { "l2_fsl", 0.8f }, { "l2_level", 0.45f },
            { "lfo_rate", 5.5f }, { "lfo_pitch", 0.06f }, { "ch_mix", 0.6f }, { "ch_depth", 0.65f }, { "rv_mix", 0.4f }, { "rv_size", 0.8f } } },

        { "Glass Pad", {
            { "l1_wave", 2 }, { "l1_pw", 0.35f }, { "l1_oct", 2 }, { "l1_lpf", 1800 }, { "l1_res", 0.2f }, { "l1_amt", 0.3f }, { "l1_hpf", 120 },
            { "l1_fa", 1.2f }, { "l1_fd", 2.5f }, { "l1_fsl", 0.6f }, { "l1_fr", 1.5f }, { "l1_aa", 1.0f }, { "l1_ad", 1.5f }, { "l1_as", 0.85f }, { "l1_ar", 1.8f }, { "l1_level", 0.7f },
            { "l2_wave", 2 }, { "l2_pw", 0.6f }, { "l2_oct", 3 }, { "l2_detune", 8 }, { "l2_lpf", 2400 }, { "l2_res", 0.2f }, { "l2_amt", 0.3f }, { "l2_hpf", 200 },
            { "l2_fa", 1.5f }, { "l2_fd", 2.5f }, { "l2_fsl", 0.6f }, { "l2_fr", 1.5f }, { "l2_aa", 1.3f }, { "l2_ad", 1.5f }, { "l2_as", 0.85f }, { "l2_ar", 2.0f }, { "l2_level", 0.5f },
            { "lfo_rate", 0.6f }, { "lfo_pwm", 0.6f }, { "lfo_cut", 0.15f }, { "lfo_shape", 1 }, { "ch_mix", 0.5f }, { "ch_rate", 0.4f }, { "rv_mix", 0.5f }, { "rv_size", 0.9f }, { "dl_mix", 0.18f }, { "dl_time", 560 }, { "dl_fb", 0.45f } } },

        { "Sub Bass", {
            { "l1_wave", 0 }, { "l1_oct", 1 }, { "l1_sub", 0.6f }, { "l1_lpf", 260 }, { "l1_res", 0.25f }, { "l1_amt", 0.45f }, { "l1_key", 0.3f },
            { "l1_fil", 0.0f }, { "l1_fa", 0.005f }, { "l1_fd", 0.25f }, { "l1_fsl", 0.15f }, { "l1_fr", 0.15f },
            { "l1_aa", 0.003f }, { "l1_ad", 0.3f }, { "l1_as", 0.85f }, { "l1_ar", 0.12f }, { "l1_level", 0.95f },
            { "l2_level", 0.0f }, { "glide", 0.06f }, { "ch_mix", 0.0f }, { "rv_mix", 0.06f }, { "master", 0.65f } } },

        { "Ribbon Lead", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_lpf", 2200 }, { "l1_res", 0.35f }, { "l1_amt", 0.35f }, { "l1_brill", 0.6f },
            { "l1_fa", 0.02f }, { "l1_fd", 0.5f }, { "l1_fsl", 0.6f }, { "l1_fr", 0.3f }, { "l1_aa", 0.02f }, { "l1_ad", 0.3f }, { "l1_as", 0.9f }, { "l1_ar", 0.3f }, { "l1_level", 0.7f },
            { "l2_wave", 1 }, { "l2_oct", 3 }, { "l2_detune", 5 }, { "l2_lpf", 2800 }, { "l2_res", 0.3f }, { "l2_amt", 0.3f },
            { "l2_fa", 0.02f }, { "l2_fd", 0.5f }, { "l2_fsl", 0.6f }, { "l2_aa", 0.02f }, { "l2_ad", 0.3f }, { "l2_as", 0.9f }, { "l2_ar", 0.3f }, { "l2_level", 0.35f },
            { "glide", 0.09f }, { "bend", 12 }, { "lfo_rate", 5.8f }, { "at_vib", 0.6f }, { "at_filter", 0.4f }, { "lfo_pitch", 0.0f },
            { "dl_mix", 0.28f }, { "dl_time", 340 }, { "dl_fb", 0.4f }, { "rv_mix", 0.25f }, { "ch_mix", 0.18f } } },

        { "Brass Stab", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -8 }, { "l1_lpf", 600 }, { "l1_res", 0.35f }, { "l1_amt", 0.7f },
            { "l1_fil", 0.0f }, { "l1_fa", 0.04f }, { "l1_fd", 0.35f }, { "l1_fsl", 0.45f }, { "l1_fr", 0.25f },
            { "l1_aa", 0.02f }, { "l1_ad", 0.4f }, { "l1_as", 0.8f }, { "l1_ar", 0.2f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 3 }, { "l2_detune", 9 }, { "l2_lpf", 800 }, { "l2_res", 0.3f }, { "l2_amt", 0.7f },
            { "l2_fa", 0.05f }, { "l2_fd", 0.35f }, { "l2_fsl", 0.45f }, { "l2_aa", 0.02f }, { "l2_ad", 0.4f }, { "l2_as", 0.8f }, { "l2_ar", 0.2f }, { "l2_level", 0.55f },
            { "ch_mix", 0.3f }, { "rv_mix", 0.22f }, { "dl_mix", 0.0f } } },

        { "Wind Swell", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_noise", 0.9f }, { "l1_lpf", 900 }, { "l1_res", 0.55f }, { "l1_amt", 0.5f }, { "l1_hpf", 300 },
            { "l1_fil", 0.1f }, { "l1_fa", 2.5f }, { "l1_fd", 3.0f }, { "l1_fsl", 0.5f }, { "l1_fr", 2.0f },
            { "l1_aa", 2.0f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 2.5f }, { "l1_level", 0.6f },
            { "l2_wave", 2 }, { "l2_oct", 3 }, { "l2_noise", 0.6f }, { "l2_lpf", 1400 }, { "l2_res", 0.5f }, { "l2_amt", 0.5f }, { "l2_hpf", 500 },
            { "l2_fa", 3.0f }, { "l2_fd", 3.0f }, { "l2_fsl", 0.5f }, { "l2_fr", 2.0f }, { "l2_aa", 2.5f }, { "l2_ad", 1.0f }, { "l2_as", 0.9f }, { "l2_ar", 2.5f }, { "l2_level", 0.4f },
            { "lfo_rate", 0.35f }, { "lfo_cut", 0.4f }, { "lfo_shape", 1 }, { "ch_mix", 0.4f }, { "rv_mix", 0.55f }, { "rv_size", 0.95f } } },
    };
    return p;
}
} // namespace cs
