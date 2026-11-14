#pragma once
#include <engine/core/entity_registry.hpp>

namespace App {
    class PlayerMovementSystem {
    public:
        static void update(Solace::EntityRegistry& registry, float deltaTime, bool isDevFreeFly);
    };
}