#include "SplashWindow.h"

#include <cmath>

namespace
{
constexpr auto splashWindowWidth = 560;
constexpr auto splashWindowHeight = 260;
const auto splashBackgroundColour = juce::Colour::fromRGB(15, 23, 42);
} // namespace

class SplashWindow::SplashContent final : public juce::Component,
                                          private juce::Timer
{
public:
    SplashContent()
    {
        startTimerHz(30);
    }

    void setStatus(const juce::String& message, bool hasError)
    {
        statusMessage = message;
        showError = hasError;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        juce::ColourGradient backgroundGradient(juce::Colour::fromRGB(11, 17, 30), bounds.getTopLeft(),
                                                juce::Colour::fromRGB(7, 9, 14), bounds.getBottomRight(), false);
        backgroundGradient.addColour(0.32, juce::Colour::fromRGBA(232, 122, 28, 42));
        backgroundGradient.addColour(0.72, juce::Colour::fromRGBA(74, 222, 128, 20));
        g.setGradientFill(backgroundGradient);
        g.fillAll();

        auto panelBounds = getLocalBounds().reduced(24).toFloat();
        g.setColour(juce::Colour::fromRGBA(10, 10, 10, 214));
        g.fillRoundedRectangle(panelBounds, 20.0f);

        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 26));
        g.drawRoundedRectangle(panelBounds, 20.0f, 1.0f);

        auto contentBounds = panelBounds.reduced(28.0f, 24.0f);

        g.setColour(juce::Colour::fromRGB(232, 122, 28));
        g.setFont(juce::FontOptions(12.0f).withKerningFactor(0.28f));
        g.drawText("AUDIO HOST", contentBounds.removeFromTop(18.0f), juce::Justification::topLeft);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(46.0f, juce::Font::bold));
        g.drawText("Kasane", contentBounds.removeFromTop(58.0f), juce::Justification::topLeft);

        g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
        g.drawText(showError ? "Startup issue detected" : "Preparing application",
                   contentBounds.removeFromTop(32.0f),
                   juce::Justification::topLeft);

        g.setColour(showError ? juce::Colour::fromRGB(248, 113, 113)
                              : juce::Colour::fromRGB(214, 214, 214));
        g.setFont(juce::FontOptions(16.0f));
        g.drawFittedText(statusMessage,
                         contentBounds.removeFromTop(56.0f).toNearestInt(),
                         juce::Justification::topLeft,
                         3,
                         1.0f);

        if (!showError)
        {
            auto progressBounds = contentBounds.removeFromTop(14.0f).reduced(0.0f, 5.0f);
            g.setColour(juce::Colour::fromRGBA(255, 255, 255, 18));
            g.fillRoundedRectangle(progressBounds, 2.0f);

            const auto indicatorWidth = juce::jlimit(96.0f, progressBounds.getWidth(), progressBounds.getWidth() * 0.3f);
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

private:
    void timerCallback() override
    {
        if (!showError)
            repaint();
    }

    juce::String statusMessage { "Preparing startup..." };
    bool showError = false;
};

SplashWindow::SplashWindow(const juce::String& title)
    : juce::DocumentWindow(title,
                           splashBackgroundColour,
                           juce::DocumentWindow::allButtons & ~juce::DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(false);
    setTitleBarHeight(0);
    setResizable(false, false);
    setAlwaysOnTop(true);

    setContentOwned(new SplashContent(), true);
    centreWithSize(splashWindowWidth, splashWindowHeight);
    setVisible(true);
    toFront(true);
}

SplashWindow::~SplashWindow() = default;

void SplashWindow::setStatus(const juce::String& message, bool isError)
{
    if (auto* content = dynamic_cast<SplashContent*>(getContentComponent()))
        content->setStatus(message, isError);
}

void SplashWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
