#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalCompressorAudioProcessor::VocalCompressorAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    thresholdParam = apvts.getRawParameterValue("threshold");
    ratioParam     = apvts.getRawParameterValue("ratio");
    attackParam    = apvts.getRawParameterValue("attack");
    releaseParam   = apvts.getRawParameterValue("release");
    driveParam     = apvts.getRawParameterValue("drive");
    makeupParam    = apvts.getRawParameterValue("makeup");
    mixParam       = apvts.getRawParameterValue("mix");
    hpfParam       = apvts.getRawParameterValue("hpf");
    stage2Param    = apvts.getRawParameterValue("stage2");
}

juce::AudioProcessorValueTreeState::ParameterLayout
VocalCompressorAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ratio", "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.5f), 4.0f, ":1"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.3f), 5.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release Character",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Saturation Drive",
        juce::NormalisableRange<float>(1.0f, 8.0f, 0.01f), 1.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "makeup", "Makeup Gain",
        juce::NormalisableRange<float>(-12.0f, 24.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "hpf", "Detector HPF (Vocal Mode)", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "stage2", "Stage 2 (Aggressive)",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 0.0f, "%"));

    return { params.begin(), params.end() };
}

void VocalCompressorAudioProcessor::prepareToPlay(double sampleRate, int)
{
    compressorL.prepare(sampleRate);
    compressorR.prepare(sampleRate);
}

bool VocalCompressorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void VocalCompressorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    bool hpfOn = hpfParam->load() > 0.5f;
    float stage2Amount = stage2Param->load();

    compressorL.setParameters(thresholdParam->load(), ratioParam->load(), attackParam->load(),
                               releaseParam->load(), driveParam->load(), makeupParam->load(),
                               hpfOn, stage2Amount);
    compressorR.setParameters(thresholdParam->load(), ratioParam->load(), attackParam->load(),
                               releaseParam->load(), driveParam->load(), makeupParam->load(),
                               hpfOn, stage2Amount);

    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    float mix = mixParam->load() / 100.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float dryL = left[i];
        float wetL = compressorL.processSample(dryL);
        left[i] = dryL * (1.0f - mix) + wetL * mix;

        if (right != nullptr)
        {
            float dryR = right[i];
            float wetR = compressorR.processSample(dryR);
            right[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

    // Para o medidor de GR na GUI: usa o maior corte entre os canais (valores negativos)
    currentGainReductionDb.store(
        juce::jmin(compressorL.getLastGainReductionDb(), compressorR.getLastGainReductionDb()));
}

juce::AudioProcessorEditor* VocalCompressorAudioProcessor::createEditor()
{
    return new VocalCompressorAudioProcessorEditor(*this);
}

void VocalCompressorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VocalCompressorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// Ponto de entrada exigido pelo JUCE para criar o processador
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalCompressorAudioProcessor();
}
