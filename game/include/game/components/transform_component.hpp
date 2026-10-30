#pragma once
#include <engine/utils/math_utils.hpp>

namespace App {

    // ============================================================
    // TransformComponent — позиция, поворот, масштаб в мире.
    // ============================================================
    struct TransformComponent {
        Solace::Vec3 position{ 0.0f, 0.0f, 0.0f };
        float yaw = 0.0f;   // вокруг Y, градусы
        float pitch = 0.0f;   // вокруг X, градусы
        Solace::Vec3 scale{ 1.0f, 1.0f, 1.0f };
    };

} // namespace App