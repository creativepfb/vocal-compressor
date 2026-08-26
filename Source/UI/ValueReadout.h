#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Texto de valor desenhado direto (sem juce::Label, sem drawFittedText) -
    sempre no tamanho de fonte configurado, nunca encolhe sozinho pra caber.

    Duplo clique troca o texto por um TextEditor sobreposto (mesmo lugar,
    mesmo tamanho) pra digitar um valor numérico direto - Enter confirma,
    Esc ou perder o foco cancela. Este componente não sabe nada sobre o
    slider/parâmetro que representa: só devolve o número digitado via
    onValueEntered, quem usa decide o que fazer com ele (setValue no
    slider certo). */
class ValueReadout : public juce::Component
{
public:
    ValueReadout()
    {
        editor.setJustification(juce::Justification::centred);
        editor.setSelectAllWhenFocused(true);
        editor.setInputRestrictions(0, "0123456789.,-");
        editor.onReturnKey = [this] { commitEdit(); };
        editor.onFocusLost = [this] { commitEdit(); };
        editor.onEscapeKey = [this] { editor.setVisible(false); };
        addChildComponent(editor);
    }

    void setValueText(const juce::String& newText)
    {
        if (text != newText)
        {
            text = newText;
            repaint();
        }
    }

    void setFontHeight(float newHeight)
    {
        fontHeight = newHeight;
        editor.setFont(juce::Font(juce::FontOptions(fontHeight, juce::Font::bold)));
    }

    void paint(juce::Graphics& g) override
    {
        if (editor.isVisible())
            return;

        g.setColour(NFLookAndFeel::kKnobGreen);
        g.setFont(juce::Font(juce::FontOptions(fontHeight, juce::Font::bold)));
        g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
    }

    void resized() override { editor.setBounds(getLocalBounds()); }

    void mouseDoubleClick(const juce::MouseEvent&) override
    {
        editor.setText(text.trim(), juce::dontSendNotification);
        editor.setVisible(true);
        editor.toFront(true);
        editor.grabKeyboardFocus();
        editor.selectAll();
        repaint();
    }

    /** Disparado quando o usuário confirma (Enter/perde foco) um número
        digitado - já convertido de String pra float, sem unidade. */
    std::function<void(float)> onValueEntered;

private:
    void commitEdit()
    {
        if (! editor.isVisible())
            return;

        auto typed = editor.getText().trim().replaceCharacter(',', '.');
        editor.setVisible(false);
        repaint();

        if (typed.isNotEmpty() && onValueEntered)
            onValueEntered(typed.getFloatValue());
    }

    juce::String text;
    float fontHeight = 34.0f;
    juce::TextEditor editor;
};
