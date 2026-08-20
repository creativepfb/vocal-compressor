#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** "BYPASS" grande, verde, piscando - aparece na caixa preta impressa na
    faceplate (ao lado da palavra "BYPASS" já desenhada no PNG) só quando o
    plugin está em bypass. */
class BypassIndicator : public juce::Component, private juce::Timer
{
public:
    explicit BypassIndicator(std::atomic<float>& bypassParamValueIn) : bypassParamValue(bypassParamValueIn)
    {
        startTimerHz(3);
    }

    void paint(juce::Graphics& g) override
    {
        if (bypassParamValue.load() <= 0.5f || !blinkOn)
            return;

        g.setColour(NFLookAndFeel::kGreen);
        g.setFont(juce::Font(juce::FontOptions((float) getHeight() * 0.62f, juce::Font::bold)));
        g.drawFittedText("BYPASS", getLocalBounds(), juce::Justification::centred, 1);
    }

private:
    void timerCallback() override
    {
        blinkOn = !blinkOn;
        repaint();
    }

    std::atomic<float>& bypassParamValue;
    bool blinkOn = true;
};
