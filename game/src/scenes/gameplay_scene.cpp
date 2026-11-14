#include <game/scenes/gameplay_scene.hpp>

#include <game/utils/mixamo_bone_utils.hpp>
#include <engine/debug/logger.hpp>
#include <engine/core/window.hpp>
#include <engine/graphics/post_process.hpp>
#include <engine/animations/animation_clip.hpp>
#include <game/systems/player_movement_system.hpp>

#include <glad/glad.h>
#include <engine/debug/debug_overlay.hpp>
#include <engine/debug/fps.hpp>
#include <engine/systems/input_system.hpp>
#include <imgui.h>



namespace App {

    static void upload_remapped_bones_to_ssbo(
        GLuint ssbo,
        const std::vector<int>& assimp_to_ozz_map,
        ozz::span<const ozz::math::Float4x4> matrices) // <-- Изменено на ozz::span
    {
        if (ssbo == 0 || assimp_to_ozz_map.empty() || matrices.empty()) {
            return;
        }

        std::vector<ozz::math::Float4x4> remapped(
            assimp_to_ozz_map.size(),
            ozz::math::Float4x4::identity());

        for (size_t i = 0; i < assimp_to_ozz_map.size(); ++i) {
            const int ozz_idx = assimp_to_ozz_map[i];
            if (ozz_idx >= 0 && ozz_idx < static_cast<int>(matrices.size())) {
                remapped[i] = matrices[ozz_idx];
            }
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferData(
            GL_SHADER_STORAGE_BUFFER,
            remapped.size() * sizeof(ozz::math::Float4x4),
            remapped.data(),
            GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    GameplayScene::GameplayScene() = default;

    void GameplayScene::on_enter(Solace::SceneContext& ctx) {
        SOLACE_INFO("GameplayScene: enter");

        m_lighting = ctx.config.get<Solace::GraphicsSettings>().lighting;
        SOLACE_INFO("GameplayScene: lighting loaded (rimPower=", m_lighting.rimPower,
            ", toonMid=", m_lighting.toonMidStep, ")");

        m_configSubs = std::make_unique<Solace::ScopedSubscriptions>(ctx.bus);
        m_configSubs->subscribe<Solace::ConfigReloadedEvent>(
            [this, &ctx](const Solace::ConfigReloadedEvent& e) {
                if (e.success) {
                    m_lighting = ctx.config.get<Solace::GraphicsSettings>().lighting;
                    SOLACE_INFO("GameplayScene: lighting reloaded (rimPower=",
                        m_lighting.rimPower, ")");
                }
            });

        // 1) World shader
        m_worldShader = std::make_unique<Solace::Shader>(
            "assets/shaders/core/base_vs.glsl",
            "assets/shaders/core/base_fs.glsl");
        if (!m_worldShader) {
            SOLACE_ERROR("Failed to create shader!");
        }

        // 2) SSBO for bones
        glGenBuffers(1, &m_boneSsbo);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSsbo);

        std::vector<float> identityData(128 * 16, 0.0f);
        for (size_t i = 0; i < 128; ++i) {
            identityData[i * 16 + 0] = 1.0f;
            identityData[i * 16 + 5] = 1.0f;
            identityData[i * 16 + 10] = 1.0f;
            identityData[i * 16 + 15] = 1.0f;
        }

        glBufferData(GL_SHADER_STORAGE_BUFFER, identityData.size() * sizeof(float),
            identityData.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // 3) World generation
        m_worldGen = std::make_unique<WorldGen>();

        // 4) Load model and animation data from the SAME rig
        const std::string model_path = "assets/models/Mannequin_F.glb";
        SOLACE_INFO("Loading Mixamo model: ", model_path);

        m_playerAsset = ctx.assets.get_model(model_path);

        if (!m_playerAsset) {
            SOLACE_ERROR("FAILED to load model!");
            return;
        }

        SOLACE_INFO("Model loaded. Meshes: ", m_playerAsset->meshes.size());

        // Загружаем .ozz файлы
        ctx.assets.load_skeleton_ozz(*m_playerAsset, "assets/animations/skeleton.ozz");
        ctx.assets.load_animation_ozz(*m_playerAsset, "idle", "assets/animations/CharacterArmature_Idle.ozz");
        ctx.assets.load_animation_ozz(*m_playerAsset, "run", "assets/animations/CharacterArmature_Walk.ozz");
        ctx.assets.load_animation_ozz(*m_playerAsset, "excited", "assets/animations/player_excited.ozz");

        // 5) Enemy (Использует ТУ ЖЕ модель!)
        auto enemy = std::make_unique<App::Enemy>(
            ctx.registry.create(), ctx.bus, 0.0f, 0.0f, -6.0f);
        enemy->setup_from_asset(m_playerAsset);
        m_enemies.push_back(std::move(enemy));

        // 6) Projection / camera
        const float aspect = static_cast<float>(ctx.window.width()) / static_cast<float>(ctx.window.height());
        const float fovRad = 45.0f * 3.14159265f / 180.0f;
        m_projMatrix = Solace::mat4Perspective(fovRad, aspect, 0.1f, 100.0f);
        m_camera.setAspect(aspect);

        // 7) Player entity
        m_playerEntity = ctx.registry.create();
        ctx.registry.add_component(m_playerEntity, App::TransformComponent{ 0.0f, 0.0f, 0.0f, 0.0f, 0.0f });
        ctx.registry.add_component(m_playerEntity, App::MovementComponent{});
        ctx.registry.add_component(m_playerEntity, App::PlayerTag{});

        if (!m_playerAsset->has_skeleton()) {
            SOLACE_WARN("Model has NO skeleton. ASM disabled.");
            return;
        }

        SOLACE_INFO("Skeleton found. Initializing ASM.");

        m_playerAsm = std::make_unique<Solace::AnimationStateMachine>(
            m_playerEntity, ctx.bus, m_playerAsset);

        const auto& assimp_names = m_playerAsset->bone_names;
        auto ozz_joint_names = m_playerAsset->skeleton.joint_names();

        SOLACE_INFO("Bone count: assimp=", assimp_names.size(), ", ozz=", ozz_joint_names.size());

        m_assimp_to_ozz_map = App::build_mixamo_bone_map(assimp_names, ozz_joint_names);

        if (!App::validate_bone_map(assimp_names, ozz_joint_names, m_assimp_to_ozz_map)) {
            SOLACE_ERROR("Bone mismatch detected.");
            //m_playerAsm.reset();
            //return;
        }

        int mapped_count = 0;
        for (int v : m_assimp_to_ozz_map) {
            if (v >= 0) ++mapped_count;
        }

        SOLACE_INFO("Bone mapping complete: ", mapped_count, "/", assimp_names.size(), " bones mapped.");

        if (m_playerAsset->animations.count("idle")) {
            Solace::AnimationClip idle;
            idle.animation = &m_playerAsset->animations.at("idle");
            idle.loop = true;
            m_playerAsm->add_clip("idle", idle);
        }

        if (m_playerAsset->animations.count("run")) {
            Solace::AnimationClip run;
            run.animation = &m_playerAsset->animations.at("run");
            run.loop = true;
            m_playerAsm->add_clip("run", run);
        }

        if (m_playerAsset->animations.count("excited")) {
            Solace::AnimationClip excited;
            excited.animation = &m_playerAsset->animations.at("excited");
            excited.loop = true;
            m_playerAsm->add_clip("excited", excited);
        }

        if (m_playerAsset->animations.count("idle")) {
            m_playerAsm->play("idle");
        }

        // Debug: print names only once
        for (size_t i = 0; i < std::min<size_t>(10, assimp_names.size()); ++i) {
            SOLACE_INFO("Assimp bone[", i, "] = ", assimp_names[i]);
        }
        for (size_t i = 0; i < std::min<size_t>(10, ozz_joint_names.size()); ++i) {
            SOLACE_INFO("OZZ joint[", i, "] = ", ozz_joint_names[i]);
        }

        while (glGetError() != GL_NO_ERROR) {}

        // Console commands
        Solace::DebugOverlay::register_command("noclip", [this](const std::vector<std::string>&) {
            m_cameraMode = (m_cameraMode == App::CameraMode::Default)
                ? App::CameraMode::Noclip
                : App::CameraMode::Default;
            Solace::DebugOverlay::add_log("Camera mode toggled: Noclip");
            });

        Solace::DebugOverlay::register_command("fps", [](const std::vector<std::string>& args) {
            if (!args.empty()) {
                Solace::Fps::setVisible(std::stoi(args[0]) > 0);
            }
            });

        Solace::DebugOverlay::register_command("tp", [this, &ctx](const std::vector<std::string>& args) {
            if (args.size() >= 3) {
                try {
                    auto* transform = ctx.registry.get_component<App::TransformComponent>(m_playerEntity);
                    if (transform) {
                        transform->x = std::stof(args[0]);
                        transform->y = std::stof(args[1]);
                        transform->z = std::stof(args[2]);
                        Solace::DebugOverlay::add_log("Teleported to: " + args[0] + ", " + args[1] + ", " + args[2]);
                    }
                }
                catch (...) {
                    Solace::DebugOverlay::add_log("Error: invalid numbers");
                }
            }
            else {
                Solace::DebugOverlay::add_log("Usage: tp <x> <y> <z>");
            }
            });
    }

    void GameplayScene::on_exit(Solace::SceneContext& ctx) {
        SOLACE_INFO("GameplayScene: exit");

        m_configSubs.reset();

        if (m_playerEntity != Solace::INVALID_ENTITY) {
            ctx.registry.destroy(m_playerEntity);
            ctx.registry.process_destroy();
            m_playerEntity = Solace::INVALID_ENTITY;
        }

        m_playerAsm.reset();
        m_playerAsset.reset();
        m_worldGen.reset();
        m_worldShader.reset();

        if (m_boneSsbo != 0) {
            glDeleteBuffers(1, &m_boneSsbo);
            m_boneSsbo = 0;
        }
    }

    void GameplayScene::on_update(float gameplayDt, float realDt, Solace::SceneContext& ctx) {
        bool isDevFreeFly = (m_cameraMode == App::CameraMode::Noclip);

        Solace::Fps::update(realDt);

        // Player movement
        App::PlayerMovementSystem::update(ctx.registry, gameplayDt, isDevFreeFly);

        if (m_playerAsm) {
            const float mv_x = Solace::InputSystem::getAxis(Solace::GameAxis::MoveX);
            const float mv_z = Solace::InputSystem::getAxis(Solace::GameAxis::MoveY);
            const bool is_moving = (mv_x * mv_x + mv_z * mv_z) > 0.01f;

            if (is_moving && m_playerAsm->is_playing("idle")) {
                m_playerAsm->play("run", 0.15f);
            }
            else if (!is_moving && m_playerAsm->is_playing("run")) {
                m_playerAsm->play("idle", 0.15f);
            }

            m_playerAsm->update(gameplayDt);

            auto matrices = m_playerAsm->get_skinning_matrices();
            if (!matrices.empty() && !m_assimp_to_ozz_map.empty()) {
                upload_remapped_bones_to_ssbo(m_boneSsbo, m_assimp_to_ozz_map, matrices);
            }
        }

        auto* tr = ctx.registry.get_component<App::TransformComponent>(m_playerEntity);
        if (tr) {
            m_camera.update(gameplayDt, m_cameraMode,
                tr->x, tr->y, tr->z, tr->yaw, tr->pitch);
            m_viewMatrix = m_camera.getViewMatrix();
        }

        auto* mv = ctx.registry.get_component<App::MovementComponent>(m_playerEntity);
        if (mv) {
            if (mv->attackTimer > 0.0f) mv->attackTimer -= gameplayDt;

            const bool want_attack =
                Solace::InputSystem::isActionJustPressed(Solace::GameAction::AttackPrimary);

            if (want_attack && mv->hp > 0.0f && mv->attackTimer <= 0.0f && tr) {
                for (auto& enemy : m_enemies) {
                    if (!enemy || !enemy->is_alive()) continue;
                    const float dx = enemy->x() - tr->x;
                    const float dz = enemy->z() - tr->z;
                    const float dist = std::sqrt(dx * dx + dz * dz);
                    if (dist <= mv->attackRange) {
                        enemy->take_damage(mv->attackDamage);
                    }
                }
                mv->attackTimer = mv->attackCooldown;
            }
        }

        if (mv && mv->hp > 0.0f && tr) {
            for (auto& enemy : m_enemies) {
                if (!enemy || !enemy->is_alive()) continue;
                const float dmg = enemy->update(gameplayDt, tr->x, tr->y, tr->z);
                if (dmg > 0.0f) {
                    mv->hp -= dmg;
                    if (mv->hp < 0.0f) mv->hp = 0.0f;
                    Solace::Logger::info("Player: took ", dmg, " dmg, HP=", mv->hp);
                }
            }
        }

        if (glGetError() != GL_NO_ERROR) {
            SOLACE_ERROR("on_update: GL error");
        }
    }

    void GameplayScene::on_event(const SDL_Event& event, Solace::SceneContext& ctx) {
        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            if (event.key.scancode == SDL_SCANCODE_V) {
                m_cameraMode = (m_cameraMode == App::CameraMode::Default)
                    ? App::CameraMode::Noclip
                    : App::CameraMode::Default;
            }
        }
    }

    void GameplayScene::on_render(Solace::SceneContext& ctx) {
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_boneSsbo);

        if (!m_worldShader) return;

        m_worldShader->bind();

        // Освещение — всё из graphics.json, единая точка правды.
        m_worldShader->setVec3("u_sunDir",
            m_lighting.sunDir[0], m_lighting.sunDir[1], m_lighting.sunDir[2]);
        m_worldShader->setVec3("u_sunColor",
            m_lighting.sunColor[0], m_lighting.sunColor[1], m_lighting.sunColor[2]);
        m_worldShader->setVec3("u_skyColor",
            m_lighting.skyColor[0], m_lighting.skyColor[1], m_lighting.skyColor[2]);
        m_worldShader->setVec3("u_groundColor",
            m_lighting.groundColor[0], m_lighting.groundColor[1], m_lighting.groundColor[2]);
        m_worldShader->setVec3("u_rimColor",
            m_lighting.rimColor[0], m_lighting.rimColor[1], m_lighting.rimColor[2]);
        m_worldShader->setFloat("u_toonMidStep", m_lighting.toonMidStep);
        m_worldShader->setFloat("u_toonShadowStep", m_lighting.toonShadowStep);
        m_worldShader->setFloat("u_rimPower", m_lighting.rimPower);

        // Позиция камеры — из App::Camera (m_camX/Y/Z), а не из столбца view-матрицы.
        m_worldShader->setVec3("u_viewPos",
            m_camera.getX(), m_camera.getY(), m_camera.getZ());
        m_worldShader->setMat4("u_view", m_viewMatrix.data());
        m_worldShader->setMat4("u_proj", m_projMatrix.data());

        if (m_worldGen) {
            m_worldGen->render(*m_worldShader, m_viewMatrix, m_projMatrix);
        }

        for (auto& enemy : m_enemies) {
            if (enemy && enemy->is_alive()) {
                enemy->render(*m_worldShader, m_viewMatrix, m_projMatrix);
            }
        }

        if (m_playerAsset && m_playerAsset->meshes.size() > 0) {
            if (auto* transform = ctx.registry.get_component<App::TransformComponent>(m_playerEntity)) {
                m_worldShader->bind();

                glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_boneSsbo);

                constexpr float MODEL_YAW_OFFSET = 1.5707963f;
                const float yawRad = transform->yaw * 3.14159265f / 180.0f;
                Solace::Mat4 model = Solace::mat4Multiply(
                    Solace::mat4Translate(transform->x, transform->y, transform->z),
                    Solace::Mat4(Solace::mat4RotateY(-yawRad + MODEL_YAW_OFFSET))); // <-- МИНУС ЗДЕСЬ
                m_worldShader->setMat4("u_model", model.data());

                for (auto& mesh : m_playerAsset->meshes) {
                    if (mesh->getTextureID() != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh->getTextureID());
                        m_worldShader->setInt("u_textureDiffuse", 0);
                        m_worldShader->setInt("u_hasTexture", 1);
                        m_worldShader->setVec3("u_baseColor", 1.0f, 1.0f, 1.0f);
                    }
                    else {
                        m_worldShader->setInt("u_hasTexture", 0);
                        m_worldShader->setVec3("u_baseColor", 0.8f, 0.6f, 0.4f);
                    }
                    mesh->draw();
                }
                m_worldShader->unbind();
            }
        }

        m_worldShader->unbind();
    }

    void GameplayScene::on_ui_render(Solace::SceneContext& ctx) {
        ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
        ImGui::Begin("Gameplay", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

        if (auto* mv = ctx.registry.get_component<App::MovementComponent>(m_playerEntity)) {
            const float frac = mv->maxHp > 0.0f ? mv->hp / mv->maxHp : 0.0f;
            ImGui::Text("Player HP: %.0f / %.0f", mv->hp, mv->maxHp);
            ImGui::ProgressBar(frac, ImVec2(220, 0), frac > 0.3f ? "" : "LOW");
        }

        for (size_t i = 0; i < m_enemies.size(); ++i) {
            auto& e = m_enemies[i];
            if (!e) continue;
            const float frac = e->max_hp() > 0.0f ? e->hp() / e->max_hp() : 0.0f;
            ImGui::Text("Enemy #%zu HP: %.0f / %.0f", i, e->hp(), e->max_hp());
            ImGui::ProgressBar(frac, ImVec2(220, 0));
        }

        ImGui::End();
    }

} // namespace App