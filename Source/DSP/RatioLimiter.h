#pragma once

/** Constante e checagem compartilhadas entre o DSP (VocalCompressorDSP) e
    a GUI (knob de Ratio, LookAndFeel) pra decidir quando o Ratio está no
    modo Limiter dedicado (extremo "∞:1" do knob). Usar SÓ essa checagem
    nos dois lados - nunca duplicar o número "20.0" solto em outro lugar -
    pra jamais a tela mostrar "LIMITER" sem o DSP estar realmente
    processando como limiter, ou vice-versa. */
namespace RatioLimiter
{
    // Tem que ser o mesmo valor do máximo do NormalisableRange do
    // parâmetro "ratio" em PluginProcessor.cpp (createParameterLayout).
    constexpr float ratioMax = 20.0f;
    constexpr float epsilon = 0.01f;

    inline bool isLimiterMode(float ratio) { return ratio >= ratioMax - epsilon; }
}
