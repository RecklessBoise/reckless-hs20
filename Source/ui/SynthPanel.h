#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PatchBay.h"
#include "PresetBar.h"

class HS20AudioProcessor;

namespace hs20::ui
{
    // A knob with its printed label underneath.
    class LabelledKnob : public juce::Component
    {
    public:
        enum class Size { big, medium, small };

        LabelledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText,
                      Size size, const juce::String& ticks = "numbers", const juce::String& choiceLabels = {});

        void paint (juce::Graphics&) override;
        void resized() override;

        static int knobSize (Size s) { return s == Size::big ? 74 : (s == Size::medium ? 62 : 50); }
        int preferredHeight() const { return knobSize (size) + labelHeight; }
        juce::Point<float> getKnobCentre() const;

        juce::Slider slider;

    private:
        static constexpr int labelHeight = 12;
        Size size;
        juce::String label;
        juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    };

    // Button that cycles through the values of a choice or bool parameter.
    class ParamToggle : public juce::TextButton
    {
    public:
        ParamToggle (juce::RangedAudioParameter& param, juce::StringArray texts);

    private:
        juce::StringArray texts;
        int current = 0;
        juce::ParameterAttachment attachment;
    };

    class SynthPanel : public juce::Component, private juce::Timer
    {
    public:
        static constexpr int baseWidth = 1200;
        static constexpr int baseHeight = 680;

        explicit SynthPanel (HS20AudioProcessor& processor);
        ~SynthPanel() override;

        void paint (juce::Graphics&) override;
        void resized() override;

        std::function<void (float)> onScaleChosen;
        std::function<float()> getCurrentScale;

    private:
        struct Section
        {
            juce::String title;
            juce::Rectangle<int> bounds;
            std::vector<LabelledKnob*> knobs;
            bool horizontal = false;
        };

        void timerCallback() override;
        LabelledKnob* addKnob (const juce::String& paramId, const juce::String& label, LabelledKnob::Size,
                               const juce::String& ticks = "numbers", const juce::String& labels = {});
        void addSection (const juce::String& title, juce::Rectangle<int> bounds, std::vector<LabelledKnob*> knobs, bool horizontal = false);
        void layoutSection (const Section&);
        void paintScope (juce::Graphics&, juce::Rectangle<float>);
        void paintSignalFlow (juce::Graphics&, juce::Rectangle<float>);
        void paintLogo (juce::Graphics&, juce::Rectangle<float>);
        void showSizeMenu();

        HS20AudioProcessor& processor;
        std::vector<std::unique_ptr<LabelledKnob>> knobs;
        std::vector<Section> sections;

        PatchBay patchBay;
        PresetBar presetBar;
        std::vector<std::unique_ptr<ParamToggle>> toggles;
        juce::TextButton sizeButton;
        juce::MidiKeyboardComponent keyboard;
        juce::Slider pitchWheel, modWheel;
        juce::TooltipWindow tooltips { this, 600 };

        juce::Rectangle<float> scopeArea, mgLedPos, egLedPos, espLedPos, peakLedPos;
        juce::Image texture;
        std::array<float, 512> scopeTrace {};
    };
}
