#include <engine/assets/model_loader.hpp>
#include <engine/debug/logger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <cstring>
#include <filesystem>
#include <map>
#include <unordered_map>

namespace Solace {

    void processNode(aiNode* node, const aiScene* scene, std::vector<MeshData>& outMeshes,
        const std::filesystem::path& basePath,
        std::unordered_map<std::string, int>& globalBoneMap,
        std::vector<std::string>& globalBoneNames)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* aiMeshPtr = scene->mMeshes[node->mMeshes[i]];
            MeshData meshData;

            // ... (БЛОК ЗАГРУЗКИ ТЕКСТУР ОСТАЕТСЯ БЕЗ ИЗМЕНЕНИЙ) ...
            if (scene->mMaterials && aiMeshPtr->mMaterialIndex < scene->mNumMaterials) {
                aiMaterial* material = scene->mMaterials[aiMeshPtr->mMaterialIndex];
                aiString aiTexPath;
                aiTextureType typesToCheck[] = {
                    aiTextureType_DIFFUSE, aiTextureType_BASE_COLOR, aiTextureType_UNKNOWN,
                    aiTextureType_EMISSIVE, aiTextureType_AMBIENT, aiTextureType_SPECULAR
                };
                bool textureFound = false;
                for (aiTextureType type : typesToCheck) {
                    if (material->GetTextureCount(type) > 0) {
                        if (material->GetTexture(type, 0, &aiTexPath) == AI_SUCCESS) {
                            std::string texPath = aiTexPath.C_Str();
                            std::filesystem::path p(texPath);
                            std::string filename = p.filename().string();
                            const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texPath.c_str());
                            if (embeddedTex) {
                                meshData.hasEmbeddedTexture = true;
                                size_t sizeInBytes = (embeddedTex->mHeight == 0) ? embeddedTex->mWidth : embeddedTex->mWidth * embeddedTex->mHeight * 4;
                                meshData.embeddedBytes.resize(sizeInBytes);
                                std::memcpy(meshData.embeddedBytes.data(), embeddedTex->pcData, sizeInBytes);
                                textureFound = true; break;
                            }
                            else {
                                std::filesystem::path path1 = basePath / "textures" / filename;
                                std::filesystem::path path2 = basePath / filename;
                                if (std::filesystem::exists(path1)) { meshData.diffuseTexturePath = path1.string(); textureFound = true; break; }
                                else if (std::filesystem::exists(path2)) { meshData.diffuseTexturePath = path2.string(); textureFound = true; break; }
                            }
                        }
                    }
                }
                if (!textureFound) {
                    aiColor3D aiColor(1.0f, 1.0f, 1.0f);
                    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor) == AI_SUCCESS) {
                        meshData.baseColor[0] = aiColor.r; meshData.baseColor[1] = aiColor.g; meshData.baseColor[2] = aiColor.b;
                    }
                }
            }

            // ... (БЛОК ЗАГРУЗКИ ВЕРШИН) ...
            for (unsigned int v = 0; v < aiMeshPtr->mNumVertices; v++) {
                Vertex vertex{};
                vertex.position[0] = aiMeshPtr->mVertices[v].x;
                vertex.position[1] = aiMeshPtr->mVertices[v].y;
                vertex.position[2] = aiMeshPtr->mVertices[v].z;

                if (aiMeshPtr->HasNormals()) {
                    vertex.normal[0] = aiMeshPtr->mNormals[v].x;
                    vertex.normal[1] = aiMeshPtr->mNormals[v].y;
                    vertex.normal[2] = aiMeshPtr->mNormals[v].z;
                }
                if (aiMeshPtr->HasTextureCoords(0)) {
                    vertex.uv[0] = aiMeshPtr->mTextureCoords[0][v].x;
                    vertex.uv[1] = aiMeshPtr->mTextureCoords[0][v].y;
                }
                meshData.vertices.push_back(vertex);
            }

            // --- ИСПРАВЛЕНИЕ КОСТЕЙ ---
            if (aiMeshPtr->HasBones()) {
                // 1. Сначала собираем ВСЕ уникальные имена костей в глобальный список
                for (unsigned int b = 0; b < aiMeshPtr->mNumBones; b++) {
                    std::string boneName = aiMeshPtr->mBones[b]->mName.C_Str();
                    if (globalBoneMap.find(boneName) == globalBoneMap.end()) {
                        globalBoneMap[boneName] = static_cast<int>(globalBoneNames.size());
                        globalBoneNames.push_back(boneName);
                    }
                }

                // 2. Теперь назначаем веса, используя ГЛОБАЛЬНЫЙ индекс кости
                for (unsigned int b = 0; b < aiMeshPtr->mNumBones; b++) {
                    aiBone* bone = aiMeshPtr->mBones[b];
                    std::string boneName = bone->mName.C_Str();
                    int globalId = globalBoneMap[boneName];

                    for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                        unsigned int vId = bone->mWeights[w].mVertexId;
                        float weight = bone->mWeights[w].mWeight;
                        if (weight <= 0.0f || vId >= meshData.vertices.size()) continue;

                        Vertex& v = meshData.vertices[vId];
                        for (int s = 0; s < 4; s++) {
                            if (v.boneWeights[s] == 0.0f) {
                                v.boneWeights[s] = weight;
                                v.boneIDs[s] = globalId; // <-- ИСПОЛЬЗУЕМ ГЛОБАЛЬНЫЙ ID
                                break;
                            }
                        }
                    }
                }

                for (auto& v : meshData.vertices) {
                    float sum = 0.0f;
                    for (int i = 0; i < 4; i++) sum += v.boneWeights[i];
                    if (sum > 0.0f) for (int i = 0; i < 4; i++) v.boneWeights[i] /= sum;
                }
            }

            for (unsigned int f = 0; f < aiMeshPtr->mNumFaces; f++) {
                aiFace face = aiMeshPtr->mFaces[f];
                for (unsigned int j = 0; j < face.mNumIndices; j++) {
                    meshData.indices.push_back(face.mIndices[j]);
                }
            }

            outMeshes.push_back(meshData);
        }

        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, outMeshes, basePath, globalBoneMap, globalBoneNames);
        }
    }

    std::vector<MeshData> ModelLoader::loadFromFile(const std::string& path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            throw std::runtime_error("Assimp error loading model (" + path + "): " + importer.GetErrorString());
        }

        std::vector<MeshData> outMeshes;
        std::filesystem::path basePath = std::filesystem::path(path).parent_path();

        // Глобальные карты для всей модели
        std::unordered_map<std::string, int> globalBoneMap;
        std::vector<std::string> globalBoneNames;

        processNode(scene->mRootNode, scene, outMeshes, basePath, globalBoneMap, globalBoneNames);

        // Записываем глобальный список костей в каждый меш (чтобы AssetCache его видел)
        for (auto& mesh : outMeshes) {
            mesh.boneNames = globalBoneNames;
        }

        SOLACE_INFO("ModelLoader: loaded '", path, "' (Meshes: ", outMeshes.size(), ", Global Bones: ", globalBoneNames.size(), ")");
        return outMeshes;
    }

    MeshData ModelLoader::loadObj(const std::string& path) {
        auto meshes = loadFromFile(path);
        if (meshes.empty()) throw std::runtime_error("ModelLoader: No meshes found in file: " + path);
        return meshes[0];
    }
}