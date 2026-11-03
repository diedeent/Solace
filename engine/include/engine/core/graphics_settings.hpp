#pragma once
#include <string>
#include <array>
#include <nlohmann/json.hpp>

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
        std::array<float, 4> clearColor = { 0.45f, 0.55f, 0.65f, 1.0f };
        
            static bool parse(const nlohmann::json & j, GraphicsSettings & out) {
            if (j.contains("lighting")) {
                const auto& l = j["lighting"];
                l.at("sunDir").get_to(out.lighting.sunDir);
                l.at("sunColor").get_to(out.lighting.sunColor);
                l.at("skyColor").get_to(out.lighting.skyColor);
                l.at("groundColor").get_to(out.lighting.groundColor);
                l.at("rimColor").get_to(out.lighting.rimColor);
                out.lighting.toonMidStep = l.value("toonMidStep", out.lighting.toonMidStep);
                out.lighting.toonShadowStep = l.value("toonShadowStep", out.lighting.toonShadowStep);
                out.lighting.rimPower = l.value("rimPower", out.lighting.rimPower);
                
            }
            if (j.contains("postProcess")) {
                const auto& p = j["postProcess"];
                out.postProcess.targetHeight = p.value("targetHeight", out.postProcess.targetHeight);
                out.postProcess.paletteBits = p.value("paletteBits", out.postProcess.paletteBits);
                out.postProcess.ditherAmount = p.value("ditherAmount", out.postProcess.ditherAmount);
                out.postProcess.bloomStrength = p.value("bloomStrength", out.postProcess.bloomStrength);
                out.postProcess.bloomThreshold = p.value("bloomThreshold", out.postProcess.bloomThreshold);
                
            }
            if (j.contains("clearColor")) {
                j.at("clearColor").get_to(out.clearColor);
                
            }
            return true;
            
        }
    };
}