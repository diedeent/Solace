#include <engine/graphics/mesh.hpp>
#include <engine/core/logger.hpp>
#include <stb_image.h>
#include <cstddef>
#include <utility>

namespace Solace {

    GLuint Mesh::s_defaultTextureID = 0;

    Mesh::Mesh(const MeshData& data) { upload(data); }
    Mesh::~Mesh() { release(); }

    Mesh::Mesh(Mesh&& other) noexcept
        : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo), m_indexCount(other.m_indexCount), m_textureID(other.m_textureID) {
        other.m_vao = other.m_vbo = other.m_ebo = other.m_textureID = 0;
        other.m_indexCount = 0;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        if (this != &other) {
            release();
            m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
            m_textureID = other.m_textureID;
            m_indexCount = other.m_indexCount;
            other.m_vao = other.m_vbo = other.m_ebo = other.m_textureID = 0;
            other.m_indexCount = 0;
        }
        return *this;
    }

    GLuint Mesh::getDefaultTextureID() {
        if (s_defaultTextureID == 0) {
            int width, height, channels;

            // Укажи здесь путь к твоей дефолтной текстуре в проекте
            // Например: "assets/textures/default.png" или "assets/default.png"
            std::string defaultTexPath = "assets/textures/missing_texture.jpg";

            unsigned char* imgData = stbi_load(defaultTexPath.c_str(), &width, &height, &channels, 4);

            glGenTextures(1, &s_defaultTextureID);
            glBindTexture(GL_TEXTURE_2D, s_defaultTextureID);

            if (imgData) {
                // Успешно загрузили твой файл текстуры
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
                glGenerateMipmap(GL_TEXTURE_2D);

                // Пиксель-арт фильтрация (чтобы не было размытия)
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                stbi_image_free(imgData);
                Logger::info("Mesh: Loaded default texture file: ", defaultTexPath);
            }
            else {
                // Если файл не найден, выводим ошибку и создаем аварийный белый пиксель, чтобы движок не упал
                Logger::error("Mesh: FAILED to load default texture from '", defaultTexPath, "'. Using white fallback.");
                unsigned char whitePixel[] = { 255, 255, 255, 255 };
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }

            glBindTexture(GL_TEXTURE_2D, 0);
        }
        return s_defaultTextureID;
    }

    void Mesh::upload(const MeshData& data) {
        release();
        if (data.vertices.empty() || data.indices.empty()) return;
        m_baseColor[0] = data.baseColor[0];
        m_baseColor[1] = data.baseColor[1];
        m_baseColor[2] = data.baseColor[2];
        m_hasTexture = false;

        bool textureLoaded = false;

        // 1. Пробуем загрузить встроенную текстуру
        if (data.hasEmbeddedTexture && !data.embeddedBytes.empty()) {
            int width, height, channels;
            unsigned char* imgData = stbi_load_from_memory(
                data.embeddedBytes.data(), static_cast<int>(data.embeddedBytes.size()),
                &width, &height, &channels, 4
            );
            if (imgData) {
                glGenTextures(1, &m_textureID);
                glBindTexture(GL_TEXTURE_2D, m_textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                stbi_image_free(imgData);
                textureLoaded = true;
            }
        }
        // 2. Пробуем загрузить внешнюю текстуру
        else if (!data.diffuseTexturePath.empty()) {
            int width, height, channels;
            unsigned char* imgData = stbi_load(data.diffuseTexturePath.c_str(), &width, &height, &channels, 4);
            if (imgData) {
                glGenTextures(1, &m_textureID);
                glBindTexture(GL_TEXTURE_2D, m_textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                stbi_image_free(imgData);
                textureLoaded = true;
            }
        }

        if (textureLoaded) {
            m_hasTexture = true;
        }
        else {
            m_hasTexture = false;
        }



        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);
        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, data.vertices.size() * sizeof(Vertex), data.vertices.data(), GL_STATIC_DRAW);

        const GLsizei stride = sizeof(Vertex);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, uv));

        glEnableVertexAttribArray(3);
        glVertexAttribIPointer(3, 4, GL_INT, stride, (void*)offsetof(Vertex, boneIDs));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, boneWeights));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices.size() * sizeof(unsigned int), data.indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
        m_indexCount = static_cast<unsigned int>(data.indices.size());
    }

    void Mesh::draw() const {
        if (m_vao == 0) return;
        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

    void Mesh::release() {
        if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
        if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
        if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
        if (m_textureID) { glDeleteTextures(1, &m_textureID); m_textureID = 0; }
        m_indexCount = 0;
    }

} // namespace Solace 