#pragma once
#include <cstdint>

namespace App {

    // Позиция и поворот в мире
    struct TransformComponent {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float yaw = 0.0f, pitch = 0.0f;
    };

    // Параметры движения, стамина, рывок
    struct MovementComponent {
        float speed = 5.0f;

        // Стамина и рывок
        float stamina = 100.0f;
        float maxStamina = 100.0f;
        float staminaRegenRate = 25.0f;

        bool  isDashing = false;
        float dashTimer = 0.0f;
        float dashDuration = 0.25f;
        float dashCooldown = 0.5f;
        float dashCooldownTimer = 0.0f;

        // --- Combat (MVP) ---
        float hp = 100.0f;
        float maxHp = 100.0f;
        float attackRange = 2.0f;
        float attackDamage = 20.0f;
        float attackCooldown = 0.5f;
        float attackTimer = 0.0f;
    };

    // Пустой тег, чтобы помечать именно Игрока (и отличать от NPC/врагов)
    struct PlayerTag {};

} // namespace App