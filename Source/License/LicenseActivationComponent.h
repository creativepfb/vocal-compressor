#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLicenseManager.h"

/** Barra de ativação - fica só numa faixa fina no topo da janela, com o
    plugin inteiro visível por baixo (modo "demo": a pessoa vê e mexe na
    interface antes de comprar; o bloqueio de verdade acontece no áudio,
    não na UI). Campo de chave + botão + status, tudo numa linha. Some
    sozinha quando a ativação der certo. */
class LicenseActivationComponent : public juce::Component
{
public:
    explicit LicenseActivationComponent (NFLicenseManager& managerIn) : manager (managerIn)
    {
        titleLabel.setText ("Licenca nao ativada:", juce::dontSendNotification);
        titleLabel.setJustificationType (juce::Justification::centredLeft);
        titleLabel.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (titleLabel);

        keyEditor.setTextToShowWhenEmpty ("NFVC-XXXXX-XXXXX-XXXXX", juce::Colours::grey);
        keyEditor.setJustification (juce::Justification::centred);
        addAndMakeVisible (keyEditor);

        activateButton.setButtonText ("Activate License");
        activateButton.onClick = [this] { attemptActivation(); };
        addAndMakeVisible (activateButton);

        statusLabel.setJustificationType (juce::Justification::centredLeft);
        statusLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
        addAndMakeVisible (statusLabel);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xe0141416));
        g.setColour (juce::Colour (0xff7c3aed));
        g.fillRect (getLocalBounds().removeFromBottom (2));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10, 6);
        titleLabel.setBounds (area.removeFromLeft (140));
        area.removeFromLeft (10);
        activateButton.setBounds (area.removeFromRight (140));
        area.removeFromRight (10);
        keyEditor.setBounds (area.removeFromLeft (220));
        area.removeFromLeft (14);
        statusLabel.setBounds (area);
    }

private:
    void attemptActivation()
    {
        auto key = keyEditor.getText().trim();
        if (key.isEmpty())
        {
            statusLabel.setText ("Digite a chave de licenca.", juce::dontSendNotification);
            statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);
            return;
        }

        activateButton.setEnabled (false);
        statusLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        statusLabel.setText ("Verificando...", juce::dontSendNotification);

        manager.activateAsync (key, [this] (bool success, juce::String errorMessage)
        {
            activateButton.setEnabled (true);

            if (success)
            {
                statusLabel.setColour (juce::Label::textColourId, juce::Colours::limegreen);
                statusLabel.setText ("Ativado!", juce::dontSendNotification);
                if (onActivated)
                    onActivated();
            }
            else
            {
                statusLabel.setColour (juce::Label::textColourId, juce::Colours::orangered);
                statusLabel.setText (errorMessage, juce::dontSendNotification);
            }
        });
    }

public:
    std::function<void()> onActivated;

private:
    NFLicenseManager& manager;
    juce::Label titleLabel, statusLabel;
    juce::TextEditor keyEditor;
    juce::TextButton activateButton;
};
