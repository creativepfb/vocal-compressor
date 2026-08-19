#include "PluginEditor.h"

VocalCompressorAudioProcessorEditor::VocalCompressorAudioProcessorEditor(
    VocalCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), grMeter(p),
      inputMeter(p.currentInputDb), outputMeter(p.currentOutputDb)
{
    setLookAndFeel(&nfLookAndFeel);

    setupKnob(thresholdSlider, thresholdLabel, "Threshold");
    setupKnob(ratioSlider, ratioLabel, "Ratio");
    setupKnob(attackSlider, attackLabel, "Attack");
    setupKnob(releaseSlider, releaseLabel, "Release");
    setupKnob(driveSlider, driveLabel, "Drive");
    setupKnob(makeupSlider, makeupLabel, "Makeup");
    setupKnob(mixSlider, mixLabel, "Mix");

    addAndMakeVisible(hpfButton);
    addAndMakeVisible(sidechainCaption);
    sidechainCaption.setJustificationType(juce::Justification::centred);
    sidechainCaption.setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
    sidechainCaption.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));

    addAndMakeVisible(grMeter);
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    for (auto* caption : { &inputCaption, &outputCaption, &grCaption })
    {
        addAndMakeVisible(*caption);
        caption->setJustificationType(juce::Justification::centred);
        caption->setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
        caption->setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    }

    auto& apvts = audioProcessor.apvts;
    thresholdAttach = std::make_unique<SliderAttachment>(apvts, "threshold", thresholdSlider);
    ratioAttach     = std::make_unique<SliderAttachment>(apvts, "ratio", ratioSlider);
    attackAttach    = std::make_unique<SliderAttachment>(apvts, "attack", attackSlider);
    releaseAttach   = std::make_unique<SliderAttachment>(apvts, "release", releaseSlider);
    driveAttach     = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    makeupAttach    = std::make_unique<SliderAttachment>(apvts, "makeup", makeupSlider);
    mixAttach       = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    hpfAttach       = std::make_unique<ButtonAttachment>(apvts, "hpf", hpfButton);

    setSize(760, 320);
}

VocalCompressorAudioProcessorEditor::~VocalCompressorAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void VocalCompressorAudioProcessorEditor::setupKnob(juce::Slider& slider, juce::Label& label,
                                                       const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 18);
    slider.setColour(juce::Slider::textBoxTextColourId, NFLookAndFeel::kTextLight);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
    label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    addAndMakeVisible(label);
}

void VocalCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fundo com leve vinheta radial (mais claro no centro, mais escuro nas bordas)
    juce::ColourGradient bg(juce::Colour(0xff141417), getWidth() * 0.5f, getHeight() * 0.5f,
                             NFLookAndFeel::kBackground, 0.0f, 0.0f, true);
    g.setGradientFill(bg);
    g.fillAll();

    // --- Painéis das seções (dão profundidade, evitam visual "chapado") ---
    auto contentArea = getLocalBounds().withTrimmedTop(72).reduced(12);
    contentArea.removeFromBottom(48 + 6);

    auto drawPanel = [&g](juce::Rectangle<int> r)
    {
        auto rf = r.toFloat();
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRoundedRectangle(rf.translated(0.0f, 1.5f), 8.0f);
        g.setColour(juce::Colour(0xff151518));
        g.fillRoundedRectangle(rf, 8.0f);
        g.setColour(juce::Colour(0xff28282e));
        g.drawRoundedRectangle(rf.reduced(0.5f), 8.0f, 1.0f);
    };

    auto panels = contentArea;
    drawPanel(panels.removeFromLeft(40));
    panels.removeFromLeft(10);
    drawPanel(panels.removeFromRight(90));
    panels.removeFromRight(10);
    drawPanel(panels);

    // --- Barra de topo ---
    auto topBar = getLocalBounds().removeFromTop(36);
    g.setColour(NFLookAndFeel::kPanel);
    g.fillRect(topBar);

    // bolinhas de janela
    auto dotsArea = topBar.reduced(12, 0).removeFromLeft(60);
    juce::Colour dotColours[] { juce::Colour(0xffff5f57), juce::Colour(0xffffbd2e), juce::Colour(0xff28c840) };
    for (int i = 0; i < 3; ++i)
    {
        auto d = juce::Rectangle<float>((float) dotsArea.getX() + i * 18.0f, (float) dotsArea.getCentreY() - 5.0f, 10.0f, 10.0f);
        g.setColour(dotColours[i]);
        g.fillEllipse(d);
    }

    // logo NF + título
    auto textArea = topBar.withTrimmedLeft(90);
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.setColour(NFLookAndFeel::kGreen);
    auto nfWidth = 40;
    g.drawFittedText("NF", textArea.removeFromLeft(nfWidth), juce::Justification::centredLeft, 1);

    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::plain)));
    g.setColour(NFLookAndFeel::kTextLight);
    g.drawFittedText("Vocal Compressor", textArea, juce::Justification::centredLeft, 1);

    // separador
    g.setColour(juce::Colour(0xff2a2a30));
    g.drawLine(0.0f, (float) topBar.getBottom(), (float) getWidth(), (float) topBar.getBottom(), 1.0f);

    // --- Título central + tagline ---
    auto centreArea = getLocalBounds().withTrimmedTop(topBar.getBottom() + 6).removeFromTop(30);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.setColour(NFLookAndFeel::kTextDim);
    g.drawFittedText("SMOOTH  *  CLEAR  *  CONTROL", centreArea, juce::Justification::centred, 1);
}

void VocalCompressorAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(36);  // barra de topo (desenhada no paint)
    area.removeFromTop(36);  // tagline (desenhada no paint)
    area = area.reduced(12);

    // Coluna Input (esquerda)
    auto inputCol = area.removeFromLeft(40);
    inputCaption.setBounds(inputCol.removeFromTop(16));
    inputCol.removeFromBottom(20);
    inputMeter.setBounds(inputCol);
    area.removeFromLeft(10);

    // Coluna Output + GR (direita)
    auto rightCol = area.removeFromRight(90);
    auto outputCol = rightCol.removeFromLeft(40);
    outputCaption.setBounds(outputCol.removeFromTop(16));
    outputCol.removeFromBottom(20);
    outputMeter.setBounds(outputCol);

    rightCol.removeFromLeft(10);
    grCaption.setBounds(rightCol.removeFromTop(16));
    rightCol.removeFromBottom(20);
    grMeter.setBounds(rightCol);

    area.removeFromRight(10);

    // Área central: knobs em cima, HPF embaixo
    auto hpfArea = area.removeFromBottom(48);
    sidechainCaption.setBounds(hpfArea.removeFromTop(14).withSizeKeepingCentre(90, 14));
    hpfButton.setBounds(hpfArea.withSizeKeepingCentre(90, 22));

    constexpr int numKnobs = 7;
    int knobWidth = area.getWidth() / numKnobs;

    juce::Slider* sliders[numKnobs] = { &thresholdSlider, &ratioSlider, &attackSlider,
                                         &releaseSlider, &driveSlider,
                                         &makeupSlider, &mixSlider };
    juce::Label* labels[numKnobs] = { &thresholdLabel, &ratioLabel, &attackLabel,
                                       &releaseLabel, &driveLabel,
                                       &makeupLabel, &mixLabel };

    for (int i = 0; i < numKnobs; ++i)
    {
        auto col = area.removeFromLeft(knobWidth);
        labels[i]->setBounds(col.removeFromTop(20));
        sliders[i]->setBounds(col.reduced(5));
    }
}
