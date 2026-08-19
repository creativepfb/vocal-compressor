#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "UI/NFLookAndFeel.h"
#include "UI/LevelMeter.h"

class VocalCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCompressorAudioProcessorEditor(VocalCompressorAudioProcessor&);
    ~VocalCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text);

    VocalCompressorAudioProcessor& audioProcessor;
    NFLookAndFeel nfLookAndFeel;

    juce::Slider thresholdSlider, ratioSlider, attackSlider,
                 releaseSlider, driveSlider, makeupSlider, mixSlider;

    juce::Label thresholdLabel, ratioLabel, attackLabel,
                releaseLabel, driveLabel, makeupLabel, mixLabel;

    juce::ToggleButton hpfButton { "Vocal HPF" };
    juce::Label sidechainCaption { {}, "SIDECHAIN" };

    GainReductionMeter grMeter;
    LevelMeter inputMeter, outputMeter;
    juce::Label inputCaption { {}, "INPUT" }, outputCaption { {}, "OUTPUT" }, grCaption { {}, "GR" };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> hpfAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
