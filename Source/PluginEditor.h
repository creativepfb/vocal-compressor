#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GainReductionMeter.h"
#include "UI/NFLookAndFeel.h"
#include "UI/LevelMeter.h"
#include "UI/MeterScale.h"

class VocalCompressorAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalCompressorAudioProcessorEditor(VocalCompressorAudioProcessor&);
    ~VocalCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int kInputColWidth = 58;
    static constexpr int kRightColWidth = 112;

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
    MeterScale inputScale { -60.0f, { 0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f } };
    MeterScale outputScale { -60.0f, { 0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f } };
    MeterScale grScale { -GainReductionMeter::maxRangeDb, { 0.0f, -6.0f, -12.0f, -18.0f, -24.0f } };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttach, ratioAttach, attackAttach,
                                       releaseAttach, driveAttach, makeupAttach, mixAttach;
    std::unique_ptr<ButtonAttachment> hpfAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocalCompressorAudioProcessorEditor)
};
