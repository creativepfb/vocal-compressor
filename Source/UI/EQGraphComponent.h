#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "NFLookAndFeel.h"
#include "HardwareButton.h"

/**
    EQ paramétrico gráfico: fica por CIMA do RTA (componente separado, não
    mexe nele). Mostra e edita o único EQ que existe (Pre, antes do
    compressor) ou o HPF do detector, dependendo do botão selecionado.

    Desenha a curva REAL (mesmos coeficientes IIR usados no áudio) e 5 nós
    arrastáveis: Low Cut - Peak - Peak - Peak - High Cut (as 3 bandas do
    meio ainda usam os nomes internos lowShelf/peak/highShelf, mas as três
    são filtros Peaking de verdade, com Q ajustável via scroll).
*/
class EQGraphComponent : public juce::Component, private juce::Timer
{
public:
    /** ON/OFF (parâmetro de áudio real, por botão) e "qual filtro está
        selecionado pro gráfico" são estados separados de propósito - o
        EQ e o HPF podem estar ON ao mesmo tempo, mas só um fica
        selecionado (mostrado/editável) no gráfico por vez. */
    enum class SelectedFilter { Pre, Hpf };

    EQGraphComponent(juce::AudioProcessorValueTreeState& stateIn, double sampleRateIn)
        : apvts(stateIn), sampleRate(sampleRateIn > 0.0 ? sampleRateIn : 44100.0)
    {
        // onBase de cada nó = parâmetro de bypass INDIVIDUAL daquela banda
        // (duplo clique na bolinha liga/desliga - ver mouseDoubleClick).
        // Não confundir com o "Enabled" do estágio inteiro (preOnButton)
        // nem entre si - cada banda é independente.
        nodes.push_back({ "LowCutFreq", "", "", "LowCutOn", false, false, juce::Colour(0xffff5b5b) });
        nodes.push_back({ "LowShelfFreq", "LowShelfGain", "LowShelfQ", "LowShelfOn", true, true, juce::Colour(0xffffd23f) });
        nodes.push_back({ "PeakFreq", "PeakGain", "PeakQ", "PeakOn", true, true, NFLookAndFeel::kKnobGreen });
        nodes.push_back({ "HighShelfFreq", "HighShelfGain", "HighShelfQ", "HighShelfOn", true, true, juce::Colour(0xff6ec6ff) });
        nodes.push_back({ "HighCutFreq", "", "", "HighCutOn", false, false, juce::Colour(0xffff5b5b) });

        resetButton.setMomentary(true);
        resetButton.setShowLed(false);
        resetButton.onClick = [this] { resetSelectedEQ(); };
        addAndMakeVisible(resetButton);

        // Precisa permitir clique nos filhos (o botão RESET) - por padrão
        // esse componente intercepta tudo pra si mesmo (arrasto de nós).
        setInterceptsMouseClicks(true, true);
        startTimerHz(30);
    }

    /** Troca qual filtro (Pre EQ ou HPF) é mostrado/editado no gráfico. Não
        afeta o processamento de áudio nem os outros filtros - é só estado
        da GUI. */
    void setSelectedFilter(SelectedFilter f)
    {
        selected = f;
        draggingIndex = hoverIndex = -1;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        drawGrid(g, area);
        drawCurve(g, area);
        drawNodes(g, area);
        drawTooltip(g, area);
        drawHeader(g, area);
    }

    void resized() override
    {
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        resetButton.setBounds(juce::Rectangle<float>(area.getX() + 4.0f, area.getY() + 4.0f, 52.0f, 20.0f)
                                   .toNearestInt());
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (selected == SelectedFilter::Hpf)
            return; // HPF não tem nós arrastáveis (filtro fixo, só on/off)

        auto area = getLocalBounds().toFloat().reduced(2.0f);
        draggingIndex = findNodeNear(e.position, area);
        repaint();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        if (draggingIndex < 0)
            return;

        auto area = getLocalBounds().toFloat().reduced(2.0f);
        auto& n = nodes[(size_t) draggingIndex];

        setRangedValue(n.freqBase, xToFreq(e.position.x, area));

        if (n.hasGain)
            setRangedValue(n.gainBase, yToDb(e.position.y, area));

        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        draggingIndex = -1;
    }

    /** Duplo clique numa bolinha liga/desliga o BYPASS individual daquela
        banda - a bolinha continua no lugar, frequência/ganho/Q continuam
        guardados, só para (ou volta) de atuar no áudio. Usa o mecanismo
        de duplo clique nativo do JUCE (não detecção manual de 2 cliques),
        então o clique simples (seleção/arrasto) continua 100% intocado. */
    void mouseDoubleClick(const juce::MouseEvent& e) override
    {
        if (selected == SelectedFilter::Hpf)
            return;

        auto area = getLocalBounds().toFloat().reduced(2.0f);
        int hit = findNodeNear(e.position, area);
        if (hit < 0)
            return;

        auto& n = nodes[(size_t) hit];
        if (n.onBase.isEmpty())
            return;

        bool currentlyOn = getReal(n.onBase) > 0.5f;
        setNormalized(n.onBase, currentlyOn ? 0.0f : 1.0f);
        repaint();
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
        if (selected == SelectedFilter::Hpf)
            return;

        auto area = getLocalBounds().toFloat().reduced(2.0f);
        int hit = findNodeNear(e.position, area);
        if (hit < 0 || !nodes[(size_t) hit].hasQ)
            return;

        auto& n = nodes[(size_t) hit];
        float current = getReal(n.qBase);
        float newQ = juce::jlimit(0.1f, 10.0f, current + wheel.deltaY * 3.0f);
        setRangedValue(n.qBase, newQ);
        repaint();
    }

private:
    struct Node
    {
        juce::String freqBase, gainBase, qBase, onBase;
        bool hasGain, hasQ;
        juce::Colour colour;
    };

    static constexpr float minFreq = 31.0f;
    static constexpr float maxFreq = 16000.0f;
    static constexpr float minDb = -15.0f;
    static constexpr float maxDb = 15.0f;

    juce::String prefix() const { return "pre"; }
    juce::String fullId(const juce::String& base) const { return prefix() + base; }

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

    float getReal(const juce::String& base) const
    {
        auto* raw = apvts.getRawParameterValue(fullId(base));
        return raw != nullptr ? raw->load() : 0.0f;
    }

    void setRangedValue(const juce::String& base, float realValue)
    {
        if (auto* param = apvts.getParameter(fullId(base)))
            param->setValueNotifyingHost(param->convertTo0to1(realValue));
    }

    void setNormalized(const juce::String& base, float norm)
    {
        if (auto* param = apvts.getParameter(fullId(base)))
            param->setValueNotifyingHost(norm);
    }

    juce::Point<float> nodePosition(const Node& n, juce::Rectangle<float> area) const
    {
        float x = freqToX(getReal(n.freqBase), area);
        float y = n.hasGain ? dbToY(getReal(n.gainBase), area) : area.getCentreY();
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

        if (getReal("LowCutOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, juce::jmax(20.0f, getReal("LowCutFreq")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        if (getReal("LowShelfOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, juce::jmax(20.0f, getReal("LowShelfFreq")),
                       juce::jmax(0.1f, getReal("LowShelfQ")), juce::Decibels::decibelsToGain(getReal("LowShelfGain")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        if (getReal("PeakOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, juce::jmax(20.0f, getReal("PeakFreq")),
                       juce::jmax(0.1f, getReal("PeakQ")), juce::Decibels::decibelsToGain(getReal("PeakGain")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        if (getReal("HighShelfOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, juce::jmax(20.0f, getReal("HighShelfFreq")),
                       juce::jmax(0.1f, getReal("HighShelfQ")), juce::Decibels::decibelsToGain(getReal("HighShelfGain")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        if (getReal("HighCutOn") > 0.5f)
            mag *= juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, juce::jmax(200.0f, getReal("HighCutFreq")))
                       ->getMagnitudeForFrequency((double) freq, sampleRate);

        return juce::Decibels::gainToDecibels((float) mag, -60.0f);
    }

    /** Curva do HPF (filtro fixo do detector, ~120Hz - mesmo valor usado no
        áudio, ver VocalCompressorDSP::highPassDetector) - sem nós, só liga/
        desliga via o parâmetro "hpf". */
    float hpfMagnitudeDb(float freq) const
    {
        auto* raw = apvts.getRawParameterValue("hpf");
        if (raw == nullptr || raw->load() <= 0.5f)
            return 0.0f;

        constexpr float hpfFreq = 120.0f;
        double mag = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpfFreq)
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
            float db = juce::jlimit(minDb, maxDb,
                selected == SelectedFilter::Hpf ? hpfMagnitudeDb(freq) : combinedMagnitudeDb(freq));
            float x = area.getX() + t * area.getWidth();
            float y = dbToY(db, area);

            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }

        auto curveColour = selected == SelectedFilter::Pre ? juce::Colour(0xffff9a3c)
                                                             : juce::Colour(0xffff5b5b);
        g.setColour(curveColour.withAlpha(0.35f));
        g.strokePath(curve, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(curveColour);
        g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawNodes(juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (selected == SelectedFilter::Hpf)
            return; // sem nós arrastáveis no modo HPF

        for (int i = 0; i < (int) nodes.size(); ++i)
        {
            auto& n = nodes[(size_t) i];
            auto p = nodePosition(n, area);
            bool isOn = n.onBase.isEmpty() || getReal(n.onBase) > 0.5f;
            float radius = (i == hoverIndex || i == draggingIndex) ? 8.0f : 6.0f;

            g.setColour(n.colour.withAlpha(isOn ? 0.85f : 0.30f));
            g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.drawEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);
        }
    }

    void drawTooltip(juce::Graphics& g, juce::Rectangle<float> area)
    {
        if (selected == SelectedFilter::Hpf)
            return;

        int idx = draggingIndex >= 0 ? draggingIndex : (hoverIndex >= 0 ? hoverIndex : -1);
        if (idx < 0)
            return;

        auto& n = nodes[(size_t) idx];
        auto p = nodePosition(n, area);

        juce::String text = formatFreq(getReal(n.freqBase));
        if (n.hasGain)
            text += "   " + juce::String(getReal(n.gainBase), 1) + " dB";
        if (n.hasQ)
            text += "   Q " + juce::String(getReal(n.qBase), 2);

        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
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

    /** Rótulo PRE EQ / HPF, sempre no canto superior esquerdo (ao lado
        do botão físico RESET, que é um componente filho de verdade -
        ver resetButton/resized()), indicando qual filtro está sendo
        mostrado/editado agora. */
    void drawHeader(juce::Graphics& g, juce::Rectangle<float> /*area*/)
    {
        auto curveColour = selected == SelectedFilter::Pre ? juce::Colour(0xffff9a3c)
                                                             : juce::Colour(0xffff5b5b);
        juce::String label = selected == SelectedFilter::Pre ? "PRE EQ" : "HPF";
        auto labelBounds = juce::Rectangle<float>(resetButton.getRight() + 6.0f, (float) resetButton.getY(),
                                                    70.0f, (float) resetButton.getHeight());
        g.setColour(curveColour);
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawFittedText(label, labelBounds.getSmallestIntegerContainer(),
                          juce::Justification::centredLeft, 1);
    }

    /** Reseta pro valor default SOMENTE o filtro atualmente selecionado
        (Pre, Post ou HPF) - nunca mexe nos outros. */
    void resetSelectedEQ()
    {
        if (selected == SelectedFilter::Hpf)
        {
            if (auto* param = apvts.getParameter("hpf"))
                param->setValueNotifyingHost(param->getDefaultValue());
            repaint();
            return;
        }

        static const char* bases[] = {
            "LowCutOn", "LowCutFreq",
            "LowShelfOn", "LowShelfFreq", "LowShelfGain", "LowShelfQ",
            "PeakOn", "PeakFreq", "PeakGain", "PeakQ",
            "HighShelfOn", "HighShelfFreq", "HighShelfGain", "HighShelfQ",
            "HighCutOn", "HighCutFreq"
        };

        for (auto* base : bases)
            if (auto* param = apvts.getParameter(fullId(base)))
                param->setValueNotifyingHost(param->getDefaultValue());

        repaint();
    }

    void timerCallback() override { repaint(); }

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate;
    std::vector<Node> nodes;
    SelectedFilter selected = SelectedFilter::Pre;
    int draggingIndex = -1;
    int hoverIndex = -1;
    HardwareButton resetButton { "RESET" };
};
