#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace hs20
{
    struct PresetInfo
    {
        juce::String name, category, author;
        bool isFactory = true;
        juce::File file;   // user presets only
        juce::var params;  // { paramId: realValue }

        // Stable identity used by the favourites file.
        juce::String key() const { return (isFactory ? "factory/" : "user/") + category + "/" + name; }
    };

    // Factory + user presets, the "like" system and the auto-maintained
    // Liked bank. Message-thread only.
    class PresetManager : public juce::ChangeBroadcaster
    {
    public:
        static inline const juce::String bankAll = "All Presets";
        static inline const juce::String bankLiked = juce::String::fromUTF8 ("\xe2\x99\xa5 Liked");
        static inline const juce::String bankUser = "User";
        static inline const juce::String presetExtension = ".hs20preset";
        static inline const juce::String bankExtension = ".hs20bank";

        explicit PresetManager (juce::AudioProcessorValueTreeState& state,
                                juce::File dataDirectory = getDefaultDataDirectory());

        static juce::File getDefaultDataDirectory();
        juce::File getUserPresetDirectory() const { return dataDir.getChildFile ("User Presets"); }
        juce::File getLikedBankFile() const { return dataDir.getChildFile ("Liked Bank" + bankExtension); }
        juce::File getFavouritesFile() const { return dataDir.getChildFile ("favorites.json"); }

        void rescan();

        const std::vector<PresetInfo>& getPresets() const { return presets; }
        int getNumFactoryPresets() const { return numFactory; }
        juce::StringArray getBanks() const;
        juce::StringArray getCategories() const;
        std::vector<int> getIndicesInBank (const juce::String& bank) const;

        void setCurrentBank (const juce::String& bank);
        juce::String getCurrentBank() const { return currentBank; }

        bool loadPreset (int index);
        void loadNext()     { step (1); }
        void loadPrevious() { step (-1); }
        int getCurrentIndex() const { return currentIndex; }
        juce::String getCurrentName() const;
        int findPreset (const juce::String& key) const;

        bool isLiked (int index) const;
        void setLiked (int index, bool shouldBeLiked);
        void toggleLiked (int index) { setLiked (index, ! isLiked (index)); }
        int getNumLiked() const { return likedKeys.size(); }

        // Saves the current sound; returns the new preset index or -1.
        int saveUserPreset (const juce::String& name, const juce::String& category);
        bool deleteUserPreset (int index);

        // Writes every liked preset as individual files + one bank file.
        bool exportLikedBank (const juce::File& folder) const;

        // Bank files (.hs20bank) can be re-imported as user presets.
        int importBank (const juce::File& bankFile);

        juce::var captureCurrentParams() const;
        void applyParams (const juce::var& params);

        // Used by the processor to restore the preset label from host state.
        void restoreCurrent (const juce::String& key);
        juce::String getCurrentKey() const;

    private:
        void loadFactoryPresets();
        void loadUserPresets();
        void loadFavourites();
        void saveFavourites() const;
        void writeLikedBank() const;
        void step (int delta);
        static juce::var presetToVar (const PresetInfo& p);

        juce::AudioProcessorValueTreeState& apvts;
        juce::File dataDir;
        std::vector<PresetInfo> presets;
        int numFactory = 0;
        int currentIndex = -1;
        juce::String currentBank = bankAll;
        juce::StringArray likedKeys;
    };
}
