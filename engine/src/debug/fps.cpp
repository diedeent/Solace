#include <engine/debug/fps.hpp>

namespace Solace {
    float Fps::s_fps = 0.0f;
    float Fps::s_accumTime = 0.0f;
    int Fps::s_frameCount = 0;
    bool Fps::s_isVisible = true;

    void Fps::update(float deltaTimeSeconds) {
        s_accumTime += deltaTimeSeconds;
        s_frameCount++;

        if (s_accumTime >= 1.0f) {
            s_fps = static_cast<float>(s_frameCount) / s_accumTime;
            s_accumTime = 0.0f;
            s_frameCount = 0;
        }
    }

    void Fps::renderOverlay() {
        if (!s_isVisible) return;
    }
}