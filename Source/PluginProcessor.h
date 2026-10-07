#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "ParameterBridge.h"
#include "Parameters.h"
#include "dsp/SynthEngine.h"
#include "presets/PresetManager.h"

class HS20AudioProcessor : public juce::AudioProcessor
{
public:
    HS20AudioProcessor();
    ~HS20AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    // Factory presets are exposed to hosts as programs too.
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    hs20::PresetManager presetManager;
    hs20::dsp::SynthEngine engine;
    juce::MidiKeyboardState keyboardState;

    // Editor scale, persisted with the session and as the default for new instances.
    float getUiScale() const { return uiScale; }
    void setUiScale (float newScale);

private:
    hs20::dsp::EngineParams params;
    hs20::ParameterBridge bridge { apvts };

    float uiScale = 1.0f;
    int lastLatency = -1;
    juce::AudioBuffer<float> extMono;

    JUCE_DECLARE_WEAK_REFERENCEABLE (HS20AudioProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HS20AudioProcessor)
};
