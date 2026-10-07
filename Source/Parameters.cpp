#include "Parameters.h"

namespace hs20::ids
{
    juce::String patchSrc (int cable) { return "patch" + juce::String (cable + 1) + "_src"; }
    juce::String patchDst (int cable) { return "patch" + juce::String (cable + 1) + "_dst"; }
    juce::String patchAmt (int cable) { return "patch" + juce::String (cable + 1) + "_amt"; }
}

namespace hs20
{
    juce::StringArray patchSourceNames()
    {
        return { "Off", "MG Triangle", "MG Square", "EG1", "EG1 Reverse", "EG2", "EG2 Reverse",
                 "Sample & Hold", "White Noise", "Pink Noise", "Keyboard CV", "Velocity",
                 "Mod Wheel", "ESP Envelope", "ESP F/V" };
    }

    juce::StringArray patchSourceShortNames()
    {
        return { "OFF", "MG TRI", "MG SQR", "EG1", "EG1 REV", "EG2", "EG2 REV",
                 "S&H", "WHITE", "PINK", "KBD CV", "VELO", "WHEEL", "ENV OUT", "F-V OUT" };
    }

    juce::StringArray patchDestNames()
    {
        return { "Total Pitch", "VCO2 Pitch", "VCO1 Pulse Width", "HPF Cutoff", "LPF Cutoff",
                 "HPF Peak", "LPF Peak", "VCA Gain", "MG Rate", "VCO1 Level", "VCO2 Level" };
    }

    juce::StringArray patchDestShortNames()
    {
        return { "TOTAL", "VCO2 CV", "PW", "HP FREQ", "LP FREQ",
                 "HP PEAK", "LP PEAK", "VCA", "MG RATE", "VCO1 LVL", "VCO2 LVL" };
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;
        int version = 1;

        auto knob = [&] (const String& id, const String& name, float def, float lo = 0.0f, float hi = 10.0f)
        {
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, version }, name,
                                                               NormalisableRange<float> (lo, hi, 0.0f), def));
        };
        auto choice = [&] (const String& id, const String& name, const StringArray& items, int def)
        {
            layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, version }, name, items, def));
        };

        const StringArray scales { "32'", "16'", "8'", "4'" };

        // Global
        knob (ids::volume, "Volume", 7.0f);
        choice (ids::voiceMode, "Voice Mode", { "Mono", "Poly" }, 0);
        choice (ids::filterModel, "Filter Model", { "Rev 1 (Early)", "Rev 2 (Late)" }, 0);
        choice (ids::quality, "Oversampling", { "2x", "4x" }, 0);
        layout.add (std::make_unique<AudioParameterInt> (ParameterID { ids::bendRange, version }, "Bend Range", 0, 12, 2));
        knob (ids::drift, "Analog Drift", 3.0f);
        knob (ids::drive, "Drive", 3.0f);
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { ids::legato, version }, "Single Trigger", true));
        knob (ids::portamento, "Portamento", 0.0f);
        knob (ids::masterTune, "Master Tune", 0.0f, -5.0f, 5.0f);
        knob (ids::vcaGain, "VCA Initial Gain", 0.0f);

        // VCO 1
        choice (ids::vco1Wave, "VCO1 Waveform", { "Triangle", "Sawtooth", "Pulse", "Noise" }, 1);
        knob (ids::vco1Pw, "VCO1 Pulse Width", 0.0f);
        choice (ids::vco1Scale, "VCO1 Scale", scales, 2);

        // VCO 2
        choice (ids::vco2Wave, "VCO2 Waveform", { "Sawtooth", "Square", "Narrow Pulse", "Ring Mod" }, 0);
        knob (ids::vco2Pitch, "VCO2 Pitch", 0.0f, -5.0f, 5.0f);
        choice (ids::vco2Scale, "VCO2 Scale", scales, 2);

        // Mixer + FM
        knob (ids::vco1Level, "VCO1 Level", 8.0f);
        knob (ids::vco2Level, "VCO2 Level", 0.0f);
        knob (ids::fmMg, "FM MG/T.EXT", 0.0f);
        knob (ids::fmEg1, "FM EG1/EXT", 0.0f);

        // Filters
        knob (ids::hpfCutoff, "HPF Cutoff", 0.0f);
        knob (ids::hpfPeak, "HPF Peak", 0.0f);
        knob (ids::hpfModMg, "HPF Mod MG/T.EXT", 0.0f);
        knob (ids::hpfModEg2, "HPF Mod EG2/EXT", 0.0f);
        knob (ids::lpfCutoff, "LPF Cutoff", 10.0f);
        knob (ids::lpfPeak, "LPF Peak", 0.0f);
        knob (ids::lpfModMg, "LPF Mod MG/T.EXT", 0.0f);
        knob (ids::lpfModEg2, "LPF Mod EG2/EXT", 0.0f);

        // MG + EGs
        knob (ids::mgWave, "MG Waveform", 5.0f);
        knob (ids::mgFreq, "MG Frequency", 4.0f);
        knob (ids::eg1Delay, "EG1 Delay", 0.0f);
        knob (ids::eg1Attack, "EG1 Attack", 0.0f);
        knob (ids::eg1Release, "EG1 Release", 3.0f);
        knob (ids::eg2Hold, "EG2 Hold", 0.0f);
        knob (ids::eg2Attack, "EG2 Attack", 0.0f);
        knob (ids::eg2Decay, "EG2 Decay", 5.0f);
        knob (ids::eg2Sustain, "EG2 Sustain", 10.0f);
        knob (ids::eg2Release, "EG2 Release", 3.0f);

        // ESP
        choice (ids::espMode, "ESP Mode", { "Off", "Audio > Filter", "Pitch > VCO", "Audio + Pitch" }, 0);
        knob (ids::espLevel, "ESP Signal Level", 5.0f);
        knob (ids::espLowCut, "ESP Low Cut", 0.0f);
        knob (ids::espHighCut, "ESP High Cut", 10.0f);
        knob (ids::espCvAdjust, "ESP CV Adjust", 5.0f);
        knob (ids::espThreshold, "ESP Threshold", 3.0f);

        // Patch panel cables
        for (int i = 0; i < ids::numPatchCables; ++i)
        {
            const auto n = String (i + 1);
            choice (ids::patchSrc (i), "Cable " + n + " Source", patchSourceNames(), 0);
            choice (ids::patchDst (i), "Cable " + n + " Destination", patchDestNames(), 0);
            knob (ids::patchAmt (i), "Cable " + n + " Amount", 5.0f, -10.0f, 10.0f);
        }

        return layout;
    }
}
