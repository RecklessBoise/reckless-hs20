#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../Parameters.h"

namespace hs20::ui
{
    // The patch panel: drag from an output jack to an input jack to plug a
    // cable. Click a plugged input to pull the cable out (and drag it to
    // re-patch). Right-click an input to unplug everything from it.
    class PatchBay : public juce::Component,
                     private juce::AudioProcessorValueTreeState::Listener,
                     private juce::AsyncUpdater
    {
    public:
        explicit PatchBay (juce::AudioProcessorValueTreeState& state);
        ~PatchBay() override;

        void paint (juce::Graphics&) override;
        void paintOverChildren (juce::Graphics&) override;
        void resized() override;

        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

        // Output-jack position in this component, for cable colours elsewhere.
        static constexpr int numSources = (int) PatchSource::count - 1;  // without "off"
        static constexpr int numDests = (int) PatchDest::count;

    private:
        struct Cable { int source = 0; int dest = 0; };  // source is 1-based PatchSource

        void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
        void handleAsyncUpdate() override { repaint(); }

        Cable getCable (int index) const;
        void setCable (int index, int source, int dest);
        int findSourceAt (juce::Point<float>) const;
        int findDestAt (juce::Point<float>) const;
        void drawJack (juce::Graphics&, juce::Point<float>, bool hovered) const;
        void drawCable (juce::Graphics&, juce::Point<float> from, juce::Point<float> to, juce::Colour, bool plugAtEnd) const;

        juce::AudioProcessorValueTreeState& apvts;
        std::array<juce::Point<float>, numSources> sourcePos;
        std::array<juce::Point<float>, numDests> destPos;

        int hoverSource = -1, hoverDest = -1;
        int dragSource = -1;     // PatchSource index being dragged (1-based), -1 when idle
        int dragCable = -1;      // cable slot being re-patched, or -1 for a new one
        juce::Point<float> dragPos;
        float jackRadius = 7.0f;
    };
}
