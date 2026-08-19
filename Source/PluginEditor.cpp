#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    // Retângulo em frações (0..1) da imagem de fundo, convertido pra pixels
    // reais conforme o tamanho atual do editor.
    juce::Rectangle<int> fracRect(int w, int h, float x0, float y0, float x1, float y1)
    {
        return { juce::roundToInt(x0 * (float) w), juce::roundToInt(y0 * (float) h),
                 juce::roundToInt((x1 - x0) * (float) w), juce::roundToInt((y1 - y0) * (float) h) };
    }
}

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p),
      inputMeter(p.currentInputDb), outputMeter(p.currentOutputDb)
{
    faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplate1200px_png, BinaryData::faceplate1200px_pngSize);
    nfLookAndFeel.knobImage = juce::ImageCache::getFromMemory(BinaryData::knobpequenopng_png, BinaryData::knobpequenopng_pngSize);

    setLookAndFeel(&nfLookAndFeel);

    for (auto* s : { &thresholdSlider, &ratioSlider, &attackSlider, &releaseSlider,
                      &driveSlider, &makeupSlider, &mixSlider })
        setupKnob(*s);

    addAndMakeVisible(hpfButton);
    addAndMakeVisible(grMeter);
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    for (auto* scale : { &inputScale, &outputScale, &grScale })
        addAndMakeVisible(*scale);

    auto& apvts = audioProcessor.apvts;
    thresholdAttach = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdSlider);
    ratioAttach     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioSlider);
    attackAttach    = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    releaseAttach   = std::make_unique<SliderAttachment>(apvts, "release", releaseSlider);
    driveAttach     = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    makeupAttach    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupSlider);
    mixAttach       = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    hpfAttach       = std::make_unique<ButtonAttachment>(apvts, "hpf", hpfButton);

    setSize(1200, 631);
}

VocalCompressorAudioProcessorEditor::~VocalCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VocalCompressorAudioProcessorEditor::setupKnob(juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 36);
    slider.setColour(juce::Slider::textBoxTextColourId, NFLookAndFeel::kGreen);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(slider);
}

void VocalCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    if (faceplate.isValid())
        g.drawImage(faceplate, getLocalBounds().toFloat());
}

void VocalCompressorAudioProcessorEditor::resized()
{
    const int w = getWidth();
    const int h = getHeight();

    // --- Knobs: 7 colunas igualmente espaçadas sobre a imagem ---
    constexpr float knobCentresX[7] = { 0.1852f, 0.2884f, 0.3915f, 0.4946f, 0.5978f, 0.7009f, 0.8041f };
    constexpr float knobColHalfWidth = 0.052f;
    constexpr float knobTop = 0.335f;
    constexpr float knobBottom = 0.628f;

    juce::Slider* sliders[7] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                  &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };

    for (int i = 0; i < 7; ++i)
        sliders[i]->setBounds(fracRect(w, h, knobCentresX[i] - knobColHalfWidth, knobTop,
                                        knobCentresX[i] + knobColHalfWidth, knobBottom));

    // --- Coluna INPUT (esquerda) ---
    inputScale.setBounds(fracRect(w, h, 0.0350f, 0.2367f, 0.0507f, 0.5520f));
    inputMeter.setBounds(fracRect(w, h, 0.0531f, 0.2367f, 0.0857f, 0.5520f));

    // --- Coluna OUTPUT (direita) ---
    outputScale.setBounds(fracRect(w, h, 0.8934f, 0.2367f, 0.9071f, 0.5520f));
    outputMeter.setBounds(fracRect(w, h, 0.9096f, 0.2367f, 0.9421f, 0.5520f));

    // --- GR (abaixo do OUTPUT) ---
    grMeter.setBounds(fracRect(w, h, 0.9096f, 0.6836f, 0.9421f, 0.8834f));
    grScale.setBounds(fracRect(w, h, 0.9457f, 0.6836f, 0.9650f, 0.8834f));

    // --- HPF: em cima da área "SIDECHAIN OFF/HPF" da imagem ---
    hpfButton.setBounds(fracRect(w, h, 0.0362f, 0.8083f, 0.1291f, 0.8522f));
}
