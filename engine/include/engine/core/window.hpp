#pragma once 

#include <string>
#include <SDL3/SDL.h>

namespace Solace {
    class Window {
    public:
        Window(const std::string& title, int width, int height);
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        int width()  const { return m_width; }
        int height() const { return m_height; }

        bool shouldClose() const;

        void swapBuffers() const;
        void toggleFullscreen();

        void updateSize(int newWidth, int newHeight) {
            m_width = newWidth;
            m_height = newHeight;
        }

        SDL_Window* getSDLWindow() const { return m_window; }
        SDL_GLContext GetGLContext() const { return m_glContext; }

    private:
        SDL_Window* m_window = nullptr;
        SDL_GLContext m_glContext = nullptr;
       
        int m_width = 0;
        int m_height = 0;

        bool m_isClosed = false;
        bool m_isFullscreen = false;
    };
}