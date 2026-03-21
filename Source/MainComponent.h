#pragma once

#include <JuceHeader.h>

#include "AudioHostEngine.h"

#include <functional>
#include <optional>

namespace kasane
{

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    using StatusCallback = std::function<void(const juce::String&, bool)>;
    using ReadyCallback = std::function<void()>;

    explicit MainComponent(juce::ApplicationProperties& properties);
    ~MainComponent() override = default;

    void initialiseAsync(StatusCallback onStatus,
                         ReadyCallback onBackendReady,
                         ReadyCallback onFrontendReady);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using Resource = juce::WebBrowserComponent::Resource;
    enum class StartupStage
    {
        createEngine,
        prepareEngine,
        restoreState,
        initialiseRuntime,
        restorePlugins,
        completeStartup,
        createWebView,
        waitingForFrontend,
        failed,
        finished
    };

    juce::WebBrowserComponent::Options createBrowserOptions();
    std::optional<Resource> getResource(const juce::String& path) const;
    juce::String getStartupErrorMessage();
    void advanceStartup();
    void scheduleStartupAdvance();
    void reportStartupStatus(const juce::String& message, bool isError = false);

    void timerCallback() override;
    void emitAppState();
    void emitBootstrapState();
    void emitAudioState();
    void emitPluginState();
    void emitChainState();
    void emitTunerState();
    void emitMeterState();
    void emitErrorIfNeeded();

    juce::ApplicationProperties& appProperties;
    std::unique_ptr<AudioHostEngine> engine;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    bool browserReady = false;
    bool initialised = false;
    StartupStage startupStage = StartupStage::createEngine;
    StatusCallback startupStatusCallback;
    ReadyCallback backendReadyCallback;
    ReadyCallback frontendReadyCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace kasane
