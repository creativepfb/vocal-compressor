#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "UI/NFLookAndFeel.h"
#include "UI/LevelMeter.h"
#include "UI/DbReadout.h"
#include "UI/RTADisplay.h"

class VocalCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCompressorAudioProcessorEditor(VocalCompressorAudioProcessor&);
    ~VocalCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numKnobs = 7;
    static constexpr int numEqKnobs = 8;

    void setupKnob(int index, juce::Slider& slider);
    void updateValueLabel(int index);
    void setupEqKnob(int index, juce::Slider& slider, const juce::String& labelText);

    VocalCompressorAudioProcessor& audioProcessor;
    NFLookAndFeel nfLookAndFeel;
    juce::Image faceplate;

    juce::Slider thresholdSlider, ratioSlider, attackSlider,
                 releaseSlider, driveSlider, makeupSlider, mixSlider;
    juce::Label valueLabels[numKnobs];

    GainReductionMeter grMeter;
    LevelMeter inputMeter, outputMeter;
    DbReadout inputReadout, outputReadout;

    // --- Área provisória abaixo da faceplate (será reposicionada quando a
    // imagem nova, com esses elementos já desenhados, chegar) ---
    juce::ToggleButton hpfButton { "HPF" };
    juce::ToggleButton bypassButton { "ON" };
    juce::Label filterCaption { {}, "FILTER" }, bypassCaption { {}, "BYPASS" };
    RTADisplay rtaDisplay;

    // --- EQ paramétrico: 8 knobs + toggle de Low Cut, provisório também ---
    juce::Slider lowCutFreqSlider, lowShelfFreqSlider, lowShelfGainSlider,
                 peakFreqSlider, peakGainSlider, peakQSlider,
                 highShelfFreqSlider, highShelfGainSlider;
    juce::Label eqLabels[numEqKnobs];
    juce::ToggleButton lowCutOnButton { "ON" };
    juce::Label lowCutCaption { {}, "LOW CUT" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> hpfAttach, bypassAttach, lowCutOnAttach;

    std::unique_ptr<SliderAttachment> lowCutFreqAttach, lowShelfFreqAttach, lowShelfGainAttach,
                                       peakFreqAttach, peakGainAttach, peakQAttach,
                                       highShelfFreqAttach, highShelfGainAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
