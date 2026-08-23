#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLicenseManager.h"

/** Tela simples de ativação - campo de texto pra chave + botão + status.
    Fica por cima de tudo (overlay) enquanto o produto não estiver
    ativado; some sozinha quando a ativação der certo. */
class LicenseActivationComponent : public juce::Component
{
public:
    explicit LicenseActivationComponent (NFLicenseManager& managerIn) : manager (managerIn)
    {
        titleLabel.setText ("Ative sua licenca", juce::dontSendNotification);
        titleLabel.setJustificationType (juce::Justification::centred);
        titleLabel.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
        addAndMakeVisible (titleLabel);

        keyEditor.setTextToShowWhenEmpty ("NFVC-XXXXX-XXXXX-XXXXX", juce::Colours::grey);
        keyEditor.setJustification (juce::Justification::centred);
        addAndMakeVisible (keyEditor);

        activateButton.setButtonText ("Activate License");
        activateButton.onClick = [this] { attemptActivation(); };
        addAndMakeVisible (activateButton);

        statusLabel.setJustificationType (juce::Justification::centred);
        statusLabel.setFont (juce::Font (juce::FontOptions (13.0f)));
        addAndMakeVisible (statusLabel);

        setOpaque (true);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xee0b0b0d));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (30).withSizeKeepingCentre (320, 160);
        titleLabel.setBounds (area.removeFromTop (30));
        area.removeFromTop (10);
        keyEditor.setBounds (area.removeFromTop (30));
        area.removeFromTop (10);
        activateButton.setBounds (area.removeFromTop (30));
        area.removeFromTop (10);
        statusLabel.setBounds (area.removeFromTop (30));
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
