#include <engine/core/window.hpp>
#include <engine/core/logger.hpp>
#include <glad/glad.h>

namespace Solace {
    Window::Window(const std::string& title, int width, int height) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            Logger::error("Failed to initialize SDL: ", SDL_GetError());
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

        m_window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (!m_window) {
            Logger::error("Failed to create SDL window: ", SDL_GetError());
        }

        m_glContext = SDL_GL_CreateContext(m_window);
        if (!m_glContext) {
            Logger::error("Failed to create OpenGL Context!");
        }

        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            Logger::error("Failed to initialize GLAD (OpenGL loader)!");
        }
        else {
            Logger::info("GLAD successfully initialized! OpenGL version loaded.");
        }

        m_height = height;
        m_width = width;

        Logger::info("Window successfully created: ", width, "x", height);
    }

    Window::~Window() {
        SDL_GL_DestroyContext(m_glContext);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        Logger::info("Window destroyed safely.");
    }

    bool Window::shouldClose() const {
        return m_isClosed;
    }

    void Window::swapBuffers() const {
        SDL_GL_SwapWindow(m_window);
    }

    void Window::toggleFullscreen() {
        m_isFullscreen = !m_isFullscreen;

        SDL_SetWindowFullscreen(m_window, m_isFullscreen);

        Logger::info(m_isFullscreen ? "Switched to Fullscreen mode." : "Switched to Windowed mode.");
    }
}