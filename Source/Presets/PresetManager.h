#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/** Gerencia os presets do NF Vocal Compressor: lista, salva, carrega e
    navega (anterior/próximo) entre arquivos .xml guardados na pasta de
    presets do usuário.

    Serializa através do MESMO ValueTree/estado já usado pelo
    AudioProcessorValueTreeState (apvts.copyState()/replaceState()) - o
    mesmo caminho que getStateInformation()/setStateInformation() já usam
    pra sessão da DAW. Não duplica nenhum parâmetro nem cria uma segunda
    fonte de verdade: o arquivo de preset é só esse ValueTree existente,
    empacotado com um nome e uma versão de formato.

    Rastreia "modificado desde o último load/save" ouvindo TODOS os
    parâmetros via apvts.addParameterListener() de forma genérica (sem
    hardcode de IDs) - se o plugin ganhar parâmetros novos no futuro, o
    preset system continua funcionando sem precisar editar este arquivo.

    ChangeBroadcaster: a GUI (PresetBarComponent) escuta pra saber quando
    redesenhar (preset trocado/carregado/salvo, ou parâmetro alterado à
    mão -> "modificado"). */
class PresetManager : private juce::AudioProcessorValueTreeState::Listener,
                       private juce::AsyncUpdater,
                       public juce::ChangeBroadcaster
{
public:
    static constexpr int presetFormatVersion = 1;
    static const inline juce::String defaultPresetName = "Default";

    explicit PresetManager(juce::AudioProcessorValueTreeState& apvtsIn) : apvts(apvtsIn)
    {
        for (auto* p : apvts.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                apvts.addParameterListener(ranged->paramID, this);

        refreshPresetList();
    }

    ~PresetManager() override
    {
        cancelPendingUpdate();
        for (auto* p : apvts.processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p))
                apvts.removeParameterListener(ranged->paramID, this);
    }

    /** Pasta onde os presets ficam salvos - criada na primeira chamada se
        ainda não existir. Mesma convenção de local usada pelo
        NFLicenseManager (%AppData%/NF Plugins/...), mas em subpasta
        própria de presets. */
    juce::File getPresetsDirectory() const
    {
        auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("NF Plugins")
                       .getChildFile("presets")
                       .getChildFile("NF Vocal Compressor");
        dir.createDirectory();
        return dir;
    }

    /** Nomes dos presets salvos em disco, em ordem alfabética (sem
        extensão, sem incluir "Default" - esse é virtual, ver
        getAllEntriesIncludingDefault()). */
    const juce::StringArray& getPresetNames() const { return presetNames; }

    /** "Default" seguido dos presets salvos, na ordem usada pela
        navegação anterior/próximo. */
    juce::StringArray getAllEntriesIncludingDefault() const
    {
        juce::StringArray all;
        all.add(defaultPresetName);
        all.addArray(presetNames);
        return all;
    }

    juce::String getCurrentPresetName() const { return currentPresetName; }
    bool isModified() const { return modified; }

    /** Recarrega a lista de presets do disco - chamar depois de salvar,
        apagar ou adicionar um arquivo por fora (ex: usuário mexeu na
        pasta manualmente). */
    void refreshPresetList()
    {
        presetNames.clear();
        for (auto& f : getPresetsDirectory().findChildFiles(juce::File::findFiles, false, "*" + presetFileExtension))
            presetNames.add(f.getFileNameWithoutExtension());
        presetNames.sort(true);
    }

    /** Reseta todos os parâmetros pro valor default de fábrica - o
        "preset" Default, que não tem arquivo próprio. */
    void loadDefault()
    {
        for (auto* p : apvts.processor.getParameters())
            p->setValueNotifyingHost(p->getDefaultValue());

        cancelPendingUpdate();
        currentPresetName = defaultPresetName;
        modified = false;
        sendChangeMessage();
    }

    /** Carrega um preset pelo nome (ou o Default, se for o nome dele). */
    void loadByName(const juce::String& name)
    {
        if (name == defaultPresetName)
        {
            loadDefault();
            return;
        }

        loadFile(getPresetsDirectory().getChildFile(name + presetFileExtension));
    }

    void loadNext()
    {
        auto all = getAllEntriesIncludingDefault();
        auto idx = all.indexOf(currentPresetName);
        if (idx < 0 || idx >= all.size() - 1)
            return;
        loadByName(all[idx + 1]);
    }

    void loadPrevious()
    {
        auto all = getAllEntriesIncludingDefault();
        auto idx = all.indexOf(currentPresetName);
        if (idx <= 0)
            return;
        loadByName(all[idx - 1]);
    }

    /** Salva o estado atual dos parâmetros como um preset novo (ou
        sobrescreve um existente com o mesmo nome). Retorna false se o
        nome (depois de sanitizado) ficar vazio ou a escrita falhar. */
    bool savePreset(const juce::String& rawName)
    {
        auto name = sanitizeName(rawName);
        if (name.isEmpty())
            return false;

        juce::ValueTree wrapper("NFVocalCompressorPreset");
        wrapper.setProperty("name", name, nullptr);
        wrapper.setProperty("formatVersion", presetFormatVersion, nullptr);
        wrapper.appendChild(apvts.copyState(), nullptr);

        std::unique_ptr<juce::XmlElement> xml(wrapper.createXml());
        auto file = getPresetsDirectory().getChildFile(name + presetFileExtension);
        bool ok = xml != nullptr && xml->writeTo(file);

        if (ok)
        {
            refreshPresetList();
            cancelPendingUpdate();
            currentPresetName = name;
            modified = false;
            sendChangeMessage();
        }
        return ok;
    }

private:
    static const inline juce::String presetFileExtension = ".xml";

    void loadFile(const juce::File& file)
    {
        if (! file.existsAsFile())
            return;

        std::unique_ptr<juce::XmlElement> xml(juce::XmlDocument::parse(file));
        if (xml == nullptr)
            return;

        auto* paramsXml = xml->getChildElement(0);
        if (paramsXml == nullptr)
            return;

        auto newState = juce::ValueTree::fromXml(*paramsXml);
        if (! newState.isValid() || newState.getType() != apvts.state.getType())
            return;

        apvts.replaceState(newState);

        // Zero de-sincronia entre os N callbacks assíncronos de
        // parameterChanged() disparados pelo replaceState() (todos
        // coalescidos num só handleAsyncUpdate() pendente, ver
        // AsyncUpdater) e o estado final que queremos anunciar - em vez
        // de deixar o callback rodar (e marcar "modified" à toa logo
        // depois de um load), cancela o que estiver pendente e manda a
        // mensagem certa manualmente aqui.
        cancelPendingUpdate();
        currentPresetName = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
        modified = false;
        sendChangeMessage();
    }

    static juce::String sanitizeName(const juce::String& raw)
    {
        auto trimmed = raw.trim();
        juce::String cleaned;
        for (auto c : trimmed)
        {
            if (juce::CharacterFunctions::isLetterOrDigit(c) || c == ' ' || c == '-' || c == '_')
                cleaned += juce::String::charToString(c);
        }
        return cleaned.trim();
    }

    void parameterChanged(const juce::String&, float) override
    {
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        if (! modified)
        {
            modified = true;
            sendChangeMessage();
        }
    }

    juce::AudioProcessorValueTreeState& apvts;
    juce::StringArray presetNames;
    juce::String currentPresetName { defaultPresetName };
    bool modified = false;
};
