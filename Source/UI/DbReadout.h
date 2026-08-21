#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Numerozinho verde que mostra o nível atual (em dB) de um atomic, tipo o
    valor real de entrada/saída - complementa o medidor visual. */
class DbReadout : public juce::Label, private juce::Timer
{
public:
    explicit DbReadout(std::atomic<float>& dbSource) : level(dbSource)
    {
        setJustificationType(juce::Justification::centred);
        setColour(juce::Label::textColourId, NFLookAndFeel::kGreen);
        setFont(juce::Font(juce::FontOptions(36.0f, juce::Font::bold)));
        setInterceptsMouseClicks(false, false);
        startTimerHz(12);
    }

private:
    void timerCallback() override
    {
        float db = level.load();
        setText(db <= -59.5f ? juce::String("-inf") : juce::String(db, 1),
                juce::dontSendNotification);
    }

    std::atomic<float>& level;
};
