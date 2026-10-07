#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Every automatable control of the HS20. Most knobs use the 0..10 scale
// printed on the original panel; the DSP maps them to physical units.
namespace hs20::ids
{
    // Global / performance
    inline constexpr auto volume       = "volume";
    inline constexpr auto voiceMode    = "voice_mode";
    inline constexpr auto filterModel  = "filter_model";
    inline constexpr auto quality      = "quality";
    inline constexpr auto bendRange    = "bend_range";
    inline constexpr auto drift        = "drift";
    inline constexpr auto drive        = "drive";
    inline constexpr auto legato       = "legato";
    inline constexpr auto portamento   = "portamento";
    inline constexpr auto masterTune   = "master_tune";
    inline constexpr auto vcaGain      = "vca_gain";

    // VCO 1 / VCO 2
    inline constexpr auto vco1Wave     = "vco1_wave";
    inline constexpr auto vco1Pw       = "vco1_pw";
    inline constexpr auto vco1Scale    = "vco1_scale";
    inline constexpr auto vco2Wave     = "vco2_wave";
    inline constexpr auto vco2Pitch    = "vco2_pitch";
    inline constexpr auto vco2Scale    = "vco2_scale";

    // Mixer + frequency modulation
    inline constexpr auto vco1Level    = "vco1_level";
    inline constexpr auto vco2Level    = "vco2_level";
    inline constexpr auto fmMg         = "fm_mg";
    inline constexpr auto fmEg1        = "fm_eg1";

    // Filters
    inline constexpr auto hpfCutoff    = "hpf_cutoff";
    inline constexpr auto hpfPeak      = "hpf_peak";
    inline constexpr auto hpfModMg     = "hpf_mod_mg";
    inline constexpr auto hpfModEg2    = "hpf_mod_eg2";
    inline constexpr auto lpfCutoff    = "lpf_cutoff";
    inline constexpr auto lpfPeak      = "lpf_peak";
    inline constexpr auto lpfModMg     = "lpf_mod_mg";
    inline constexpr auto lpfModEg2    = "lpf_mod_eg2";

    // Modulation generator + envelopes
    inline constexpr auto mgWave       = "mg_wave";
    inline constexpr auto mgFreq       = "mg_freq";
    inline constexpr auto eg1Delay     = "eg1_delay";
    inline constexpr auto eg1Attack    = "eg1_attack";
    inline constexpr auto eg1Release   = "eg1_release";
    inline constexpr auto eg2Hold      = "eg2_hold";
    inline constexpr auto eg2Attack    = "eg2_attack";
    inline constexpr auto eg2Decay     = "eg2_decay";
    inline constexpr auto eg2Sustain   = "eg2_sustain";
    inline constexpr auto eg2Release   = "eg2_release";

    // External signal processor
    inline constexpr auto espMode      = "esp_mode";
    inline constexpr auto espLevel     = "esp_level";
    inline constexpr auto espLowCut    = "esp_lowcut";
    inline constexpr auto espHighCut   = "esp_highcut";
    inline constexpr auto espCvAdjust  = "esp_cv_adjust";
    inline constexpr auto espThreshold = "esp_threshold";

    inline constexpr int numPatchCables = 6;
    juce::String patchSrc (int cable);  // cable is 0-based
    juce::String patchDst (int cable);
    juce::String patchAmt (int cable);
}

namespace hs20
{
    enum class PatchSource
    {
        off = 0, mgTriangle, mgSquare, eg1, eg1Rev, eg2, eg2Rev, sampleHold,
        whiteNoise, pinkNoise, keyboard, velocity, modWheel, espEnvelope, espPitch,
        count
    };

    enum class PatchDest
    {
        totalPitch = 0, vco2Pitch, vco1Pw, hpfCutoff, lpfCutoff, hpfPeak, lpfPeak,
        vcaGain, mgRate, vco1Level, vco2Level,
        count
    };

    juce::StringArray patchSourceNames();
    juce::StringArray patchSourceShortNames();
    juce::StringArray patchDestNames();
    juce::StringArray patchDestShortNames();

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
}
