#include <game/utils/player_model.hpp>

#include <engine/debug/logger.hpp>
#include <engine/assets/model_loader.hpp>

namespace App {

    void PlayerModel::loadModel(const std::string& filePath) {
        m_meshes.clear();

        try {
            auto meshesData = Solace::ModelLoader::loadFromFile(filePath);
            for (auto& data : meshesData) {
                m_meshes.push_back(std::make_unique<Solace::Mesh>(data));
            }
            SOLACE_INFO("Player model loaded: {}", filePath);
        }
        catch (const std::exception& e) {
            SOLACE_ERROR("Failed to load player model: {}", e.what());
        }
    }

    void PlayerModel::render(Solace::Shader& shader,
        const Solace::Mat4& viewMatrix,
        const Solace::Mat4& projMatrix,
        float worldX, float worldY, float worldZ, float yaw)
    {
        if (m_meshes.empty()) return;

        shader.bind();

        Solace::Mat4 translation = Solace::mat4Translate(worldX, worldY, worldZ);
        Solace::Mat4 scaleMat = Solace::mat4Scale(m_scale, m_scale, m_scale);
        Solace::Mat4 modelMatrix = Solace::mat4Multiply(translation, scaleMat);

        shader.setMat4("u_view", viewMatrix.data());
        shader.setMat4("u_proj", projMatrix.data());
        shader.setMat4("u_model", modelMatrix.data());

        for (const auto& mesh : m_meshes) {
            if (mesh->getTextureID() != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh->getTextureID());
                shader.setInt("u_textureDiffuse", 0);
                shader.setInt("u_hasTexture", 1);
                shader.setVec3("u_baseColor", 1.0f, 1.0f, 1.0f);
            }
            else {
                shader.setInt("u_hasTexture", 0);
                const float* col = mesh->getBaseColor();
                shader.setVec3("u_baseColor", col[0], col[1], col[2]);
            }
            mesh->draw();
        }

        shader.unbind();
    }

} // namespace App