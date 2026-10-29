#pragma once

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace Solace {

    using ConsoleCommandFunc = std::function<void(const std::vector<std::string>& args)>;

    class DebugOverlay {
    public:
        static void init();
        static void shutdown();

        static void toggle() { s_isOpen = !s_isOpen; }
        static bool isOpen() { return s_isOpen; }

        static void registerCommand(const std::string& name, ConsoleCommandFunc func);
        static void executeCommand(const std::string& commandLine);
        static void addLog(const std::string& text);

        static void render();

    private:
        static bool s_isOpen;
        static char s_inputBuf[256];
        static std::vector<std::string> s_logs;
        static std::unordered_map<std::string, ConsoleCommandFunc> s_commands;
        static bool s_scrollToBottom;

        const static size_t MAX_LOGS = 100;
    };

} // namespace Solace 