#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "NFLookAndFeel.h"

/**
    EQ paramétrico gráfico: fica por CIMA do RTA (que continua intocado por
    baixo, componente separado). Desenha a curva REAL (calculada com os
    mesmos coeficientes IIR usados no áudio, não uma curva "de mentira"
    baseada só nos pontos) e 5 nós arrastáveis:

      Low Cut (só frequência) - Low Shelf - Peak/Bell (freq+gain+Q via
      scroll) - High Shelf - High Cut (só frequência)

    Arrastar horizontal = frequência (escala log). Arrastar vertical =
    ganho (bandas com ganho). Scroll do mouse sobre o nó Peak = Q.
    Clicar num nó Low/High Cut já liga o filtro (lowCutOn/highCutOn).

    Criação/remoção dinâmica de bandas (double-click / botão direito) fica
    pra uma próxima etapa - por enquanto são 5 bandas fixas, cada uma já
    ligada a um parâmetro real da APVTS.
*/
class EQGraphComponent : public juce::Component, private juce::Timer
{
public:
    EQGraphComponent(juce::AudioProcessorValueTreeState& stateIn, double sampleRateIn)
        : apvts(stateIn), sampleRate(sampleRateIn > 0.0 ? sampleRateIn : 44100.0)
    {
        nodes.push_back({ "lowCutFreq", "", "", "lowCutOn", false, false, juce::Colour(0xffff5b5b) });
        nodes.push_back({ "lowShelfFreq", "lowShelfGain", "", "", true, false, juce::Colour(0xffffd23f) });
        nodes.push_back({ "peakFreq", "peakGain", "peakQ", "", true, true, NFLookAndFeel::kKnobGreen });
        nodes.push_back({ "highShelfFreq", "highShelfGain", "", "", true, false, juce::Colour(0xff6ec6ff) });
        nodes.push_back({ "highCutFreq", "", "", "highCutOn", false, false, juce::Colour(0xffff5b5b) });

        setInterceptsMouseClicks(true, false);
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        drawGrid(g, area);
        drawCurve(g, area);
        drawNodes(g, area);
        drawTooltip(g, area);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        int hit = findNodeNear(e.position, area);
        draggingIndex = hit;
        selectedIndex = hit;

        if (hit >= 0 && nodes[(size_t) hit].onParamId.isNotEmpty())
            setNormalized(nodes[(size_t) hit].onParamId, 1.0f);

        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggingIndex < 0)
            return;

        auto area = getLocalBounds().toFloat().reduced(2.0f);
        auto& n = nodes[(size_t) draggingIndex];

        setRangedValue(n.freqParamId, xToFreq(e.position.x, area));

        if (n.hasGain)
            setRangedValue(n.gainParamId, yToDb(e.position.y, area));

        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingIndex = -1;
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        int newHover = findNodeNear(e.position, area);
        if (newHover != hoverIndex)
        {
            hoverIndex = newHover;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override
    {
        hoverIndex = -1;
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        int hit = findNodeNear(e.position, area);
        if (hit < 0 || !nodes[(size_t) hit].hasQ)
            return;

        auto& n = nodes[(size_t) hit];
        float current = getReal(n.qParamId);
        float newQ = juce::jlimit(0.1f, 10.0f, current + wheel.deltaY * 3.0f);
        setRangedValue(n.qParamId, newQ);
        selectedIndex = hit;
        repaint();
    }

private:
    struct Node
    {
        juce::String freqParamId, gainParamId, qParamId, onParamId;
        bool hasGain, hasQ;
        juce::Colour colour;
    };

    static constexpr float minFreq = 31.0f;
    static constexpr float maxFreq = 16000.0f;
    static constexpr float minDb = -15.0f;
    static constexpr float maxDb = 15.0f;

    float freqToX(float freq, juce::Rectangle<float> area) const
    {
        float t = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
        return area.getX() + t * area.getWidth();
    }

    float xToFreq(float x, juce::Rectangle<float> area) const
    {
        float t = juce::jlimit(0.0f, 1.0f, (x - area.getX()) / area.getWidth());
        return minFreq * std::pow(maxFreq / minFreq, t);
    }

    float dbToY(float db, juce::Rectangle<float> area) const
    {
        float t = (db - minDb) / (maxDb - minDb);
        return area.getBottom() - t * area.getHeight();
    }

    float yToDb(float y, juce::Rectangle<float> area) const
    {
        float t = juce::jlimit(0.0f, 1.0f, (area.getBottom() - y) / area.getHeight());
        return minDb + t * (maxDb - minDb);
    }

    float getReal(const juce::String& id) const
    {
        auto* raw = apvts.getRawParameterValue(id);
        return raw != nullptr ? raw->load() : 0.0f;
    }

    void setRangedValue(const juce::String& id, float realValue)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(realValue));
    }

    void setNormalized(const juce::String& id, float norm)
    {
        if (auto* param = apvts.getParameter(id))
            param->setValueNotifyingHost(norm);
    }

    juce::Point<float> nodePosition(const Node& n, juce::Rectangle<float> area) const
    {
        float x = freqToX(getReal(n.freqParamId), area);
        float y = n.hasGain ? dbToY(getReal(n.gainParamId), area) : area.getCentreY();
        return { x, y };
    }

    int findNodeNear(juce::Point<float> pos, juce::Rectangle<float> area) const
    {
        for (int i = (int) nodes.size() - 1; i >= 0; --i)
            if (pos.getDistanceFrom(nodePosition(nodes[(size_t) i], area)) < 12.0f)
                return i;
        return -1;
    }

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setColour(juce::Colour(0x331c1c22));
        for (float f : { 100.0f, 1000.0f, 10000.0f })
            g.drawVerticalLine(juce::roundToInt(freqToX(f, area)), area.getY(), area.getBottom());

        g.setColour(juce::Colour(0x552a2a30));
        g.drawHorizontalLine(juce::roundToInt(dbToY(0.0f, area)), area.getX(), area.getRight());
    }

    float combinedMagnitudeDb(float freq) const
    {
        double mag = 1.0;

        if (getReal("lowCutOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, juce::jmax(20.0f, getReal("lowCutFreq")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        mag *= juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, juce::jmax(20.0f, getReal("lowShelfFreq")), 0.707f,
                   juce::Decibels::decibelsToGain(getReal("lowShelfGain")))->getMagnitudeForFrequency((double) freq, sampleRate);

        mag *= juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, juce::jmax(20.0f, getReal("peakFreq")),
                   juce::jmax(0.1f, getReal("peakQ")), juce::Decibels::decibelsToGain(getReal("peakGain")))
                   ->getMagnitudeForFrequency((double) freq, sampleRate);

        mag *= juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, juce::jmax(20.0f, getReal("highShelfFreq")), 0.707f,
                   juce::Decibels::decibelsToGain(getReal("highShelfGain")))->getMagnitudeForFrequency((double) freq, sampleRate);

        if (getReal("highCutOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, juce::jmax(200.0f, getReal("highCutFreq")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        return juce::Decibels::gainToDecibels((float) mag, -60.0f);
    }

    void drawCurve(juce::Graphics& g, juce::Rectangle<float> area)
    {
        juce::Path curve;
        constexpr int steps = 150;
        for (int i = 0; i <= steps; ++i)
        {
            float t = (float) i / (float) steps;
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            float db = juce::jlimit(minDb, maxDb, combinedMagnitudeDb(freq));
            float x = area.getX() + t * area.getWidth();
            float y = dbToY(db, area);

            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }

        g.setColour(NFLookAndFeel::kPurple.withAlpha(0.35f));
        g.strokePath(curve, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(NFLookAndFeel::kPurple);
        g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawNodes(juce::Graphics& g, juce::Rectangle<float> area)
    {
        for (int i = 0; i < (int) nodes.size(); ++i)
        {
            auto& n = nodes[(size_t) i];
            auto p = nodePosition(n, area);
            bool isOn = n.onParamId.isEmpty() || getReal(n.onParamId) > 0.5f;
            float radius = (i == hoverIndex || i == draggingIndex) ? 8.0f : 6.0f;

            g.setColour(n.colour.withAlpha(isOn ? 0.85f : 0.30f));
            g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.drawEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);
        }
    }

    void drawTooltip(juce::Graphics& g, juce::Rectangle<float> area)
    {
        int idx = draggingIndex >= 0 ? draggingIndex : (hoverIndex >= 0 ? hoverIndex : -1);
        if (idx < 0)
            return;

        auto& n = nodes[(size_t) idx];
        auto p = nodePosition(n, area);

        juce::String text = formatFreq(getReal(n.freqParamId));
        if (n.hasGain)
            text += "   " + juce::String(getReal(n.gainParamId), 1) + " dB";
        if (n.hasQ)
            text += "   Q " + juce::String(getReal(n.qParamId), 2);

        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        // Estimativa simples de largura (evita depender de API de medição de
        // texto que varia entre versões do JUCE) - só usada pra desenhar o
        // fundo do tooltip, não precisa ser exata.
        float textWidth = (float) text.length() * 7.5f + 14.0f;
        auto box = juce::Rectangle<float>(p.x - textWidth * 0.5f, p.y - 26.0f, textWidth, 18.0f);
        box = box.constrainedWithin(area);

        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.fillRoundedRectangle(box, 3.0f);
        g.setColour(n.colour);
        g.drawFittedText(text, box.getSmallestIntegerContainer(), juce::Justification::centred, 1);
    }

    static juce::String formatFreq(float freq)
    {
        if (freq >= 1000.0f)
            return juce::String(freq / 1000.0f, 2) + " kHz";
        return juce::String((int) freq) + " Hz";
    }

    void timerCallback() override { repaint(); }

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate;
    std::vector<Node> nodes;
    int draggingIndex = -1;
    int hoverIndex = -1;
    int selectedIndex = -1;
};
