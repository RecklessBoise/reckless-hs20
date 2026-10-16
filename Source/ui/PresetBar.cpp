#include "PresetBar.h"
#include "HS20LookAndFeel.h"

namespace hs20::ui
{
    void HeartButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        auto r = getLocalBounds().toFloat().reduced ((float) getHeight() * 0.18f);
        r = r.withSizeKeepingCentre (r.getHeight() * 1.1f, r.getHeight());
        const auto heart = HS20LookAndFeel::heartPath (r.translated (0.0f, down ? 1.0f : 0.0f));

        if (getToggleState())
        {
            g.setColour (colours::accent.withAlpha (0.35f));
            g.strokePath (heart, juce::PathStrokeType (4.0f));
            g.setColour (colours::accent);
            g.fillPath (heart);
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillEllipse (r.getX() + r.getWidth() * 0.22f, r.getY() + r.getHeight() * 0.2f, r.getWidth() * 0.18f, r.getHeight() * 0.14f);
        }
        else
        {
            g.setColour (highlighted ? colours::accent : colours::printDim);
            g.strokePath (heart, juce::PathStrokeType (1.6f));
        }
    }

    //==========================================================================
    DiceButton::DiceButton() : juce::Button ("Random")
    {
        setTooltip ("Random patch: click to roll, right-click to choose the kind of sound");
    }

    void DiceButton::mouseDown (const juce::MouseEvent& e)
    {
        if (e.mods.isPopupMenu() || e.mods.isCommandDown())
        {
            if (onStyleMenu)
                onStyleMenu();
            return;
        }
        juce::Button::mouseDown (e);
    }

    void DiceButton::roll()
    {
        tumbleFrames = 8;
        startTimerHz (40);
    }

    void DiceButton::timerCallback()
    {
        int next = face;
        while (next == face)
            next = 1 + rng.nextInt (6);
        face = next;
        angle = tumbleFrames > 1 ? (rng.nextFloat() - 0.5f) * 0.9f : 0.0f;
        repaint();

        if (--tumbleFrames <= 0)
        {
            stopTimer();
            angle = 0.0f;
            repaint();
            if (onRolled)
                onRolled();
        }
    }

    void DiceButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
    {
        const float size = (float) juce::jmin (getWidth(), getHeight()) - 6.0f;
        auto body = juce::Rectangle<float> (size, size).withCentre (getLocalBounds().toFloat().getCentre())
                                                       .translated (0.0f, down ? 1.0f : 0.0f);
        const auto centre = body.getCentre();
        const auto spin = juce::AffineTransform::rotation (angle, centre.x, centre.y);
        const float corner = size * 0.22f;

        juce::Path cube;
        cube.addRoundedRectangle (body, corner);

        // drop shadow, then an ivory body with a bevel, like a worn bakelite die
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillPath (cube, spin.translated (1.0f, 2.0f));

        juce::ColourGradient ivory (juce::Colour (0xfff6f1e4), body.getX(), body.getY(),
                                    juce::Colour (0xffb9b2a2), body.getRight(), body.getBottom(), false);
        g.setGradientFill (ivory);
        g.fillPath (cube, spin);
        g.setColour (highlighted ? colours::accent : juce::Colour (0xff4a463e));
        g.strokePath (cube, juce::PathStrokeType (highlighted ? 1.6f : 1.0f), spin);

        // pip layout on a 3x3 grid
        static const int layouts[6][9] = {
            { 0,0,0, 0,1,0, 0,0,0 }, { 1,0,0, 0,0,0, 0,0,1 }, { 1,0,0, 0,1,0, 0,0,1 },
            { 1,0,1, 0,0,0, 1,0,1 }, { 1,0,1, 0,1,0, 1,0,1 }, { 1,0,1, 1,0,1, 1,0,1 }
        };
        const float step = size * 0.27f, pip = size * 0.17f;
        for (int i = 0; i < 9; ++i)
        {
            if (layouts[face - 1][i] == 0)
                continue;
            const auto p = juce::Point<float> (centre.x + (float) (i % 3 - 1) * step, centre.y + (float) (i / 3 - 1) * step)
                               .transformedBy (spin);
            // the single pip is red, as on classic dice
            g.setColour (face == 1 ? colours::accent : juce::Colour (0xff141414));
            g.fillEllipse (juce::Rectangle<float> (pip, pip).withCentre (p));
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (pip * 0.35f, pip * 0.3f).withCentre (p.translated (-pip * 0.15f, pip * 0.2f)));
        }
    }

    //==========================================================================
    PresetBar::PresetBar (PresetManager& manager) : presets (manager)
    {
        addAndMakeVisible (bankBox);
        bankBox.setTooltip ("Preset bank: browse a category or only the presets you liked");
        bankBox.onChange = [this]
        {
            if (bankBox.getText().isNotEmpty())
                presets.setCurrentBank (bankBox.getText());
        };

        for (auto* b : { &prevButton, &nextButton, &nameButton, &saveButton, &menuButton })
            addAndMakeVisible (b);

        prevButton.onClick = [this] { presets.loadPrevious(); };
        nextButton.onClick = [this] { presets.loadNext(); };
        nameButton.onClick = [this] { showBrowserMenu(); };
        saveButton.onClick = [this] { showSaveDialog(); };
        menuButton.onClick = [this] { showOptionsMenu(); };
        nameButton.setTooltip ("Browse presets (hearts mark the ones you liked)");
        saveButton.setTooltip ("Save the current sound as a user preset");
        menuButton.setTooltip ("Liked bank: export, import, open folder");

        addAndMakeVisible (likeButton);
        likeButton.setTooltip ("Like this preset: it is added to the Liked bank");
        likeButton.onClick = [this] { presets.likeCurrent(); };

        addAndMakeVisible (diceButton);
        diceButton.onClick = [this] { diceButton.roll(); };
        diceButton.onRolled = [this] { presets.loadRandom (diceButton.style); };
        diceButton.onStyleMenu = [this] { showDiceMenu(); };

        addAndMakeVisible (likedCount);
        likedCount.setFont (HS20LookAndFeel::panelFont (10.0f));
        likedCount.setColour (juce::Label::textColourId, colours::printDim);
        likedCount.setJustificationType (juce::Justification::centredLeft);

        presets.addChangeListener (this);
        refresh();
    }

    PresetBar::~PresetBar()
    {
        presets.removeChangeListener (this);
    }

    void PresetBar::refresh()
    {
        const auto banks = presets.getBanks();
        if (bankBox.getNumItems() != banks.size())
        {
            bankBox.clear (juce::dontSendNotification);
            bankBox.addItemList (banks, 1);
        }
        bankBox.setText (presets.getCurrentBank(), juce::dontSendNotification);

        const int index = presets.getCurrentIndex();
        juce::String label = presets.getCurrentName();
        if (juce::isPositiveAndBelow (index, (int) presets.getPresets().size()))
            label = presets.getPresets()[(size_t) index].category.toUpperCase() + "  /  " + label;
        else if (presets.isRandomPatch())
            label = "RANDOM " + presets.getRandomStyle().toUpperCase() + "  /  " + label;
        nameButton.setButtonText (label);
        likeButton.setToggleState (presets.isLiked (index), juce::dontSendNotification);
        likeButton.setEnabled (index >= 0 || presets.isRandomPatch());
        likedCount.setText (juce::String (presets.getNumLiked()), juce::dontSendNotification);
        repaint();
    }

    void PresetBar::paint (juce::Graphics& g)
    {
        // recessed LCD-like display behind the preset name
        auto r = nameButton.getBounds().toFloat().expanded (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (r.translated (0.0f, -1.0f), 4.0f);
    }

    void PresetBar::resized()
    {
        auto r = getLocalBounds().reduced (0, 4);
        bankBox.setBounds (r.removeFromLeft (128));
        r.removeFromLeft (6);
        prevButton.setBounds (r.removeFromLeft (24));
        r.removeFromLeft (2);
        auto right = r.removeFromRight (150);
        diceButton.setBounds (r.removeFromRight (34));
        r.removeFromRight (4);
        nextButton.setBounds (r.removeFromRight (24));
        r.removeFromRight (2);
        nameButton.setBounds (r);

        right.removeFromLeft (6);
        likeButton.setBounds (right.removeFromLeft (30));
        likedCount.setBounds (right.removeFromLeft (26));
        saveButton.setBounds (right.removeFromLeft (44));
        right.removeFromLeft (4);
        menuButton.setBounds (right);
    }

    void PresetBar::showBrowserMenu()
    {
        juce::PopupMenu menu;
        const auto& all = presets.getPresets();
        const auto heart = juce::String::fromUTF8 ("\xe2\x99\xa5 ");

        auto itemFor = [&] (int index)
        {
            juce::PopupMenu::Item item ((presets.isLiked (index) ? heart : juce::String ("    ")) + all[(size_t) index].name);
            item.itemID = index + 1;
            item.isTicked = index == presets.getCurrentIndex();
            return item;
        };

        juce::PopupMenu liked;
        for (int i : presets.getIndicesInBank (PresetManager::bankLiked))
            liked.addItem (itemFor (i));
        if (liked.getNumItems() == 0)
            liked.addItem (juce::PopupMenu::Item ("Click the heart to like presets").setEnabled (false));
        menu.addSubMenu (PresetManager::bankLiked + "  (" + juce::String (presets.getNumLiked()) + ")", liked);
        menu.addSeparator();

        for (const auto& cat : presets.getCategories())
        {
            juce::PopupMenu sub;
            for (int i : presets.getIndicesInBank (cat))
                sub.addItem (itemFor (i));
            menu.addSubMenu (cat, sub);
        }

        juce::PopupMenu user;
        for (int i : presets.getIndicesInBank (PresetManager::bankUser))
            user.addItem (itemFor (i));
        if (user.getNumItems() == 0)
            user.addItem (juce::PopupMenu::Item ("No user presets yet").setEnabled (false));
        menu.addSeparator();
        menu.addSubMenu (PresetManager::bankUser, user);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (nameButton).withMinimumWidth (nameButton.getWidth()),
                            [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
                            {
                                if (safe != nullptr && result > 0)
                                    safe->presets.loadPreset (result - 1);
                            });
    }

    void PresetBar::showDiceMenu()
    {
        juce::PopupMenu menu;
        menu.addSectionHeader ("Random sound type");
        const auto names = randomStyleNames();
        for (int i = 0; i < names.size(); ++i)
            menu.addItem (i + 1, names[i], true, (int) diceButton.style == i);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (diceButton),
                            [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
                            {
                                if (safe == nullptr || result <= 0)
                                    return;
                                safe->diceButton.style = (RandomStyle) (result - 1);
                                safe->diceButton.roll();
                            });
    }

    void PresetBar::showOptionsMenu()
    {
        juce::PopupMenu menu;
        menu.addSectionHeader ("Liked bank (" + juce::String (presets.getNumLiked()) + " presets)");
        menu.addItem (1, "Export Liked Bank...", presets.getNumLiked() > 0);
        menu.addItem (2, "Show Liked Bank File");
        menu.addSeparator();
        menu.addItem (3, "Import Bank...");
        menu.addItem (4, "Open User Presets Folder");
        menu.addItem (5, "Rescan Presets");
        const int current = presets.getCurrentIndex();
        const bool isUser = juce::isPositiveAndBelow (current, (int) presets.getPresets().size())
                         && ! presets.getPresets()[(size_t) current].isFactory;
        menu.addSeparator();
        menu.addItem (6, "Delete Current User Preset", isUser);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuButton),
                            [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
        {
            if (safe == nullptr)
                return;
            auto& self = *safe;

            switch (result)
            {
                case 1:
                    self.chooser = std::make_unique<juce::FileChooser> ("Export the Liked bank to a folder",
                                                                        juce::File::getSpecialLocation (juce::File::userDesktopDirectory));
                    self.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
                                               [safe] (const juce::FileChooser& fc)
                                               {
                                                   if (safe == nullptr || fc.getResult() == juce::File())
                                                       return;
                                                   const auto folder = fc.getResult().getChildFile ("Reckless HS20 Liked Bank");
                                                   if (safe->presets.exportLikedBank (folder))
                                                       folder.revealToUser();
                                               });
                    break;
                case 2:
                    self.presets.getLikedBankFile().getParentDirectory().createDirectory();
                    (self.presets.getLikedBankFile().existsAsFile() ? self.presets.getLikedBankFile()
                                                                     : self.presets.getLikedBankFile().getParentDirectory())
                        .revealToUser();
                    break;
                case 3:
                    self.chooser = std::make_unique<juce::FileChooser> ("Import a bank", juce::File(), "*" + PresetManager::bankExtension);
                    self.chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                               [safe] (const juce::FileChooser& fc)
                                               {
                                                   if (safe != nullptr && fc.getResult().existsAsFile())
                                                       safe->presets.importBank (fc.getResult());
                                               });
                    break;
                case 4:
                    self.presets.getUserPresetDirectory().createDirectory();
                    self.presets.getUserPresetDirectory().startAsProcess();
                    break;
                case 5:
                    self.presets.rescan();
                    break;
                case 6:
                    self.presets.deleteUserPreset (self.presets.getCurrentIndex());
                    break;
                default:
                    break;
            }
        });
    }

    void PresetBar::showSaveDialog()
    {
        saveWindow = std::make_unique<juce::AlertWindow> ("Save Preset", "Name your sound:", juce::MessageBoxIconType::NoIcon, this);
        const auto current = presets.getCurrentName();
        saveWindow->setLookAndFeel (&getLookAndFeel());
        saveWindow->addTextEditor ("name", current == "Init" ? juce::String ("My Sound") : current + " 2");
        juce::StringArray cats { PresetManager::bankUser };
        cats.addArray (presets.getCategories());
        saveWindow->addComboBox ("category", cats, "Category");
        saveWindow->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        saveWindow->addButton ("Save + Like", 2);
        saveWindow->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

        saveWindow->enterModalState (true, juce::ModalCallbackFunction::create (
            [safe = juce::Component::SafePointer<PresetBar> (this)] (int result)
            {
                if (safe == nullptr || safe->saveWindow == nullptr)
                    return;

                if (result > 0)
                {
                    const auto name = safe->saveWindow->getTextEditorContents ("name");
                    const auto* box = safe->saveWindow->getComboBoxComponent ("category");
                    const int index = safe->presets.saveUserPreset (name, box != nullptr ? box->getText() : juce::String());
                    if (index >= 0 && result == 2)
                        safe->presets.setLiked (index, true);
                }
                safe->saveWindow.reset();
            }), false);
    }
}
