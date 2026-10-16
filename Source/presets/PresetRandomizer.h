#pragma once

#include <juce_core/juce_core.h>

namespace hs20
{
    // Musical random patches: each roll picks a sound archetype and draws
    // every knob from a range that keeps the result playable (audible
    // filter, sane envelopes, useful patch cables) instead of pure noise.
    enum class RandomStyle { any = 0, bass, lead, pad, pluck, fx, sequence };

    struct RandomPatch
    {
        juce::String name;
        juce::String style;  // archetype that was actually used
        juce::var params;    // { paramId: value }, missing ids fall back to defaults
    };

    juce::StringArray randomStyleNames();  // index matches RandomStyle
    RandomPatch makeRandomPatch (RandomStyle style, juce::Random& rng);
}
