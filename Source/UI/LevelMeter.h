#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Medidor de nível vertical (0 a -60 dB), preenchendo de baixo pra cima. */
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter(std::atomic<float>& levelDbSource) : level(levelDbSource)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(0xff0a0a0c));
        g.fillRoundedRectangle(bounds, 3.0f);

        float db = juce::jlimit(-60.0f, 0.0f, displayed);
        float fraction = (db + 60.0f) / 60.0f;

        auto barArea = bounds.reduced(2.0f);
        auto fillRect = barArea.removeFromBottom(barArea.getHeight() * fraction);

        juce::ColourGradient grad(NFLookAndFeel::kGreen, 0.0f, (float) getHeight(),
                                   NFLookAndFeel::kPurple, 0.0f, 0.0f, false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(fillRect, 2.0f);

        g.setColour(juce::Colour(0xff2f2f36));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    }

private:
    void timerCallback() override
    {
        float target = level.load();
        if (target > displayed)
            displayed = target;
        else
            displayed += (target - displayed) * 0.15f;

        repaint();
    }

    std::atomic<float>& level;
    float displayed = -60.0f;
};
