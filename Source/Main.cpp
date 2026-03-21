#include <JuceHeader.h>

#include "MainComponent.h"
#include "SplashWindow.h"

namespace
{
constexpr auto windowBoundsKey = "window.bounds";
constexpr auto fixedWindowWidth = 1024;
constexpr auto fixedWindowHeight = 768;
const auto windowBackgroundColour = juce::Colour::fromRGB(15, 23, 42);

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& title, juce::ApplicationProperties& propertiesIn)
        : juce::DocumentWindow(title,
                               windowBackgroundColour,
                               juce::DocumentWindow::closeButton),
          properties(propertiesIn)
    {
        setUsingNativeTitleBar(true);
        setResizable(false, false);
        auto* content = new kasane::MainComponent(properties);
        mainComponent = content;
        setContentOwned(content, true);

        const auto savedBounds = juce::Rectangle<int>::fromString(properties.getUserSettings()->getValue(windowBoundsKey));

        if (savedBounds.isEmpty())
            centreWithSize(fixedWindowWidth, fixedWindowHeight);
        else
            setBounds(savedBounds.withSizeKeepingCentre(fixedWindowWidth, fixedWindowHeight));

        setVisible(false);
    }

    ~MainWindow() override
    {
        properties.getUserSettings()->setValue(windowBoundsKey, getBounds().toString());
        properties.getUserSettings()->saveIfNeeded();
    }

    void beginStartup(kasane::MainComponent::StatusCallback onStatus,
                      kasane::MainComponent::ReadyCallback onBackendReady,
                      kasane::MainComponent::ReadyCallback onFrontendReady)
    {
        if (mainComponent != nullptr)
            mainComponent->initialiseAsync(std::move(onStatus), std::move(onBackendReady), std::move(onFrontendReady));
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    juce::ApplicationProperties& properties;
    kasane::MainComponent* mainComponent = nullptr;
};
} // namespace

class KasaneApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Kasane"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "kasane";
        options.filenameSuffix = "settings";
        options.folderName = "kasane";
        options.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters(options);

        splashWindow = std::make_unique<SplashWindow>(getApplicationName());
        mainWindow = std::make_unique<MainWindow>(getApplicationName(), appProperties);

        mainWindow->beginStartup(
            [this](const juce::String& message, bool isError)
            {
                setSplashStatus(message, isError);
            },
            [this]
            {
                revealMainWindow();
            },
            [this]
            {
                finishStartupTransition();
            });
    }

    void shutdown() override
    {
        splashWindow.reset();
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String&) override
    {
    }

private:
    void setSplashStatus(const juce::String& message, bool isError)
    {
        if (splashWindow != nullptr)
            splashWindow->setStatus(message, isError);
    }

    void revealMainWindow()
    {
        if (mainWindow != nullptr)
            mainWindow->setVisible(true);

        if (splashWindow != nullptr)
            splashWindow->toFront(true);
    }

    void finishStartupTransition()
    {
        splashWindow.reset();

        if (mainWindow != nullptr)
            mainWindow->toFront(true);
    }

    juce::ApplicationProperties appProperties;
    std::unique_ptr<SplashWindow> splashWindow;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(KasaneApplication)
