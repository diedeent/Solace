#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Solace {

    class SceneContext;
    class SceneManager;

    using ConsoleCommandFunc = std::function<void(const std::vector<std::string>& args)>;

    class DebugOverlay {
    public:
        static void init();
        static void shutdown();

        // ---- Context (передаётся из Game::onInit) ----
        static void set_context(SceneContext* ctx, SceneManager* scenes);

        // ---- Console (~) ----
        static void toggle();
        static bool is_open();

        // ---- Panels (F1) ----
        static void toggle_panels();
        static bool is_panels_open();

        static void register_command(const std::string& name, ConsoleCommandFunc func);
        static void execute_command(const std::string& commandLine);
        static void add_log(const std::string& text);

        static void render();

    private:
        static void render_console();
        static void render_panels();

        // ---- Console state ----
        static bool s_console_open;
        static char s_input_buf[256];
        static std::vector<std::string> s_logs;
        static std::vector<std::string> s_history;
        static int  s_history_pos;
        static std::unordered_map<std::string, ConsoleCommandFunc> s_commands;
        static bool s_scroll_to_bottom;

        // ---- Panels state ----
        static bool s_panels_open;
        static SceneContext* s_ctx;
        static SceneManager* s_scenes;

        static constexpr size_t MAX_LOGS = 100;
    };

} // namespace Solace 