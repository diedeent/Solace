#include <engine/graphics/post_process.hpp>
#include <engine/core/logger.hpp>
#include <algorithm>
#include <array>
#include <cmath>

namespace Solace {

    namespace {

        const std::array<float, 12> kFullscreenTriangle = {
            // pos.x, pos.y, uv.x, uv.y
            -1.f, -1.f,  0.f, 0.f,
             3.f, -1.f,  2.f, 0.f,
            -1.f,  3.f,  0.f, 2.f,
        };
    } // namespace

    PostProcess::PostProcess(const PostSettings& settings)
        : m_settings(settings)
    {
        m_shader = std::make_unique<Shader>("assets/shaders/core/post_vs.glsl",
            "assets/shaders/core/post_fs.glsl");
        createFullscreenTriangle();
        m_internalW = 320;
        m_internalH = 180;
        recreateTarget();
        Logger::info("PostProcess: pixel-art lens ready (target height ",
            m_settings.targetHeight, ")");
    }

    PostProcess::~PostProcess() {
        release();
        Logger::info("PostProcess: destroyed.");
    }

    void PostProcess::createFullscreenTriangle() {
        glGenVertexArrays(1, &m_quadVao);
        glGenBuffers(1, &m_quadVbo);
        glBindVertexArray(m_quadVao);
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(kFullscreenTriangle.size() * sizeof(float)),
            kFullscreenTriangle.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
            4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
        glBindVertexArray(0);

        GLint bufferSize = 0;
        glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
        Logger::info("createFullscreenTriangle: buffer size = ", bufferSize);
    }

    void PostProcess::recreateTarget() {
        if (m_colorTex) { glDeleteTextures(1, &m_colorTex);       m_colorTex = 0; }
        if (m_depthRbo) { glDeleteRenderbuffers(1, &m_depthRbo);  m_depthRbo = 0; }
        if (m_fbo) { glDeleteFramebuffers(1, &m_fbo);        m_fbo = 0; }

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glGenTextures(1, &m_colorTex);
        glBindTexture(GL_TEXTURE_2D, m_colorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_internalW, m_internalH,
            0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_colorTex, 0);

        glGenRenderbuffers(1, &m_depthRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
            m_internalW, m_internalH);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER, m_depthRbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Logger::error("PostProcess: framebuffer is incomplete!");
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PostProcess::beginScene(int windowW, int windowH) {
        if (windowW != m_lastWindowW || windowH != m_lastWindowH) {
            m_lastWindowW = windowW;
            m_lastWindowH = windowH;


            const int scale = std::max(1, static_cast<int>(std::lround(
                static_cast<double>(windowH) / m_settings.targetHeight)));

            const int newW = (windowW + scale - 1) / scale;
            const int newH = (windowH + scale - 1) / scale;

            if (newW != m_internalW || newH != m_internalH) {
                m_internalW = newW;
                m_internalH = newH;
                recreateTarget();
                Logger::info("PostProcess: internal resolution ",
                    m_internalW, "x", m_internalH, " (pixel ", scale, "x", scale, ")");
            }

            static bool firstFrame = true;
            if (firstFrame) {
                Logger::info("PostProcess: first frame rendered! Drawing quad to screen.");
                firstFrame = false;
            }
        }

        if (m_fbo == 0 || m_colorTex == 0) {
            Logger::error("PostProcess: FBO is not initialized before render!");
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glViewport(0, 0, m_internalW, m_internalH);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.45f, 0.55f, 0.65f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLenum err = glGetError();
        if (err != GL_NO_ERROR) Logger::error("beginScene: GL error after clear: ", (int)err);
    }

    void PostProcess::endScene(int windowW, int windowH) {

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glViewport(0, 0, windowW, windowH);

        m_shader->bind();
        m_shader->setInt("u_scene", 0);
        m_shader->setVec2("u_lowRes",

        static_cast<float>(m_internalW),
        static_cast<float>(m_internalH));

        m_shader->setFloat("u_paletteBits", m_settings.paletteBits);
        m_shader->setFloat("u_dither", m_settings.ditherAmount);
        m_shader->setFloat("u_bloomStrength", m_settings.bloomStrength);
        m_shader->setFloat("u_bloomThreshold", m_settings.bloomThreshold);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_colorTex);

        glBindVertexArray(m_quadVao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);

        m_shader->unbind();
    }

    void PostProcess::release() {
        if (m_quadVbo) { glDeleteBuffers(1, &m_quadVbo);        m_quadVbo = 0; }
        if (m_quadVao) { glDeleteVertexArrays(1, &m_quadVao);   m_quadVao = 0; }
        if (m_depthRbo) { glDeleteRenderbuffers(1, &m_depthRbo); m_depthRbo = 0; }
        if (m_colorTex) { glDeleteTextures(1, &m_colorTex);      m_colorTex = 0; }
        if (m_fbo) { glDeleteFramebuffers(1, &m_fbo);       m_fbo = 0; }
    }

    void PostProcess::applySettings(const PostSettings& newSettings) {
        m_settings = newSettings;
        m_lastWindowW = -1;
        m_lastWindowH = -1;
        Logger::info("PostProcess: settings applied. Target height: ", m_settings.targetHeight);
    }

} // namespace Solace 