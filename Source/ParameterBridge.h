#pragma once

#include "Parameters.h"
#include "dsp/SynthEngine.h"

namespace hs20
{
    // Caches the raw parameter pointers once and copies them into an
    // EngineParams snapshot every block (real-time safe, no lookups).
    class ParameterBridge
    {
    public:
        explicit ParameterBridge (juce::AudioProcessorValueTreeState& state);
        void read (dsp::EngineParams& out) const;

    private:
        using Raw = std::atomic<float>*;
        std::array<Raw, dsp::numSmoothed> smoothed {};
        Raw voiceMode, filterModel, quality, bendRange, legato, drift, portamento;
        Raw vco1Wave, vco1Scale, vco2Wave, vco2Scale;
        std::array<Raw, 8> eg {};
        Raw espMode, espLowCut, espHighCut, espThreshold;
        std::array<Raw, ids::numPatchCables> cableSrc {}, cableDst {};
    };
}
