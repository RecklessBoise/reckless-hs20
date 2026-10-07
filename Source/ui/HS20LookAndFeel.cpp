#include "HS20LookAndFeel.h"

namespace hs20::ui
{
    juce::Colour colours::cable (int index)
    {
        static const juce::Colour palette[] = {
            juce::Colour (0xffd8392b), juce::Colour (0xffe8b923), juce::Colour (0xff2f7fd6),
            juce::Colour (0xff3fae5a), juce::Colour (0xffef7d22), juce::Colour (0xff9a5bd3)
        };
        return palette[(size_t) juce::jlimit (0, 5, index)];
    }

    HS20LookAndFeel::HS20LookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a1a1b));
        setColour (juce::PopupMenu::textColourId, colours::print);
        setColour (juce::PopupMenu::headerTextColourId, colours::printDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent.withAlpha (0.85f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::ComboBox::textColourId, colours::print);
        setColour (juce::ComboBox::arrowColourId, colours::printDim);
        setColour (juce::TextButton::textColourOffId, colours::print);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1d1e20));
        setColour (juce::AlertWindow::textColourId, colours::print);
        setColour (juce::AlertWindow::outlineColourId, colours::printDim);
        setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0d0d0e));
        setColour (juce::TextEditor::textColourId, colours::print);
        setColour (juce::TextEditor::outlineColourId, colours::printDim.withAlpha (0.5f));
        setColour (juce::TextEditor::focusedOutlineColourId, colours::accent);
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff0d0d0e));
        setColour (juce::TooltipWindow::textColourId, colours::print);
        setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (0xff111112));
    }

    juce::Font HS20LookAndFeel::panelFont (float height, bool bold)
    {
        auto f = juce::Font (juce::FontOptions ("Helvetica Neue", height, bold ? juce::Font::bold : juce::Font::plain));
        return f.withHorizontalScale (0.88f);
    }

    juce::Font HS20LookAndFeel::getComboBoxFont (juce::ComboBox&) { return panelFont (12.0f); }
    juce::Font HS20LookAndFeel::getPopupMenuFont() { return panelFont (14.0f, false); }
    juce::Font HS20LookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return panelFont (juce::jmin (12.0f, (float) h * 0.5f)); }

    void HS20LookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
    {
        label.setBounds (6, 1, box.getWidth() - 22, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    //==========================================================================
    void HS20LookAndFeel::drawWaveIcon (juce::Graphics& g, const juce::String& id, juce::Rectangle<float> r)
    {
        juce::Path p;
        const float l = r.getX(), t = r.getY(), w = r.getWidth(), h = r.getHeight(), b = t + h, m = t + h * 0.5f;

        if (id == "~tri")
        {
            p.startNewSubPath (l, b);
            p.lineTo (l + w * 0.25f, t); p.lineTo (l + w * 0.75f, b); p.lineTo (l + w, t);
        }
        else if (id == "~saw")
        {
            p.startNewSubPath (l, b);
            p.lineTo (l + w * 0.5f, t); p.lineTo (l + w * 0.5f, b); p.lineTo (l + w, t); p.lineTo (l + w, b);
        }
        else if (id == "~pulse" || id == "~square" || id == "~narrow")
        {
            const float duty = id == "~narrow" ? 0.15f : (id == "~pulse" ? 0.3f : 0.5f);
            p.startNewSubPath (l, b);
            p.lineTo (l, t); p.lineTo (l + w * duty * 0.5f, t); p.lineTo (l + w * duty * 0.5f, b);
            p.lineTo (l + w * 0.5f, b); p.lineTo (l + w * 0.5f, t); p.lineTo (l + w * (0.5f + duty * 0.5f), t);
            p.lineTo (l + w * (0.5f + duty * 0.5f), b); p.lineTo (l + w, b);
        }
        else if (id == "~noise")
        {
            juce::Random rng (7);
            p.startNewSubPath (l, m);
            for (int i = 1; i <= 9; ++i)
                p.lineTo (l + w * (float) i / 9.0f, t + h * rng.nextFloat());
        }
        else if (id == "~ring")
        {
            // "RING" print as on the original panel
            g.setFont (panelFont (h * 0.95f));
            g.drawText ("RING", r.expanded (6.0f, 0.0f), juce::Justification::centred, false);
            return;
        }

        g.strokePath (p, juce::PathStrokeType (1.1f, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
    }

    void HS20LookAndFeel::drawScrew (juce::Graphics& g, juce::Point<float> c, float r, float angle)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - r + 0.8f, c.y - r + 1.2f, r * 2, r * 2);
        juce::ColourGradient grad (juce::Colour (0xffc9c9c6), c.x - r, c.y - r, juce::Colour (0xff4b4b4a), c.x + r, c.y + r, false);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colour (0xff2a2a2a));
        g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 0.6f);
        juce::Path slot;
        slot.addRectangle (-r * 0.75f, -r * 0.13f, r * 1.5f, r * 0.26f);
        slot.addRectangle (-r * 0.13f, -r * 0.75f, r * 0.26f, r * 1.5f);
        g.setColour (juce::Colour (0xff222222));
        g.fillPath (slot, juce::AffineTransform::rotation (angle).translated (c));
    }

    void HS20LookAndFeel::drawLed (juce::Graphics& g, juce::Point<float> c, float r, float brightness)
    {
        brightness = juce::jlimit (0.0f, 1.0f, brightness);
        if (brightness > 0.02f)
        {
            juce::ColourGradient glow (colours::led.withAlpha (0.55f * brightness), c.x, c.y,
                                       colours::led.withAlpha (0.0f), c.x + r * 4.0f, c.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (c.x - r * 4, c.y - r * 4, r * 8, r * 8);
        }
        g.setColour (juce::Colour (0xff3a0d08).interpolatedWith (colours::led, brightness));
        g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colours::white.withAlpha (0.25f + 0.5f * brightness));
        g.fillEllipse (c.x - r * 0.45f, c.y - r * 0.6f, r * 0.7f, r * 0.5f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 0.6f);
    }

    juce::Path HS20LookAndFeel::heartPath (juce::Rectangle<float> r)
    {
        juce::Path p;
        const float x = r.getX(), y = r.getY(), w = r.getWidth(), h = r.getHeight();
        p.startNewSubPath (x + w * 0.5f, y + h * 0.95f);
        p.cubicTo (x - w * 0.05f, y + h * 0.55f, x + w * 0.05f, y - h * 0.05f, x + w * 0.5f, y + h * 0.25f);
        p.cubicTo (x + w * 0.95f, y - h * 0.05f, x + w * 1.05f, y + h * 0.55f, x + w * 0.5f, y + h * 0.95f);
        p.closeSubPath();
        return p;
    }

    //==========================================================================
    void HS20LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos,
                                            float startAngle, float endAngle, juce::Slider& slider)
    {
        const auto& props = slider.getProperties();
        const auto size = props.getWithDefault ("size", "medium").toString();
        const auto ticks = props.getWithDefault ("ticks", "numbers").toString();
        const auto labels = juce::StringArray::fromTokens (props.getWithDefault ("labels", "").toString(), "|", "");

        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        const auto centre = bounds.getCentre();
        const float margin = ticks == "none" ? 3.0f : (size == "small" ? 11.0f : 14.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - margin;
        const float angle = startAngle + pos * (endAngle - startAngle);

        // ---- printed scale ---------------------------------------------------
        g.setColour (colours::print);
        if (ticks == "numbers" || ticks == "bipolar")
        {
            const int numTicks = 11;
            for (int i = 0; i < numTicks; ++i)
            {
                const float a = startAngle + (float) i / (numTicks - 1) * (endAngle - startAngle);
                const auto inner = centre.getPointOnCircumference (radius + 2.0f, a);
                const auto outer = centre.getPointOnCircumference (radius + (i % 2 == 0 ? 6.0f : 4.5f), a);
                g.drawLine ({ inner, outer }, i % 2 == 0 ? 1.1f : 0.8f);
            }

            if (size != "small")
            {
                g.setFont (panelFont (8.0f, false));
                const bool bipolar = ticks == "bipolar";
                for (int i = 0; i <= 10; i += bipolar ? 5 : 2)
                {
                    const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
                    const auto p = centre.getPointOnCircumference (radius + 11.5f, a);
                    const juce::String txt = bipolar ? (i == 0 ? "-" : (i == 10 ? "+" : "0")) : juce::String (i);
                    g.drawText (txt, juce::Rectangle<float> (16.0f, 9.0f).withCentre (p), juce::Justification::centred, false);
                }
            }
        }
        else if (ticks == "choice" && labels.size() > 1)
        {
            const int n = labels.size();
            for (int i = 0; i < n; ++i)
            {
                const float a = startAngle + (float) i / (float) (n - 1) * (endAngle - startAngle);
                const auto inner = centre.getPointOnCircumference (radius + 2.0f, a);
                const auto outer = centre.getPointOnCircumference (radius + 5.5f, a);
                g.drawLine ({ inner, outer }, 1.1f);

                const auto p = centre.getPointOnCircumference (radius + 13.0f, a);
                if (labels[i].startsWith ("~"))
                    drawWaveIcon (g, labels[i], juce::Rectangle<float> (12.0f, 7.0f).withCentre (p));
                else
                {
                    g.setFont (panelFont (8.5f));
                    g.drawText (labels[i], juce::Rectangle<float> (26.0f, 10.0f).withCentre (p), juce::Justification::centred, false);
                }
            }
        }

        // ---- knob body -------------------------------------------------------
        const auto knob = juce::Rectangle<float> (radius * 2, radius * 2).withCentre (centre);

        // drop shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (knob.translated (1.5f, 2.5f).expanded (1.0f));

        // skirt with knurled grip
        g.setColour (juce::Colour (0xff0c0c0c));
        g.fillEllipse (knob);
        const int ridges = size == "small" ? 18 : 28;
        for (int i = 0; i < ridges; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) i / (float) ridges;
            const float light = 0.5f + 0.5f * std::cos (a + 2.3f);
            g.setColour (juce::Colour (0xff3a3a3a).withAlpha (0.25f + 0.5f * light));
            g.drawLine ({ centre.getPointOnCircumference (radius * 0.80f, a), centre.getPointOnCircumference (radius * 0.98f, a) }, 1.0f);
        }

        // cap
        const auto cap = knob.reduced (radius * 0.2f);
        const auto ringColour = props.contains ("ring") ? juce::Colour ((juce::uint32) (juce::int64) props["ring"]) : juce::Colour();
        juce::ColourGradient capGrad (juce::Colour (0xff3b3b3c), cap.getX(), cap.getY(),
                                      juce::Colour (0xff070707), cap.getRight(), cap.getBottom(), false);
        g.setGradientFill (capGrad);
        g.fillEllipse (cap);
        if (! ringColour.isTransparent())
        {
            g.setColour (ringColour.withAlpha (0.9f));
            g.drawEllipse (cap.reduced (1.0f), 2.2f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawEllipse (cap.reduced (0.5f), 0.8f);

        // sheen
        juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.16f), cap.getCentreX() - radius * 0.3f, cap.getY(),
                                    juce::Colours::transparentWhite, cap.getCentreX(), cap.getCentreY(), true);
        g.setGradientFill (sheen);
        g.fillEllipse (cap.reduced (radius * 0.1f));

        // pointer line, white like the original knobs
        const auto p1 = centre.getPointOnCircumference (radius * 0.18f, angle);
        const auto p2 = centre.getPointOnCircumference (radius * 0.95f, angle);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine ({ p1.translated (0.6f, 0.8f), p2.translated (0.6f, 0.8f) }, size == "small" ? 2.0f : 2.6f);
        g.setColour (juce::Colour (0xfff4f1e8));
        g.drawLine ({ p1, p2 }, size == "small" ? 1.6f : 2.1f);
    }

    void HS20LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float, float, juce::Slider::SliderStyle, juce::Slider&)
    {
        // Performance wheel: a ribbed black wheel in a slot.
        auto slot = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        g.setColour (juce::Colour (0xff050505));
        g.fillRoundedRectangle (slot, 4.0f);
        g.setColour (juce::Colour (0xff2c2c2c));
        g.drawRoundedRectangle (slot, 4.0f, 1.0f);

        auto wheel = slot.reduced (3.0f, 2.0f);
        juce::ColourGradient grad (juce::Colour (0xff050505), wheel.getX(), wheel.getY(),
                                   juce::Colour (0xff050505), wheel.getX(), wheel.getBottom(), false);
        grad.addColour (0.5, juce::Colour (0xff383838));
        g.setGradientFill (grad);
        g.fillRoundedRectangle (wheel, 3.0f);

        // ribs move with the wheel position
        const float offset = sliderPos - (float) y;
        g.saveState();
        g.reduceClipRegion (wheel.toNearestInt());
        for (float ry = wheel.getY() + std::fmod (offset, 6.0f) - 6.0f; ry < wheel.getBottom(); ry += 6.0f)
        {
            const float rel = (ry - wheel.getY()) / wheel.getHeight();
            const float shade = std::sin (rel * juce::MathConstants<float>::pi);
            g.setColour (juce::Colours::white.withAlpha (0.04f + 0.10f * shade));
            g.drawLine (wheel.getX() + 1.0f, ry, wheel.getRight() - 1.0f, ry, 1.0f);
        }
        g.restoreState();

        g.setColour (colours::print.withAlpha (0.9f));
        g.fillRect (wheel.getX() + 2.0f, sliderPos - 1.0f, wheel.getWidth() - 4.0f, 2.0f);
    }

    void HS20LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();

        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f), 3.0f);

        juce::ColourGradient grad (juce::Colour (down ? 0xff151516 : 0xff3a3b3d), r.getX(), r.getY(),
                                   juce::Colour (down ? 0xff252627 : 0xff17181a), r.getX(), r.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (r, 3.0f);

        g.setColour (highlighted ? colours::print.withAlpha (0.5f) : juce::Colours::black);
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawLine (r.getX() + 3.0f, r.getY() + 1.0f, r.getRight() - 3.0f, r.getY() + 1.0f, 1.0f);

        if (on && b.getProperties().getWithDefault ("led", true))
        {
            g.setColour (colours::accent);
            g.fillRoundedRectangle (r.getX() + 4.0f, r.getBottom() - 3.5f, r.getWidth() - 8.0f, 2.0f, 1.0f);
        }
    }

    void HS20LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
    {
        g.setFont (getTextButtonFont (b, b.getHeight()));
        g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId)
                         .withMultipliedAlpha (b.isEnabled() ? 1.0f : 0.5f));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 1), juce::Justification::centred, 1);
    }

    void HS20LookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
    {
        auto r = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        g.setColour (juce::Colour (0xff0b0b0c));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (box.hasKeyboardFocus (false) ? colours::accent : juce::Colour (0xff3a3a3c));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);

        juce::Path arrow;
        const float ax = (float) width - 12.0f, ay = (float) height * 0.5f;
        arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
        g.setColour (colours::printDim);
        g.fillPath (arrow);
    }
}
