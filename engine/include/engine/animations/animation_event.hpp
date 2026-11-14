#pragma once

#include <engine/core/entity_registry.hpp>

#include <string>

namespace Solace {

    // ============================================================
    // AnimationEventTriggered — эмитится ASM при проходе через кадр события.
    //
    // Примеры: "swing" (замах), "hit" (hitbox active), "step" (пыль),
    //          "cast" (магия), "reload" (оружие).
    //
    // Подписчики:
    //   CombatSystem   → "hit" → hitbox.active = true
    //   AudioSystem    → "swing" / "step" → play sound
    //   VFXSystem      → "step" → spawn dust
    //   AnimationSystem → "end" → см. AnimationEndedEvent
    // ============================================================
    struct AnimationEventTriggered {
        EntityID    entity;
        std::string name;
    };

    struct AnimationEndedEvent {
        EntityID    entity;
        std::string clip_name;
    };

} // namespace Solace