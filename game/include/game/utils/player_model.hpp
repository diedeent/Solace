#pragma once

#include <engine/animations/animator.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/graphics/mesh.hpp>
#include <engine/utils/math_utils.hpp>
#include <memory>
#include <vector>
#include <string>

namespace App {

    class PlayerModel {
    public:
        PlayerModel();
        ~PlayerModel();

        void loadModel(const std::string& filePath);
        void render(Solace::Shader& shader, const Solace::Mat4& viewMatrix, const Solace::Mat4& projMatrix,
            float worldX, float worldY, float worldZ, float yaw);
        void update(float deltaTime);

        void setScale(float scale) { m_scale = scale; }
        float getScale() const { return m_scale; }
        void swapModel(const std::string& newFilePath);

        const ozz::animation::Skeleton& GetSkeleton() const;

    private:
        std::string m_currentModelPath;
        float m_scale;
        std::vector<std::unique_ptr<Solace::Mesh>> m_meshes;

        GLuint m_boneSSBO = 0;

        std::unique_ptr<Solace::Animator> m_animator;
    };

} // namespace App 