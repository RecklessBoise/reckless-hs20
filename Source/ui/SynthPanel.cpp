#include "SynthPanel.h"
#include "HS20LookAndFeel.h"
#include "PanelTexture.h"
#include "../PluginProcessor.h"

namespace hs20::ui
{
    using Size = LabelledKnob::Size;

    //==========================================================================
    LabelledKnob::LabelledKnob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText,
                                Size s, const juce::String& ticks, const juce::String& choiceLabels)
        : size (s), label (labelText), attachment (state, paramId, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.22f, juce::MathConstants<float>::pi * 2.78f, true);
        slider.setMouseDragSensitivity (220);
        slider.setVelocityBasedMode (false);
        slider.getProperties().set ("size", s == Size::big ? "big" : (s == Size::medium ? "medium" : "small"));
        slider.getProperties().set ("ticks", ticks);
        slider.getProperties().set ("labels", choiceLabels);
        if (auto* p = state.getParameter (paramId))
            slider.setTooltip (p->getName (64));
        addAndMakeVisible (slider);
    }

    void LabelledKnob::resized()
    {
        auto r = getLocalBounds();
        slider.setBounds (r.removeFromTop (getHeight() - labelHeight));
    }

    juce::Point<float> LabelledKnob::getKnobCentre() const
    {
        return getBounds().toFloat().getTopLeft() + slider.getBounds().toFloat().getCentre();
    }

    void LabelledKnob::paint (juce::Graphics& g)
    {
        g.setColour (colours::print);
        g.setFont (HS20LookAndFeel::panelFont (8.0f));
        g.drawFittedText (label, getLocalBounds().removeFromBottom (labelHeight + 1), juce::Justification::centredTop, 1, 0.8f);
    }

    //==========================================================================
    ParamToggle::ParamToggle (juce::RangedAudioParameter& param, juce::StringArray t)
        : texts (std::move (t)),
          attachment (param, [this] (float v)
          {
              current = juce::jlimit (0, texts.size() - 1, juce::roundToInt (v));
              setButtonText (texts[current]);
              setToggleState (current > 0, juce::dontSendNotification);
          })
    {
        setTooltip (param.getName (64));
        onClick = [this] { attachment.setValueAsCompleteGesture ((float) ((current + 1) % texts.size())); };
        attachment.sendInitialUpdate();
    }

    //==========================================================================
    SynthPanel::SynthPanel (HS20AudioProcessor& p)
        : processor (p),
          patchBay (p.apvts),
          presetBar (p.presetManager),
          keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
    {
        using namespace hs20::ids;
        const int x[] = { 22, 106, 190, 274, 358, 442, 526, 610, 694 };
        const int top = 58, mid = 330, bottom = 566;
        auto col = [&] (int a, int b, int y0, int y1) { return juce::Rectangle<int> (x[a], y0, x[b] - x[a], y1 - y0); };

        const juce::String scales = "32'|16'|8'|4'";

        addSection ("VOLTAGE CONTROLLED\nOSCILLATOR 1", col (0, 1, top, mid), {
            addKnob (vco1Wave, "WAVE FORM", Size::big, "choice", "~tri|~saw|~pulse|~noise"),
            addKnob (vco1Pw, "PW", Size::small),
            addKnob (vco1Scale, "SCALE", Size::big, "choice", scales) });

        addSection ("VOLTAGE CONTROLLED\nOSCILLATOR 2", col (1, 2, top, mid), {
            addKnob (vco2Wave, "WAVE FORM", Size::big, "choice", "~saw|~square|~narrow|~ring"),
            addKnob (vco2Pitch, "PITCH", Size::small, "bipolar"),
            addKnob (vco2Scale, "SCALE", Size::big, "choice", scales) });

        addSection ("VCO MIXER", col (2, 3, top, mid), {
            addKnob (vco1Level, "VCO 1 LEVEL", Size::medium),
            addKnob (vco2Level, "VCO 2 LEVEL", Size::medium) });

        addSection ("VOLTAGE CONTROLLED\nHIGHPASS FILTER", col (3, 4, top, mid), {
            addKnob (hpfCutoff, "CUTOFF FREQUENCY", Size::big),
            addKnob (hpfPeak, "PEAK", Size::medium) });

        addSection ("VOLTAGE CONTROLLED\nLOWPASS FILTER", col (4, 5, top, mid), {
            addKnob (lpfCutoff, "CUTOFF FREQUENCY", Size::big),
            addKnob (lpfPeak, "PEAK", Size::medium) });

        addSection ("VOLTAGE CONTROLLED AMPLIFIER", col (5, 7, top, mid), {
            addKnob (vcaGain, "INITIAL GAIN", Size::small),
            addKnob (drive, "DRIVE", Size::small) }, true);

        addSection ("ENVELOPE\nGENERATOR 2", col (7, 8, top, bottom), {
            addKnob (eg2Hold, "HOLD TIME", Size::medium),
            addKnob (eg2Attack, "ATTACK TIME", Size::medium),
            addKnob (eg2Decay, "DECAY TIME", Size::medium),
            addKnob (eg2Sustain, "SUSTAIN LEVEL", Size::medium),
            addKnob (eg2Release, "RELEASE TIME", Size::medium) });

        addSection ("PORTAMENTO", col (0, 1, mid, bottom), {
            addKnob (portamento, "TIME", Size::medium),
            addKnob (bendRange, "BEND RANGE", Size::small, "none") });

        addSection ("MASTER TUNE", col (1, 2, mid, bottom), {
            addKnob (masterTune, "TUNE", Size::medium, "bipolar"),
            addKnob (drift, "ANALOG DRIFT", Size::small) });

        addSection ("FREQUENCY\nMODULATION", col (2, 3, mid, bottom), {
            addKnob (fmMg, "MG/T.EXT", Size::medium),
            addKnob (fmEg1, "EG1/EXT", Size::medium) });

        addSection ("CUTOFF FREQUENCY\nMODULATION", col (3, 4, mid, bottom), {
            addKnob (hpfModMg, "MG/T.EXT", Size::medium),
            addKnob (hpfModEg2, "EG2/EXT", Size::medium) });

        addSection ("CUTOFF FREQUENCY\nMODULATION", col (4, 5, mid, bottom), {
            addKnob (lpfModMg, "MG/T.EXT", Size::medium),
            addKnob (lpfModEg2, "EG2/EXT", Size::medium) });

        addSection ("MODULATION\nGENERATOR", col (5, 6, mid, bottom), {
            addKnob (mgWave, "WAVE FORM", Size::medium, "none"),
            addKnob (mgFreq, "FREQUENCY", Size::medium) });

        addSection ("ENVELOPE\nGENERATOR 1", col (6, 7, mid, bottom), {
            addKnob (eg1Delay, "DELAY TIME", Size::small),
            addKnob (eg1Attack, "ATTACK TIME", Size::small),
            addKnob (eg1Release, "RELEASE TIME", Size::small) });

        // ---- right side ----------------------------------------------------
        addSection ({}, { 1092, 56, 90, 92 }, { addKnob (volume, "VOLUME", Size::big) });

        std::vector<LabelledKnob*> cableKnobs;
        for (int i = 0; i < numPatchCables; ++i)
        {
            auto* k = addKnob (patchAmt (i), "CABLE " + juce::String (i + 1), Size::small, "bipolar");
            k->slider.getProperties().set ("ring", (juce::int64) colours::cable (i).getARGB());
            cableKnobs.push_back (k);
        }
        addSection ({}, { 704, 398, 476, 66 }, cableKnobs, true);

        addSection ("EXTERNAL SIGNAL PROCESSOR", { 704, 466, 476, 100 }, {
            addKnob (espMode, "MODE", Size::small, "choice", "OFF|AUD|PIT|A+P"),
            addKnob (espLevel, "SIGNAL LEVEL", Size::small),
            addKnob (espLowCut, "LOW CUT FREQ", Size::small),
            addKnob (espHighCut, "HIGH CUT FREQ", Size::small),
            addKnob (espCvAdjust, "CV ADJUST", Size::small, "bipolar"),
            addKnob (espThreshold, "THRESHOLD LEVEL", Size::small) }, true);

        addAndMakeVisible (patchBay);
        addAndMakeVisible (presetBar);

        auto addToggle = [this] (const juce::String& id, juce::StringArray texts, const juce::String& tip)
        {
            auto* param = processor.apvts.getParameter (id);
            jassert (param != nullptr);
            auto t = std::make_unique<ParamToggle> (*param, std::move (texts));
            t->setTooltip (tip);
            addAndMakeVisible (*t);
            toggles.push_back (std::move (t));
        };
        addToggle (voiceMode, { "MONO", "POLY" }, "Mono (faithful) or 8-voice poly");
        addToggle (filterModel, { "REV 1", "REV 2" }, "Filter model: Rev 1 (early, aggressive) or Rev 2 (late, smoother)");
        addToggle (legato, { "MULTI TRIG", "SINGLE TRIG" }, "Single trigger: legato notes do not retrigger the envelopes");
        addToggle (quality, { "2X", "4X" }, "Oversampling quality");

        addAndMakeVisible (sizeButton);
        sizeButton.setTooltip ("Plug-in window size");
        sizeButton.onClick = [this] { showSizeMenu(); };

        addAndMakeVisible (keyboard);
        keyboard.setAvailableRange (36, 96);
        keyboard.setOctaveForMiddleC (4);
        keyboard.setScrollButtonsVisible (true);
        keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff3f1ea));
        keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff111111));
        keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff5a5a5a));
        keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, colours::accent.withAlpha (0.18f));
        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, colours::accent.withAlpha (0.55f));
        keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.5f));
        keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, juce::Colour (0xff1b1b1c));
        keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, colours::printDim);

        for (auto* wheel : { &pitchWheel, &modWheel })
        {
            wheel->setSliderStyle (juce::Slider::LinearVertical);
            wheel->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            addAndMakeVisible (wheel);
        }
        pitchWheel.setRange (-1.0, 1.0);
        pitchWheel.setValue (0.0, juce::dontSendNotification);
        pitchWheel.setTooltip ("Pitch bend (springs back)");
        pitchWheel.onValueChange = [this] { processor.engine.uiPitchBend.store ((float) pitchWheel.getValue()); };
        pitchWheel.onDragEnd = [this] { pitchWheel.setValue (0.0); };
        modWheel.setRange (0.0, 1.0);
        modWheel.setTooltip ("Modulation wheel (patch source WHEEL)");
        modWheel.onValueChange = [this] { processor.engine.uiModWheel.store ((float) modWheel.getValue()); };

        setSize (baseWidth, baseHeight);
        startTimerHz (30);
    }

    SynthPanel::~SynthPanel()
    {
        stopTimer();
    }

    LabelledKnob* SynthPanel::addKnob (const juce::String& paramId, const juce::String& label, Size size,
                                       const juce::String& ticks, const juce::String& labels)
    {
        knobs.push_back (std::make_unique<LabelledKnob> (processor.apvts, paramId, label, size, ticks, labels));
        auto* k = knobs.back().get();
        addAndMakeVisible (k);
        return k;
    }

    void SynthPanel::addSection (const juce::String& title, juce::Rectangle<int> bounds, std::vector<LabelledKnob*> ks, bool horizontal)
    {
        sections.push_back ({ title, bounds, std::move (ks), horizontal });
    }

    void SynthPanel::layoutSection (const Section& s)
    {
        auto area = s.bounds.withTrimmedTop (s.title.isEmpty() ? 0 : (s.title.containsChar ('\n') ? 28 : 18)).reduced (1, 2);

        if (s.title.startsWith ("VOLTAGE CONTROLLED AMP"))
        {
            scopeArea = area.removeFromTop (150).reduced (10, 6).toFloat();
            area.removeFromTop (4);
        }

        const int n = (int) s.knobs.size();
        if (n == 0)
            return;

        for (int i = 0; i < n; ++i)
        {
            auto* k = s.knobs[(size_t) i];
            const int w = juce::jmin (LabelledKnob::knobSize (Size::big), s.bounds.getWidth() / (s.horizontal ? n : 1));
            const int h = k->preferredHeight();
            juce::Rectangle<int> slot;
            if (s.horizontal)
                slot = area.withWidth (area.getWidth() / n).withX (area.getX() + i * area.getWidth() / n);
            else
                slot = area.withHeight (area.getHeight() / n).withY (area.getY() + i * area.getHeight() / n);
            k->setBounds (juce::Rectangle<int> (juce::jmax (w, LabelledKnob::knobSize (Size::small)), h).withCentre (slot.getCentre()));
        }
    }

    void SynthPanel::resized()
    {
        for (const auto& s : sections)
            layoutSection (s);

        patchBay.setBounds (704, 150, 476, 248);

        auto header = juce::Rectangle<int> (0, 0, baseWidth, 52).reduced (14, 10);
        header.removeFromLeft (232);
        sizeButton.setBounds (header.removeFromRight (52));
        header.removeFromRight (6);
        // right-to-left: quality, trigger mode, filter model, voice mode
        const std::pair<int, int> toggleLayout[] = { { 3, 38 }, { 2, 84 }, { 1, 54 }, { 0, 54 } };
        for (const auto& [index, width] : toggleLayout)
        {
            toggles[(size_t) index]->setBounds (header.removeFromRight (width));
            header.removeFromRight (4);
        }
        header.removeFromRight (8);
        presetBar.setBounds (header);

        // performance wheels + keyboard
        pitchWheel.setBounds (30, 590, 26, 70);
        modWheel.setBounds (74, 590, 26, 70);
        keyboard.setBounds (124, 580, baseWidth - 124 - 14, 92);
        keyboard.setKeyWidth ((float) (keyboard.getWidth() - 2 * 12) / 22.0f);
        keyboard.setLowestVisibleKey (48);

        mgLedPos = { 512.0f, 338.0f, 0, 0 };
        egLedPos = { 596.0f, 338.0f, 0, 0 };
        espLedPos = { 1166.0f, 474.0f, 0, 0 };
        peakLedPos = { 1172.0f, 64.0f, 0, 0 };

        // Knob positions drive the paint wear rings in the texture.
        PanelTexture::Style style;
        style.seed = 20;
        for (auto& k : knobs)
            style.scuffs.push_back (k->getKnobCentre());
        texture = PanelTexture::get (baseWidth, baseHeight, style);
    }

    //==========================================================================
    void SynthPanel::paintLogo (juce::Graphics& g, juce::Rectangle<float> r)
    {
        g.setColour (colours::accent);
        g.setFont (HS20LookAndFeel::panelFont (11.0f).withExtraKerningFactor (0.35f));
        g.drawText ("RECKLESS", r.removeFromTop (13.0f), juce::Justification::left);

        auto line = r.removeFromTop (22.0f);
        const auto big = HS20LookAndFeel::panelFont (26.0f).withHorizontalScale (1.0f).withExtraKerningFactor (0.04f);
        g.setFont (big);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawText ("HS20", line.translated (1.0f, 1.5f), juce::Justification::left);
        g.setColour (colours::print);
        g.drawText ("HS20", line, juce::Justification::left);

        g.setFont (HS20LookAndFeel::panelFont (7.5f).withExtraKerningFactor (0.2f));
        g.setColour (colours::printDim);
        g.drawText ("MONOPHONIC SYNTHESIZER", line.withTrimmedLeft (78.0f).withTrimmedTop (10.0f), juce::Justification::left);
    }

    void SynthPanel::paintSignalFlow (juce::Graphics& g, juce::Rectangle<float> r)
    {
        const char* blocks[] = { "VCO 1", "VCO 2", "MIXER", "HP FILTER", "LP FILTER", "AMPLIFIER" };
        const int n = 6;
        const float gap = 12.0f;
        const float w = (r.getWidth() - gap * (n - 1)) / n;
        const float h = 26.0f;
        const float y = r.getY() + 8.0f;

        g.setFont (HS20LookAndFeel::panelFont (7.6f));
        for (int i = 0; i < n; ++i)
        {
            auto box = juce::Rectangle<float> (r.getX() + (float) i * (w + gap), y, w, h);
            g.setColour (colours::print.withAlpha (0.9f));
            g.drawRect (box, 1.0f);
            g.drawFittedText (blocks[i], box.reduced (2.0f).toNearestInt(),
                              juce::Justification::centred, 2, 0.7f);
            if (i < n - 1)
            {
                juce::Path arrow;
                const float ax = box.getRight(), ay = box.getCentreY();
                arrow.addArrow ({ ax + 1.0f, ay, ax + gap - 1.0f, ay }, 1.0f, 5.0f, 4.0f);
                g.fillPath (arrow);
            }
        }

        // little jacks under the flow, as decorations: the real ones live in the patch panel
        g.setFont (HS20LookAndFeel::panelFont (7.0f, false));
        g.setColour (colours::printDim);
        g.drawText ("DRAG FROM OUT TO IN TO PATCH  /  CLICK A PLUGGED IN JACK TO PULL THE CABLE",
                    juce::Rectangle<float> (r.getX(), y + h + 10.0f, r.getWidth(), 10.0f), juce::Justification::centred);
    }

    void SynthPanel::paintScope (juce::Graphics& g, juce::Rectangle<float> r)
    {
        // bezel
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.expanded (3.0f).translated (0.0f, 1.0f), 6.0f);
        juce::ColourGradient bezel (juce::Colour (0xff6b6d70), r.getX(), r.getY() - 3.0f, juce::Colour (0xff1c1d1e), r.getX(), r.getBottom() + 3.0f, false);
        g.setGradientFill (bezel);
        g.drawRoundedRectangle (r.expanded (2.0f), 5.0f, 2.0f);

        juce::ColourGradient glass (juce::Colour (0xff0d1410), r.getCentreX(), r.getCentreY(), colours::glass, r.getX(), r.getY(), true);
        g.setGradientFill (glass);
        g.fillRoundedRectangle (r, 4.0f);

        g.setColour (colours::accent.withAlpha (0.08f));
        for (int i = 1; i < 6; ++i)
            g.drawVerticalLine (juce::roundToInt (r.getX() + r.getWidth() * (float) i / 6.0f), r.getY(), r.getBottom());
        for (int i = 1; i < 4; ++i)
            g.drawHorizontalLine (juce::roundToInt (r.getY() + r.getHeight() * (float) i / 4.0f), r.getX(), r.getRight());

        // amplifier symbol, as printed in the original VCA box
        juce::Path amp;
        const auto c = juce::Point<float> (r.getX() + 16.0f, r.getY() + 12.0f);
        amp.addTriangle (c.x - 7.0f, c.y - 6.0f, c.x - 7.0f, c.y + 6.0f, c.x + 6.0f, c.y);
        g.setColour (colours::print.withAlpha (0.35f));
        g.strokePath (amp, juce::PathStrokeType (1.0f));

        juce::Path trace;
        const auto n = scopeTrace.size();
        for (size_t i = 0; i < n; ++i)
        {
            const float px = r.getX() + 2.0f + (r.getWidth() - 4.0f) * (float) i / (float) (n - 1);
            const float py = r.getCentreY() - juce::jlimit (-1.0f, 1.0f, scopeTrace[i]) * (r.getHeight() * 0.42f);
            if (i == 0) trace.startNewSubPath (px, py);
            else        trace.lineTo (px, py);
        }
        g.saveState();
        g.reduceClipRegion (r.toNearestInt());
        g.setColour (juce::Colour (0xffff7a3c).withAlpha (0.18f));
        g.strokePath (trace, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved));
        g.setColour (juce::Colour (0xffffa070));
        g.strokePath (trace, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved));
        g.restoreState();

        // reflection
        juce::ColourGradient refl (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false);
        g.setGradientFill (refl);
        g.fillRoundedRectangle (r.withHeight (r.getHeight() * 0.5f), 4.0f);

        g.setColour (colours::print);
        g.setFont (HS20LookAndFeel::panelFont (8.0f));
        g.drawText ("EG 2 & EXT", r.withY (r.getBottom() + 3.0f).withHeight (10.0f), juce::Justification::centred);
    }

    void SynthPanel::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.fillAll (juce::Colour (0xff0b0b0c));

        if (texture.isValid())
            g.drawImage (texture, bounds, juce::RectanglePlacement::stretchToFit);

        // Header and keyboard bed: darker, heavier coat over the same metal.
        auto header = bounds.withHeight (52.0f);
        auto bed = bounds.withTop (572.0f);
        g.setColour (juce::Colour (0xff09090a).withAlpha (0.78f));
        g.fillRect (header);
        g.fillRect (bed);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawHorizontalLine (52, 0.0f, bounds.getRight());
        g.drawHorizontalLine (571, 0.0f, bounds.getRight());
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawHorizontalLine (53, 0.0f, bounds.getRight());
        g.drawHorizontalLine (572, 0.0f, bounds.getRight());

        paintLogo (g, { 22.0f, 8.0f, 220.0f, 38.0f });

        // ---- printed sections ------------------------------------------
        g.setColour (colours::print.withAlpha (0.88f));
        for (const auto& s : sections)
        {
            if (s.title.isEmpty())
                continue;

            const auto b = s.bounds.toFloat();
            g.setColour (colours::print.withAlpha (0.85f));
            g.drawRect (b, 1.1f);
            g.setColour (colours::print);
            g.setFont (HS20LookAndFeel::panelFont (s.title.containsChar ('\n') ? 8.2f : 8.6f));
            g.drawFittedText (s.title, b.withHeight (s.title.containsChar ('\n') ? 26.0f : 17.0f).reduced (2.0f, 1.0f).toNearestInt(),
                              juce::Justification::centred, 2, 0.75f);
        }

        // VCO group divider below the oscillators, like the panel's double line
        g.setColour (colours::print.withAlpha (0.85f));
        g.drawRect (juce::Rectangle<float> (704.0f, 56.0f, 476.0f, 92.0f), 1.1f);
        paintSignalFlow (g, { 712.0f, 66.0f, 372.0f, 70.0f });
        g.drawRect (juce::Rectangle<float> (704.0f, 148.0f, 476.0f, 318.0f), 1.1f);

        // knob markings for the MG waveform (ramp down / triangle / ramp up)
        const auto mgIt = std::find_if (knobs.begin(), knobs.end(), [] (auto& k) { return k->slider.getTooltip() == "MG Waveform"; });
        if (mgIt != knobs.end())
        {
            auto* mg = mgIt->get();
            const auto c = mg->getKnobCentre();
            g.setColour (colours::print);
            HS20LookAndFeel::drawWaveIcon (g, "~saw", { c.x - 32.0f, c.y + 10.0f, 11.0f, 6.0f });
            HS20LookAndFeel::drawWaveIcon (g, "~tri", { c.x - 5.5f, c.y - 33.0f, 11.0f, 6.0f });
            juce::Path rampUp;
            rampUp.startNewSubPath (c.x + 21.0f, c.y + 16.0f);
            rampUp.lineTo (c.x + 21.0f, c.y + 10.0f);
            rampUp.lineTo (c.x + 32.0f, c.y + 16.0f);
            rampUp.lineTo (c.x + 32.0f, c.y + 10.0f);
            g.strokePath (rampUp, juce::PathStrokeType (1.1f));
        }

        paintScope (g, scopeArea);

        // LEDs
        HS20LookAndFeel::drawLed (g, mgLedPos.getPosition(), 3.2f, processor.engine.mgLed.load());
        HS20LookAndFeel::drawLed (g, egLedPos.getPosition(), 3.2f, processor.engine.egLed.load());
        HS20LookAndFeel::drawLed (g, espLedPos.getPosition(), 3.2f, processor.engine.espLed.load());
        HS20LookAndFeel::drawLed (g, peakLedPos.getPosition(), 3.2f, processor.engine.peakLevel.load() > 0.95f ? 1.0f : 0.0f);
        g.setFont (HS20LookAndFeel::panelFont (6.5f));
        g.setColour (colours::print);
        g.drawText ("PEAK", juce::Rectangle<float> (1158.0f, 70.0f, 28.0f, 8.0f), juce::Justification::centred);

        // wheel labels
        g.setFont (HS20LookAndFeel::panelFont (7.5f));
        g.drawText ("PITCH", juce::Rectangle<float> (18.0f, 662.0f, 50.0f, 10.0f), juce::Justification::centred);
        g.drawText ("MOD", juce::Rectangle<float> (62.0f, 662.0f, 50.0f, 10.0f), juce::Justification::centred);
        g.drawText ("BEND", juce::Rectangle<float> (18.0f, 578.0f, 50.0f, 10.0f), juce::Justification::centred);
        g.drawText ("WHEEL", juce::Rectangle<float> (62.0f, 578.0f, 50.0f, 10.0f), juce::Justification::centred);

        // corner screws
        const float screwR = 5.0f;
        for (auto pt : { juce::Point<float> (9.0f, 62.0f), juce::Point<float> (1191.0f, 62.0f),
                         juce::Point<float> (9.0f, 560.0f), juce::Point<float> (1191.0f, 560.0f),
                         juce::Point<float> (700.0f, 62.0f), juce::Point<float> (700.0f, 560.0f) })
            HS20LookAndFeel::drawScrew (g, pt, screwR, pt.x * 0.013f + pt.y * 0.007f);
    }

    void SynthPanel::timerCallback()
    {
        auto& e = processor.engine;
        constexpr int window = (int) std::tuple_size<decltype (scopeTrace)>::value;
        const int size = dsp::SynthEngine::scopeSize;
        const int end = e.scopeWritePos.load (std::memory_order_relaxed);

        // find a rising zero crossing in the older half so the trace is stable
        int start = (end - window - 1 + size * 2) % size;
        for (int back = 0; back < size - window * 2; ++back)
        {
            const int i = (end - window - back + size * 2) % size;
            const int prev = (i - 1 + size) % size;
            if (e.scope[(size_t) prev].load (std::memory_order_relaxed) < 0.0f && e.scope[(size_t) i].load (std::memory_order_relaxed) >= 0.0f)
            {
                start = i;
                break;
            }
        }

        float peak = 0.05f;
        for (int i = 0; i < window; ++i)
        {
            scopeTrace[(size_t) i] = e.scope[(size_t) ((start + i) % size)].load (std::memory_order_relaxed);
            peak = std::max (peak, std::abs (scopeTrace[(size_t) i]));
        }
        for (auto& v : scopeTrace)
            v /= peak;  // auto gain, like a scope with its sensitivity knob ridden

        repaint (scopeArea.expanded (6.0f).toNearestInt());
        for (auto r : { mgLedPos, egLedPos, espLedPos, peakLedPos })
            repaint (juce::Rectangle<float> (28.0f, 28.0f).withCentre (r.getPosition()).toNearestInt());

        if (getCurrentScale)
            sizeButton.setButtonText (juce::String (juce::roundToInt (getCurrentScale() * 100.0f)) + "%");
    }

    void SynthPanel::showSizeMenu()
    {
        juce::PopupMenu menu;
        menu.addSectionHeader ("Window size");
        const float current = getCurrentScale ? getCurrentScale() : 1.0f;
        for (int pct : { 50, 75, 100, 125, 150, 175, 200 })
            menu.addItem (pct, juce::String (pct) + "%", true, std::abs (current * 100.0f - (float) pct) < 1.0f);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (sizeButton),
                            [safe = juce::Component::SafePointer<SynthPanel> (this)] (int result)
                            {
                                if (safe != nullptr && result > 0 && safe->onScaleChosen)
                                    safe->onScaleChosen ((float) result / 100.0f);
                            });
    }
}
