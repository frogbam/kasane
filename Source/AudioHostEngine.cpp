#include "AudioHostEngine.h"

#include <array>
#include <cmath>

namespace kasane
{
namespace
{
constexpr auto settingsLanguageKey = "ui.language";
constexpr auto settingsThemeKey = "ui.theme";
constexpr auto settingsPluginListKey = "plugins.knownListXml";
constexpr auto settingsAudioStateKey = "audio.deviceStateXml";
constexpr auto settingsInputGainKey = "audio.inputGainDb";
constexpr auto settingsOutputGainKey = "audio.outputGainDb";

constexpr float minimumDb = -100.0f;

juce::String sanitiseTheme(const juce::String& value)
{
    return value == "light" ? "light" : "dark";
}

juce::String sanitiseLanguage(const juce::String& value)
{
    static const std::array<juce::String, 4> supported { "en", "ko", "ja", "zh" };

    for (const auto& language : supported)
        if (value == language)
            return value;

    return "en";
}

float magnitudeToDb(const float magnitude)
{
    return juce::jlimit(minimumDb, 6.0f, juce::Decibels::gainToDecibels(magnitude, minimumDb));
}

void storeMeterState(std::atomic<float>& destination, float magnitude)
{
    destination.store(magnitudeToDb(magnitude), std::memory_order_relaxed);
}

std::pair<float, int> detectPitch(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    if (sampleRate <= 0.0 || buffer.getNumSamples() < 128 || buffer.getNumChannels() == 0)
        return { 0.0f, -1 };

    const auto* samples = buffer.getReadPointer(0);
    const auto numSamples = buffer.getNumSamples();

    double rms = 0.0;

    for (int index = 0; index < numSamples; ++index)
        rms += static_cast<double>(samples[index]) * static_cast<double>(samples[index]);

    rms = std::sqrt(rms / static_cast<double>(numSamples));

    if (rms < 0.01)
        return { 0.0f, -1 };

    const auto minLag = juce::jlimit(16, numSamples / 2, static_cast<int>(sampleRate / 1000.0));
    const auto maxLag = juce::jlimit(minLag + 1, numSamples - 1, static_cast<int>(sampleRate / 70.0));

    double bestCorrelation = 0.0;
    int bestLag = -1;

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double numerator = 0.0;
        double denominatorA = 0.0;
        double denominatorB = 0.0;

        for (int index = 0; index < numSamples - lag; ++index)
        {
            const auto a = static_cast<double>(samples[index]);
            const auto b = static_cast<double>(samples[index + lag]);
            numerator += a * b;
            denominatorA += a * a;
            denominatorB += b * b;
        }

        const auto normaliser = std::sqrt(denominatorA * denominatorB);

        if (normaliser <= 0.0)
            continue;

        const auto correlation = numerator / normaliser;

        if (correlation > bestCorrelation)
        {
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }

    if (bestLag <= 0 || bestCorrelation < 0.8)
        return { static_cast<float>(rms), -1 };

    const auto frequency = static_cast<float>(sampleRate / static_cast<double>(bestLag));
    const auto midi = static_cast<int>(std::round(69.0 + 12.0 * std::log2(frequency / 440.0f)));
    return { frequency, midi };
}

int frequencyToCents(float frequency)
{
    if (frequency <= 0.0f)
        return 0;

    const auto midi = 69.0 + 12.0 * std::log2(frequency / 440.0f);
    return static_cast<int>(std::round((midi - std::round(midi)) * 100.0));
}

juce::String midiNoteToName(int midiNote)
{
    static const std::array<const char*, 12> noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    if (midiNote < 0)
        return "--";

    return noteNames[static_cast<size_t>(midiNote % 12)];
}

juce::String makeChannelId(int channelIndex)
{
    return juce::String(channelIndex);
}

std::optional<int> parseChannelId(const juce::String& channelId)
{
    const auto trimmed = channelId.trim();

    if (trimmed.isEmpty() || ! trimmed.containsOnly("0123456789"))
        return std::nullopt;

    return trimmed.getIntValue();
}

std::pair<int, int> getSelectedOutputChannels(const juce::AudioIODevice* device,
                                              const juce::AudioDeviceManager::AudioDeviceSetup& setup)
{
    juce::BigInteger selectedChannels;

    if (setup.useDefaultOutputChannels)
    {
        if (device != nullptr)
            selectedChannels = device->getActiveOutputChannels();
    }
    else
    {
        selectedChannels = setup.outputChannels;
    }

    auto leftChannel = selectedChannels.findNextSetBit(0);
    auto rightChannel = leftChannel >= 0 ? selectedChannels.findNextSetBit(leftChannel + 1) : -1;

    if (leftChannel < 0 && device != nullptr)
        leftChannel = device->getActiveOutputChannels().findNextSetBit(0);

    if (rightChannel < 0 && device != nullptr)
        rightChannel = device->getActiveOutputChannels().findNextSetBit(leftChannel + 1);

    if (leftChannel < 0)
        leftChannel = 0;

    if (rightChannel < 0)
        rightChannel = leftChannel == 0 ? 1 : leftChannel;

    return { leftChannel, rightChannel };
}

class PassthroughProcessor : public juce::AudioProcessor
{
public:
    explicit PassthroughProcessor(juce::String processorName)
        : juce::AudioProcessor(BusesProperties()
                                   .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          name(std::move(processorName))
    {
    }

    const juce::String getName() const override { return name; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override
    {
        return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
    }

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    const juce::String getInputChannelName(int channelIndex) const override { return juce::String(channelIndex + 1); }
    const juce::String getOutputChannelName(int channelIndex) const override { return juce::String(channelIndex + 1); }
    bool isInputChannelStereoPair(int) const override { return true; }
    bool isOutputChannelStereoPair(int) const override { return true; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    juce::String name;
};

class GainProcessor final : public PassthroughProcessor
{
public:
    explicit GainProcessor(const juce::String& processorName)
        : PassthroughProcessor(processorName)
    {
        setGainDb(0.0f);
    }

    void setGainDb(float nextGainDb)
    {
        gain.store(juce::Decibels::decibelsToGain(nextGainDb), std::memory_order_relaxed);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        buffer.applyGain(gain.load(std::memory_order_relaxed));
    }

private:
    std::atomic<float> gain { 1.0f };
};

class AnalysisTapProcessor final : public PassthroughProcessor
{
public:
    AnalysisTapProcessor(const juce::String& processorName,
                         std::atomic<float>& leftDbIn,
                         std::atomic<float>& rightDbIn,
                         std::atomic<float>* tunerFrequencyHzIn = nullptr,
                         std::atomic<float>* tunerSignalLevelIn = nullptr,
                         std::atomic<int>* tunerCentsIn = nullptr,
                         std::atomic<int>* tunerMidiNoteIn = nullptr)
        : PassthroughProcessor(processorName),
          leftDb(leftDbIn),
          rightDb(rightDbIn),
          tunerFrequencyHz(tunerFrequencyHzIn),
          tunerSignalLevel(tunerSignalLevelIn),
          tunerCents(tunerCentsIn),
          tunerMidiNote(tunerMidiNoteIn)
    {
    }

    void setSampleRate(double sampleRate)
    {
        currentSampleRate = sampleRate;
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        const auto leftMagnitude = buffer.getNumChannels() > 0 ? buffer.getMagnitude(0, 0, buffer.getNumSamples()) : 0.0f;
        const auto rightMagnitude = buffer.getNumChannels() > 1 ? buffer.getMagnitude(1, 0, buffer.getNumSamples()) : leftMagnitude;

        storeMeterState(leftDb, leftMagnitude);
        storeMeterState(rightDb, rightMagnitude);

        if (tunerFrequencyHz == nullptr
            || tunerSignalLevel == nullptr
            || tunerCents == nullptr
            || tunerMidiNote == nullptr)
            return;

        if ((analysisCounter++ % 4) != 0)
            return;

        const auto [frequency, midiNote] = detectPitch(buffer, currentSampleRate);
        tunerSignalLevel->store(leftMagnitude, std::memory_order_relaxed);

        if (midiNote < 0)
        {
            tunerFrequencyHz->store(0.0f, std::memory_order_relaxed);
            tunerCents->store(0, std::memory_order_relaxed);
            tunerMidiNote->store(-1, std::memory_order_relaxed);
            return;
        }

        tunerFrequencyHz->store(frequency, std::memory_order_relaxed);
        tunerCents->store(frequencyToCents(frequency), std::memory_order_relaxed);
        tunerMidiNote->store(midiNote, std::memory_order_relaxed);
    }

private:
    std::atomic<float>& leftDb;
    std::atomic<float>& rightDb;
    std::atomic<float>* tunerFrequencyHz = nullptr;
    std::atomic<float>* tunerSignalLevel = nullptr;
    std::atomic<int>* tunerCents = nullptr;
    std::atomic<int>* tunerMidiNote = nullptr;
    double currentSampleRate = 44100.0;
    int analysisCounter = 0;
};

std::optional<std::pair<juce::String, juce::String>> parseDeviceId(const juce::String& deviceId)
{
    const auto separatorIndex = deviceId.indexOf("::");

    if (separatorIndex < 0)
        return std::nullopt;

    return std::make_pair(deviceId.substring(0, separatorIndex), deviceId.substring(separatorIndex + 2));
}

bool addConnectionForChannels(juce::AudioProcessorGraph& graph,
                              const juce::AudioProcessorGraph::NodeID source,
                              int sourceChannel,
                              const juce::AudioProcessorGraph::NodeID destination,
                              int destinationChannel)
{
    return graph.addConnection({ { source, sourceChannel }, { destination, destinationChannel } },
                               juce::AudioProcessorGraph::UpdateKind::none);
}
} // namespace

struct AudioHostEngine::HostedPlugin
{
    juce::String chainId;
    juce::PluginDescription description;
    juce::AudioProcessorGraph::Node::Ptr node;
    bool isEnabled = true;
};

class AudioHostEngine::PluginEditorWindow final : public juce::DocumentWindow
{
public:
    PluginEditorWindow(const juce::String& title,
                       const juce::String& chainPluginIdIn,
                       juce::AudioPluginInstance& pluginInstance,
                       std::function<void(const juce::String&)> onCloseIn)
        : juce::DocumentWindow(title,
                               juce::Desktop::getInstance().getDefaultLookAndFeel()
                                   .findColour(juce::ResizableWindow::backgroundColourId),
                               juce::DocumentWindow::closeButton),
          chainPluginId(chainPluginIdIn),
          onClose(std::move(onCloseIn))
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);

        auto* editor = pluginInstance.createEditorIfNeeded();

        if (editor == nullptr)
            editor = new juce::GenericAudioProcessorEditor(pluginInstance);

        setContentOwned(editor, true);
        centreWithSize(editor->getWidth(), editor->getHeight());
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);

        if (onClose != nullptr)
            onClose(chainPluginId);
    }

private:
    juce::String chainPluginId;
    std::function<void(const juce::String&)> onClose;
};

AudioHostEngine::AudioHostEngine(juce::ApplicationProperties& properties)
    : appProperties(properties)
{
    initialisePluginFormats();
    restoreState();
    initialiseAudioDeviceManager();
    createBaseGraphNodes();
    deviceManager.addAudioCallback(this);
    refreshDeviceLists();
}

AudioHostEngine::~AudioHostEngine()
{
    clearEditors();
    deviceManager.removeAudioCallback(this);
    graph.releaseResources();
    persistState();
}

AppState AudioHostEngine::getAppStateSnapshot() const
{
    AppState state;
    state.language = language;
    state.theme = theme;
    state.statusMessage = statusMessage;
    state.lastError = lastError;
    state.isScanningPlugins = isScanningPlugins;
    state.audio = getAudioStateSnapshot();
    state.tuner = getTunerStateSnapshot();
    state.meters = getMeterStateSnapshot();

    for (const auto& description : knownPluginList.getTypes())
    {
        if (description.pluginFormatName != "VST3")
            continue;

        state.availablePlugins.push_back({
            description.createIdentifierString(),
            description.name,
            description.manufacturerName,
            description.category,
            description.pluginFormatName,
            true
        });
    }

    std::sort(state.availablePlugins.begin(), state.availablePlugins.end(), [] (const auto& left, const auto& right)
    {
        return left.name.compareNatural(right.name, true) < 0;
    });

    for (size_t index = 0; index < hostedPlugins.size(); ++index)
    {
        const auto& plugin = hostedPlugins[index];
        state.chain.push_back({
            plugin.chainId,
            plugin.description.name,
            plugin.description.manufacturerName,
            plugin.description.category,
            static_cast<int>(index),
            ! plugin.isEnabled
        });
    }

    return state;
}

AudioState AudioHostEngine::getAudioStateSnapshot() const
{
    AudioState state;
    state.inputGainDb = inputGainDb;
    state.outputGainDb = outputGainDb;
    state.audioDeviceType = currentDeviceType;
    state.availableDeviceTypes = availableDeviceTypes;
    state.inputDevices = inputDevices;
    state.outputDevices = outputDevices;
    state.outputChannelOptions = outputChannelOptions;
    state.bufferSizeOptions = bufferSizeOptions;
    state.sampleRateOptions = sampleRateOptions;

    const auto setup = deviceManager.getAudioDeviceSetup();
    const auto* device = deviceManager.getCurrentAudioDevice();
    const auto [leftMonitorChannel, rightMonitorChannel] = getSelectedOutputChannels(device, setup);
    state.inputDeviceName = setup.inputDeviceName;
    state.outputDeviceName = setup.outputDeviceName;
    state.inputDeviceId = makeDeviceId(currentDeviceType, setup.inputDeviceName);
    state.outputDeviceId = makeDeviceId(currentDeviceType, setup.outputDeviceName);
    state.leftMonitorChannelId = makeChannelId(leftMonitorChannel);
    state.rightMonitorChannelId = makeChannelId(rightMonitorChannel);
    state.bufferSize = setup.bufferSize;
    state.sampleRate = setup.sampleRate;
    return state;
}

TunerState AudioHostEngine::getTunerStateSnapshot() const
{
    return {
        tunerOpen,
        midiNoteToName(tunerMidiNote.load(std::memory_order_relaxed)),
        tunerFrequencyHz.load(std::memory_order_relaxed),
        tunerCents.load(std::memory_order_relaxed),
        tunerSignalLevel.load(std::memory_order_relaxed)
    };
}

MeterState AudioHostEngine::getMeterStateSnapshot() const
{
    return {
        inputLeftDb.load(std::memory_order_relaxed),
        inputRightDb.load(std::memory_order_relaxed),
        outputLeftDb.load(std::memory_order_relaxed),
        outputRightDb.load(std::memory_order_relaxed)
    };
}

void AudioHostEngine::setLanguage(const juce::String& languageCode)
{
    language = sanitiseLanguage(languageCode);
    persistState();
}

void AudioHostEngine::setTheme(const juce::String& themeName)
{
    theme = sanitiseTheme(themeName);
    persistState();
}

void AudioHostEngine::setInputGainDb(float gainDb)
{
    inputGainDb = juce::jlimit(-60.0f, 20.0f, gainDb);

    if (auto* processor = dynamic_cast<GainProcessor*>(inputGainNode->getProcessor()))
        processor->setGainDb(inputGainDb);

    persistState();
}

void AudioHostEngine::setOutputGainDb(float gainDb)
{
    outputGainDb = juce::jlimit(-60.0f, 20.0f, gainDb);

    if (auto* processor = dynamic_cast<GainProcessor*>(outputGainNode->getProcessor()))
        processor->setGainDb(outputGainDb);

    persistState();
}

void AudioHostEngine::setTunerOpen(bool shouldBeOpen)
{
    tunerOpen = shouldBeOpen;
}

bool AudioHostEngine::setAudioDeviceType(const juce::String& deviceType)
{
    if (deviceType.isEmpty() || deviceType == currentDeviceType)
    {
        refreshDeviceLists();
        return true;
    }

    const auto iterator = std::find(availableDeviceTypes.begin(), availableDeviceTypes.end(), deviceType);

    if (iterator == availableDeviceTypes.end())
    {
        setLastError("The selected audio device type is unavailable.");
        return false;
    }

    deviceManager.setCurrentAudioDeviceType(deviceType, true);

    refreshDeviceLists();

    if (currentDeviceType != deviceType)
    {
        setLastError("Failed to switch the audio device type.");
        return false;
    }

    statusMessage = "Audio device type updated.";
    persistState();
    return true;
}

void AudioHostEngine::refreshAudioOptions()
{
    refreshDeviceLists();
}

bool AudioHostEngine::setAudioDeviceSetup(const juce::String& inputDeviceId,
                                          const juce::String& outputDeviceId,
                                          double sampleRate,
                                          int bufferSize,
                                          const juce::String& leftMonitorChannelId,
                                          const juce::String& rightMonitorChannelId)
{
    auto desiredType = currentDeviceType;

    if (const auto parsedInput = parseDeviceId(inputDeviceId))
        desiredType = parsedInput->first;

    if (const auto parsedOutput = parseDeviceId(outputDeviceId))
    {
        if (desiredType.isNotEmpty() && desiredType != parsedOutput->first)
        {
            setLastError("Input and output devices must use the same audio device type.");
            return false;
        }

        desiredType = parsedOutput->first;
    }

    if (desiredType.isNotEmpty() && desiredType != currentDeviceType)
        deviceManager.setCurrentAudioDeviceType(desiredType, true);

    auto setup = deviceManager.getAudioDeviceSetup();

    if (const auto parsedInput = parseDeviceId(inputDeviceId))
        setup.inputDeviceName = parsedInput->second;

    if (const auto parsedOutput = parseDeviceId(outputDeviceId))
        setup.outputDeviceName = parsedOutput->second;

    if (sampleRate > 0.0)
        setup.sampleRate = sampleRate;

    if (bufferSize > 0)
        setup.bufferSize = bufferSize;

    if (const auto leftMonitorChannel = parseChannelId(leftMonitorChannelId))
    {
        setup.useDefaultOutputChannels = false;
        setup.outputChannels.clear();
        setup.outputChannels.setBit(*leftMonitorChannel);

        if (const auto rightMonitorChannel = parseChannelId(rightMonitorChannelId))
        {
            if (*rightMonitorChannel == *leftMonitorChannel)
            {
                setLastError("Left and right monitor outputs must be different.");
                return false;
            }

            setup.outputChannels.setBit(*rightMonitorChannel);
        }
    }

    const auto error = deviceManager.setAudioDeviceSetup(setup, true);

    if (error.isNotEmpty())
    {
        setLastError(error);
        return false;
    }

    refreshDeviceLists();
    statusMessage = "Audio device updated.";
    persistState();
    return true;
}

bool AudioHostEngine::scanForPlugins()
{
    auto* format = pluginFormatManager.getFormat(0);

    if (format == nullptr)
    {
        setLastError("The VST3 format is unavailable.");
        return false;
    }

    const auto searchPath = format->getDefaultLocationsToSearch();

    if (searchPath.toString().isEmpty())
    {
        setLastError("No default VST3 search locations were found.");
        return false;
    }

    isScanningPlugins = true;
    statusMessage = "Scanning VST3 plug-ins...";

    auto deadMansPedal = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                             .getChildFile("Kasane")
                             .getChildFile("KnownPlugins")
                             .getChildFile("deadmanspedal.txt");

    deadMansPedal.getParentDirectory().createDirectory();

    juce::PluginDirectoryScanner scanner(knownPluginList, *format, searchPath, true, deadMansPedal, false);
    juce::String currentPluginName;

    while (scanner.scanNextFile(true, currentPluginName))
    {
    }

    knownPluginList.sort(juce::KnownPluginList::sortAlphabetically, true);
    isScanningPlugins = false;
    statusMessage = "Plug-in scan finished.";
    persistState();
    return true;
}

bool AudioHostEngine::addPluginToChain(const juce::String& pluginDescriptorId)
{
    const auto description = knownPluginList.getTypeForIdentifierString(pluginDescriptorId);

    if (description == nullptr)
    {
        setLastError("The selected plug-in is no longer available.");
        return false;
    }

    juce::String errorMessage;
    auto instance = pluginFormatManager.createPluginInstance(*description, currentSampleRate, currentBlockSize, errorMessage);

    if (instance == nullptr)
    {
        setLastError(errorMessage.isNotEmpty() ? errorMessage : "Failed to create the selected plug-in.");
        return false;
    }

    instance->enableAllBuses();

    auto node = graph.addNode(std::move(instance), std::nullopt, juce::AudioProcessorGraph::UpdateKind::none);

    if (node == nullptr)
    {
        setLastError("Failed to add the selected plug-in to the graph.");
        return false;
    }

    hostedPlugins.push_back({ juce::Uuid().toString(), *description, node, true });
    rebuildGraphConnections();
    statusMessage = description->name + " added to the chain.";
    return true;
}

bool AudioHostEngine::removePluginFromChain(const juce::String& chainPluginId)
{
    const auto iterator = std::find_if(hostedPlugins.begin(), hostedPlugins.end(), [&] (const auto& plugin)
    {
        return plugin.chainId == chainPluginId;
    });

    if (iterator == hostedPlugins.end())
    {
        setLastError("The selected plug-in instance was not found.");
        return false;
    }

    closeEditorForPlugin(chainPluginId);
    graph.removeNode(iterator->node->nodeID, juce::AudioProcessorGraph::UpdateKind::none);
    const auto removedName = iterator->description.name;
    hostedPlugins.erase(iterator);
    rebuildGraphConnections();
    statusMessage = removedName + " removed from the chain.";
    return true;
}

bool AudioHostEngine::togglePluginBypass(const juce::String& chainPluginId)
{
    if (auto* plugin = findHostedPlugin(chainPluginId))
    {
        plugin->isEnabled = ! plugin->isEnabled;
        rebuildGraphConnections();
        statusMessage = plugin->description.name + (plugin->isEnabled ? " enabled." : " bypassed.");
        return true;
    }

    setLastError("The selected plug-in instance was not found.");
    return false;
}

bool AudioHostEngine::reorderPlugin(const juce::String& chainPluginId, int newIndex)
{
    if (newIndex < 0 || newIndex >= static_cast<int>(hostedPlugins.size()))
    {
        setLastError("The target plug-in position is out of range.");
        return false;
    }

    const auto iterator = std::find_if(hostedPlugins.begin(), hostedPlugins.end(), [&] (const auto& plugin)
    {
        return plugin.chainId == chainPluginId;
    });

    if (iterator == hostedPlugins.end())
    {
        setLastError("The selected plug-in instance was not found.");
        return false;
    }

    auto plugin = std::move(*iterator);
    hostedPlugins.erase(iterator);
    hostedPlugins.insert(hostedPlugins.begin() + newIndex, std::move(plugin));
    rebuildGraphConnections();
    statusMessage = "Plug-in chain reordered.";
    return true;
}

bool AudioHostEngine::openPluginEditor(const juce::String& chainPluginId, juce::Component& parentComponent)
{
    auto* plugin = findHostedPlugin(chainPluginId);

    if (plugin == nullptr)
    {
        setLastError("The selected plug-in instance was not found.");
        return false;
    }

    if (auto existing = editorWindows.find(chainPluginId); existing != editorWindows.end())
    {
        existing->second->toFront(true);
        return true;
    }

    auto* instance = dynamic_cast<juce::AudioPluginInstance*>(plugin->node->getProcessor());

    if (instance == nullptr)
    {
        setLastError("The plug-in editor could not be opened.");
        return false;
    }

    auto window = std::make_unique<PluginEditorWindow>(plugin->description.name,
                                                       chainPluginId,
                                                       *instance,
                                                       [this] (const juce::String& pluginId)
                                                       {
                                                           juce::MessageManager::callAsync([this, pluginId]
                                                           {
                                                               closeEditorForPlugin(pluginId);
                                                           });
                                                       });

    window->centreAroundComponent(&parentComponent, window->getWidth(), window->getHeight());
    editorWindows.emplace(chainPluginId, std::move(window));
    return true;
}

std::optional<juce::String> AudioHostEngine::consumeLastError()
{
    if (lastError.isEmpty())
        return std::nullopt;

    auto message = lastError;
    lastError.clear();
    return message;
}

void AudioHostEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                       int numInputChannels,
                                                       float* const* outputChannelData,
                                                       int numOutputChannels,
                                                       int numSamples,
                                                       const juce::AudioIODeviceCallbackContext&)
{
    juce::AudioBuffer<float> buffer(outputChannelData, numOutputChannels, numSamples);
    buffer.clear();

    for (int channel = 0; channel < juce::jmin(numInputChannels, numOutputChannels); ++channel)
        if (inputChannelData[channel] != nullptr)
            buffer.copyFrom(channel, 0, inputChannelData[channel], numSamples);

    juce::MidiBuffer midi;
    graph.processBlock(buffer, midi);
}

void AudioHostEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device == nullptr)
        return;

    currentSampleRate = device->getCurrentSampleRate();
    currentBlockSize = device->getCurrentBufferSizeSamples();

    graph.releaseResources();
    graph.setPlayConfigDetails(device->getActiveInputChannels().countNumberOfSetBits(),
                               device->getActiveOutputChannels().countNumberOfSetBits(),
                               currentSampleRate,
                               currentBlockSize);
    graph.prepareToPlay(currentSampleRate, currentBlockSize);

    if (auto* inputProcessor = dynamic_cast<AnalysisTapProcessor*>(inputAnalysisNode->getProcessor()))
        inputProcessor->setSampleRate(currentSampleRate);

    if (auto* outputProcessor = dynamic_cast<AnalysisTapProcessor*>(outputAnalysisNode->getProcessor()))
        outputProcessor->setSampleRate(currentSampleRate);

    // Rebuild the passthrough chain after the audio device has reported its
    // active channel layout, so startup uses the live I/O configuration.
    rebuildGraphConnections();
    refreshDeviceLists();
}

void AudioHostEngine::audioDeviceStopped()
{
    graph.releaseResources();
}

void AudioHostEngine::audioDeviceError(const juce::String& errorMessage)
{
    setLastError(errorMessage);
}

void AudioHostEngine::initialisePluginFormats()
{
    pluginFormatManager.addFormat(std::make_unique<juce::VST3PluginFormat>());
}

void AudioHostEngine::initialiseAudioDeviceManager()
{
    std::unique_ptr<juce::XmlElement> audioStateXml;

    if (const auto audioStateString = appProperties.getUserSettings()->getValue(settingsAudioStateKey);
        audioStateString.isNotEmpty())
    {
        audioStateXml = juce::parseXML(audioStateString);
    }

    if (audioStateXml != nullptr)
        deviceManager.initialise(2, 2, audioStateXml.get(), true);
    else
        deviceManager.initialiseWithDefaultDevices(2, 2);
}

void AudioHostEngine::createBaseGraphNodes()
{
    audioInputNode = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                                       juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode),
                                   std::nullopt,
                                   juce::AudioProcessorGraph::UpdateKind::none);

    audioOutputNode = graph.addNode(std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
                                        juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode),
                                    std::nullopt,
                                    juce::AudioProcessorGraph::UpdateKind::none);

    inputGainNode = graph.addNode(std::make_unique<GainProcessor>("Input Gain"),
                                  std::nullopt,
                                  juce::AudioProcessorGraph::UpdateKind::none);

    inputAnalysisNode = graph.addNode(std::make_unique<AnalysisTapProcessor>("Input Analysis",
                                                                             inputLeftDb,
                                                                             inputRightDb,
                                                                             &tunerFrequencyHz,
                                                                             &tunerSignalLevel,
                                                                             &tunerCents,
                                                                             &tunerMidiNote),
                                      std::nullopt,
                                      juce::AudioProcessorGraph::UpdateKind::none);

    outputGainNode = graph.addNode(std::make_unique<GainProcessor>("Output Gain"),
                                   std::nullopt,
                                   juce::AudioProcessorGraph::UpdateKind::none);

    outputAnalysisNode = graph.addNode(std::make_unique<AnalysisTapProcessor>("Output Analysis",
                                                                              outputLeftDb,
                                                                              outputRightDb),
                                       std::nullopt,
                                       juce::AudioProcessorGraph::UpdateKind::none);

    setInputGainDb(inputGainDb);
    setOutputGainDb(outputGainDb);
    rebuildGraphConnections();
}

void AudioHostEngine::rebuildGraphConnections()
{
    for (const auto& connection : graph.getConnections())
        graph.removeConnection(connection, juce::AudioProcessorGraph::UpdateKind::none);

    const auto connectNodePair = [&] (const juce::AudioProcessorGraph::NodeID source, const juce::AudioProcessorGraph::NodeID destination)
    {
        const auto* sourceNode = graph.getNodeForId(source);
        const auto* destinationNode = graph.getNodeForId(destination);

        if (sourceNode == nullptr || destinationNode == nullptr)
            return;

        const auto sourceChannels = juce::jmax(1, sourceNode->getProcessor()->getTotalNumOutputChannels());
        const auto destinationChannels = juce::jmax(1, destinationNode->getProcessor()->getTotalNumInputChannels());
        const auto channelsToConnect = juce::jmin(2, sourceChannels, destinationChannels);

        for (int channel = 0; channel < channelsToConnect; ++channel)
            addConnectionForChannels(graph, source, channel, destination, channel);

        if (sourceChannels == 1 && destinationChannels > 1)
            addConnectionForChannels(graph, source, 0, destination, 1);
    };

    connectNodePair(audioInputNode->nodeID, inputGainNode->nodeID);
    connectNodePair(inputGainNode->nodeID, inputAnalysisNode->nodeID);

    auto previousNodeId = inputAnalysisNode->nodeID;

    for (const auto& plugin : hostedPlugins)
    {
        if (! plugin.isEnabled)
            continue;

        connectNodePair(previousNodeId, plugin.node->nodeID);
        previousNodeId = plugin.node->nodeID;
    }

    connectNodePair(previousNodeId, outputGainNode->nodeID);
    connectNodePair(outputGainNode->nodeID, outputAnalysisNode->nodeID);
    connectNodePair(outputAnalysisNode->nodeID, audioOutputNode->nodeID);
    graph.rebuild();
    persistState();
}

void AudioHostEngine::refreshDeviceLists()
{
    auto& deviceTypes = deviceManager.getAvailableDeviceTypes();
    availableDeviceTypes.clear();
    juce::String preferredDeviceType;

    for (auto* type : deviceTypes)
    {
        type->scanForDevices();
        availableDeviceTypes.push_back(type->getTypeName());

        if (preferredDeviceType.isEmpty() && type->getTypeName().containsIgnoreCase("ASIO"))
        {
            preferredDeviceType = type->getTypeName();
        }
    }

    if (currentDeviceType.isEmpty() && preferredDeviceType.isNotEmpty())
        deviceManager.setCurrentAudioDeviceType(preferredDeviceType, true);

    currentDeviceType = deviceManager.getCurrentAudioDeviceType();
    updateDeviceOptionsForType(currentDeviceType);
}

void AudioHostEngine::updateDeviceOptionsForType(const juce::String& deviceType)
{
    inputDevices.clear();
    outputDevices.clear();
    outputChannelOptions.clear();
    bufferSizeOptions.clear();
    sampleRateOptions.clear();

    for (auto* type : deviceManager.getAvailableDeviceTypes())
    {
        type->scanForDevices();

        if (type->getTypeName() != deviceType)
            continue;

        for (const auto& name : type->getDeviceNames(true))
            inputDevices.push_back({ makeDeviceId(deviceType, name), name, deviceType });

        for (const auto& name : type->getDeviceNames(false))
            outputDevices.push_back({ makeDeviceId(deviceType, name), name, deviceType });
    }

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        const auto outputChannelNames = device->getOutputChannelNames();

        for (int channelIndex = 0; channelIndex < outputChannelNames.size(); ++channelIndex)
        {
            const auto channelName = outputChannelNames[channelIndex].isNotEmpty()
                                   ? outputChannelNames[channelIndex]
                                   : "Output " + juce::String(channelIndex + 1);
            outputChannelOptions.push_back({ makeChannelId(channelIndex), channelName, channelIndex });
        }

        for (const auto sampleRate : device->getAvailableSampleRates())
            sampleRateOptions.push_back(sampleRate);

        for (const auto bufferSize : device->getAvailableBufferSizes())
            bufferSizeOptions.push_back(bufferSize);
    }
}

void AudioHostEngine::persistState() const
{
    auto* settings = appProperties.getUserSettings();
    settings->setValue(settingsLanguageKey, language);
    settings->setValue(settingsThemeKey, theme);
    settings->setValue(settingsInputGainKey, inputGainDb);
    settings->setValue(settingsOutputGainKey, outputGainDb);

    if (auto xml = knownPluginList.createXml())
        settings->setValue(settingsPluginListKey, xml->toString());

    if (auto xml = deviceManager.createStateXml())
        settings->setValue(settingsAudioStateKey, xml->toString());

    settings->saveIfNeeded();
}

void AudioHostEngine::restoreState()
{
    auto* settings = appProperties.getUserSettings();
    language = sanitiseLanguage(settings->getValue(settingsLanguageKey, "en"));
    theme = sanitiseTheme(settings->getValue(settingsThemeKey, "dark"));
    inputGainDb = static_cast<float>(settings->getDoubleValue(settingsInputGainKey, 0.0));
    outputGainDb = static_cast<float>(settings->getDoubleValue(settingsOutputGainKey, 0.0));

    if (const auto xmlString = settings->getValue(settingsPluginListKey); xmlString.isNotEmpty())
        if (auto xml = juce::parseXML(xmlString))
            knownPluginList.recreateFromXml(*xml);
}

void AudioHostEngine::clearEditors()
{
    editorWindows.clear();
}

void AudioHostEngine::closeEditorForPlugin(const juce::String& chainPluginId)
{
    editorWindows.erase(chainPluginId);
}

void AudioHostEngine::setLastError(const juce::String& message)
{
    lastError = message;
    statusMessage = message;
}

AudioHostEngine::HostedPlugin* AudioHostEngine::findHostedPlugin(const juce::String& chainPluginId)
{
    const auto iterator = std::find_if(hostedPlugins.begin(), hostedPlugins.end(), [&] (const auto& plugin)
    {
        return plugin.chainId == chainPluginId;
    });

    return iterator != hostedPlugins.end() ? &*iterator : nullptr;
}

const AudioHostEngine::HostedPlugin* AudioHostEngine::findHostedPlugin(const juce::String& chainPluginId) const
{
    const auto iterator = std::find_if(hostedPlugins.begin(), hostedPlugins.end(), [&] (const auto& plugin)
    {
        return plugin.chainId == chainPluginId;
    });

    return iterator != hostedPlugins.end() ? &*iterator : nullptr;
}

} // namespace kasane
