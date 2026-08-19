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

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(4.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto knobRadius = radius * 0.66f;

        // Sombra suave por trás do knob
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(centre.x - knobRadius + 1.5f, centre.y - knobRadius + 3.5f, knobRadius * 2.0f, knobRadius * 2.0f);

        // Marcas de escala ao redor do arco
        for (int i = 0; i <= 10; ++i)
        {
            float t = (float) i / 10.0f;
            float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
            auto p1 = centre.getPointOnCircumference(radius + 2.0f, tickAngle);
            auto p2 = centre.getPointOnCircumference(radius + 5.0f, tickAngle);
            g.setColour(juce::Colour(0xff3a3a42));
            g.drawLine({ p1, p2 }, 1.2f);
        }

        // Trilho de fundo do arco
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff26262c));
        g.strokePath(track, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Glow por trás do arco de valor
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(kPurple.withAlpha(0.35f));
        g.strokePath(valueArc, juce::PathStrokeType(6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(kPurple);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Corpo do knob: gradiente metálico (luz vindo de cima-esquerda)
        juce::ColourGradient bodyGrad(juce::Colour(0xff4a4a52), centre.x - knobRadius * 0.6f, centre.y - knobRadius * 0.7f,
                                       juce::Colour(0xff0e0e10), centre.x + knobRadius * 0.7f, centre.y + knobRadius * 0.8f, false);
        bodyGrad.addColour(0.55, juce::Colour(0xff2a2a30));
        g.setGradientFill(bodyGrad);
        g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        // Bezel: anel de brilho fino na borda superior-esquerda
        juce::Path bezel;
        bezel.addArc(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f,
                     juce::MathConstants<float>::pi * 1.05f, juce::MathConstants<float>::pi * 1.95f, true);
        g.setColour(juce::Colour(0xff6a6a74).withAlpha(0.6f));
        g.strokePath(bezel, juce::PathStrokeType(1.2f));

        g.setColour(juce::Colour(0xff020203));
        g.drawEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.3f);

        // Ponteiro (verde neon) com glow
        juce::Path pointer;
        float pointerLength = knobRadius * 0.8f;
        float pointerThickness = 2.4f;
        pointer.addRoundedRectangle(-pointerThickness * 0.5f, -knobRadius * 0.9f, pointerThickness, pointerLength, 1.2f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
        g.setColour(kGreen.withAlpha(0.4f));
        g.strokePath(pointer, juce::PathStrokeType(3.0f));
        g.setColour(kGreen);
        g.fillPath(pointer);

        auto tip = centre.getPointOnCircumference(knobRadius * 0.92f, angle);
        g.setColour(kGreen.withAlpha(0.30f));
        g.fillEllipse(tip.x - 5.0f, tip.y - 5.0f, 10.0f, 10.0f);
        g.setColour(kGreen);
        g.fillEllipse(tip.x - 2.2f, tip.y - 2.2f, 4.4f, 4.4f);
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
