#include <JuceHeader.h>

#include "MainComponent.h"

namespace
{
constexpr auto windowBoundsKey = "window.bounds";
constexpr auto fixedWindowWidth = 900;
constexpr auto fixedWindowHeight = 640;
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
                                   juce::DocumentWindow::closeButton),
              properties(propertiesIn)
        {
            setUsingNativeTitleBar(true);
            setResizable(false, false);
            setContentOwned(new kasane::MainComponent(properties), true);

            const auto savedBounds = juce::Rectangle<int>::fromString(properties.getUserSettings()->getValue(windowBoundsKey));

            if (savedBounds.isEmpty())
                centreWithSize(fixedWindowWidth, fixedWindowHeight);
            else
                setBounds(savedBounds.withSizeKeepingCentre(fixedWindowWidth, fixedWindowHeight));

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
