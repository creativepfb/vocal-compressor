#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/VocalCompressorDSP.h"

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

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> currentGainReductionDb { 0.0f };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    VocalCompressorDSP compressorL, compressorR;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam     = nullptr;
    std::atomic<float>* attackParam    = nullptr;
    std::atomic<float>* releaseParam   = nullptr;
    std::atomic<float>* driveParam     = nullptr;
    std::atomic<float>* makeupParam    = nullptr;
    std::atomic<float>* mixParam       = nullptr;
    std::atomic<float>* hpfParam       = nullptr;
    std::atomic<float>* stage2Param    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessor)
};
