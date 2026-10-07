#pragma once

#include <array>
#include <atomic>
#include <memory>

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include "DspCore.h"
#include "../Parameters.h"

namespace hs20::dsp
{
    // Continuous parameters that are smoothed at the oversampled rate.
    enum SmoothedId
    {
        sVolume, sDrive, sVcaGain, sMasterTune, sVco1Pw, sVco2Pitch, sVco1Level, sVco2Level,
        sFmMg, sFmEg1, sHpfCutoff, sHpfPeak, sHpfModMg, sHpfModEg2, sLpfCutoff, sLpfPeak,
        sLpfModMg, sLpfModEg2, sMgWave, sMgFreq, sEspLevel, sEspCvAdjust,
        sCableAmt0, sCableAmtLast = sCableAmt0 + ids::numPatchCables - 1,
        numSmoothed
    };

    struct PatchCable
    {
        PatchSource source = PatchSource::off;
        PatchDest dest = PatchDest::totalPitch;
    };

    // A snapshot of every parameter, filled once per audio block.
    struct EngineParams
    {
        std::array<float, numSmoothed> smooth {};

        int voiceMode = 0, filterModel = 0, quality = 0, bendRange = 2;
        bool legato = true;
        int vco1Wave = 1, vco1Scale = 2, vco2Wave = 0, vco2Scale = 2;
        float drift = 3.0f, portamento = 0.0f;
        float mgWave = 5.0f;
        float eg1Delay = 0, eg1Attack = 0, eg1Release = 3;
        float eg2Hold = 0, eg2Attack = 0, eg2Decay = 5, eg2Sustain = 10, eg2Release = 3;
        int espMode = 0;
        float espLowCut = 0, espHighCut = 10, espThreshold = 3;
        std::array<PatchCable, ids::numPatchCables> cables {};

        EngineParams();
    };

    // Values shared by all voices for the current (oversampled) sample.
    struct GlobalMods
    {
        float mgTri = 0, mgSquare = 0, sampleHold = 0, white = 0, pink = 0;
        float modWheel = 0, bendSemis = 0;
        float espAudio = 0, espEnv = 0, espNote = 60.0f;
        bool espPitchActive = false;
    };

    // External Signal Processor: pre-amp, band-pass, envelope follower,
    // gate trigger and frequency-to-voltage (pitch) converter.
    class ExternalSignalProcessor
    {
    public:
        void prepare (float sampleRate);
        void reset();
        void update (float lowCutKnob, float highCutKnob, float thresholdKnob);
        float process (float input, float levelKnob);

        float getEnvelope() const noexcept { return envNorm; }
        bool getGate() const noexcept { return gate; }
        bool hasPitch() const noexcept { return pitchValid; }
        float getNote() const noexcept { return note; }

    private:
        float sampleRate = 44100.0f;
        OnePole lowCut1, lowCut2, highCut1, highCut2, pitchLp;
        float env = 0, envNorm = 0, envAttack = 0, envRelease = 0;
        float thresholdLin = 0.03f;
        bool gate = false;

        float lastSample = 0;
        bool armed = true;
        int samplesSinceCrossing = 0;
        float note = 60.0f;
        bool pitchValid = false;
    };

    class Voice
    {
    public:
        void prepare (float sampleRate, uint32_t seed, float voiceSpread);
        void reset();

        void noteOn (int note, float velocity, bool retrigger, bool glide);
        void setNoteLegato (int note);  // mono legato: glide without retrigger
        void noteOff();

        float process (const EngineParams& p, const std::array<float, numSmoothed>& s, const GlobalMods& g, float espAudioGain);
        void updateControl (const EngineParams& p);

        bool isSounding() const noexcept { return eg2.isActive(); }
        bool isGateOn() const noexcept { return gateHeld; }
        int getNote() const noexcept { return currentNote; }
        float getEg1() const noexcept { return eg1.getValue(); }
        float getEg2() const noexcept { return eg2.getValue(); }
        float getVelocity() const noexcept { return velocity; }
        float getPitch() const noexcept { return pitch; }
        uint64_t age = 0;
        bool everTriggered = false;

    private:
        float sampleRate = 44100.0f;
        int currentNote = 60;
        float velocity = 1.0f;
        float pitch = 60.0f, targetPitch = 60.0f, glideCoeff = 1.0f;
        bool gateHeld = false;
        float phase1 = 0, phase2 = 0;

        // analog character
        Noise rng;
        float driftTarget1 = 0, driftTarget2 = 0, drift1 = 0, drift2 = 0, driftCoeff = 0;
        int driftCounter = 0;
        float spreadPitch = 0, spreadCutoff = 0;

        EnvelopeDAR eg1;
        EnvelopeHADSR eg2;
        MS20Filter hpf { MS20Filter::Mode::highpass };
        MS20Filter lpf { MS20Filter::Mode::lowpass };
        float dcBlock = 0;
    };

    class SynthEngine
    {
    public:
        static constexpr int maxVoices = 8;
        static constexpr int scopeSize = 2048;

        SynthEngine();

        void prepare (double sampleRate, int maxBlockSize);
        void reset();
        int getLatencySamples() const;

        // extInput may be null. Output channels receive the same mono signal.
        void render (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi,
                     const float* extInput, const EngineParams& params);

        // UI telemetry (written on the audio thread, read by the editor)
        std::atomic<float> mgLed { 0.0f }, egLed { 0.0f }, espLed { 0.0f }, peakLevel { 0.0f };
        std::array<std::atomic<float>, scopeSize> scope {};
        std::atomic<int> scopeWritePos { 0 };

        // On-screen wheels from the editor (0..1 and -1..1).
        std::atomic<float> uiModWheel { 0.0f }, uiPitchBend { 0.0f };

    private:
        void handleMidi (const juce::MidiMessage& m, const EngineParams& p);
        void noteOn (int note, float velocity, const EngineParams& p);
        void noteOff (int note, const EngineParams& p);
        void allNotesOff();
        Voice& findVoiceForNote();
        void renderOversampled (float* data, int numSamples, int factor, const juce::MidiBuffer& midi,
                                const EngineParams& p);

        double baseSampleRate = 44100.0;
        int osFactor = 2, activeQuality = -1;
        std::unique_ptr<juce::dsp::Oversampling<float>> os2x, os4x;
        juce::AudioBuffer<float> monoBuffer;

        std::array<Voice, maxVoices> voices;
        uint64_t voiceCounter = 0;

        // Mono note memory (last-note priority) and sustain pedal.
        std::array<int, 128> noteStack {};
        int noteStackSize = 0;
        std::array<bool, 128> sustainedNotes {};
        bool sustainPedal = false;
        int lastVoiceMode = -1;
        int lastNote = 60;
        float lastVelocity = 1.0f;

        // Smoothed parameters
        std::array<float, numSmoothed> smoothed {};
        bool smoothInitialised = false;
        float smoothCoeff = 0.01f;

        // Global modulation generator, noise and sample & hold
        float mgPhase = 0, sampleHoldValue = 0;
        bool lastMgSquareHigh = false;
        Noise noise;
        PinkFilter pink;
        float midiModWheel = 0, midiBend = 0;

        ExternalSignalProcessor esp;
        bool espGateWasOn = false;

        float outputDc = 0, peakHold = 0;
        int scopeDecimator = 0;
    };
}
