#pragma once

#include <JuceHeader.h>

#include <vector>

namespace kasane
{
inline constexpr const char* bridgeVersion = "1.0.0";

struct DeviceOption
{
    juce::String id;
    juce::String name;
    juce::String type;
};

struct PluginDescriptor
{
    juce::String id;
    juce::String name;
    juce::String manufacturer;
    juce::String category;
    juce::String format;
    bool isEnabled = true;
};

struct ChainSlot
{
    juce::String pluginId;
    juce::String name;
    juce::String manufacturer;
    juce::String category;
    int order = 0;
    bool bypassed = false;
};

struct AudioState
{
    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    juce::String audioDeviceType;
    juce::String inputDeviceId;
    juce::String outputDeviceId;
    juce::String inputDeviceName;
    juce::String outputDeviceName;
    int bufferSize = 0;
    double sampleRate = 0.0;
    std::vector<DeviceOption> inputDevices;
    std::vector<DeviceOption> outputDevices;
    std::vector<int> bufferSizeOptions;
    std::vector<double> sampleRateOptions;
};

struct TunerState
{
    bool isOpen = false;
    juce::String note = "--";
    double frequencyHz = 0.0;
    int cents = 0;
    float signalLevel = 0.0f;
};

struct MeterState
{
    float inputLeftDb = -100.0f;
    float inputRightDb = -100.0f;
    float outputLeftDb = -100.0f;
    float outputRightDb = -100.0f;
};

struct AppState
{
    juce::String bridgeVersion = kasane::bridgeVersion;
    juce::String language = "en";
    juce::String theme = "dark";
    juce::String statusMessage;
    juce::String lastError;
    bool isScanningPlugins = false;
    AudioState audio;
    TunerState tuner;
    MeterState meters;
    std::vector<PluginDescriptor> availablePlugins;
    std::vector<ChainSlot> chain;
};

juce::String makeDeviceId(const juce::String& deviceType, const juce::String& deviceName);
juce::var toVar(const DeviceOption& device);
juce::var toVar(const PluginDescriptor& plugin);
juce::var toVar(const ChainSlot& slot);
juce::var toVar(const AudioState& state);
juce::var toVar(const TunerState& state);
juce::var toVar(const MeterState& state);
juce::var toVar(const AppState& state);

} // namespace kasane
