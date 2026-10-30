#include <engine/debug/debug_overlay.hpp>

#include <engine/core/clock.hpp>
#include <engine/core/config_cache.hpp>
#include <engine/core/entity_registry.hpp>
#include <engine/core/event_bus.hpp>
#include <engine/core/logger.hpp>
#include <engine/scene/scene_manager.hpp>

#include <imgui.h>

#include <algorithm>
#include <sstream>

namespace Solace {

    // ============================================================
    // Static state
    // ============================================================
    bool DebugOverlay::s_console_open = false;
    bool DebugOverlay::s_panels_open = false;
    char DebugOverlay::s_input_buf[256] = "";
    std::vector<std::string> DebugOverlay::s_logs;
    std::vector<std::string> DebugOverlay::s_history;
    int  DebugOverlay::s_history_pos = -1;
    std::unordered_map<std::string, ConsoleCommandFunc> DebugOverlay::s_commands;
    bool DebugOverlay::s_scroll_to_bottom = false;
    SceneContext* DebugOverlay::s_ctx = nullptr;
    SceneManager* DebugOverlay::s_scenes = nullptr;

    // ============================================================
    // Lifecycle
    // ============================================================

    void DebugOverlay::init() {
        Logger::info("DebugOverlay: initialized");
        add_log("=== Solace Debug Overlay ===");
        add_log("Press F1 for panels, ` for console");

        register_command("help", [](const std::vector<std::string>&) {
            add_log("--- Commands ---");
            add_log("  help          - this message");
            add_log("  clear         - clear log");
            add_log("  fps 0/1       - toggle FPS");
            add_log("  mode game/dev - camera mode");
            });

        register_command("clear", [](const std::vector<std::string>&) {
            s_logs.clear();
            });
    }

    void DebugOverlay::shutdown() {
        s_commands.clear();
        s_logs.clear();
        s_history.clear();
        s_ctx = nullptr;
        s_scenes = nullptr;
    }

    void DebugOverlay::set_context(SceneContext* ctx, SceneManager* scenes) {
        s_ctx = ctx;
        s_scenes = scenes;
    }

    // ============================================================
    // Toggle
    // ============================================================

    void DebugOverlay::toggle() { s_console_open = !s_console_open; }
    bool DebugOverlay::is_open() { return s_console_open; }

    void DebugOverlay::toggle_panels() { s_panels_open = !s_panels_open; }
    bool DebugOverlay::is_panels_open() { return s_panels_open; }

    // ============================================================
    // Commands
    // ============================================================

    void DebugOverlay::add_log(const std::string& text) {
        if (s_logs.size() >= MAX_LOGS) s_logs.erase(s_logs.begin());
        s_logs.push_back(text);
        s_scroll_to_bottom = true;
    }

    void DebugOverlay::register_command(const std::string& name, ConsoleCommandFunc func) {
        s_commands[name] = std::move(func);
    }

    void DebugOverlay::execute_command(const std::string& cmdline) {
        std::stringstream ss(cmdline);
        std::string name;
        ss >> name;
        if (name.empty()) return;

        std::vector<std::string> args;
        std::string arg;
        while (ss >> arg) args.push_back(arg);

        auto it = s_commands.find(name);
        if (it != s_commands.end()) it->second(args);
        else add_log("Unknown command: '" + name + "'");
    }

    // ============================================================
    // Render entry
    // ============================================================

    void DebugOverlay::render() {
        if (s_console_open) render_console();
        if (s_panels_open)  render_panels();
    }

    // ============================================================
    // Console (~)
    // ============================================================

    void DebugOverlay::render_console() {
        ImGui::SetNextWindowSize(ImVec2(900, 320), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.92f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.5f, 0.8f, 0.5f));

        if (ImGui::Begin("Solace Console (F1=panels)", &s_console_open,
            ImGuiWindowFlags_NoScrollbar))
        {
            const float footer = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            if (ImGui::BeginChild("##scroll", ImVec2(0, -footer), false,
                ImGuiWindowFlags_HorizontalScrollbar))
            {
                for (const auto& log : s_logs) {
                    ImVec4 color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
                    if (log.find("[ERROR]") != std::string::npos) color = ImVec4(1.00f, 0.35f, 0.35f, 1.0f);
                    if (log.find("[WARNING]") != std::string::npos) color = ImVec4(1.00f, 0.80f, 0.20f, 1.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(log.c_str());
                    ImGui::PopStyleColor();
                }
                if (s_scroll_to_bottom) {
                    ImGui::SetScrollHereY(1.0f);
                    s_scroll_to_bottom = false;
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            ImGui::PushItemWidth(-1.0f);
            if (ImGui::InputText("##input", s_input_buf, IM_ARRAYSIZE(s_input_buf),
                ImGuiInputTextFlags_EnterReturnsTrue))
            {
                std::string s(s_input_buf);
                if (!s.empty()) {
                    add_log("> " + s);
                    execute_command(s);
                    s_history.push_back(s);
                    s_history_pos = -1;
                }
                s_input_buf[0] = '\0';
                ImGui::SetKeyboardFocusHere(-1);
            }
            ImGui::PopItemWidth();
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
    }

    // ============================================================
    // Panels (F1)
    // ============================================================

    void DebugOverlay::render_panels() {
        if (!s_ctx) {
            ImGui::Begin("Debug Panels (F1)");
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "No SceneContext set");
            ImGui::End();
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(360, 500), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(10, 100), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Debug Panels (F1)", &s_panels_open))
        {
            // ---- ECS ----
            if (ImGui::CollapsingHeader("ECS", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Alive:   %zu", s_ctx->registry.alive_count());
                ImGui::Text("Pending: %zu", s_ctx->registry.pending_count());
                ImGui::Text("Next ID: %u", s_ctx->registry.next_id());
            }

            // ---- Clock ----
            if (ImGui::CollapsingHeader("Clock", ImGuiTreeNodeFlags_DefaultOpen)) {
                const auto st = s_ctx->clock.debug_stats();
                ImGui::Text("FPS:        %.1f", st.fps);
                ImGui::Text("realDt:     %.2f ms", st.real_dt * 1000.0f);
                ImGui::Text("gameplayDt: %.2f ms", st.gameplay_dt * 1000.0f);
                ImGui::Text("total real: %.2f s", st.total_real);
                ImGui::Text("total game: %.2f s", st.total_gameplay);
                if (st.paused)          ImGui::TextColored(ImVec4(1, 0.8f, 0.2f, 1), "PAUSED");
                if (st.in_hitstop)      ImGui::TextColored(ImVec4(1, 0.6f, 0.3f, 1),
                    "HITSTOP %.2fs", st.hitstop_remaining);
                if (st.gameplay_scale != 1.0f)
                    ImGui::Text("scale: %.2f", st.gameplay_scale);
            }

            // ---- EventBus ----
            if (ImGui::CollapsingHeader("EventBus")) {
                const auto st = s_ctx->bus.debug_stats();
                ImGui::Text("Subscribers:   %zu", st.total_subscribers);
                ImGui::Text("Unique events: %zu", st.unique_event_types);
                ImGui::Text("Queued:        %zu", st.queued_events);
            }

            // ---- Scene ----
            if (s_scenes && ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Depth: %zu", s_scenes->depth());
                ImGui::Text("Top:   %s", s_scenes->top_scene_name());
            }

            // ---- Config ----
            if (ImGui::CollapsingHeader("Config")) {
                const auto sections = s_ctx->config.debug_section_names();
                ImGui::Text("Sections: %zu", sections.size());
                for (const auto& name : sections) {
                    ImGui::BulletText("%s", name.c_str());
                }
            }

            // ---- Hint ----
            ImGui::Separator();
            ImGui::TextDisabled("` - console, F1 - close panels");
        }
        ImGui::End();
    }

} // namespace Solace