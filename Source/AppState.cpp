#include "AppState.h"

namespace kasane
{
namespace
{
template <typename Collection, typename Mapper>
juce::var makeArray(const Collection& values, Mapper mapper)
{
    juce::Array<juce::var> result;

    for (const auto& value : values)
        result.add(mapper(value));

    return result;
}

juce::DynamicObject::Ptr makeObject()
{
    return new juce::DynamicObject();
}
} // namespace

juce::String makeDeviceId(const juce::String& deviceType, const juce::String& deviceName)
{
    return deviceType + "::" + deviceName;
}

juce::var toVar(const DeviceOption& device)
{
    auto object = makeObject();
    object->setProperty("id", device.id);
    object->setProperty("name", device.name);
    object->setProperty("type", device.type);
    return object.get();
}

juce::var toVar(const PluginDescriptor& plugin)
{
    auto object = makeObject();
    object->setProperty("id", plugin.id);
    object->setProperty("name", plugin.name);
    object->setProperty("manufacturer", plugin.manufacturer);
    object->setProperty("category", plugin.category);
    object->setProperty("format", plugin.format);
    object->setProperty("isEnabled", plugin.isEnabled);
    return object.get();
}

juce::var toVar(const ChainSlot& slot)
{
    auto object = makeObject();
    object->setProperty("pluginId", slot.pluginId);
    object->setProperty("name", slot.name);
    object->setProperty("manufacturer", slot.manufacturer);
    object->setProperty("category", slot.category);
    object->setProperty("order", slot.order);
    object->setProperty("bypassed", slot.bypassed);
    return object.get();
}

juce::var toVar(const AudioState& state)
{
    auto object = makeObject();
    object->setProperty("inputGainDb", state.inputGainDb);
    object->setProperty("outputGainDb", state.outputGainDb);
    object->setProperty("audioDeviceType", state.audioDeviceType);
    object->setProperty("inputDeviceId", state.inputDeviceId);
    object->setProperty("outputDeviceId", state.outputDeviceId);
    object->setProperty("inputDeviceName", state.inputDeviceName);
    object->setProperty("outputDeviceName", state.outputDeviceName);
    object->setProperty("bufferSize", state.bufferSize);
    object->setProperty("sampleRate", state.sampleRate);
    object->setProperty("inputDevices", makeArray(state.inputDevices, [] (const auto& device) { return toVar(device); }));
    object->setProperty("outputDevices", makeArray(state.outputDevices, [] (const auto& device) { return toVar(device); }));
    object->setProperty("bufferSizeOptions", makeArray(state.bufferSizeOptions, [] (const auto& value) { return juce::var(value); }));
    object->setProperty("sampleRateOptions", makeArray(state.sampleRateOptions, [] (const auto& value) { return juce::var(value); }));
    return object.get();
}

juce::var toVar(const TunerState& state)
{
    auto object = makeObject();
    object->setProperty("isOpen", state.isOpen);
    object->setProperty("note", state.note);
    object->setProperty("frequencyHz", state.frequencyHz);
    object->setProperty("cents", state.cents);
    object->setProperty("signalLevel", state.signalLevel);
    return object.get();
}

juce::var toVar(const MeterState& state)
{
    auto object = makeObject();
    object->setProperty("inputLeftDb", state.inputLeftDb);
    object->setProperty("inputRightDb", state.inputRightDb);
    object->setProperty("outputLeftDb", state.outputLeftDb);
    object->setProperty("outputRightDb", state.outputRightDb);
    return object.get();
}

juce::var toVar(const AppState& state)
{
    auto object = makeObject();
    object->setProperty("bridgeVersion", state.bridgeVersion);
    object->setProperty("language", state.language);
    object->setProperty("theme", state.theme);
    object->setProperty("statusMessage", state.statusMessage);
    object->setProperty("lastError", state.lastError);
    object->setProperty("isScanningPlugins", state.isScanningPlugins);
    object->setProperty("audio", toVar(state.audio));
    object->setProperty("tuner", toVar(state.tuner));
    object->setProperty("meters", toVar(state.meters));
    object->setProperty("availablePlugins", makeArray(state.availablePlugins, [] (const auto& plugin) { return toVar(plugin); }));
    object->setProperty("chain", makeArray(state.chain, [] (const auto& slot) { return toVar(slot); }));
    return object.get();
}

} // namespace kasane
