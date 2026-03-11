#include "MainComponent.h"

#include "AppState.h"

#include <cstddef>
#include <cstring>
#include <vector>

namespace kasane
{
namespace
{
juce::String getMimeTypeForFile(const juce::File& file)
{
    const auto extension = file.getFileExtension().toLowerCase();

    if (extension == ".html")
        return "text/html";
    if (extension == ".css")
        return "text/css";
    if (extension == ".js")
        return "application/javascript";
    if (extension == ".json")
        return "application/json";
    if (extension == ".svg")
        return "image/svg+xml";
    if (extension == ".png")
        return "image/png";
    if (extension == ".jpg" || extension == ".jpeg")
        return "image/jpeg";

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
} // namespace

MainComponent::MainComponent(juce::ApplicationProperties& properties)
    : appProperties(properties)
{
    setOpaque(true);

    fallbackLabel.setJustificationType(juce::Justification::centred);
    fallbackLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    fallbackLabel.setText("Starting Kasane...", juce::dontSendNotification);
    addAndMakeVisible(fallbackLabel);
}

void MainComponent::initialiseAsync()
{
    if (initialised)
        return;

    initialised = true;

    juce::MessageManager::callAsync([this]
    {
        engine = std::make_unique<AudioHostEngine>(appProperties);
        webRoot = findWebRoot();
        fallbackLabel.setText(getStartupErrorMessage(), juce::dontSendNotification);

        const auto options = createBrowserOptions();

        if (webRoot.isDirectory() && juce::WebBrowserComponent::areOptionsSupported(options))
        {
            webView = std::make_unique<juce::WebBrowserComponent>(options);
            addAndMakeVisible(*webView);
            webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
            startTimerHz(12);
            resized();
        }
        else
        {
            fallbackLabel.toFront(false);
        }
    });
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(15, 23, 42));
}

void MainComponent::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());

    fallbackLabel.setBounds(getLocalBounds().reduced(24));
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
                                                       .getChildFile("KasaneWebView2"))
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
                                browserReady = true;
                                completion(engine != nullptr ? toVar(engine->getAppStateSnapshot()) : juce::var());
                                emitBootstrapState();
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
                                                                                           readStringArgument(args, 5))
                                                             : false);
                                emitAudioState();
                                emitErrorIfNeeded();
                            });

    return options;
}

std::optional<MainComponent::Resource> MainComponent::getResource(const juce::String& path) const
{
    const auto file = resolveResourceFile(path);

    if (!file.existsAsFile())
        return std::nullopt;

    juce::MemoryBlock bytes;

    if (!file.loadFileAsData(bytes))
        return std::nullopt;

    return Resource { toByteVector(bytes), getMimeTypeForFile(file) };
}

juce::String MainComponent::getStartupErrorMessage() const
{
    const auto options = const_cast<MainComponent*>(this)->createBrowserOptions();

    if (!webRoot.isDirectory())
        return "Kasane frontend assets were not found. Build the Vite frontend before launching the app.";

    if (!juce::WebBrowserComponent::areOptionsSupported(options))
        return "The WebView2 backend is unavailable. Rebuild with WebView2 support enabled.";

    return {};
}

juce::File MainComponent::findWebRoot() const
{
    const auto executableFolder = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    const auto bundledWebRoot = executableFolder.getChildFile("frontend");

    if (bundledWebRoot.isDirectory())
        return bundledWebRoot;

   #ifdef KASANE_SOURCE_DIR
    const juce::File sourceRoot { juce::String { KASANE_SOURCE_DIR }.unquoted() };
    const auto distRoot = sourceRoot.getChildFile("frontend").getChildFile("dist");

    if (distRoot.isDirectory())
        return distRoot;
   #endif

    return bundledWebRoot;
}

juce::File MainComponent::resolveResourceFile(const juce::String& path) const
{
    const auto trimmedPath = path.trim();

    if (trimmedPath.isEmpty() || trimmedPath == "/")
        return webRoot.getChildFile("index.html");

    const auto relativePath = trimmedPath.startsWithChar('/') ? trimmedPath.substring(1) : trimmedPath;

    if (relativePath.contains(".."))
        return {};

    return webRoot.getChildFile(relativePath);
}

void MainComponent::timerCallback()
{
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
