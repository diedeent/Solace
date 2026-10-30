#pragma once
#include <cstdint>

namespace App {

    // ============================================================
    // HealthComponent — HP, i-frames, смерть.
    //
    // i-frames: >0 → неуязвим (dash, parry). Decrement в HealthSystem.
    // is_dead: true → отложенное удаление через EntityDestroyedEvent.
    // ============================================================
    struct HealthComponent {
        float current = 100.0f;
        float maximum = 100.0f;
        float iframe_timer = 0.0f;
        bool  is_dead = false;

        float hp_fraction() const {
            return maximum > 0.0f ? current / maximum : 0.0f;
        }
    };

} // namespace App