#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    // Retângulo em frações (0..1) da imagem de fundo, convertido pra pixels
    // reais conforme o tamanho atual do editor. Frações medidas por pixel
    // direto na imagem faceplate-1200px.png (1200x631).
    juce::Rectangle<int> fracRect(int w, int h, float x0, float y0, float x1, float y1)
    {
        return { juce::roundToInt(x0 * (float) w), juce::roundToInt(y0 * (float) h),
                 juce::roundToInt((x1 - x0) * (float) w), juce::roundToInt((y1 - y0) * (float) h) };
    }

    juce::Rectangle<int> fracRectCentred(int w, int h, float cx, float cy, float width, float height)
    {
        return fracRect(w, h, cx - width * 0.5f, cy - height * 0.5f, cx + width * 0.5f, cy + height * 0.5f);
    }

    constexpr float knobCentresX[7] = { 0.1875f, 0.29125f, 0.397083f, 0.501667f, 0.605f, 0.708333f, 0.811667f };
    constexpr float knobCentreY = 0.408083f;
    // Resolução nativa da imagem do knob (88x88) sobre a faceplate de 1200px
    // de largura - sem encolher, senão o anel/ponteiro perde nitidez e fica
    // pequeno demais perto das marcações da faceplate.
    constexpr float knobDiameter = 88.0f / 1200.0f;

    constexpr float valueBoxTop = 0.573693f;
    constexpr float valueBoxBottom = 0.630744f;
    constexpr float valueBoxWidth = 0.075f;

    constexpr int decimalPlaces[7] = { 1, 1, 1, 0, 2, 1, 0 };
}

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p),
      inputMeter(p.currentInputDb), outputMeter(p.currentOutputDb)
{
    faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplate1200px_png, BinaryData::faceplate1200px_pngSize);
    nfLookAndFeel.knobImage = juce::ImageCache::getFromMemory(BinaryData::knobpequenopng_png, BinaryData::knobpequenopng_pngSize);

    setLookAndFeel(&nfLookAndFeel);

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };
    for (int i = 0; i < numKnobs; ++i)
        setupKnob(i, *sliders[i], decimalPlaces[i]);

    addAndMakeVisible(hpfButton);
    addAndMakeVisible(grMeter);
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    auto& apvts = audioProcessor.apvts;
    thresholdAttach = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdSlider);
    ratioAttach     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioSlider);
    attackAttach    = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    releaseAttach   = std::make_unique<SliderAttachment>(apvts, "release", releaseSlider);
    driveAttach     = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    makeupAttach    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupSlider);
    mixAttach       = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    hpfAttach       = std::make_unique<ButtonAttachment>(apvts, "hpf", hpfButton);

    for (int i = 0; i < numKnobs; ++i)
        updateValueLabel(i);

    setSize(1200, 631);
}

VocalCompressorAudioProcessorEditor::~VocalCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VocalCompressorAudioProcessorEditor::setupKnob(int index, juce::Slider& slider, int decimals)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(slider);

    slider.onValueChange = [this, index] { updateValueLabel(index); };

    auto& label = valueLabels[index];
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, NFLookAndFeel::kGreen);
    label.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);

    juce::ignoreUnused(decimals);
}

void VocalCompressorAudioProcessorEditor::updateValueLabel(int index)
{
    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };
    valueLabels[index].setText(juce::String(sliders[index]->getValue(), decimalPlaces[index]),
                                juce::dontSendNotification);
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

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };

    for (int i = 0; i < numKnobs; ++i)
    {
        sliders[i]->setBounds(fracRectCentred(w, h, knobCentresX[i], knobCentreY, knobDiameter, knobDiameter));
        valueLabels[i].setBounds(fracRectCentred(w, h, knobCentresX[i],
                                                  (valueBoxTop + valueBoxBottom) * 0.5f,
                                                  valueBoxWidth, valueBoxBottom - valueBoxTop));
    }

    // --- Coluna INPUT (esquerda) ---
    inputMeter.setBounds(fracRect(w, h, 0.056667f, 0.237718f, 0.084167f, 0.553091f));

    // --- Coluna OUTPUT (direita) ---
    outputMeter.setBounds(fracRect(w, h, 0.918333f, 0.237718f, 0.946667f, 0.553091f));

    // --- GR (abaixo do OUTPUT) ---
    grMeter.setBounds(fracRect(w, h, 0.918333f, 0.686212f, 0.946667f, 0.885896f));

    // --- HPF: em cima da área "SIDECHAIN OFF/HPF" da imagem ---
    hpfButton.setBounds(fracRect(w, h, 0.038333f, 0.811410f, 0.129167f, 0.849445f));
}
