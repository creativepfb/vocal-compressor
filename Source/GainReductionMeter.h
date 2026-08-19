#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/NFLookAndFeel.h"

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

        g.setColour(juce::Colour(0xff0a0a0c));
        g.fillRoundedRectangle(bounds, 3.0f);

        float gr = juce::jlimit(0.0f, 24.0f, -displayedGr);
        float fillFraction = gr / 24.0f;

        auto barArea = bounds.reduced(2.0f);
        auto fillHeight = barArea.getHeight() * fillFraction;
        auto fillRect = barArea.removeFromTop(fillHeight);

        juce::Colour meterColour = fillFraction < 0.5f
            ? NFLookAndFeel::kGreen
            : (fillFraction < 0.8f ? NFLookAndFeel::kPurple : juce::Colour(0xffff3b3b));

        g.setColour(meterColour);
        g.fillRoundedRectangle(fillRect, 2.0f);

        g.setColour(juce::Colour(0xff2f2f36));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
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
