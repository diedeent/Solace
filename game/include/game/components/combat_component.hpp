#pragma once
#include <engine/core/entity_registry.hpp>

namespace App {

    // ============================================================
    // CombatComponent — цель, атака, cooldowns.
    // ============================================================
    struct CombatComponent {
        Solace::EntityID target = Solace::INVALID_ENTITY;
        float            attack_cooldown = 0.0f;
        float            parry_window = 0.0f;
        bool             is_attacking = false;
        float            current_damage = 10.0f;
    };

} // namespace App