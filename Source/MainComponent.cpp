#include "MainComponent.h"

#include "AppState.h"
#include "KasaneFrontendData.h"

#include <cmath>
#include <cstddef>
#include <cstring>
#include <vector>

namespace kasane
{
namespace
{
juce::String getMimeTypeForPath(const juce::String& path)
{
    const auto trimmedPath = path.trim();
    const auto lowerPath = trimmedPath.toLowerCase();

    if (lowerPath.endsWith(".html"))
        return "text/html";
    if (lowerPath.endsWith(".css"))
        return "text/css";
    if (lowerPath.endsWith(".js"))
        return "application/javascript";
    if (lowerPath.endsWith(".json"))
        return "application/json";
    if (lowerPath.endsWith(".svg"))
        return "image/svg+xml";
    if (lowerPath.endsWith(".png"))
        return "image/png";
    if (lowerPath.endsWith(".jpg") || lowerPath.endsWith(".jpeg"))
        return "image/jpeg";
    if (lowerPath.endsWith(".ttf"))
        return "font/ttf";
    if (lowerPath.endsWith(".otf"))
        return "font/otf";
    if (lowerPath.endsWith(".woff"))
        return "font/woff";
    if (lowerPath.endsWith(".woff2"))
        return "font/woff2";

    return "application/octet-stream";
}

std::vector<std::byte> toByteVector(const juce::MemoryBlock& block)
{
    std::vector<std::byte> data(block.getSize());

    if (block.getSize() > 0)
        std::memcpy(data.data(), block.getData(), block.getSize());

    return data;
}

juce::String readStringArgument(const juce::Array<juce::var>& arguments, int index, const juce::String& fallback = {})
{
    if (juce::isPositiveAndBelow(index, arguments.size()) && arguments[index].isString())
        return arguments[index].toString();

    return fallback;
}

double readDoubleArgument(const juce::Array<juce::var>& arguments, int index, double fallback = 0.0)
{
    if (juce::isPositiveAndBelow(index, arguments.size()) && (arguments[index].isDouble() || arguments[index].isInt() || arguments[index].isInt64()))
        return static_cast<double>(arguments[index]);

    return fallback;
}

bool readBoolArgument(const juce::Array<juce::var>& arguments, int index, bool fallback = false)
{
    if (juce::isPositiveAndBelow(index, arguments.size()) && arguments[index].isBool())
        return static_cast<bool>(arguments[index]);

    return fallback;
}

juce::String normaliseEmbeddedResourcePath(const juce::String& path)
{
    auto relativePath = path.trim();

    if (relativePath.isEmpty() || relativePath == "/")
        relativePath = "index.html";
    else
    {
        const auto queryIndex = relativePath.indexOfChar('?');
        const auto hashIndex = relativePath.indexOfChar('#');
        auto truncateIndex = -1;

        if (queryIndex >= 0)
            truncateIndex = queryIndex;

        if (hashIndex >= 0)
            truncateIndex = truncateIndex >= 0 ? juce::jmin(truncateIndex, hashIndex) : hashIndex;

        if (truncateIndex >= 0)
            relativePath = relativePath.substring(0, truncateIndex);

        while (relativePath.startsWith("./"))
            relativePath = relativePath.substring(2);

        if (relativePath.startsWithChar('/'))
            relativePath = relativePath.substring(1);
    }

    return relativePath;
}

const char* getEmbeddedResourceData(const juce::String& path, int& dataSize)
{
    const auto relativePath = normaliseEmbeddedResourcePath(path);
    const auto originalFileName = relativePath.fromLastOccurrenceOf("/", false, false);

    if (relativePath == "index.html")
        return KasaneFrontendData::getNamedResource("index_html", dataSize);

    for (auto index = 0; index < KasaneFrontendData::namedResourceListSize; ++index)
    {
        if (originalFileName == KasaneFrontendData::originalFilenames[index])
            return KasaneFrontendData::getNamedResource(KasaneFrontendData::namedResourceList[index], dataSize);
    }

    dataSize = 0;
    return nullptr;
}
} // namespace

MainComponent::MainComponent(juce::ApplicationProperties& properties)
    : appProperties(properties)
{
    setOpaque(true);
    startupMessage = "Preparing audio engine and interface...";
}

void MainComponent::initialiseAsync(StatusCallback onStatus,
                                    ReadyCallback onBackendReady,
                                    ReadyCallback onFrontendReady)
{
    if (initialised)
        return;

    initialised = true;
    startupStatusCallback = std::move(onStatus);
    backendReadyCallback = std::move(onBackendReady);
    frontendReadyCallback = std::move(onFrontendReady);
    startupStage = StartupStage::createEngine;

    reportStartupStatus("Preparing startup...");
    scheduleStartupAdvance();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(15, 23, 42));
}

void MainComponent::paintOverChildren(juce::Graphics& g)
{
    if (!startupOverlayVisible)
        return;

    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient backgroundGradient(juce::Colour::fromRGB(11, 17, 30), bounds.getTopLeft(),
                                            juce::Colour::fromRGB(7, 9, 14), bounds.getBottomRight(), false);
    backgroundGradient.addColour(0.35, juce::Colour::fromRGBA(232, 122, 28, 38));
    backgroundGradient.addColour(0.7, juce::Colour::fromRGBA(74, 222, 128, 20));
    g.setGradientFill(backgroundGradient);
    g.fillAll();

    auto panelBounds = getLocalBounds().reduced(48).withSizeKeepingCentre(560, 240).toFloat();
    g.setColour(juce::Colour::fromRGBA(10, 10, 10, 210));
    g.fillRoundedRectangle(panelBounds, 18.0f);

    g.setColour(juce::Colour::fromRGBA(255, 255, 255, 24));
    g.drawRoundedRectangle(panelBounds, 18.0f, 1.0f);

    auto contentBounds = panelBounds.reduced(28.0f, 24.0f);

    g.setColour(juce::Colour::fromRGB(232, 122, 28));
    g.setFont(juce::FontOptions(12.0f).withKerningFactor(0.28f));
    g.drawText("AUDIO HOST", contentBounds.removeFromTop(18.0f), juce::Justification::topLeft);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(48.0f, juce::Font::bold));
    g.drawText("Kasane", contentBounds.removeFromTop(62.0f), juce::Justification::topLeft);

    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText(startupOverlayHasError ? "Startup issue detected" : "Initializing application",
               contentBounds.removeFromTop(34.0f),
               juce::Justification::topLeft);

    g.setColour(startupOverlayHasError ? juce::Colour::fromRGB(248, 113, 113)
                                       : juce::Colour::fromRGB(214, 214, 214));
    g.setFont(juce::FontOptions(16.0f));
    g.drawFittedText(startupMessage,
                     contentBounds.removeFromTop(58.0f).toNearestInt(),
                     juce::Justification::topLeft,
                     3,
                     1.0f);

    if (!startupOverlayHasError)
    {
        auto progressBounds = contentBounds.removeFromTop(14.0f).reduced(0.0f, 5.0f);
        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 18));
        g.fillRoundedRectangle(progressBounds, 2.0f);

        const auto indicatorWidth = juce::jlimit(96.0f, progressBounds.getWidth(), progressBounds.getWidth() * 0.32f);
        const auto travel = juce::jmax(0.0f, progressBounds.getWidth() - indicatorWidth);
        const auto phase = static_cast<float>((juce::Time::getMillisecondCounterHiRes() * 0.001) * 1.6);
        const auto oscillation = 0.5f + 0.5f * std::sin(phase);
        auto indicatorBounds = progressBounds.withWidth(indicatorWidth).translated(travel * oscillation, 0.0f);

        juce::ColourGradient indicatorGradient(juce::Colour::fromRGBA(232, 122, 28, 0), indicatorBounds.getTopLeft(),
                                               juce::Colour::fromRGBA(232, 122, 28, 242), indicatorBounds.getCentre(),
                                               false);
        indicatorGradient.addColour(1.0, juce::Colour::fromRGBA(232, 122, 28, 0));
        g.setGradientFill(indicatorGradient);
        g.fillRoundedRectangle(indicatorBounds, 2.0f);
    }
}

void MainComponent::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}

juce::WebBrowserComponent::Options MainComponent::createBrowserOptions()
{
    auto options = juce::WebBrowserComponent::Options{}
                       .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
                       .withKeepPageLoadedWhenBrowserIsHidden()
                       .withNativeIntegrationEnabled()
                       .withInitialisationData("bridgeVersion", juce::String(kasane::bridgeVersion))
                       .withWinWebView2Options(
                           juce::WebBrowserComponent::Options::WinWebView2{}
                               .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                                                       .getChildFile("kasane-webview2"))
                               .withStatusBarDisabled()
                               .withBuiltInErrorPageDisabled()
                               .withBackgroundColour(juce::Colour::fromRGB(15, 23, 42)))
                       .withResourceProvider(
                           [this](const juce::String& path)
                           {
                               return getResource(path);
                           });

    options = options
        .withNativeFunction("frontendReady",
                            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                reportStartupStatus("Applying startup state...");
                                completion(engine != nullptr ? toVar(engine->getAppStateSnapshot()) : juce::var());
                                emitBootstrapState();
                            })
        .withNativeFunction("frontendVisualReady",
                            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                browserReady = true;
                                hideStartupOverlay();
                                startupStage = StartupStage::finished;

                                if (frontendReadyCallback != nullptr)
                                    frontendReadyCallback();

                                completion(true);
                            })
        .withNativeFunction("setLanguage",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    engine->setLanguage(readStringArgument(args, 0, "en"));
                                emitBootstrapState();
                                completion(true);
                            })
        .withNativeFunction("setTheme",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    engine->setTheme(readStringArgument(args, 0, "dark"));
                                emitBootstrapState();
                                completion(true);
                            })
        .withNativeFunction("setBpm",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    engine->setBpm(readDoubleArgument(args, 0, 120.0));
                                emitBootstrapState();
                                completion(true);
                            })
        .withNativeFunction("setInputGain",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    engine->setInputGainDb(static_cast<float>(readDoubleArgument(args, 0)));
                                emitAudioState();
                                completion(true);
                            })
        .withNativeFunction("setOutputGain",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    engine->setOutputGainDb(static_cast<float>(readDoubleArgument(args, 0)));
                                emitAudioState();
                                completion(true);
                            })
        .withNativeFunction("scanPlugins",
                            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->scanForPlugins() : false);
                                emitPluginState();
                                emitErrorIfNeeded();
                            })
        .withNativeFunction("addPlugin",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->addPluginToChain(readStringArgument(args, 0)) : false);
                                emitChainState();
                                emitPluginState();
                                emitErrorIfNeeded();
                            })
        .withNativeFunction("removePlugin",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->removePluginFromChain(readStringArgument(args, 0)) : false);
                                emitChainState();
                                emitErrorIfNeeded();
                            })
        .withNativeFunction("togglePlugin",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->togglePluginBypass(readStringArgument(args, 0)) : false);
                                emitChainState();
                                emitErrorIfNeeded();
                            })
        .withNativeFunction("reorderPlugins",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->reorderPlugin(readStringArgument(args, 0), static_cast<int>(readDoubleArgument(args, 1))) : false);
                                emitChainState();
                                emitErrorIfNeeded();
                            })
        .withNativeFunction("openPluginEditor",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->openPluginEditor(readStringArgument(args, 0), *this) : false);
                                emitErrorIfNeeded();
                            })
        .withNativeFunction("openAudioSettings",
                            [this](const juce::Array<juce::var>&, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                {
                                    engine->refreshAudioOptions();
                                    completion(toVar(engine->getAudioStateSnapshot()));
                                }
                                else
                                {
                                    completion(juce::var());
                                }
                                emitAudioState();
                            })
        .withNativeFunction("previewAudioDeviceSetup",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    completion(toVar(engine->previewAudioDeviceSetup(readStringArgument(args, 0),
                                                                                    readStringArgument(args, 1))));
                                else
                                    completion(juce::var());
                            })
        .withNativeFunction("setAudioDeviceType",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                {
                                    completion(engine->setAudioDeviceType(readStringArgument(args, 0)));
                                    emitAudioState();
                                    emitErrorIfNeeded();
                                }
                                else
                                {
                                    completion(false);
                                }
                            })
        .withNativeFunction("toggleTuner",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                if (engine != nullptr)
                                    engine->setTunerOpen(readBoolArgument(args, 0, true));
                                completion(true);
                                emitTunerState();
                            })
        .withNativeFunction("setAudioDeviceSetup",
                            [this](const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                            {
                                completion(engine != nullptr ? engine->setAudioDeviceSetup(readStringArgument(args, 0),
                                                                                           readStringArgument(args, 1),
                                                                                           readDoubleArgument(args, 2),
                                                                                           static_cast<int>(readDoubleArgument(args, 3)),
                                                                                           readStringArgument(args, 4),
                                                                                           readStringArgument(args, 5),
                                                                                           readStringArgument(args, 6),
                                                                                           readStringArgument(args, 7))
                                                             : false);
                                emitAudioState();
                                emitErrorIfNeeded();
                            });

    return options;
}

std::optional<MainComponent::Resource> MainComponent::getResource(const juce::String& path) const
{
    const auto normalisedPath = normaliseEmbeddedResourcePath(path);
    auto dataSize = 0;
    const auto* data = getEmbeddedResourceData(normalisedPath, dataSize);

    if (data == nullptr || dataSize <= 0)
        return std::nullopt;

    juce::MemoryBlock bytes { data, static_cast<size_t>(dataSize) };
    return Resource { toByteVector(bytes), getMimeTypeForPath(normalisedPath) };
}

juce::String MainComponent::getStartupErrorMessage() const
{
    const auto options = const_cast<MainComponent*>(this)->createBrowserOptions();
    auto dataSize = 0;

    if (getEmbeddedResourceData("/", dataSize) == nullptr || dataSize <= 0)
        return "Kasane frontend assets were not embedded correctly. Rebuild the app.";

    if (!juce::WebBrowserComponent::areOptionsSupported(options))
        return "The WebView2 backend is unavailable. Rebuild with WebView2 support enabled.";

    return {};
}

void MainComponent::setStartupMessage(const juce::String& message, bool isError)
{
    startupMessage = message;
    startupOverlayHasError = isError;
    startupOverlayVisible = true;
    repaint();
}

void MainComponent::hideStartupOverlay()
{
    if (!startupOverlayVisible)
        return;

    startupOverlayVisible = false;
    startupOverlayHasError = false;
    repaint();
}

void MainComponent::advanceStartup()
{
    switch (startupStage)
    {
        case StartupStage::createEngine:
            reportStartupStatus("Preparing audio engine...");
            engine = std::make_unique<AudioHostEngine>(appProperties);
            startupStage = StartupStage::prepareEngine;
            scheduleStartupAdvance();
            return;

        case StartupStage::prepareEngine:
            reportStartupStatus("Preparing plugin formats...");
            engine->prepareStartup();
            startupStage = StartupStage::restoreState;
            scheduleStartupAdvance();
            return;

        case StartupStage::restoreState:
            reportStartupStatus("Loading saved settings...");
            engine->restorePersistedState();
            startupStage = StartupStage::initialiseRuntime;
            scheduleStartupAdvance();
            return;

        case StartupStage::initialiseRuntime:
            reportStartupStatus("Initializing audio devices and graph...");
            engine->initialiseRuntime();
            startupStage = StartupStage::restorePlugins;
            scheduleStartupAdvance();
            return;

        case StartupStage::restorePlugins:
        {
            const auto totalPlugins = engine->getTotalPersistedPluginsToRestore();

            if (engine->hasPendingPersistedPluginsToRestore())
            {
                const auto remainingPlugins = engine->getRemainingPersistedPluginsToRestore();
                const auto currentIndex = totalPlugins - remainingPlugins + 1;
                reportStartupStatus("Restoring saved plug-in chain... (" + juce::String(currentIndex) + "/" + juce::String(totalPlugins) + ")");
                engine->restoreNextHostedPluginFromState();
                scheduleStartupAdvance();
                return;
            }

            engine->finishHostedPluginRestore();
            startupStage = StartupStage::completeStartup;
            scheduleStartupAdvance();
            return;
        }

        case StartupStage::completeStartup:
            reportStartupStatus("Finalizing startup state...");
            engine->completeStartup();
            startupStage = StartupStage::createWebView;
            scheduleStartupAdvance();
            return;

        case StartupStage::createWebView:
        {
            const auto startupError = getStartupErrorMessage();
            if (startupError.isNotEmpty())
            {
                reportStartupStatus(startupError, true);
                startupStage = StartupStage::failed;
                return;
            }

            reportStartupStatus("Loading interface...");
            const auto options = createBrowserOptions();

            if (!juce::WebBrowserComponent::areOptionsSupported(options))
            {
                reportStartupStatus("The WebView2 backend is unavailable. Rebuild with WebView2 support enabled.", true);
                startupStage = StartupStage::failed;
                return;
            }

            webView = std::make_unique<juce::WebBrowserComponent>(options);
            addAndMakeVisible(*webView);
            webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
            startTimerHz(12);
            resized();
            startupStage = StartupStage::waitingForFrontend;

            if (backendReadyCallback != nullptr)
                backendReadyCallback();

            return;
        }

        case StartupStage::waitingForFrontend:
        case StartupStage::failed:
        case StartupStage::finished:
            return;
    }
}

void MainComponent::scheduleStartupAdvance()
{
    juce::Component::SafePointer<MainComponent> safeThis(this);
    juce::MessageManager::callAsync([safeThis]
    {
        if (safeThis != nullptr)
            safeThis->advanceStartup();
    });
}

void MainComponent::reportStartupStatus(const juce::String& message, bool isError)
{
    setStartupMessage(message, isError);

    if (startupStatusCallback != nullptr)
        startupStatusCallback(message, isError);
}

void MainComponent::timerCallback()
{
    if (startupOverlayVisible && !startupOverlayHasError)
        repaint();

    if (!browserReady || webView == nullptr)
        return;

    emitMeterState();
    emitTunerState();
    emitErrorIfNeeded();
}

void MainComponent::emitBootstrapState()
{
    if (webView != nullptr && engine != nullptr)
        webView->emitEventIfBrowserIsVisible("bootstrapState", toVar(engine->getAppStateSnapshot()));
}

void MainComponent::emitAudioState()
{
    if (webView != nullptr && engine != nullptr)
        webView->emitEventIfBrowserIsVisible("audioStateChanged", toVar(engine->getAudioStateSnapshot()));
}

void MainComponent::emitPluginState()
{
    if (webView != nullptr && engine != nullptr)
        webView->emitEventIfBrowserIsVisible("pluginListChanged", toVar(engine->getAppStateSnapshot()));
}

void MainComponent::emitChainState()
{
    if (webView != nullptr && engine != nullptr)
        webView->emitEventIfBrowserIsVisible("pluginChainChanged", toVar(engine->getAppStateSnapshot()));
}

void MainComponent::emitTunerState()
{
    if (webView != nullptr && engine != nullptr)
        webView->emitEventIfBrowserIsVisible("tunerUpdated", toVar(engine->getTunerStateSnapshot()));
}

void MainComponent::emitMeterState()
{
    if (webView != nullptr && engine != nullptr)
        webView->emitEventIfBrowserIsVisible("meterUpdated", toVar(engine->getMeterStateSnapshot()));
}

void MainComponent::emitErrorIfNeeded()
{
    if (webView == nullptr)
        return;

    if (engine != nullptr)
    {
        if (const auto errorMessage = engine->consumeLastError())
            webView->emitEventIfBrowserIsVisible("errorRaised", *errorMessage);
    }
}

} // namespace kasane
