#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/** Texto estático curto (ex.: "BYPASS" acima do botão físico) desenhado
    direto, sem juce::Label/drawFittedText - mesma técnica do ValueReadout,
    mas com cor configurável (não é sempre verde de valor de knob). */
class CaptionLabel : public juce::Component
{
public:
    void setText(const juce::String& newText)
    {
        if (text != newText)
        {
            text = newText;
            repaint();
        }
    }

    void setFontHeight(float newHeight) { fontHeight = newHeight; }
    void setColour(juce::Colour newColour) { colour = newColour; }

    void paint(juce::Graphics& g) override
    {
        g.setColour(colour);
        g.setFont(juce::Font(juce::FontOptions(fontHeight, juce::Font::bold)));
        g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
    }

private:
    juce::String text;
    float fontHeight = 12.0f;
    juce::Colour colour { 0xffe8e8ec };
};
