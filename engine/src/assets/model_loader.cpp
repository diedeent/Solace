#include <engine/assets/model_loader.hpp>
#include <engine/core/logger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <cstring>
#include <filesystem>
#include <map>

namespace Solace {

    void processNode(aiNode* node, const aiScene* scene, std::vector<MeshData>& outMeshes, const std::filesystem::path& basePath) {
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* aiMeshPtr = scene->mMeshes[node->mMeshes[i]];
            MeshData meshData;

            if (scene->mMaterials && aiMeshPtr->mMaterialIndex < scene->mNumMaterials) {
                aiMaterial* material = scene->mMaterials[aiMeshPtr->mMaterialIndex];
                aiString aiTexPath;
                aiTextureType typesToCheck[] = {
                    aiTextureType_DIFFUSE,
                    aiTextureType_BASE_COLOR,
                    aiTextureType_UNKNOWN,
                    aiTextureType_EMISSIVE,
                    aiTextureType_AMBIENT,
                    aiTextureType_SPECULAR
                };

                Logger::info("--- Material ", aiMeshPtr->mMaterialIndex, " textures ---");
                for (int t = 0; t < aiTextureType_UNKNOWN + 1; t++) {
                    aiTextureType type = static_cast<aiTextureType>(t);
                    UINT count = material->GetTextureCount(type);
                    if (count > 0) {
                        aiString texPath;
                        material->GetTexture(type, 0, &texPath);
                        Logger::info("  Type ", t, ": ", count, " textures, first = '", texPath.C_Str(), "'");
                    }
                }
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
                                meshData.diffuseTexturePath = "embedded_" + std::to_string(aiMeshPtr->mMaterialIndex);
                                textureFound = true;
                                break;
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
                        meshData.baseColor[0] = aiColor.r;
                        meshData.baseColor[1] = aiColor.g;
                        meshData.baseColor[2] = aiColor.b;
                        Logger::info("ModelLoader: No texture, using material color: R=", aiColor.r, " G=", aiColor.g, " B=", aiColor.b);
                    }
                }
            }



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
                else {
                    vertex.normal[0] = 0.0f; vertex.normal[1] = 1.0f; vertex.normal[2] = 0.0f;
                }

                if (aiMeshPtr->HasTextureCoords(0)) {
                    vertex.uv[0] = aiMeshPtr->mTextureCoords[0][v].x;
                    vertex.uv[1] = aiMeshPtr->mTextureCoords[0][v].y;
                }
                else {
                    vertex.uv[0] = 0.0f; vertex.uv[1] = 0.0f;
                }

                meshData.vertices.push_back(vertex);
            }

            if (aiMeshPtr->HasBones()) {
                Logger::info("  Mesh has ", aiMeshPtr->mNumBones, " bones");

                for (unsigned int b = 0; b < aiMeshPtr->mNumBones; b++) {
                    meshData.boneNames.push_back(aiMeshPtr->mBones[b]->mName.C_Str());
                }

                int vertsWithWeights = 0;
                for (unsigned int b = 0; b < aiMeshPtr->mNumBones; b++) {
                    aiBone* bone = aiMeshPtr->mBones[b];
                    for (unsigned int w = 0; w < bone->mNumWeights; w++) {
                        unsigned int vId = bone->mWeights[w].mVertexId;
                        float weight = bone->mWeights[w].mWeight;
                        if (weight <= 0.0f) continue;
                        if (vId >= meshData.vertices.size()) continue;
                        Vertex& v = meshData.vertices[vId];
                        bool placed = false;
                        for (int s = 0; s < 4; s++) {
                            if (v.boneWeights[s] == 0.0f) {
                                v.boneWeights[s] = weight;
                                v.boneIDs[s] = static_cast<int>(b);
                                placed = true;
                                if (s == 0) vertsWithWeights++;
                                break;
                            }
                        }
                    }
                }
                Logger::info("  Vertices with weights: ", vertsWithWeights, " / ", meshData.vertices.size());
            }
            else {
                Logger::warning("  Mesh has NO bones!");
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
            processNode(node->mChildren[i], scene, outMeshes, basePath);
        }
    }

    std::vector<MeshData> ModelLoader::loadFromFile(const std::string& path) {
        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_FlipUVs |
            aiProcess_CalcTangentSpace |
            aiProcess_LimitBoneWeights);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            throw std::runtime_error("Assimp error loading model (" + path + "): " + importer.GetErrorString());
        }

        std::vector<MeshData> outMeshes;
        std::filesystem::path modelPath(path);
        std::filesystem::path basePath = modelPath.parent_path();

        processNode(scene->mRootNode, scene, outMeshes, basePath);

        Logger::info("ModelLoader: successfully loaded '", path, "' (Meshes found: ", outMeshes.size(), ")");
        return outMeshes;
    }

    MeshData ModelLoader::loadObj(const std::string& path) {
        auto meshes = loadFromFile(path);
        if (meshes.empty()) throw std::runtime_error("ModelLoader: No meshes found in file: " + path);
        return meshes[0];
    }
}