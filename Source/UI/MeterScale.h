#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Coluna de números em dB ao lado de um medidor (0 no topo, minDb na base). */
class MeterScale : public juce::Component
{
public:
    MeterScale(float minDbIn, std::vector<float> ticksIn)
        : minDb(minDbIn), ticks(std::move(ticksIn)) {}

    void paint(juce::Graphics& g) override
    {
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(NFLookAndFeel::kTextDim);

        auto h = (float) getHeight();
        for (float db : ticks)
        {
            float fraction = db / minDb; // minDb é negativo; 0 no topo, 1 na base
            float y = fraction * h;
            juce::String text = juce::String((int) db);
            g.drawFittedText(text, 0, (int) (y - 6), getWidth(), 12, juce::Justification::centred, 1);
        }
    }

private:
    float minDb;
    std::vector<float> ticks;
};
