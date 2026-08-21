#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Numerozinho verde que mostra o nível atual (em dB) de um atomic, tipo o
    valor real de entrada/saída - complementa o medidor visual.

    Desenhado direto (sem juce::Label/drawFittedText), mesma técnica do
    ValueReadout dos knobs - Label encolhe o texto sozinho pra caber no
    componente quando a fonte pedida é maior que a área disponível, o que
    anula silenciosamente qualquer setFont() maior (aconteceu de verdade
    aqui: só aumentar a fonte não bastava, o componente precisava ser
    alto o suficiente também - ver readoutY0/Y1 em PluginEditor.cpp). */
class DbReadout : public juce::Component, private juce::Timer
{
public:
    explicit DbReadout(std::atomic<float>& dbSource) : level(dbSource)
    {
        setInterceptsMouseClicks(false, false);
        startTimerHz(12);
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(NFLookAndFeel::kGreen);
        g.setFont(juce::Font(juce::FontOptions(36.0f, juce::Font::bold)));
        g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
    }

private:
    void timerCallback() override
    {
        float db = level.load();
        juce::String newText = db <= -59.5f ? juce::String("-inf") : juce::String(db, 1);
        if (newText != text)
        {
            text = newText;
            repaint();
        }
    }

    std::atomic<float>& level;
    juce::String text;
};
