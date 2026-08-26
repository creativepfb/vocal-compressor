#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Medidor de nível vertical (0 a -60 dB), verde->amarelo->vermelho, enche de
    baixo pra cima - com um pequeno LED de CLIP no topo (latch: acende
    quando o processor detecta pico > 0dBFS, ver
    VocalCompressorAudioProcessor::processBlock, e só apaga quando o
    usuário clica nele). A detecção em si acontece inteira no audio
    thread; este componente só lê o atomic e desenha/reseta - não faz
    nenhuma detecção própria a partir do valor suavizado do medidor. */
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    LevelMeter(std::atomic<float>& levelDbSource, std::atomic<bool>& clipFlagSource)
        : level(levelDbSource), clipFlag(clipFlagSource)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto ledArea = area.removeFromTop(ledHeight).reduced(1.0f, 0.0f);
        area.removeFromTop(2.0f); // respiro entre o LED e a barra

        float db = juce::jlimit(-60.0f, 0.0f, displayed);
        float fraction = (db + 60.0f) / 60.0f;
        NFLookAndFeel::drawLevelMeterBar(g, area, fraction);

        drawClipLed(g, ledArea);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto ledArea = getLocalBounds().toFloat().removeFromTop(ledHeight);
        if (ledArea.contains(e.position))
            clipFlag.store(false);
    }

private:
    static constexpr float ledHeight = 12.0f;

    void drawClipLed(juce::Graphics& g, juce::Rectangle<float> area)
    {
        bool clipped = clipFlag.load();

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(area.translated(0.0f, 1.0f), 2.5f);

        if (clipped)
        {
            // Glow por baixo, igual o resto dos indicadores acesos da
            // interface (HardwareButton/EQGraphComponent).
            g.setColour(NFLookAndFeel::kRed.withAlpha(0.35f));
            g.fillRoundedRectangle(area.expanded(1.5f), 3.0f);
        }

        g.setColour(clipped ? NFLookAndFeel::kRed : juce::Colour(0xff2a1414));
        g.fillRoundedRectangle(area, 2.5f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(area, 2.5f, 1.0f);
    }

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
    std::atomic<bool>& clipFlag;
    float displayed = -60.0f;
};
