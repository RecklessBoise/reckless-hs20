// Headless checks for the Reckless HS20 engine and preset system.
//   HS20Tests                 run every unit test
//   HS20Tests --levels FILE   also write each factory preset's RMS (dBFS) to FILE
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ParameterBridge.h"
#include "Parameters.h"
#include "dsp/SynthEngine.h"
#include "presets/PresetManager.h"
#include "presets/PresetRandomizer.h"

namespace
{
    // Minimal processor so the real parameter layout and preset manager can be used.
    class TestProcessor : public juce::AudioProcessor
    {
    public:
        TestProcessor() : apvts (*this, nullptr, "HS20", hs20::createParameterLayout()) {}
        const juce::String getName() const override { return "Test"; }
        void prepareToPlay (double, int) override {}
        void releaseResources() override {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override { return 0; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        juce::AudioProcessorEditor* createEditor() override { return nullptr; }
        bool hasEditor() const override { return false; }
        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override {}
        void setStateInformation (const void*, int) override {}

        juce::AudioProcessorValueTreeState apvts;
    };

    void setParam (juce::AudioProcessorValueTreeState& s, const juce::String& id, float value)
    {
        auto* p = s.getParameter (id);
        jassert (p != nullptr);
        p->setValueNotifyingHost (p->convertTo0to1 (value));
    }

    struct RenderStats
    {
        float peak = 0, rmsDb = -200, loudestWindowDb = -200;
        bool finite = true;
        std::vector<float> samples;
    };

    // Plays a note (or nothing) and returns statistics of the mono output.
    RenderStats render (juce::AudioProcessorValueTreeState& state, int note, double seconds, double holdSeconds,
                        double sampleRate = 48000.0, const std::function<void (juce::AudioBuffer<float>&)>& input = {})
    {
        hs20::dsp::SynthEngine engine;
        hs20::ParameterBridge bridge (state);
        hs20::dsp::EngineParams params;
        const int block = 256;
        engine.prepare (sampleRate, block);

        juce::AudioBuffer<float> buffer (2, block), in (1, block);
        RenderStats stats;
        double sumSq = 0;
        const int total = (int) (seconds * sampleRate), noteOffAt = (int) (holdSeconds * sampleRate);

        for (int pos = 0; pos < total; pos += block)
        {
            bridge.read (params);
            juce::MidiBuffer midi;
            if (note >= 0 && pos == 0)
                midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
            if (note >= 0 && pos <= noteOffAt && noteOffAt < pos + block)
                midi.addEvent (juce::MidiMessage::noteOff (1, note), noteOffAt - pos);

            buffer.clear();
            const float* ext = nullptr;
            if (input)
            {
                input (in);
                ext = in.getReadPointer (0);
            }
            engine.render (buffer, midi, ext, params);

            for (int i = 0; i < block; ++i)
            {
                const float v = buffer.getSample (0, i);
                stats.finite &= std::isfinite (v);
                stats.peak = std::max (stats.peak, std::abs (v));
                sumSq += (double) v * v;
                stats.samples.push_back (v);
            }
        }

        const double rms = std::sqrt (sumSq / std::max (1, total));
        stats.rmsDb = (float) (20.0 * std::log10 (rms + 1.0e-12));

        // loudest 50 ms window: a playing-level measure that is fair to percussive sounds
        const size_t win = (size_t) (0.05 * sampleRate);
        for (size_t start = 0; start + win <= stats.samples.size(); start += win / 2)
        {
            double w = 0;
            for (size_t i = start; i < start + win; ++i)
                w += (double) stats.samples[i] * stats.samples[i];
            stats.loudestWindowDb = std::max (stats.loudestWindowDb, (float) (10.0 * std::log10 (w / (double) win + 1.0e-24)));
        }
        return stats;
    }

    double estimateFrequency (const std::vector<float>& x, double sampleRate, size_t skip)
    {
        int first = -1, last = -1, count = 0;
        for (size_t i = skip + 1; i < x.size(); ++i)
            if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            {
                if (first < 0) first = (int) i;
                last = (int) i;
                ++count;
            }
        return count > 1 ? (count - 1) * sampleRate / (double) (last - first) : 0.0;
    }

    juce::File makeTempDir()
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("hs20-tests-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
        dir.createDirectory();
        return dir;
    }

    void resetAll (juce::AudioProcessorValueTreeState& s)
    {
        for (auto* p : s.processor.getParameters())
            p->setValueNotifyingHost (p->getDefaultValue());
    }
}

//==============================================================================
class FilterTests : public juce::UnitTest
{
public:
    FilterTests() : juce::UnitTest ("MS-20 filter", "HS20") {}

    void runTest() override
    {
        using hs20::dsp::MS20Filter;
        const float fs = 96000.0f;

        beginTest ("Lowpass attenuates above cutoff (12 dB/oct)");
        {
            auto gainAt = [&] (float freq)
            {
                MS20Filter f (MS20Filter::Mode::lowpass);
                f.set (1000.0f, fs, 0.01f, 1.0f);
                double sum = 0;
                for (int n = 0; n < 40000; ++n)
                {
                    const float y = f.process (0.1f * std::sin (2.0f * hs20::dsp::pi * freq * (float) n / fs));
                    if (n > 20000) sum += (double) y * y;
                }
                return 20.0 * std::log10 (std::sqrt (sum / 19999.0) / (0.1 / std::sqrt (2.0)));
            };
            const double g4k = gainAt (4000.0f), g8k = gainAt (8000.0f);
            expectWithinAbsoluteError (gainAt (100.0f), 0.0, 1.0);
            expectWithinAbsoluteError (g4k - g8k, 12.0, 1.5);
        }

        for (auto mode : { MS20Filter::Mode::lowpass, MS20Filter::Mode::highpass })
        {
            beginTest (juce::String (mode == MS20Filter::Mode::lowpass ? "Lowpass" : "Highpass") + " self-oscillates at cutoff");
            MS20Filter f (mode);
            f.set (880.0f, fs, 2.45f, 1.0f);
            std::vector<float> out;
            for (int n = 0; n < (int) fs; ++n)
                out.push_back (f.process (n == 0 ? 1.0e-3f : 0.0f));
            float peak = 0;
            for (size_t i = out.size() / 2; i < out.size(); ++i)
                peak = std::max (peak, std::abs (out[i]));
            expect (peak > 0.05f && peak < 2.0f, "oscillation should be bounded and audible, peak " + juce::String (peak));
            expectWithinAbsoluteError (estimateFrequency (out, fs, out.size() / 2), 880.0, 15.0);
        }
    }
};

class EngineTests : public juce::UnitTest
{
public:
    EngineTests() : juce::UnitTest ("Synth engine", "HS20") {}

    void runTest() override
    {
        TestProcessor proc;
        auto& s = proc.apvts;

        beginTest ("VCO tuning: A4 = 440 Hz on the 8' scale");
        resetAll (s);
        setParam (s, hs20::ids::drift, 0.0f);
        setParam (s, hs20::ids::vco1Wave, 0.0f);  // triangle: clean zero crossings
        auto a4 = render (s, 69, 1.0, 1.0);
        const double f = estimateFrequency (a4.samples, 48000.0, 4800);
        expect (std::abs (1200.0 * std::log2 (f / 440.0)) < 2.0, "measured " + juce::String (f) + " Hz");

        beginTest ("Scale switch transposes by octaves");
        setParam (s, hs20::ids::vco1Scale, 1.0f);  // 16'
        const double f16 = estimateFrequency (render (s, 69, 1.0, 1.0).samples, 48000.0, 4800);
        expectWithinAbsoluteError (f16, 220.0, 1.0);

        beginTest ("Silence when no note is played");
        resetAll (s);
        expect (render (s, -1, 0.5, 0.0).peak < 1.0e-3f);

        beginTest ("Envelope releases to silence");
        auto rel = render (s, 60, 2.0, 0.2);
        float tail = 0;
        for (size_t i = rel.samples.size() - 4800; i < rel.samples.size(); ++i)
            tail = std::max (tail, std::abs (rel.samples[i]));
        expect (tail < 1.0e-3f, "tail " + juce::String (tail));

        beginTest ("Peak at maximum keeps the output finite and bounded");
        setParam (s, hs20::ids::lpfPeak, 10.0f);
        setParam (s, hs20::ids::hpfPeak, 10.0f);
        setParam (s, hs20::ids::drive, 10.0f);
        auto hot = render (s, 36, 1.0, 1.0);
        expect (hot.finite && hot.peak <= 1.0f);

        beginTest ("Poly mode renders a chord without clipping beyond full scale");
        resetAll (s);
        setParam (s, hs20::ids::voiceMode, 1.0f);
        {
            hs20::dsp::SynthEngine engine;
            hs20::ParameterBridge bridge (s);
            hs20::dsp::EngineParams params;
            bridge.read (params);
            engine.prepare (48000.0, 512);
            juce::AudioBuffer<float> buffer (2, 512);
            juce::MidiBuffer midi;
            for (int n : { 48, 52, 55, 59, 62, 64, 67, 71 })
                midi.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 127), 0);
            bool finite = true;
            float peak = 0;
            for (int b = 0; b < 100; ++b)
            {
                engine.render (buffer, b == 0 ? midi : juce::MidiBuffer(), nullptr, params);
                for (int i = 0; i < 512; ++i)
                {
                    finite &= std::isfinite (buffer.getSample (0, i));
                    peak = std::max (peak, std::abs (buffer.getSample (0, i)));
                }
            }
            expect (finite && peak <= 1.0f && peak > 0.05f);
        }

        beginTest ("4x oversampling matches 2x tuning");
        resetAll (s);
        setParam (s, hs20::ids::drift, 0.0f);
        setParam (s, hs20::ids::vco1Wave, 0.0f);
        setParam (s, hs20::ids::quality, 1.0f);
        expectWithinAbsoluteError (estimateFrequency (render (s, 57, 1.0, 1.0).samples, 48000.0, 4800), 220.0, 1.0);

        beginTest ("ESP envelope follower triggers the VCA from external audio");
        resetAll (s);
        setParam (s, hs20::ids::espMode, 2.0f);  // pitch > VCO, gate from the follower
        setParam (s, hs20::ids::espThreshold, 2.0f);
        setParam (s, hs20::ids::drift, 0.0f);
        double phase = 0;
        auto ext = render (s, -1, 1.0, 0.0, 48000.0, [&phase] (juce::AudioBuffer<float>& in)
        {
            for (int i = 0; i < in.getNumSamples(); ++i)
            {
                in.setSample (0, i, 0.3f * (float) std::sin (phase));
                phase += 2.0 * juce::MathConstants<double>::pi * 220.0 / 48000.0;
            }
        });
        expect (ext.rmsDb > -40.0f, "ESP should open the VCA, rms " + juce::String (ext.rmsDb));
    }
};

class PresetTests : public juce::UnitTest
{
public:
    explicit PresetTests (juce::File levelsOut) : juce::UnitTest ("Presets", "HS20"), levelsFile (std::move (levelsOut)) {}

    void runTest() override
    {
        TestProcessor proc;
        const auto dir = makeTempDir();
        hs20::PresetManager pm (proc.apvts, dir);

        beginTest ("Factory library is large and uniquely named");
        expectGreaterThan (pm.getNumFactoryPresets(), 150);
        juce::StringArray keys;
        for (const auto& p : pm.getPresets())
            keys.add (p.key());
        keys.removeDuplicates (false);
        expectEquals (keys.size(), (int) pm.getPresets().size());

        beginTest ("Every factory preset loads and renders cleanly");
        auto* levels = new juce::DynamicObject();
        for (int i = 0; i < pm.getNumFactoryPresets(); ++i)
        {
            const auto& info = pm.getPresets()[(size_t) i];
            expect (pm.loadPreset (i));
            const bool external = info.category.startsWith ("ESP");
            auto stats = render (proc.apvts, info.category == "Bass" || info.category == "Acid" ? 43 : 60, 2.0, 1.2);
            expect (stats.finite, info.name + " produced NaN/Inf");
            expect (stats.peak <= 1.0f, info.name + " clips: " + juce::String (stats.peak));
            if (! external)
                expect (stats.loudestWindowDb > -40.0f, info.name + " is too quiet: " + juce::String (stats.loudestWindowDb) + " dB");
            levels->setProperty (info.name, stats.loudestWindowDb);
        }
        if (levelsFile != juce::File())
            levelsFile.replaceWithText (juce::JSON::toString (juce::var (levels)));
        else
            delete levels;

        beginTest ("Loading restores every parameter value");
        const int idx = pm.findPreset ("factory/Acid/Acid Squelch");
        expect (idx >= 0);
        pm.loadPreset (idx);
        expectWithinAbsoluteError (proc.apvts.getRawParameterValue (hs20::ids::lpfPeak)->load(), 8.5f, 0.01f);

        beginTest ("Likes persist and build the Liked bank");
        pm.setLiked (idx, true);
        pm.setLiked (3, true);
        expect (pm.isLiked (idx));
        expectEquals ((int) pm.getIndicesInBank (hs20::PresetManager::bankLiked).size(), 2);
        {
            hs20::PresetManager reopened (proc.apvts, dir);
            expect (reopened.isLiked (idx), "likes must survive a restart");
            expectEquals (reopened.getNumLiked(), 2);
        }
        const auto bank = juce::JSON::parse (pm.getLikedBankFile());
        expect (bank["presets"].getArray() != nullptr && bank["presets"].getArray()->size() == 2);

        pm.setLiked (3, false);
        expectEquals (pm.getNumLiked(), 1);

        beginTest ("Prev / next stay inside the selected bank");
        pm.setCurrentBank (hs20::PresetManager::bankLiked);
        pm.loadNext();
        expectEquals (pm.getCurrentIndex(), idx);
        pm.setCurrentBank ("Bass");
        pm.loadNext();
        expectEquals (pm.getPresets()[(size_t) pm.getCurrentIndex()].category, juce::String ("Bass"));

        beginTest ("User presets save, can be liked, exported and re-imported");
        setParam (proc.apvts, hs20::ids::lpfCutoff, 3.3f);
        const int user = pm.saveUserPreset ("My Test Sound", "User");
        expect (user >= pm.getNumFactoryPresets());
        pm.setLiked (user, true);
        const auto exportDir = dir.getChildFile ("export");
        expect (pm.exportLikedBank (exportDir));
        expect (exportDir.getChildFile ("My Test Sound.hs20preset").existsAsFile());
        expect (exportDir.getChildFile ("Liked Bank.hs20bank").existsAsFile());

        setParam (proc.apvts, hs20::ids::lpfCutoff, 9.0f);
        pm.loadPreset (user);
        expectWithinAbsoluteError (proc.apvts.getRawParameterValue (hs20::ids::lpfCutoff)->load(), 3.3f, 0.01f);

        expect (pm.deleteUserPreset (pm.findPreset ("user/User/My Test Sound")));
        expect (pm.importBank (exportDir.getChildFile ("Liked Bank.hs20bank")) == 2);

        dir.deleteRecursively();
    }

private:
    juce::File levelsFile;
};

class RandomizerTests : public juce::UnitTest
{
public:
    RandomizerTests() : juce::UnitTest ("Randomizer", "HS20") {}

    void runTest() override
    {
        TestProcessor proc;
        const auto dir = makeTempDir();
        hs20::PresetManager pm (proc.apvts, dir);

        for (int style = 0; style < hs20::randomStyleNames().size(); ++style)
        {
            beginTest ("Random " + hs20::randomStyleNames()[style] + " patches are playable");
            float quietest = 0.0f, loudest = -200.0f;
            for (int i = 0; i < 25; ++i)
            {
                pm.loadRandom ((hs20::RandomStyle) style);
                expect (pm.isRandomPatch() && pm.getCurrentName().isNotEmpty());
                const int note = pm.getRandomStyle() == "Bass" ? 43 : 60;
                auto stats = render (proc.apvts, note, 1.5, 1.0);
                const auto label = pm.getRandomStyle() + " '" + pm.getCurrentName() + "'";
                expect (stats.finite, label + " produced NaN/Inf");
                expect (stats.peak <= 1.0f, label + " clips");
                expect (stats.loudestWindowDb > -40.0f, label + " is too quiet: " + juce::String (stats.loudestWindowDb));
                quietest = std::min (quietest, stats.loudestWindowDb);
                loudest = std::max (loudest, stats.loudestWindowDb);
            }
            logMessage ("  level range " + juce::String (quietest, 1) + " .. " + juce::String (loudest, 1) + " dB");
            expect (loudest - quietest < 12.0f, "random patches should be level-matched");
        }

        beginTest ("Rolling the die shows a named, unsaved patch");
        pm.loadRandom (hs20::RandomStyle::lead);
        expect (pm.isRandomPatch());
        expectEquals (pm.getRandomStyle(), juce::String ("Lead"));
        expect (pm.getCurrentKey().startsWith ("random/Lead/"));
        const auto name = pm.getCurrentName();

        beginTest ("Liking a random patch saves it and adds it to the Liked bank");
        pm.likeCurrent();
        expect (! pm.isRandomPatch());
        expect (pm.getCurrentIndex() >= pm.getNumFactoryPresets());
        expectEquals (pm.getCurrentName(), name);
        expect (pm.isLiked (pm.getCurrentIndex()));

        beginTest ("Loading a preset clears the random state");
        pm.loadRandom (hs20::RandomStyle::any);
        pm.loadPreset (0);
        expect (! pm.isRandomPatch());

        dir.deleteRecursively();
    }
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    juce::File levels;
    for (int i = 1; i + 1 < argc; ++i)
        if (juce::String (argv[i]) == "--levels")
            levels = juce::File::getCurrentWorkingDirectory().getChildFile (argv[i + 1]);

    FilterTests filterTests;
    EngineTests engineTests;
    PresetTests presetTests (levels);
    RandomizerTests randomizerTests;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runTests ({ &filterTests, &engineTests, &presetTests, &randomizerTests });

    int failures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
        failures += runner.getResult (i)->failures;

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILURE(S)") << std::endl;
    return failures == 0 ? 0 : 1;
}
