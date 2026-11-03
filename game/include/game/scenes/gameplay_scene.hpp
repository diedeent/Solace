#pragma once

#include <engine/scene/scene.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/assets/asset_cache.hpp> // Для m_playerAsset
#include <engine/animations/animation_state_machine.hpp>
#include <engine/utils/math_utils.hpp>

#include <game/world/world_gen.hpp>
#include <game/components/player_component.hpp>
#include <game/components/camera_component.hpp>
#include <game/entities/enemy.hpp>

#include <engine/core/graphics_settings.hpp>

#include <memory>

namespace App {

    class GameplayScene : public Solace::Scene {
    public:
        GameplayScene();
        ~GameplayScene() override = default;

        void on_enter(Solace::SceneContext& ctx) override;
        void on_exit(Solace::SceneContext& ctx) override;
        void on_update(float gameplayDt, float realDt, Solace::SceneContext& ctx) override;
        void on_render(Solace::SceneContext& ctx) override;
        void on_event(const SDL_Event& event, Solace::SceneContext& ctx) override;

        void on_ui_render(Solace::SceneContext& ctx) override;

    private:
        App::CameraMode m_cameraMode = App::CameraMode::ThirdPerson; // или Noclip

        std::unique_ptr<WorldGen>        m_worldGen;
        std::unique_ptr<Solace::Shader>  m_worldShader;

        Solace::EntityID m_playerEntity = Solace::INVALID_ENTITY;
        std::shared_ptr<Solace::ModelAsset> m_playerAsset;
        std::unique_ptr<Solace::AnimationStateMachine> m_playerAsm;

        Solace::Mat4 m_viewMatrix;
        Solace::Mat4 m_projMatrix;

        Solace::LightingSettings m_lighting;
        std::unique_ptr<Solace::ScopedSubscriptions> m_configSubs;

        App::Camera m_camera;
        std::vector<std::unique_ptr<App::Enemy>> m_enemies;




        std::vector<int> m_assimp_to_ozz_map;

        GLuint m_boneSsbo = 0;
    };

}