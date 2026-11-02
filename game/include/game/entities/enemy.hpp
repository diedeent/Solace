#pragma once
#include <engine/graphics/mesh.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/graphics/model_asset.hpp>
#include <engine/animations/animation_state_machine.hpp>
#include <engine/core/event_bus.hpp>
#include <engine/core/entity_registry.hpp>
#include <engine/utils/math_utils.hpp>
#include <glad/glad.h>
#include <memory>
#include <string>
#include <vector>

namespace App {
    enum class EnemyState { Idle, Chase, Windup, Cooldown, Dead };

    class Enemy {
    public:
        Enemy(Solace::EntityID animId, Solace::EventBus& bus, float x, float y, float z);
        ~Enemy();

        // Использует уже загруженный ModelAsset (общий с игроком — экономия VRAM)
        void setup_from_asset(const std::shared_ptr<Solace::ModelAsset>& asset);

        float update(float dt, float px, float py, float pz);
        void render(Solace::Shader&, const Solace::Mat4&, const Solace::Mat4&);
        void take_damage(float dmg);

        float x() const { return m_posX; }
        float y() const { return m_posY; }
        float z() const { return m_posZ; }
        float hp() const { return m_hp; }
        float max_hp() const { return m_maxHp; }
        float attack_range() const { return m_attackRange; }
        bool is_alive() const { return m_state != EnemyState::Dead; }

    private:
        Solace::EntityID m_animId;
        Solace::EventBus& m_bus;
        std::shared_ptr<Solace::ModelAsset> m_asset;
        std::unique_ptr<Solace::AnimationStateMachine> m_asm;
        std::vector<int> m_boneMap;
        GLuint m_boneSsbo = 0;

        float m_posX, m_posY, m_posZ;
        float m_yaw = 0.0f;
        float m_speed = 2.5f;
        float m_hp = 60.0f, m_maxHp = 60.0f;
        float m_attackRange = 1.6f;
        float m_attackDamage = 10.0f;
        float m_attackWindup = 0.35f;
        float m_attackCooldown = 1.2f;
        EnemyState m_state = EnemyState::Idle;
        float m_windupTimer = 0.0f;
        float m_cooldownTimer = 0.0f;
        float m_hitFlashTimer = 0.0f;
    };
}