#pragma once

#include <JuceHeader.h>

#include "AppState.h"

#include <optional>

namespace kasane
{

class TransportPlayHead final : public juce::AudioPlayHead
{
public:
    void setBpm(double nextBpm)
    {
        bpm.store(nextBpm, std::memory_order_relaxed);
    }

    void setSampleRate(double nextSampleRate)
    {
        sampleRate.store(nextSampleRate, std::memory_order_relaxed);
    }

    void reset()
    {
        timeInSamples.store(0, std::memory_order_relaxed);
    }

    void advance(int numSamples)
    {
        timeInSamples.fetch_add(numSamples, std::memory_order_relaxed);
    }

    Optional<PositionInfo> getPosition() const override
    {
        const auto currentSampleRate = sampleRate.load(std::memory_order_relaxed);
        const auto currentTimeInSamples = timeInSamples.load(std::memory_order_relaxed);
        const auto currentBpm = bpm.load(std::memory_order_relaxed);
        const auto currentPpqPosition = currentSampleRate > 0.0
                                      ? (static_cast<double>(currentTimeInSamples) / currentSampleRate) * (currentBpm / 60.0)
                                      : 0.0;

        PositionInfo info;
        info.setTimeInSamples(currentTimeInSamples);
        info.setTimeInSeconds(currentSampleRate > 0.0 ? static_cast<double>(currentTimeInSamples) / currentSampleRate : 0.0);
        info.setBpm(currentBpm);
        info.setTimeSignature(TimeSignature { 4, 4 });
        info.setPpqPosition(currentPpqPosition);
        info.setPpqPositionOfLastBarStart(0.0);
        info.setIsPlaying(true);
        info.setIsRecording(false);
        info.setIsLooping(false);
        info.setFrameRate(fpsUnknown);
        return info;
    }

private:
    std::atomic<double> bpm { 120.0 };
    std::atomic<double> sampleRate { 44100.0 };
    std::atomic<int64_t> timeInSamples { 0 };
};

class AudioHostEngine final : private juce::AudioIODeviceCallback
{
public:
    explicit AudioHostEngine(juce::ApplicationProperties& properties);
    ~AudioHostEngine() override;

    AppState getAppStateSnapshot() const;
    AudioState getAudioStateSnapshot() const;
    AudioState previewAudioDeviceSetup(const juce::String& inputDeviceId,
                                       const juce::String& outputDeviceId);
    TunerState getTunerStateSnapshot() const;
    MeterState getMeterStateSnapshot() const;

    void setLanguage(const juce::String& languageCode);
    void setTheme(const juce::String& themeName);
    void setBpm(double bpmIn);
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
    void refreshMonitorChannelSelection();
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
    juce::AudioBuffer<float> processingBuffer;
    TransportPlayHead transportPlayHead;

    juce::String language { "en" };
    juce::String theme { "dark" };
    juce::String statusMessage;
    juce::String lastError;
    bool tunerOpen = false;
    bool isScanningPlugins = false;

    double bpm = 120.0;
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
    std::atomic<int> leftMonitorChannelIndex { 0 };
    std::atomic<int> rightMonitorChannelIndex { 1 };
    std::atomic<float> tunerFrequencyHz { 0.0f };
    std::atomic<float> tunerSignalLevel { 0.0f };
    std::atomic<int> tunerCents { 0 };
    std::atomic<int> tunerMidiNote { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioHostEngine)
};

} // namespace kasane
