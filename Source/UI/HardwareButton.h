#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Botão físico estilo hardware de áudio profissional: corpo escuro com
    gradiente, aro roxo/magenta (combinando com o faceplate), bevel,
    sombra interna, highlight superior e "LED"/backlight verde-limão
    aceso quando ON - a MESMA cor exata do pontinho dos knobs
    (NFLookAndFeel::kKnobGreen), pra manter uma identidade única de verde
    no plugin inteiro.

    Dois estados independentes, propositalmente NÃO confundidos:
    - toggle state (juce::Button, via ButtonAttachment) = processamento
      ligado/desligado (ON/OFF) -> controla o brilho verde.
    - "selected for editing" (setSelectedForEditing) = qual EQ está sendo
      mostrado no gráfico agora -> só um aro fino adicional, nunca afeta
      o verde de ON. Usado só pelos botões PRE EQ / POST EQ.

    Também serve pro botão RESET, em modo momentâneo (setMomentary) - sem
    LED verde, só feedback físico de pressionado. */
class HardwareButton : public juce::Button
{
public:
    explicit HardwareButton(const juce::String& name) : juce::Button(name)
    {
        setClickingTogglesState(true);
        setButtonText(name);
    }

    /** Botão momentâneo (ex.: RESET) - nunca acende verde, é só um push físico. */
    void setMomentary(bool shouldBeMomentary) { momentary = shouldBeMomentary; }

    /** Aro extra indicando "é este EQ que está sendo editado agora no
        gráfico" - independente do ON/OFF do processamento. */
    void setSelectedForEditing(bool shouldBeSelected)
    {
        if (selected != shouldBeSelected)
        {
            selected = shouldBeSelected;
            repaint();
        }
    }

    bool isSelectedForEditing() const { return selected; }

protected:
    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto full = getLocalBounds().toFloat();
        if (full.getWidth() < 4.0f || full.getHeight() < 4.0f)
            return;

        bool buttonOn = !momentary && getToggleState();
        bool pressed = shouldDrawButtonAsDown;
        float corner = juce::jmin(full.getWidth(), full.getHeight()) * 0.22f;

        // 1. Sombra externa - o botão "flutua" levemente sobre o faceplate.
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillRoundedRectangle(full.reduced(1.0f).translated(0.0f, pressed ? 0.6f : 1.8f), corner);

        auto body = full.reduced(1.5f).translated(0.0f, pressed ? 0.8f : 0.0f);

        // 2. Aro externo roxo/magenta (moldura física do botão no painel).
        g.setColour(NFLookAndFeel::kPurple.withAlpha(0.9f));
        g.fillRoundedRectangle(body, corner);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawRoundedRectangle(body, corner, 1.0f);

        // 3. Corpo escuro (dentro do aro), gradiente vertical sutil - metal/plástico.
        auto inner = body.reduced(juce::jmax(1.6f, corner * 0.22f));
        float innerCorner = juce::jmax(1.0f, corner * 0.78f);

        juce::Colour top = juce::Colour(0xff2c2c33);
        juce::Colour bottom = juce::Colour(0xff131316);
        if (pressed) { top = top.darker(0.3f); bottom = bottom.darker(0.15f); }

        juce::ColourGradient bodyGrad(top, inner.getCentreX(), inner.getY(),
                                       bottom, inner.getCentreX(), inner.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(inner, innerCorner);

        // 4. LED / backlight verde quando ON - glow por fora + iluminação por dentro.
        if (buttonOn)
        {
            auto glowArea = inner.expanded(juce::jmax(2.0f, corner * 0.3f));
            g.setColour(NFLookAndFeel::kKnobGreen.withAlpha(0.22f));
            g.fillRoundedRectangle(glowArea, innerCorner + 2.0f);

            juce::ColourGradient ledGrad(NFLookAndFeel::kKnobGreen.withAlpha(0.5f), inner.getCentreX(), inner.getY(),
                                          NFLookAndFeel::kKnobGreen.withAlpha(0.05f), inner.getCentreX(), inner.getBottom(), false);
            g.setGradientFill(ledGrad);
            g.fillRoundedRectangle(inner, innerCorner);

            g.setColour(NFLookAndFeel::kKnobGreen.withAlpha(0.9f));
            g.drawRoundedRectangle(inner.reduced(0.6f), innerCorner, 1.1f);
        }

        // 5. Sombra interna (vinheta sutil nas bordas do corpo).
        juce::ColourGradient innerShadow(juce::Colours::transparentBlack, inner.getCentreX(), inner.getCentreY(),
                                          juce::Colours::black.withAlpha(0.5f), inner.getX(), inner.getY(), true);
        innerShadow.addColour(0.82, juce::Colours::transparentBlack);
        g.setGradientFill(innerShadow);
        g.fillRoundedRectangle(inner, innerCorner);

        // 6. Highlight superior (reflexo de plástico/metal premium).
        auto highlightArea = inner.withHeight(inner.getHeight() * 0.4f).reduced(inner.getWidth() * 0.1f, 0.0f);
        g.setColour(juce::Colours::white.withAlpha(pressed ? 0.04f : 0.11f));
        g.fillRoundedRectangle(highlightArea, innerCorner * 0.7f);

        // 7. Hover discreto.
        if (shouldDrawButtonAsHighlighted && !pressed)
        {
            g.setColour(juce::Colours::white.withAlpha(0.04f));
            g.fillRoundedRectangle(inner, innerCorner);
        }

        // 8. Aro de "selecionado pro gráfico" - só PRE/POST EQ usam isso,
        // independente do verde de ON.
        if (selected)
        {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.drawRoundedRectangle(body.reduced(0.5f), corner, 1.4f);
        }

        // 9. Texto.
        auto label = getButtonText();
        if (label.isNotEmpty())
        {
            g.setColour(buttonOn ? juce::Colours::white : NFLookAndFeel::kTextDim);
            g.setFont(juce::Font(juce::FontOptions(juce::jlimit(8.0f, 13.0f, inner.getHeight() * 0.6f), juce::Font::bold)));
            g.drawFittedText(label, inner.getSmallestIntegerContainer(), juce::Justification::centred, 1);
        }
    }

private:
    bool momentary = false;
    bool selected = false;
};
