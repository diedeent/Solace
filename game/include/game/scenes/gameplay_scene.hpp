#pragma once

#include <engine/scene/scene.hpp>
#include <game/configs/player_config.hpp>

#include <game/entities/player.hpp>
#include <game/world/world_gen.hpp>

#include <memory>
#include <vector>

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

    private:
        App::CameraMode m_cameraMode = App::CameraMode::Default;
        // Scene resources (destroyed in on_exit)
        std::unique_ptr<Player>          m_player;
        std::unique_ptr<WorldGen>        m_worldGen;
        std::unique_ptr<Solace::Shader>  m_worldShader;

        /*
        std::shared_ptr<Solace::ModelAsset>                  m_model_asset;
        std::unique_ptr<Solace::AnimationStateMachine>       m_asm;
        Solace::EntityID                                     m_anim_entity = Solace::INVALID_ENTITY;
        */

        // View / proj (per-scene, не глобальные)
        Solace::Mat4 m_viewMatrix;
        Solace::Mat4 m_projMatrix;
    };

}