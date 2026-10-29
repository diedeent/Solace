#include <engine/core/settings_manager.hpp>
#include <engine/core/logger.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace Solace {

    GraphicsSettings SettingsManager::load(const std::string& filepath) {
        GraphicsSettings settings;

        if (!std::filesystem::exists(filepath)) {
            Logger::warning("Settings file not found. Creating default: ", filepath);
            save(settings, filepath);
            return settings;
        }

        try {
            std::ifstream file(filepath);
            json j;
            file >> j;

            if (j.contains("lighting")) {
                auto& l = j["lighting"];
                l.at("sunDir").get_to(settings.lighting.sunDir);
                l.at("sunColor").get_to(settings.lighting.sunColor);
                l.at("skyColor").get_to(settings.lighting.skyColor);
                l.at("groundColor").get_to(settings.lighting.groundColor);
                l.at("toonMidStep").get_to(settings.lighting.toonMidStep);
                l.at("toonShadowStep").get_to(settings.lighting.toonShadowStep);
                l.at("rimColor").get_to(settings.lighting.rimColor);
                l.at("rimPower").get_to(settings.lighting.rimPower);
            }
            if (j.contains("postProcess")) {
                auto& p = j["postProcess"];
                p.at("targetHeight").get_to(settings.postProcess.targetHeight);
                p.at("paletteBits").get_to(settings.postProcess.paletteBits);
                p.at("ditherAmount").get_to(settings.postProcess.ditherAmount);
                p.at("bloomStrength").get_to(settings.postProcess.bloomStrength);
                p.at("bloomThreshold").get_to(settings.postProcess.bloomThreshold);
            }

            if (j.contains("clearColor")) {
                j.at("clearColor").get_to(settings.clearColor);
            }

            Logger::info("Graphics settings loaded from: ", filepath);
        }
        catch (const std::exception& e) {
            Logger::error("Failed to parse graphics.json: ", e.what());
        }

        return settings;
    }

    void SettingsManager::save(const GraphicsSettings& settings, const std::string& filepath) {
        json j;

        j["lighting"]["sunDir"] = settings.lighting.sunDir;
        j["lighting"]["sunColor"] = settings.lighting.sunColor;
        j["lighting"]["skyColor"] = settings.lighting.skyColor;
        j["lighting"]["groundColor"] = settings.lighting.groundColor;
        j["lighting"]["toonMidStep"] = settings.lighting.toonMidStep;
        j["lighting"]["toonShadowStep"] = settings.lighting.toonShadowStep;
        j["lighting"]["rimColor"] = settings.lighting.rimColor;
        j["lighting"]["rimPower"] = settings.lighting.rimPower;

        j["postProcess"]["targetHeight"] = settings.postProcess.targetHeight;
        j["postProcess"]["paletteBits"] = settings.postProcess.paletteBits;
        j["postProcess"]["ditherAmount"] = settings.postProcess.ditherAmount;
        j["postProcess"]["bloomStrength"] = settings.postProcess.bloomStrength;
        j["postProcess"]["bloomThreshold"] = settings.postProcess.bloomThreshold;

        j["clearColor"] = settings.clearColor;

        std::filesystem::create_directories(std::filesystem::path(filepath).parent_path());

        std::ofstream file(filepath);
        file << j.dump(4);
        Logger::info("Graphics settings saved to: ", filepath);
    }
}