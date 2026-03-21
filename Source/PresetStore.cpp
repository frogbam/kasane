#include "PresetStore.h"

#include <algorithm>

namespace kasane
{
namespace
{
constexpr auto presetFileSuffix = ".preset.xml";
constexpr auto presetRootTag = "PRESET";
constexpr auto toneTag = "TONE";
constexpr auto pluginTag = "PLUGIN";
constexpr auto presetIdAttribute = "id";
constexpr auto presetNameAttribute = "name";
constexpr auto presetCreatedAtAttribute = "createdAt";
constexpr auto presetUpdatedAtAttribute = "updatedAt";
constexpr auto toneBpmAttribute = "bpm";
constexpr auto toneInputGainAttribute = "inputGainDb";
constexpr auto toneOutputGainAttribute = "outputGainDb";
constexpr auto pluginIdentifierAttribute = "identifier";
constexpr auto pluginEnabledAttribute = "enabled";
constexpr auto pluginStateAttribute = "state";

bool newerPresetFirst(const PresetSummary& left, const PresetSummary& right)
{
    return juce::Time::fromISO8601(left.updatedAt) > juce::Time::fromISO8601(right.updatedAt);
}
} // namespace

PresetStore::PresetStore(juce::File rootDirectoryIn)
    : rootDirectory(std::move(rootDirectoryIn))
{
}

std::vector<PresetSummary> PresetStore::loadSummaries() const
{
    std::vector<PresetSummary> summaries;

    if (!rootDirectory.exists())
        return summaries;

    for (const auto& file : rootDirectory.findChildFiles(juce::File::findFiles, false, "*" + juce::String(presetFileSuffix)))
    {
        PresetRecord record;
        if (loadPresetFromFile(file, record).wasOk())
            summaries.push_back(record.summary);
    }

    std::sort(summaries.begin(), summaries.end(), newerPresetFirst);
    return summaries;
}

juce::Result PresetStore::loadPreset(const juce::String& presetId, PresetRecord& outRecord) const
{
    const auto file = getPresetFile(presetId);

    if (!file.existsAsFile())
        return juce::Result::fail("The selected preset could not be found.");

    return loadPresetFromFile(file, outRecord);
}

juce::Result PresetStore::saveNewPreset(const juce::String& name,
                                        const ToneSnapshot& snapshot,
                                        PresetRecord& outRecord) const
{
    const auto trimmedName = normalisePresetName(name);

    if (trimmedName.isEmpty())
        return juce::Result::fail("Preset name cannot be empty.");

    if (const auto directoryResult = ensureDirectoryExists(); directoryResult.failed())
        return directoryResult;

    if (const auto uniqueNameResult = ensureUniqueName(trimmedName); uniqueNameResult.failed())
        return uniqueNameResult;

    const auto timestamp = makeTimestamp();
    outRecord = {
        {
            juce::Uuid().toString(),
            trimmedName,
            timestamp,
            static_cast<int>(snapshot.plugins.size()),
        },
        timestamp,
        snapshot,
    };

    return writePreset(outRecord);
}

juce::Result PresetStore::saveExistingPreset(const juce::String& presetId,
                                             const ToneSnapshot& snapshot,
                                             PresetRecord& outRecord) const
{
    PresetRecord existingRecord;
    if (const auto loadResult = loadPreset(presetId, existingRecord); loadResult.failed())
        return loadResult;

    existingRecord.summary.updatedAt = makeTimestamp();
    existingRecord.summary.pluginCount = static_cast<int>(snapshot.plugins.size());
    existingRecord.snapshot = snapshot;
    outRecord = existingRecord;
    return writePreset(outRecord);
}

juce::Result PresetStore::renamePreset(const juce::String& presetId,
                                       const juce::String& name,
                                       PresetRecord& outRecord) const
{
    const auto trimmedName = normalisePresetName(name);

    if (trimmedName.isEmpty())
        return juce::Result::fail("Preset name cannot be empty.");

    if (const auto uniqueNameResult = ensureUniqueName(trimmedName, presetId); uniqueNameResult.failed())
        return uniqueNameResult;

    PresetRecord existingRecord;
    if (const auto loadResult = loadPreset(presetId, existingRecord); loadResult.failed())
        return loadResult;

    existingRecord.summary.name = trimmedName;
    existingRecord.summary.updatedAt = makeTimestamp();
    outRecord = existingRecord;
    return writePreset(outRecord);
}

juce::Result PresetStore::duplicatePreset(const juce::String& presetId,
                                          const juce::String& name,
                                          PresetRecord& outRecord) const
{
    const auto trimmedName = normalisePresetName(name);

    if (trimmedName.isEmpty())
        return juce::Result::fail("Preset name cannot be empty.");

    if (const auto uniqueNameResult = ensureUniqueName(trimmedName); uniqueNameResult.failed())
        return uniqueNameResult;

    PresetRecord sourceRecord;
    if (const auto loadResult = loadPreset(presetId, sourceRecord); loadResult.failed())
        return loadResult;

    const auto timestamp = makeTimestamp();
    sourceRecord.summary.id = juce::Uuid().toString();
    sourceRecord.summary.name = trimmedName;
    sourceRecord.summary.updatedAt = timestamp;
    sourceRecord.createdAt = timestamp;
    outRecord = sourceRecord;
    return writePreset(outRecord);
}

juce::Result PresetStore::deletePreset(const juce::String& presetId) const
{
    const auto file = getPresetFile(presetId);

    if (!file.existsAsFile())
        return juce::Result::fail("The selected preset could not be found.");

    return file.deleteFile()
        ? juce::Result::ok()
        : juce::Result::fail("Failed to delete the selected preset.");
}

juce::String PresetStore::serialiseToneSnapshotToXml(const ToneSnapshot& snapshot)
{
    juce::XmlElement xml(toneTag);
    xml.setAttribute(toneBpmAttribute, snapshot.bpm);
    xml.setAttribute(toneInputGainAttribute, snapshot.inputGainDb);
    xml.setAttribute(toneOutputGainAttribute, snapshot.outputGainDb);

    for (const auto& plugin : snapshot.plugins)
    {
        auto pluginXml = std::make_unique<juce::XmlElement>(pluginTag);
        pluginXml->setAttribute(pluginIdentifierAttribute, plugin.identifier);
        pluginXml->setAttribute(pluginEnabledAttribute, plugin.isEnabled);
        pluginXml->setAttribute(pluginStateAttribute, plugin.stateBase64);
        xml.addChildElement(pluginXml.release());
    }

    return xml.toString();
}

std::optional<ToneSnapshot> PresetStore::deserialiseToneSnapshotFromXml(const juce::XmlElement& xml)
{
    const auto* toneXml = xml.hasTagName(toneTag) ? &xml : xml.getChildByName(toneTag);

    if (toneXml == nullptr)
        return std::nullopt;

    ToneSnapshot snapshot;
    snapshot.bpm = toneXml->getDoubleAttribute(toneBpmAttribute, 120.0);
    snapshot.inputGainDb = static_cast<float>(toneXml->getDoubleAttribute(toneInputGainAttribute, 0.0));
    snapshot.outputGainDb = static_cast<float>(toneXml->getDoubleAttribute(toneOutputGainAttribute, 0.0));

    for (auto* pluginXml = toneXml->getFirstChildElement();
         pluginXml != nullptr;
         pluginXml = pluginXml->getNextElement())
    {
        if (!pluginXml->hasTagName(pluginTag))
            continue;

        const auto identifier = pluginXml->getStringAttribute(pluginIdentifierAttribute).trim();
        if (identifier.isEmpty())
            continue;

        snapshot.plugins.push_back({
            identifier,
            pluginXml->getStringAttribute(pluginStateAttribute),
            pluginXml->getBoolAttribute(pluginEnabledAttribute, true),
        });
    }

    return snapshot;
}

juce::File PresetStore::getPresetFile(const juce::String& presetId) const
{
    return rootDirectory.getChildFile(presetId + presetFileSuffix);
}

juce::Result PresetStore::ensureDirectoryExists() const
{
    return rootDirectory.createDirectory()
        ? juce::Result::ok()
        : juce::Result::fail("Failed to prepare the preset storage directory.");
}

juce::Result PresetStore::ensureUniqueName(const juce::String& name, const juce::String& ignoredPresetId) const
{
    for (const auto& summary : loadSummaries())
    {
        if (ignoredPresetId.isNotEmpty() && summary.id == ignoredPresetId)
            continue;

        if (summary.name.equalsIgnoreCase(name))
            return juce::Result::fail("A preset with the same name already exists.");
    }

    return juce::Result::ok();
}

juce::Result PresetStore::loadPresetFromFile(const juce::File& file, PresetRecord& outRecord) const
{
    auto xml = juce::parseXML(file);

    if (xml == nullptr)
        return juce::Result::fail("Failed to read preset file.");

    const auto parsedRecord = parsePresetXml(*xml);

    if (!parsedRecord.has_value())
        return juce::Result::fail("Preset file is corrupted.");

    outRecord = *parsedRecord;
    return juce::Result::ok();
}

juce::Result PresetStore::writePreset(const PresetRecord& record) const
{
    if (const auto directoryResult = ensureDirectoryExists(); directoryResult.failed())
        return directoryResult;

    const auto xml = createPresetXml(record);
    if (xml == nullptr)
        return juce::Result::fail("Failed to build preset data.");

    const auto file = getPresetFile(record.summary.id);
    return file.replaceWithText(xml->toString())
        ? juce::Result::ok()
        : juce::Result::fail("Failed to write preset file.");
}

std::optional<PresetRecord> PresetStore::parsePresetXml(const juce::XmlElement& xml)
{
    if (!xml.hasTagName(presetRootTag))
        return std::nullopt;

    const auto id = xml.getStringAttribute(presetIdAttribute).trim();
    const auto name = normalisePresetName(xml.getStringAttribute(presetNameAttribute));
    const auto createdAt = xml.getStringAttribute(presetCreatedAtAttribute).trim();
    const auto updatedAt = xml.getStringAttribute(presetUpdatedAtAttribute).trim();
    const auto snapshot = deserialiseToneSnapshotFromXml(xml);

    if (id.isEmpty() || name.isEmpty() || createdAt.isEmpty() || updatedAt.isEmpty() || !snapshot.has_value())
        return std::nullopt;

    return PresetRecord {
        {
            id,
            name,
            updatedAt,
            static_cast<int>(snapshot->plugins.size()),
        },
        createdAt,
        *snapshot,
    };
}

std::unique_ptr<juce::XmlElement> PresetStore::createPresetXml(const PresetRecord& record)
{
    auto xml = std::make_unique<juce::XmlElement>(presetRootTag);
    xml->setAttribute(presetIdAttribute, record.summary.id);
    xml->setAttribute(presetNameAttribute, record.summary.name);
    xml->setAttribute(presetCreatedAtAttribute, record.createdAt);
    xml->setAttribute(presetUpdatedAtAttribute, record.summary.updatedAt);

    if (auto toneXml = juce::parseXML(serialiseToneSnapshotToXml(record.snapshot)))
        xml->addChildElement(toneXml.release());

    return xml;
}

juce::String PresetStore::makeTimestamp()
{
    return juce::Time::getCurrentTime().toISO8601(true);
}

juce::String PresetStore::normalisePresetName(const juce::String& name)
{
    return name.trim();
}

} // namespace kasane
