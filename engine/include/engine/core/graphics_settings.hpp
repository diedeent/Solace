#pragma once
#include <string>
#include <array> // Р”РћР‘РђР’Р›Р•РќРћ: РЅСѓР¶РЅРѕ РґР»СЏ std::array

namespace Solace {

    struct LightingSettings {
        std::array<float, 3> sunDir = { 0.4f, 0.8f, 0.35f };
        std::array<float, 3> sunColor = { 1.0f, 0.92f, 0.78f };
        std::array<float, 3> skyColor = { 0.45f, 0.55f, 0.60f };
        std::array<float, 3> groundColor = { 0.35f, 0.32f, 0.28f };
        std::array<float, 3> rimColor = { 1.0f, 0.8f, 0.5f };

        float toonMidStep = 0.4f;
        float toonShadowStep = 0.05f;
        float rimPower = 0.7f;
    };

    struct PostProcessSettings {
        int targetHeight = 180;
        float paletteBits = 6.0f;
        float ditherAmount = 0.6f;
        float bloomStrength = 0.7f;
        float bloomThreshold = 0.85f;
    };

    struct GraphicsSettings {
        LightingSettings lighting;
        PostProcessSettings postProcess;
        std::array<float, 3> clearColor = { 0.45f, 0.55f, 0.65f };
    };

}