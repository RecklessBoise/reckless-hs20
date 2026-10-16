#include "PresetManager.h"
#include "HS20BinaryData.h"
#include "../ParameterBridge.h"
#include "../dsp/SynthEngine.h"

namespace hs20
{
    namespace
    {
        // Plays one note offline through a private engine and returns the
        // loudest 50 ms window in dBFS (same measure as the factory library).
        float measureLoudness (juce::AudioProcessorValueTreeState& state, int note)
        {
            constexpr double sampleRate = 48000.0;
            constexpr int block = 512;
            dsp::SynthEngine engine;
            dsp::EngineParams params;
            ParameterBridge (state).read (params);
            engine.prepare (sampleRate, block);

            juce::AudioBuffer<float> buffer (1, block);
            const int window = 2400;
            double acc = 0.0, loudest = 0.0;
            int filled = 0;

            for (int b = 0; b < 70; ++b)  // ~0.75 s
            {
                juce::MidiBuffer midi;
                if (b == 0)  midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                if (b == 55) midi.addEvent (juce::MidiMessage::noteOff (1, note), 0);
                engine.render (buffer, midi, nullptr, params);

                for (int i = 0; i < block; ++i)
                {
                    const double v = buffer.getSample (0, i);
                    acc += v * v;
                    if (++filled == window)
                    {
                        loudest = std::max (loudest, acc / window);
                        acc = 0.0;
                        filled = 0;
                    }
                }
            }
            return (float) (10.0 * std::log10 (loudest + 1.0e-12));
        }
    }

    PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state, juce::File dataDirectory)
        : apvts (state), dataDir (std::move (dataDirectory))
    {
        loadFactoryPresets();
        rescan();
    }

    juce::File PresetManager::getDefaultDataDirectory()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
            .getChildFile ("Application Support/Reckless/HS20");
    }

    void PresetManager::loadFactoryPresets()
    {
        presets.clear();
        const auto json = juce::String::fromUTF8 (HS20BinaryData::factory_presets_json,
                                                  HS20BinaryData::factory_presets_jsonSize);
        const auto root = juce::JSON::parse (json);

        if (auto* list = root["presets"].getArray())
        {
            for (const auto& item : *list)
            {
                PresetInfo p;
                p.name = item["name"].toString();
                p.category = item["category"].toString();
                p.author = item["author"].toString();
                p.params = item["params"];
                p.isFactory = true;
                presets.push_back (std::move (p));
            }
        }

        numFactory = (int) presets.size();
    }

    void PresetManager::loadUserPresets()
    {
        presets.resize ((size_t) numFactory);
        const auto dir = getUserPresetDirectory();
        if (! dir.isDirectory())
            return;

        auto files = dir.findChildFiles (juce::File::findFiles, false, "*" + presetExtension);
        files.sort();

        for (const auto& f : files)
        {
            const auto root = juce::JSON::parse (f);
            if (! root.isObject())
                continue;

            PresetInfo p;
            p.name = root["name"].toString();
            if (p.name.isEmpty())
                p.name = f.getFileNameWithoutExtension();
            p.category = root["category"].toString().isNotEmpty() ? root["category"].toString() : bankUser;
            p.author = root["author"].toString();
            p.params = root["params"];
            p.isFactory = false;
            p.file = f;
            presets.push_back (std::move (p));
        }
    }

    void PresetManager::loadFavourites()
    {
        likedKeys.clear();
        const auto root = juce::JSON::parse (getFavouritesFile());
        if (auto* list = root["liked"].getArray())
            for (const auto& k : *list)
                likedKeys.addIfNotAlreadyThere (k.toString());
    }

    void PresetManager::rescan()
    {
        const auto previousKey = getCurrentKey();
        loadUserPresets();
        loadFavourites();
        currentIndex = previousKey.isNotEmpty() ? findPreset (previousKey) : currentIndex;
        sendChangeMessage();
    }

    juce::StringArray PresetManager::getCategories() const
    {
        juce::StringArray cats;
        for (const auto& p : presets)
            if (p.isFactory)
                cats.addIfNotAlreadyThere (p.category);
        return cats;
    }

    juce::StringArray PresetManager::getBanks() const
    {
        juce::StringArray banks { bankAll, bankLiked, bankUser };
        banks.addArray (getCategories());
        return banks;
    }

    std::vector<int> PresetManager::getIndicesInBank (const juce::String& bank) const
    {
        std::vector<int> result;
        for (int i = 0; i < (int) presets.size(); ++i)
        {
            const auto& p = presets[(size_t) i];
            const bool include = bank == bankAll
                              || (bank == bankLiked && likedKeys.contains (p.key()))
                              || (bank == bankUser && ! p.isFactory)
                              || (p.isFactory && p.category == bank);
            if (include)
                result.push_back (i);
        }
        return result;
    }

    void PresetManager::setCurrentBank (const juce::String& bank)
    {
        currentBank = getBanks().contains (bank) ? bank : bankAll;
        sendChangeMessage();
    }

    juce::String PresetManager::getCurrentName() const
    {
        if (isRandomPatch())
            return randomName;
        return juce::isPositiveAndBelow (currentIndex, (int) presets.size()) ? presets[(size_t) currentIndex].name
                                                                              : juce::String ("Init");
    }

    juce::String PresetManager::getCurrentKey() const
    {
        if (isRandomPatch())
            return "random/" + randomStyle + "/" + randomName;
        return juce::isPositiveAndBelow (currentIndex, (int) presets.size()) ? presets[(size_t) currentIndex].key()
                                                                              : juce::String();
    }

    int PresetManager::findPreset (const juce::String& key) const
    {
        for (int i = 0; i < (int) presets.size(); ++i)
            if (presets[(size_t) i].key() == key)
                return i;
        return -1;
    }

    void PresetManager::restoreCurrent (const juce::String& key)
    {
        currentIndex = findPreset (key);
        randomName = randomStyle = {};
        if (key.startsWith ("random/"))
        {
            randomStyle = key.fromFirstOccurrenceOf ("random/", false, false).upToFirstOccurrenceOf ("/", false, false);
            randomName = key.fromLastOccurrenceOf ("/", false, false);
        }
        sendChangeMessage();
    }

    juce::var PresetManager::captureCurrentParams() const
    {
        auto* obj = new juce::DynamicObject();
        for (auto* param : apvts.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
                obj->setProperty (ranged->getParameterID(), ranged->convertFrom0to1 (ranged->getValue()));
        return juce::var (obj);
    }

    void PresetManager::applyParams (const juce::var& params)
    {
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            {
                const auto& id = ranged->getParameterID();
                const auto value = params.hasProperty (id) ? (float) params[juce::Identifier (id)]
                                                           : ranged->convertFrom0to1 (ranged->getDefaultValue());
                ranged->beginChangeGesture();
                ranged->setValueNotifyingHost (ranged->convertTo0to1 (value));
                ranged->endChangeGesture();
            }
        }
    }

    bool PresetManager::loadPreset (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()))
            return false;

        applyParams (presets[(size_t) index].params);
        currentIndex = index;
        randomName = randomStyle = {};
        sendChangeMessage();
        return true;
    }

    void PresetManager::step (int delta)
    {
        const auto list = getIndicesInBank (currentBank);
        if (list.empty())
            return;

        auto it = std::find (list.begin(), list.end(), currentIndex);
        int pos = it == list.end() ? (delta > 0 ? -1 : 0) : (int) std::distance (list.begin(), it);
        pos = (pos + delta + (int) list.size()) % (int) list.size();
        loadPreset (list[(size_t) pos]);
    }

    void PresetManager::loadRandom (RandomStyle style)
    {
        const auto patch = makeRandomPatch (style, random);
        applyParams (patch.params);

        // Self-levelling: bring every roll close to the factory presets' loudness.
        if (auto* volume = apvts.getParameter (ids::volume))
        {
            const int note = patch.style == "Bass" ? 43 : 60;
            for (int pass = 0; pass < 2; ++pass)
            {
                const float level = measureLoudness (apvts, note);
                if (level < -90.0f)
                    break;
                const float current = volume->convertFrom0to1 (volume->getValue());
                const float target = juce::jlimit (2.0f, 10.0f, current * std::pow (10.0f, (-14.0f - level) / 40.0f));
                volume->setValueNotifyingHost (volume->convertTo0to1 (target));
            }
        }
        currentIndex = -1;
        randomName = patch.name;
        randomStyle = patch.style;
        sendChangeMessage();
    }

    void PresetManager::likeCurrent()
    {
        if (isRandomPatch())
        {
            auto name = randomName;
            for (int n = 2; findPreset ("user/" + bankUser + "/" + name) >= 0; ++n)
                name = randomName + " " + juce::String (n);

            const int index = saveUserPreset (name, bankUser);
            if (index >= 0)
                setLiked (index, true);
            return;
        }
        toggleLiked (currentIndex);
    }

    bool PresetManager::isLiked (int index) const
    {
        return juce::isPositiveAndBelow (index, (int) presets.size()) && likedKeys.contains (presets[(size_t) index].key());
    }

    void PresetManager::setLiked (int index, bool shouldBeLiked)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()))
            return;

        // Re-read first so several plug-in instances never lose each other's likes.
        loadFavourites();
        const auto key = presets[(size_t) index].key();
        if (shouldBeLiked)
            likedKeys.addIfNotAlreadyThere (key);
        else
            likedKeys.removeString (key);

        saveFavourites();
        writeLikedBank();
        sendChangeMessage();
    }

    void PresetManager::saveFavourites() const
    {
        dataDir.createDirectory();
        juce::Array<juce::var> list;
        for (const auto& k : likedKeys)
            list.add (k);

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("liked", list);
        getFavouritesFile().replaceWithText (juce::JSON::toString (juce::var (obj)));
    }

    juce::var PresetManager::presetToVar (const PresetInfo& p)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("name", p.name);
        obj->setProperty ("category", p.category);
        obj->setProperty ("author", p.author);
        obj->setProperty ("params", p.params);
        return juce::var (obj);
    }

    void PresetManager::writeLikedBank() const
    {
        juce::Array<juce::var> list;
        for (const auto& p : presets)
            if (likedKeys.contains (p.key()))
                list.add (presetToVar (p));

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("bank", "Liked");
        obj->setProperty ("plugin", "Reckless HS20");
        obj->setProperty ("presets", list);
        dataDir.createDirectory();
        getLikedBankFile().replaceWithText (juce::JSON::toString (juce::var (obj)));
    }

    int PresetManager::saveUserPreset (const juce::String& name, const juce::String& category)
    {
        const auto cleanName = juce::File::createLegalFileName (name.trim());
        if (cleanName.isEmpty())
            return -1;

        PresetInfo p;
        p.name = name.trim();
        p.category = category.isNotEmpty() ? category : bankUser;
        p.author = "User";
        p.params = captureCurrentParams();
        p.isFactory = false;

        auto dir = getUserPresetDirectory();
        dir.createDirectory();
        p.file = dir.getChildFile (cleanName + presetExtension);
        if (! p.file.replaceWithText (juce::JSON::toString (presetToVar (p))))
            return -1;

        rescan();
        currentIndex = findPreset (p.key());
        randomName = randomStyle = {};
        writeLikedBank();
        sendChangeMessage();
        return currentIndex;
    }

    bool PresetManager::deleteUserPreset (int index)
    {
        if (! juce::isPositiveAndBelow (index, (int) presets.size()) || presets[(size_t) index].isFactory)
            return false;

        setLiked (index, false);
        const bool ok = presets[(size_t) index].file.moveToTrash() || presets[(size_t) index].file.deleteFile();
        if (currentIndex == index)
            currentIndex = -1;
        rescan();
        return ok;
    }

    bool PresetManager::exportLikedBank (const juce::File& folder) const
    {
        if (! folder.createDirectory())
            return false;

        bool ok = true;
        juce::Array<juce::var> list;
        for (const auto& p : presets)
        {
            if (! likedKeys.contains (p.key()))
                continue;

            const auto v = presetToVar (p);
            list.add (v);
            ok &= folder.getChildFile (juce::File::createLegalFileName (p.name) + presetExtension)
                      .replaceWithText (juce::JSON::toString (v));
        }

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("bank", "Liked");
        obj->setProperty ("plugin", "Reckless HS20");
        obj->setProperty ("presets", list);
        ok &= folder.getChildFile ("Liked Bank" + bankExtension).replaceWithText (juce::JSON::toString (juce::var (obj)));
        return ok;
    }

    int PresetManager::importBank (const juce::File& bankFile)
    {
        const auto root = juce::JSON::parse (bankFile);
        auto* list = root["presets"].getArray();
        if (list == nullptr)
            return 0;

        auto dir = getUserPresetDirectory();
        dir.createDirectory();
        int count = 0;

        for (const auto& item : *list)
        {
            const auto name = item["name"].toString();
            if (name.isEmpty())
                continue;

            auto* obj = new juce::DynamicObject();
            obj->setProperty ("name", name);
            obj->setProperty ("category", bankUser);
            obj->setProperty ("author", item["author"]);
            obj->setProperty ("params", item["params"]);
            if (dir.getChildFile (juce::File::createLegalFileName (name) + presetExtension)
                    .replaceWithText (juce::JSON::toString (juce::var (obj))))
                ++count;
        }

        rescan();
        return count;
    }
}
