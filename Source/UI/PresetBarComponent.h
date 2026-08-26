#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "NFLookAndFeel.h"
#include "../Presets/PresetManager.h"

/** Barra de presets estilo hardware: "‹  Nome do Preset  ›  ☰", desenhada
    inteira num Component só (hit-test manual das 4 regiões), no mesmo
    estilo visual dos outros controles (corpo escuro, aro roxo, texto
    verde-limão quando ativo) - ver HardwareButton.h pro mesmo padrão.

    Não guarda nenhum estado de preset por conta própria - é só a "cara"
    do PresetManager (referência), que faz todo o trabalho de verdade
    (navegar, salvar, carregar). Escuta o ChangeBroadcaster dele pra saber
    quando redesenhar. */
class PresetBarComponent : public juce::Component, private juce::ChangeListener
{
public:
    explicit PresetBarComponent(PresetManager& managerIn) : manager(managerIn)
    {
        manager.addChangeListener(this);
    }

    ~PresetBarComponent() override
    {
        manager.removeChangeListener(this);
    }

    /** Disparado quando o usuário escolhe "Save Preset..." no menu - o
        editor é quem sabe mostrar o diálogo de nome (ver
        PresetNameDialog), essa classe não guarda referência pra ele. */
    std::function<void()> onSaveRequested;

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        float corner = 6.0f;

        // Sombra + corpo, mesmo padrão visual do HardwareButton (nunca
        // muda de cor no corpo, só o texto/detalhes reagem ao estado).
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.2f), corner);

        g.setColour(NFLookAndFeel::kPurple.withAlpha(0.5f));
        g.fillRoundedRectangle(bounds, corner);

        auto inner = bounds.reduced(1.1f);
        juce::ColourGradient bodyGrad(juce::Colour(0xff2a2a30), inner.getCentreX(), inner.getY(),
                                       juce::Colour(0xff121215), inner.getCentreX(), inner.getBottom(), false);
        g.setGradientFill(bodyGrad);
        g.fillRoundedRectangle(inner, corner * 0.85f);

        auto regions = computeRegions();

        auto drawChevron = [&g](juce::Rectangle<float> r, bool pointingLeft, bool enabled)
        {
            g.setColour(enabled ? NFLookAndFeel::kKnobGreen : NFLookAndFeel::kTextDim.withAlpha(0.4f));
            juce::Path p;
            float cx = r.getCentreX(), cy = r.getCentreY(), s = juce::jmin(r.getWidth(), r.getHeight()) * 0.28f;
            if (pointingLeft)
            {
                p.startNewSubPath(cx + s * 0.5f, cy - s);
                p.lineTo(cx - s * 0.5f, cy);
                p.lineTo(cx + s * 0.5f, cy + s);
            }
            else
            {
                p.startNewSubPath(cx - s * 0.5f, cy - s);
                p.lineTo(cx + s * 0.5f, cy);
                p.lineTo(cx - s * 0.5f, cy + s);
            }
            g.strokePath(p, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        };

        auto all = manager.getAllEntriesIncludingDefault();
        auto idx = all.indexOf(manager.getCurrentPresetName());
        bool hasPrev = idx > 0;
        bool hasNext = idx >= 0 && idx < all.size() - 1;

        drawChevron(regions.prev, true, hasPrev);
        drawChevron(regions.next, false, hasNext);

        // Nome do preset - verde-limão (mesmo tom dos indicadores) pra
        // ficar claramente "ativo/vivo", com "*" indicando alterações não
        // salvas desde o último load/save.
        juce::String text = manager.getCurrentPresetName();
        if (manager.isModified())
            text += " *";

        g.setColour(NFLookAndFeel::kKnobGreen);
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        g.drawFittedText(text, regions.name.getSmallestIntegerContainer(), juce::Justification::centred, 1);

        // Separador fino antes do menu.
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawVerticalLine(juce::roundToInt(regions.menu.getX() - 6.0f),
                            regions.menu.getY() + 4.0f, regions.menu.getBottom() - 4.0f);

        // Ícone hamburguer (3 linhas).
        g.setColour(NFLookAndFeel::kTextLight);
        float hx = regions.menu.getCentreX(), hy = regions.menu.getCentreY(), hw = regions.menu.getWidth() * 0.34f;
        for (int i = -1; i <= 1; ++i)
            g.drawHorizontalLine(juce::roundToInt(hy + (float) i * 5.0f), hx - hw, hx + hw);
    }

    void resized() override { repaint(); }

    void mouseDown(const juce::MouseEvent& e) override
    {
        auto regions = computeRegions();
        auto pos = e.position;

        if (regions.prev.contains(pos))
            manager.loadPrevious();
        else if (regions.next.contains(pos))
            manager.loadNext();
        else if (regions.menu.contains(pos))
            showMenu();
        else if (regions.name.contains(pos))
            showPresetPicker();
    }

private:
    struct Regions
    {
        juce::Rectangle<float> prev, name, next, menu;
    };

    Regions computeRegions() const
    {
        auto area = getLocalBounds().toFloat().reduced(3.0f);
        constexpr float arrowW = 32.0f, menuW = 34.0f, gap = 4.0f;

        Regions r;
        r.prev = area.removeFromLeft(arrowW);
        area.removeFromLeft(gap);
        r.menu = area.removeFromRight(menuW);
        area.removeFromRight(gap);
        r.next = area.removeFromRight(arrowW);
        area.removeFromRight(gap);
        r.name = area;
        return r;
    }

    /** Clicar no nome/meio da barra (não nas setas nem no menu) abre
        direto uma lista com TODOS os presets disponíveis (Default +
        salvos), com um check marcando o atual - atalho mais rápido que
        entrar em "menu -> Load Preset" pra quem só quer trocar de preset. */
    void showPresetPicker()
    {
        juce::PopupMenu menu;
        auto all = manager.getAllEntriesIncludingDefault();
        auto current = manager.getCurrentPresetName();

        for (int i = 0; i < all.size(); ++i)
            menu.addItem(i + 1, all[i], true, all[i] == current);

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                            [this, all](int result)
        {
            if (result >= 1 && result <= all.size())
                manager.loadByName(all[result - 1]);
        });
    }

    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Save Preset...");

        juce::PopupMenu loadMenu;
        auto names = manager.getPresetNames();
        if (names.isEmpty())
        {
            loadMenu.addItem(-1, "(nenhum preset salvo)", false);
        }
        else
        {
            int itemId = 100;
            for (auto& name : names)
                loadMenu.addItem(itemId++, name);
        }
        menu.addSubMenu("Load Preset", loadMenu);

        menu.addSeparator();
        menu.addItem(2, "Open Preset Folder");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
                            [this, names](int result)
        {
            if (result == 1)
            {
                if (onSaveRequested)
                    onSaveRequested();
            }
            else if (result == 2)
            {
                manager.getPresetsDirectory().revealToUser();
            }
            else if (result >= 100)
            {
                auto index = result - 100;
                if (index >= 0 && index < names.size())
                    manager.loadByName(names[index]);
            }
        });
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); }

    PresetManager& manager;
};
