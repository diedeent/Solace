#pragma once

namespace App {

    // ============================================================
    // Tag-компоненты — нулевого размера, только для запросов.
    //
    // Пример: registry.for_each<PlayerTag, Health>(...)
    // ============================================================
    struct PlayerTag {};
    struct EnemyTag {};

} // namespace App