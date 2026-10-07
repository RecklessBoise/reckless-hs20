// Renders the HS20 editor offscreen to a PNG:  HS20Snapshot <out.png> [scale] [preset key]
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginEditor.h"
#include "PluginProcessor.h"

class SnapshotApp : public juce::JUCEApplicationBase
{
public:
    const juce::String getApplicationName() override { return "HS20Snapshot"; }
    const juce::String getApplicationVersion() override { return "1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }
    void anotherInstanceStarted (const juce::String&) override {}
    void suspended() override {}
    void resumed() override {}
    void systemRequestedQuit() override { quit(); }
    void unhandledException (const std::exception*, const juce::String&, int) override {}
    void shutdown() override {}

    void initialise (const juce::String&) override
    {
        const auto args = getCommandLineParameterArray();
        out = juce::File::getCurrentWorkingDirectory().getChildFile (args.isEmpty() ? "hs20.png" : args[0]);
        const float scale = args.size() > 1 ? args[1].getFloatValue() : 1.0f;

        processor = std::make_unique<HS20AudioProcessor>();
        if (args.size() > 2)
            processor->presetManager.loadPreset (processor->presetManager.findPreset (args[2]));
        processor->setUiScale (scale);

        // play a note through the engine so the scope shows a real waveform
        processor->setPlayConfigDetails (0, 2, 48000.0, 512);
        processor->prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        for (int b = 0; b < 40; ++b)
        {
            juce::MidiBuffer midi;
            if (b == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 110), 0);
            processor->processBlock (buffer, midi);
        }

        editor.reset (processor->createEditor());
        editor->setSize (juce::roundToInt (1200 * scale), juce::roundToInt (680 * scale));

        juce::Timer::callAfterDelay (400, [this]
        {
            const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.0f);
            out.deleteFile();
            juce::FileOutputStream stream (out);
            juce::PNGImageFormat().writeImageToStream (image, stream);
            stream.flush();
            std::cout << "wrote " << out.getFullPathName() << " " << image.getWidth() << "x" << image.getHeight() << std::endl;

            editor.reset();
            processor->setUiScale (1.0f);
            processor.reset();
            setApplicationReturnValue (0);
            quit();
        });
    }

private:
    juce::File out;
    std::unique_ptr<HS20AudioProcessor> processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
};

START_JUCE_APPLICATION (SnapshotApp)
