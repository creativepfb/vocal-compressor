#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <functional>
#include "NFLookAndFeel.h"
#include "HardwareButton.h"
#include "../DSP/SpectrumAnalyzer.h"

/**
    EQ paramétrico gráfico: fica por CIMA do RTA (componente separado, não
    mexe nele). Mostra e edita o único EQ que existe (Pre, antes do
    compressor).

    Desenha a curva REAL (mesmos coeficientes IIR usados no áudio) e 5 nós
    arrastáveis: Low Cut - Peak - Peak - Peak - High Cut (as 3 bandas do
    meio ainda usam os nomes internos lowShelf/peak/highShelf, mas as três
    são filtros Peaking de verdade, com Q ajustável via scroll).
*/
class EQGraphComponent : public juce::Component, private juce::Timer
{
public:
    EQGraphComponent(juce::AudioProcessorValueTreeState& stateIn, double sampleRateIn,
                      const SpectrumAnalyzer& analyzerIn)
        : apvts(stateIn), sampleRate(sampleRateIn > 0.0 ? sampleRateIn : 44100.0), analyzer(analyzerIn)
    {
        for (auto& v : smoothedSpectrumDb)
            v = spectrumMinDb;

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

    void paint(juce::Graphics& g) override
    {
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        auto area = getLocalBounds().toFloat().reduced(2.0f);
        drawGrid(g, area);
        drawSpectrum(g, area);   // atrás da curva, de propósito
        drawCurve(g, area);
        drawAxisLabels(g, area); // por cima do espectro/curva, sempre legível
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

    // Tamanho dos "knobs" das bandas - bem maiores que antes (6/8px), fácil
    // de ver e clicar, como pedido.
    static constexpr float nodeRadius = 11.0f;
    static constexpr float nodeRadiusActive = 14.0f;

    // Espectro preenchido: range de dB próprio (não o mesmo eixo +-15dB da
    // curva de EQ) - o sinal de áudio real varia numa faixa bem mais ampla,
    // mapeado pra ocupar a altura do gráfico de forma proporcional.
    static constexpr int spectrumPoints = 200;
    static constexpr float spectrumMinDb = -70.0f;
    static constexpr float spectrumMaxDb = -6.0f;

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

    /** Estado do botão EQ ON/OFF (liga/desliga o EQ inteiro no áudio, ver
        PluginProcessor::processBlock) - usado pra desenhar a curva e os
        nós em cinza (aparência "desligada") quando ele estiver OFF. */
    bool isEqEnabled() const { return getReal("Enabled") > 0.5f; }

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
            if (pos.getDistanceFrom(nodePosition(nodes[(size_t) i], area)) < nodeRadiusActive + 4.0f)
                return i;
        return -1;
    }

    // Mesmo conjunto de frequências usado na grade E nos rótulos - listado
    // uma vez só, pra sempre bater um com o outro (igual EQs profissionais
    // tipo Waves F6: uma linha de grade PRA CADA rótulo, não um subconjunto).
    struct FreqTick { float freq; const char* text; };
    static constexpr FreqTick freqTicks[] = { { 31, "31" }, { 63, "63" }, { 125, "125" }, { 250, "250" },
                                                { 500, "500" }, { 1000, "1K" }, { 2000, "2K" }, { 4000, "4K" },
                                                { 8000, "8K" }, { 16000, "16K" } };
    struct DbTick { float db; const char* text; };
    static constexpr DbTick dbTicks[] = { { 12, "+12" }, { 6, "+6" }, { 0, "0" }, { -6, "-6" }, { -12, "-12" } };

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area)
    {
        // Uma linha vertical por frequência marcada, mesma densidade dos
        // rótulos - discretas, mas cobrindo a escala toda.
        g.setColour(juce::Colour(0x332a3430));
        for (auto& t : freqTicks)
            g.drawVerticalLine(juce::roundToInt(freqToX(t.freq, area)), area.getY(), area.getBottom());

        // Linhas horizontais de dB - 0dB mais visível (linha de referência).
        for (auto& t : dbTicks)
        {
            if (t.db == 0.0f || t.db < minDb || t.db > maxDb) continue;
            g.setColour(juce::Colour(0x262a3430));
            g.drawHorizontalLine(juce::roundToInt(dbToY(t.db, area)), area.getX(), area.getRight());
        }
        g.setColour(juce::Colour(0x662a3430));
        g.drawHorizontalLine(juce::roundToInt(dbToY(0.0f, area)), area.getX(), area.getRight());
    }

    /** Rótulos de frequência (embaixo) e dB (à esquerda), com uma pastilha
        escura atrás pra ficar bem legível mesmo em cima do espectro/curva -
        mesma densidade/legibilidade de um EQ profissional (ex: Waves F6). */
    void drawAxisLabels(juce::Graphics& g, juce::Rectangle<float> area)
    {
        g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));

        auto drawChip = [&g](juce::Rectangle<float> box, const juce::String& text, juce::Justification j)
        {
            g.setColour(juce::Colours::black.withAlpha(0.55f));
            g.fillRoundedRectangle(box, 2.5f);
            g.setColour(juce::Colour(0xffe4e4e8));
            g.drawFittedText(text, box.getSmallestIntegerContainer(), j, 1);
        };

        for (auto& l : freqTicks)
        {
            // Encosta as pontas (31 e 16K) pra dentro, senão a pastilha
            // fica cortada na borda do gráfico.
            float halfW = 14.0f;
            float x = juce::jlimit(area.getX() + halfW, area.getRight() - halfW, freqToX(l.freq, area));
            drawChip({ x - halfW, area.getBottom() - 14.0f, halfW * 2.0f, 12.0f }, l.text, juce::Justification::centred);
        }

        for (auto& l : dbTicks)
        {
            if (l.db < minDb || l.db > maxDb) continue;
            float y = dbToY(l.db, area);
            drawChip({ area.getX() + 2.0f, y - 6.0f, 22.0f, 12.0f }, l.text, juce::Justification::centredLeft);
        }
    }

    /** Suaviza um caminho de pontos com beziers quadráticas passando pelo
        ponto médio de cada segmento - visualmente contínuo, sem "quinas"
        entre amostras, custo desprezível (só geometria, nenhum cálculo de
        DSP aqui). */
    static juce::Path buildSmoothPath(const std::vector<juce::Point<float>>& pts)
    {
        juce::Path path;
        if (pts.empty())
            return path;

        path.startNewSubPath(pts.front());
        for (size_t i = 1; i < pts.size(); ++i)
        {
            auto mid = (pts[i - 1] + pts[i]) * 0.5f;
            path.quadraticTo(pts[i - 1], mid);
        }
        path.lineTo(pts.back());
        return path;
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

    juce::Path buildCurvePath(juce::Rectangle<float> area, const std::function<float(float)>& magnitudeFn) const
    {
        constexpr int steps = 300; // amostragem densa + suavização por bezier = curva bem lisa
        std::vector<juce::Point<float>> pts;
        pts.reserve(steps + 1);
        for (int i = 0; i <= steps; ++i)
        {
            float t = (float) i / (float) steps;
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            float db = juce::jlimit(minDb, maxDb, magnitudeFn(freq));
            float x = area.getX() + t * area.getWidth();
            float y = dbToY(db, area);
            pts.push_back({ x, y });
        }
        return buildSmoothPath(pts);
    }

    /** Curva do Pré EQ (único filtro que existe agora) - glow duplo pra dar
        destaque sobre o espectro/grade atrás dela. Quando o botão EQ
        ON/OFF está OFF (EQ bypassado no áudio de verdade), a curva fica cinza e
        discreta - visualmente "desligada", em vez de continuar laranja
        como se estivesse atuando. */
    void drawCurve(juce::Graphics& g, juce::Rectangle<float> area)
    {
        auto curve = buildCurvePath(area, [this](float f) { return combinedMagnitudeDb(f); });
        bool enabled = isEqEnabled();
        auto colour = enabled ? juce::Colour(0xffff9a3c) : juce::Colour(0xff707078);

        g.setColour(colour.withAlpha(enabled ? 0.18f : 0.09f));
        g.strokePath(curve, juce::PathStrokeType(7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(colour.withAlpha(enabled ? 0.35f : 0.16f));
        g.strokePath(curve, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(colour.withAlpha(enabled ? 1.0f : 0.55f));
        g.strokePath(curve, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    /** Atualiza o buffer suavizado do espectro (attack rápido, release mais
        lento - efeito "fluido" de analisador profissional) a partir dos
        bins já calculados pelo SpectrumAnalyzer (FFT no audio thread, ver
        DSP/SpectrumAnalyzer.h). Só leitura de atomics + um lerp por ponto -
        chamado 1x por frame do timer, nunca dentro de paint(). */
    void updateSpectrumSmoothing()
    {
        float binHz = (float) (sampleRate / (double) SpectrumAnalyzer::fftSize);

        for (int i = 0; i < spectrumPoints; ++i)
        {
            float t = (float) i / (float) (spectrumPoints - 1);
            float freq = minFreq * std::pow(maxFreq / minFreq, t);
            int bin = juce::jlimit(1, SpectrumAnalyzer::numBins - 1, (int) (freq / binHz));
            float raw = analyzer.magnitudesDb[(size_t) bin].load();

            float& smoothed = smoothedSpectrumDb[(size_t) i];
            float coeff = raw > smoothed ? 0.55f : 0.12f; // sobe rápido, desce suave
            smoothed += (raw - smoothed) * coeff;
        }
    }

    /** Espectro em tempo real como área preenchida translúcida, atrás da
        curva do EQ - substitui as barras verdes grosseiras de antes. Usa
        só os dados já suavizados em updateSpectrumSmoothing(), nenhum
        cálculo de FFT/DSP aqui dentro. */
    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> area)
    {
        std::vector<juce::Point<float>> pts;
        pts.reserve(spectrumPoints);
        for (int i = 0; i < spectrumPoints; ++i)
        {
            float t = (float) i / (float) (spectrumPoints - 1);
            float x = area.getX() + t * area.getWidth();
            float frac = juce::jlimit(0.0f, 1.0f,
                (smoothedSpectrumDb[(size_t) i] - spectrumMinDb) / (spectrumMaxDb - spectrumMinDb));
            float y = area.getBottom() - frac * area.getHeight();
            pts.push_back({ x, y });
        }

        auto line = buildSmoothPath(pts);

        juce::Path fill(line);
        fill.lineTo(area.getRight(), area.getBottom());
        fill.lineTo(area.getX(), area.getBottom());
        fill.closeSubPath();

        juce::ColourGradient gradient(NFLookAndFeel::kGreen.withAlpha(0.28f), 0, area.getY(),
                                       NFLookAndFeel::kGreen.withAlpha(0.02f), 0, area.getBottom(), false);
        g.setGradientFill(gradient);
        g.fillPath(fill);

        g.setColour(NFLookAndFeel::kGreen.withAlpha(0.55f));
        g.strokePath(line, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    /** Desenha os 5 nós arrastáveis. Quando o EQ ON/OFF está OFF, todos os
        nós (independente da cor original de cada banda) ficam cinza e
        mais apagados - mesma lógica "desligada" da curva, ver drawCurve(). */
    void drawNodes(juce::Graphics& g, juce::Rectangle<float> area)
    {
        bool enabled = isEqEnabled();

        for (int i = 0; i < (int) nodes.size(); ++i)
        {
            auto& n = nodes[(size_t) i];
            auto p = nodePosition(n, area);
            bool isOn = n.onBase.isEmpty() || getReal(n.onBase) > 0.5f;
            bool active = (i == hoverIndex || i == draggingIndex);
            float radius = active ? nodeRadiusActive : nodeRadius;
            float alpha = isOn ? 1.0f : 0.35f;
            if (!enabled)
                alpha *= 0.55f;

            juce::Colour colour = enabled ? n.colour : juce::Colour(0xff8c8c92);

            // Glow externo suave, só pra dar profundidade/destaque sobre a curva.
            g.setColour(colour.withAlpha(0.16f * alpha));
            g.fillEllipse(p.x - radius * 1.9f, p.y - radius * 1.9f, radius * 3.8f, radius * 3.8f);

            // Corpo do "knob" com leve gradiente radial (efeito de profundidade).
            juce::ColourGradient body(colour.brighter(0.35f).withAlpha(alpha),
                                       p.x - radius * 0.35f, p.y - radius * 0.4f,
                                       colour.darker(0.25f).withAlpha(alpha),
                                       p.x + radius * 0.5f, p.y + radius * 0.6f, true);
            g.setGradientFill(body);
            g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);

            // Anel escuro de contorno, define bem o ponto sobre o espectro/curva.
            g.setColour(juce::Colours::black.withAlpha(0.7f * alpha + 0.15f));
            g.drawEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f, active ? 2.2f : 1.6f);

            // Pequeno brilho no canto superior esquerdo, reforça o aspecto de knob 3D.
            g.setColour(juce::Colours::white.withAlpha(0.45f * alpha));
            float hl = radius * 0.42f;
            g.fillEllipse(p.x - radius * 0.42f, p.y - radius * 0.52f, hl, hl);
        }
    }

    void drawTooltip(juce::Graphics& g, juce::Rectangle<float> area)
    {
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

    /** Rótulo EQ ON/OFF, sempre no canto superior esquerdo (ao lado do
        botão físico RESET, que é um componente filho de verdade - ver
        resetButton/resized()). Cinza quando o EQ está OFF, mesma lógica da
        curva/nós. */
    void drawHeader(juce::Graphics& g, juce::Rectangle<float> /*area*/)
    {
        auto labelBounds = juce::Rectangle<float>(resetButton.getRight() + 6.0f, (float) resetButton.getY(),
                                                    90.0f, (float) resetButton.getHeight());
        g.setColour(isEqEnabled() ? juce::Colour(0xffff9a3c) : juce::Colour(0xff707078));
        g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        g.drawFittedText("EQ ON/OFF", labelBounds.getSmallestIntegerContainer(),
                          juce::Justification::centredLeft, 1);
    }

    /** Reseta pro valor default o Pré EQ (único filtro que existe agora). */
    void resetSelectedEQ()
    {
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

    void timerCallback() override
    {
        updateSpectrumSmoothing();
        repaint();
    }

    juce::AudioProcessorValueTreeState& apvts;
    double sampleRate;
    const SpectrumAnalyzer& analyzer;
    std::array<float, spectrumPoints> smoothedSpectrumDb;
    std::vector<Node> nodes;
    int draggingIndex = -1;
    int hoverIndex = -1;
    HardwareButton resetButton { "RESET" };
};
