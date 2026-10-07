#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hs20::ui
{
    // Procedural worn, brushed-metal finish. Generated once at high
    // resolution and cached for every editor, so resizing stays smooth.
    class PanelTexture
    {
    public:
        struct Style
        {
            juce::Colour paint { 0xff202124 };    // painted panel colour
            juce::Colour metal { 0xff8d9094 };    // bare aluminium under the paint
            float wear = 1.0f;                    // how much bare metal shows through
            float scratches = 1.0f;
            juce::uint32 seed = 1;
            // Knob centres: paint gets rubbed off in rings around them.
            std::vector<juce::Point<float>> scuffs;
        };

        // Message thread only. Returns a texture covering width x height base pixels.
        static juce::Image get (int width, int height, const Style& style, float resolution = 2.0f);

    private:
        static juce::Image generate (int width, int height, const Style& style, float resolution);
    };
}
