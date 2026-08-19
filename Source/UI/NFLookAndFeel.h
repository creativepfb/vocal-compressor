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

        // Trilho de fundo
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff2a2a30));
        g.strokePath(track, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arco de valor (roxo)
        juce::Path valueArc;
        valueArc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(kPurple);
        g.strokePath(valueArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Corpo do knob (metal escuro)
        auto knobRadius = radius * 0.70f;
        juce::ColourGradient grad(juce::Colour(0xff3c3c42), centre.x - knobRadius * 0.4f, centre.y - knobRadius * 0.4f,
                                   juce::Colour(0xff121214), centre.x + knobRadius * 0.6f, centre.y + knobRadius * 0.6f, true);
        g.setGradientFill(grad);
        g.fillEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f);

        g.setColour(juce::Colour(0xff050506));
        g.drawEllipse(centre.x - knobRadius, centre.y - knobRadius, knobRadius * 2.0f, knobRadius * 2.0f, 1.5f);

        // Ponteiro (verde neon)
        juce::Path pointer;
        float pointerLength = knobRadius * 0.82f;
        float pointerThickness = 2.6f;
        pointer.addRoundedRectangle(-pointerThickness * 0.5f, -knobRadius * 0.92f, pointerThickness, pointerLength, 1.3f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
        g.setColour(kGreen);
        g.fillPath(pointer);

        auto tip = centre.getPointOnCircumference(knobRadius * 0.95f, angle);
        g.setColour(kGreen);
        g.fillEllipse(tip.x - 2.5f, tip.y - 2.5f, 5.0f, 5.0f);
    }

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                           bool /*highlighted*/, bool /*down*/) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        bool isOn = button.getToggleState();
        auto cornerSize = bounds.getHeight() * 0.5f;

        g.setColour(juce::Colour(0xff1c1c22));
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(juce::Colour(0xff33333a));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);

        auto half = bounds.reduced(2.0f);
        auto offRect = half.removeFromLeft(half.getWidth() * 0.5f);
        auto onRect = half;

        if (isOn)
        {
            g.setColour(kGreen.withAlpha(0.85f));
            g.fillRoundedRectangle(onRect, cornerSize - 2.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xff3a3a42));
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
};

inline const juce::Colour NFLookAndFeel::kBackground { 0xff0d0d10 };
inline const juce::Colour NFLookAndFeel::kPanel      { 0xff17171a };
inline const juce::Colour NFLookAndFeel::kPurple     { 0xffa855f7 };
inline const juce::Colour NFLookAndFeel::kGreen      { 0xff39ff6a };
inline const juce::Colour NFLookAndFeel::kTextLight  { 0xffe8e8ec };
inline const juce::Colour NFLookAndFeel::kTextDim    { 0xff8a8a92 };
