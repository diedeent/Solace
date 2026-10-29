#include <game/utils/player_model.hpp>
#include <engine/core/logger.hpp>
#include <engine/assets/model_loader.hpp>
#include <glad/glad.h>
#include <algorithm>

#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/soa_transform.h>

namespace App {

    PlayerModel::PlayerModel()
        : m_scale(1.0f)
    {
        glGenBuffers(1, &m_boneSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 500 * sizeof(ozz::math::Float4x4), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    PlayerModel::~PlayerModel() {
        if (m_boneSSBO) {
            glDeleteBuffers(1, &m_boneSSBO);
            m_boneSSBO = 0;
        }
    }

    void PlayerModel::loadModel(const std::string& filePath) {
        m_currentModelPath = filePath;
        m_meshes.clear();

        m_animator = std::make_unique<Solace::Animator>();
        if (!m_animator->loadSkeleton("assets/animations/player_skeleton.ozz")) {
            Solace::Logger::error("PlayerModel: skeleton NOT loaded");
        }
        if (!m_animator->loadAnimation("assets/animations/player_run.ozz")) {
            Solace::Logger::error("PlayerModel: animation NOT loaded");
        }

        try {
            auto meshesData = Solace::ModelLoader::loadFromFile(filePath);
            for (auto& data : meshesData) {
                auto cleanBoneName = [](std::string name) {
                    size_t colon = name.find(':');
                    if (colon != std::string::npos) name = name.substr(colon + 1);

                    size_t pipe = name.rfind('|');
                    if (pipe != std::string::npos) name = name.substr(pipe + 1);
                    if (name.size() > 3 && name[name.size() - 4] == '_' &&
                        isdigit(name[name.size() - 3]) && isdigit(name[name.size() - 2]) &&
                        isdigit(name[name.size() - 1])) {
                        name = name.substr(0, name.size() - 4);
                    }
                    else if (name.size() > 2 && name[name.size() - 3] == '_' &&
                        isdigit(name[name.size() - 2]) && isdigit(name[name.size() - 1])) {
                        name = name.substr(0, name.size() - 3);
                    }

                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    return name;
                    };

                if (!data.boneNames.empty() && m_animator) {
                    const auto& skeleton = m_animator->GetSkeleton();
                    auto jointNames = skeleton.joint_names();

                    Solace::Logger::info("=== Assimp bones (first 5) ===");
                    for (size_t i = 0; i < std::min<size_t>(5, data.boneNames.size()); i++) {
                        Solace::Logger::info("  [", i, "] '", data.boneNames[i], "' -> cleaned '", cleanBoneName(data.boneNames[i]), "'");
                    }

                    Solace::Logger::info("=== ozz joints (first 5) ===");
                    for (size_t i = 0; i < std::min<size_t>(5, jointNames.size()); i++) {
                        if (jointNames[i]) {
                            Solace::Logger::info("  [", i, "] '", jointNames[i], "' -> cleaned '", cleanBoneName(jointNames[i]), "'");
                        }
                    }

                    int mappedWeights = 0;
                    int unmappedWeights = 0;
                    int fullyMappedVerts = 0;
                    int fullyUnmappedVerts = 0;

                    for (auto& v : data.vertices) {
                        float totalW = 0.0f;
                        bool anyMapped = false;

                        for (int s = 0; s < 4; s++) {
                            if (v.boneWeights[s] > 0.0f) {
                                int assimpIdx = v.boneIDs[s];
                                if (assimpIdx >= 0 && assimpIdx < (int)data.boneNames.size()) {
                                    std::string cleanAssimpName = cleanBoneName(data.boneNames[assimpIdx]);
                                    int ozzIdx = -1;

                                    for (size_t j = 0; j < jointNames.size(); j++) {
                                        if (jointNames[j] != nullptr) {
                                            std::string cleanOzzName = cleanBoneName(jointNames[j]);
                                            if (cleanOzzName == cleanAssimpName) {
                                                ozzIdx = (int)j;
                                                break;
                                            }
                                        }
                                    }

                                    if (ozzIdx >= 0) {
                                        v.boneIDs[s] = ozzIdx;
                                        totalW += v.boneWeights[s];
                                        mappedWeights++;
                                        anyMapped = true;
                                    }
                                    else {
                                        if (unmappedWeights < 10) {
                                            Solace::Logger::warning("Unmapped bone: '", data.boneNames[assimpIdx], "' (cleaned: '", cleanAssimpName, "')");
                                        }
                                        v.boneWeights[s] = 0.0f;
                                        v.boneIDs[s] = 0;
                                        unmappedWeights++;
                                    }
                                }
                                else {
                                    v.boneWeights[s] = 0.0f;
                                    v.boneIDs[s] = 0;
                                    unmappedWeights++;
                                }
                            }
                        }

                        if (anyMapped) fullyMappedVerts++; else fullyUnmappedVerts++;

                        if (totalW > 0.01f) {
                            for (int s = 0; s < 4; s++) v.boneWeights[s] /= totalW;
                        }
                        else {
                            for (int s = 0; s < 4; s++) v.boneWeights[s] = 0.0f;
                        }
                    }

                    Solace::Logger::info("Bone mapping: ", mappedWeights, " mapped, ", unmappedWeights, " unmapped");
                    Solace::Logger::info("Verts fully mapped: ", fullyMappedVerts, "/", (int)data.vertices.size());
                }
                m_meshes.push_back(std::make_unique<Solace::Mesh>(data));
            }
            Solace::Logger::info("Player model loaded with SSBO. Meshes: ", m_meshes.size());
        }
        catch (const std::exception& e) {
            Solace::Logger::error("Failed to load player model: ", e.what());
        }
    }

    void PlayerModel::swapModel(const std::string& newFilePath) {
        loadModel(newFilePath);
    }

    void PlayerModel::update(float deltaTime) {
        if (m_animator) m_animator->update(deltaTime);
    }

    void PlayerModel::render(Solace::Shader& shader, const Solace::Mat4& viewMatrix, const Solace::Mat4& projMatrix,
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

        if (m_animator) {
            auto bones = m_animator->getBoneMatrices();
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_boneSSBO);
            GLsizeiptr requiredSize = static_cast<GLsizeiptr>(bones.size() * sizeof(ozz::math::Float4x4));
            glBufferData(GL_SHADER_STORAGE_BUFFER, requiredSize, bones.data(), GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_boneSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }

        for (const auto& mesh : m_meshes) {
            // Проверяем, есть ли у меша реальная текстура
            if (mesh->getTextureID() != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh->getTextureID());
                shader.setInt("u_textureDiffuse", 0);
                shader.setInt("u_hasTexture", 1);

                // Если текстура есть, белый базовый цвет не будет тонировать её лишним оттенком
                shader.setVec3("u_baseColor", 1.0f, 1.0f, 1.0f);
            }
            else {
                // Если текстуры НЕТ, включаем режим чистой заливки цветом материала
                shader.setInt("u_hasTexture", 0);
                const float* col = mesh->getBaseColor();
                shader.setVec3("u_baseColor", col[0], col[1], col[2]);
            }
            mesh->draw();
        }

        shader.unbind();
    }

    const ozz::animation::Skeleton& PlayerModel::GetSkeleton() const {
        static ozz::animation::Skeleton dummySkeleton;
        if (m_animator) {
            return m_animator->GetSkeleton();
        }
        return dummySkeleton;
    }

}