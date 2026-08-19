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

    static constexpr float maxRangeDb = 24.0f;

    void paint(juce::Graphics& g) override
    {
        float gr = juce::jlimit(0.0f, maxRangeDb, -displayedGr);
        float fillFraction = gr / maxRangeDb;
        NFLookAndFeel::drawReductionMeterBar(g, getLocalBounds().toFloat(), fillFraction);
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
