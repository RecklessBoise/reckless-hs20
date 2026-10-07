#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace hs20::ui
{
    namespace colours
    {
        inline const juce::Colour print     { 0xffe8e4d8 };  // panel silkscreen (slightly worn ivory)
        inline const juce::Colour printDim  { 0xffa9a69c };
        inline const juce::Colour accent    { 0xffe8452c };  // Reckless red-orange
        inline const juce::Colour led       { 0xffff3b22 };
        inline const juce::Colour knobBody  { 0xff141414 };
        inline const juce::Colour darkMetal { 0xff1b1c1e };
        inline const juce::Colour glass     { 0xff070908 };

        juce::Colour cable (int index);
    }

    // Knob slider properties understood by the look-and-feel:
    //   "size"   : "big" | "medium" | "small"
    //   "ticks"  : "numbers" (0..10), "bipolar" (-5..+5 / -10..+10), "choice", "none"
    //   "labels" : choice labels separated by '|', or wave icon ids "~tri|~saw|~pulse|~noise|~square|~narrow|~ring"
    //   "ring"   : optional ARGB colour for a coloured cap (patch cable amounts)
    class HS20LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        HS20LookAndFeel();

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted, bool down) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
        void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY,
                           int buttonW, int buttonH, juce::ComboBox&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;
        juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
        void positionComboBoxText (juce::ComboBox&, juce::Label&) override;

        static juce::Font panelFont (float height, bool bold = true);
        static void drawWaveIcon (juce::Graphics&, const juce::String& id, juce::Rectangle<float> area);
        static void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius, float angle);
        static void drawLed (juce::Graphics&, juce::Point<float> centre, float radius, float brightness);
        static juce::Path heartPath (juce::Rectangle<float> area);
    };
}
