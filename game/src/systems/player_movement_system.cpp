#include <game/systems/player_movement_system.hpp>
#include <engine/systems/input_system.hpp>
#include <cmath>
#include <algorithm>

namespace App {

    void PlayerMovementSystem::update(Player& player, float deltaTime, CameraMode camMode) {
        if (!player.m_isDashing && player.m_stamina < player.m_maxStamina) {
            player.m_stamina += player.m_staminaRegenRate * deltaTime;
            if (player.m_stamina > player.m_maxStamina) player.m_stamina = player.m_maxStamina;
        }

        if (player.m_dashCooldownTimer > 0.0f) player.m_dashCooldownTimer -= deltaTime;
        if (player.m_isDashing) {
            player.m_dashTimer -= deltaTime;
            if (player.m_dashTimer <= 0.0f) player.m_isDashing = false;
        }

        if (Solace::InputSystem::isActionJustPressed(Solace::GameAction::Dash)
            && player.m_stamina >= 25.0f && !player.m_isDashing && player.m_dashCooldownTimer <= 0.0f)
        {
            player.m_isDashing = true;
            player.m_dashTimer = player.m_dashDuration;
            player.m_stamina -= 25.0f;
            player.m_dashCooldownTimer = player.m_dashCooldown;
        }

        float moveX = Solace::InputSystem::getAxis(Solace::GameAxis::MoveX);
        float moveZ = Solace::InputSystem::getAxis(Solace::GameAxis::MoveY);

        float length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 1.0f) {
            moveX /= length;
            moveZ /= length;
        }

        float currentSpeed = player.m_speed;
        if (player.m_isDashing) {
            currentSpeed *= 3.5f;
        }

        if (camMode == CameraMode::Noclip) {
            float deltaX = Solace::InputSystem::getAxis(Solace::GameAxis::MouseDeltaX);
            float deltaY = Solace::InputSystem::getAxis(Solace::GameAxis::MouseDeltaY);
            float flyY = Solace::InputSystem::getAxis(Solace::GameAxis::FlyY);

            if (deltaX != 0.0f || deltaY != 0.0f) {
                player.m_yaw += deltaX * player.m_mouseSensitivity;
                player.m_pitch -= deltaY * player.m_mouseSensitivity;
                player.m_pitch = std::clamp(player.m_pitch, -89.0f, 89.0f);
            }

            float yawRad = player.m_yaw * 3.14159265f / 180.0f;
            float pitchRad = player.m_pitch * 3.14159265f / 180.0f;

            float forwardX = std::cos(yawRad) * std::cos(pitchRad);
            float forwardY = std::sin(pitchRad);
            float forwardZ = std::sin(yawRad) * std::cos(pitchRad);

            float rightX = -std::sin(yawRad);
            float rightZ = std::cos(yawRad);

            player.m_posX += (forwardX * moveZ + rightX * moveX) * currentSpeed * deltaTime;
            player.m_posY += (forwardY * moveZ + flyY) * currentSpeed * deltaTime;
            player.m_posZ += (forwardZ * moveZ + rightZ * moveX) * currentSpeed * deltaTime;
        }
        else {
            player.m_posX += moveX * currentSpeed * deltaTime;
            player.m_posZ -= moveZ * currentSpeed * deltaTime;
        }
    }

} // namespace App 