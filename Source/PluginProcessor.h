#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/VocalCompressorDSP.h"
#include "DSP/VoiceEQ.h"
#include "DSP/SpectrumAnalyzer.h"

/** Ponteiros pros parâmetros de UM estágio de EQ completo (Pre ou Post) -
    evita duplicar 12 campos nomeados x2 no processor. */
struct EqStageParams
{
    std::atomic<float>* enabled       = nullptr;
    std::atomic<float>* lowCutOn      = nullptr;
    std::atomic<float>* lowCutFreq    = nullptr;
    std::atomic<float>* lowShelfOn    = nullptr;
    std::atomic<float>* lowShelfFreq  = nullptr;
    std::atomic<float>* lowShelfGain  = nullptr;
    std::atomic<float>* peakOn        = nullptr;
    std::atomic<float>* peakFreq      = nullptr;
    std::atomic<float>* peakGain      = nullptr;
    std::atomic<float>* peakQ         = nullptr;
    std::atomic<float>* highShelfOn   = nullptr;
    std::atomic<float>* highShelfFreq = nullptr;
    std::atomic<float>* highShelfGain = nullptr;
    std::atomic<float>* highCutOn     = nullptr;
    std::atomic<float>* highCutFreq   = nullptr;

    void resolve(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
    {
        enabled       = apvts.getRawParameterValue(prefix + "Enabled");
        lowCutOn      = apvts.getRawParameterValue(prefix + "LowCutOn");
        lowCutFreq    = apvts.getRawParameterValue(prefix + "LowCutFreq");
        lowShelfOn    = apvts.getRawParameterValue(prefix + "LowShelfOn");
        lowShelfFreq  = apvts.getRawParameterValue(prefix + "LowShelfFreq");
        lowShelfGain  = apvts.getRawParameterValue(prefix + "LowShelfGain");
        peakOn        = apvts.getRawParameterValue(prefix + "PeakOn");
        peakFreq      = apvts.getRawParameterValue(prefix + "PeakFreq");
        peakGain      = apvts.getRawParameterValue(prefix + "PeakGain");
        peakQ         = apvts.getRawParameterValue(prefix + "PeakQ");
        highShelfOn   = apvts.getRawParameterValue(prefix + "HighShelfOn");
        highShelfFreq = apvts.getRawParameterValue(prefix + "HighShelfFreq");
        highShelfGain = apvts.getRawParameterValue(prefix + "HighShelfGain");
        highCutOn     = apvts.getRawParameterValue(prefix + "HighCutOn");
        highCutFreq   = apvts.getRawParameterValue(prefix + "HighCutFreq");
    }

    void applyTo(VoiceEQ& eq) const
    {
        eq.setParameters(lowCutOn->load() > 0.5f, lowCutFreq->load(),
                          lowShelfOn->load() > 0.5f, lowShelfFreq->load(), lowShelfGain->load(),
                          peakOn->load() > 0.5f, peakFreq->load(), peakGain->load(), peakQ->load(),
                          highShelfOn->load() > 0.5f, highShelfFreq->load(), highShelfGain->load(),
                          highCutOn->load() > 0.5f, highCutFreq->load());
    }
};

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

    // Espectro do sinal FINAL (pós Pre EQ + compressor + Post EQ, imediatamente
    // antes do output) pro RTA de fundo na GUI.
    SpectrumAnalyzer spectrumAnalyzer;
    double getCurrentSampleRateForGui() const { return currentSampleRate; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    VoiceEQ preEqL, preEqR, postEqL, postEqR;
    EqStageParams preEqParams, postEqParams;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessor)
};
