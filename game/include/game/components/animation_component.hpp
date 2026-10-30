#pragma once

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

#include <vector>

namespace App {

    // ============================================================
    // AnimationComponent — per-instance состояние анимации.
    //
    // Модель (skeleton, animations) — в ModelAsset (shared).
    // Здесь — только runtime state для ОДНОГО персонажа.
    //
    // Phase 0.7: структура готова. Логика — в AnimationSystem (Phase 1.1).
    // ============================================================
    struct AnimationComponent {
        int         current_clip = -1;     // индекс в ModelAsset::animations (по имени)
        float       current_time = 0.0f;
        float       fade_timer = 0.0f;
        float       fade_time = 0.2f;

        // Sampling state (per-instance, ozz требует свой context)
        ozz::animation::SamplingJob::Context sampling_context;

        // Local transforms (SOA) — after sampling
        std::vector<ozz::math::SoaTransform> locals;

        // Model matrices — after LocalToModel
        std::vector<ozz::math::Float4x4> models;

        // Skinning matrices — final, для SSBO
        std::vector<ozz::math::Float4x4> skinning_matrices;
    };

} // namespace App