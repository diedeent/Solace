#include <game/systems/player_movement_system.hpp>
#include <game/components/player_component.hpp>
#include <engine/systems/input_system.hpp>
#include <engine/debug/logger.hpp>
#include <cmath>
#include <algorithm>

namespace App {

    void PlayerMovementSystem::update(Solace::EntityRegistry& registry,
        float deltaTime, bool isDevFreeFly) {
        constexpr float SENS = 0.12f;

        // --- Mouse look ---
        const float dX = Solace::InputSystem::getAxis(Solace::GameAxis::MouseDeltaX);
        const float dY = Solace::InputSystem::getAxis(Solace::GameAxis::MouseDeltaY);

        // --- WASD ---
        float moveX = Solace::InputSystem::getAxis(Solace::GameAxis::MoveX);
        float moveZ = Solace::InputSystem::getAxis(Solace::GameAxis::MoveY);
        const float len = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (len > 1.0f) { moveX /= len; moveZ /= len; }

        const float flyY = Solace::InputSystem::getAxis(Solace::GameAxis::FlyY);

        registry.for_each<TransformComponent, MovementComponent, PlayerTag>(
            [&](Solace::EntityID, TransformComponent& t,
                MovementComponent& m, PlayerTag&)
            {
                // 1. Mouse look
                t.yaw += dX * SENS;
                t.pitch -= dY * SENS;
                t.pitch = std::clamp(t.pitch, -60.0f, 45.0f);

                // 2. Stamina + dash
                if (!m.isDashing && m.stamina < m.maxStamina)
                    m.stamina = std::min(m.maxStamina, m.stamina + m.staminaRegenRate * deltaTime);
                if (m.dashCooldownTimer > 0.0f) m.dashCooldownTimer -= deltaTime;
                if (m.isDashing) {
                    m.dashTimer -= deltaTime;
                    if (m.dashTimer <= 0.0f) m.isDashing = false;
                }
                if (Solace::InputSystem::isActionJustPressed(Solace::GameAction::Dash)
                    && m.stamina >= 25.0f && !m.isDashing && m.dashCooldownTimer <= 0.0f) {
                    m.isDashing = true;
                    m.dashTimer = m.dashDuration;
                    m.stamina -= 25.0f;
                    m.dashCooldownTimer = m.dashCooldown;
                }

                // 3. Движение (camera-relative, ЕДИНАЯ конвенция с camera.cpp)
                const float speed = m.speed * (m.isDashing ? 3.5f : 1.0f);
                const float yawRad = t.yaw * 3.14159265f / 180.0f;
                const float fwdX = std::cos(yawRad);
                const float fwdZ = std::sin(yawRad);
                const float rgtX = -std::sin(yawRad);
                const float rgtZ = std::cos(yawRad);

                if (isDevFreeFly) {
                    const float pitchRad = t.pitch * 3.14159265f / 180.0f;
                    const float pFwdY = std::sin(pitchRad);
                    const float pFwdMag = std::cos(pitchRad);

                    t.x += ((fwdX * pFwdMag * moveZ) + (rgtX * moveX)) * speed * deltaTime;
                    t.y += (pFwdY * moveZ + flyY) * speed * deltaTime;
                    t.z += ((fwdZ * pFwdMag * moveZ) + (rgtZ * moveX)) * speed * deltaTime;
                }
                else {
                    t.x += ((fwdX * moveZ) + (rgtX * moveX)) * speed * deltaTime;
                    t.z += ((fwdZ * moveZ) + (rgtZ * moveX)) * speed * deltaTime;
                    t.y = 0.0f;
                }
            });
    }
}