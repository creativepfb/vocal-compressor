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
    bypassParam    = apvts.getRawParameterValue("bypass");

    lowCutOnParam      = apvts.getRawParameterValue("lowCutOn");
    lowCutFreqParam    = apvts.getRawParameterValue("lowCutFreq");
    lowShelfFreqParam  = apvts.getRawParameterValue("lowShelfFreq");
    lowShelfGainParam  = apvts.getRawParameterValue("lowShelfGain");
    peakFreqParam      = apvts.getRawParameterValue("peakFreq");
    peakGainParam      = apvts.getRawParameterValue("peakGain");
    peakQParam         = apvts.getRawParameterValue("peakQ");
    highShelfFreqParam = apvts.getRawParameterValue("highShelfFreq");
    highShelfGainParam = apvts.getRawParameterValue("highShelfGain");
    highCutOnParam     = apvts.getRawParameterValue("highCutOn");
    highCutFreqParam   = apvts.getRawParameterValue("highCutFreq");

    for (auto& v : waveformBuffer)
        v.store(0.0f);
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
        "hpf", "Detector HPF (Filter)", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "bypass", "Bypass", false));

    // --- EQ paramétrico (antes do compressor) ---
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "lowCutOn", "Low Cut On", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowCutFreq", "Low Cut Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f), 80.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowShelfFreq", "Low Shelf Freq",
        juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f), 150.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lowShelfGain", "Low Shelf Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "peakFreq", "Peak Freq",
        juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.3f), 1000.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "peakGain", "Peak Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "peakQ", "Peak Q",
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highShelfFreq", "High Shelf Freq",
        juce::NormalisableRange<float>(1000.0f, 18000.0f, 1.0f, 0.3f), 8000.0f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highShelfGain", "High Shelf Gain",
        juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "highCutOn", "High Cut On", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "highCutFreq", "High Cut Freq",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.4f), 12000.0f, "Hz"));

    return { params.begin(), params.end() };
}

void VocalCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    compressorL.prepare(sampleRate);
    compressorR.prepare(sampleRate);

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    eqL.prepare(spec);
    eqR.prepare(spec);
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
    bool bypassed = bypassParam->load() > 0.5f;

    compressorL.setParameters(thresholdParam->load(), ratioParam->load(), attackParam->load(),
                               releaseParam->load(), driveParam->load(), makeupParam->load(), hpfOn);
    compressorR.setParameters(thresholdParam->load(), ratioParam->load(), attackParam->load(),
                               releaseParam->load(), driveParam->load(), makeupParam->load(), hpfOn);

    bool lowCutOn = lowCutOnParam->load() > 0.5f;
    bool highCutOn = highCutOnParam->load() > 0.5f;
    eqL.setParameters(lowCutOn, lowCutFreqParam->load(), lowShelfFreqParam->load(), lowShelfGainParam->load(),
                       peakFreqParam->load(), peakGainParam->load(), peakQParam->load(),
                       highShelfFreqParam->load(), highShelfGainParam->load(), highCutOn, highCutFreqParam->load());
    eqR.setParameters(lowCutOn, lowCutFreqParam->load(), lowShelfFreqParam->load(), lowShelfGainParam->load(),
                       peakFreqParam->load(), peakGainParam->load(), peakQParam->load(),
                       highShelfFreqParam->load(), highShelfGainParam->load(), highCutOn, highCutFreqParam->load());

    float inPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        inPeak = juce::jmax(inPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    currentInputDb.store(juce::Decibels::gainToDecibels(inPeak, -60.0f));

    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    // RTA mostra o espectro do sinal de ENTRADA (antes do EQ) - pra dar pra
    // ver o que está sendo moldado, igual EQ de referência.
    spectrumAnalyzer.pushSamples(left, buffer.getNumSamples());

    float mix = mixParam->load() / 100.0f;

    if (!bypassed)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float dryL = eqL.processSample(left[i]);
            float wetL = compressorL.processSample(dryL);
            left[i] = dryL * (1.0f - mix) + wetL * mix;

            if (right != nullptr)
            {
                float dryR = eqR.processSample(right[i]);
                float wetR = compressorR.processSample(dryR);
                right[i] = dryR * (1.0f - mix) + wetR * mix;
            }
        }

        // Para o medidor de GR na GUI: usa o maior corte entre os canais (valores negativos)
        currentGainReductionDb.store(
            juce::jmin(compressorL.getLastGainReductionDb(), compressorR.getLastGainReductionDb()));
    }
    else
    {
        currentGainReductionDb.store(0.0f);
    }

    float outPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        outPeak = juce::jmax(outPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    currentOutputDb.store(juce::Decibels::gainToDecibels(outPeak, -60.0f));

    // RTA/waveform: um ponto por bloco, normalizado 0..1 na mesma faixa dos
    // medidores (0 a -60 dB).
    float waveformFraction = juce::jlimit(0.0f, 1.0f,
        (juce::Decibels::gainToDecibels(outPeak, -60.0f) + 60.0f) / 60.0f);
    int writeIdx = waveformWriteIndex.load();
    waveformBuffer[(size_t) writeIdx].store(waveformFraction);
    waveformWriteIndex.store((writeIdx + 1) % waveformBufferSize);
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
