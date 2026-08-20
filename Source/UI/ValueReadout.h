#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Texto de valor desenhado direto (sem juce::Label, sem drawFittedText) -
    sempre no tamanho de fonte configurado, nunca encolhe sozinho pra caber. */
class ValueReadout : public juce::Component
{
public:
    void setValueText(const juce::String& newText)
    {
        if (text != newText)
        {
            text = newText;
            repaint();
        }
    }

    void setFontHeight(float newHeight) { fontHeight = newHeight; }

    void paint(juce::Graphics& g) override
    {
        g.setColour(NFLookAndFeel::kKnobGreen);
        g.setFont(juce::Font(juce::FontOptions(fontHeight, juce::Font::bold)));
        g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
    }

private:
    juce::String text;
    float fontHeight = 34.0f;
};
