#include <game/components/camera_component.hpp>
#include <cmath>

namespace App {
    void Camera::update(float deltaTime, CameraMode mode, float targetX, float targetY, float targetZ, float yaw, float pitch) {
        float yawRad = yaw * 3.14159265f / 180.0f;
        float pitchRad = pitch * 3.14159265f / 180.0f;

        float dirX = std::cos(yawRad) * std::cos(pitchRad);
        float dirY = std::sin(pitchRad);
        float dirZ = std::sin(yawRad) * std::cos(pitchRad);
        
        if (mode == CameraMode::Noclip) {
            m_camX = targetX; m_camY = targetY; m_camZ = targetZ;

            m_viewMatrix = Solace::mat4LookAt(
                m_camX, m_camY, m_camZ,
                targetX + dirX, targetY + dirY, targetZ + dirZ,
                0.0f, 1.0f, 0.0f
            );
        }
        else if (mode == CameraMode::ThirdPerson) {
            // Орбитальная точка — на уровне головы игрока
            const float orbitY = targetY + 1.5f;
            const float distance = 5.5f;

            // Камера позади игрока (противоположно forward)
            float desiredX = targetX - dirX * distance;
            float desiredY = orbitY - dirY * distance;
            float desiredZ = targetZ - dirZ * distance;

            // Не уходим под пол
            desiredY = std::max(desiredY, 0.7f);

            // Первый кадр — snap, дальше — экспоненциальное сглаживание
            const float smooth = m_initialized
                ? (1.0f - std::exp(-15.0f * deltaTime))
                : 1.0f;

            m_camX += (desiredX - m_camX) * smooth;
            m_camY += (desiredY - m_camY) * smooth;
            m_camZ += (desiredZ - m_camZ) * smooth;

            m_viewMatrix = Solace::mat4LookAt(
                m_camX, m_camY, m_camZ,
                targetX, orbitY, targetZ,
                0.0f, 1.0f, 0.0f
            );
        }
        else if (mode == CameraMode::Default) {
            float targetEyeX = targetX;
            float targetEyeY = targetY + 3.0f;
            float targetEyeZ = targetZ + 4.5f;

            float smoothSpeed = 5.0f * deltaTime;
            m_camX += (targetEyeX - m_camX) * smoothSpeed;
            m_camY += (targetEyeY - m_camY) * smoothSpeed;
            m_camZ += (targetEyeZ - m_camZ) * smoothSpeed;

            m_viewMatrix = Solace::mat4LookAt(
                m_camX, m_camY, m_camZ,
                targetX, targetY + 1.0f, targetZ,
                0.0f, 1.0f, 0.0f
            );
        }
        m_initialized = true;
    }
}