#pragma once

#include <engine/graphics/model_asset.hpp>

#include <cstdint>
#include <memory>

namespace App {

    // ============================================================
    // RenderComponent — shared модель + per-instance SSBO.
    //
    // - model     : shared_ptr<ModelAsset> (1 экземпляр на 20 врагов)
    // - bone_ssbo : GLuint (per-instance, 64-500 матриц × 64 байт = ~16-32 KB)
    //
    // Phase 0.7: структура готова. Логика — в RenderSystem (Phase 1.1).
    // ============================================================
    struct RenderComponent {
        std::shared_ptr<Solace::ModelAsset> model;
        uint32_t bone_ssbo = 0;   // GLuint, но храним как uint32 (не тащим GL в header)
        bool     visible = true;
    };

} // namespace App