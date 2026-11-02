#pragma once

#include <string>
#include <memory>
#include <vector>

#include <glad/glad.h>
#include <engine/graphics/mesh.hpp>
#include <engine/graphics/shader.hpp>
#include <engine/utils/math_utils.hpp>

namespace App {

    class PlayerModel {
    public:
        PlayerModel() = default;
        ~PlayerModel() = default;

        void loadModel(const std::string& filePath);
        void swapModel(const std::string& newFilePath) { loadModel(newFilePath); }
        void update(float deltaTime) {}
        void render(Solace::Shader& shader,
            const Solace::Mat4& viewMatrix,
            const Solace::Mat4& projMatrix,
            float worldX, float worldY, float worldZ, float yaw);

    private:
        std::vector<std::unique_ptr<Solace::Mesh>> m_meshes;
        float m_scale = 1.0f;
    };

} // namespace App