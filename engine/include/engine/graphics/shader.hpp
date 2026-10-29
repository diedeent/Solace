#pragma once

#include <string>
#include <glad/glad.h>

namespace Solace {
    class Shader {
    public:
        Shader(const std::string& vertexPath, const std::string& fragmentPath);
        ~Shader();

        Shader(const Shader&) = delete;

        Shader& operator=(const Shader&) = delete;

        void bind() const;
        void unbind() const;

        void setInt(const std::string& name, int value) const;
        void setFloat(const std::string& name, float value) const;

        void setVec2(const std::string& name, float x, float y) const;
        void setVec3(const std::string& name, float x, float y, float z) const;
        void setVec4(const std::string& name, float x, float y, float z, float w) const;

        void setMat4(const std::string& name, const float* columnMajor4x4) const;

        GLuint getID() const { return m_rendererID; }

    private:
        GLuint m_rendererID = 0;

        std::string readFile(const std::string& filepath);
        GLuint compileShader(GLenum type, const std::string& source);
    };
}