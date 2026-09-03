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

# AAX - SDK da Avid, NUNCA commitado (repo é público, SDK é material sob
# licença da Avid - ver .gitignore). Cada máquina que for buildar AAX
# precisa da pasta AAX_SDK/ na raiz do repo, extraída localmente.
if (EXISTS "${CMAKE_SOURCE_DIR}/AAX_SDK")
    juce_set_aax_sdk_path(${CMAKE_SOURCE_DIR}/AAX_SDK)
    list(APPEND VOCALCOMPRESSOR_FORMATS AAX)
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
    FORMATS ${VOCALCOMPRESSOR_FORMATS}     # AAX entra sozinho na lista se AAX_SDK/ existir (ver acima)
    PRODUCT_NAME "NF - Color Comp"
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
