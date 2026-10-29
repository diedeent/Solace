#pragma once

#include <glad/glad.h>
#include <engine/graphics/shader.hpp>
#include <memory>

namespace Solace {

    struct PostSettings {
        int targetHeight = 90;
        float paletteBits = 6.0f;
        float ditherAmount = 0.5f;
        float bloomStrength = 0.8f;
        float bloomThreshold = 0.9f;
    };

    class PostProcess {
    public:
        explicit PostProcess(const PostSettings& settings = PostSettings{});
        ~PostProcess();

        PostProcess(const PostProcess&) = delete;
        PostProcess& operator=(const PostProcess&) = delete;


        void applySettings(const PostSettings& newSettings);

        void beginScene(int windowW, int windowH);
        void endScene(int windowW, int windowH);

        PostSettings& settings() { return m_settings; }
        int internalWidth()  const { return m_internalW; }
        int internalHeight() const { return m_internalH; }

    private:
        void recreateTarget();
        void createFullscreenTriangle();
        void release();

        PostSettings m_settings;
        std::unique_ptr<Shader> m_shader;
        GLuint m_fbo = 0;
        GLuint m_colorTex = 0;
        GLuint m_depthRbo = 0;
        GLuint m_quadVao = 0;
        GLuint m_quadVbo = 0;
        int m_internalW = 0;
        int m_internalH = 0;
        int m_lastWindowW = 0;
        int m_lastWindowH = 0;
    };

} // namespace Solace 
