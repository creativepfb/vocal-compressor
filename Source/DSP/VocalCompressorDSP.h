#pragma once
#include <juce_core/juce_core.h>
#include <cmath>
#include <vector>
#include "RatioLimiter.h"

/**
    Compressor de voz com release adaptativo (program-dependent) e
    saturação com bastante impacto real no caráter/volume percebido.
    Detector pode passar por um high-pass (Filtro/HPF) pra não deixar
    graves/plosivas dispararem a compressão à toa. Uma instância por
    canal de áudio.

    Ratio no extremo máximo (RatioLimiter::isLimiterMode) vira um modo
    Limiter dedicado: hard-knee, threshold funciona como teto (ceiling) e
    o detector usa lookahead pra reagir a transientes antes deles saírem.
*/
class VocalCompressorDSP
{
public:
    void prepare(double sampleRate)
    {
        fs = sampleRate;

        envelope = 0.0f;
        smoothedGainDb = 0.0f;
        transientTracker = 0.0f;
        hpfLowpassState = 0.0f;

        attackCoef       = calcCoef(5.0f);
        fastReleaseCoef  = calcCoef(50.0f);
        slowReleaseCoef  = calcCoef(400.0f);
        transientCoef    = calcCoef(30.0f);
        hpfCoef          = onePoleCoef(120.0f);

        // Lookahead fixo, SEMPRE ativo (mesmo fora do modo Limiter) -
        // isso é o que garante latência constante, que nunca muda girando
        // o Ratio (só muda se a sample rate mudar, o que é normal/
        // esperado). Nos ratios normais o detector continua analisando a
        // mesma amostra que está sendo processada (ver processSample) -
        // o único efeito é um atraso constante de ~lookaheadMs em tudo,
        // sem mudar o caráter do compressor. No modo Limiter, o detector
        // passa a "espiar" a amostra ainda não processada, adiantada em
        // lookaheadSamples, pra reagir ANTES do pico sair.
        lookaheadSamples = juce::jmax(1, (int) std::ceil(lookaheadMs * 0.001 * fs));
        lookaheadBuffer.assign((size_t) lookaheadSamples, 0.0f);
        lookaheadWriteIndex = 0;
        lastDelayedInput = 0.0f;
    }

    void setParameters(float thresholdDbIn, float ratioIn, float attackMs,
                        float releaseCharacterPercent, float driveIn, float makeupDbIn,
                        bool detectorHpfOnIn)
    {
        thresholdDb = thresholdDbIn;
        ratio = juce::jmax(1.0f, ratioIn);
        isLimiterMode = RatioLimiter::isLimiterMode(ratio);
        attackCoef = calcCoef(juce::jmax(0.1f, attackMs));

        // releaseCharacterPercent vem do parâmetro "release" (0% = FAST na
        // UI, 100% = SLOW) - releaseBlend é a variável INTERNA usada em
        // getAdaptiveRelease() como "o quanto pende pro lado rápido"
        // (jmap(finalBlend, slowReleaseCoef, fastReleaseCoef): 0->slow,
        // 1->fast). Por isso é o INVERSO do percentual do usuário: FAST
        // (0%) precisa virar releaseBlend=1 (rápido), SLOW (100%) precisa
        // virar releaseBlend=0 (lento). Antes disso NÃO estava invertido
        // aqui - releaseBlend era só releaseCharacterPercent/100 direto,
        // o que fazia FAST (0%) ficar sempre preso em slowReleaseCoef e
        // SLOW (100%) conseguir alcançar fastReleaseCoef durante
        // transientes - exatamente o comportamento invertido reportado.
        releaseBlend = juce::jlimit(0.0f, 1.0f, 1.0f - (releaseCharacterPercent / 100.0f));
        driveAmount = juce::jmax(1.0f, driveIn);
        makeupGainLinear = dbToLinear(makeupDbIn);
        detectorHpfOn = detectorHpfOnIn;
    }

    float processSample(float input)
    {
        // Escreve a amostra atual e lê a amostra atrasada (delay fixo de
        // lookaheadSamples) - é ESSA que realmente vira o áudio de saída.
        float delayed = lookaheadBuffer[(size_t) lookaheadWriteIndex];
        lookaheadBuffer[(size_t) lookaheadWriteIndex] = input;
        lookaheadWriteIndex = (lookaheadWriteIndex + 1) % lookaheadSamples;
        lastDelayedInput = delayed;

        // Modo normal: detector vê a mesma amostra (delayed) que está
        // sendo processada - comportamento idêntico ao de antes, só que
        // tudo desliza ~lookaheadMs (imperceptível, não muda o caráter).
        // Modo Limiter: detector vê "input" (adiantado em lookaheadSamples
        // em relação a "delayed") - o ganho já começa a cair antes do
        // pico real chegar na saída, evitando estouro no teto.
        float detectorSample = isLimiterMode ? input : delayed;
        float detectorSignal = detectorHpfOn ? highPassDetector(detectorSample) : detectorSample;
        float rectified = std::abs(detectorSignal);

        float releaseCoef = getAdaptiveRelease(rectified);
        float coef = (rectified > envelope) ? attackCoef : releaseCoef;
        envelope += (rectified - envelope) * coef;

        float gainReductionDb = computeGainReduction(envelope, thresholdDb, ratio, kneeWidth, isLimiterMode);
        smoothedGainDb += (gainReductionDb - smoothedGainDb) * smoothingSpeed;

        float compressedOut = delayed * dbToLinear(smoothedGainDb);

        // Saturação: normalização mais leve que na V1, então o Drive
        // realmente muda o caráter/volume percebido, não só a distorção.
        float driven = std::tanh(compressedOut * driveAmount);
        float satNormalise = juce::jmax(0.35f, std::tanh(driveAmount));
        float output = driven / satNormalise;

        // O teto final do modo Limiter (fixo em -0.5dBFS) NÃO fica aqui -
        // esse estágio só faz Drive+Makeup. O teto de verdade é aplicado
        // no fim de TODA a cadeia (depois do Post EQ), em
        // PluginProcessor::processBlock, porque precisa valer pro sinal
        // realmente final que sai pro output, não só pra saída do
        // compressor isolado.
        return output * makeupGainLinear;
    }

    // Usado pelo medidor de GR na GUI
    float getLastGainReductionDb() const { return smoothedGainDb; }

    /** Amostra "seca" (sem compressão) mas já com o mesmo atraso de
        lookahead aplicado ao sinal processado - usar essa (em vez do
        preEQ cru) pra fazer o blend de Mix, senão dry e wet ficam fora de
        alinhamento no tempo (o lookahead atrasa só o wet). */
    float getLastDelayedInput() const { return lastDelayedInput; }

    /** Latência fixa introduzida pelo lookahead, em samples - constante,
        não muda girando o Ratio (só se a sample rate mudar). Reportar ao
        host via AudioProcessor::setLatencySamples() no prepareToPlay. */
    int getLookaheadSamples() const { return lookaheadSamples; }

    bool isInLimiterMode() const { return isLimiterMode; }

private:
    float calcCoef(float timeMs) const
    {
        return 1.0f - std::exp(-1.0f / (0.001f * timeMs * (float) fs));
    }

    float onePoleCoef(float freqHz) const
    {
        return 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * freqHz / (float) fs);
    }

    float highPassDetector(float x)
    {
        hpfLowpassState += (x - hpfLowpassState) * hpfCoef;
        return x - hpfLowpassState;
    }

    float getAdaptiveRelease(float rectified)
    {
        transientTracker += (rectified - transientTracker) * transientCoef;
        float delta = std::abs(rectified - transientTracker);

        // Blend controlado pelo parâmetro "release character" do usuário
        float autoBlend = juce::jlimit(0.0f, 1.0f, delta * 10.0f);
        float finalBlend = juce::jlimit(0.0f, 1.0f, autoBlend * releaseBlend + (1.0f - releaseBlend) * 0.0f);

        return juce::jmap(finalBlend, slowReleaseCoef, fastReleaseCoef);
    }

    static float computeGainReduction(float env, float thresholdDbIn, float ratioIn,
                                       float kneeWidthIn, bool limiterModeIn)
    {
        float envDb = linearToDb(env + 1.0e-6f);

        if (limiterModeIn)
        {
            // Hard-knee: abaixo do threshold, nada; acima, corta exatamente
            // no teto - sem o vazamento residual que um ratio finito (por
            // maior que seja) sempre deixa passar.
            return envDb <= thresholdDbIn ? 0.0f : (thresholdDbIn - envDb);
        }

        float overshoot = envDb - thresholdDbIn;

        if (overshoot <= -kneeWidthIn * 0.5f)
            return 0.0f;

        if (overshoot > kneeWidthIn * 0.5f)
            return -overshoot * (1.0f - 1.0f / ratioIn);

        float kneeOvershoot = overshoot + kneeWidthIn * 0.5f;
        return -(kneeOvershoot * kneeOvershoot) / (2.0f * kneeWidthIn) * (1.0f - 1.0f / ratioIn);
    }

    static float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
    static float linearToDb(float lin) { return 20.0f * std::log10(lin); }

    double fs = 44100.0;

    float envelope = 0.0f;
    float smoothedGainDb = 0.0f;
    float transientTracker = 0.0f;
    float attackCoef = 0.0f, fastReleaseCoef = 0.0f, slowReleaseCoef = 0.0f, transientCoef = 0.0f;

    float thresholdDb = -18.0f;
    float ratio = 4.0f;
    float kneeWidth = 6.0f;
    float driveAmount = 1.5f;
    float makeupGainLinear = 1.0f;
    float releaseBlend = 0.5f;
    bool isLimiterMode = false;

    // Detector high-pass (Filtro/HPF)
    bool detectorHpfOn = false;
    float hpfLowpassState = 0.0f;
    float hpfCoef = 0.0f;

    // Suavização final do ganho antes de aplicar (evita zipper noise).
    static constexpr float smoothingSpeed = 0.35f;

    // Lookahead fixo (ver comentário em prepare()/processSample()).
    static constexpr float lookaheadMs = 2.0f;
    std::vector<float> lookaheadBuffer;
    int lookaheadSamples = 1;
    int lookaheadWriteIndex = 0;
    float lastDelayedInput = 0.0f;
};
