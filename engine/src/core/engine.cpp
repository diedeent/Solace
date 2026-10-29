#include <engine/core/engine.hpp>
#include <engine/core/logger.hpp>
#include <engine/systems/input_system.hpp>
#include <SDL3/SDL.h>
#include <engine/debug/fps.hpp>

namespace Solace {
    Engine::Engine() {
        Logger::info("Initializing Solace Engine (Clean Core)...");
        m_window = std::make_unique<Window>("Solace", 1280, 720);
        m_post = std::make_unique<PostProcess>();

        if (m_window && m_window->getSDLWindow()) {
            SDL_Window* sdlWin = m_window->getSDLWindow();
            SDL_SetWindowRelativeMouseMode(sdlWin, true);
            InputSystem::init();
        }
    }

    void Engine::run() {
        Logger::info("Entering main game loop...");
        SDL_Event event;

        Uint64 lastCounter = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        while (m_isRunning) {
            Uint64 currentCounter = SDL_GetPerformanceCounter();
            float deltaTime = static_cast<float>(currentCounter - lastCounter) / static_cast<float>(frequency);
            lastCounter = currentCounter;

            if (deltaTime > 0.1f) deltaTime = 0.1f;

            InputSystem::update();

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) m_isRunning = false;

                onEvent(event);
                InputSystem::handleEvent(event);

                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    m_window->updateSize(event.window.data1, event.window.data2);
                }
            }

            if (InputSystem::isActionJustPressed(GameAction::ToggleFullscreen)) {
                m_window->toggleFullscreen();
            }

            if (m_window->shouldClose()) m_isRunning = false;

            Fps::update(deltaTime);
            onUpdate(deltaTime);

            m_post->beginScene(m_window->width(), m_window->height());
            onRender();
            m_post->endScene(m_window->width(), m_window->height());

            if (m_uiCallback) {
                m_uiCallback();
            }

            m_window->swapBuffers();
        }
    }

    Engine::~Engine() {
        Logger::info("Shutting down Solace Engine...");
    }
}