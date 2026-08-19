# Vocal Compressor — Projeto JUCE

Compressor de voz com release adaptativo (program-dependent), soft-knee,
saturação suave e medidor de gain reduction. Formatos: VST3 e Standalone
(AAX pode ser adicionado depois — veja abaixo).

## Estrutura

```
VocalCompressor/
├── CMakeLists.txt
├── README.md
└── Source/
    ├── PluginProcessor.h / .cpp   → DSP + parâmetros + estado
    ├── PluginEditor.h / .cpp      → GUI (knobs + medidor)
    ├── GainReductionMeter.h       → componente do medidor de GR
    └── DSP/
        └── VocalCompressorDSP.h   → núcleo do compressor (por canal)
```

## Pré-requisitos

- CMake 3.22+
- Um compilador C++17 (Xcode no Mac, Visual Studio no Windows, GCC/Clang no Linux)
- O framework JUCE (não incluído aqui — baixe separadamente)

## Passo a passo

1. **Baixe o JUCE**
   Clone ou baixe de https://github.com/juce-framework/JUCE e coloque a
   pasta `JUCE` dentro da raiz deste projeto (ao lado do `CMakeLists.txt`),
   ou edite o caminho no `add_subdirectory(JUCE)` do `CMakeLists.txt`.

2. **Configure o projeto**
   ```bash
   cd VocalCompressor
   cmake -B build
   ```

3. **Compile**
   ```bash
   cmake --build build --config Release
   ```

4. **Encontre o plugin compilado**
   O VST3 é copiado automaticamente para a pasta de plugins do sistema:
   - macOS: `~/Library/Audio/Plug-Ins/VST3/`
   - Windows: `C:\Program Files\Common Files\VST3\`

   O executável Standalone fica em `build/VocalCompressor_artefacts/Release/Standalone/`.

5. **Teste no DAW**
   Abra o REAPER (ou outro host), faça um rescan de plugins e insira o
   "Vocal Compressor" numa trilha de voz.

## Ajustando os parâmetros da DSP

Os valores de tempo de ataque/release "base" (5ms attack, 50ms/400ms
release rápido/lento) estão em `VocalCompressorDSP::prepare()`. Ajuste-os
lá se quiser mudar o comportamento padrão antes mesmo de mexer nos knobs.

## Habilitando AAX

1. Registre-se como desenvolvedor Avid em developer.avid.com e baixe o AAX SDK
2. Descomente e ajuste a linha `juce_set_aax_sdk_path(...)` no `CMakeLists.txt`
3. Adicione `AAX` à lista `FORMATS` em `juce_add_plugin(...)`
4. A assinatura final do binário AAX exige PACE/iLok — processo separado,
   feito através do próprio programa de desenvolvedor Avid

## Próximos incrementos sugeridos

- Stereo linking (usar o envelope máximo entre L/R em vez de processar
  os canais de forma independente)
- LookAndFeel customizado para os knobs (visual próprio)
- Sidechain input (compressão disparada por outro sinal)
- Oversampling na saturação para reduzir aliasing em frequências altas
