#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Botão físico compacto estilo hardware de áudio: corpo SEMPRE preto/
    grafite (bevel, gradiente sutil, sombra interna/externa, highlight),
    com um pequeno LED circular no lado esquerdo que acende quando ON - o
    corpo nunca vira verde/vermelho inteiro, só o LED. Cor do LED
    configurável (setLedColour) - verde-limão (mesmo tom dos knobs) por
    padrão, vermelho pro BYPASS.

    Dois estados independentes, propositalmente NÃO confundidos:
    - toggle state (juce::Button, via ButtonAttachment) = processamento
      ligado/desligado (ON/OFF) -> controla o LED.
    - "selected for editing" (setSelectedForEditing) = qual filtro está
      sendo mostrado no gráfico agora -> só um aro fino adicional, nunca
      afeta o LED. Usado pelos botões PRE EQ / POST EQ / HPF.

    Também serve pro botão RESET, em modo momentâneo (setMomentary) e sem
    LED (setShowLed(false)) - só corpo físico com feedback de pressionado. */
class HardwareButton : public juce::Button
{
public:
    explicit HardwareButton(const juce::String& name) : juce::Button(name)
    {
        setClickingTogglesState(true);
        setButtonText(name);
    }

    void setMomentary(bool shouldBeMomentary) { momentary = shouldBeMomentary; }
    void setShowLed(bool shouldShowLed) { showLed = shouldShowLed; }
    void setLedColour(juce::Colour newColour) { ledColour = newColour; }

    /** Aro extra indicando "é este filtro que está sendo editado agora no
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

        // 1. Sombra externa - efeito sutil de "flutuar" sobre o painel;
        // ao pressionar, a sombra encolhe (o botão "afunda").
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(full.reduced(1.0f).translated(0.0f, pressed ? 0.4f : 1.4f), corner);

        // 2. Corpo - desloca ~1px quando pressionado (afundou).
        auto body = full.reduced(1.2f).translated(0.0f, pressed ? 1.0f : 0.0f);

        // 3. Aro roxo/magenta sutil, combinando com o faceplate.
        g.setColour(NFLookAndFeel::kPurple.withAlpha(0.55f));
        g.fillRoundedRectangle(body, corner);

        // 4. Corpo escuro (dentro do aro) - SEMPRE preto/grafite, em
        // qualquer estado ON/OFF. Gradiente vertical sutil.
        auto inner = body.reduced(1.1f);
        float innerCorner = juce::jmax(1.0f, corner * 0.85f);

        juce::Colour top = juce::Colour(0xff2a2a30);
        juce::Colour bottom = juce::Colour(0xff121215);
        if (pressed) { top = top.darker(0.25f); bottom = bottom.darker(0.12f); }

        juce::ColourGradient bodyGrad(top, inner.getCentreX(), inner.getY(),
                                       bottom, inner.getCentreX(), inner.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(inner, innerCorner);

        // 5. Sombra interna sutil (vinheta nas bordas do corpo).
        juce::ColourGradient innerShadow(juce::Colours::transparentBlack, inner.getCentreX(), inner.getCentreY(),
                                          juce::Colours::black.withAlpha(0.4f), inner.getX(), inner.getY(), true);
        innerShadow.addColour(0.85, juce::Colours::transparentBlack);
        g.setGradientFill(innerShadow);
        g.fillRoundedRectangle(inner, innerCorner);

        // 6. Highlight superior discreto (reflexo de plástico/metal).
        auto highlightArea = inner.withHeight(inner.getHeight() * 0.38f).reduced(inner.getWidth() * 0.08f, 0.0f);
        g.setColour(juce::Colours::white.withAlpha(pressed ? 0.03f : 0.09f));
        g.fillRoundedRectangle(highlightArea, innerCorner * 0.7f);

        // 7. Hover bem discreto.
        if (shouldDrawButtonAsHighlighted && !pressed)
        {
            g.setColour(juce::Colours::white.withAlpha(0.03f));
            g.fillRoundedRectangle(inner, innerCorner);
        }

        // 8. LED circular à esquerda - o ÚNICO elemento que acende quando ON.
        float ledDiameter = juce::jmin(inner.getHeight() * 0.4f, 11.0f);
        float ledCentreX = inner.getX() + inner.getWidth() * 0.14f + ledDiameter * 0.5f;
        float ledCentreY = inner.getCentreY();

        if (showLed)
        {
            if (buttonOn)
            {
                // Glow bem discreto ao redor do LED, não no corpo inteiro.
                g.setColour(ledColour.withAlpha(0.25f));
                g.fillEllipse(ledCentreX - ledDiameter, ledCentreY - ledDiameter, ledDiameter * 2.0f, ledDiameter * 2.0f);
            }

            g.setColour(buttonOn ? ledColour : juce::Colour(0xff2b2b2f));
            g.fillEllipse(ledCentreX - ledDiameter * 0.5f, ledCentreY - ledDiameter * 0.5f, ledDiameter, ledDiameter);
            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.drawEllipse(ledCentreX - ledDiameter * 0.5f, ledCentreY - ledDiameter * 0.5f, ledDiameter, ledDiameter, 1.0f);

            if (buttonOn)
            {
                g.setColour(juce::Colours::white.withAlpha(0.55f));
                g.fillEllipse(ledCentreX - ledDiameter * 0.16f, ledCentreY - ledDiameter * 0.3f,
                               ledDiameter * 0.26f, ledDiameter * 0.26f);
            }
        }

        // 9. Aro de "selecionado pro gráfico" - só PRE/POST/HPF usam isso,
        // independente do LED de ON.
        if (selected)
        {
            g.setColour(juce::Colours::white.withAlpha(0.32f));
            g.drawRoundedRectangle(body.reduced(0.5f), corner, 1.1f);
        }

        // 10. Texto (à direita do LED, ou centralizado se não tiver LED).
        auto label = getButtonText();
        if (label.isNotEmpty())
        {
            auto textArea = inner;
            if (showLed)
                textArea.removeFromLeft(inner.getWidth() * 0.14f + ledDiameter + 5.0f);

            g.setColour(buttonOn ? juce::Colours::white : NFLookAndFeel::kTextDim);
            g.setFont(juce::Font(juce::FontOptions(juce::jlimit(9.0f, 13.0f, inner.getHeight() * 0.42f), juce::Font::bold)));
            g.drawFittedText(label, textArea.getSmallestIntegerContainer(), juce::Justification::centred, 1);
        }
    }

private:
    bool momentary = false;
    bool selected = false;
    bool showLed = true;
    juce::Colour ledColour { NFLookAndFeel::kKnobGreen };
};
