#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace App {

    struct PlayerConfig {
        struct Stats {
            float maxHealth = 100.0f;
            float maxStamina = 100.0f;
            float staminaRegen = 25.0f;
        } stats;

        struct Movement {
            float speed = 5.0f;
            float dashSpeed = 17.5f;
            float dashDuration = 0.25f;
            float dashCooldown = 0.5f;
            float dashStaminaCost = 25.0f;
        } movement;

        struct Animation {
            std::string defaultClip = "idle";
            float fadeTime = 0.2f;
        } animation;

        static bool parse(const nlohmann::json& j, PlayerConfig& out) {
            if (!j.contains("stats") || !j.contains("movement") || !j.contains("animation"))
                return false;

            out.stats.maxHealth = j["stats"].value("maxHealth", out.stats.maxHealth);
            out.stats.maxStamina = j["stats"].value("maxStamina", out.stats.maxStamina);
            out.stats.staminaRegen = j["stats"].value("staminaRegen", out.stats.staminaRegen);

            out.movement.speed = j["movement"].value("speed", out.movement.speed);
            out.movement.dashSpeed = j["movement"].value("dashSpeed", out.movement.dashSpeed);
            out.movement.dashDuration = j["movement"].value("dashDuration", out.movement.dashDuration);
            out.movement.dashCooldown = j["movement"].value("dashCooldown", out.movement.dashCooldown);
            out.movement.dashStaminaCost = j["movement"].value("dashStaminaCost", out.movement.dashStaminaCost);

            out.animation.defaultClip = j["animation"].value("defaultClip", out.animation.defaultClip);
            out.animation.fadeTime = j["animation"].value("fadeTime", out.animation.fadeTime);
            return true;
        }
    };

} // namespace App