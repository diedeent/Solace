#pragma once

namespace App {

    // ============================================================
    // MovementComponent — скорость, dash, stamina.
    //
    // data-only. Логика — в PlayerMovementSystem (Phase 1.1).
    // ============================================================
    struct MovementComponent {
        // Постоянные параметры (из PlayerConfig)
        float speed = 5.0f;
        float dash_speed_mult = 3.5f;
        float dash_duration = 0.25f;
        float dash_cooldown = 0.5f;
        float dash_stamina_cost = 25.0f;
        float stamina_regen = 25.0f;
        float max_stamina = 100.0f;

        // Runtime state
        float stamina = 100.0f;
        bool  is_dashing = false;
        float dash_timer = 0.0f;
        float dash_cooldown_timer = 0.0f;
    };

} // namespace App