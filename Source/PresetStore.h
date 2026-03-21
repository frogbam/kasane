#pragma once

#include <JuceHeader.h>

#include <optional>
#include <vector>

namespace kasane
{

struct ToneSnapshotPlugin
{
    juce::String identifier;
    juce::String stateBase64;
    bool isEnabled = true;
};

struct ToneSnapshot
{
    double bpm = 120.0;
    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    std::vector<ToneSnapshotPlugin> plugins;
};

struct PresetSummary
{
    juce::String id;
    juce::String name;
    juce::String updatedAt;
    int pluginCount = 0;
};

struct PresetRecord
{
    PresetSummary summary;
    juce::String createdAt;
    ToneSnapshot snapshot;
};

class PresetStore
{
public:
    explicit PresetStore(juce::File rootDirectory);

    std::vector<PresetSummary> loadSummaries() const;
    juce::Result loadPreset(const juce::String& presetId, PresetRecord& outRecord) const;
    juce::Result saveNewPreset(const juce::String& name,
                               const ToneSnapshot& snapshot,
                               PresetRecord& outRecord) const;
    juce::Result saveExistingPreset(const juce::String& presetId,
                                    const ToneSnapshot& snapshot,
                                    PresetRecord& outRecord) const;
    juce::Result renamePreset(const juce::String& presetId,
                              const juce::String& name,
                              PresetRecord& outRecord) const;
    juce::Result duplicatePreset(const juce::String& presetId,
                                 const juce::String& name,
                                 PresetRecord& outRecord) const;
    juce::Result deletePreset(const juce::String& presetId) const;

    static juce::String serialiseToneSnapshotToXml(const ToneSnapshot& snapshot);
    static std::optional<ToneSnapshot> deserialiseToneSnapshotFromXml(const juce::XmlElement& xml);

private:
    juce::File getPresetFile(const juce::String& presetId) const;
    juce::Result ensureDirectoryExists() const;
    juce::Result ensureUniqueName(const juce::String& name, const juce::String& ignoredPresetId = {}) const;
    juce::Result loadPresetFromFile(const juce::File& file, PresetRecord& outRecord) const;
    juce::Result writePreset(const PresetRecord& record) const;
    static std::optional<PresetRecord> parsePresetXml(const juce::XmlElement& xml);
    static std::unique_ptr<juce::XmlElement> createPresetXml(const PresetRecord& record);
    static juce::String makeTimestamp();
    static juce::String normalisePresetName(const juce::String& name);

    juce::File rootDirectory;
};

} // namespace kasane
