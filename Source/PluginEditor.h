#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"

class VocalCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCompressorAudioProcessorEditor(VocalCompressorAudioProcessor&);
    ~VocalCompressorAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text);

    VocalCompressorAudioProcessor& audioProcessor;

    juce::Slider thresholdSlider, ratioSlider, attackSlider,
                 releaseSlider, driveSlider, makeupSlider, mixSlider, stage2Slider;

    juce::Label thresholdLabel, ratioLabel, attackLabel,
                releaseLabel, driveLabel, makeupLabel, mixLabel, stage2Label;

    juce::ToggleButton hpfButton { "Vocal HPF" };

    GainReductionMeter grMeter;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach,
                                       stage2Attach;
    std::unique_ptr<ButtonAttachment> hpfAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
