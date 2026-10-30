#include <game/scenes/gameplay_scene.hpp>

#include <engine/core/logger.hpp>
#include <engine/core/window.hpp>
#include <engine/graphics/post_process.hpp>

#include <glad/glad.h>
#include <engine/animations/animation_clip.hpp>
#include <engine/animations/animation_state_machine.hpp>

namespace App {

    GameplayScene::GameplayScene() = default;

    void GameplayScene::on_enter(Solace::SceneContext& ctx) {
        Solace::Logger::info("GameplayScene: enter");

        // Шейдер
        m_worldShader = std::make_unique<Solace::Shader>(
            "assets/shaders/core/base_vs.glsl",
            "assets/shaders/core/base_fs.glsl"
        );

        // Мир
        m_worldGen = std::make_unique<WorldGen>();

        // Игрок
        m_player = std::make_unique<Player>(0.0f, 0.0f, 0.0f);

        // Aspect ratio
        const float aspect = static_cast<float>(ctx.window.width()) /
            static_cast<float>(ctx.window.height());
        const float fovRad = 45.0f * 3.14159265f / 180.0f;
        m_projMatrix = Solace::mat4Perspective(fovRad, aspect, 0.1f, 100.0f);

        /*
        m_asm = std::make_unique<Solace::AnimationStateMachine>(
            entity_id, ctx.bus, model_asset
        );

        // Idle
        Solace::AnimationClip idle;
        idle.animation = &model_asset->animations.at("idle");
        idle.fps = 30.0f;
        idle.loop = true;
        m_asm->add_clip("idle", idle);

        // Run
        Solace::AnimationClip run;
        run.animation = &model_asset->animations.at("run");
        run.fps = 30.0f;
        run.loop = true;
        // Events: step на кадрах 4, 12, 20, 28
        run.events.push_back({ 4,  "step" });
        run.events.push_back({ 12, "step" });
        run.events.push_back({ 20, "step" });
        run.events.push_back({ 28, "step" });
        m_asm->add_clip("run", run);

        // Attack
        Solace::AnimationClip attack;
        attack.animation = &model_asset->animations.at("attack_1");
        attack.fps = 30.0f;
        attack.loop = false;
        attack.events.push_back({ 8,  "swing" });
        attack.events.push_back({ 14, "hit" });
        m_asm->add_clip("attack_1", attack);

        // Старт
        m_asm->play("idle");*/
    }

    void GameplayScene::on_exit(Solace::SceneContext& ctx) {
        Solace::Logger::info("GameplayScene: exit");

        // Явно освобождаем сцену-локальные ресурсы
        m_player.reset();
        m_worldGen.reset();
        m_worldShader.reset();

        // Очищаем все EntityID, созданные этой сценой
        // (Phase 0.5: Registry пока не используется сценой — но обязательно для Phase 1)
        // Если сцена создавала сущности — обнулить:
        // for (auto id : m_spawned_entities) ctx.registry.destroy(id);
        // ctx.registry.process_destroy();
    }

    void GameplayScene::on_event(const SDL_Event& event, Solace::SceneContext& ctx) {
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_F5) {
            // Уже эмитится из Game::onEvent
        }
    }

    void GameplayScene::on_update(float gameplayDt, float realDt, Solace::SceneContext& ctx) {
        // 1. Player
        if (m_player) {
            m_player->update(gameplayDt, m_cameraMode);
        }

        // 2. Камера
        if (m_player) {
            const float px = m_player->getX();
            const float py = m_player->getY();
            const float pz = m_player->getZ();

            // Camera::update переключается по режиму
            // (В текущем коде камера в Game — здесь inline)
            constexpr float cameraHeight = 6.0f;
            constexpr float cameraDistance = 8.0f;
            const float eyeX = px;
            const float eyeY = py + cameraHeight;
            const float eyeZ = pz + cameraDistance;
            m_viewMatrix = Solace::mat4LookAt(
                eyeX, eyeY, eyeZ,
                px, py + 1.0f, pz,
                0.0f, 1.0f, 0.0f
            );
        }
    }

    void GameplayScene::on_render(Solace::SceneContext& ctx) {
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);

        if (m_worldShader) {
            m_worldShader->bind();

            // Lighting (legacy, до Phase 0.7)
            // ... set uniforms ...

            m_worldShader->unbind();
        }

        if (m_worldGen && m_worldShader) {
            m_worldGen->render(*m_worldShader, m_viewMatrix, m_projMatrix);
        }

        if (m_player && m_worldShader) {
            m_player->render(*m_worldShader, m_viewMatrix, m_projMatrix);
        }
    }

}