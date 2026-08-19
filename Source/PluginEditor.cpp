#include "PluginEditor.h"

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p)
{
    setupKnob(thresholdSlider, thresholdLabel, "Threshold");
    setupKnob(ratioSlider, ratioLabel, "Ratio");
    setupKnob(attackSlider, attackLabel, "Attack");
    setupKnob(releaseSlider, releaseLabel, "Release");
    setupKnob(driveSlider, driveLabel, "Drive");
    setupKnob(makeupSlider, makeupLabel, "Makeup");
    setupKnob(mixSlider, mixLabel, "Mix");

    addAndMakeVisible(grMeter);

    auto& apvts = audioProcessor.apvts;
    thresholdAttach = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdSlider);
    ratioAttach     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioSlider);
    attackAttach    = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    releaseAttach   = std::make_unique<SliderAttachment>(apvts, "release", releaseSlider);
    driveAttach     = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    makeupAttach    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupSlider);
    mixAttach       = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);

    setSize(600, 260);
}

void VocalCompressorAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label,
                                                       const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void VocalCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2b2b2b));

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawFittedText("Vocal Compressor", getLocalBounds().removeFromTop(30),
                      juce::Justification::centred, 1);
}

void VocalCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30); // espaço do título

    auto meterArea = area.removeFromRight(40);
    grMeter.setBounds(meterArea);
    area.removeFromRight(10);

    int knobWidth = area.getWidth() / 7;

    juce::Slider* sliders[] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                 &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };
    juce::Label* labels[] = { &thresholdLabel, &ratioLabel, &attackLabel,
                               &releaseLabel, &driveLabel, &makeupLabel, &mixLabel };

    for (int i = 0; i < 7; ++i)
    {
        auto col = area.removeFromLeft(knobWidth);
        labels[i]->setBounds(col.removeFromTop(20));
        sliders[i]->setBounds(col.reduced(5));
    }
}
