#include <engine/core/engine.hpp>
#include <engine/core/logger.hpp>
#include <engine/debug/debug_overlay.hpp>
#include <engine/systems/input_system.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <SDL3/SDL.h>

namespace Solace {

    Engine::Engine()
        : m_registry(m_bus)
    {
        Logger::info("Initializing Solace Engine...");
        m_window = std::make_unique<Window>("Solace", 1280, 720);
        m_post = std::make_unique<PostProcess>();

        m_ctx = std::make_unique<SceneContext>(
            m_bus, m_clock, m_config, m_registry, m_assets,
            *m_window, *m_post
        );
        m_scenes = std::make_unique<SceneManager>(*m_ctx);

        DebugOverlay::set_context(m_ctx.get(), m_scenes.get());

        // Config hot-reload
        m_bus.subscribe<ConfigReloadRequestEvent>([this](const ConfigReloadRequestEvent&) {
            const bool ok = m_config.reload();
            m_bus.emit(ConfigReloadedEvent{ ok });
            });

        if (m_window && m_window->getSDLWindow()) {
            SDL_Window* sdlWin = m_window->getSDLWindow();
            SDL_SetWindowRelativeMouseMode(sdlWin, true);
            InputSystem::init();

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf", 16.0f);
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            ImGui::StyleColorsDark();

            ImGui_ImplSDL3_InitForOpenGL(sdlWin, m_window->GetGLContext());
            ImGui_ImplOpenGL3_Init("#version 430 core");

            DebugOverlay::init();
        }

        onInit();  // ← scene initialization
    }

    void Engine::run() {
        Logger::info("Entering main loop...");
        SDL_Event event;

        while (m_isRunning) {
            m_clock.tick();
            m_registry.process_destroy();       // deferred destroy
            m_bus.processQueue();               // deferred events

            if (!DebugOverlay::is_open()) {
                InputSystem::update();
            }

            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) m_isRunning = false;

                onEvent(event);                 // Game::onEvent
                m_scenes->on_event(event);       // scene event

                ImGui_ImplSDL3_ProcessEvent(&event);
                if (!DebugOverlay::is_open()) {
                    InputSystem::handleEvent(event);
                }
                if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                    m_window->updateSize(event.window.data1, event.window.data2);
                }
            }

            if (InputSystem::isActionJustPressed(GameAction::ToggleFullscreen)) {
                m_window->toggleFullscreen();
            }
            if (m_window->shouldClose()) m_isRunning = false;

            onUpdate(m_clock.real_dt(), m_clock.gameplay_dt());  // Game::onUpdate
            m_scenes->update(m_clock.gameplay_dt(), m_clock.real_dt());
            m_scenes->process_transitions();

            m_post->beginScene(m_window->width(), m_window->height());
            onRender();                          // Game::onRender (optional)
            m_scenes->render();
            m_post->endScene(m_window->width(), m_window->height());

            onUIRender();                        // Game::onUIRender (ImGui frame)
            m_scenes->ui_render();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            m_clock.frame_end();
            m_window->swapBuffers();
        }

        onShutdown();
        m_scenes.reset();                        // ~SceneManager → on_exit всех
    }

    Engine::~Engine() {
        DebugOverlay::shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        Logger::info("Engine destroyed.");
    }
}