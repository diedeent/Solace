#pragma once
#include <engine/utils/math_utils.hpp>

namespace App {
    enum class CameraMode { Default, Noclip };

    class Camera {
    public:
        void update(float deltaTime, CameraMode mode, float targetX, float targetY, float targetZ, float yaw, float pitch);

        Solace::Mat4 getViewMatrix() const { return m_viewMatrix; }
        Solace::Mat4 getProjMatrix() const { return m_projMatrix; }
        void setAspect(float aspect) {
            float fovRad = 45.0f * 3.14159265f / 180.0f;
            m_projMatrix = Solace::mat4Perspective(fovRad, aspect, 0.1f, 100.0f);
        }

    private:
        Solace::Mat4 m_viewMatrix;
        Solace::Mat4 m_projMatrix;
        float m_camX = 0.0f, m_camY = 0.0f, m_camZ = 0.0f;
    };
}