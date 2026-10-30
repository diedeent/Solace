#pragma once

#include <engine/graphics/mesh.hpp>

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/simd_math.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Solace {

    // ============================================================
    // ModelAsset — shared, immutable ресурс модели.
    //
    // Живёт в AssetCache. Один экземпляр на путь. Все сущности,
    // использующие одну модель, делят её (20 врагов = 1 ModelAsset).
    //
    // Содержит:
    //   - meshes            : геометрия (VAO/VBO/EBO на GPU)
    //   - skeleton          : скелет (bind pose, иерархия)
    //   - inverse_bind_pose : для skinning
    //   - animations        : все клипы по имени
    //   - bone_names        : для маппинга Assimp → ozz (Phase 1)
    //
    // НЕ содержит:
    //   - per-instance state (current_time, sampling_context)
    //   - SSBO (per-instance GPU буфер матриц костей)
    // ============================================================
    class ModelAsset {
    public:
        std::string path;

        std::vector<std::unique_ptr<Mesh>> meshes;

        // Skeleton (shared)
        ozz::animation::Skeleton skeleton;
        std::vector<ozz::math::Float4x4> inverse_bind_poses;

        // Animations: name → runtime animation
        std::unordered_map<std::string, ozz::animation::Animation> animations;

        // Bone names в порядке ozz (для маппинга)
        std::vector<std::string> bone_names;

        bool has_skeleton() const {
            return skeleton.num_joints() > 0;
        }

        // Debug
        size_t total_vertices() const;
    };

} // namespace Solace