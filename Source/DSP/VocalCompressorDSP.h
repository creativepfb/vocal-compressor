#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

/**
    Compressor de voz com release adaptativo (program-dependent) e
    saturação com bastante impacto real no caráter/volume percebido.
    Detector pode passar por um high-pass (modo VOCAL) pra não deixar
    graves/plosivas dispararem a compressão à toa. Uma instância por
    canal de áudio.
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
    }

    void setParameters(float thresholdDbIn, float ratioIn, float attackMs,
                        float releaseCharacterPercent, float driveIn, float makeupDbIn,
                        bool detectorHpfOnIn)
    {
        thresholdDb = thresholdDbIn;
        ratio = juce::jmax(1.0f, ratioIn);
        attackCoef = calcCoef(juce::jmax(0.1f, attackMs));
        releaseBlend = juce::jlimit(0.0f, 1.0f, releaseCharacterPercent / 100.0f);
        driveAmount = juce::jmax(1.0f, driveIn);
        makeupGainLinear = dbToLinear(makeupDbIn);
        detectorHpfOn = detectorHpfOnIn;
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

        float compressedOut = input * dbToLinear(smoothedGainDb);

        // Saturação: normalização mais leve que na V1, então o Drive
        // realmente muda o caráter/volume percebido, não só a distorção.
        float driven = std::tanh(compressedOut * driveAmount);
        float satNormalise = juce::jmax(0.35f, std::tanh(driveAmount));
        float output = driven / satNormalise;

        return output * makeupGainLinear;
    }

    // Usado pelo medidor de GR na GUI
    float getLastGainReductionDb() const { return smoothedGainDb; }

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

    // Suavização final do ganho antes de aplicar (evita zipper noise).
    static constexpr float smoothingSpeed = 0.35f;
};
