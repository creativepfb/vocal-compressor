#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"
#include "../DSP/SpectrumAnalyzer.h"

/** RTA em barras (estilo analisador de frequência clássico), lido do
    SpectrumAnalyzer do processor. A caixa/moldura já vem desenhada na
    faceplate - aqui desenha as barras + a régua de frequência por cima. */
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
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        auto labelArea = bounds.removeFromBottom(labelHeight);

        drawGrid(g, bounds);
        drawBars(g, bounds);
        drawLabels(g, labelArea, bounds);
    }

private:
    static constexpr float minFreq = 31.0f;
    static constexpr float maxFreq = 16000.0f;
    static constexpr float minDb = -70.0f;
    static constexpr float maxDb = 0.0f;
    static constexpr int numBars = 32;
    static constexpr float labelHeight = 14.0f;

    static float freqToX(float freq, juce::Rectangle<float> area)
    {
        float t = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
        return area.getX() + t * area.getWidth();
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour(juce::Colour(0xff1a2a1a));
        for (float f : { 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f })
            g.drawVerticalLine(juce::roundToInt(freqToX(f, area)), area.getY(), area.getBottom());
    }

    void drawLabels(juce::Graphics& g, juce::Rectangle<float> labelArea, juce::Rectangle<float> barsArea)
    {
        g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
        g.setColour(NFLookAndFeel::kTextDim);

        struct FreqLabel { float freq; const char* text; };
        static const FreqLabel labels[] = { { 31, "31" }, { 63, "63" }, { 125, "125" }, { 250, "250" },
                                             { 500, "500" }, { 1000, "1k" }, { 2000, "2k" }, { 4000, "4k" },
                                             { 8000, "8k" }, { 16000, "16k" } };
        for (auto& l : labels)
        {
            float x = freqToX(l.freq, barsArea);
            g.drawFittedText(l.text, juce::roundToInt(x - 14.0f), (int) labelArea.getY(), 28, (int) labelArea.getHeight(),
                              juce::Justification::centred, 1);
        }
    }

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
