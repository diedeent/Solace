#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <engine/debug/fps.hpp>

#include <game/core/game.hpp>
#include <engine/core/logger.hpp>
#include <engine/debug/debug_overlay.hpp>
#include <glad/glad.h>
#include <engine/systems/input_system.hpp>
#include <cmath>
#include <engine/core/settings_manager.hpp>
#include <engine/graphics/camera.hpp>


namespace App {

    Game::Game() {
        Solace::Logger::info("Game instance successfully created!");

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontFromFileTTF("assets/fonts/JetBrainsMono-Regular.ttf", 16.0f);
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForOpenGL(getWindow().getSDLWindow(), getWindow().GetGLContext());
        ImGui_ImplOpenGL3_Init("#version 430 core");

        Solace::DebugOverlay::init();

        setUIRenderCallback([this]() { renderUI(); });
        Solace::DebugOverlay::registerCommand("fps", [](const std::vector<std::string>& args) {
            if (!args.empty()) {
                int val = std::stoi(args[0]);
                Solace::Fps::setVisible(val > 0);
                Solace::DebugOverlay::addLog(val > 0 ? "FPS counter enabled." : "FPS counter disabled.");
            }
            else {
                Solace::DebugOverlay::addLog(Solace::Fps::isVisible() ? "FPS counter is ON" : "FPS counter is OFF");
            }
            });

        Solace::DebugOverlay::registerCommand("mode", [this](const std::vector<std::string>& args) {
            if (!args.empty()) {
                if (args[0] == "noclip") {
                    m_cameraMode = CameraMode::Noclip;
                    Solace::Logger::info(">>> Switched to DEV MODE (Free-Fly Camera)");
                }
                else if (args[0] == "default") {
                    m_cameraMode = CameraMode::Default;
                    Solace::Logger::info(">>> Switched to GAMEPLAY 2.5D MODE (Fixed ARPG Camera)");
                }
            }
            });

        m_worldShader = std::make_unique<Solace::Shader>(
            "assets/shaders/core/base_vs.glsl",
            "assets/shaders/core/base_fs.glsl"
        );

        m_worldGen = std::make_unique<App::WorldGen>();

        m_settings = Solace::SettingsManager::load();

        Solace::PostSettings ppSettings;
        ppSettings.targetHeight = m_settings.postProcess.targetHeight;
        ppSettings.paletteBits = m_settings.postProcess.paletteBits;
        ppSettings.ditherAmount = m_settings.postProcess.ditherAmount;
        ppSettings.bloomStrength = m_settings.postProcess.bloomStrength;
        ppSettings.bloomThreshold = m_settings.postProcess.bloomThreshold;

        getPostProcess()->applySettings(ppSettings);

        float aspect = static_cast<float>(getWindow().width()) / static_cast<float>(getWindow().height());
        m_camera.setAspect(aspect);

        m_player = std::make_unique<Player>(0.0f, 0.0f, 0.0f);
    }

    Game::~Game() {
        Solace::DebugOverlay::shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        Solace::Logger::info("Game destroyed.");
    }

    void Game::reloadSettings() {
        Solace::Logger::info("Reloading graphics settings from JSON...");
        m_settings = Solace::SettingsManager::load();

        Solace::PostSettings ppSettings;
        ppSettings.targetHeight = m_settings.postProcess.targetHeight;
        ppSettings.paletteBits = m_settings.postProcess.paletteBits;
        ppSettings.ditherAmount = m_settings.postProcess.ditherAmount;
        ppSettings.bloomStrength = m_settings.postProcess.bloomStrength;
        ppSettings.bloomThreshold = m_settings.postProcess.bloomThreshold;

        getPostProcess()->applySettings(ppSettings);
        Solace::Logger::info("Graphics settings successfully reloaded!");
    }

    void Game::onUpdate(float realDt, float gameplayDt) {
        const bool* keyStates = SDL_GetKeyboardState(nullptr);
        static bool f5PressedLastFrame = false;
        if (keyStates[SDL_SCANCODE_F5] && !f5PressedLastFrame) {
            reloadSettings();
        }
        f5PressedLastFrame = keyStates[SDL_SCANCODE_F5];

        if (m_player) {
            m_player->update(gameplayDt, m_cameraMode);

            float px = m_player->getX();
            float py = m_player->getY();
            float pz = m_player->getZ();

            m_camera.update(gameplayDt, m_cameraMode, px, py, pz,
                m_player->getYaw(), m_player->getPitch());
        }
    }

    void Game::onRender() {
        glEnable(GL_DEPTH_TEST);
        //glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glClearColor(m_settings.clearColor[0], m_settings.clearColor[1], m_settings.clearColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Solace::Mat4 viewMatrix = m_camera.getViewMatrix();
        Solace::Mat4 projMatrix = m_camera.getProjMatrix();

        if (m_worldShader) {
            m_worldShader->bind();

            m_worldShader->setVec3("u_sunDir", m_settings.lighting.sunDir[0], m_settings.lighting.sunDir[1], m_settings.lighting.sunDir[2]);
            m_worldShader->setVec3("u_sunColor", m_settings.lighting.sunColor[0], m_settings.lighting.sunColor[1], m_settings.lighting.sunColor[2]);
            m_worldShader->setVec3("u_skyColor", m_settings.lighting.skyColor[0], m_settings.lighting.skyColor[1], m_settings.lighting.skyColor[2]);
            m_worldShader->setVec3("u_groundColor", m_settings.lighting.groundColor[0], m_settings.lighting.groundColor[1], m_settings.lighting.groundColor[2]);

            m_worldShader->setFloat("u_toonMidStep", m_settings.lighting.toonMidStep);
            m_worldShader->setFloat("u_toonShadowStep", m_settings.lighting.toonShadowStep);

            m_worldShader->setVec3("u_rimColor", m_settings.lighting.rimColor[0], m_settings.lighting.rimColor[1], m_settings.lighting.rimColor[2]);
            m_worldShader->setFloat("u_rimPower", m_settings.lighting.rimPower);

            if (m_player) {
                m_worldShader->setVec3("u_viewPos", m_player->getX(), m_player->getY() + 6.0f, m_player->getZ() + 8.0f);
            }

            m_worldShader->unbind();
        }

        if (m_worldGen && m_worldShader) {
            m_worldGen->render(*m_worldShader, viewMatrix, projMatrix);
        }
        else {
            Solace::Logger::error("Game: worldGen or shader is null!");
        }

        if (m_player && m_worldShader && m_cameraMode == CameraMode::Default) {
            m_player->render(*m_worldShader, viewMatrix, projMatrix);
        }

        static bool errLogged = false;
        GLenum err = glGetError();
        if (err != GL_NO_ERROR && !errLogged) {
            Solace::Logger::error("OpenGL error after render, code: ", static_cast<int>(err));
            errLogged = true;
        }
    }

    void Game::onEvent(const SDL_Event& event) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_GRAVE) {
            Solace::DebugOverlay::toggle();
            SDL_SetWindowRelativeMouseMode(
                getWindow().getSDLWindow(),
                !Solace::DebugOverlay::isOpen()
            );
        }
    }

    void Game::renderUI() {
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

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
}