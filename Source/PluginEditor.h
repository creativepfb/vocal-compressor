#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "UI/NFLookAndFeel.h"
#include "UI/LevelMeter.h"
#include "UI/DbReadout.h"
#include "UI/RTADisplay.h"
#include "UI/BypassIndicator.h"
#include "UI/ValueReadout.h"
#include "UI/EQGraphComponent.h"

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

    juce::ToggleButton hpfButton { "HPF" };
    juce::ToggleButton bypassButton { "ON" };

    // Seletor Pre/Post EQ: SELECT (qual aparece no gráfico, radio group,
    // não é parâmetro de áudio) e ON (liga/desliga o DSP daquele estágio,
    // parâmetro real via attachment). As duas funções são independentes.
    juce::ToggleButton preSelectButton { "PRE EQ" }, postSelectButton { "POST EQ" };
    juce::ToggleButton preOnButton { "ON" }, postOnButton { "ON" };

    RTADisplay rtaDisplay;
    EQGraphComponent eqGraph;
    BypassIndicator bypassIndicator;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> hpfAttach, bypassAttach, preOnAttach, postOnAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
