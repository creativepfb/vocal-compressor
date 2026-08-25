cmake_minimum_required(VERSION 3.22)

project(VocalCompressor VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------------
# JUCE
# Ajuste o caminho abaixo para onde o JUCE está instalado na sua máquina.
# Se você clonou o repositório oficial (github.com/juce-framework/JUCE),
# aponte para a pasta raiz clonada.
# --------------------------------------------------------------------------
add_subdirectory(JUCE)   # <- coloque o JUCE como subpasta ou ajuste o caminho

if (APPLE)
    set(VOCALCOMPRESSOR_FORMATS VST3 AU Standalone)
else()
    set(VOCALCOMPRESSOR_FORMATS VST3 Standalone)
endif()

juce_add_plugin(VocalCompressor
    COMPANY_NAME "NF AUDIO TOOLS"
    # Bundle ID em minusculas/sem espaco, separado do COMPANY_NAME (que é só
    # o nome de exibição) - o JUCE não sanitiza o nome da empresa ao montar
    # o identificador reverse-DNS, e "NF AUDIO TOOLS" com espaço quebraria o
    # Bundle ID no macOS (AU exige identificador válido, sem espaços).
    BUNDLE_ID com.nfaudiotools.vocalcompressor
    PLUGIN_MANUFACTURER_CODE Nfat          # NF Audio Tools
    PLUGIN_CODE Vco1                       # troque por um código único de 4 letras
    FORMATS ${VOCALCOMPRESSOR_FORMATS}     # adicione AAX aqui depois de configurar o SDK da Avid
    PRODUCT_NAME "NF - Vocal Compressor"
    # Ícone embutido no .exe/.app do Standalone (o que aparece na barra de
    # tarefas do Windows, Alt-Tab, Dock do macOS) - sem isso o JUCE usa um
    # ícone genérico. O juceaide gera .ico/.icns automaticamente a partir
    # desse PNG único em tempo de build.
    ICON_BIG "Source/Resources/app-icon.png"
    ICON_SMALL "Source/Resources/app-icon.png"
)

target_sources(VocalCompressor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

juce_add_binary_data(VocalCompressorAssets
    SOURCES
        Source/Resources/faceplate-6.png
        Source/Resources/botao-png-126px.png
)

target_compile_definitions(VocalCompressor
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(VocalCompressor
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_cryptography
        juce::juce_events
        VocalCompressorAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# --------------------------------------------------------------------------
# Para habilitar AAX depois:
# 1. Baixe o AAX SDK em developer.avid.com
# 2. Descomente as linhas abaixo e ajuste o caminho
# 3. Adicione AAX na lista FORMATS acima
# --------------------------------------------------------------------------
# juce_set_aax_sdk_path(${CMAKE_SOURCE_DIR}/AAX_SDK)
