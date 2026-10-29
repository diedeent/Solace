#include <engine/debug/debug_overlay.hpp>
#include <engine/core/logger.hpp>
#include <imgui.h>
#include <sstream>
#include <algorithm>

namespace Solace {

    bool DebugOverlay::s_isOpen = false;
    char DebugOverlay::s_inputBuf[256] = "";
    std::vector<std::string> DebugOverlay::s_logs;
    std::unordered_map<std::string, ConsoleCommandFunc> DebugOverlay::s_commands;
    bool DebugOverlay::s_scrollToBottom = false;

    static std::vector<std::string> s_history;
    static int s_historyPos = -1;

    static int TextEditCallback(ImGuiInputTextCallbackData* data) {
        if (data->EventKey == ImGuiKey_UpArrow) {
            if (s_history.empty()) return 0;
            if (s_historyPos == -1)
                s_historyPos = static_cast<int>(s_history.size()) - 1;
            else if (s_historyPos > 0)
                s_historyPos--;

            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, s_history[s_historyPos].c_str());
        }
        else if (data->EventKey == ImGuiKey_DownArrow) {
            if (s_historyPos == -1) return 0;
            if (s_historyPos < static_cast<int>(s_history.size()) - 1) {
                s_historyPos++;
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, s_history[s_historyPos].c_str());
            }
            else {
                s_historyPos = -1;
                data->DeleteChars(0, data->BufTextLen);
            }
        }
        return 0;
    }

    void DebugOverlay::init() {
        Logger::info("DebugOverlay (ImGui) initialized.");
        addLog("=== Solace Engine Dev Console ===");
        addLog("Type 'help' for available commands. Use Up/Down arrows for history.");

        registerCommand("help", [](const std::vector<std::string>& args) {
            addLog("--- Available Commands ---");
            addLog("  help          - Show this help message");
            addLog("  clear         - Clear console log");
            addLog("  mode dev/game - Switch camera mode");
            addLog("  fps 0/1       - Toggle FPS counter overlay");
            });

        registerCommand("clear", [](const std::vector<std::string>& args) {
            s_logs.clear();
            });
    }

    void DebugOverlay::shutdown() {
        s_commands.clear();
        s_logs.clear();
        s_history.clear();
    }

    void DebugOverlay::addLog(const std::string& text) {
        if (s_logs.size() >= MAX_LOGS) {
            s_logs.erase(s_logs.begin());
        }
        s_logs.push_back(text);
        s_scrollToBottom = true;
    }

    void DebugOverlay::registerCommand(const std::string& name, ConsoleCommandFunc func) {
        s_commands[name] = func;
    }

    void DebugOverlay::executeCommand(const std::string& commandLine) {
        std::stringstream ss(commandLine);
        std::string cmdName;
        ss >> cmdName;

        std::vector<std::string> args;
        std::string arg;
        while (ss >> arg) {
            args.push_back(arg);
        }

        auto it = s_commands.find(cmdName);
        if (it != s_commands.end()) {
            it->second(args);
        }
        else {
            addLog("Unknown command: '" + cmdName + "'. Type 'help'.");
        }
    }

    void DebugOverlay::render() {
        if (!s_isOpen) return;

        ImGui::SetNextWindowSize(ImVec2(900, 320), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.1f, 0.92f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.5f, 0.8f, 0.5f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);

        if (ImGui::Begin("Solace Developer Console", &s_isOpen, ImGuiWindowFlags_NoScrollbar)) {
            const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
            if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar)) {

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
                for (const auto& log : s_logs) {
                    ImVec4 color = ImVec4(0.85f, 0.85f, 0.85f, 1.0f); 
                    if (log.find("[ERROR]") != std::string::npos) color = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); 
                    else if (log.find("[WARNING]") != std::string::npos) color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f); 
                    else if (log.find("> ") == 0) color = ImVec4(0.4f, 0.85f, 1.0f, 1.0f); 

                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(log.c_str());
                    ImGui::PopStyleColor();
                }

                if (s_scrollToBottom) {
                    ImGui::SetScrollHereY(1.0f);
                    s_scrollToBottom = false;
                }

                ImGui::PopStyleVar();
            }
            ImGui::EndChild();

            ImGui::Separator();

            bool reclaim_focus = false;
            ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackHistory;

            ImGui::PushItemWidth(-1.0f);
            if (ImGui::InputText("##ConsoleInput", s_inputBuf, IM_ARRAYSIZE(s_inputBuf), input_text_flags, &TextEditCallback)) {
                std::string s(s_inputBuf);
                if (!s.empty()) {
                    addLog("> " + s);
                    executeCommand(s);
                    s_history.push_back(s);
                    s_historyPos = -1;
                }
                strcpy_s(s_inputBuf, "");
                reclaim_focus = true;
            }
            ImGui::PopItemWidth();

            if (ImGui::IsWindowAppearing() || reclaim_focus) {
                ImGui::SetKeyboardFocusHere(-1);
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

} // namespace Solace 