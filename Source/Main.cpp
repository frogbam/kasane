#include <JuceHeader.h>

#include "MainComponent.h"

namespace
{
constexpr auto windowBoundsKey = "window.bounds";
constexpr auto minimumWindowWidth = 1024;
constexpr auto minimumWindowHeight = 640;
constexpr auto defaultWindowWidth = 1280;
constexpr auto defaultWindowHeight = 800;
}

class KasaneApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Kasane"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "Kasane";
        options.filenameSuffix = "settings";
        options.folderName = "Kasane";
        options.osxLibrarySubFolder = "Application Support";
        appProperties.setStorageParameters(options);

        mainWindow = std::make_unique<MainWindow>(getApplicationName(), appProperties);
    }

    void shutdown() override
    {
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
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        MainWindow(const juce::String& title, juce::ApplicationProperties& propertiesIn)
            : juce::DocumentWindow(title,
                                   juce::Desktop::getInstance().getDefaultLookAndFeel()
                                       .findColour(juce::ResizableWindow::backgroundColourId),
                                   juce::DocumentWindow::allButtons),
              properties(propertiesIn)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, true);
            setResizeLimits(minimumWindowWidth, minimumWindowHeight, 4096, 2160);
            setContentOwned(new kasane::MainComponent(properties), true);

            const auto savedBounds = juce::Rectangle<int>::fromString(properties.getUserSettings()->getValue(windowBoundsKey));

            if (savedBounds.isEmpty())
                centreWithSize(defaultWindowWidth, defaultWindowHeight);
            else
                setBounds(savedBounds.withSizeKeepingCentre(juce::jmax(savedBounds.getWidth(), minimumWindowWidth),
                                                           juce::jmax(savedBounds.getHeight(), minimumWindowHeight)));

            setVisible(true);

            if (auto* mainComponent = dynamic_cast<kasane::MainComponent*>(getContentComponent()))
                mainComponent->initialiseAsync();
        }

        ~MainWindow() override
        {
            properties.getUserSettings()->setValue(windowBoundsKey, getBounds().toString());
            properties.getUserSettings()->saveIfNeeded();
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        juce::ApplicationProperties& properties;
    };

    juce::ApplicationProperties appProperties;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(KasaneApplication)
