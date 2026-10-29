#include <engine/graphics/shader.hpp>
#include <engine/core/logger.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace Solace {
    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode = readFile(vertexPath);
        std::string fragmentCode = readFile(fragmentPath);

        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexCode);
        GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentCode);

        m_rendererID = glCreateProgram();
        glAttachShader(m_rendererID, vertexShader);
        glAttachShader(m_rendererID, fragmentShader);
        glLinkProgram(m_rendererID);

        GLint success;
        glGetProgramiv(m_rendererID, GL_LINK_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetProgramInfoLog(m_rendererID, 512, NULL, infoLog);
            Logger::error("Failed to link shader program! Error: ", infoLog);
        }
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        Logger::info("Shader program successfully created (ID: ", m_rendererID, ")");
    }

    Shader::~Shader() {
        glDeleteProgram(m_rendererID);
    }

    void Shader::bind() const {
        glUseProgram(m_rendererID);
    }

    void Shader::unbind() const {
        glUseProgram(0);
    }

    void Shader::setInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(m_rendererID, name.c_str()), value);
    }

    void Shader::setFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(m_rendererID, name.c_str()), value);
    }

    void Shader::setVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(m_rendererID, name.c_str()), x, y, z);
    }

    void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const {
        glUniform4f(glGetUniformLocation(m_rendererID, name.c_str()), x, y, z, w);
    }

    void Shader::setVec2(const std::string& name, float x, float y) const {
        glUniform2f(glGetUniformLocation(m_rendererID, name.c_str()), x, y);
    }

    void Shader::setMat4(const std::string& name, const float* columnMajor4x4) const {
        glUniformMatrix4fv(glGetUniformLocation(m_rendererID, name.c_str()),
            1, GL_FALSE, columnMajor4x4);
    }

    std::string Shader::readFile(const std::string& filepath) {
        namespace fs = std::filesystem;

        std::vector<fs::path> candidatePaths = {
            filepath,
            fs::path("..") / filepath,
            fs::path("../..") / filepath,
            fs::path("../../..") / filepath,
            fs::path("../../../..") / filepath
        };

        for (const auto& candidate : candidatePaths) {
            if (fs::exists(candidate) && fs::is_regular_file(candidate)) {
                std::ifstream file;
                file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                try {
                    file.open(candidate);
                    std::stringstream stream;
                    stream << file.rdbuf();
                    file.close();
                    Logger::debug("Shader successfully loaded from: ", fs::absolute(candidate).string());
                    return stream.str();
                }
                catch (std::ifstream::failure& e) {
                    Logger::warning("Found shader at ", candidate.string(), ", but failed to read it.");
                }
            }
        }
        Logger::error("==================================================");
        Logger::error("FATAL: Failed to find shader file: ", filepath);
        Logger::error("Current Working Directory (CWD): ", fs::current_path().string());
        Logger::error("Check your Visual Studio Working Directory settings!");
        Logger::error("==================================================");

        throw std::runtime_error("Shader file not found: " + filepath);
    }

    GLuint Shader::compileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);

        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);

        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            GLchar infoLog[512];
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::string typeStr = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
            Logger::error("Failed to compile ", typeStr, " shader! Error: ", infoLog);
        }

        return shader;
    }
}