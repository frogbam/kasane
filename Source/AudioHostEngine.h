#pragma once

#include <JuceHeader.h>

#include "AppState.h"

#include <optional>

namespace kasane
{

class AudioHostEngine final : private juce::AudioIODeviceCallback
{
public:
    explicit AudioHostEngine(juce::ApplicationProperties& properties);
    ~AudioHostEngine() override;

    AppState getAppStateSnapshot() const;
    AudioState getAudioStateSnapshot() const;
    TunerState getTunerStateSnapshot() const;
    MeterState getMeterStateSnapshot() const;

    void setLanguage(const juce::String& languageCode);
    void setTheme(const juce::String& themeName);
    void setInputGainDb(float gainDb);
    void setOutputGainDb(float gainDb);
    void setTunerOpen(bool shouldBeOpen);
    bool setAudioDeviceType(const juce::String& deviceType);

    void refreshAudioOptions();
    bool setAudioDeviceSetup(const juce::String& inputDeviceId,
                             const juce::String& outputDeviceId,
                             double sampleRate,
                             int bufferSize,
                             const juce::String& leftMonitorChannelId,
                             const juce::String& rightMonitorChannelId);

    bool scanForPlugins();
    bool addPluginToChain(const juce::String& pluginDescriptorId);
    bool removePluginFromChain(const juce::String& chainPluginId);
    bool togglePluginBypass(const juce::String& chainPluginId);
    bool reorderPlugin(const juce::String& chainPluginId, int newIndex);
    bool openPluginEditor(const juce::String& chainPluginId, juce::Component& parentComponent);

    std::optional<juce::String> consumeLastError();

private:
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String& errorMessage) override;

    struct HostedPlugin;
    class PluginEditorWindow;

    void initialisePluginFormats();
    void initialiseAudioDeviceManager();
    void createBaseGraphNodes();
    void rebuildGraphConnections();
    void refreshDeviceLists();
    void updateDeviceOptionsForType(const juce::String& deviceType);
    void persistState() const;
    void restoreState();
    void clearEditors();
    void closeEditorForPlugin(const juce::String& chainPluginId);
    void setLastError(const juce::String& message);
    HostedPlugin* findHostedPlugin(const juce::String& chainPluginId);
    const HostedPlugin* findHostedPlugin(const juce::String& chainPluginId) const;

    juce::ApplicationProperties& appProperties;
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList knownPluginList;
    juce::AudioDeviceManager deviceManager;
    juce::AudioProcessorGraph graph;

    juce::String language { "en" };
    juce::String theme { "dark" };
    juce::String statusMessage;
    juce::String lastError;
    bool tunerOpen = false;
    bool isScanningPlugins = false;

    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    juce::String currentDeviceType;
    std::vector<juce::String> availableDeviceTypes;
    std::vector<DeviceOption> inputDevices;
    std::vector<DeviceOption> outputDevices;
    std::vector<ChannelOption> outputChannelOptions;
    std::vector<int> bufferSizeOptions;
    std::vector<double> sampleRateOptions;

    juce::AudioProcessorGraph::Node::Ptr audioInputNode;
    juce::AudioProcessorGraph::Node::Ptr audioOutputNode;
    juce::AudioProcessorGraph::Node::Ptr inputGainNode;
    juce::AudioProcessorGraph::Node::Ptr inputAnalysisNode;
    juce::AudioProcessorGraph::Node::Ptr outputGainNode;
    juce::AudioProcessorGraph::Node::Ptr outputAnalysisNode;

    std::vector<HostedPlugin> hostedPlugins;
    std::map<juce::String, std::unique_ptr<PluginEditorWindow>> editorWindows;

    std::atomic<float> inputLeftDb { -100.0f };
    std::atomic<float> inputRightDb { -100.0f };
    std::atomic<float> outputLeftDb { -100.0f };
    std::atomic<float> outputRightDb { -100.0f };
    std::atomic<float> tunerFrequencyHz { 0.0f };
    std::atomic<float> tunerSignalLevel { 0.0f };
    std::atomic<int> tunerCents { 0 };
    std::atomic<int> tunerMidiNote { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioHostEngine)
};

} // namespace kasane
