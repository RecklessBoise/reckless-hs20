#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

// Small, allocation-free building blocks shared by the HS20 voices.
namespace hs20::dsp
{
    inline constexpr float pi = 3.14159265358979f;

    // Rational tanh approximation, exact enough for analog-style saturation.
    inline float fastTanh (float x) noexcept
    {
        x = std::clamp (x, -3.0f, 3.0f);
        const float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    // Maps a 0..10 panel knob onto an exponential time range in seconds.
    inline float knobToTime (float knob, float minSeconds, float maxSeconds) noexcept
    {
        return minSeconds * std::pow (maxSeconds / minSeconds, std::clamp (knob, 0.0f, 10.0f) * 0.1f);
    }

    // One-pole smoothing coefficient for a time constant in seconds.
    inline float onePoleCoeff (float seconds, float sampleRate) noexcept
    {
        return 1.0f - std::exp (-1.0f / std::max (1.0e-6f, seconds * sampleRate));
    }

    inline float semitonesToRatio (float semis) noexcept { return std::exp2 (semis * (1.0f / 12.0f)); }

    // PolyBLEP residual used to band-limit hard waveform edges.
    inline float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            t = (t - 1.0f) / dt;
            return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    inline float wrap01 (float x) noexcept { return x - std::floor (x); }

    // Fast deterministic white noise in [-1, 1].
    struct Noise
    {
        uint32_t state = 0x1234567u;

        void seed (uint32_t s) noexcept { state = s == 0 ? 0x9e3779b9u : s; }

        float next() noexcept
        {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return static_cast<float> (static_cast<int32_t> (state)) * (1.0f / 2147483648.0f);
        }
    };

    // Paul Kellet's economy pink noise filter.
    struct PinkFilter
    {
        float b0 = 0, b1 = 0, b2 = 0;

        float process (float white) noexcept
        {
            b0 = 0.99765f * b0 + white * 0.0990460f;
            b1 = 0.96300f * b1 + white * 0.2965164f;
            b2 = 0.57000f * b2 + white * 1.0526913f;
            return (b0 + b1 + b2 + white * 0.1848f) * 0.25f;
        }
    };

    // Topology-preserving one-pole used inside the Sallen-Key style filters.
    struct OnePoleTPT
    {
        float alpha = 0.0f, beta = 0.0f, z1 = 0.0f;

        float lowpass (float x) noexcept
        {
            const float v = (x - z1) * alpha;
            const float lp = v + z1;
            z1 = lp + v;
            return lp;
        }

        float highpass (float x) noexcept { return x - lowpass (x); }
        float feedback() const noexcept { return z1 * beta; }
        void reset() noexcept { z1 = 0.0f; }
    };

    // Simple TPT one-pole with its own coefficient, for the ESP band-pass.
    struct OnePole
    {
        float g = 0.0f, z1 = 0.0f;

        void setCutoff (float hz, float sampleRate) noexcept
        {
            const float w = std::tan (pi * std::min (hz, sampleRate * 0.45f) / sampleRate);
            g = w / (1.0f + w);
        }

        float lowpass (float x) noexcept
        {
            const float v = (x - z1) * g;
            const float lp = v + z1;
            z1 = lp + v;
            return lp;
        }

        float highpass (float x) noexcept { return x - lowpass (x); }
        void reset() noexcept { z1 = 0.0f; }
    };

    // Zero-delay-feedback model of the MS-20's 2-pole "35" filter with a
    // saturating feedback path. K approaching ~2 makes it scream and above
    // that it self-oscillates at the cutoff frequency, bounded by the clipper.
    class MS20Filter
    {
    public:
        enum class Mode { lowpass, highpass };

        explicit MS20Filter (Mode m = Mode::lowpass) : mode (m) {}

        void reset() noexcept
        {
            a.reset();
            b.reset();
            c.reset();
        }

        // headroom: saturation knee of the feedback clipper (higher = cleaner)
        void set (float cutoffHz, float sampleRate, float k, float headroomIn) noexcept
        {
            const float fc = std::clamp (cutoffHz, 5.0f, sampleRate * 0.45f);
            const float g = std::tan (pi * fc / sampleRate);
            const float G = g / (1.0f + g);
            K = std::max (0.01f, k);
            headroom = headroomIn;
            a.alpha = b.alpha = c.alpha = G;

            if (mode == Mode::lowpass)
            {
                b.beta = (K - K * G) / (1.0f + g);  // lpf2
                c.beta = -1.0f / (1.0f + g);        // hpf1
            }
            else
            {
                c.beta = -G / (1.0f + g);           // hpf2
                b.beta = 1.0f / (1.0f + g);         // lpf1
            }

            alpha0 = 1.0f / (1.0f - K * G + K * G * G);
        }

        float process (float x) noexcept
        {
            float y;

            if (mode == Mode::lowpass)
            {
                // a = lpf1, b = lpf2, c = hpf1
                const float y1 = a.lowpass (x);
                const float s35 = c.feedback() + b.feedback();
                float u = alpha0 * (y1 + s35);
                u = headroom * fastTanh (u / headroom);
                y = K * b.lowpass (u);
                c.highpass (y);
            }
            else
            {
                // a = hpf1, b = lpf1, c = hpf2
                const float y1 = a.highpass (x);
                const float s35 = c.feedback() + b.feedback();
                const float u = alpha0 * y1 + s35;
                y = headroom * fastTanh (K * u / headroom);
                b.lowpass (c.highpass (y));
            }

            return y / K;
        }

    private:
        Mode mode;
        OnePoleTPT a, b, c;
        float K = 0.01f, alpha0 = 1.0f, headroom = 1.0f;
    };

    // EG1 of the MS-20: Delay -> Attack -> (held at full) -> Release.
    class EnvelopeDAR
    {
    public:
        enum class Stage { idle, delay, attack, sustain, release };

        void setSampleRate (float sr) noexcept { sampleRate = sr; }

        void setTimes (float delaySec, float attackSec, float releaseSec) noexcept
        {
            delaySamples = static_cast<int> (delaySec * sampleRate);
            attackCoeff = onePoleCoeff (attackSec / 1.466f, sampleRate);
            releaseCoeff = onePoleCoeff (releaseSec / 4.0f, sampleRate);
        }

        void gateOn() noexcept
        {
            counter = 0;
            stage = delaySamples > 0 ? Stage::delay : Stage::attack;
        }

        void gateOff() noexcept
        {
            if (stage != Stage::idle)
                stage = Stage::release;
        }

        void reset() noexcept
        {
            stage = Stage::idle;
            value = 0.0f;
        }

        float process() noexcept
        {
            switch (stage)
            {
                case Stage::idle:
                    break;
                case Stage::delay:
                    if (++counter >= delaySamples)
                        stage = Stage::attack;
                    break;
                case Stage::attack:
                    value += (1.3f - value) * attackCoeff;
                    if (value >= 1.0f)
                    {
                        value = 1.0f;
                        stage = Stage::sustain;
                    }
                    break;
                case Stage::sustain:
                    value = 1.0f;
                    break;
                case Stage::release:
                    value -= value * releaseCoeff;
                    if (value < 1.0e-5f)
                    {
                        value = 0.0f;
                        stage = Stage::idle;
                    }
                    break;
            }
            return value;
        }

        Stage getStage() const noexcept { return stage; }
        float getValue() const noexcept { return value; }

    private:
        Stage stage = Stage::idle;
        float sampleRate = 44100.0f, value = 0.0f;
        float attackCoeff = 1.0f, releaseCoeff = 1.0f;
        int delaySamples = 0, counter = 0;
    };

    // EG2 of the MS-20: Attack -> Decay -> Sustain, with Hold extending the
    // gate after key release, then Release. Curves follow analog RC charging.
    class EnvelopeHADSR
    {
    public:
        enum class Stage { idle, attack, decay, sustain, hold, release };

        void setSampleRate (float sr) noexcept { sampleRate = sr; }

        void setParams (float holdSec, float attackSec, float decaySec, float sustainLevel, float releaseSec) noexcept
        {
            holdSamples = static_cast<int> (holdSec * sampleRate);
            attackCoeff = onePoleCoeff (attackSec / 1.466f, sampleRate);
            decayCoeff = onePoleCoeff (decaySec / 4.0f, sampleRate);
            releaseCoeff = onePoleCoeff (releaseSec / 4.0f, sampleRate);
            sustain = std::clamp (sustainLevel, 0.0f, 1.0f);
        }

        void gateOn() noexcept { stage = Stage::attack; }

        void gateOff() noexcept
        {
            if (stage == Stage::idle || stage == Stage::release)
                return;
            counter = 0;
            stage = holdSamples > 0 ? Stage::hold : Stage::release;
        }

        void reset() noexcept
        {
            stage = Stage::idle;
            value = 0.0f;
        }

        float process() noexcept
        {
            switch (stage)
            {
                case Stage::idle:
                    break;
                case Stage::attack:
                    value += (1.3f - value) * attackCoeff;
                    if (value >= 1.0f)
                    {
                        value = 1.0f;
                        stage = Stage::decay;
                    }
                    break;
                case Stage::decay:
                case Stage::sustain:
                    value += (sustain - value) * decayCoeff;
                    break;
                case Stage::hold:
                    value += (sustain - value) * decayCoeff;
                    if (++counter >= holdSamples)
                        stage = Stage::release;
                    break;
                case Stage::release:
                    value -= value * releaseCoeff;
                    if (value < 1.0e-5f)
                    {
                        value = 0.0f;
                        stage = Stage::idle;
                    }
                    break;
            }
            return value;
        }

        bool isActive() const noexcept { return stage != Stage::idle; }
        float getValue() const noexcept { return value; }

    private:
        Stage stage = Stage::idle;
        float sampleRate = 44100.0f, value = 0.0f, sustain = 1.0f;
        float attackCoeff = 1.0f, decayCoeff = 1.0f, releaseCoeff = 1.0f;
        int holdSamples = 0, counter = 0;
    };
}
