#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
    // Retângulo em frações (0..1) da faceplate, convertido pra pixels reais
    // na resolução nativa passada (a "content" nunca muda de tamanho
    // internamente, só é escalada visualmente pelo transform).
    juce::Rectangle<int> fracRect(int w, int h, float x0, float y0, float x1, float y1)
    {
        return { juce::roundToInt(x0 * (float) w), juce::roundToInt(y0 * (float) h),
                 juce::roundToInt((x1 - x0) * (float) w), juce::roundToInt((y1 - y0) * (float) h) };
    }

    juce::Rectangle<int> fracRectCentred(int w, int h, float cx, float cy, float width, float height)
    {
        return fracRect(w, h, cx - width * 0.5f, cy - height * 0.5f, cx + width * 0.5f, cy + height * 0.5f);
    }

    // Frações medidas por pixel direto na imagem faceplate-3-rta.png (1525x920).
    constexpr float knobCentresX[7] = { 0.192131f, 0.300328f, 0.405246f, 0.502951f,
                                         0.601311f, 0.700984f, 0.802623f };
    constexpr float knobCentreY = 0.382609f;
    // Tamanho ABSOLUTO em pixels (na resolução nativa 1525x920), não fração.
    // BUG que existia antes: usar uma fração calculada sobre a LARGURA
    // (126/1525) como largura E altura do quadrado do knob - como largura
    // (1525) e altura (920) da faceplate são diferentes, a mesma fração
    // aplicada nos dois eixos gerava um retângulo não quadrado (126 de
    // largura x só ~76 de altura), e o código pega o menor lado pra
    // desenhar o círculo - por isso saía menor que devia.
    constexpr int knobSizePx = 126;

    constexpr float valueBoxTop = 0.540217f;
    constexpr float valueBoxBottom = 0.591304f;
    constexpr float valueBoxWidth = 0.074098f;

    constexpr int decimalPlaces[7] = { 1, 1, 1, 0, 2, 1, 0 };

    // INPUT (esquerda)
    constexpr float inputMeterX0 = 0.062295f, inputMeterX1 = 0.092459f;
    constexpr float meterY0 = 0.236957f, meterY1 = 0.527174f;
    constexpr float inReadoutX0 = 0.036721f, inReadoutX1 = 0.109508f;
    constexpr float readoutY0 = 0.570652f, readoutY1 = 0.606522f;

    // OUTPUT (direita)
    constexpr float outputMeterX0 = 0.919672f, outputMeterX1 = 0.949508f;
    constexpr float outReadoutX0 = 0.893115f, outReadoutX1 = 0.966557f;

    // GR (embaixo, mesma coluna do OUTPUT)
    constexpr float grMeterX0 = 0.913443f, grMeterX1 = 0.944918f;
    constexpr float grMeterY0 = 0.696739f, grMeterY1 = 0.894565f;

    // Filtro / Bypass / RTA (linha de baixo)
    constexpr float filterX0 = 0.034754f, filterX1 = 0.141639f;
    constexpr float filterY0 = 0.789130f, filterY1 = 0.833696f;
    constexpr float bypassX0 = 0.163934f, bypassX1 = 0.208525f;
    constexpr float bypassY0 = 0.790217f, bypassY1 = 0.833696f;
    constexpr float rtaX0 = 0.252459f, rtaX1 = 0.752787f;
    constexpr float rtaY0 = 0.677174f, rtaY1 = 0.905435f;

    // Caixa preta do topo (indicador de Bypass)
    constexpr float topBoxX0 = 0.626885f, topBoxX1 = 0.886557f;
    constexpr float topBoxY0 = 0.057609f, topBoxY1 = 0.125f;
}

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p),
      inputMeter(p.currentInputDb), outputMeter(p.currentOutputDb),
      inputReadout(p.currentInputDb), outputReadout(p.currentOutputDb),
      rtaDisplay(p.spectrumAnalyzer, p.getSampleRate() > 0.0 ? p.getSampleRate() : 44100.0),
      bypassIndicator(*p.apvts.getRawParameterValue("bypass"))
{
    content.faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplate3rta_png, BinaryData::faceplate3rta_pngSize);
    nfLookAndFeel.knobImage = juce::ImageCache::getFromMemory(BinaryData::botaopng126px_png, BinaryData::botaopng126px_pngSize);

    setLookAndFeel(&nfLookAndFeel);

    addAndMakeVisible(content);

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };
    for (int i = 0; i < numKnobs; ++i)
        setupKnob(i, *sliders[i]);

    content.addAndMakeVisible(grMeter);
    content.addAndMakeVisible(inputMeter);
    content.addAndMakeVisible(outputMeter);
    content.addAndMakeVisible(inputReadout);
    content.addAndMakeVisible(outputReadout);
    content.addAndMakeVisible(hpfButton);
    content.addAndMakeVisible(bypassButton);
    content.addAndMakeVisible(rtaDisplay);
    content.addAndMakeVisible(bypassIndicator);

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

    // Janela redimensionável, proporção travada na da faceplate.
    constrainer.setFixedAspectRatio((double) nativeW / (double) nativeH);
    constrainer.setSizeLimits(nativeW / 3, nativeH / 3, nativeW * 2, nativeH * 2);
    setConstrainer(&constrainer);
    setResizable(true, true);

    setSize(juce::roundToInt(nativeW * 0.85f), juce::roundToInt(nativeH * 0.85f));
}

VocalCompressorAudioProcessorEditor::~VocalCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VocalCompressorAudioProcessorEditor::setupKnob(int index, juce::Slider& slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    content.addAndMakeVisible(slider);

    slider.onValueChange = [this, index] { updateValueLabel(index); };

    auto& label = valueLabels[index];
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, NFLookAndFeel::kKnobGreen);
    label.setFont(juce::Font(juce::FontOptions(38.0f, juce::Font::bold)));
    label.setMinimumHorizontalScale(0.75f);
    label.setBorderSize(juce::BorderSize<int>(0, 1, 0, 1));
    label.setInterceptsMouseClicks(false, false);
    content.addAndMakeVisible(label);
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
    // Fundo fora da área de conteúdo (só aparece se a proporção não bater
    // por algum motivo - normalmente a "content" cobre tudo).
    g.fillAll(juce::Colours::black);
}

void VocalCompressorAudioProcessorEditor::resized()
{
    float scale = (float) getWidth() / (float) nativeW;
    content.setTransform(juce::AffineTransform::scale(scale));
    content.setBounds(0, 0, nativeW, nativeH);

    const int w = nativeW;
    const int h = nativeH;

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };

    for (int i = 0; i < numKnobs; ++i)
    {
        int cx = juce::roundToInt(knobCentresX[i] * (float) w);
        int cy = juce::roundToInt(knobCentreY * (float) h);
        sliders[i]->setBounds(cx - knobSizePx / 2, cy - knobSizePx / 2, knobSizePx, knobSizePx);

        valueLabels[i].setBounds(fracRectCentred(w, h, knobCentresX[i],
                                                  (valueBoxTop + valueBoxBottom) * 0.5f,
                                                  valueBoxWidth, valueBoxBottom - valueBoxTop));
    }

    DBG("Editor: " << getWidth() << " x " << getHeight());
    DBG("Knob image: " << (nfLookAndFeel.knobImage.isValid() ? nfLookAndFeel.knobImage.getWidth() : -1)
                        << " x " << (nfLookAndFeel.knobImage.isValid() ? nfLookAndFeel.knobImage.getHeight() : -1));
    DBG("Knob rendered: " << knobSizePx << " x " << knobSizePx << " (na area de conteudo nativa 1525x920)");
    DBG("UI scale (content vs janela real): " << ((float) getWidth() / (float) nativeW));

    inputMeter.setBounds(fracRect(w, h, inputMeterX0, meterY0, inputMeterX1, meterY1));
    inputReadout.setBounds(fracRect(w, h, inReadoutX0, readoutY0, inReadoutX1, readoutY1));

    outputMeter.setBounds(fracRect(w, h, outputMeterX0, meterY0, outputMeterX1, meterY1));
    outputReadout.setBounds(fracRect(w, h, outReadoutX0, readoutY0, outReadoutX1, readoutY1));

    grMeter.setBounds(fracRect(w, h, grMeterX0, grMeterY0, grMeterX1, grMeterY1));

    hpfButton.setBounds(fracRect(w, h, filterX0, filterY0, filterX1, filterY1));
    bypassButton.setBounds(fracRect(w, h, bypassX0, bypassY0, bypassX1, bypassY1));
    rtaDisplay.setBounds(fracRect(w, h, rtaX0, rtaY0, rtaX1, rtaY1));

    bypassIndicator.setBounds(fracRect(w, h, topBoxX0, topBoxY0, topBoxX1, topBoxY1));
}
