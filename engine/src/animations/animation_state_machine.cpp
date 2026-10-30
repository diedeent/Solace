#include <engine/animations/animation_state_machine.hpp>
#include <engine/core/logger.hpp>

#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>

#include <algorithm>
#include <cmath>

namespace Solace {

    AnimationStateMachine::AnimationStateMachine(
        EntityID entity,
        EventBus& bus,
        std::shared_ptr<ModelAsset> model_asset)
        : m_entity(entity)
        , m_bus(bus)
        , m_asset(std::move(model_asset))
    {
        if (!m_asset || !m_asset->has_skeleton()) {
            Logger::error("ASM: model has no skeleton, animation disabled");
            return;
        }

        const size_t joints = m_asset->skeleton.num_joints();
        const size_t soa_joints = m_asset->skeleton.num_soa_joints();

        m_ctx_a.Resize(joints);
        m_ctx_b.Resize(joints);

        m_locals_a.resize(soa_joints);
        m_locals_b.resize(soa_joints);
        m_blended.resize(soa_joints);

        m_models.resize(joints);
        m_skinning_matrices.resize(joints);

        m_initialized = true;
    }

    void AnimationStateMachine::add_clip(const std::string& name, AnimationClip clip) {
        if (!m_initialized) return;
        clip.name = name;
        m_clips[name] = std::move(clip);
        Logger::info("ASM: clip added '", name, "' (",
            m_clips[name].duration(), "s, ", m_clips[name].events.size(), " events)");
    }

    void AnimationStateMachine::play(const std::string& name, float fade_time) {
        if (!m_initialized) return;

        auto it = m_clips.find(name);
        if (it == m_clips.end()) {
            Logger::error("ASM: clip not found '", name, "'");
            return;
        }

        // Если уже играем этот клип — не сбрасывать
        if (m_current && m_current->name == name && !m_current->loop) {
            return;
        }

        // Смена клипа с crossfade
        m_previous = m_current;
        m_previous_time = m_current_time;

        m_current = &it->second;
        m_current_name = name;
        m_current_time = 0.0f;
        m_fade_time = fade_time;
        m_fade_timer = fade_time;

        Logger::info("ASM[", m_entity, "]: play '", name,
            "' (fade ", fade_time, "s)");
    }

    void AnimationStateMachine::play_immediate(const std::string& name) {
        play(name, 0.0f);
    }

    bool AnimationStateMachine::is_playing(const std::string& name) const {
        return m_current_name == name;
    }

    bool AnimationStateMachine::is_finished() const {
        if (!m_current) return true;
        if (m_current->loop) return false;
        return m_current_time >= m_current->duration();
    }

    float AnimationStateMachine::fade_progress() const {
        if (m_fade_time <= 0.0f) return 1.0f;
        return 1.0f - (m_fade_timer / m_fade_time);
    }

    bool AnimationStateMachine::is_hitbox_active() const {
        if (!m_current) return false;
        const int frame = m_current->frame_at(m_current_time);
        for (const auto& ev : m_current->events) {
            if (ev.name == "hit" && ev.frame == frame) return true;
        }
        return false;
    }

    // ============================================================
    // Update
    // ============================================================

    void AnimationStateMachine::update(float dt) {
        if (!m_initialized || !m_current) return;

        // 1. Продвинуть время
        const float prev_time = m_current_time;
        m_current_time += dt;

        // 2. Loop или завершение
        if (m_current->loop) {
            if (m_current_time >= m_current->duration()) {
                m_current_time = std::fmod(m_current_time, m_current->duration());
            }
        }
        else if (m_current_time >= m_current->duration()) {
            m_current_time = m_current->duration();
            m_bus.queue(AnimationEndedEvent{ m_entity, m_current->name });
        }

        // 3. Emit frame events
        emit_events_between(prev_time, m_current_time);

        // 4. Fade timer
        if (m_fade_timer > 0.0f) {
            m_fade_timer -= dt;
            if (m_fade_timer <= 0.0f) {
                m_fade_timer = 0.0f;
                m_previous = nullptr;  // crossfade завершён
            }
        }

        // 5. Sampling
        sample_clip(*m_current, m_current_time, m_ctx_a, m_locals_a);

        if (m_previous && m_fade_timer > 0.0f) {
            m_previous_time += dt;
            if (m_previous->loop) {
                m_previous_time = std::fmod(m_previous_time, m_previous->duration());
            }
            else {
                m_previous_time = std::min(m_previous_time, m_previous->duration());
            }
            sample_clip(*m_previous, m_previous_time, m_ctx_b, m_locals_b);
        }

        // 6. Blending
        blend_and_skin();
    }

    void AnimationStateMachine::emit_events_between(float prev_time, float curr_time) {
        if (!m_current) return;

        const int prev_frame = m_current->frame_at(prev_time);
        const int curr_frame = m_current->frame_at(curr_time);

        // Обрабатываем проход через границу кадра (loop-safe)
        if (curr_frame < prev_frame) {
            // loop перескочил через 0
            for (const auto& ev : m_current->events) {
                if (ev.frame > prev_frame || ev.frame <= curr_frame) {
                    m_bus.queue(AnimationEventTriggered{ m_entity, ev.name });
                }
            }
        }
        else {
            for (const auto& ev : m_current->events) {
                if (ev.frame > prev_frame && ev.frame <= curr_frame) {
                    m_bus.queue(AnimationEventTriggered{ m_entity, ev.name });
                }
            }
        }
    }

    void AnimationStateMachine::sample_clip(
        const AnimationClip& clip,
        float time,
        ozz::animation::SamplingJob::Context& ctx,
        std::vector<ozz::math::SoaTransform>& out_locals)
    {
        if (!clip.animation) return;

        const float duration = clip.duration();
        const float ratio = duration > 0.0f
            ? time / duration
            : 0.0f;

        ozz::animation::SamplingJob job;
        job.animation = clip.animation;
        job.context = &ctx;
        job.ratio = std::clamp(ratio, 0.0f, 1.0f);
        job.output = ozz::make_span(out_locals);
        job.Run();
    }

    void AnimationStateMachine::blend_and_skin() {
        // Blending
        if (m_previous && m_fade_timer > 0.0f) {
            const float alpha = fade_progress();  // 0 → previous, 1 → current

            ozz::animation::BlendingJob::Layer layers[2];
            layers[0].transform = ozz::make_span(m_locals_b);  // previous
            layers[0].weight = 1.0f - alpha;
            layers[1].transform = ozz::make_span(m_locals_a);  // current
            layers[1].weight = alpha;

            ozz::animation::BlendingJob blend;
            blend.layers = ozz::make_span(layers);
            blend.rest_pose = m_asset->skeleton.joint_rest_poses();
            blend.output = ozz::make_span(m_blended);
            blend.threshold = 0.1f;
            if (!blend.Run()) {
                Logger::error("ASM: blending failed");
                return;
            }
        }
        else {
            // No fade — просто копируем current
            std::copy(m_locals_a.begin(), m_locals_a.end(), m_blended.begin());
        }

        // LocalToModel
        ozz::animation::LocalToModelJob ltm;
        ltm.skeleton = &m_asset->skeleton;
        ltm.input = ozz::make_span(m_blended);
        ltm.output = ozz::make_span(m_models);
        if (!ltm.Run()) {
            Logger::error("ASM: local_to_model failed");
            return;
        }

        // Skinning = Model * InverseBind
        for (size_t i = 0; i < m_models.size(); ++i) {
            m_skinning_matrices[i] = m_models[i] * m_asset->inverse_bind_poses[i];
        }
    }

    const ozz::span<const ozz::math::Float4x4>
        AnimationStateMachine::get_skinning_matrices() const {
        return ozz::make_span(m_skinning_matrices);
    }

} // namespace Solace