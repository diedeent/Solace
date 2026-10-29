#pragma once
#include <memory>
#include <functional>
#include <engine/core/window.hpp>
#include <engine/graphics/post_process.hpp>

#include <engine/core/clock.hpp>
#include <engine/core/event_bus.hpp>
#include <engine/core/entity_registry.hpp>

namespace Solace {
    class Engine {
    public:
        Engine();
        virtual ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void run();

        Solace::EventBus& event_bus() { return m_bus; }
        Solace::Clock& clock() { return m_clock; }
        Solace::EntityRegistry& registry() { return m_registry; }

        using RenderUICallback = std::function<void()>;
        void setUIRenderCallback(RenderUICallback callback) { m_uiCallback = callback; }

        Window& getWindow() { return *m_window; }
        PostProcess* getPostProcess() { return m_post.get(); }
        PostSettings& postSettings() { return m_post->settings(); }

    protected:
        virtual void onUpdate(float realDt, float gameplayDt) {}
        virtual void onRender() {}
        virtual void onEvent(const SDL_Event& event) {}

    private:
        Solace::EventBus m_bus; // Должен стоять первым, чтобы жить дольше
        Solace::Clock   m_clock;
        Solace::EntityRegistry  m_registry;

        std::unique_ptr<Window>      m_window;
        std::unique_ptr<PostProcess> m_post;
        RenderUICallback             m_uiCallback;
        bool m_isRunning = true;
    };
}