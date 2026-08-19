#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

/**
    Visual da marca NF: preto/metal escuro com acentos roxo/verde neon.
    Knobs e switches desenhados via Graphics vetorial (sem imagens externas).
*/
class NFLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static const juce::Colour kBackground;
    static const juce::Colour kPanel;
    static const juce::Colour kPurple;
    static const juce::Colour kGreen;
    static const juce::Colour kRed;
    static const juce::Colour kYellow;
    static const juce::Colour kTextLight;
    static const juce::Colour kTextDim;

    NFLookAndFeel()
    {
        setColour(juce::Slider::textBoxTextColourId, kTextLight);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId, kTextDim);
    }

    /** Imagem única do knob (com o próprio anel roxo + indicador verde
        desenhados nela) - giramos ela inteira conforme o valor, técnica
        padrão de skin de plugin. Precisa ser setada antes do primeiro paint. */
    juce::Image knobImage;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        if (!knobImage.isValid())
            return;

        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);

        // A imagem do knob já nasce "em repouso" apontando pra posição mínima
        // (medido: o pontinho verde fica quase exatamente no ângulo padrão
        // rotaryStartAngle do JUCE). Por isso giramos só o quanto falta a
        // partir dali, não o ângulo absoluto.
        auto rotationFromRest = sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Sem encolher: a imagem já foi feita no tamanho certo pra faceplate,
        // então desenhamos ela na resolução nativa (bounds já vem calibrado
        // pra isso no editor).
        float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        float imgSize = (float) knobImage.getWidth();
        float scale = diameter / imgSize;

        auto transform = juce::AffineTransform::rotation(rotationFromRest, imgSize * 0.5f, imgSize * 0.5f)
                              .scaled(scale)
                              .translated(bounds.getCentreX() - diameter * 0.5f,
                                          bounds.getCentreY() - diameter * 0.5f);

        g.drawImageTransformed(knobImage, transform, false);

        // Arquinho de valor (verde, igual o pontinho), crescendo bem rente à
        // borda do knob, no vãozinho antes das marcações da faceplate.
        auto centre = bounds.getCentre();
        auto arcRadius = diameter * 0.5f * 1.11f;
        auto angle = rotaryStartAngle + rotationFromRest;

        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(kGreen.withAlpha(0.35f));
        g.strokePath(valueArc, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(kGreen);
        g.strokePath(valueArc, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                           bool /*highlighted*/, bool /*down*/) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        bool isOn = button.getToggleState();
        auto cornerSize = bounds.getHeight() * 0.5f;

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), cornerSize);

        g.setColour(juce::Colour(0xff1c1c22));
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(juce::Colour(0xff3a3a42));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        auto half = bounds.reduced(2.0f);
        auto offRect = half.removeFromLeft(half.getWidth() * 0.5f);
        auto onRect = half;

        if (isOn)
        {
            g.setColour(kGreen.withAlpha(0.25f));
            g.fillRoundedRectangle(onRect.expanded(1.0f), cornerSize);
            g.setColour(kGreen);
            g.fillRoundedRectangle(onRect, cornerSize - 2.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xff44444c));
            g.fillRoundedRectangle(offRect, cornerSize - 2.0f);
        }

        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.setColour(isOn ? juce::Colour(0xff8a8a92) : kTextLight);
        g.drawFittedText("OFF", offRect.getSmallestIntegerContainer(), juce::Justification::centred, 1);
        g.setColour(isOn ? juce::Colour(0xff0a0a0c) : juce::Colour(0xff6a6a72));
        g.drawFittedText("HPF", onRect.getSmallestIntegerContainer(), juce::Justification::centred, 1);
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return juce::Font(juce::FontOptions(12.0f, juce::Font::bold));
    }

    static void drawMeterBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), 3.0f);
        g.setColour(juce::Colour(0xff08080a));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(juce::Colour(0xff35353c));
        g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
    }

    /** Medidor de nível (Input/Output): verde -> amarelo -> vermelho, padrão de VU, enche de baixo pra cima. */
    static void drawLevelMeterBar(juce::Graphics& g, juce::Rectangle<float> bounds, float fraction)
    {
        fraction = juce::jlimit(0.0f, 1.0f, fraction);
        drawMeterBackground(g, bounds);

        auto area = bounds.reduced(2.5f);
        auto fillRect = area.removeFromBottom(area.getHeight() * fraction);
        if (fillRect.getHeight() <= 0.0f)
            return;

        // Gradiente fixo na altura total do medidor (não só no trecho preenchido),
        // pra cor bater sempre com a posição real em dB, não com o quanto está aceso.
        juce::ColourGradient grad(kGreen, 0.0f, bounds.getBottom(), kRed, 0.0f, bounds.getY(), false);
        grad.addColour(0.7, kYellow);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(fillRect, 2.0f);
    }

    /** Medidor de Gain Reduction: sempre vermelho, enche de cima pra baixo. */
    static void drawReductionMeterBar(juce::Graphics& g, juce::Rectangle<float> bounds, float fraction)
    {
        fraction = juce::jlimit(0.0f, 1.0f, fraction);
        drawMeterBackground(g, bounds);

        auto area = bounds.reduced(2.5f);
        auto fillRect = area.removeFromTop(area.getHeight() * fraction);
        if (fillRect.getHeight() <= 0.0f)
            return;

        g.setColour(kRed);
        g.fillRoundedRectangle(fillRect, 2.0f);
    }
};

inline const juce::Colour NFLookAndFeel::kBackground { 0xff0b0b0d };
inline const juce::Colour NFLookAndFeel::kPanel      { 0xff17171a };
inline const juce::Colour NFLookAndFeel::kPurple     { 0xffa855f7 };
inline const juce::Colour NFLookAndFeel::kGreen      { 0xff39ff6a };
inline const juce::Colour NFLookAndFeel::kRed        { 0xffff3b3b };
inline const juce::Colour NFLookAndFeel::kYellow     { 0xffffd23f };
inline const juce::Colour NFLookAndFeel::kTextLight  { 0xffe8e8ec };
inline const juce::Colour NFLookAndFeel::kTextDim    { 0xff8a8a92 };
