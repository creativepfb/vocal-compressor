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
    COMPANY_NAME "SeuNomeAqui"
    PLUGIN_MANUFACTURER_CODE Sean          # troque pelo seu código de 4 letras
    PLUGIN_CODE Vco1                       # troque por um código único de 4 letras
    FORMATS ${VOCALCOMPRESSOR_FORMATS}     # adicione AAX aqui depois de configurar o SDK da Avid
    PRODUCT_NAME "Vocal Compressor"
)

target_sources(VocalCompressor
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
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
