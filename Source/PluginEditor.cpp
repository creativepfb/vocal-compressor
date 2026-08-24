#include "PluginEditor.h"
#include "DSP/RatioLimiter.h"
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
    // O componente do slider precisa ser MAIOR que a imagem do knob (126px),
    // senão o arco (que tem raio maior que o knob, de propósito) fica
    // cortado pelo clipping automático do JUCE nas bordas do componente.
    // A imagem em si continua sendo desenhada em 126x126 fixo (ver
    // NFLookAndFeel::drawRotarySlider) - só a área clicável/de desenho
    // fica maior, dando espaço pro arco existir ao redor sem ser cortado.
    constexpr int knobComponentSizePx = 150;

    constexpr float valueBoxTop = 0.540217f;
    constexpr float valueBoxBottom = 0.591304f;
    constexpr float valueBoxWidth = 0.074098f;

    constexpr int decimalPlaces[7] = { 1, 1, 1, 0, 2, 1, 0 };

    // Tamanho de fonte dos displays abaixo dos KNOBS - configuração
    // própria, independente da fonte dos displays de INPUT/OUTPUT
    // (DbReadout.h tem a dela). Mudar um não afeta o outro.
    constexpr float knobValueFontSize = 32.0f;

    // Posição do slider de Ratio no array `sliders` usado em vários pontos
    // deste arquivo - único knob com o texto/arco especiais de modo Limiter.
    constexpr int ratioKnobIndex = 1;

    // INPUT (esquerda)
    constexpr float inputMeterX0 = 0.062295f, inputMeterX1 = 0.092459f;
    constexpr float meterY0 = 0.236957f, meterY1 = 0.527174f;
    constexpr float inReadoutX0 = 0.036721f, inReadoutX1 = 0.109508f;
    // ALTURA REAL do componente do texto (não é o tamanho da caixa preta
    // impressa na faceplate, que é só ~38px) - propositalmente maior que
    // a fonte de 36pt do DbReadout, porque juce::Label usa drawFittedText
    // por baixo dos panos: se o componente for mais baixo que a fonte
    // pedida, o Label ENCOLHE o texto sozinho pra caber, silenciosamente
    // anulando qualquer setFont() maior (foi exatamente isso que aconteceu
    // antes - só aumentar a fonte não bastava). O texto (sem descendentes,
    // só dígitos/"-"/"."/"inf") fica visualmente dentro da caixa impressa
    // mesmo com o componente um pouco mais alto que ela.
    constexpr float readoutY0 = 0.563587f, readoutY1 = 0.613587f;

    // OUTPUT (direita)
    constexpr float outputMeterX0 = 0.919672f, outputMeterX1 = 0.949508f;
    constexpr float outReadoutX0 = 0.893115f, outReadoutX1 = 0.966557f;

    // GR (embaixo, mesma coluna do OUTPUT)
    constexpr float grMeterX0 = 0.913443f, grMeterX1 = 0.944918f;
    constexpr float grMeterY0 = 0.696739f, grMeterY1 = 0.894565f;

    constexpr float rtaX0 = 0.252459f, rtaX1 = 0.752787f;
    constexpr float rtaY0 = 0.677174f, rtaY1 = 0.905435f;

    // Caixa FILTER (faceplate-6.png) - retângulo pontilhado único embaixo da
    // escrita "FILTER", abriga o botão PRE EQ. Coordenadas medidas pixel a
    // pixel na imagem nativa (1525x920): caixa pontilhada vai de
    // (92,688) até (368,770).
    constexpr float filterX0 = 0.060328f, filterX1 = 0.241311f;
    constexpr float filterY0 = 0.747826f, filterY1 = 0.836957f;

    // Botão físico de BYPASS - no faceplate-5.png essa região do topo
    // ficou completamente lisa (sem caixa preta nem texto impresso), então
    // o botão (com "BYPASS" desenhado dentro dele mesmo) é o único
    // elemento aqui. Posição escolhida na área lisa, longe do parafuso
    // decorativo no canto e da borda do painel OUTPUT.
    constexpr float bypassBtnX0 = 0.773770f, bypassBtnX1 = 0.885246f;
    constexpr float bypassBtnY0 = 0.054348f, bypassBtnY1 = 0.108696f;
}

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p),
      inputMeter(p.currentInputDb), outputMeter(p.currentOutputDb),
      inputReadout(p.currentInputDb), outputReadout(p.currentOutputDb),
      rtaDisplay(p.spectrumAnalyzer, p.getSampleRate() > 0.0 ? p.getSampleRate() : 44100.0),
      eqGraph(p.apvts, p.getSampleRate() > 0.0 ? p.getSampleRate() : 44100.0, p.spectrumAnalyzer),
      licenseOverlay(p.licenseManager)
{
    content.faceplate = juce::ImageCache::getFromMemory(BinaryData::faceplate6_png, BinaryData::faceplate6_pngSize);
    nfLookAndFeel.knobImage = juce::ImageCache::getFromMemory(BinaryData::botaopng126px_png, BinaryData::botaopng126px_pngSize);

    setLookAndFeel(&nfLookAndFeel);

    addAndMakeVisible(content);

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };
    for (int i = 0; i < numKnobs; ++i)
        setupKnob(i, *sliders[i]);

    // Arco do Ratio muda pra violeta fluorescente quando ele entra no
    // modo Limiter (ver NFLookAndFeel::drawRotarySlider) - nenhum outro
    // knob é afetado.
    nfLookAndFeel.setLimiterAwareSlider(&ratioSlider);

    content.addAndMakeVisible(grMeter);
    content.addAndMakeVisible(inputMeter);
    content.addAndMakeVisible(outputMeter);
    content.addAndMakeVisible(inputReadout);
    content.addAndMakeVisible(outputReadout);

    bypassButton.setLedColour(NFLookAndFeel::kRed); // bypass = alerta, não "ligado normal"
    bypassButton.setPulseTextWhenOn(true); // texto pulsa branco<->vermelho devagar quando ON
    content.addAndMakeVisible(bypassButton);

    // PRE EQ: único botão da caixa FILTER agora - o clique liga/desliga o
    // EQ inteiro de verdade (o ButtonAttachment abaixo já cuida disso
    // sozinho, sem precisar de onClick customizado aqui). O aro de
    // "selecionado" fica sempre aceso, já que é o único filtro que existe
    // no gráfico agora (não tem mais o que selecionar entre HPF/Pre).
    preOnButton.setSelectedForEditing(true);
    content.addAndMakeVisible(preOnButton);

    // RTA antigo (barras) não é mais mostrado - o EQGraphComponent agora
    // desenha o próprio espectro preenchido (mesmos dados do
    // SpectrumAnalyzer), por cima. Mantido como child invisível só porque
    // ainda ocupa o SpectrumAnalyzer/sampleRate como dependência - sem
    // custo, não desenha nada.
    content.addChildComponent(rtaDisplay);
    content.addAndMakeVisible(eqGraph); // por cima do RTA, de proposito

    auto& apvts = audioProcessor.apvts;
    thresholdAttach = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdSlider);
    ratioAttach     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioSlider);
    attackAttach    = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    releaseAttach   = std::make_unique<SliderAttachment>(apvts, "release", releaseSlider);
    driveAttach     = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    makeupAttach    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupSlider);
    mixAttach       = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    bypassAttach    = std::make_unique<ButtonAttachment>(apvts, "bypass", bypassButton);
    preOnAttach     = std::make_unique<ButtonAttachment>(apvts, "preEnabled", preOnButton);

    for (int i = 0; i < numKnobs; ++i)
        updateValueLabel(i);

    // NF License System: popup centralizado até ativar, fundo escurecido
    // semi-transparente (plugin continua visível por baixo - o bloqueio de
    // verdade é no áudio). Some sozinho quando a ativação der certo
    // (audioProcessor.licenseManager já embutido dentro do próprio componente).
    addChildComponent(licenseOverlay);
    licenseOverlay.setVisible(! audioProcessor.licenseManager.isActivated());
    licenseOverlay.onActivated = [this] { licenseOverlay.setVisible(false); };

    // Janela redimensionável, proporção travada na da faceplate.
    constrainer.setFixedAspectRatio((double) nativeW / (double) nativeH);
    constrainer.setSizeLimits(nativeW / 3, nativeH / 3, nativeW * 2, nativeH * 2);
    setConstrainer(&constrainer);
    setResizable(true, true);

    // Abre bem menor que o tamanho nativo (usuário reportou que 0.85 abria
    // grande demais dentro da DAW) - o resize continua livre depois, é só
    // o tamanho inicial que muda.
    setSize(juce::roundToInt(nativeW * 0.45f), juce::roundToInt(nativeH * 0.45f));
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

    auto& readout = valueLabels[index];
    readout.setFontHeight(knobValueFontSize);
    readout.setInterceptsMouseClicks(false, false);
    content.addAndMakeVisible(readout);
}

void VocalCompressorAudioProcessorEditor::updateValueLabel(int index)
{
    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider, &makeupSlider, &mixSlider };

    // Ratio no extremo máximo (modo Limiter, ver DSP/RatioLimiter.h) troca
    // o número por "LIMITER" - texto bem mais largo que "20.0", por isso
    // usa uma fonte menor só nesse estado (ValueReadout não faz auto-
    // shrink de propósito, então precisa caber de verdade).
    if (index == ratioKnobIndex && RatioLimiter::isLimiterMode((float) ratioSlider.getValue()))
    {
        valueLabels[index].setFontHeight(16.0f);
        valueLabels[index].setValueText("LIMITER");
        return;
    }

    if (index == ratioKnobIndex)
        valueLabels[index].setFontHeight(knobValueFontSize);

    valueLabels[index].setValueText(juce::String(sliders[index]->getValue(), decimalPlaces[index]));
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
        sliders[i]->setBounds(cx - knobComponentSizePx / 2, cy - knobComponentSizePx / 2,
                               knobComponentSizePx, knobComponentSizePx);

        valueLabels[i].setBounds(fracRectCentred(w, h, knobCentresX[i],
                                                  (valueBoxTop + valueBoxBottom) * 0.5f,
                                                  valueBoxWidth, valueBoxBottom - valueBoxTop));
    }

    DBG("Editor: " << getWidth() << " x " << getHeight());
    DBG("Knob image: " << (nfLookAndFeel.knobImage.isValid() ? nfLookAndFeel.knobImage.getWidth() : -1)
                        << " x " << (nfLookAndFeel.knobImage.isValid() ? nfLookAndFeel.knobImage.getHeight() : -1));
    DBG("Knob rendered: 126 x 126 (componente/area do arco: " << knobComponentSizePx << " x " << knobComponentSizePx << ")");
    DBG("UI scale (content vs janela real): " << ((float) getWidth() / (float) nativeW));

    inputMeter.setBounds(fracRect(w, h, inputMeterX0, meterY0, inputMeterX1, meterY1));
    inputReadout.setBounds(fracRect(w, h, inReadoutX0, readoutY0, inReadoutX1, readoutY1));

    outputMeter.setBounds(fracRect(w, h, outputMeterX0, meterY0, outputMeterX1, meterY1));
    outputReadout.setBounds(fracRect(w, h, outReadoutX0, readoutY0, outReadoutX1, readoutY1));

    grMeter.setBounds(fracRect(w, h, grMeterX0, grMeterY0, grMeterX1, grMeterY1));

    // Botão único (PRE EQ) centralizado dentro do retângulo pontilhado da
    // caixa FILTER.
    {
        auto filterBox = fracRect(w, h, filterX0, filterY0, filterX1, filterY1);
        constexpr int sideMargin = 8;
        constexpr float widthShrink = 0.85f;
        constexpr float heightShrink = 0.75f;

        int slotW = filterBox.getWidth() - sideMargin * 2;
        int slotH = filterBox.getHeight() - sideMargin * 2;

        int buttonWidth = juce::roundToInt((float) slotW * widthShrink);
        int buttonHeight = juce::roundToInt((float) slotH * heightShrink);
        int buttonX = filterBox.getX() + (filterBox.getWidth() - buttonWidth) / 2;
        int buttonY = filterBox.getY() + (filterBox.getHeight() - buttonHeight) / 2;

        preOnButton.setBounds(buttonX, buttonY, buttonWidth, buttonHeight);
    }

    // Botão físico de BYPASS - sozinho na área lisa do topo.
    bypassButton.setBounds(fracRect(w, h, bypassBtnX0, bypassBtnY0, bypassBtnX1, bypassBtnY1));

    rtaDisplay.setBounds(fracRect(w, h, rtaX0, rtaY0, rtaX1, rtaY1));
    eqGraph.setBounds(fracRect(w, h, rtaX0, rtaY0, rtaX1, rtaY1));

    // Popup de licença cobre a JANELA inteira (não a "content" nativa) -
    // fica direto num filho do editor, fora do transform de escala. Ele
    // mesmo só escurece o fundo e centraliza um cartão pequeno - o resto
    // do plugin continua visível por baixo, semi-transparente.
    licenseOverlay.setBounds(getLocalBounds());
}
