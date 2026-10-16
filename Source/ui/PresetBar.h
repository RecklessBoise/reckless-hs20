#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../presets/PresetManager.h"

namespace hs20::ui
{
    // Heart toggle drawn as a vector shape.
    class HeartButton : public juce::Button
    {
    public:
        HeartButton() : juce::Button ("Like") { setClickingTogglesState (false); }
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    };

    // Die that rolls a random patch. Left click rolls, right click (or
    // Cmd-click) chooses which kind of sound it rolls.
    class DiceButton : public juce::Button, private juce::Timer
    {
    public:
        DiceButton();
        void paintButton (juce::Graphics&, bool highlighted, bool down) override;
        void mouseDown (const juce::MouseEvent&) override;

        void roll();  // tumbles briefly, then calls onRolled
        RandomStyle style = RandomStyle::any;
        std::function<void()> onStyleMenu, onRolled;

    private:
        void timerCallback() override;
        int face = 5, tumbleFrames = 0;
        float angle = 0.0f;
        juce::Random rng;
    };

    // Bank selector, previous/next, preset browser menu, like, save and
    // bank management (export / import of liked presets).
    class PresetBar : public juce::Component, private juce::ChangeListener
    {
    public:
        explicit PresetBar (PresetManager& manager);
        ~PresetBar() override;

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void changeListenerCallback (juce::ChangeBroadcaster*) override { refresh(); }
        void refresh();
        void showBrowserMenu();
        void showOptionsMenu();
        void showSaveDialog();
        void showDiceMenu();

        PresetManager& presets;
        juce::ComboBox bankBox;
        juce::TextButton prevButton { "<" }, nextButton { ">" }, nameButton, saveButton { "SAVE" }, menuButton { "BANK" };
        HeartButton likeButton;
        DiceButton diceButton;
        juce::Label likedCount;
        std::unique_ptr<juce::FileChooser> chooser;
        std::unique_ptr<juce::AlertWindow> saveWindow;
    };
}
