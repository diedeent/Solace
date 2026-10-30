#include <game/world/world_gen.hpp>
#include <engine/assets/model_loader.hpp>
#include <engine/core/logger.hpp>
#include <cmath>
#include <stdexcept>

namespace App {

    WorldGen::WorldGen() {
        Solace::Logger::info("WorldGen: initialized with default structure positions.");
        loadAssets();
    }

    void WorldGen::loadAssets() {
        try {
            std::vector<Solace::MeshData> modelParts = Solace::ModelLoader::loadFromFile("assets/models/kobold_setter-female.glb");

            for (auto& part : modelParts) {
                for (auto& v : part.vertices) {
                    for (int i = 0; i < 4; i++) {
                        v.boneWeights[i] = 0.0f;
                        v.boneIDs[i] = 0;
                    }
                }
            }

            if (!modelParts.empty()) {
                m_monolith = std::make_unique<Solace::Mesh>(modelParts[0]);
                Solace::Logger::info("WorldGen: Monolith model successfully created from GLB.");
            }

        }
        catch (const std::exception& e) {
            Solace::Logger::error("Failed to load 3D model: ", e.what());

            m_monolith = std::make_unique<Solace::Mesh>(buildPillarData(0.6f, 3.0f, 4));
        }

        m_pillar = std::make_unique<Solace::Mesh>(buildPillarData(0.5f, 4.0f, 8));

        Solace::Logger::info("WorldGen: Pillar created. Valid: ", m_pillar->valid());
    }

    Solace::MeshData WorldGen::buildPillarData(float radius, float height, int segments) {
        Solace::MeshData data;
        const float pi2 = 6.2831853f;
        for (int i = 0; i < segments; ++i) {
            const float a0 = pi2 * static_cast<float>(i) / static_cast<float>(segments);
            const float a1 = pi2 * static_cast<float>(i + 1) / static_cast<float>(segments);
            const float x0 = std::cos(a0) * radius, z0 = std::sin(a0) * radius;
            const float x1 = std::cos(a1) * radius, z1 = std::sin(a1) * radius;
            const float nx = std::cos((a0 + a1) * 0.5f), nz = std::sin((a0 + a1) * 0.5f);

            const unsigned int base = static_cast<unsigned int>(data.vertices.size());

            data.vertices.push_back({ { x0, 0.0f,    z0 },{ nx, 0.0f, nz },{ 0.0f, 0.0f } });
            data.vertices.push_back({ { x1, 0.0f,    z1 },{ nx, 0.0f, nz },{ 1.0f, 0.0f } });
            data.vertices.push_back({ { x1, height,  z1 },{ nx, 0.0f, nz },{ 1.0f, 1.0f } });
            data.vertices.push_back({ { x0, height,  z0 },{ nx, 0.0f, nz },{ 0.0f, 1.0f } });

            data.indices.insert(data.indices.end(),
                { base, base + 2, base + 1,
                  base, base + 3, base + 2 });
        }
        return data;
    }

    void WorldGen::render(const Solace::Shader& shader,
        Solace::Mat4 view, Solace::Mat4 proj) const {
        shader.bind();

        shader.setMat4("u_view", view.data());
        shader.setMat4("u_proj", proj.data());

        // Освещение из конфига/настроек
        shader.setVec3("u_sunDir", 0.4f, 0.8f, 0.35f);
        shader.setVec3("u_sunColor", 1.00f, 0.92f, 0.78f);
        shader.setVec3("u_skyColor", 0.45f, 0.55f, 0.60f);
        shader.setVec3("u_groundColor", 0.35f, 0.32f, 0.28f);

        // --- 1. РЕНДЕР КОЛОННЫ (процедурная, без текстуры -> цветная) ---
        Vec3 m_pillarPos = { 5.0f, 0.0f, 0.0f };
        const Solace::Mat4 model = Solace::mat4Translate(m_pillarPos.x, 0.0f, m_pillarPos.z);
        shader.setMat4("u_model", model.data());

        // Явно говорим шейдеру: это цветная модель без текстуры
        shader.setInt("u_hasTexture", 0);
        shader.setVec3("u_baseColor", 1.0f, 0.0f, 0.0f); // Красная колонна

        if (!m_pillar || !m_pillar->valid()) {
            Solace::Logger::error("WorldGen: Pillar is invalid!");
            return;
        }
        m_pillar->draw();


        // --- 2. РЕНДЕР МОНОЛИТА ---
        Vec3 m_monolithPos = { -5.0f, 0.0f, 0.0f };
        if (m_monolith) {
            const Solace::Mat4 model2 = Solace::mat4Translate(m_monolithPos.x, 0.0f, m_monolithPos.z);
            shader.setMat4("u_model", model2.data());

            // Проверяем, есть ли текстура у монолита
            if (m_monolith->getTextureID() != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_monolith->getTextureID());
                shader.setInt("u_textureDiffuse", 0);
                shader.setInt("u_hasTexture", 1);
                shader.setVec3("u_baseColor", 1.0f, 1.0f, 1.0f); // Сбрасываем тонирование в белый
            }
            else {
                shader.setInt("u_hasTexture", 0);
                shader.setVec3("u_baseColor", 0.40f, 0.42f, 0.45f); // Темный камень для монолита
            }

            m_monolith->draw();
        }

        shader.unbind();
    }

} // namespace App