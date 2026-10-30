#include <game/core/game.hpp>
#include <game/scenes/gameplay_scene.hpp>
#include <game/configs/player_config.hpp>

#include <engine/core/logger.hpp>
#include <engine/core/settings_manager.hpp>
#include <engine/debug/debug_overlay.hpp>
#include <engine/debug/fps.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <engine/systems/input_system.hpp>


namespace App {

    Game::Game() {
        Solace::Logger::info("Game instance created");
    }

    Game::~Game() {
        Solace::Logger::info("Game destroyed");
    }

    void Game::onInit() {
        // 1. Загрузка graphics (legacy) и применение к PostProcess
        auto settings = Solace::SettingsManager::load();
        Solace::PostSettings pp;
        pp.targetHeight = settings.postProcess.targetHeight;
        pp.paletteBits = settings.postProcess.paletteBits;
        pp.ditherAmount = settings.postProcess.ditherAmount;
        pp.bloomStrength = settings.postProcess.bloomStrength;
        pp.bloomThreshold = settings.postProcess.bloomThreshold;
        getPostProcess()->applySettings(pp);

        // 2. Регистрация gameplay-секций ConfigCache
        config().register_section<PlayerConfig>("player", PlayerConfig::parse);
        if (!config().load("assets/configs")) {
            Solace::Logger::error("Game: failed to load gameplay configs, using defaults");
        }

        // 3. Регистрация консольных команд (упрощённо)
        Solace::DebugOverlay::register_command("fps", [](const std::vector<std::string>& args) {
            if (!args.empty()) {
                Solace::Fps::setVisible(std::stoi(args[0]) > 0);
            }
            });

        // 4. Стартовая сцена
        scenes().push_immediate(std::make_unique<GameplayScene>());

        Solace::Logger::info("Game initialized");
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
        Solace::Logger::info("Game shutdown");
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