#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "UI/NFLookAndFeel.h"
#include "UI/LevelMeter.h"
#include "UI/DbReadout.h"
#include "UI/RTADisplay.h"
#include "UI/ValueReadout.h"
#include "UI/HardwareButton.h"
#include "UI/EQGraphComponent.h"
#include "License/LicenseActivationComponent.h"

/** Segura a faceplate + todos os controles, sempre no tamanho nativo da
    imagem (1525x920). A janela em si pode ser redimensionada livremente -
    quem faz a escala visual é o AffineTransform aplicado nessa área, não o
    layout interno (assim não precisa recalcular nada ao arrastar o canto). */
class FaceplateArea : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        if (faceplate.isValid())
            g.drawImage(faceplate, getLocalBounds().toFloat());
    }

    juce::Image faceplate;
};

class VocalCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCompressorAudioProcessorEditor(VocalCompressorAudioProcessor&);
    ~VocalCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numKnobs = 7;
    static constexpr int nativeW = 1525;
    static constexpr int nativeH = 920;

    void setupKnob(int index, juce::Slider& slider);
    void updateValueLabel(int index);

    VocalCompressorAudioProcessor& audioProcessor;
    NFLookAndFeel nfLookAndFeel;
    juce::ComponentBoundsConstrainer constrainer;

    FaceplateArea content;

    juce::Slider thresholdSlider, ratioSlider, attackSlider,
                 releaseSlider, driveSlider, makeupSlider, mixSlider;
    ValueReadout valueLabels[numKnobs];

    GainReductionMeter grMeter;
    LevelMeter inputMeter, outputMeter;
    DbReadout inputReadout, outputReadout;

    // Botão físico compacto (corpo sempre preto/grafite, LED circular à
    // esquerda que acende quando ON) - ver UI/HardwareButton.h. Único
    // dentro da caixa FILTER agora: PRE EQ - liga/desliga o EQ inteiro
    // (parâmetro real, via attachment). O HPF do detector do compressor
    // foi removido da UI (redundante - o Pre EQ já tem seu próprio Low Cut).
    HardwareButton preOnButton { "PRE EQ" };

    // BYPASS: no faceplate-5.png essa região do topo ficou lisa (sem caixa
    // preta nem texto impresso) - agora é SÓ este botão, com "BYPASS"
    // desenhado dentro dele mesmo (texto pulsando branco<->vermelho quando
    // ON, ver HardwareButton::setPulseTextWhenOn).
    HardwareButton bypassButton { "BYPASS" };

    RTADisplay rtaDisplay;
    EQGraphComponent eqGraph;

    // NF License System - overlay que cobre a GUI toda enquanto o
    // produto não estiver ativado (o áudio já fica mudo no processor
    // independente disso; aqui é só a interface de ativação).
    LicenseActivationComponent licenseOverlay;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> bypassAttach, preOnAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
