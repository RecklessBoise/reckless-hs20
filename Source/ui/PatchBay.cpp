#include "PatchBay.h"
#include "HS20LookAndFeel.h"

namespace hs20::ui
{
    PatchBay::PatchBay (juce::AudioProcessorValueTreeState& state) : apvts (state)
    {
        for (int i = 0; i < ids::numPatchCables; ++i)
        {
            apvts.addParameterListener (ids::patchSrc (i), this);
            apvts.addParameterListener (ids::patchDst (i), this);
        }
        setRepaintsOnMouseActivity (false);
    }

    PatchBay::~PatchBay()
    {
        for (int i = 0; i < ids::numPatchCables; ++i)
        {
            apvts.removeParameterListener (ids::patchSrc (i), this);
            apvts.removeParameterListener (ids::patchDst (i), this);
        }
    }

    PatchBay::Cable PatchBay::getCable (int index) const
    {
        Cable c;
        c.source = juce::roundToInt (apvts.getRawParameterValue (ids::patchSrc (index))->load());
        c.dest = juce::roundToInt (apvts.getRawParameterValue (ids::patchDst (index))->load());
        return c;
    }

    void PatchBay::setCable (int index, int source, int dest)
    {
        auto set = [this] (const juce::String& id, float value)
        {
            if (auto* p = apvts.getParameter (id))
            {
                p->beginChangeGesture();
                p->setValueNotifyingHost (p->convertTo0to1 (value));
                p->endChangeGesture();
            }
        };
        set (ids::patchSrc (index), (float) source);
        set (ids::patchDst (index), (float) dest);
    }

    void PatchBay::resized()
    {
        auto r = getLocalBounds().toFloat().reduced (8.0f, 0.0f);
        const float top = 26.0f;
        const float rowH = (r.getHeight() - top) / 4.0f;
        jackRadius = juce::jmin (8.0f, rowH * 0.2f);

        const int perRow = (numSources + 1) / 2;
        for (int i = 0; i < numSources; ++i)
        {
            const int row = i / perRow, col = i % perRow;
            const float colW = r.getWidth() / (float) perRow;
            sourcePos[(size_t) i] = { r.getX() + colW * ((float) col + 0.5f), top + rowH * ((float) row + 0.38f) };
        }

        const int destPerRow = (numDests + 1) / 2;
        for (int i = 0; i < numDests; ++i)
        {
            const int row = i / destPerRow, col = i % destPerRow;
            const float colW = r.getWidth() / (float) destPerRow;
            destPos[(size_t) i] = { r.getX() + colW * ((float) col + 0.5f), top + rowH * ((float) row + 2.38f) };
        }
    }

    int PatchBay::findSourceAt (juce::Point<float> p) const
    {
        for (int i = 0; i < numSources; ++i)
            if (p.getDistanceFrom (sourcePos[(size_t) i]) < jackRadius * 1.8f)
                return i;
        return -1;
    }

    int PatchBay::findDestAt (juce::Point<float> p) const
    {
        for (int i = 0; i < numDests; ++i)
            if (p.getDistanceFrom (destPos[(size_t) i]) < jackRadius * 1.8f)
                return i;
        return -1;
    }

    void PatchBay::drawJack (juce::Graphics& g, juce::Point<float> c, bool hovered) const
    {
        const float r = jackRadius;
        // hex nut
        juce::Path nut;
        nut.addPolygon (c, 6, r * 1.25f, juce::MathConstants<float>::pi / 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillPath (nut, juce::AffineTransform::translation (0.8f, 1.4f));
        juce::ColourGradient metal (juce::Colour (0xffd7d8d6), c.x - r, c.y - r, juce::Colour (0xff5d5f60), c.x + r, c.y + r, false);
        g.setGradientFill (metal);
        g.fillPath (nut);
        g.setColour (juce::Colour (0xff303132));
        g.strokePath (nut, juce::PathStrokeType (0.6f));
        // socket
        g.setColour (juce::Colour (0xff060606));
        g.fillEllipse (c.x - r * 0.58f, c.y - r * 0.58f, r * 1.16f, r * 1.16f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (c.x - r * 0.58f, c.y - r * 0.58f, r * 1.16f, r * 1.16f, 0.6f);

        if (hovered)
        {
            g.setColour (colours::accent.withAlpha (0.8f));
            g.drawEllipse (c.x - r * 1.5f, c.y - r * 1.5f, r * 3.0f, r * 3.0f, 1.4f);
        }
    }

    void PatchBay::drawCable (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour col, bool plugAtEnd) const
    {
        const float dist = a.getDistanceFrom (b);
        const auto mid = (a + b) * 0.5f + juce::Point<float> (0.0f, 18.0f + dist * 0.28f);
        juce::Path p;
        p.startNewSubPath (a);
        p.quadraticTo (mid, b);

        const float w = jackRadius * 0.85f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.strokePath (p, juce::PathStrokeType (w + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (2.0f, 4.0f));
        g.setColour (col.darker (0.6f));
        g.strokePath (p, juce::PathStrokeType (w + 1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col);
        g.strokePath (p, juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.strokePath (p, juce::PathStrokeType (w * 0.22f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::translation (-w * 0.18f, -w * 0.22f));

        auto plug = [&] (juce::Point<float> c)
        {
            const float r = jackRadius * 0.95f;
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillEllipse (c.x - r + 1.0f, c.y - r + 2.0f, r * 2, r * 2);
            juce::ColourGradient grad (col.brighter (0.4f), c.x - r, c.y - r, col.darker (0.7f), c.x + r, c.y + r, false);
            g.setGradientFill (grad);
            g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
            g.setColour (juce::Colour (0xffbfc0c0));
            g.drawEllipse (c.x - r * 0.55f, c.y - r * 0.55f, r * 1.1f, r * 1.1f, 1.0f);
        };
        plug (a);
        if (plugAtEnd)
            plug (b);
    }

    void PatchBay::paint (juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const auto srcNames = patchSourceShortNames();
        const auto dstNames = patchDestShortNames();

        g.setColour (colours::print);
        g.setFont (HS20LookAndFeel::panelFont (10.5f));
        g.drawText ("PATCH PANEL", bounds.removeFromTop (18.0f), juce::Justification::centred);

        // OUT / IN group outlines, printed like the original panel boxes.
        const float rowH = ((float) getHeight() - 26.0f) / 4.0f;
        auto outBox = juce::Rectangle<float> (4.0f, 22.0f, (float) getWidth() - 8.0f, rowH * 2.0f - 4.0f);
        auto inBox = outBox.translated (0.0f, rowH * 2.0f);
        g.setColour (colours::print.withAlpha (0.75f));
        g.drawRoundedRectangle (outBox, 4.0f, 1.0f);
        g.drawRoundedRectangle (inBox, 4.0f, 1.0f);

        g.setFont (HS20LookAndFeel::panelFont (8.0f));
        g.setColour (colours::print);
        g.drawText ("OUT", juce::Rectangle<float> (outBox.getX() + 4.0f, outBox.getY() + 2.0f, 30.0f, 10.0f), juce::Justification::left);
        g.drawText ("IN", juce::Rectangle<float> (inBox.getX() + 4.0f, inBox.getY() + 2.0f, 30.0f, 10.0f), juce::Justification::left);

        for (int i = 0; i < numSources; ++i)
        {
            const auto c = sourcePos[(size_t) i];
            drawJack (g, c, hoverSource == i);
            g.setColour (colours::print);
            g.drawText (srcNames[i + 1], juce::Rectangle<float> (70.0f, 10.0f).withCentre (c.translated (0.0f, jackRadius + 9.0f)),
                        juce::Justification::centred, false);
        }

        for (int i = 0; i < numDests; ++i)
        {
            const auto c = destPos[(size_t) i];
            drawJack (g, c, hoverDest == i);
            g.setColour (colours::print);
            g.drawText (dstNames[i], juce::Rectangle<float> (70.0f, 10.0f).withCentre (c.translated (0.0f, jackRadius + 9.0f)),
                        juce::Justification::centred, false);
        }
    }

    void PatchBay::paintOverChildren (juce::Graphics& g)
    {
        for (int i = 0; i < ids::numPatchCables; ++i)
        {
            if (i == dragCable)
                continue;

            const auto c = getCable (i);
            if (c.source <= 0 || c.source > numSources || c.dest < 0 || c.dest >= numDests)
                continue;

            drawCable (g, sourcePos[(size_t) (c.source - 1)], destPos[(size_t) c.dest], colours::cable (i), true);
        }

        if (dragSource > 0)
        {
            int colourIndex = dragCable;
            if (colourIndex < 0)
                for (int i = 0; i < ids::numPatchCables && colourIndex < 0; ++i)
                    if (getCable (i).source == 0)
                        colourIndex = i;

            drawCable (g, sourcePos[(size_t) (dragSource - 1)], dragPos,
                       colours::cable (juce::jmax (0, colourIndex)).withAlpha (0.85f), true);
        }
    }

    void PatchBay::mouseMove (const juce::MouseEvent& e)
    {
        const int s = findSourceAt (e.position), d = findDestAt (e.position);
        if (s != hoverSource || d != hoverDest)
        {
            hoverSource = s;
            hoverDest = d;
            repaint();
        }
        setMouseCursor (s >= 0 || d >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    void PatchBay::mouseExit (const juce::MouseEvent&)
    {
        hoverSource = hoverDest = -1;
        repaint();
    }

    void PatchBay::mouseDown (const juce::MouseEvent& e)
    {
        dragPos = e.position;
        const int s = findSourceAt (e.position);
        const int d = findDestAt (e.position);

        if (d >= 0)
        {
            // Pull out the most recently patched cable on this input.
            for (int i = ids::numPatchCables - 1; i >= 0; --i)
            {
                const auto c = getCable (i);
                if (c.source > 0 && c.dest == d)
                {
                    if (e.mods.isPopupMenu())
                    {
                        setCable (i, 0, 0);
                        continue;
                    }
                    dragSource = c.source;
                    dragCable = i;
                    break;
                }
            }
            repaint();
            return;
        }

        if (s >= 0 && ! e.mods.isPopupMenu())
        {
            dragSource = s + 1;
            dragCable = -1;
            repaint();
        }
    }

    void PatchBay::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragSource <= 0)
            return;
        dragPos = e.position;
        hoverDest = findDestAt (e.position);
        repaint();
    }

    void PatchBay::mouseUp (const juce::MouseEvent& e)
    {
        if (dragSource <= 0)
            return;

        const int d = findDestAt (e.position);
        int slot = dragCable;

        if (d >= 0)
        {
            if (slot < 0)
            {
                // reuse an identical cable, otherwise the first free slot, otherwise the oldest
                for (int i = 0; i < ids::numPatchCables && slot < 0; ++i)
                    if (getCable (i).source == dragSource && getCable (i).dest == d)
                        slot = i;
                for (int i = 0; i < ids::numPatchCables && slot < 0; ++i)
                    if (getCable (i).source == 0)
                        slot = i;
                if (slot < 0)
                    slot = ids::numPatchCables - 1;
            }
            setCable (slot, dragSource, d);
        }
        else if (slot >= 0)
        {
            setCable (slot, 0, 0);  // dropped in the air: unplugged
        }

        dragSource = -1;
        dragCable = -1;
        repaint();
    }
}
