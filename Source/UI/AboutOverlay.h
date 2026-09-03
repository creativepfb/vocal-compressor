#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"
#include "../License/NFLicenseManager.h"

/** Popup "About" (mesmo padrão visual dos outros popups do plugin - fundo
    escurecido cobrindo a janela toda + cartão roxo/preto centralizado):
    versão, serial/licença atual (com botão de copiar), status de
    ativação, créditos e os formatos que ESSE binário específico foi
    compilado com (via os mesmos macros JucePlugin_Build_* que o JUCE já
    usa pra decidir o que compilar - reflete exatamente o build real, sem
    lista hardcoded que possa ficar desatualizada). */
class AboutOverlay : public juce::Component, private juce::Timer
{
public:
    explicit AboutOverlay(NFLicenseManager& managerIn) : manager(managerIn)
    {
        titleLabel.setText("NF COLOR COMP", juce::dontSendNotification);
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(titleLabel);

        versionLabel.setJustificationType(juce::Justification::centred);
        versionLabel.setFont(juce::Font(juce::FontOptions(13.0f)));
        versionLabel.setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
        addAndMakeVisible(versionLabel);

        statusLabel.setJustificationType(juce::Justification::centred);
        statusLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        addAndMakeVisible(statusLabel);

        serialCaptionLabel.setText("Serial Number", juce::dontSendNotification);
        serialCaptionLabel.setJustificationType(juce::Justification::centred);
        serialCaptionLabel.setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
        serialCaptionLabel.setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
        addAndMakeVisible(serialCaptionLabel);

        serialValueLabel.setJustificationType(juce::Justification::centred);
        serialValueLabel.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        serialValueLabel.setColour(juce::Label::textColourId, NFLookAndFeel::kKnobGreen);
        addAndMakeVisible(serialValueLabel);

        copyButton.setButtonText("Copy");
        copyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a30));
        copyButton.setColour(juce::TextButton::textColourOffId, NFLookAndFeel::kTextLight);
        copyButton.onClick = [this] { copySerialToClipboard(); };
        addAndMakeVisible(copyButton);

        developedByLabel.setText("Developed by", juce::dontSendNotification);
        distributedByLabel.setText("Distributed by", juce::dontSendNotification);
        for (auto* caption : { &developedByLabel, &distributedByLabel })
        {
            caption->setJustificationType(juce::Justification::centred);
            caption->setFont(juce::Font(juce::FontOptions(10.5f, juce::Font::bold)));
            caption->setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
            addAndMakeVisible(*caption);
        }

        developerNameLabel.setText("Nenno Fernando Audio Tools", juce::dontSendNotification);
        distributorNameLabel.setText("Master Library", juce::dontSendNotification);
        for (auto* name : { &developerNameLabel, &distributorNameLabel })
        {
            name->setJustificationType(juce::Justification::centred);
            name->setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
            name->setColour(juce::Label::textColourId, juce::Colours::white);
            addAndMakeVisible(*name);
        }

        formatsLabel.setText("Formats: " + getSupportedFormatsString(), juce::dontSendNotification);
        formatsLabel.setJustificationType(juce::Justification::centred);
        formatsLabel.setFont(juce::Font(juce::FontOptions(11.5f)));
        formatsLabel.setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
        addAndMakeVisible(formatsLabel);

        // Abre o PDF instalado num local fixo/compartilhado (ver
        // getManualFile()) no visualizador padrão do sistema - só
        // aparece se o arquivo realmente existir (instalação feita pelo
        // instalador oficial; num build de teste solto pode não ter).
        manualButton.setButtonText("Manual (PT)");
        manualButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a30));
        manualButton.setColour(juce::TextButton::textColourOffId, NFLookAndFeel::kKnobGreen);
        manualButton.onClick = [this] { getManualFile().startAsProcess(); };
        addChildComponent(manualButton);

        // Codepoint numerico (nao caractere UTF-8 cru no literal) - evita
        // depender de como o compilador interpreta a codificacao do
        // arquivo fonte pra simbolos fora do ASCII.
        copyrightLabel.setText(juce::String::charToString((juce::juce_wchar) 0x00A9) + " 2026 Nenno Fernando Audio Tools",
                                juce::dontSendNotification);
        websiteLabel.setText("masterlibrary.com.br", juce::dontSendNotification);
        for (auto* footer : { &copyrightLabel, &websiteLabel })
        {
            footer->setJustificationType(juce::Justification::centred);
            footer->setFont(juce::Font(juce::FontOptions(10.5f)));
            footer->setColour(juce::Label::textColourId, NFLookAndFeel::kTextDim);
            addAndMakeVisible(*footer);
        }

        closeButton.setButtonText(juce::String::charToString((juce::juce_wchar) 0x2715)); // "✕"
        closeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        closeButton.setColour(juce::TextButton::textColourOffId, NFLookAndFeel::kTextDim);
        closeButton.onClick = [this] { setVisible(false); };
        addAndMakeVisible(closeButton);
    }

    /** Recarrega o status/serial atual da licença (pode ter mudado desde
        a última vez que a janela foi aberta) e mostra o popup. */
    void show()
    {
        bool activated = manager.isActivated();
        auto key = manager.getLicenseKey();

        statusLabel.setText(activated ? "License Status: Activated" : "License Status: Not Activated",
                             juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId,
                               activated ? NFLookAndFeel::kKnobGreen : juce::Colour(0xffff6b6b));

        bool hasSerial = activated && key.isNotEmpty();
        serialCaptionLabel.setVisible(hasSerial);
        serialValueLabel.setVisible(hasSerial);
        copyButton.setVisible(hasSerial);
        serialValueLabel.setText(key, juce::dontSendNotification);

        versionLabel.setText(juce::String("Version: ") + JucePlugin_VersionString, juce::dontSendNotification);

        manualButton.setVisible(getManualFile().existsAsFile());

        copyButton.setButtonText("Copy");
        stopTimer();

        resized();
        setVisible(true);
        toFront(true);
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
        auto card = cardBounds();
        closeButton.setBounds(card.getRight() - 34, card.getY() + 6, 28, 28);

        auto area = card.reduced(26, 20);
        titleLabel.setBounds(area.removeFromTop(28));
        area.removeFromTop(4);
        versionLabel.setBounds(area.removeFromTop(18));
        area.removeFromTop(12);
        statusLabel.setBounds(area.removeFromTop(20));
        area.removeFromTop(14);

        if (serialCaptionLabel.isVisible())
        {
            serialCaptionLabel.setBounds(area.removeFromTop(14));
            auto serialRow = area.removeFromTop(28);
            copyButton.setBounds(serialRow.removeFromRight(58));
            serialRow.removeFromRight(6);
            serialValueLabel.setBounds(serialRow);
            area.removeFromTop(14);
        }

        auto creditsRow = area.removeFromTop(56);
        auto devCol = creditsRow.removeFromLeft(creditsRow.getWidth() / 2);
        auto distCol = creditsRow;
        developedByLabel.setBounds(devCol.removeFromTop(14));
        developerNameLabel.setBounds(devCol);
        distributedByLabel.setBounds(distCol.removeFromTop(14));
        distributorNameLabel.setBounds(distCol);

        area.removeFromTop(12);
        formatsLabel.setBounds(area.removeFromTop(18));

        if (manualButton.isVisible())
        {
            area.removeFromTop(12);
            manualButton.setBounds(area.removeFromTop(30).withSizeKeepingCentre(140, 30));
        }

        area.removeFromTop(12);
        copyrightLabel.setBounds(area.removeFromTop(16));
        websiteLabel.setBounds(area.removeFromTop(16));
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (! cardBounds().toFloat().contains(e.position))
            setVisible(false);
    }

private:
    juce::Rectangle<int> cardBounds() const
    {
        return getLocalBounds().withSizeKeepingCentre(360, 460);
    }

    /** Caminho FIXO e compartilhado do manual em PDF, fora de qualquer
        pasta específica de formato (VST3/AAX/AU instalam em locais
        diferentes) - ver comentário equivalente no instalador Windows
        (.iss) e no workflow do macOS. Só um arquivo estático no disco:
        não afeta o binário do plugin, só "carrega" quando o usuário
        clica e o sistema abre no visualizador de PDF padrão dele. */
    static juce::File getManualFile()
    {
        return juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory)
                   .getChildFile("NF Plugins")
                   .getChildFile("Color Comp")
                   .getChildFile("NF-Color-Comp-Manual-PT.pdf");
    }

    static juce::String getSupportedFormatsString()
    {
        juce::StringArray formats;
       #if JucePlugin_Build_VST3
        formats.add("VST3");
       #endif
       #if JucePlugin_Build_AU
        formats.add("AU");
       #endif
       #if JucePlugin_Build_AAX
        formats.add("AAX");
       #endif
       #if JucePlugin_Build_Standalone
        formats.add("Standalone");
       #endif
        return formats.isEmpty() ? juce::String("-") : formats.joinIntoString("  ·  ");
    }

    void copySerialToClipboard()
    {
        juce::SystemClipboard::copyTextToClipboard(serialValueLabel.getText());
        copyButton.setButtonText(juce::String("COPIED ") + juce::String::charToString((juce::juce_wchar) 0x2713));
        startTimer(1500);
    }

    void timerCallback() override
    {
        copyButton.setButtonText("Copy");
        stopTimer();
    }

    NFLicenseManager& manager;

    juce::Label titleLabel, versionLabel, statusLabel;
    juce::Label serialCaptionLabel, serialValueLabel;
    juce::TextButton copyButton;
    juce::Label developedByLabel, developerNameLabel, distributedByLabel, distributorNameLabel;
    juce::Label formatsLabel;
    juce::TextButton manualButton;
    juce::Label copyrightLabel, websiteLabel;
    juce::TextButton closeButton;
};
