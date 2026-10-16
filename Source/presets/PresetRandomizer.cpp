#include "PresetRandomizer.h"
#include "../Parameters.h"

namespace hs20
{
    namespace
    {
        constexpr int tri = 0, saw = 1, pulse = 2, noise = 3;               // VCO1
        constexpr int saw2 = 0, square2 = 1, narrow2 = 2, ring2 = 3;        // VCO2
        constexpr int f32 = 0, f16 = 1, f8 = 2, f4 = 3;

        struct Builder
        {
            juce::Random& rng;
            juce::DynamicObject::Ptr obj { new juce::DynamicObject() };
            int cables = 0;

            float range (float lo, float hi) { return lo + rng.nextFloat() * (hi - lo); }
            bool chance (float p) { return rng.nextFloat() < p; }
            template <typename T> T pick (std::initializer_list<T> items)
            {
                return *(items.begin() + rng.nextInt ((int) items.size()));
            }

            void set (const juce::String& id, float v) { obj->setProperty (id, std::round (v * 100.0f) / 100.0f); }
            void set (const juce::String& id, int v)   { obj->setProperty (id, v); }
            float get (const juce::String& id, float fallback) const
            {
                return obj->hasProperty (id) ? (float) obj->getProperty (id) : fallback;
            }

            void cable (PatchSource src, PatchDest dst, float amount)
            {
                if (cables >= ids::numPatchCables)
                    return;
                set (ids::patchSrc (cables), (int) src);
                set (ids::patchDst (cables), (int) dst);
                set (ids::patchAmt (cables), amount);
                ++cables;
            }
        };

        // VCO2 tuning: unison detune, fifth, octave or something wild.
        void tuneVco2 (Builder& b, bool wild)
        {
            const int choice = b.rng.nextInt (wild ? 5 : 4);
            switch (choice)
            {
                case 0:  b.set (ids::vco2Pitch, b.range (0.02f, 0.09f) * (b.chance (0.5f) ? 1.0f : -1.0f)); break;
                case 1:  b.set (ids::vco2Pitch, 2.92f); break;   // fifth up
                case 2:  b.set (ids::vco2Pitch, -2.92f); break;  // fourth down
                case 3:  b.set (ids::vco2Pitch, b.range (-0.05f, 0.05f)); break;
                default: b.set (ids::vco2Pitch, b.range (-5.0f, 5.0f)); break;
            }
        }

        // Curated routings that almost always sound good.
        void addMusicalCables (Builder& b, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                switch (b.rng.nextInt (9))
                {
                    case 0: b.cable (PatchSource::keyboard, PatchDest::lpfCutoff, b.range (4.0f, 10.0f)); break;
                    case 1: b.cable (PatchSource::mgTriangle, PatchDest::vco1Pw, b.range (2.0f, 6.0f)); break;
                    case 2: b.cable (PatchSource::sampleHold, PatchDest::lpfCutoff, b.range (2.0f, 5.0f)); break;
                    case 3: b.cable (PatchSource::eg2Rev, PatchDest::hpfCutoff, b.range (2.0f, 5.0f)); break;
                    case 4: b.cable (PatchSource::velocity, PatchDest::lpfCutoff, b.range (3.0f, 7.0f)); break;
                    case 5: b.cable (PatchSource::modWheel, PatchDest::lpfCutoff, b.range (4.0f, 9.0f)); break;
                    case 6: b.cable (PatchSource::mgTriangle, PatchDest::vco2Pitch, b.range (0.2f, 1.5f)); break;
                    case 7: b.cable (PatchSource::eg1, PatchDest::totalPitch, b.range (-1.0f, 1.0f)); break;
                    default: b.cable (PatchSource::modWheel, PatchDest::totalPitch, b.range (0.3f, 0.8f)); break;
                }
            }
        }

        void common (Builder& b)
        {
            b.set (ids::filterModel, b.chance (0.65f) ? 0 : 1);
            b.set (ids::drift, b.range (1.5f, 6.0f));
            b.set (ids::drive, b.range (1.0f, 7.0f));
            b.set (ids::vco1Pw, b.chance (0.5f) ? 0.0f : b.range (1.0f, 8.0f));
            b.set (ids::vco1Level, b.range (6.5f, 10.0f));
            b.set (ids::vco2Level, b.chance (0.25f) ? 0.0f : b.range (4.0f, 9.0f));
            b.set (ids::mgWave, b.chance (0.6f) ? 5.0f : b.range (0.0f, 10.0f));
            b.set (ids::mgFreq, b.range (3.0f, 6.5f));
            b.set (ids::eg1Release, b.range (2.0f, 5.0f));
            b.set (ids::legato, 1);
        }

        void bass (Builder& b)
        {
            b.set (ids::voiceMode, 0);
            b.set (ids::vco1Wave, b.pick ({ saw, saw, pulse, tri }));
            b.set (ids::vco1Scale, b.pick ({ f16, f16, f32 }));
            b.set (ids::vco2Wave, b.pick ({ saw2, square2, square2, ring2 }));
            b.set (ids::vco2Scale, b.pick ({ f16, f32, f8 }));
            tuneVco2 (b, false);
            b.set (ids::lpfCutoff, b.range (3.3f, 5.0f));
            b.set (ids::lpfPeak, b.range (0.0f, 8.2f));
            b.set (ids::lpfModEg2, b.range (2.0f, 6.0f));
            b.set (ids::eg2Attack, b.range (0.0f, 0.6f));
            b.set (ids::eg2Decay, b.range (3.0f, 6.0f));
            b.set (ids::eg2Sustain, b.chance (0.5f) ? b.range (0.0f, 2.0f) : b.range (3.0f, 7.0f));
            b.set (ids::eg2Release, b.range (1.5f, 3.5f));
            b.set (ids::portamento, b.chance (0.3f) ? b.range (1.5f, 4.5f) : 0.0f);
            if (b.chance (0.3f)) b.set (ids::fmEg1, b.range (0.8f, 1.8f));
        }

        void lead (Builder& b)
        {
            b.set (ids::voiceMode, 0);
            b.set (ids::vco1Wave, b.pick ({ saw, saw, pulse, tri }));
            b.set (ids::vco1Scale, b.pick ({ f8, f8, f16 }));
            b.set (ids::vco2Wave, b.pick ({ saw2, square2, narrow2, ring2 }));
            b.set (ids::vco2Scale, b.pick ({ f8, f4, f8 }));
            tuneVco2 (b, false);
            b.set (ids::lpfCutoff, b.range (5.5f, 7.6f));
            b.set (ids::lpfPeak, b.range (2.0f, 8.8f));
            b.set (ids::lpfModEg2, b.range (0.0f, 3.0f));
            b.set (ids::eg2Attack, b.range (0.0f, 3.5f));
            b.set (ids::eg2Sustain, b.range (7.0f, 10.0f));
            b.set (ids::eg2Release, b.range (2.5f, 5.0f));
            b.set (ids::portamento, b.range (0.0f, 4.0f));
            if (b.chance (0.6f))
            {
                b.set (ids::fmMg, b.range (0.8f, 1.4f));
                b.set (ids::mgFreq, b.range (5.5f, 6.5f));
            }
            if (b.chance (0.25f)) b.set (ids::hpfCutoff, b.range (2.5f, 4.5f));
        }

        void pad (Builder& b)
        {
            b.set (ids::voiceMode, 1);
            b.set (ids::vco1Wave, b.pick ({ saw, pulse, tri }));
            b.set (ids::vco1Scale, b.pick ({ f8, f16 }));
            b.set (ids::vco2Wave, b.pick ({ saw2, square2, saw2 }));
            b.set (ids::vco2Scale, b.pick ({ f8, f4, f16 }));
            b.set (ids::vco2Level, b.range (6.0f, 9.0f));
            b.set (ids::vco2Pitch, b.range (0.04f, 0.14f) * (b.chance (0.5f) ? 1.0f : -1.0f));
            b.set (ids::lpfCutoff, b.range (4.6f, 6.6f));
            b.set (ids::lpfPeak, b.range (0.0f, 5.5f));
            b.set (ids::lpfModEg2, b.range (0.0f, 3.0f));
            b.set (ids::lpfModMg, b.range (0.0f, 2.5f));
            b.set (ids::mgFreq, b.range (1.0f, 3.5f));
            b.set (ids::eg2Attack, b.range (5.5f, 8.0f));
            b.set (ids::eg2Decay, b.range (5.0f, 8.0f));
            b.set (ids::eg2Sustain, b.range (7.0f, 10.0f));
            b.set (ids::eg2Release, b.range (6.0f, 8.5f));
            b.set (ids::drift, b.range (5.0f, 8.5f));
            if (b.chance (0.4f)) b.cable (PatchSource::mgTriangle, PatchDest::vco1Pw, b.range (2.0f, 4.0f));
        }

        void pluck (Builder& b)
        {
            b.set (ids::voiceMode, b.chance (0.6f) ? 1 : 0);
            b.set (ids::vco1Wave, b.pick ({ saw, pulse, tri, pulse }));
            b.set (ids::vco1Scale, b.pick ({ f8, f16, f4 }));
            b.set (ids::vco2Wave, b.pick ({ saw2, square2, narrow2, ring2 }));
            b.set (ids::vco2Scale, b.pick ({ f8, f4, f16 }));
            tuneVco2 (b, false);
            b.set (ids::lpfCutoff, b.range (3.6f, 6.0f));
            b.set (ids::lpfPeak, b.range (1.0f, 7.5f));
            b.set (ids::lpfModEg2, b.range (2.5f, 5.5f));
            b.set (ids::eg2Attack, 0.0f);
            b.set (ids::eg2Decay, b.range (3.5f, 6.5f));
            b.set (ids::eg2Sustain, 0.0f);
            b.set (ids::eg2Release, b.range (3.0f, 5.5f));
            if (b.chance (0.4f)) b.cable (PatchSource::velocity, PatchDest::lpfCutoff, b.range (3.0f, 6.0f));
        }

        void fx (Builder& b)
        {
            b.set (ids::voiceMode, 0);
            b.set (ids::vco1Wave, b.pick ({ saw, pulse, tri, noise }));
            b.set (ids::vco1Scale, b.rng.nextInt (4));
            b.set (ids::vco2Wave, b.pick ({ saw2, square2, narrow2, ring2, ring2 }));
            b.set (ids::vco2Scale, b.rng.nextInt (4));
            tuneVco2 (b, true);
            b.set (ids::hpfCutoff, b.range (0.0f, 5.0f));
            b.set (ids::hpfPeak, b.range (0.0f, 9.0f));
            b.set (ids::lpfCutoff, b.range (5.0f, 8.5f));
            b.set (ids::lpfPeak, b.range (3.0f, 9.5f));
            b.set (ids::lpfModMg, b.range (0.0f, 4.0f));
            b.set (ids::fmMg, b.chance (0.5f) ? b.range (1.5f, 5.0f) : 0.0f);
            b.set (ids::fmEg1, b.chance (0.5f) ? b.range (1.0f, 6.0f) : 0.0f);
            b.set (ids::eg1Attack, b.range (0.0f, 8.0f));
            b.set (ids::eg1Release, b.range (2.0f, 7.0f));
            b.set (ids::mgFreq, b.range (2.0f, 9.0f));
            b.set (ids::eg2Attack, b.range (0.0f, 6.0f));
            b.set (ids::eg2Sustain, b.range (5.0f, 10.0f));
            b.set (ids::eg2Release, b.range (3.0f, 7.0f));

            const int n = 1 + b.rng.nextInt (3);
            for (int i = 0; i < n; ++i)
            {
                switch (b.rng.nextInt (6))
                {
                    case 0: b.cable (PatchSource::sampleHold, PatchDest::totalPitch, b.range (1.0f, 5.0f)); break;
                    case 1: b.cable (PatchSource::eg1Rev, PatchDest::totalPitch, b.range (2.0f, 8.0f)); break;
                    case 2: b.cable (PatchSource::mgTriangle, PatchDest::mgRate, b.range (2.0f, 6.0f)); break;
                    case 3: b.cable (PatchSource::mgSquare, PatchDest::vcaGain, b.range (5.0f, 10.0f)); break;
                    case 4: b.cable (PatchSource::sampleHold, PatchDest::lpfCutoff, b.range (3.0f, 6.0f)); break;
                    default: b.cable (PatchSource::eg2, PatchDest::vco2Pitch, b.range (-5.0f, 5.0f)); break;
                }
            }
        }

        void sequence (Builder& b)
        {
            b.set (ids::voiceMode, 0);
            b.set (ids::vco1Wave, b.pick ({ saw, pulse, tri }));
            b.set (ids::vco1Scale, b.pick ({ f8, f16 }));
            b.set (ids::vco2Wave, b.pick ({ saw2, square2, narrow2 }));
            tuneVco2 (b, false);
            b.set (ids::lpfCutoff, b.range (4.8f, 6.5f));
            b.set (ids::lpfPeak, b.range (5.0f, 8.8f));
            b.set (ids::mgFreq, b.range (5.0f, 7.2f));
            b.set (ids::eg2Sustain, 10.0f);
            b.set (ids::eg2Release, b.range (2.0f, 4.0f));

            switch (b.rng.nextInt (4))
            {
                case 0: b.cable (PatchSource::sampleHold, PatchDest::lpfCutoff, b.range (3.0f, 6.0f)); break;
                case 1: b.cable (PatchSource::sampleHold, PatchDest::totalPitch, b.range (1.5f, 4.0f)); break;
                case 2: b.cable (PatchSource::mgSquare, PatchDest::vcaGain, 10.0f); break;
                default: b.cable (PatchSource::mgSquare, PatchDest::totalPitch, b.pick ({ 0.5f, 0.29f, 0.21f })); break;
            }
            if (b.chance (0.5f)) b.cable (PatchSource::sampleHold, PatchDest::lpfPeak, b.range (1.0f, 3.0f));
        }

        // Guarantees the patch is heard: open enough filter, one loud VCO.
        void makePlayable (Builder& b)
        {
            const float sweep = b.get (ids::lpfModEg2, 0.0f) * 0.8f * (b.get (ids::eg2Sustain, 10.0f) > 3.0f ? 1.0f : 0.5f);
            b.set (ids::lpfCutoff, juce::jmax (b.get (ids::lpfCutoff, 10.0f), 5.2f - sweep));
            if (b.get (ids::hpfCutoff, 0.0f) > b.get (ids::lpfCutoff, 10.0f) - 1.0f)
                b.set (ids::hpfCutoff, b.get (ids::lpfCutoff, 10.0f) - 2.5f);
            b.set (ids::volume, 7.0f);
        }

        juce::String makeName (juce::Random& rng)
        {
            static const char* adjectives[] = { "Rusty", "Feral", "Molten", "Haunted", "Broken", "Neon", "Dusty", "Wired",
                                                "Crooked", "Restless", "Electric", "Lost", "Burning", "Hollow", "Frozen",
                                                "Wild", "Velvet", "Toxic", "Cosmic", "Reckless", "Savage", "Liquid", "Static" };
            static const char* nouns[] = { "Comet", "Wolf", "Circuit", "Ghost", "Engine", "Signal", "Robot", "Machine",
                                           "Storm", "Siren", "Voltage", "Orbit", "Lizard", "Furnace", "Satellite", "Beast",
                                           "Tide", "Spark", "Drone", "Crow", "Reactor", "Vortex", "Specter" };
            return juce::String (adjectives[rng.nextInt ((int) std::size (adjectives))]) + " "
                 + nouns[rng.nextInt ((int) std::size (nouns))];
        }
    }

    juce::StringArray randomStyleNames()
    {
        return { "Any", "Bass", "Lead", "Pad", "Pluck", "FX", "Sequence" };
    }

    RandomPatch makeRandomPatch (RandomStyle style, juce::Random& rng)
    {
        if (style == RandomStyle::any)
            style = (RandomStyle) (1 + rng.nextInt (6));

        Builder b { rng };
        common (b);

        switch (style)
        {
            case RandomStyle::bass:     bass (b); break;
            case RandomStyle::lead:     lead (b); break;
            case RandomStyle::pad:      pad (b); break;
            case RandomStyle::pluck:    pluck (b); break;
            case RandomStyle::fx:       fx (b); break;
            case RandomStyle::sequence: sequence (b); break;
            case RandomStyle::any:      break;
        }

        if (style != RandomStyle::fx && style != RandomStyle::sequence && rng.nextFloat() < 0.35f)
            addMusicalCables (b, 1 + rng.nextInt (2));

        makePlayable (b);

        RandomPatch patch;
        patch.style = randomStyleNames()[(int) style];
        patch.name = makeName (rng);
        patch.params = juce::var (b.obj.get());
        return patch;
    }
}
