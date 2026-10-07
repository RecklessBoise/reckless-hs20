#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::File settingsFile() { return hs20::PresetManager::getDefaultDataDirectory().getChildFile ("settings.json"); }

    float loadDefaultUiScale()
    {
        const auto v = juce::JSON::parse (settingsFile());
        const auto s = (float) (double) v.getProperty ("uiScale", 1.0);
        return juce::jlimit (0.5f, 2.0f, s > 0.0f ? s : 1.0f);
    }
}

HS20AudioProcessor::HS20AudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("External Signal", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "HS20", hs20::createParameterLayout()),
      presetManager (apvts),
      uiScale (loadDefaultUiScale())
{
    // Start on the first factory preset rather than a bare init sound.
    presetManager.loadPreset (presetManager.findPreset ("factory/Classic MS/Patch 1: Dual VCO"));
}

bool HS20AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    const auto in = layouts.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void HS20AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    extMono.setSize (1, samplesPerBlock);
    keyboardState.reset();
    bridge.read (params);
    lastLatency = -1;
}

void HS20AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);
    bridge.read (params);

    // Grab the external input before the output overwrites the shared buffer.
    const float* ext = nullptr;
    const auto inputChannels = getTotalNumInputChannels();
    if (inputChannels > 0 && params.espMode != 0 && extMono.getNumSamples() >= numSamples)
    {
        extMono.copyFrom (0, 0, buffer, 0, 0, numSamples);
        if (inputChannels > 1)
        {
            extMono.addFrom (0, 0, buffer, 1, 0, numSamples);
            extMono.applyGain (0, 0, numSamples, 0.5f);
        }
        ext = extMono.getReadPointer (0);
    }

    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    auto out = getBusBuffer (buffer, false, 0);
    engine.render (out, midi, ext, params);

    const int latency = engine.getLatencySamples();
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples (latency);
    }
}

juce::AudioProcessorEditor* HS20AudioProcessor::createEditor()
{
    return new HS20AudioProcessorEditor (*this);
}

void HS20AudioProcessor::setUiScale (float newScale)
{
    uiScale = juce::jlimit (0.5f, 2.0f, newScale);

    auto* obj = new juce::DynamicObject();
    obj->setProperty ("uiScale", uiScale);
    settingsFile().getParentDirectory().createDirectory();
    settingsFile().replaceWithText (juce::JSON::toString (juce::var (obj)));
}

int HS20AudioProcessor::getNumPrograms()
{
    return juce::jmax (1, presetManager.getNumFactoryPresets());
}

int HS20AudioProcessor::getCurrentProgram()
{
    return juce::jmax (0, presetManager.getCurrentIndex());
}

void HS20AudioProcessor::setCurrentProgram (int index)
{
    if (index != presetManager.getCurrentIndex())
        presetManager.loadPreset (index);
}

const juce::String HS20AudioProcessor::getProgramName (int index)
{
    const auto& all = presetManager.getPresets();
    return juce::isPositiveAndBelow (index, (int) all.size()) ? all[(size_t) index].name : juce::String();
}

void HS20AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetKey", presetManager.getCurrentKey(), nullptr);
    state.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void HS20AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto key = tree.getProperty ("presetKey").toString();
            uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", uiScale));
            apvts.replaceState (tree);
            juce::MessageManager::callAsync ([safe = juce::WeakReference<HS20AudioProcessor> (this), key]
            {
                if (safe != nullptr)
                    safe->presetManager.restoreCurrent (key);
            });
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HS20AudioProcessor();
}
