#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/VocalCompressorDSP.h"
#include "DSP/VoiceEQ.h"
#include "DSP/SpectrumAnalyzer.h"

class VocalCompressorAudioProcessor : public juce::AudioProcessor
{
public:
    VocalCompressorAudioProcessor();
    ~VocalCompressorAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Vocal Compressor"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    // Liga o bypass nativo do host (botão de bypass da DAW) ao nosso
    // parâmetro "bypass", assim os dois ficam sempre sincronizados.
    juce::AudioProcessorParameter* getBypassParameter() const override
    {
        return apvts.getParameter("bypass");
    }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentGainReductionDb { 0.0f };
    std::atomic<float> currentInputDb { -60.0f };
    std::atomic<float> currentOutputDb { -60.0f };

    // Buffer circular pro RTA/waveform da GUI: um ponto (0..1) por bloco de
    // áudio processado. Escrita só no audio thread, leitura só na GUI - sem
    // lock, só um índice atômico de escrita (dado "quase certo" já serve
    // pra visualização, não precisa ser sample-accurate).
    static constexpr int waveformBufferSize = 300;
    std::array<std::atomic<float>, waveformBufferSize> waveformBuffer;
    std::atomic<int> waveformWriteIndex { 0 };

    // Espectro do sinal de ENTRADA (antes do EQ) pro RTA de fundo na GUI.
    SpectrumAnalyzer spectrumAnalyzer;
    double getCurrentSampleRateForGui() const { return currentSampleRate; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    VoiceEQ eqL, eqR;
    VocalCompressorDSP compressorL, compressorR;
    double currentSampleRate = 44100.0;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* driveParam     = nullptr;
    std::atomic<float>* makeupParam    = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* hpfParam       = nullptr;
    std::atomic<float>* bypassParam    = nullptr;

    std::atomic<float>* lowCutOnParam     = nullptr;
    std::atomic<float>* lowCutFreqParam   = nullptr;
    std::atomic<float>* lowShelfFreqParam = nullptr;
    std::atomic<float>* lowShelfGainParam = nullptr;
    std::atomic<float>* peakFreqParam     = nullptr;
    std::atomic<float>* peakGainParam     = nullptr;
    std::atomic<float>* peakQParam        = nullptr;
    std::atomic<float>* highShelfFreqParam = nullptr;
    std::atomic<float>* highShelfGainParam = nullptr;

    std::atomic<float>* highCutOnParam   = nullptr;
    std::atomic<float>* highCutFreqParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessor)
};
