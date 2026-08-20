#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** RTA simplificado: desenha o histórico recente do nível de saída,
    rolando da esquerda pra direita. Lê direto do buffer circular do
    processor (lock-free, escrita só no audio thread). */
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay(const std::array<std::atomic<float>, 300>& bufferIn,
                     const std::atomic<int>& writeIndexIn)
        : buffer(bufferIn), writeIndex(writeIndexIn)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 4.0f);
        g.setColour(juce::Colour(0xff06060a));
        g.fillRoundedRectangle(bounds, 4.0f);

        auto area = bounds.reduced(3.0f);
        const int n = (int) buffer.size();
        int startIdx = writeIndex.load();

        juce::Path path;
        for (int i = 0; i < n; ++i)
        {
            int idx = (startIdx + i) % n;
            float v = juce::jlimit(0.0f, 1.0f, buffer[(size_t) idx].load());
            float x = area.getX() + area.getWidth() * ((float) i / (float) (n - 1));
            float y = area.getBottom() - v * area.getHeight();

            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }

        g.setColour(NFLookAndFeel::kGreen.withAlpha(0.5f));
        g.strokePath(path, juce::PathStrokeType(2.5f));
        g.setColour(NFLookAndFeel::kGreen);
        g.strokePath(path, juce::PathStrokeType(1.2f));

        g.setColour(juce::Colour(0xff2a2a30));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

private:
    void timerCallback() override { repaint(); }

    const std::array<std::atomic<float>, 300>& buffer;
    const std::atomic<int>& writeIndex;
};
