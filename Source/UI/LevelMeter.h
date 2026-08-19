#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Medidor de nível vertical (0 a -60 dB), verde->amarelo->vermelho, enche de baixo pra cima. */
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter(std::atomic<float>& levelDbSource) : level(levelDbSource)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        float db = juce::jlimit(-60.0f, 0.0f, displayed);
        float fraction = (db + 60.0f) / 60.0f;
        NFLookAndFeel::drawLevelMeterBar(g, getLocalBounds().toFloat(), fraction);
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
