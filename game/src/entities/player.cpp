#include <game/entities/player.hpp>
#include <game/systems/player_movement_system.hpp>
#include <engine/debug/logger.hpp>

namespace App {

    Player::Player(float startX, float startY)
        : Player(startX, 0.0f, startY) {
    }

    Player::Player(float startX, float startY, float startZ)
        : m_posX(startX), m_posY(startY), m_posZ(startZ),
        m_speed(5.0f), m_yaw(-90.0f), m_pitch(0.0f), m_mouseSensitivity(0.15f),
        m_maxStamina(100.0f), m_stamina(100.0f), m_staminaRegenRate(25.0f),
        m_isDashing(false), m_dashTimer(0.0f), m_dashDuration(0.25f),
        m_dashCooldown(0.5f), m_dashCooldownTimer(0.0f),
        m_playerModel(std::make_unique<PlayerModel>())
    {
        SOLACE_INFO("Player spawned at X=", m_posX, ", Y=", m_posY, ", Z=", m_posZ);
        m_playerModel->loadModel("assets/models/runModel.glb");
    }

    void Player::update(float deltaTime, CameraMode camMode) {
        if (m_attackTimer > 0.0f) m_attackTimer -= deltaTime;

        if (m_playerModel) m_playerModel->update(deltaTime);
        //PlayerMovementSystem::update(*this, deltaTime, CameraMode camMode);
    }

    void Player::render(Solace::Shader& shader, const Solace::Mat4& viewMatrix, const Solace::Mat4& projMatrix) {
        if (m_playerModel) {
            m_playerModel->render(shader, viewMatrix, projMatrix, m_posX, m_posY, m_posZ, m_yaw);
        }
    }

    void Player::take_damage(float dmg) {
        if (!is_alive()) return;
        m_hp -= dmg;
        if (m_hp < 0.0f) m_hp = 0.0f;
        Solace::Logger::info("Player: took ", dmg, " dmg, HP=", m_hp);
    }

} // namespace App 