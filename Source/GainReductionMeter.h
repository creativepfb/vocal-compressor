#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    explicit GainReductionMeter(VocalCompressorAudioProcessor& p) : processor(p)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(bounds, 4.0f);

        float gr = juce::jlimit(0.0f, 24.0f, -displayedGr);
        float fillFraction = gr / 24.0f;

        auto barArea = bounds.reduced(2.0f);
        auto fillHeight = barArea.getHeight() * fillFraction;
        auto fillRect = barArea.removeFromTop(fillHeight);

        juce::Colour meterColour = fillFraction < 0.5f
            ? juce::Colours::limegreen
            : (fillFraction < 0.8f ? juce::Colours::yellow : juce::Colours::red);

        g.setColour(meterColour);
        g.fillRoundedRectangle(fillRect, 3.0f);

        g.setColour(juce::Colours::grey);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

private:
    void timerCallback() override
    {
        float target = processor.currentGainReductionDb.load();

        if (target < displayedGr)
            displayedGr = target;
        else
            displayedGr += (target - displayedGr) * 0.2f;

        repaint();
    }

    VocalCompressorAudioProcessor& processor;
    float displayedGr = 0.0f;
};
