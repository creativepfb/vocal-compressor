#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"
#include "../DSP/SpectrumAnalyzer.h"

/** RTA simplificado: espectro contínuo (20Hz-20kHz, escala log), lido do
    SpectrumAnalyzer do processor. Placeholder de layout - a curva do EQ em
    cima disso vem depois. */
class RTADisplay : public juce::Component, private juce::Timer
{
public:
    RTADisplay(const SpectrumAnalyzer& analyzerIn, double sampleRateIn)
        : analyzer(analyzerIn), sampleRate(sampleRateIn)
    {
        startTimerHz(24);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 4.0f);
        g.setColour(juce::Colour(0xff06060a));
        g.fillRoundedRectangle(bounds, 4.0f);

        auto area = bounds.reduced(3.0f);
        drawGrid(g, area);
        drawSpectrum(g, area);

        g.setColour(juce::Colour(0xff2a2a30));
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

private:
    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb = -80.0f;
    static constexpr float maxDb = 0.0f;

    float freqToX(float freq, juce::Rectangle<float> area) const
    {
        float t = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
        return area.getX() + t * area.getWidth();
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour(juce::Colour(0xff1c1c22));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
        {
            float x = freqToX(f, area);
            g.drawVerticalLine(juce::roundToInt(x), area.getY(), area.getBottom());
        }
    }

    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> area)
    {
        constexpr int numBins = SpectrumAnalyzer::numBins;
        float binHz = (float) (sampleRate / (double) SpectrumAnalyzer::fftSize);

        juce::Path path;
        path.startNewSubPath(area.getX(), area.getBottom());

        bool first = true;
        for (int i = 1; i < numBins; ++i)
        {
            float freq = (float) i * binHz;
            if (freq < minFreq)
                continue;
            if (freq > maxFreq)
                break;

            float db = juce::jlimit(minDb, maxDb, analyzer.magnitudesDb[(size_t) i].load());
            float x = freqToX(freq, area);
            float y = juce::jmap(db, minDb, maxDb, area.getBottom(), area.getY());

            if (first)
            {
                path.lineTo(x, y);
                first = false;
            }
            else
            {
                path.lineTo(x, y);
            }
        }
        path.lineTo(area.getRight(), area.getBottom());
        path.closeSubPath();

        g.setColour(NFLookAndFeel::kGreen.withAlpha(0.18f));
        g.fillPath(path);
        g.setColour(NFLookAndFeel::kGreen.withAlpha(0.75f));
        g.strokePath(path, juce::PathStrokeType(1.3f));
    }

    void timerCallback() override { repaint(); }

    const SpectrumAnalyzer& analyzer;
    double sampleRate;
};
