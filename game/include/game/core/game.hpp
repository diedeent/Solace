#pragma once

#include <engine/core/engine.hpp>
#include <game/entities/player.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/utils/math_utils.hpp>
#include <game/world/world_gen.hpp>
#include <engine/core/graphics_settings.hpp>
#include <memory>
#include <engine/graphics/camera.hpp>

namespace App {
    class Game : public Solace::Engine {
    public:
        Game();
        ~Game();

        void onUpdate(float deltaTime) override;
        void onRender() override;
        void onEvent(const SDL_Event& event) override;

    private:
        std::unique_ptr<Solace::Shader> m_worldShader;
        std::unique_ptr<App::WorldGen> m_worldGen;
        std::unique_ptr<Player> m_player;
        App::Camera m_camera;

        Solace::Mat4 m_viewMatrix;
        Solace::Mat4 m_projMatrix;

        float m_camX = 0.0f;
        float m_camY = 0.0f;
        float m_camZ = 0.0f;

        Solace::GraphicsSettings m_settings;

        CameraMode m_cameraMode = CameraMode::Default;

        void reloadSettings();
        void renderUI();
    };
}