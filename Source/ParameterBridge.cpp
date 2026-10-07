#include "ParameterBridge.h"

namespace hs20
{
    using namespace dsp;

    ParameterBridge::ParameterBridge (juce::AudioProcessorValueTreeState& state)
    {
        auto raw = [&state] (const juce::String& id)
        {
            auto* p = state.getRawParameterValue (id);
            jassert (p != nullptr);
            return p;
        };

        const std::pair<SmoothedId, const char*> smoothedIds[] = {
            { sVolume, ids::volume }, { sDrive, ids::drive }, { sVcaGain, ids::vcaGain },
            { sMasterTune, ids::masterTune }, { sVco1Pw, ids::vco1Pw }, { sVco2Pitch, ids::vco2Pitch },
            { sVco1Level, ids::vco1Level }, { sVco2Level, ids::vco2Level }, { sFmMg, ids::fmMg },
            { sFmEg1, ids::fmEg1 }, { sHpfCutoff, ids::hpfCutoff }, { sHpfPeak, ids::hpfPeak },
            { sHpfModMg, ids::hpfModMg }, { sHpfModEg2, ids::hpfModEg2 }, { sLpfCutoff, ids::lpfCutoff },
            { sLpfPeak, ids::lpfPeak }, { sLpfModMg, ids::lpfModMg }, { sLpfModEg2, ids::lpfModEg2 },
            { sMgWave, ids::mgWave }, { sMgFreq, ids::mgFreq }, { sEspLevel, ids::espLevel },
            { sEspCvAdjust, ids::espCvAdjust },
        };
        for (const auto& [index, id] : smoothedIds)
            smoothed[(size_t) index] = raw (id);

        for (int i = 0; i < ids::numPatchCables; ++i)
        {
            smoothed[(size_t) (sCableAmt0 + i)] = raw (ids::patchAmt (i));
            cableSrc[(size_t) i] = raw (ids::patchSrc (i));
            cableDst[(size_t) i] = raw (ids::patchDst (i));
        }

        voiceMode = raw (ids::voiceMode);
        filterModel = raw (ids::filterModel);
        quality = raw (ids::quality);
        bendRange = raw (ids::bendRange);
        legato = raw (ids::legato);
        drift = raw (ids::drift);
        portamento = raw (ids::portamento);
        vco1Wave = raw (ids::vco1Wave);
        vco1Scale = raw (ids::vco1Scale);
        vco2Wave = raw (ids::vco2Wave);
        vco2Scale = raw (ids::vco2Scale);

        const char* egIds[] = { ids::eg1Delay, ids::eg1Attack, ids::eg1Release, ids::eg2Hold,
                                ids::eg2Attack, ids::eg2Decay, ids::eg2Sustain, ids::eg2Release };
        for (size_t i = 0; i < eg.size(); ++i)
            eg[i] = raw (egIds[i]);

        espMode = raw (ids::espMode);
        espLowCut = raw (ids::espLowCut);
        espHighCut = raw (ids::espHighCut);
        espThreshold = raw (ids::espThreshold);
    }

    void ParameterBridge::read (EngineParams& p) const
    {
        auto get = [] (Raw r) { return r->load (std::memory_order_relaxed); };
        auto getInt = [] (Raw r) { return (int) std::lround (r->load (std::memory_order_relaxed)); };

        for (size_t i = 0; i < smoothed.size(); ++i)
            p.smooth[i] = get (smoothed[i]);

        p.voiceMode = getInt (voiceMode);
        p.filterModel = getInt (filterModel);
        p.quality = getInt (quality);
        p.bendRange = getInt (bendRange);
        p.legato = get (legato) > 0.5f;
        p.drift = get (drift);
        p.portamento = get (portamento);
        p.vco1Wave = getInt (vco1Wave);
        p.vco1Scale = getInt (vco1Scale);
        p.vco2Wave = getInt (vco2Wave);
        p.vco2Scale = getInt (vco2Scale);
        p.eg1Delay = get (eg[0]);
        p.eg1Attack = get (eg[1]);
        p.eg1Release = get (eg[2]);
        p.eg2Hold = get (eg[3]);
        p.eg2Attack = get (eg[4]);
        p.eg2Decay = get (eg[5]);
        p.eg2Sustain = get (eg[6]);
        p.eg2Release = get (eg[7]);
        p.espMode = getInt (espMode);
        p.espLowCut = get (espLowCut);
        p.espHighCut = get (espHighCut);
        p.espThreshold = get (espThreshold);

        for (size_t i = 0; i < p.cables.size(); ++i)
        {
            p.cables[i].source = (PatchSource) juce::jlimit (0, (int) PatchSource::count - 1, getInt (cableSrc[i]));
            p.cables[i].dest = (PatchDest) juce::jlimit (0, (int) PatchDest::count - 1, getInt (cableDst[i]));
        }
    }
}
