#pragma once

#include <JuceHeader.h>

class SplashWindow final : public juce::DocumentWindow
{
public:
    explicit SplashWindow(const juce::String& title);
    ~SplashWindow() override;

    void setStatus(const juce::String& message, bool isError);
    void closeButtonPressed() override;

private:
    class SplashContent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashWindow)
};
