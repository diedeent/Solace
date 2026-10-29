#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>

namespace Solace {

    struct Vertex {
        float position[3];
        float normal[3];
        float uv[2];
        int   boneIDs[4] = { 0, 0, 0, 0 };
        float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    struct MeshData {
        std::vector<Vertex>         vertices;
        std::vector<unsigned int>   indices;
        std::string                 diffuseTexturePath;
        bool                        hasEmbeddedTexture = false;
        std::vector<unsigned char>  embeddedBytes;
        std::vector<std::string>    boneNames;

        float baseColor[3] = { 1.0f, 1.0f, 1.0f };
    };

    class Mesh {
    public:
        Mesh() = default;
        explicit Mesh(const MeshData& data);
        ~Mesh();

        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        void upload(const MeshData& data);
        void draw() const;
        static GLuint getDefaultTextureID();

        unsigned int indexCount() const { return m_indexCount; }
        bool valid() const { return m_vao != 0; }
        GLuint getTextureID() const { return m_textureID; }
        const float* getBaseColor() const { return m_baseColor; }

        bool hasTexture() const { return m_hasTexture; }

    private:
        void release();
        GLuint m_vao = 0, m_vbo = 0, m_ebo = 0, m_textureID = 0;
        unsigned int m_indexCount = 0;

        float m_baseColor[3] = { 1.0f, 1.0f, 1.0f };

        static GLuint s_defaultTextureID;

        bool m_hasTexture = false;
    };

} // namespace Solace 