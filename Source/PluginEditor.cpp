#include "PluginEditor.h"

using hs20::ui::SynthPanel;

HS20AudioProcessorEditor::HS20AudioProcessorEditor (HS20AudioProcessor& p)
    : AudioProcessorEditor (&p), hs20Processor (p), panel (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (panel);

    panel.getCurrentScale = [this] { return currentScale(); };
    panel.onScaleChosen = [this] (float scale)
    {
        setSize (juce::roundToInt (SynthPanel::baseWidth * scale), juce::roundToInt (SynthPanel::baseHeight * scale));
    };

    setResizable (true, true);
    setResizeLimits (SynthPanel::baseWidth / 2, SynthPanel::baseHeight / 2, SynthPanel::baseWidth * 2, SynthPanel::baseHeight * 2);
    getConstrainer()->setFixedAspectRatio ((double) SynthPanel::baseWidth / (double) SynthPanel::baseHeight);

    const float scale = hs20Processor.getUiScale();
    setSize (juce::roundToInt (SynthPanel::baseWidth * scale), juce::roundToInt (SynthPanel::baseHeight * scale));
}

HS20AudioProcessorEditor::~HS20AudioProcessorEditor()
{
    hs20Processor.setUiScale (currentScale());
    setLookAndFeel (nullptr);
}

void HS20AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0b0b0c));
}

void HS20AudioProcessorEditor::resized()
{
    panel.setBounds (0, 0, SynthPanel::baseWidth, SynthPanel::baseHeight);
    panel.setTransform (juce::AffineTransform::scale (currentScale()));
}
