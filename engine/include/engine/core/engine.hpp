#pragma once
#include <memory>
#include <functional>
#include <engine/core/window.hpp>
#include <engine/graphics/post_process.hpp>

namespace Solace {
    class Engine {
    public:
        Engine();
        virtual ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void run();

        virtual void onUpdate(float deltaTime) {}
        virtual void onRender() {}
        virtual void onEvent(const SDL_Event& event) {}

        using RenderUICallback = std::function<void()>;
        void setUIRenderCallback(RenderUICallback callback) { m_uiCallback = callback; }

        Window& getWindow() { return *m_window; }
        PostProcess* getPostProcess() { return m_post.get(); }
        PostSettings& postSettings() { return m_post->settings(); }

    private:
        std::unique_ptr<Window>      m_window;
        std::unique_ptr<PostProcess> m_post;
        RenderUICallback             m_uiCallback;
        bool m_isRunning = true;
    };
}