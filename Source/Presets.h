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

        // =============================== Erweiterungspaket ===============================
        // ---- Brass ----
        { "Fanfare Brass", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -5 }, { "l1_lpf", 500 }, { "l1_res", 0.28f }, { "l1_amt", 0.6f },
            { "l1_fil", 0.1f }, { "l1_fa", 0.12f }, { "l1_fd", 1.2f }, { "l1_fsl", 0.6f }, { "l1_fr", 0.4f },
            { "l1_aa", 0.06f }, { "l1_ad", 0.4f }, { "l1_as", 0.95f }, { "l1_ar", 0.35f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 6 }, { "l2_lpf", 650 }, { "l2_res", 0.25f }, { "l2_amt", 0.55f },
            { "l2_fil", 0.1f }, { "l2_fa", 0.14f }, { "l2_fd", 1.2f }, { "l2_fsl", 0.6f }, { "l2_fr", 0.4f },
            { "l2_aa", 0.07f }, { "l2_ad", 0.4f }, { "l2_as", 0.95f }, { "l2_ar", 0.35f }, { "l2_level", 0.7f },
            { "at_filter", 0.4f }, { "vel_flt", 0.3f }, { "ch_mix", 0.2f }, { "rv_mix", 0.3f }, { "rv_size", 0.7f } } },

        { "Soft Horns", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_lpf", 380 }, { "l1_res", 0.15f }, { "l1_amt", 0.4f }, { "l1_brill", 0.4f },
            { "l1_fa", 0.25f }, { "l1_fd", 1.5f }, { "l1_fsl", 0.6f }, { "l1_fr", 0.5f },
            { "l1_aa", 0.18f }, { "l1_ad", 0.6f }, { "l1_as", 0.9f }, { "l1_ar", 0.5f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 8 }, { "l2_lpf", 450 }, { "l2_res", 0.15f }, { "l2_amt", 0.4f }, { "l2_brill", 0.4f },
            { "l2_fa", 0.3f }, { "l2_fd", 1.5f }, { "l2_fsl", 0.6f }, { "l2_fr", 0.5f },
            { "l2_aa", 0.2f }, { "l2_ad", 0.6f }, { "l2_as", 0.9f }, { "l2_ar", 0.5f }, { "l2_level", 0.55f },
            { "lfo_rate", 5.2f }, { "lfo_pitch", 0.04f }, { "ch_mix", 0.3f }, { "rv_mix", 0.35f }, { "rv_size", 0.75f } } },

        { "Synth Trumpet", {
            { "l1_wave", 1 }, { "l1_oct", 2 }, { "l1_lpf", 900 }, { "l1_res", 0.4f }, { "l1_amt", 0.6f }, { "l1_hpf", 150 },
            { "l1_fil", 0.2f }, { "l1_fa", 0.05f }, { "l1_fd", 0.5f }, { "l1_fsl", 0.6f }, { "l1_fr", 0.25f },
            { "l1_aa", 0.04f }, { "l1_ad", 0.3f }, { "l1_as", 0.9f }, { "l1_ar", 0.2f }, { "l1_level", 0.75f },
            { "l2_wave", 0 }, { "l2_oct", 3 }, { "l2_detune", 5 }, { "l2_lpf", 1400 }, { "l2_res", 0.3f }, { "l2_amt", 0.5f }, { "l2_hpf", 300 },
            { "l2_fa", 0.06f }, { "l2_fd", 0.5f }, { "l2_fsl", 0.6f }, { "l2_fr", 0.25f },
            { "l2_aa", 0.05f }, { "l2_ad", 0.3f }, { "l2_as", 0.9f }, { "l2_ar", 0.2f }, { "l2_level", 0.3f },
            { "at_vib", 0.5f }, { "vel_flt", 0.4f }, { "rv_mix", 0.22f }, { "dl_mix", 0.1f }, { "dl_time", 360 } } },

        { "Big Brass Section", {
            { "l1_wave", 0 }, { "l1_oct", 1 }, { "l1_detune", -6 }, { "l1_lpf", 450 }, { "l1_res", 0.25f }, { "l1_amt", 0.6f },
            { "l1_fil", 0.1f }, { "l1_fa", 0.18f }, { "l1_fd", 1.5f }, { "l1_fsl", 0.55f }, { "l1_fr", 0.5f },
            { "l1_aa", 0.08f }, { "l1_ad", 0.5f }, { "l1_as", 0.95f }, { "l1_ar", 0.45f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 7 }, { "l2_lpf", 700 }, { "l2_res", 0.25f }, { "l2_amt", 0.55f },
            { "l2_fil", 0.1f }, { "l2_fa", 0.2f }, { "l2_fd", 1.5f }, { "l2_fsl", 0.55f }, { "l2_fr", 0.5f },
            { "l2_aa", 0.09f }, { "l2_ad", 0.5f }, { "l2_as", 0.95f }, { "l2_ar", 0.45f }, { "l2_level", 0.75f },
            { "at_filter", 0.35f }, { "ch_mix", 0.25f }, { "rv_mix", 0.4f }, { "rv_size", 0.8f } } },

        // ---- Strings ----
        { "Slow Strings", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -10 }, { "l1_lpf", 2200 }, { "l1_res", 0.08f }, { "l1_amt", 0.12f },
            { "l1_fa", 0.8f }, { "l1_fd", 1.0f }, { "l1_fsl", 0.85f }, { "l1_fr", 1.0f },
            { "l1_aa", 0.9f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 1.2f }, { "l1_level", 0.7f },
            { "l2_wave", 0 }, { "l2_oct", 3 }, { "l2_detune", 12 }, { "l2_lpf", 2800 }, { "l2_res", 0.08f }, { "l2_amt", 0.12f },
            { "l2_fa", 1.0f }, { "l2_fd", 1.0f }, { "l2_fsl", 0.85f }, { "l2_fr", 1.0f },
            { "l2_aa", 1.0f }, { "l2_ad", 1.0f }, { "l2_as", 0.9f }, { "l2_ar", 1.4f }, { "l2_level", 0.45f },
            { "lfo_rate", 5.2f }, { "lfo_pitch", 0.05f }, { "ch_mix", 0.65f }, { "ch_depth", 0.7f }, { "ch_rate", 0.5f }, { "rv_mix", 0.45f }, { "rv_size", 0.85f } } },

        { "Ensemble 80", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -14 }, { "l1_lpf", 3500 }, { "l1_res", 0.05f }, { "l1_amt", 0.1f },
            { "l1_fa", 0.4f }, { "l1_fd", 1.0f }, { "l1_fsl", 0.9f }, { "l1_fr", 0.7f },
            { "l1_aa", 0.35f }, { "l1_ad", 0.8f }, { "l1_as", 0.9f }, { "l1_ar", 0.7f }, { "l1_level", 0.6f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 14 }, { "l2_lpf", 3500 }, { "l2_res", 0.05f }, { "l2_amt", 0.1f },
            { "l2_fa", 0.4f }, { "l2_fd", 1.0f }, { "l2_fsl", 0.9f }, { "l2_fr", 0.7f },
            { "l2_aa", 0.35f }, { "l2_ad", 0.8f }, { "l2_as", 0.9f }, { "l2_ar", 0.7f }, { "l2_level", 0.6f },
            { "lfo_rate", 5.0f }, { "lfo_pitch", 0.05f }, { "ch_mix", 0.75f }, { "ch_depth", 0.8f }, { "rv_mix", 0.3f }, { "rv_size", 0.7f } } },

        { "Cello Layer", {
            { "l1_wave", 0 }, { "l1_oct", 1 }, { "l1_lpf", 900 }, { "l1_res", 0.15f }, { "l1_amt", 0.3f }, { "l1_key", 0.4f },
            { "l1_fa", 0.3f }, { "l1_fd", 1.0f }, { "l1_fsl", 0.7f }, { "l1_fr", 0.5f },
            { "l1_aa", 0.25f }, { "l1_ad", 0.6f }, { "l1_as", 0.9f }, { "l1_ar", 0.5f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 6 }, { "l2_lpf", 1300 }, { "l2_res", 0.15f }, { "l2_amt", 0.3f }, { "l2_key", 0.4f },
            { "l2_fa", 0.3f }, { "l2_fd", 1.0f }, { "l2_fsl", 0.7f }, { "l2_fr", 0.5f },
            { "l2_aa", 0.3f }, { "l2_ad", 0.6f }, { "l2_as", 0.9f }, { "l2_ar", 0.5f }, { "l2_level", 0.5f },
            { "lfo_rate", 5.5f }, { "lfo_pitch", 0.08f }, { "at_vib", 0.5f }, { "ch_mix", 0.3f }, { "rv_mix", 0.3f } } },

        // ---- Pads ----
        { "Choir Pad", {
            { "l1_wave", 2 }, { "l1_pw", 0.4f }, { "l1_oct", 2 }, { "l1_hpf", 150 }, { "l1_lpf", 1400 }, { "l1_res", 0.3f }, { "l1_amt", 0.2f }, { "l1_brill", 0.4f },
            { "l1_fa", 1.0f }, { "l1_fd", 1.5f }, { "l1_fsl", 0.7f }, { "l1_fr", 1.2f },
            { "l1_aa", 1.0f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 1.5f }, { "l1_level", 0.7f },
            { "l2_wave", 2 }, { "l2_pw", 0.6f }, { "l2_oct", 3 }, { "l2_detune", 7 }, { "l2_hpf", 250 }, { "l2_lpf", 1800 }, { "l2_res", 0.3f }, { "l2_amt", 0.2f },
            { "l2_fa", 1.2f }, { "l2_fd", 1.5f }, { "l2_fsl", 0.7f }, { "l2_fr", 1.2f },
            { "l2_aa", 1.2f }, { "l2_ad", 1.0f }, { "l2_as", 0.9f }, { "l2_ar", 1.6f }, { "l2_level", 0.5f },
            { "lfo_rate", 0.5f }, { "lfo_shape", 1 }, { "lfo_pwm", 0.5f }, { "ch_mix", 0.5f }, { "rv_mix", 0.6f }, { "rv_size", 0.9f } } },

        { "Dark Pad", {
            { "l1_wave", 0 }, { "l1_oct", 1 }, { "l1_lpf", 600 }, { "l1_res", 0.2f }, { "l1_amt", 0.35f },
            { "l1_fa", 2.0f }, { "l1_fd", 3.0f }, { "l1_fsl", 0.5f }, { "l1_fr", 2.0f },
            { "l1_aa", 1.6f }, { "l1_ad", 1.5f }, { "l1_as", 0.9f }, { "l1_ar", 2.0f }, { "l1_level", 0.75f },
            { "l2_wave", 2 }, { "l2_pw", 0.45f }, { "l2_oct", 2 }, { "l2_detune", -9 }, { "l2_lpf", 800 }, { "l2_res", 0.2f }, { "l2_amt", 0.35f },
            { "l2_fa", 2.2f }, { "l2_fd", 3.0f }, { "l2_fsl", 0.5f }, { "l2_fr", 2.0f },
            { "l2_aa", 1.8f }, { "l2_ad", 1.5f }, { "l2_as", 0.9f }, { "l2_ar", 2.2f }, { "l2_level", 0.45f },
            { "lfo_rate", 0.25f }, { "lfo_shape", 1 }, { "lfo_cut", 0.2f }, { "ch_mix", 0.4f }, { "rv_mix", 0.55f }, { "rv_size", 0.9f }, { "dl_mix", 0.12f } } },

        { "Aurora Pad", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_detune", -8 }, { "l1_hpf", 100 }, { "l1_lpf", 1200 }, { "l1_res", 0.3f }, { "l1_amt", 0.5f },
            { "l1_fil", 0.05f }, { "l1_fa", 2.5f }, { "l1_fd", 3.0f }, { "l1_fsl", 0.4f }, { "l1_fr", 2.0f },
            { "l1_aa", 1.5f }, { "l1_ad", 1.5f }, { "l1_as", 0.9f }, { "l1_ar", 2.2f }, { "l1_level", 0.7f },
            { "l2_wave", 2 }, { "l2_pw", 0.3f }, { "l2_oct", 3 }, { "l2_detune", 10 }, { "l2_lpf", 2500 }, { "l2_res", 0.25f }, { "l2_amt", 0.45f },
            { "l2_fa", 3.0f }, { "l2_fd", 3.0f }, { "l2_fsl", 0.4f }, { "l2_fr", 2.0f },
            { "l2_aa", 1.8f }, { "l2_ad", 1.5f }, { "l2_as", 0.9f }, { "l2_ar", 2.4f }, { "l2_level", 0.5f },
            { "lfo_rate", 0.4f }, { "lfo_shape", 1 }, { "lfo_pwm", 0.7f }, { "lfo_cut", 0.15f },
            { "ch_mix", 0.55f }, { "ch_rate", 0.35f }, { "dl_mix", 0.22f }, { "dl_time", 620 }, { "dl_fb", 0.45f }, { "rv_mix", 0.5f }, { "rv_size", 0.9f } } },

        { "Analog Air", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_noise", 0.25f }, { "l1_hpf", 400 }, { "l1_lpf", 2000 }, { "l1_res", 0.4f }, { "l1_amt", 0.4f },
            { "l1_fa", 1.2f }, { "l1_fd", 2.0f }, { "l1_fsl", 0.5f }, { "l1_fr", 1.5f },
            { "l1_aa", 1.0f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 1.5f }, { "l1_level", 0.7f },
            { "l2_wave", 2 }, { "l2_pw", 0.5f }, { "l2_oct", 3 }, { "l2_noise", 0.3f }, { "l2_hpf", 600 }, { "l2_lpf", 3000 }, { "l2_res", 0.35f }, { "l2_amt", 0.4f },
            { "l2_fa", 1.4f }, { "l2_fd", 2.0f }, { "l2_fsl", 0.5f }, { "l2_fr", 1.5f },
            { "l2_aa", 1.2f }, { "l2_ad", 1.0f }, { "l2_as", 0.9f }, { "l2_ar", 1.6f }, { "l2_level", 0.4f },
            { "at_filter", 0.5f }, { "ch_mix", 0.5f }, { "rv_mix", 0.5f }, { "rv_size", 0.85f } } },

        // ---- Bass ----
        { "Fat Bass", {
            { "l1_wave", 0 }, { "l1_oct", 1 }, { "l1_sub", 0.5f }, { "l1_lpf", 350 }, { "l1_res", 0.3f }, { "l1_amt", 0.5f }, { "l1_key", 0.2f },
            { "l1_fil", 0.0f }, { "l1_fa", 0.004f }, { "l1_fd", 0.3f }, { "l1_fsl", 0.2f }, { "l1_fr", 0.12f },
            { "l1_aa", 0.003f }, { "l1_ad", 0.3f }, { "l1_as", 0.85f }, { "l1_ar", 0.1f }, { "l1_level", 0.9f },
            { "l2_wave", 1 }, { "l2_oct", 1 }, { "l2_lpf", 300 }, { "l2_res", 0.2f }, { "l2_amt", 0.4f },
            { "l2_fa", 0.004f }, { "l2_fd", 0.3f }, { "l2_fsl", 0.2f }, { "l2_fr", 0.12f },
            { "l2_aa", 0.003f }, { "l2_ad", 0.3f }, { "l2_as", 0.85f }, { "l2_ar", 0.1f }, { "l2_level", 0.4f },
            { "ch_mix", 0.0f }, { "rv_mix", 0.05f }, { "master", 0.65f } } },

        { "Rubber Bass", {
            { "l1_wave", 2 }, { "l1_pw", 0.3f }, { "l1_oct", 1 }, { "l1_lpf", 220 }, { "l1_res", 0.55f }, { "l1_amt", 0.65f }, { "l1_key", 0.3f },
            { "l1_fil", 0.0f }, { "l1_fa", 0.003f }, { "l1_fd", 0.22f }, { "l1_fsl", 0.1f }, { "l1_fr", 0.1f },
            { "l1_aa", 0.003f }, { "l1_ad", 0.35f }, { "l1_as", 0.7f }, { "l1_ar", 0.1f }, { "l1_level", 0.9f },
            { "l2_level", 0.0f }, { "glide", 0.05f }, { "vel_flt", 0.4f }, { "ch_mix", 0.0f }, { "rv_mix", 0.04f }, { "master", 0.65f } } },

        { "Pulse Bass", {
            { "l1_wave", 1 }, { "l1_oct", 1 }, { "l1_lpf", 500 }, { "l1_res", 0.3f }, { "l1_amt", 0.4f }, { "l1_key", 0.3f },
            { "l1_fa", 0.003f }, { "l1_fd", 0.2f }, { "l1_fsl", 0.3f }, { "l1_fr", 0.1f },
            { "l1_aa", 0.003f }, { "l1_ad", 0.3f }, { "l1_as", 0.8f }, { "l1_ar", 0.1f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 1 }, { "l2_detune", 5 }, { "l2_lpf", 400 }, { "l2_res", 0.3f }, { "l2_amt", 0.4f }, { "l2_key", 0.3f },
            { "l2_fa", 0.003f }, { "l2_fd", 0.2f }, { "l2_fsl", 0.3f }, { "l2_fr", 0.1f },
            { "l2_aa", 0.003f }, { "l2_ad", 0.3f }, { "l2_as", 0.8f }, { "l2_ar", 0.1f }, { "l2_level", 0.5f },
            { "ch_mix", 0.15f }, { "rv_mix", 0.08f }, { "master", 0.65f } } },

        { "Deep Sub", {
            { "l1_wave", 1 }, { "l1_oct", 1 }, { "l1_sub", 0.8f }, { "l1_lpf", 160 }, { "l1_res", 0.05f }, { "l1_amt", 0.1f }, { "l1_key", 0.2f },
            { "l1_fa", 0.01f }, { "l1_fd", 0.3f }, { "l1_fsl", 0.8f }, { "l1_fr", 0.2f },
            { "l1_aa", 0.01f }, { "l1_ad", 0.3f }, { "l1_as", 0.95f }, { "l1_ar", 0.2f }, { "l1_level", 0.95f },
            { "l2_level", 0.0f }, { "ch_mix", 0.0f }, { "rv_mix", 0.0f }, { "master", 0.6f } } },

        // ---- Leads ----
        { "Solo Cry", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_lpf", 1600 }, { "l1_res", 0.35f }, { "l1_amt", 0.35f },
            { "l1_fa", 0.03f }, { "l1_fd", 0.5f }, { "l1_fsl", 0.7f }, { "l1_fr", 0.3f },
            { "l1_aa", 0.03f }, { "l1_ad", 0.3f }, { "l1_as", 0.9f }, { "l1_ar", 0.3f }, { "l1_level", 0.75f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 8 }, { "l2_lpf", 1800 }, { "l2_res", 0.3f }, { "l2_amt", 0.35f },
            { "l2_fa", 0.03f }, { "l2_fd", 0.5f }, { "l2_fsl", 0.7f }, { "l2_fr", 0.3f },
            { "l2_aa", 0.03f }, { "l2_ad", 0.3f }, { "l2_as", 0.9f }, { "l2_ar", 0.3f }, { "l2_level", 0.4f },
            { "glide", 0.12f }, { "bend", 12 }, { "at_vib", 0.7f }, { "at_filter", 0.4f }, { "lfo_rate", 5.8f },
            { "dl_mix", 0.25f }, { "dl_time", 400 }, { "dl_fb", 0.4f }, { "rv_mix", 0.3f }, { "ch_mix", 0.15f } } },

        { "Pulse Lead", {
            { "l1_wave", 2 }, { "l1_pw", 0.25f }, { "l1_oct", 2 }, { "l1_lpf", 2500 }, { "l1_res", 0.3f }, { "l1_amt", 0.4f },
            { "l1_fa", 0.01f }, { "l1_fd", 0.4f }, { "l1_fsl", 0.6f }, { "l1_fr", 0.25f },
            { "l1_aa", 0.01f }, { "l1_ad", 0.3f }, { "l1_as", 0.9f }, { "l1_ar", 0.25f }, { "l1_level", 0.75f },
            { "l2_wave", 2 }, { "l2_pw", 0.75f }, { "l2_oct", 3 }, { "l2_detune", 6 }, { "l2_lpf", 3000 }, { "l2_res", 0.3f }, { "l2_amt", 0.4f },
            { "l2_fa", 0.01f }, { "l2_fd", 0.4f }, { "l2_fsl", 0.6f }, { "l2_fr", 0.25f },
            { "l2_aa", 0.01f }, { "l2_ad", 0.3f }, { "l2_as", 0.9f }, { "l2_ar", 0.25f }, { "l2_level", 0.3f },
            { "lfo_rate", 3.0f }, { "lfo_shape", 1 }, { "lfo_pwm", 0.4f }, { "glide", 0.05f },
            { "dl_mix", 0.2f }, { "dl_time", 350 }, { "rv_mix", 0.25f } } },

        { "Smooth Lead", {
            { "l1_wave", 1 }, { "l1_oct", 2 }, { "l1_lpf", 1800 }, { "l1_res", 0.2f }, { "l1_amt", 0.25f }, { "l1_brill", 0.4f },
            { "l1_fa", 0.02f }, { "l1_fd", 0.5f }, { "l1_fsl", 0.7f }, { "l1_fr", 0.3f },
            { "l1_aa", 0.02f }, { "l1_ad", 0.3f }, { "l1_as", 0.9f }, { "l1_ar", 0.3f }, { "l1_level", 0.75f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 5 }, { "l2_lpf", 2000 }, { "l2_res", 0.2f }, { "l2_amt", 0.25f },
            { "l2_fa", 0.02f }, { "l2_fd", 0.5f }, { "l2_fsl", 0.7f }, { "l2_fr", 0.3f },
            { "l2_aa", 0.02f }, { "l2_ad", 0.3f }, { "l2_as", 0.9f }, { "l2_ar", 0.3f }, { "l2_level", 0.35f },
            { "glide", 0.15f }, { "lfo_rate", 5.5f }, { "at_vib", 0.6f },
            { "ch_mix", 0.2f }, { "dl_mix", 0.18f }, { "dl_time", 380 }, { "rv_mix", 0.28f } } },

        // ---- Keys / Plucks ----
        { "Electric Pluck", {
            { "l1_wave", 2 }, { "l1_pw", 0.35f }, { "l1_oct", 2 }, { "l1_lpf", 700 }, { "l1_res", 0.35f }, { "l1_amt", 0.7f },
            { "l1_fil", 0.0f }, { "l1_fa", 0.002f }, { "l1_fd", 0.25f }, { "l1_fsl", 0.05f }, { "l1_fr", 0.2f },
            { "l1_aa", 0.002f }, { "l1_ad", 0.5f }, { "l1_as", 0.1f }, { "l1_ar", 0.25f }, { "l1_level", 0.85f },
            { "l2_wave", 0 }, { "l2_oct", 3 }, { "l2_lpf", 1500 }, { "l2_res", 0.25f }, { "l2_amt", 0.5f },
            { "l2_fa", 0.002f }, { "l2_fd", 0.25f }, { "l2_fsl", 0.05f }, { "l2_fr", 0.2f },
            { "l2_aa", 0.002f }, { "l2_ad", 0.5f }, { "l2_as", 0.05f }, { "l2_ar", 0.25f }, { "l2_level", 0.4f },
            { "ch_mix", 0.3f }, { "dl_mix", 0.2f }, { "dl_time", 300 }, { "dl_fb", 0.3f }, { "rv_mix", 0.3f } } },

        { "Glass Keys", {
            { "l1_wave", 1 }, { "l1_oct", 3 }, { "l1_lpf", 3500 }, { "l1_res", 0.15f }, { "l1_amt", 0.3f },
            { "l1_fa", 0.002f }, { "l1_fd", 0.35f }, { "l1_fsl", 0.1f }, { "l1_fr", 0.3f },
            { "l1_aa", 0.002f }, { "l1_ad", 0.6f }, { "l1_as", 0.15f }, { "l1_ar", 0.5f }, { "l1_level", 0.8f },
            { "l2_wave", 0 }, { "l2_oct", 4 }, { "l2_lpf", 5000 }, { "l2_res", 0.1f }, { "l2_amt", 0.3f },
            { "l2_fa", 0.002f }, { "l2_fd", 0.35f }, { "l2_fsl", 0.1f }, { "l2_fr", 0.3f },
            { "l2_aa", 0.002f }, { "l2_ad", 0.6f }, { "l2_as", 0.15f }, { "l2_ar", 0.5f }, { "l2_level", 0.2f },
            { "ch_mix", 0.4f }, { "dl_mix", 0.15f }, { "rv_mix", 0.45f }, { "rv_size", 0.8f } } },

        { "Clav Stab", {
            { "l1_wave", 1 }, { "l1_oct", 2 }, { "l1_hpf", 500 }, { "l1_lpf", 1200 }, { "l1_res", 0.45f }, { "l1_amt", 0.6f },
            { "l1_fa", 0.001f }, { "l1_fd", 0.2f }, { "l1_fsl", 0.0f }, { "l1_fr", 0.08f },
            { "l1_aa", 0.001f }, { "l1_ad", 0.22f }, { "l1_as", 0.0f }, { "l1_ar", 0.1f }, { "l1_level", 1.0f },
            { "l2_wave", 2 }, { "l2_pw", 0.2f }, { "l2_oct", 2 }, { "l2_detune", 4 }, { "l2_hpf", 600 }, { "l2_lpf", 1400 }, { "l2_res", 0.4f }, { "l2_amt", 0.6f },
            { "l2_fa", 0.001f }, { "l2_fd", 0.2f }, { "l2_fsl", 0.0f }, { "l2_fr", 0.08f },
            { "l2_aa", 0.001f }, { "l2_ad", 0.22f }, { "l2_as", 0.0f }, { "l2_ar", 0.1f }, { "l2_level", 0.5f },
            { "vel_flt", 0.5f }, { "ch_mix", 0.0f }, { "rv_mix", 0.1f }, { "master", 0.85f } } },

        // ---- Effekte / Flächen ----
        { "Sweep FX", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_noise", 0.4f }, { "l1_lpf", 300 }, { "l1_res", 0.6f }, { "l1_amt", 0.9f },
            { "l1_fa", 4.0f }, { "l1_fd", 4.0f }, { "l1_fsl", 0.3f }, { "l1_fr", 2.0f },
            { "l1_aa", 2.0f }, { "l1_ad", 1.5f }, { "l1_as", 0.9f }, { "l1_ar", 2.0f }, { "l1_level", 0.65f },
            { "l2_wave", 2 }, { "l2_oct", 3 }, { "l2_lpf", 400 }, { "l2_res", 0.55f }, { "l2_amt", 0.9f },
            { "l2_fa", 5.0f }, { "l2_fd", 4.0f }, { "l2_fsl", 0.3f }, { "l2_fr", 2.0f },
            { "l2_aa", 2.5f }, { "l2_ad", 1.5f }, { "l2_as", 0.9f }, { "l2_ar", 2.0f }, { "l2_level", 0.4f },
            { "ch_mix", 0.5f }, { "dl_mix", 0.25f }, { "dl_time", 500 }, { "dl_fb", 0.45f }, { "rv_mix", 0.5f }, { "rv_size", 0.85f } } },

        { "Wobble Pad", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_lpf", 800 }, { "l1_res", 0.5f }, { "l1_amt", 0.1f },
            { "l1_aa", 0.8f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 1.0f }, { "l1_level", 0.7f },
            { "l2_wave", 0 }, { "l2_oct", 2 }, { "l2_detune", 10 }, { "l2_lpf", 1000 }, { "l2_res", 0.45f }, { "l2_amt", 0.1f },
            { "l2_aa", 0.9f }, { "l2_ad", 1.0f }, { "l2_as", 0.9f }, { "l2_ar", 1.1f }, { "l2_level", 0.5f },
            { "lfo_rate", 2.0f }, { "lfo_shape", 3 }, { "lfo_cut", 0.6f }, { "ch_mix", 0.4f }, { "rv_mix", 0.4f }, { "rv_size", 0.8f } } },

        { "Space Drone", {
            { "l1_wave", 0 }, { "l1_oct", 1 }, { "l1_sub", 0.4f }, { "l1_noise", 0.1f }, { "l1_lpf", 500 }, { "l1_res", 0.4f }, { "l1_amt", 0.3f },
            { "l1_fa", 3.0f }, { "l1_fd", 3.0f }, { "l1_fsl", 0.6f }, { "l1_fr", 2.5f },
            { "l1_aa", 3.0f }, { "l1_ad", 1.5f }, { "l1_as", 0.9f }, { "l1_ar", 2.5f }, { "l1_level", 0.7f },
            { "l2_wave", 2 }, { "l2_pw", 0.4f }, { "l2_oct", 2 }, { "l2_detune", 15 }, { "l2_lpf", 700 }, { "l2_res", 0.35f }, { "l2_amt", 0.3f },
            { "l2_fa", 3.5f }, { "l2_fd", 3.0f }, { "l2_fsl", 0.6f }, { "l2_fr", 2.5f },
            { "l2_aa", 3.5f }, { "l2_ad", 1.5f }, { "l2_as", 0.9f }, { "l2_ar", 2.5f }, { "l2_level", 0.5f },
            { "lfo_rate", 0.15f }, { "lfo_shape", 1 }, { "lfo_pwm", 0.8f }, { "lfo_cut", 0.3f },
            { "ch_mix", 0.45f }, { "dl_mix", 0.28f }, { "dl_time", 700 }, { "dl_fb", 0.55f }, { "rv_mix", 0.5f }, { "rv_size", 0.9f } } },

        { "Noise Rise", {
            { "l1_wave", 0 }, { "l1_oct", 2 }, { "l1_noise", 1.0f }, { "l1_hpf", 500 }, { "l1_lpf", 400 }, { "l1_res", 0.5f }, { "l1_amt", 1.0f },
            { "l1_fa", 6.0f }, { "l1_fd", 3.0f }, { "l1_fsl", 0.6f }, { "l1_fr", 1.5f },
            { "l1_aa", 3.0f }, { "l1_ad", 1.0f }, { "l1_as", 0.9f }, { "l1_ar", 1.5f }, { "l1_level", 0.6f },
            { "l2_level", 0.0f }, { "ch_mix", 0.3f }, { "rv_mix", 0.4f }, { "rv_size", 0.8f } } },
    };
    return p;
}
} // namespace cs
