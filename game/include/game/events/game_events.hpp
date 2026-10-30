#pragma once

namespace App {

    // Событие получения урона (для боевой системы и звуков/VFX)
    struct DamageEvent {
        uint32_t targetEntityID;
        float damageAmount;
    };

    // Событие HitStop (остановка времени для импакта от ударов)
    struct HitStopEvent {
        float duration; // Длительность паузы в секундах
    };


} // namespace App