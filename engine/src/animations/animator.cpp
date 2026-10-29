#include <engine/animations/animator.hpp>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <engine/core/logger.hpp>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace Solace {

    bool Animator::loadSkeleton(const std::string& filename) {
        ozz::io::File file(filename.c_str(), "rb");
        if (!file.opened()) {
            Logger::error("Animator: FAILED to open skeleton file: ", filename);
            return false;
        }
        ozz::io::IArchive archive(&file);
        if (!archive.TestTag<ozz::animation::Skeleton>()) {
            Logger::error("Animator: file is not a valid skeleton: ", filename);
            return false;
        }
        archive >> m_skeleton;
        m_samplingContext.Resize(m_skeleton.num_joints());

        std::vector<ozz::math::SoaTransform> restLocals(m_skeleton.num_soa_joints());
        std::memcpy(restLocals.data(), m_skeleton.joint_rest_poses().data(),
            restLocals.size() * sizeof(ozz::math::SoaTransform));

        std::vector<ozz::math::Float4x4> restModels(m_skeleton.num_joints());
        ozz::animation::LocalToModelJob ltm;
        ltm.skeleton = &m_skeleton;
        ltm.input = ozz::make_span(restLocals);
        ltm.output = ozz::make_span(restModels);
        if (!ltm.Run()) {
            Logger::error("Animator: failed to compute rest pose");
            return false;
        }

        m_inverseBindPoses.resize(m_skeleton.num_joints());
        for (size_t i = 0; i < m_inverseBindPoses.size(); ++i) {
            m_inverseBindPoses[i] = ozz::math::Invert(restModels[i]);
        }

        Logger::info("Animator: skeleton loaded OK. Joints: ", m_skeleton.num_joints());
        return true;
    }

    bool Animator::loadAnimation(const std::string& filename) {
        ozz::io::File file(filename.c_str(), "rb");
        if (!file.opened()) {
            Logger::error("Animator: FAILED to open animation file: ", filename);
            return false;
        }
        ozz::io::IArchive archive(&file);
        if (!archive.TestTag<ozz::animation::Animation>()) {
            Logger::error("Animator: file is not a valid animation: ", filename);
            return false;
        }
        archive >> m_animation;
        m_duration = m_animation.duration();
        m_locals.resize(m_skeleton.num_soa_joints());
        m_models.resize(m_skeleton.num_joints());
        m_skinningMatrices.resize(m_skeleton.num_joints());
        Logger::info("Animator: animation loaded OK. Duration: ", m_duration, "s");
        return true;
    }

    void Animator::update(float deltaTime) {
        if (m_duration <= 0.0f) return;

        m_time += deltaTime;
        if (m_loop) m_time = std::fmod(m_time, m_duration);
        else m_time = std::min(m_time, m_duration);

        float ratio = m_time / m_duration;

        ozz::animation::SamplingJob sampling_job;
        sampling_job.animation = &m_animation;
        sampling_job.context = &m_samplingContext;
        sampling_job.ratio = ratio;
        sampling_job.output = ozz::make_span(m_locals);
        if (!sampling_job.Run()) return;

        // 2. Local -> Model
        ozz::animation::LocalToModelJob ltm_job;
        ltm_job.skeleton = &m_skeleton;
        ltm_job.input = ozz::make_span(m_locals);
        ltm_job.output = ozz::make_span(m_models);
        if (!ltm_job.Run()) {
            Logger::error("ozz: LocalToModel failed!");
            return;
        }

        for (size_t i = 0; i < m_models.size(); ++i) {
            m_skinningMatrices[i] = m_models[i] * m_inverseBindPoses[i];
        }
    }
}