#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"
#include "../DSP/SpectrumAnalyzer.h"

/** RTA em barras (estilo analisador de frequência clássico), lido do
    SpectrumAnalyzer do processor. A caixa/moldura já vem desenhada na
    faceplate - aqui só desenha as barras por cima. */
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
        auto area = getLocalBounds().toFloat().reduced(4.0f);
        drawBars(g, area);
    }

private:
    static constexpr float minFreq = 31.0f;
    static constexpr float maxFreq = 16000.0f;
    static constexpr float minDb = -70.0f;
    static constexpr float maxDb = 0.0f;
    static constexpr int numBars = 32;

    void drawBars(juce::Graphics& g, juce::Rectangle<float> area)
    {
        float binHz = (float) (sampleRate / (double) SpectrumAnalyzer::fftSize);
        float barWidth = area.getWidth() / (float) numBars;

        for (int bar = 0; bar < numBars; ++bar)
        {
            // Frequência central da barra, em escala logarítmica.
            float t0 = (float) bar / (float) numBars;
            float t1 = (float) (bar + 1) / (float) numBars;
            float freqLo = minFreq * std::pow(maxFreq / minFreq, t0);
            float freqHi = minFreq * std::pow(maxFreq / minFreq, t1);

            int binLo = juce::jmax(1, (int) (freqLo / binHz));
            int binHi = juce::jmin(SpectrumAnalyzer::numBins - 1, (int) (freqHi / binHz));

            float peakDb = minDb;
            for (int i = binLo; i <= binHi; ++i)
                peakDb = juce::jmax(peakDb, analyzer.magnitudesDb[(size_t) i].load());

            float fraction = juce::jlimit(0.0f, 1.0f, (peakDb - minDb) / (maxDb - minDb));

            float x = area.getX() + (float) bar * barWidth;
            float barH = fraction * area.getHeight();
            juce::Rectangle<float> barRect(x + barWidth * 0.12f, area.getBottom() - barH,
                                            barWidth * 0.76f, barH);

            g.setColour(NFLookAndFeel::kGreen.withAlpha(0.30f));
            g.fillRect(barRect.expanded(0.6f, 0.0f));
            g.setColour(NFLookAndFeel::kGreen);
            g.fillRect(barRect);
        }
    }

    void timerCallback() override { repaint(); }

    const SpectrumAnalyzer& analyzer;
    double sampleRate;
};
