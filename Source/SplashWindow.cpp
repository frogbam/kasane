#include "SplashWindow.h"

#include <cmath>

namespace
{
constexpr auto splashWindowWidth = 580;
constexpr auto splashWindowHeight = 340;
const auto splashBackgroundColour = juce::Colour::fromRGB(12, 14, 20);
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
        const auto time = static_cast<float>(juce::Time::getMillisecondCounterHiRes() * 0.001);

        paintBackground(g, bounds, time);
        paintBrandSection(g, bounds, time);
        paintStatusCard(g, bounds, time);
    }

private:
    void paintBackground(juce::Graphics& g, const juce::Rectangle<float>& bounds, float time)
    {
        juce::ColourGradient gradient(juce::Colour::fromRGB(18, 22, 30), bounds.getTopLeft(),
                                      juce::Colour::fromRGB(10, 12, 18), bounds.getBottomRight(), false);

        const auto phase1 = std::sin(time * 0.25f) * 0.5f + 0.5f;
        const auto phase2 = std::sin(time * 0.18f + 1.047f) * 0.5f + 0.5f;

        gradient.addColour(0.15f + phase1 * 0.1f,
                           juce::Colour::fromRGBA(232, 122, 28, static_cast<juce::uint8>(18.0f + phase1 * 15.0f)));
        gradient.addColour(0.55f + phase2 * 0.15f,
                           juce::Colour::fromRGBA(74, 222, 128, static_cast<juce::uint8>(12.0f + phase2 * 10.0f)));

        g.setGradientFill(gradient);
        g.fillAll();

        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 6));
        const auto scanY = bounds.getHeight() * static_cast<float>(std::fmod(time * 0.12f, 1.0f));
        g.fillRect(bounds.getX(), scanY, bounds.getWidth(), 1.5f);
    }

    void paintBrandSection(juce::Graphics& g, const juce::Rectangle<float>& bounds, float time)
    {
        const auto brandY = bounds.getHeight() * 0.22f;
        const auto centerX = bounds.getCentreX();

        const auto pulsePhase = std::sin(time * 2.2f) * 0.35f + 0.65f;
        const auto dotColour = juce::Colour::fromRGB(232, 122, 28).withAlpha(pulsePhase);

        g.setColour(dotColour);
        g.fillEllipse(centerX - 90.0f, brandY - 4.0f, 6.0f, 6.0f);

        g.setColour(juce::Colour::fromRGB(148, 163, 184));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("AUDIO HOST", juce::Rectangle<float>(centerX - 80.0f, brandY - 6.0f, 160.0f, 16.0f), juce::Justification::centred);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(38.0f, juce::Font::bold));
        g.drawText("Kasane", juce::Rectangle<float>(centerX - 120.0f, brandY + 14.0f, 240.0f, 50.0f), juce::Justification::centred);

        g.setColour(juce::Colour::fromRGB(100, 116, 139));
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("Amp & FX Host", juce::Rectangle<float>(centerX - 80.0f, brandY + 60.0f, 160.0f, 20.0f), juce::Justification::centred);
    }

    void paintStatusCard(juce::Graphics& g, const juce::Rectangle<float>& bounds, float time)
    {
        const auto cardMargin = 36.0f;
        auto cardBounds = bounds.reduced(cardMargin, 0);

        const auto cardTop = bounds.getHeight() * 0.52f;
        cardBounds.setTop(cardTop);
        cardBounds.setHeight(bounds.getHeight() - cardTop - 28.0f);

        g.setColour(juce::Colour::fromRGBA(16, 16, 22, 230));
        g.fillRoundedRectangle(cardBounds, 14.0f);

        g.setColour(juce::Colour::fromRGBA(255, 255, 255, 12));
        g.drawRoundedRectangle(cardBounds, 14.0f, 1.0f);

        auto contentBounds = cardBounds.reduced(22.0f, 18.0f);

        if (showError)
        {
            paintErrorState(g, contentBounds);
        }
        else
        {
            paintLoadingState(g, contentBounds, time);
        }
    }

    void paintErrorState(juce::Graphics& g, const juce::Rectangle<float>& bounds)
    {
        auto textBounds = bounds;

        g.setColour(juce::Colour::fromRGB(248, 113, 113));
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("Startup issue detected", textBounds.removeFromTop(22.0f), juce::Justification::topLeft);

        g.setColour(juce::Colour::fromRGB(226, 232, 240));
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(statusMessage,
                         textBounds.toNearestInt(),
                         juce::Justification::topLeft,
                         4,
                         1.0f);
    }

    void paintLoadingState(juce::Graphics& g, const juce::Rectangle<float>& bounds, float time)
    {
        auto contentBounds = bounds;

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText("Preparing application", contentBounds.removeFromTop(18.0f), juce::Justification::topLeft);

        g.setColour(juce::Colour::fromRGB(170, 170, 180));
        g.setFont(juce::FontOptions(12.0f));
        g.drawFittedText(statusMessage,
                         contentBounds.removeFromTop(44.0f).toNearestInt(),
                         juce::Justification::topLeft,
                         2,
                         1.0f);

        auto progressBounds = contentBounds.withTrimmedTop(contentBounds.getHeight() - 6.0f);
        if (progressBounds.getHeight() > 0)
        {
            g.setColour(juce::Colour::fromRGBA(255, 255, 255, 10));
            g.fillRoundedRectangle(progressBounds, 3.0f);

            const auto indicatorWidth = progressBounds.getWidth() * 0.22f;
            const auto travel = progressBounds.getWidth() - indicatorWidth;
            const auto phase = time * 0.5f;
            const auto oscillation = (std::sin(phase) + 1.0f) * 0.5f;
            auto indicatorBounds = progressBounds.withWidth(indicatorWidth).translated(travel * oscillation, 0.0f);

            juce::ColourGradient indicatorGradient(juce::Colour::fromRGBA(232, 122, 28, 0), indicatorBounds.getX(), 0,
                                                   juce::Colour::fromRGBA(232, 122, 28, 160), indicatorBounds.getCentreX(), 0,
                                                   false);
            indicatorGradient.addColour(1.0, juce::Colour::fromRGBA(232, 122, 28, 0));
            g.setGradientFill(indicatorGradient);
            g.fillRoundedRectangle(indicatorBounds, 3.0f);
        }
    }

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
                           0)
{
    setUsingNativeTitleBar(false);
    setTitleBarHeight(0);
    setResizable(false, false);
    setAlwaysOnTop(true);

    auto* content = new SplashContent();
    splashContent = content;
    setContentOwned(content, true);
    centreWithSize(splashWindowWidth, splashWindowHeight);
    setVisible(true);
    toFront(true);
}

SplashWindow::~SplashWindow() = default;

void SplashWindow::setStatus(const juce::String& message, bool isError)
{
    if (splashContent != nullptr)
        splashContent->setStatus(message, isError);
}

void SplashWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
