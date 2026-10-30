#pragma once

#include <engine/animations/animation_clip.hpp>
#include <engine/animations/animation_event.hpp>
#include <engine/core/entity_registry.hpp>
#include <engine/core/event_bus.hpp>
#include <engine/graphics/model_asset.hpp>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace Solace {

    // ============================================================
    // AnimationStateMachine — управляет переходами между клипами.
    //
    // - play(name, fade): crossfade к новому клипу
    // - update(dt): sampling + blending + local_to_model + skinning
    // - events: эмитит AnimationEventTriggered при проходе кадра
    // - end: эмитит AnimationEndedEvent при завершении non-loop клипа
    //
    // Связь с ModelAsset:
    //   - clips хранятся в ModelAsset::animations
    //   - ASM ссылается на shared skeleton + animations
    //   - per-instance: sampling context, текущий клип, fade state
    //
    // Lifetime:
    //   - ASM создаётся в AnimationSystem / Scene при spawn entity
    //   - Живёт пока жива entity
    //   - Не владеет ModelAsset (shared_ptr держит ModelAsset)
    // ============================================================
    class AnimationStateMachine {
    public:
        AnimationStateMachine(
            EntityID entity,
            EventBus& bus,
            std::shared_ptr<ModelAsset> model_asset
        );
        ~AnimationStateMachine() = default;

        AnimationStateMachine(const AnimationStateMachine&) = delete;
        AnimationStateMachine& operator=(const AnimationStateMachine&) = delete;

        // Зарегистрировать клип (вызывается один раз при создании).
        void add_clip(const std::string& name, AnimationClip clip);

        // ---- Управление ----
        void play(const std::string& name, float fade_time = 0.2f);
        void play_immediate(const std::string& name);  // без fade (death)

        // ---- Update ----
        void update(float dt);

        // ---- Access ----
        const ozz::span<const ozz::math::Float4x4> get_skinning_matrices() const;
        const std::string& current_clip_name() const { return m_current_name; }
        bool is_playing(const std::string& name) const;
        bool is_finished() const;
        bool is_in_fade() const { return m_fade_timer > 0.0f; }
        float current_time() const { return m_current_time; }
        float fade_progress() const;  // 0 = previous, 1 = current

        // Для hitbox system (Phase 1.4)
        bool is_hitbox_active() const;

    private:
        void emit_events_between(float prev_time, float curr_time);
        void sample_clip(const AnimationClip& clip,
            float time,
            ozz::animation::SamplingJob::Context& ctx,
            std::vector<ozz::math::SoaTransform>& out_locals);
        void blend_and_skin();

        EntityID          m_entity;
        EventBus& m_bus;
        std::shared_ptr<ModelAsset> m_asset;

        // Clips
        std::unordered_map<std::string, AnimationClip> m_clips;
        const AnimationClip* m_current = nullptr;
        const AnimationClip* m_previous = nullptr;  // для crossfade

        // Per-instance state
        std::string m_current_name;
        float m_current_time = 0.0f;
        float m_previous_time = 0.0f;
        float m_fade_time = 0.0f;
        float m_fade_timer = 0.0f;

        // ozz sampling contexts (per-instance)
        ozz::animation::SamplingJob::Context m_ctx_a;  // current
        ozz::animation::SamplingJob::Context m_ctx_b;  // previous

        // Buffers (per-instance)
        std::vector<ozz::math::SoaTransform> m_locals_a;
        std::vector<ozz::math::SoaTransform> m_locals_b;
        std::vector<ozz::math::SoaTransform> m_blended;

        std::vector<ozz::math::Float4x4> m_models;
        std::vector<ozz::math::Float4x4> m_skinning_matrices;

        bool m_initialized = false;
    };

} // namespace Solace