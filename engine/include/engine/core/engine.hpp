#pragma once

#include <memory>
#include <functional>

#include <engine/core/window.hpp>
#include <engine/core/clock.hpp>
#include <engine/core/event_bus.hpp>
#include <engine/core/config_cache.hpp>
#include <engine/core/entity_registry.hpp>
#include <engine/graphics/post_process.hpp>
#include <engine/scene/scene_manager.hpp>

namespace Solace {
    class Engine {
    public:
        Engine();
        virtual ~Engine();

        void run();

        // ---- Services ----
        EventBus& event_bus() { return m_bus; }
        Clock& clock() { return m_clock; }
        ConfigCache& config() { return m_config; }
        AssetCache& assets() { return m_assets; }
        EntityRegistry& registry() { return m_registry; }
        Window& getWindow() { return *m_window; }
        PostProcess* getPostProcess() { return m_post.get(); }
        PostSettings& postSettings() { return m_post->settings(); }

        // ---- Scenes ----
        SceneManager& scenes() { return *m_scenes; }

        using RenderUICallback = std::function<void()>;
        void setUIRenderCallback(RenderUICallback cb) { m_uiCallback = std::move(cb); }

    protected:
        // Хуки — вызываются Engine::run. Game переопределяет.
        virtual void onInit() {}
        virtual void onShutdown() {}
        virtual void onUpdate(float realDt, float gameplayDt) {}
        virtual void onRender() {}
        virtual void onEvent(const SDL_Event& event) {}
        virtual void onUIRender() {}

    private:
        EventBus        m_bus;         // 1
        Clock           m_clock;       // 2
        ConfigCache     m_config;      // 3
        AssetCache      m_assets;      // 4
        EntityRegistry  m_registry;    // 5

        std::unique_ptr<Window>         m_window;
        std::unique_ptr<PostProcess>    m_post;
        std::unique_ptr<SceneContext>   m_ctx;
        std::unique_ptr<SceneManager>   m_scenes;

        RenderUICallback m_uiCallback;
        bool             m_isRunning = true;
    };
}