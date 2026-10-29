#pragma once
#include <game/entities/player.hpp>
#include <engine/graphics/camera.hpp>

namespace App {

    class PlayerMovementSystem {
    public:
        static void update(Player& player, float deltaTime, CameraMode camMode);
    };

} // namespace App 