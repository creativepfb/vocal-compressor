#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/RatioLimiter.h"

namespace
{
    // Adiciona o conjunto completo de parâmetros de UM estágio de EQ
    // (Pre ou Post), prefixados (ex.: "preLowCutFreq", "postLowCutFreq") -
    // evita escrever os 12 parâmetros à mão duas vezes.
    void addEqStageParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params,
                               const juce::String& prefix, const juce::String& displayPrefix)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "Enabled", displayPrefix + " EQ Enabled", false));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "LowCutOn", displayPrefix + " Low Cut On", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "LowCutFreq", displayPrefix + " Low Cut Freq",
            juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f), 80.0f, "Hz"));

        // Bypass individual de cada banda (duplo clique na bolinha no
        // gráfico) - independente do enable geral do estágio (Enabled) e
        // dos outros bypasses de banda. Low Cut/High Cut já tinham esse
        // conceito (LowCutOn/HighCutOn); Low Shelf/Peak/High Shelf ganham
        // o mesmo aqui, default ligado (igual ao comportamento de sempre).
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "LowShelfOn", displayPrefix + " Low Shelf On", true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "LowShelfFreq", displayPrefix + " Low Shelf Freq",
            juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.4f), 150.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "LowShelfGain", displayPrefix + " Low Shelf Gain",
            juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "LowShelfQ", displayPrefix + " Low Shelf Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "PeakOn", displayPrefix + " Peak On", true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "PeakFreq", displayPrefix + " Peak Freq",
            juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.3f), 1000.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "PeakGain", displayPrefix + " Peak Gain",
            juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "PeakQ", displayPrefix + " Peak Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "HighShelfOn", displayPrefix + " High Shelf On", true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "HighShelfFreq", displayPrefix + " High Shelf Freq",
            juce::NormalisableRange<float>(1000.0f, 18000.0f, 1.0f, 0.3f), 8000.0f, "Hz"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "HighShelfGain", displayPrefix + " High Shelf Gain",
            juce::NormalisableRange<float>(-15.0f, 15.0f, 0.1f), 0.0f, "dB"));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "HighShelfQ", displayPrefix + " High Shelf Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.4f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            prefix + "HighCutOn", displayPrefix + " High Cut On", false));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            prefix + "HighCutFreq", displayPrefix + " High Cut Freq",
            juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.4f), 12000.0f, "Hz"));
    }
}

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

    preEqParams.resolve(apvts, "pre");

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
        juce::NormalisableRange<float>(1.0f, RatioLimiter::ratioMax, 0.1f, 0.5f), 4.0f, ":1"));

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

    // --- EQ parametrico, antes do compressor ---
    addEqStageParameters(params, "pre", "Pre");

    return { params.begin(), params.end() };
}

void VocalCompressorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    compressorL.prepare(sampleRate);
    compressorR.prepare(sampleRate);

    // Latência fixa do lookahead do compressor (sempre a mesma, não muda
    // girando o Ratio - só recalculada aqui se a sample rate mudar).
    setLatencySamples(compressorL.getLookaheadSamples());

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    preEqL.prepare(spec);
    preEqR.prepare(spec);
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

    bool preOn = preEqParams.enabled->load() > 0.5f;
    preEqParams.applyTo(preEqL);
    preEqParams.applyTo(preEqR);

    float inPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        inPeak = juce::jmax(inPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    currentInputDb.store(juce::Decibels::gainToDecibels(inPeak, -60.0f));

    // Clip detection do INPUT - direto do pico real (inPeak), não do valor
    // suavizado que a GUI desenha. Só liga (nunca desliga sozinho aqui).
    // >= (não só >): um sinal normalizado bem no teto (exatamente 0dBFS,
    // sem "passar" tecnicamente) também precisa acender o LED.
    if (inPeak >= 1.0f)
        inputClipped.store(true);

    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

    float mix = mixParam->load() / 100.0f;

    // Modo Limiter (Ratio no extremo, ver DSP/RatioLimiter.h): teto final
    // FIXO em -0.5dBFS, aplicado depois de TUDO - não é o mesmo teto interno
    // do compressor (esse cuida só de Drive/Makeup dentro do próprio estágio
    // de compressão). L e R sempre têm o mesmo ratio (parâmetro único),
    // então checar só compressorL já basta.
    bool limiterCeilingActive = compressorL.isInLimiterMode();
    const float finalCeilingLinear = juce::Decibels::decibelsToGain(-0.5f);

    if (!bypassed)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            // INPUT -> PRE EQ -> COMPRESSOR -> MIX -> TETO FINAL (só no modo Limiter) -> OUTPUT
            // O compressor tem um lookahead fixo interno (ver
            // VocalCompressorDSP) que atrasa o sinal em alguns samples -
            // por isso o blend de Mix usa getLastDelayedInput() (o preL
            // já atrasado do mesmo jeito) em vez do preL "cru", senão
            // dry e wet ficam fora de alinhamento no tempo.
            float preL = preOn ? preEqL.processSample(left[i]) : left[i];
            float wetL = compressorL.processSample(preL);
            float mixedL = compressorL.getLastDelayedInput() * (1.0f - mix) + wetL * mix;
            left[i] = limiterCeilingActive ? juce::jlimit(-finalCeilingLinear, finalCeilingLinear, mixedL) : mixedL;

            if (right != nullptr)
            {
                float preR = preOn ? preEqR.processSample(right[i]) : right[i];
                float wetR = compressorR.processSample(preR);
                float mixedR = compressorR.getLastDelayedInput() * (1.0f - mix) + wetR * mix;
                right[i] = limiterCeilingActive ? juce::jlimit(-finalCeilingLinear, finalCeilingLinear, mixedR) : mixedR;
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

    // RTA analisa o sinal FINAL (pós Pre EQ + compressor), o mesmo que vai
    // pro output - não mais o sinal de entrada bruto.
    spectrumAnalyzer.pushSamples(left, buffer.getNumSamples());

    float outPeak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        outPeak = juce::jmax(outPeak, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    currentOutputDb.store(juce::Decibels::gainToDecibels(outPeak, -60.0f));

    // Clip detection do OUTPUT - mesma lógica do INPUT acima.
    if (outPeak >= 1.0f)
        outputClipped.store(true);

    // RTA/waveform: um ponto por bloco, normalizado 0..1 na mesma faixa dos
    // medidores (0 a -60 dB).
    float waveformFraction = juce::jlimit(0.0f, 1.0f,
        (juce::Decibels::gainToDecibels(outPeak, -60.0f) + 60.0f) / 60.0f);
    int writeIdx = waveformWriteIndex.load();
    waveformBuffer[(size_t) writeIdx].store(waveformFraction);
    waveformWriteIndex.store((writeIdx + 1) % waveformBufferSize);

    // NF License System: sem licença ativada, o plugin continua rodando
    // de verdade por dentro (RTA, medidores, GUI - dá pra ver ele
    // funcionando), só o ÁUDIO final fica mudo. Fica depois de tudo de
    // propósito, não muda nenhum outro comportamento/processamento.
    if (! licenseManager.isActivated())
        buffer.clear();
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
