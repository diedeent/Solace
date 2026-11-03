#include <game/core/game.hpp>
#include <game/scenes/gameplay_scene.hpp>
#include <game/configs/player_config.hpp>

#include <engine/debug/logger.hpp>
#include <engine/core/graphics_settings.hpp>
#include <engine/debug/debug_overlay.hpp>
#include <engine/debug/fps.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <engine/systems/input_system.hpp>


namespace App {

    Game::Game() 
        : m_subs(event_bus()) 
    {
        SOLACE_INFO("Game instance created");
    }

    Game::~Game() {
        SOLACE_INFO("Game destroyed");
    }

    void Game::onInit() {

        config().register_section<Solace::GraphicsSettings>(
            "graphics", Solace::GraphicsSettings::parse);
        config().register_section<PlayerConfig>("player", PlayerConfig::parse);

        if (!config().load("assets/configs")) {
            SOLACE_ERROR("Game: failed to load configs, using defaults");
        }

        apply_graphics_settings();


        m_subs.subscribe<Solace::ConfigReloadedEvent>([this](const Solace::ConfigReloadedEvent& e) {
                if (e.success) {
                    SOLACE_INFO("Game: config reloaded, re-applying graphics");
                    apply_graphics_settings();
                }
                else {
                    SOLACE_WARN("Game: config reload failed — keeping previous settings");
                }
        });

        // 4. Стартовая сцена
        scenes().push_immediate(std::make_unique<GameplayScene>());
        SOLACE_INFO("Game initialized");
    }

    void Game::apply_graphics_settings() {
        const auto& gfx = config().get<Solace::GraphicsSettings>();
        
        Solace::PostSettings pp;
        pp.targetHeight = gfx.postProcess.targetHeight;
        pp.paletteBits = gfx.postProcess.paletteBits;
        pp.ditherAmount = gfx.postProcess.ditherAmount;
        pp.bloomStrength = gfx.postProcess.bloomStrength;
        pp.bloomThreshold = gfx.postProcess.bloomThreshold;
        getPostProcess()->applySettings(pp);
        
            SOLACE_INFO("Game: graphics applied (targetHeight=", pp.targetHeight,
                ", paletteBits=", pp.paletteBits, ")");
    }

    void Game::onEvent(const SDL_Event& e) {
        if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
            if (e.key.scancode == SDL_SCANCODE_F1) {
                Solace::DebugOverlay::toggle_panels();
            }
            if (e.key.scancode == SDL_SCANCODE_GRAVE) {
                Solace::DebugOverlay::toggle();
                SDL_SetWindowRelativeMouseMode(
                    getWindow().getSDLWindow(),
                    !Solace::DebugOverlay::is_open()
                );
            }
            if (e.key.scancode == SDL_SCANCODE_F5) {
                event_bus().emit(Solace::ConfigReloadRequestEvent{});
            }
        }
    }

    void Game::onShutdown() {
        // scenes() уже очищен в Engine::run перед onShutdown()
        // Здесь только глобальные ресурсы (если есть)
        SOLACE_INFO("Game shutdown");
    }

    void Game::onUpdate(float /*realDt*/, float /*gameplayDt*/) {
        // Сцены получают dt через scenes().update()
        // Тут — глобальные системы (F5, пауза и т.п.), если нужно
        /*if (Solace::InputSystem::isActionJustPressed(Solace::GameAction::ReloadConfig)) {
            // уже есть эмит из onEvent, но дублируем для надёжности
        }*/
    }

    void Game::onUIRender() {
        // ImGui NewFrame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        Solace::DebugOverlay::render();

        if (Solace::Fps::isVisible()) {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::Begin("Performance", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav);
            ImGui::Text("FPS: %.1f (%.2f ms)",
                Solace::Fps::getFPS(),
                1000.0f / (Solace::Fps::getFPS() > 0.0f ? Solace::Fps::getFPS() : 1.0f));
            ImGui::End();
        }
    }
}