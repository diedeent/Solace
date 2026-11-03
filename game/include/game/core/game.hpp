#pragma once

#include <engine/core/engine.hpp>
#include <engine/core/event_bus.hpp>

namespace App {

    class Game : public Solace::Engine {
    public:
        Game();
        ~Game();

    protected:
        void onInit() override;
        void onEvent(const SDL_Event& e) override;
        void onShutdown() override;
        void onUpdate(float realDt, float gameplayDt) override;
        void onUIRender() override;

    private:
        void renderUI();
        void reloadGraphicsSettings();
        void apply_graphics_settings();

        Solace::ScopedSubscriptions m_subs;
    };

}