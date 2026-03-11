#pragma once

#include <JuceHeader.h>

#include "AudioHostEngine.h"

#include <optional>

namespace kasane
{

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    explicit MainComponent(juce::ApplicationProperties& properties);
    ~MainComponent() override = default;

    void initialiseAsync();

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    using Resource = juce::WebBrowserComponent::Resource;

    juce::WebBrowserComponent::Options createBrowserOptions();
    std::optional<Resource> getResource(const juce::String& path) const;
    juce::String getStartupErrorMessage() const;
    juce::File findWebRoot() const;
    juce::File resolveResourceFile(const juce::String& path) const;

    void timerCallback() override;
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
    juce::Label fallbackLabel;
    juce::File webRoot;
    bool browserReady = false;
    bool initialised = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace kasane
