#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

/**
    Compressor de voz com release adaptativo (program-dependent), saturação
    e um segundo estágio opcional (mais rápido e agressivo, em série) pra
    quem quer um caráter mais "esmagado". Detector pode passar por um
    high-pass (modo VOCAL) pra não deixar graves/plosivas dispararem a
    compressão à toa. Uma instância por canal de áudio.
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

        stage2Envelope      = 0.0f;
        stage2SmoothedGainDb = 0.0f;
        stage2AttackCoef    = calcCoef(2.0f);
        stage2ReleaseCoef   = calcCoef(60.0f);
    }

    void setParameters(float thresholdDbIn, float ratioIn, float attackMs,
                        float releaseCharacterPercent, float driveIn, float makeupDbIn,
                        bool detectorHpfOnIn, float stage2AmountPercent)
    {
        thresholdDb = thresholdDbIn;
        ratio = juce::jmax(1.0f, ratioIn);
        attackCoef = calcCoef(juce::jmax(0.1f, attackMs));
        releaseBlend = juce::jlimit(0.0f, 1.0f, releaseCharacterPercent / 100.0f);
        driveAmount = juce::jmax(1.0f, driveIn);
        makeupGainLinear = dbToLinear(makeupDbIn);
        detectorHpfOn = detectorHpfOnIn;
        stage2Amount = juce::jlimit(0.0f, 1.0f, stage2AmountPercent / 100.0f);
    }

    float processSample(float input)
    {
        // --- Detector: opcionalmente só reage ao que passa do high-pass ---
        float detectorSignal = detectorHpfOn ? highPassDetector(input) : input;
        float rectified = std::abs(detectorSignal);

        float releaseCoef = getAdaptiveRelease(rectified);
        float coef = (rectified > envelope) ? attackCoef : releaseCoef;
        envelope += (rectified - envelope) * coef;

        float gainReductionDb = computeGainReduction(envelope, thresholdDb, ratio, kneeWidth);
        smoothedGainDb += (gainReductionDb - smoothedGainDb) * smoothingSpeed;

        float stage1Out = input * dbToLinear(smoothedGainDb);

        // --- Estágio 2 opcional: mais rápido, ratio maior, knee mais duro ---
        float stage2Out = stage1Out;
        if (stage2Amount > 0.0001f)
        {
            float rect2 = std::abs(stage1Out);
            float coef2 = (rect2 > stage2Envelope) ? stage2AttackCoef : stage2ReleaseCoef;
            stage2Envelope += (rect2 - stage2Envelope) * coef2;

            float gr2Db = computeGainReduction(stage2Envelope, stage2ThresholdDb, stage2Ratio, stage2KneeWidth);
            stage2SmoothedGainDb += (gr2Db - stage2SmoothedGainDb) * smoothingSpeed;

            float compressed2 = stage1Out * dbToLinear(stage2SmoothedGainDb);
            stage2Out = stage1Out + (compressed2 - stage1Out) * stage2Amount;
        }
        else
        {
            stage2SmoothedGainDb += (0.0f - stage2SmoothedGainDb) * smoothingSpeed;
        }

        // Saturação: normalização mais leve que antes, então o Drive
        // realmente muda o caráter/volume percebido, não só a distorção.
        float driven = std::tanh(stage2Out * driveAmount);
        float satNormalise = juce::jmax(0.35f, std::tanh(driveAmount));
        float output = driven / satNormalise;

        return output * makeupGainLinear;
    }

    // Usado pelo medidor de GR na GUI (soma dos dois estágios)
    float getLastGainReductionDb() const { return smoothedGainDb + stage2SmoothedGainDb; }

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

    static float computeGainReduction(float env, float thresholdDbIn, float ratioIn, float kneeWidthIn)
    {
        float envDb = linearToDb(env + 1.0e-6f);
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

    // Estágio 1
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

    // Detector high-pass (modo VOCAL)
    bool detectorHpfOn = false;
    float hpfLowpassState = 0.0f;
    float hpfCoef = 0.0f;

    // Estágio 2 (rápido/agressivo, opcional)
    float stage2Envelope = 0.0f;
    float stage2SmoothedGainDb = 0.0f;
    float stage2AttackCoef = 0.0f;
    float stage2ReleaseCoef = 0.0f;
    float stage2Amount = 0.0f;
    const float stage2ThresholdDb = -10.0f;
    const float stage2Ratio = 10.0f;
    const float stage2KneeWidth = 2.0f;

    // Suavização final do ganho antes de aplicar (evita zipper noise).
    static constexpr float smoothingSpeed = 0.35f;
};
