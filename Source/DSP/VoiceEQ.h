#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    EQ paramétrico de 5 bandas (Low Cut + Low Shelf + Peak/Bell + High Shelf
    + High Cut), processado antes do compressor. Uma instância por canal.
*/
class VoiceEQ
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lowCut.prepare(spec);
        lowShelf.prepare(spec);
        peak.prepare(spec);
        highShelf.prepare(spec);
        highCut.prepare(spec);
    }

    void setParameters(bool lowCutOnIn, float lowCutFreq,
                        float lowShelfFreq, float lowShelfGainDb,
                        float peakFreq, float peakGainDb, float peakQ,
                        float highShelfFreq, float highShelfGainDb,
                        bool highCutOnIn, float highCutFreq)
    {
        lowCutOn = lowCutOnIn;
        highCutOn = highCutOnIn;

        *lowCut.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, juce::jlimit(20.0f, 2000.0f, lowCutFreq));

        *lowShelf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, juce::jlimit(20.0f, 2000.0f, lowShelfFreq), 0.707f,
            juce::Decibels::decibelsToGain(lowShelfGainDb));

        *peak.coefficients = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, juce::jlimit(20.0f, 20000.0f, peakFreq), juce::jmax(0.1f, peakQ),
            juce::Decibels::decibelsToGain(peakGainDb));

        *highShelf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, juce::jlimit(200.0f, 20000.0f, highShelfFreq), 0.707f,
            juce::Decibels::decibelsToGain(highShelfGainDb));

        *highCut.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, juce::jlimit(200.0f, 20000.0f, highCutFreq));
    }

    float processSample(float input)
    {
        float x = input;
        if (lowCutOn)
            x = lowCut.processSample(x);
        x = lowShelf.processSample(x);
        x = peak.processSample(x);
        x = highShelf.processSample(x);
        if (highCutOn)
            x = highCut.processSample(x);
        return x;
    }

private:
    double sampleRate = 44100.0;
    bool lowCutOn = false;
    bool highCutOn = false;

    juce::dsp::IIR::Filter<float> lowCut, lowShelf, peak, highShelf, highCut;
};
