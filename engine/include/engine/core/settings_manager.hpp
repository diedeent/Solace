#pragma once
#include <engine/core/graphics_settings.hpp>

namespace Solace {
    class SettingsManager {
    public:
        // JSON
        static GraphicsSettings load(const std::string& filepath = "assets/configs/graphics.json");

        // JSON
        static void save(const GraphicsSettings& settings, const std::string& filepath = "assets/configs/graphics.json");
    };
}