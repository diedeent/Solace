#pragma once

#include <memory>
#include <string>
#include <game/utils/player_model.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/utils/math_utils.hpp>
#include <engine/graphics/camera.hpp>



namespace App {

    class PlayerMovementSystem;

    class Player {
    public:
        Player(float startX, float startY);
        Player(float startX, float startY, float startZ);
        ~Player() = default;

        void update(float deltaTime, CameraMode camMode);
        void render(Solace::Shader& shader, const Solace::Mat4& viewMatrix, const Solace::Mat4& projMatrix);

        void changeSkin(const std::string& modelPath) {
            if (m_playerModel) {
                m_playerModel->swapModel(modelPath);
            }
        }

        float getX() const { return m_posX; }
        float getY() const { return m_posY; }
        float getZ() const { return m_posZ; }
        float getYaw() const { return m_yaw; }
        float getPitch() const { return m_pitch; }

        // --- Combat ---
        float hp() const { return m_hp; }
        float max_hp() const { return m_maxHp; }
        bool is_alive() const { return m_hp > 0.0f; }
        float attack_range() const { return m_attackRange; }
        float attack_damage() const { return m_attackDamage; }
        bool can_attack() const { return m_attackTimer <= 0.0f && is_alive(); }
        void start_attack_cooldown() { m_attackTimer = m_attackCooldown; }
        void take_damage(float dmg);



    private:
        friend class PlayerMovementSystem;

        float m_posX, m_posY, m_posZ;
        float m_speed;
        float m_yaw, m_pitch;
        float m_mouseSensitivity;
        float m_stamina;
        float m_maxStamina;
        float m_staminaRegenRate;
        bool m_isDashing;
        float m_dashTimer;
        float m_dashDuration;
        float m_dashCooldown;
        float m_dashCooldownTimer;

        float m_hp = 100.0f;
        float m_maxHp = 100.0f;
        float m_attackRange = 2.0f;
        float m_attackDamage = 20.0f;
        float m_attackCooldown = 0.5f;
        float m_attackTimer = 0.0f;

        std::unique_ptr<PlayerModel> m_playerModel;
    };

} // namespace App 