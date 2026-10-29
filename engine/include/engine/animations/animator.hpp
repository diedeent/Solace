#pragma once
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/span.h>
// Р”РћР‘РђР’Р›РЇР•Рњ РЇР’РќР«Р• Р—РђР“РћР›РћР’РљР OZZ Р”Р›РЇ SIMD Р РњРђРўР РР¦:
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

#include <vector>
#include <string>

namespace Solace {
    class Animator {
    public:
        Animator() = default;
        ~Animator() = default;

        bool loadSkeleton(const std::string& filename);
        bool loadAnimation(const std::string& filename);
        void update(float deltaTime);

        const ozz::animation::Skeleton& GetSkeleton() const { return m_skeleton; }

        const ozz::span<const ozz::math::Float4x4> getBoneMatrices() const {
            return ozz::make_span(m_skinningMatrices);
        }

        void setLoop(bool loop) { m_loop = loop; }

    private:
        ozz::animation::Skeleton m_skeleton;
        ozz::animation::Animation m_animation;
        ozz::animation::SamplingJob::Context m_samplingContext;

        std::vector<ozz::math::SoaTransform> m_locals;
        std::vector<ozz::math::Float4x4> m_models;
        std::vector<ozz::math::Float4x4> m_inverseBindPoses;
        std::vector<ozz::math::Float4x4> m_skinningMatrices;

        float m_time = 0.0f;
        float m_duration = 0.0f;
        bool m_loop = true;
    };
}