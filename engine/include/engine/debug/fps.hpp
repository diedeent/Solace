#pragma once

namespace Solace {
    class Fps {
    public:
        static void update(float deltaTimeSeconds); 
        static float getFPS() { return s_fps; }

        static void setVisible(bool visible) { s_isVisible = visible; }
        static bool isVisible() { return s_isVisible; }

        static void renderOverlay();

    private:
        static float s_fps;
        static float s_accumTime;
        static int s_frameCount;
        static bool s_isVisible;
    };
}