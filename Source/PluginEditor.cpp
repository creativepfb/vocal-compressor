#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    // Retângulo em frações (0..1) da imagem de fundo, convertido pra pixels
    // reais conforme o tamanho atual do editor. Frações medidas por pixel
    // direto na imagem faceplate-v2.png (1680x643).
    juce::Rectangle<int> fracRect(int w, int h, float x0, float y0, float x1, float y1)
    {
        return { juce::roundToInt(x0 * (float) w), juce::roundToInt(y0 * (float) h),
                 juce::roundToInt((x1 - x0) * (float) w), juce::roundToInt((y1 - y0) * (float) h) };
    }

    juce::Rectangle<int> fracRectCentred(int w, int h, float cx, float cy, float width, float height)
    {
        return fracRect(w, h, cx - width * 0.5f, cy - height * 0.5f, cx + width * 0.5f, cy + height * 0.5f);
    }

    constexpr float knobCentresX[7] = { 0.17887f, 0.27173f, 0.36756f, 0.46339f, 0.55863f, 0.65446f, 0.74821f };
    constexpr float knobCentreY = 0.546656f;
    // Bem maior que o círculo preto (99px) e maior que o raio dos ticks
    // (70px) de propósito - o knob (componente filho) sempre desenha por
    // cima do fundo, não tem problema ele passar por cima das marcações.
    // Limite físico: 156px é o menor espaçamento entre dois centros de knob
    // nessa faceplate (Threshold-Ratio). Uso 154px pra chegar quase no talo
    // sem eles se tocarem.
    constexpr float knobDiameter = 154.0f / 1680.0f;

    constexpr float valueBoxTop = 0.790047f;
    constexpr float valueBoxBottom = 0.874028f;
    constexpr float valueBoxWidth = 0.069048f;

    constexpr int decimalPlaces[7] = { 1, 1, 1, 0, 2, 1, 0 };

    // INPUT (esquerda)
    constexpr float inputMeterX0 = 0.054762f, inputMeterX1 = 0.077976f;
    constexpr float meterY0 = 0.329704f, meterY1 = 0.768274f;
    constexpr float inReadoutCx = 0.064881f, inReadoutCy = 0.872473f;
    constexpr float readoutWidth = 0.071429f, readoutHeight = 0.082271f;

    // GR (meio)
    constexpr float grMeterX0 = 0.831548f, grMeterX1 = 0.853571f;
    constexpr float grMeterY0 = 0.329704f, grMeterY1 = 0.763608f;

    // OUTPUT (direita)
    constexpr float outputMeterX0 = 0.926786f, outputMeterX1 = 0.95f;
    constexpr float outReadoutCx = 0.937f;

    // Tamanho fixo da área da faceplate (a imagem em si, 65% de 1680x643).
    // O resto da janela (abaixo disso) é uma faixa provisória desenhada por
    // código - Filtro, Bypass e RTA - até a imagem nova (que já vai trazer
    // esses elementos) chegar.
    constexpr int faceplateW = 1092;
    constexpr int faceplateH = 418;
    constexpr int extraRowH = 110;
}

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p),
      inputMeter(p.currentInputDb), outputMeter(p.currentOutputDb),
      inputReadout(p.currentInputDb), outputReadout(p.currentOutputDb),
      waveformDisplay(p.waveformBuffer, p.waveformWriteIndex)
{
    faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplatev2_png, BinaryData::faceplatev2_pngSize);
    nfLookAndFeel.knobImage = juce::ImageCache::getFromMemory(BinaryData::knobpequenopng_png, BinaryData::knobpequenopng_pngSize);

    setLookAndFeel(&nfLookAndFeel);

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };
    for (int i = 0; i < numKnobs; ++i)
        setupKnob(i, *sliders[i]);

    addAndMakeVisible(grMeter);
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);
    addAndMakeVisible(inputReadout);
    addAndMakeVisible(outputReadout);

    addAndMakeVisible(hpfButton);
    addAndMakeVisible(bypassButton);
    for (auto* caption : { &filterCaption, &bypassCaption })
    {
        addAndMakeVisible(*caption);
        caption->setJustificationType(juce::Justification::centred);
        caption->setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
        caption->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    }
    addAndMakeVisible(waveformDisplay);

    auto& apvts = audioProcessor.apvts;
    thresholdAttach = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdSlider);
    ratioAttach     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioSlider);
    attackAttach    = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    releaseAttach   = std::make_unique<SliderAttachment>(apvts, "release", releaseSlider);
    driveAttach     = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    makeupAttach    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupSlider);
    mixAttach       = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    hpfAttach       = std::make_unique<ButtonAttachment>(apvts, "hpf", hpfButton);
    bypassAttach    = std::make_unique<ButtonAttachment>(apvts, "bypass", bypassButton);

    for (int i = 0; i < numKnobs; ++i)
        updateValueLabel(i);

    setSize(faceplateW, faceplateH + extraRowH);
}

VocalCompressorAudioProcessorEditor::~VocalCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VocalCompressorAudioProcessorEditor::setupKnob(int index, juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(slider);

    slider.onValueChange = [this, index] { updateValueLabel(index); };

    auto& label = valueLabels[index];
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, NFLookAndFeel::kGreen);
    label.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    label.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(label);
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
        g.drawImage(faceplate, juce::Rectangle<float>(0.0f, 0.0f, (float) faceplateW, (float) faceplateH));

    // Faixa provisória (Filtro / Bypass / RTA) até a faceplate nova chegar.
    auto extraArea = juce::Rectangle<float>(0.0f, (float) faceplateH, (float) faceplateW, (float) extraRowH);
    g.setColour(juce::Colour(0xff1c0f2e));
    g.fillRect(extraArea);
    g.setColour(juce::Colour(0xff3a2a54));
    g.drawLine(0.0f, (float) faceplateH, (float) faceplateW, (float) faceplateH, 1.5f);
}

void VocalCompressorAudioProcessorEditor::resized()
{
    // Área da faceplate: fixa, não depende do tamanho da janela (que só
    // cresce pra baixo, pra caber a faixa provisória).
    const int w = faceplateW;
    const int h = faceplateH;

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };

    for (int i = 0; i < numKnobs; ++i)
    {
        sliders[i]->setBounds(fracRectCentred(w, h, knobCentresX[i], knobCentreY, knobDiameter, knobDiameter));
        valueLabels[i].setBounds(fracRectCentred(w, h, knobCentresX[i],
                                                  (valueBoxTop + valueBoxBottom) * 0.5f,
                                                  valueBoxWidth, valueBoxBottom - valueBoxTop));
    }

    inputMeter.setBounds(fracRect(w, h, inputMeterX0, meterY0, inputMeterX1, meterY1));
    inputReadout.setBounds(fracRectCentred(w, h, inReadoutCx, inReadoutCy, readoutWidth, readoutHeight));

    grMeter.setBounds(fracRect(w, h, grMeterX0, grMeterY0, grMeterX1, grMeterY1));

    outputMeter.setBounds(fracRect(w, h, outputMeterX0, meterY0, outputMeterX1, meterY1));
    outputReadout.setBounds(fracRectCentred(w, h, outReadoutCx, inReadoutCy, readoutWidth, readoutHeight));

    // --- Faixa provisória embaixo: Filtro | Bypass | RTA ---
    auto extra = juce::Rectangle<int>(0, faceplateH, faceplateW, extraRowH).reduced(12);

    auto filterCol = extra.removeFromLeft(90);
    filterCaption.setBounds(filterCol.removeFromTop(16));
    hpfButton.setBounds(filterCol.withSizeKeepingCentre(80, 22));

    extra.removeFromLeft(14);
    auto bypassCol = extra.removeFromLeft(90);
    bypassCaption.setBounds(bypassCol.removeFromTop(16));
    bypassButton.setBounds(bypassCol.withSizeKeepingCentre(80, 22));

    extra.removeFromLeft(14);
    waveformDisplay.setBounds(extra);
}
