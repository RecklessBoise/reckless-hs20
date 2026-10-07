#include "SynthEngine.h"

namespace hs20::dsp
{
    namespace
    {
        constexpr float scaleOffsets[] = { -24.0f, -12.0f, 0.0f, 12.0f };  // 32' 16' 8' 4'

        float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

        float noteToHz (float note) noexcept { return 440.0f * std::exp2 ((note - 69.0f) * (1.0f / 12.0f)); }

        // Panel cutoff knobs are calibrated in octaves above 20 Hz.
        float octavesToHz (float octaves) noexcept { return 20.0f * std::exp2 (std::clamp (octaves, -2.0f, 11.5f)); }

        float peakToK (float peak, bool lowpass, bool earlyModel) noexcept
        {
            const float shaped = std::pow (std::clamp (peak, 0.0f, 10.0f) * 0.1f, 1.5f);
            const float range = lowpass ? (earlyModel ? 2.32f : 2.14f) : (earlyModel ? 2.45f : 2.25f);
            return 0.01f + range * shaped;
        }

        bool isKeyboardLike (PatchSource s) noexcept { return s == PatchSource::keyboard || s == PatchSource::espPitch; }
    }

    EngineParams::EngineParams()
    {
        smooth[sVolume] = 7.0f;
        smooth[sDrive] = 3.0f;
        smooth[sVcaGain] = 0.0f;
        smooth[sMasterTune] = 0.0f;
        smooth[sVco1Pw] = 0.0f;
        smooth[sVco2Pitch] = 0.0f;
        smooth[sVco1Level] = 8.0f;
        smooth[sVco2Level] = 0.0f;
        smooth[sFmMg] = 0.0f;
        smooth[sFmEg1] = 0.0f;
        smooth[sHpfCutoff] = 0.0f;
        smooth[sHpfPeak] = 0.0f;
        smooth[sHpfModMg] = 0.0f;
        smooth[sHpfModEg2] = 0.0f;
        smooth[sLpfCutoff] = 10.0f;
        smooth[sLpfPeak] = 0.0f;
        smooth[sLpfModMg] = 0.0f;
        smooth[sLpfModEg2] = 0.0f;
        smooth[sMgWave] = 5.0f;
        smooth[sMgFreq] = 4.0f;
        smooth[sEspLevel] = 5.0f;
        smooth[sEspCvAdjust] = 5.0f;
        for (int i = 0; i < ids::numPatchCables; ++i)
            smooth[(size_t) (sCableAmt0 + i)] = 5.0f;
    }

    //==========================================================================
    void ExternalSignalProcessor::prepare (float sr)
    {
        sampleRate = sr;
        envAttack = onePoleCoeff (0.001f, sr);
        envRelease = onePoleCoeff (0.12f, sr);
        pitchLp.setCutoff (900.0f, sr);
        reset();
    }

    void ExternalSignalProcessor::reset()
    {
        lowCut1.reset(); lowCut2.reset(); highCut1.reset(); highCut2.reset(); pitchLp.reset();
        env = envNorm = 0.0f;
        gate = false;
        armed = true;
        samplesSinceCrossing = 0;
        pitchValid = false;
    }

    void ExternalSignalProcessor::update (float lowCutKnob, float highCutKnob, float thresholdKnob)
    {
        const float low = 20.0f * std::exp2 (lowCutKnob * 0.8f);
        const float high = 100.0f * std::exp2 (highCutKnob * 0.75f);
        lowCut1.setCutoff (low, sampleRate);
        lowCut2.setCutoff (low, sampleRate);
        highCut1.setCutoff (high, sampleRate);
        highCut2.setCutoff (high, sampleRate);
        thresholdLin = dbToGain (-60.0f + thresholdKnob * 6.0f);
    }

    float ExternalSignalProcessor::process (float input, float levelKnob)
    {
        float y = input * dbToGain (-20.0f + levelKnob * 5.0f);
        y = lowCut2.highpass (lowCut1.highpass (y));
        y = highCut2.lowpass (highCut1.lowpass (y));

        const float rect = std::abs (y);
        env += (rect - env) * (rect > env ? envAttack : envRelease);

        if (! gate && env > thresholdLin)
            gate = true;
        else if (gate && env < thresholdLin * 0.6f)
            gate = false;

        envNorm = std::clamp ((20.0f * std::log10 (env + 1.0e-6f) + 60.0f) / 60.0f, 0.0f, 1.0f);

        // F/V converter: period between upward zero crossings, with hysteresis.
        const float z = pitchLp.lowpass (y);
        const float hysteresis = std::max (env * 0.2f, 1.0e-4f);
        ++samplesSinceCrossing;

        if (armed && z > hysteresis)
        {
            const float freq = sampleRate / (float) samplesSinceCrossing;
            if (freq > 40.0f && freq < 2000.0f && gate)
            {
                const float newNote = 69.0f + 12.0f * std::log2 (freq / 440.0f);
                note = pitchValid ? note + (newNote - note) * 0.35f : newNote;
                pitchValid = true;
            }
            samplesSinceCrossing = 0;
            armed = false;
        }
        else if (z < -hysteresis)
        {
            armed = true;
        }

        if (samplesSinceCrossing > (int) (sampleRate / 30.0f))
            pitchValid = false;

        return std::clamp (y, -4.0f, 4.0f);
    }

    //==========================================================================
    void Voice::prepare (float sr, uint32_t seed, float voiceSpread)
    {
        sampleRate = sr;
        rng.seed (seed);
        eg1.setSampleRate (sr);
        eg2.setSampleRate (sr);
        driftCoeff = onePoleCoeff (0.6f, sr);
        spreadPitch = rng.next() * voiceSpread;
        spreadCutoff = rng.next() * voiceSpread;
        phase1 = wrap01 (rng.next());
        phase2 = wrap01 (rng.next());
    }

    void Voice::reset()
    {
        eg1.reset();
        eg2.reset();
        hpf.reset();
        lpf.reset();
        gateHeld = false;
        everTriggered = false;
    }

    void Voice::noteOn (int note, float vel, bool retrigger, bool glide)
    {
        currentNote = note;
        targetPitch = (float) note;
        if (! glide || ! everTriggered)
            pitch = (float) note;
        velocity = vel;
        gateHeld = true;
        everTriggered = true;

        if (retrigger)
        {
            eg1.gateOn();
            eg2.gateOn();
        }
    }

    void Voice::setNoteLegato (int note)
    {
        currentNote = note;
        targetPitch = (float) note;
    }

    void Voice::noteOff()
    {
        gateHeld = false;
        eg1.gateOff();
        eg2.gateOff();
    }

    void Voice::updateControl (const EngineParams& p)
    {
        const auto optionalTime = [] (float knob, float minS, float maxS)
        {
            return knob <= 0.05f ? 0.0f : knobToTime (knob, minS, maxS);
        };

        eg1.setTimes (optionalTime (p.eg1Delay, 0.005f, 5.0f),
                      knobToTime (p.eg1Attack, 0.0005f, 10.0f),
                      knobToTime (p.eg1Release, 0.002f, 10.0f));

        eg2.setParams (optionalTime (p.eg2Hold, 0.005f, 5.0f),
                       knobToTime (p.eg2Attack, 0.0005f, 10.0f),
                       knobToTime (p.eg2Decay, 0.002f, 10.0f),
                       p.eg2Sustain * 0.1f,
                       knobToTime (p.eg2Release, 0.002f, 10.0f));

        glideCoeff = p.portamento <= 0.05f ? 1.0f
                                           : onePoleCoeff (knobToTime (p.portamento, 0.005f, 3.0f) / 3.0f, sampleRate);
    }

    float Voice::process (const EngineParams& p, const std::array<float, numSmoothed>& s, const GlobalMods& g, float espAudioGain)
    {
        const float e1 = eg1.process();
        const float e2 = eg2.process();

        // Slow random pitch wander, like warm analog oscillators.
        if (--driftCounter <= 0)
        {
            driftCounter = (int) (sampleRate * (0.25f + 0.25f * std::abs (rng.next())));
            driftTarget1 = rng.next();
            driftTarget2 = rng.next();
        }
        drift1 += (driftTarget1 - drift1) * driftCoeff;
        drift2 += (driftTarget2 - drift2) * driftCoeff;

        pitch += (targetPitch - pitch) * glideCoeff;

        // ---- Patch panel ---------------------------------------------------
        float modTotal = 0, modVco2 = 0, modPw = 0, modHp = 0, modLp = 0;
        float modHpPeak = 0, modLpPeak = 0, modVca = 0, modLevel1 = 0, modLevel2 = 0;

        for (size_t i = 0; i < p.cables.size(); ++i)
        {
            const auto& cable = p.cables[i];
            if (cable.source == PatchSource::off)
                continue;

            const float amt = s[sCableAmt0 + i] * 0.1f;
            float v = 0.0f;

            switch (cable.source)
            {
                case PatchSource::mgTriangle:  v = g.mgTri; break;
                case PatchSource::mgSquare:    v = g.mgSquare; break;
                case PatchSource::eg1:         v = e1; break;
                case PatchSource::eg1Rev:      v = -e1; break;
                case PatchSource::eg2:         v = e2; break;
                case PatchSource::eg2Rev:      v = -e2; break;
                case PatchSource::sampleHold:  v = g.sampleHold; break;
                case PatchSource::whiteNoise:  v = g.white; break;
                case PatchSource::pinkNoise:   v = g.pink * 2.0f; break;
                case PatchSource::keyboard:    v = ((float) currentNote - 60.0f) / 12.0f; break;
                case PatchSource::velocity:    v = velocity; break;
                case PatchSource::modWheel:    v = g.modWheel; break;
                case PatchSource::espEnvelope: v = g.espEnv; break;
                case PatchSource::espPitch:    v = (g.espNote - 60.0f) / 12.0f; break;
                case PatchSource::off:
                case PatchSource::count:       break;
            }

            // Keyboard-style sources are in octaves: amount 10 = 1V/oct tracking.
            const bool kbd = isKeyboardLike (cable.source);
            if (kbd && ! (cable.dest == PatchDest::totalPitch || cable.dest == PatchDest::vco2Pitch
                          || cable.dest == PatchDest::hpfCutoff || cable.dest == PatchDest::lpfCutoff))
                v *= 0.2f;

            const float x = v * amt;
            switch (cable.dest)
            {
                case PatchDest::totalPitch: modTotal += x * (kbd ? 12.0f : 24.0f); break;
                case PatchDest::vco2Pitch:  modVco2 += x * (kbd ? 12.0f : 24.0f); break;
                case PatchDest::vco1Pw:     modPw += x * 0.45f; break;
                case PatchDest::hpfCutoff:  modHp += x * (kbd ? 1.0f : 5.0f); break;
                case PatchDest::lpfCutoff:  modLp += x * (kbd ? 1.0f : 5.0f); break;
                case PatchDest::hpfPeak:    modHpPeak += x * 10.0f; break;
                case PatchDest::lpfPeak:    modLpPeak += x * 10.0f; break;
                case PatchDest::vcaGain:    modVca += x; break;
                case PatchDest::vco1Level:  modLevel1 += x * 10.0f; break;
                case PatchDest::vco2Level:  modLevel2 += x * 10.0f; break;
                case PatchDest::mgRate:     // global, handled by the engine
                case PatchDest::count:      break;
            }
        }

        // ---- Oscillators ---------------------------------------------------
        const float driftAmount = p.drift * 0.012f;  // up to 12 cents
        const float notePitch = g.espPitchActive ? g.espNote : pitch;
        const float fmMg = s[sFmMg] * 0.1f;
        const float fmEg = s[sFmEg1] * 0.1f;

        const float base = notePitch + g.bendSemis + s[sMasterTune] * 0.2f + spreadPitch * driftAmount * 0.5f
                         + fmMg * fmMg * 24.0f * g.mgTri + fmEg * fmEg * 48.0f * e1 + modTotal;

        const float pitch1 = base + scaleOffsets[p.vco1Scale & 3] + drift1 * driftAmount;
        const float pitch2 = base + scaleOffsets[p.vco2Scale & 3] + s[sVco2Pitch] * 2.4f + modVco2 + drift2 * driftAmount;

        const float dt1 = std::clamp (noteToHz (pitch1) / sampleRate, 0.0f, 0.45f);
        const float dt2 = std::clamp (noteToHz (pitch2) / sampleRate, 0.0f, 0.45f);

        phase1 = wrap01 (phase1 + dt1);
        phase2 = wrap01 (phase2 + dt2);

        const float width = std::clamp (0.5f - s[sVco1Pw] * 0.045f + modPw, 0.03f, 0.97f);
        float pulse1 = phase1 < width ? 1.0f : -1.0f;
        pulse1 += polyBlep (phase1, dt1);
        pulse1 -= polyBlep (wrap01 (phase1 - width), dt1);

        float osc1 = 0.0f;
        switch (p.vco1Wave)
        {
            case 0:  osc1 = 4.0f * std::abs (phase1 - 0.5f) - 1.0f; break;
            case 1:  osc1 = 2.0f * phase1 - 1.0f - polyBlep (phase1, dt1); break;
            case 2:  osc1 = pulse1; break;
            default: osc1 = rng.next(); break;
        }

        const auto pulse2 = [&] (float w)
        {
            float v = phase2 < w ? 1.0f : -1.0f;
            v += polyBlep (phase2, dt2);
            v -= polyBlep (wrap01 (phase2 - w), dt2);
            return v;
        };

        float osc2 = 0.0f;
        switch (p.vco2Wave)
        {
            case 0:  osc2 = 2.0f * phase2 - 1.0f - polyBlep (phase2, dt2); break;
            case 1:  osc2 = pulse2 (0.5f); break;
            case 2:  osc2 = pulse2 (0.1f); break;
            default: osc2 = pulse1 * pulse2 (0.5f); break;  // ring modulator
        }

        const float level1 = std::clamp (s[sVco1Level] + modLevel1, 0.0f, 10.0f) * 0.1f;
        const float level2 = std::clamp (s[sVco2Level] + modLevel2, 0.0f, 10.0f) * 0.1f;
        const float mix = osc1 * level1 + osc2 * level2 + g.espAudio * espAudioGain;

        // ---- Filters: HPF -> LPF, both "35" style 2-pole ----------------
        const bool early = p.filterModel == 0;
        const float driveGain = 1.0f + s[sDrive] * 0.3f;
        // A whisper of circuit noise lets the filters self-oscillate from silence.
        float x = mix * driveGain * 0.7f + rng.next() * 1.0e-4f;
        if (! early)
            x = 2.0f * fastTanh (x * 0.5f);  // OTA input stage

        const float cutoffSpread = spreadCutoff * p.drift * 0.01f;
        const float hpOct = s[sHpfCutoff] + g.mgTri * s[sHpfModMg] * 0.4f + e2 * s[sHpfModEg2] * 0.8f + modHp + cutoffSpread;
        const float lpOct = s[sLpfCutoff] + g.mgTri * s[sLpfModMg] * 0.4f + e2 * s[sLpfModEg2] * 0.8f + modLp + cutoffSpread;

        const float headroom = early ? 1.0f : 2.0f;
        hpf.set (octavesToHz (hpOct), sampleRate, peakToK (s[sHpfPeak] + modHpPeak, false, early), headroom);
        lpf.set (octavesToHz (lpOct), sampleRate, peakToK (s[sLpfPeak] + modLpPeak, true, early), headroom);

        float y = lpf.process (hpf.process (x));
        y *= 1.0f / std::sqrt (driveGain);

        // ---- VCA -------------------------------------------------------
        const float amp = std::clamp (e2 + s[sVcaGain] * 0.1f + modVca, 0.0f, 1.5f);
        return y * amp;
    }

    //==========================================================================
    SynthEngine::SynthEngine()
    {
        using OS = juce::dsp::Oversampling<float>;
        os2x = std::make_unique<OS> (1, 1, OS::filterHalfBandPolyphaseIIR, true, false);
        os4x = std::make_unique<OS> (1, 2, OS::filterHalfBandPolyphaseIIR, true, false);
        noise.seed (0xC0FFEEu);
    }

    void SynthEngine::prepare (double sampleRate, int maxBlockSize)
    {
        baseSampleRate = sampleRate;
        os2x->initProcessing ((size_t) maxBlockSize);
        os4x->initProcessing ((size_t) maxBlockSize);
        monoBuffer.setSize (1, maxBlockSize);
        activeQuality = -1;
        reset();
    }

    int SynthEngine::getLatencySamples() const
    {
        return (int) std::round ((osFactor == 4 ? os4x : os2x)->getLatencyInSamples());
    }

    void SynthEngine::reset()
    {
        os2x->reset();
        os4x->reset();
        allNotesOff();
        for (auto& v : voices)
            v.reset();
        noteStackSize = 0;
        smoothInitialised = false;
        outputDc = 0.0f;
        esp.reset();
    }

    void SynthEngine::allNotesOff()
    {
        for (auto& v : voices)
            if (v.isGateOn())
                v.noteOff();
        noteStackSize = 0;
        sustainedNotes.fill (false);
    }

    Voice& SynthEngine::findVoiceForNote()
    {
        Voice* best = nullptr;
        for (auto& v : voices)
            if (! v.isSounding() && (best == nullptr || v.age < best->age))
                best = &v;

        if (best == nullptr)
            for (auto& v : voices)
                if (best == nullptr || v.age < best->age)
                    best = &v;

        return *best;
    }

    void SynthEngine::noteOn (int note, float velocity, const EngineParams& p)
    {
        lastNote = note;
        lastVelocity = velocity;

        if (p.voiceMode == 0)
        {
            // remove if already in stack, then push on top
            int w = 0;
            for (int r = 0; r < noteStackSize; ++r)
                if (noteStack[(size_t) r] != note)
                    noteStack[(size_t) w++] = noteStack[(size_t) r];
            noteStackSize = w;
            const bool othersHeld = noteStackSize > 0 && voices[0].isGateOn();
            noteStack[(size_t) noteStackSize++] = note;

            if (p.legato && othersHeld)
                voices[0].setNoteLegato (note);
            else
                voices[0].noteOn (note, velocity, true, true);
            return;
        }

        sustainedNotes[(size_t) note] = false;
        auto& v = findVoiceForNote();
        v.noteOn (note, velocity, true, true);
        v.age = ++voiceCounter;
    }

    void SynthEngine::noteOff (int note, const EngineParams& p)
    {
        if (p.voiceMode == 0)
        {
            int w = 0;
            for (int r = 0; r < noteStackSize; ++r)
                if (noteStack[(size_t) r] != note)
                    noteStack[(size_t) w++] = noteStack[(size_t) r];
            noteStackSize = w;

            if (noteStackSize == 0)
            {
                if (! (p.espMode != 0 && esp.getGate()))
                    voices[0].noteOff();
            }
            else if (voices[0].getNote() == note)
            {
                const int previous = noteStack[(size_t) noteStackSize - 1];
                if (p.legato)
                    voices[0].setNoteLegato (previous);
                else
                    voices[0].noteOn (previous, lastVelocity, true, true);
            }
            return;
        }

        if (sustainPedal)
        {
            sustainedNotes[(size_t) note] = true;
            return;
        }

        for (auto& v : voices)
            if (v.isGateOn() && v.getNote() == note)
                v.noteOff();
    }

    void SynthEngine::handleMidi (const juce::MidiMessage& m, const EngineParams& p)
    {
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber(), p);
        else if (m.isPitchWheel())
            midiBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
        else if (m.isControllerOfType (1))
            midiModWheel = (float) m.getControllerValue() / 127.0f;
        else if (m.isControllerOfType (64))
        {
            sustainPedal = m.getControllerValue() >= 64;
            if (! sustainPedal)
            {
                for (int n = 0; n < 128; ++n)
                    if (sustainedNotes[(size_t) n])
                    {
                        sustainedNotes[(size_t) n] = false;
                        for (auto& v : voices)
                            if (v.isGateOn() && v.getNote() == n)
                                v.noteOff();
                    }
            }
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff())
            allNotesOff();
    }

    void SynthEngine::render (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi,
                              const float* extInput, const EngineParams& p)
    {
        const int numSamples = output.getNumSamples();
        if (numSamples == 0)
            return;

        if (p.quality != activeQuality)
        {
            activeQuality = p.quality;
            osFactor = p.quality == 1 ? 4 : 2;
            (osFactor == 4 ? os4x : os2x)->reset();

            const auto osRate = (float) (baseSampleRate * osFactor);
            for (size_t i = 0; i < voices.size(); ++i)
                voices[i].prepare (osRate, 0x51u + (uint32_t) i * 7919u, i == 0 ? 0.0f : 1.0f);
            esp.prepare (osRate);
            smoothCoeff = onePoleCoeff (0.008f, osRate);
        }

        if (p.voiceMode != lastVoiceMode)
        {
            if (lastVoiceMode != -1)
                allNotesOff();
            lastVoiceMode = p.voiceMode;
        }

        if (! smoothInitialised)
        {
            smoothed = p.smooth;
            smoothInitialised = true;
        }

        for (auto& v : voices)
            v.updateControl (p);
        esp.update (p.espLowCut, p.espHighCut, p.espThreshold);

        if (extInput != nullptr)
            monoBuffer.copyFrom (0, 0, extInput, numSamples);
        else
            monoBuffer.clear (0, 0, numSamples);

        auto& os = osFactor == 4 ? *os4x : *os2x;
        juce::dsp::AudioBlock<float> block (monoBuffer.getArrayOfWritePointers(), 1, (size_t) numSamples);
        auto up = os.processSamplesUp (block);
        renderOversampled (up.getChannelPointer (0), (int) up.getNumSamples(), osFactor, midi, p);
        os.processSamplesDown (block);

        // DC blocker + telemetry at the host rate
        auto* mono = monoBuffer.getWritePointer (0);
        float peak = 0.0f;
        int writePos = scopeWritePos.load (std::memory_order_relaxed);
        for (int i = 0; i < numSamples; ++i)
        {
            outputDc += (mono[i] - outputDc) * 0.0005f;
            mono[i] -= outputDc;
            peak = std::max (peak, std::abs (mono[i]));

            if (++scopeDecimator >= 2)
            {
                scopeDecimator = 0;
                scope[(size_t) writePos].store (mono[i], std::memory_order_relaxed);
                writePos = (writePos + 1) % scopeSize;
            }
        }
        scopeWritePos.store (writePos, std::memory_order_relaxed);
        peakHold = std::max (peak, peakHold * 0.9f);
        peakLevel.store (peakHold, std::memory_order_relaxed);

        for (int ch = 0; ch < output.getNumChannels(); ++ch)
            output.copyFrom (ch, 0, monoBuffer, 0, 0, numSamples);
    }

    void SynthEngine::renderOversampled (float* data, int numSamples, int factor, const juce::MidiBuffer& midi,
                                         const EngineParams& p)
    {
        const float osRate = (float) baseSampleRate * (float) factor;
        auto midiIt = midi.begin();
        const auto midiEnd = midi.end();

        const bool mono = p.voiceMode == 0;
        const float espAudioGain = (p.espMode & 1) != 0 ? 1.0f : 0.0f;
        const bool espPitchMode = (p.espMode & 2) != 0;

        GlobalMods g;
        Voice* lastVoice = &voices[0];
        uint64_t newest = 0;
        for (auto& v : voices)
            if (v.age >= newest)
            {
                newest = v.age;
                lastVoice = &v;
            }
        if (mono)
            lastVoice = &voices[0];

        for (int i = 0; i < numSamples; ++i)
        {
            while (midiIt != midiEnd && (*midiIt).samplePosition * factor <= i)
            {
                handleMidi ((*midiIt).getMessage(), p);
                ++midiIt;
                if (! mono)
                    for (auto& v : voices)
                        if (v.age >= newest)
                        {
                            newest = v.age;
                            lastVoice = &v;
                        }
            }

            for (size_t k = 0; k < smoothed.size(); ++k)
                smoothed[k] += (p.smooth[k] - smoothed[k]) * smoothCoeff;

            // ---- Modulation generator (global, as on the MS-20) --------
            float mgRateMod = 0.0f;
            for (size_t c = 0; c < p.cables.size(); ++c)
            {
                const auto& cable = p.cables[c];
                if (cable.dest != PatchDest::mgRate || cable.source == PatchSource::off)
                    continue;

                float v = 0.0f;
                switch (cable.source)
                {
                    case PatchSource::eg1:         v = lastVoice->getEg1(); break;
                    case PatchSource::eg1Rev:      v = -lastVoice->getEg1(); break;
                    case PatchSource::eg2:         v = lastVoice->getEg2(); break;
                    case PatchSource::eg2Rev:      v = -lastVoice->getEg2(); break;
                    case PatchSource::sampleHold:  v = sampleHoldValue; break;
                    case PatchSource::keyboard:    v = ((float) lastVoice->getNote() - 60.0f) / 60.0f; break;
                    case PatchSource::velocity:    v = lastVoice->getVelocity(); break;
                    case PatchSource::modWheel:    v = g.modWheel; break;
                    case PatchSource::espEnvelope: v = esp.getEnvelope(); break;
                    default: break;
                }
                mgRateMod += v * smoothed[sCableAmt0 + c] * 0.5f;
            }

            const float mgHz = 0.05f * std::pow (1000.0f, std::clamp (smoothed[sMgFreq] + mgRateMod, 0.0f, 10.0f) * 0.1f);
            mgPhase = wrap01 (mgPhase + mgHz / osRate);
            const float bp = std::clamp (smoothed[sMgWave] * 0.1f, 0.02f, 0.98f);
            g.mgTri = mgPhase < bp ? -1.0f + 2.0f * mgPhase / bp : 1.0f - 2.0f * (mgPhase - bp) / (1.0f - bp);
            const bool squareHigh = mgPhase < 0.5f;
            g.mgSquare = squareHigh ? 1.0f : -1.0f;
            if (squareHigh && ! lastMgSquareHigh)
                sampleHoldValue = noise.next();
            lastMgSquareHigh = squareHigh;

            g.sampleHold = sampleHoldValue;
            g.white = noise.next();
            g.pink = pink.process (g.white);
            g.modWheel = std::max (midiModWheel, uiModWheel.load (std::memory_order_relaxed));
            g.bendSemis = std::clamp (midiBend + uiPitchBend.load (std::memory_order_relaxed), -1.0f, 1.0f) * (float) p.bendRange;

            // ---- External signal processor ---------------------------
            if (p.espMode != 0)
            {
                g.espAudio = esp.process (data[i], smoothed[sEspLevel]);
                g.espEnv = esp.getEnvelope();
                g.espNote = esp.getNote() + (smoothed[sEspCvAdjust] - 5.0f) * 4.8f;
                g.espPitchActive = espPitchMode && esp.hasPitch();

                const bool gate = esp.getGate();
                if (mono && noteStackSize == 0)
                {
                    if (gate && ! espGateWasOn)
                        voices[0].noteOn (espPitchMode ? (int) std::round (g.espNote) : lastNote, 1.0f, true, true);
                    else if (! gate && espGateWasOn)
                        voices[0].noteOff();
                }
                espGateWasOn = gate;
            }
            else
            {
                g.espAudio = 0.0f;
                g.espEnv = 0.0f;
                g.espPitchActive = false;
                espGateWasOn = false;
            }

            // ---- Voices ------------------------------------------------
            float sum = 0.0f;
            if (mono)
            {
                sum = voices[0].process (p, smoothed, g, espAudioGain);
            }
            else
            {
                const bool drone = smoothed[sVcaGain] > 0.001f;
                for (auto& v : voices)
                    if (v.isSounding() || (drone && v.everTriggered))
                        sum += v.process (p, smoothed, g, espAudioGain);
                sum *= 0.55f;
            }

            const float vol = smoothed[sVolume] * 0.1f;
            data[i] = fastTanh (sum * vol * vol * 1.2f);
        }

        mgLed.store (0.5f + 0.5f * g.mgTri, std::memory_order_relaxed);
        egLed.store (lastVoice->getEg2(), std::memory_order_relaxed);
        espLed.store (g.espEnv, std::memory_order_relaxed);
    }
}
