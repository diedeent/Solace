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

    // Запрос на перезагрузку конфигов (вызывается по F5)
    struct ConfigReloadRequestEvent {
        // не требует полей, просто триггер
    };

    // Уведомление о том, что конфиги успешно перезагружены
    struct ConfigReloadedEvent {
        // Системы подписываются, чтобы обновить свои локальные кэши
    };

} // namespace App