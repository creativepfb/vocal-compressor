#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"

/** Popup centralizado pra digitar o nome de um preset ao salvar - mesmo
    padrão visual do LicenseActivationComponent (fundo escurecido
    semi-transparente cobrindo a janela, cartão roxo/preto no meio),
    pra parecer nativo da interface em vez de um diálogo do sistema. */
class PresetNameDialog : public juce::Component
{
public:
    PresetNameDialog()
    {
        titleLabel.setText("Save Preset", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(titleLabel);

        nameEditor.setTextToShowWhenEmpty("Preset name", juce::Colours::grey);
        nameEditor.setJustification(juce::Justification::centredLeft);
        nameEditor.setFont(juce::Font(juce::FontOptions(15.0f)));
        nameEditor.onReturnKey = [this] { attemptConfirm(); };
        nameEditor.onEscapeKey = [this] { cancel(); };
        addAndMakeVisible(nameEditor);

        saveButton.setButtonText("Save");
        saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7c3aed));
        saveButton.onClick = [this] { attemptConfirm(); };
        addAndMakeVisible(saveButton);

        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [this] { cancel(); };
        addAndMakeVisible(cancelButton);

        statusLabel.setJustificationType(juce::Justification::centred);
        statusLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
        addAndMakeVisible(statusLabel);

        setInterceptsMouseClicks(true, true);
    }

    /** Mostra o popup já com um nome sugerido selecionado (ex: o preset
        atual, pra facilitar "salvar como variação"). */
    void show(const juce::String& suggestedName)
    {
        nameEditor.setText(suggestedName, juce::dontSendNotification);
        statusLabel.setText({}, juce::dontSendNotification);
        setVisible(true);
        toFront(true);
        nameEditor.grabKeyboardFocus();
        nameEditor.selectAll();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0x70000000));

        auto card = cardBounds();
        g.setColour(juce::Colour(0xf2141416));
        g.fillRoundedRectangle(card.toFloat(), 14.0f);
        g.setColour(juce::Colour(0xff7c3aed));
        g.drawRoundedRectangle(card.toFloat(), 14.0f, 1.5f);
    }

    void resized() override
    {
        auto area = cardBounds().reduced(28, 24);
        titleLabel.setBounds(area.removeFromTop(28));
        area.removeFromTop(16);
        nameEditor.setBounds(area.removeFromTop(34));
        area.removeFromTop(10);
        statusLabel.setBounds(area.removeFromTop(18));
        area.removeFromTop(8);

        auto buttons = area.removeFromTop(36);
        cancelButton.setBounds(buttons.removeFromRight(100));
        buttons.removeFromRight(10);
        saveButton.setBounds(buttons.removeFromRight(100));
    }

    std::function<void(const juce::String&)> onSave;

private:
    juce::Rectangle<int> cardBounds() const
    {
        return getLocalBounds().withSizeKeepingCentre(340, 210);
    }

    void attemptConfirm()
    {
        auto name = nameEditor.getText().trim();
        if (name.isEmpty())
        {
            statusLabel.setText("Digite um nome pro preset.", juce::dontSendNotification);
            return;
        }

        setVisible(false);
        if (onSave)
            onSave(name);
    }

    void cancel() { setVisible(false); }

    juce::Label titleLabel, statusLabel;
    juce::TextEditor nameEditor;
    juce::TextButton saveButton, cancelButton;
};
