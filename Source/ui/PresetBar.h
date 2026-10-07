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

        PresetManager& presets;
        juce::ComboBox bankBox;
        juce::TextButton prevButton { "<" }, nextButton { ">" }, nameButton, saveButton { "SAVE" }, menuButton { "BANK" };
        HeartButton likeButton;
        juce::Label likedCount;
        std::unique_ptr<juce::FileChooser> chooser;
        std::unique_ptr<juce::AlertWindow> saveWindow;
    };
}
