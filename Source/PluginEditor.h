#pragma once

#include "PluginProcessor.h"
#include "ui/HS20LookAndFeel.h"
#include "ui/SynthPanel.h"

// The whole UI is laid out once at a base size and scaled with a transform,
// so every size from 50% to 200% stays pixel-sharp (everything is vector).
class HS20AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HS20AudioProcessorEditor (HS20AudioProcessor&);
    ~HS20AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    float currentScale() const { return (float) getWidth() / (float) hs20::ui::SynthPanel::baseWidth; }

    HS20AudioProcessor& processor;
    hs20::ui::HS20LookAndFeel lookAndFeel;
    hs20::ui::SynthPanel panel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HS20AudioProcessorEditor)
};
