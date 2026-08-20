#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "UI/NFLookAndFeel.h"
#include "UI/LevelMeter.h"
#include "UI/DbReadout.h"

class VocalCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCompressorAudioProcessorEditor(VocalCompressorAudioProcessor&);
    ~VocalCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numKnobs = 7;

    void setupKnob(int index, juce::Slider& slider);
    void updateValueLabel(int index);

    VocalCompressorAudioProcessor& audioProcessor;
    NFLookAndFeel nfLookAndFeel;
    juce::Image faceplate;

    juce::Slider thresholdSlider, ratioSlider, attackSlider,
                 releaseSlider, driveSlider, makeupSlider, mixSlider;
    juce::Label valueLabels[numKnobs];

    GainReductionMeter grMeter;
    LevelMeter inputMeter, outputMeter;
    DbReadout inputReadout, outputReadout;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
