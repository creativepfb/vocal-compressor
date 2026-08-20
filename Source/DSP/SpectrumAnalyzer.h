#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

/**
    Analisador de espectro (FFT) pro RTA. Escrita só no audio thread
    (pushSamples), leitura só na GUI (magnitudesDb) - sem lock, cada bin é
    um atomic independente.
*/
class SpectrumAnalyzer
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;   // 2048
    static constexpr int numBins = fftSize / 2;

    SpectrumAnalyzer()
        : fft(fftOrder), window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        for (auto& v : magnitudesDb)
            v.store(-100.0f);
    }

    void pushSamples(const float* data, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fifo[(size_t) fifoIndex++] = data[i];
            if (fifoIndex == fftSize)
            {
                fifoIndex = 0;
                computeFFT();
            }
        }
    }

    std::array<std::atomic<float>, numBins> magnitudesDb;

private:
    void computeFFT()
    {
        std::copy(fifo.begin(), fifo.end(), fftData.begin());
        std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        for (int i = 0; i < numBins; ++i)
        {
            float mag = fftData[(size_t) i] / (float) fftSize;
            magnitudesDb[(size_t) i].store(juce::Decibels::gainToDecibels(mag, -100.0f));
        }
    }

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::array<float, fftSize> fifo {};
    std::array<float, (size_t) fftSize * 2> fftData {};
    int fifoIndex = 0;
};
