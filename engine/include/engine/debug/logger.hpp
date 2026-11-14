#pragma once

#include <iostream>
#include <fstream>
#include <mutex>
#include <string>
#include <string_view>
#include <source_location>
#include <chrono>
#include <format>
#include <filesystem>
#include <sstream>
#include <utility>

namespace Solace {

    enum class LogLevel {
        Trace = 0,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
        Off
    };

    class Logger {
    public:
        static void init(const std::string& logFilePath = "logs/solace.log", LogLevel minLevel = LogLevel::Trace);
        static void shutdown();

        static void set_level(LogLevel level) { s_minLevel = level; }
        static LogLevel get_level() { return s_minLevel; }

        // --- ГЛАВНЫЙ МЕТОД (гибридный) ---
        template<typename... Args>
        static void log(LogLevel level, const std::source_location& loc, std::string_view fmt, Args&&... args) {
            // Быстрая отсечка по уровню (не тратим CPU на форматирование)
            if (level < s_minLevel) return;

            std::string message;

            if constexpr (sizeof...(args) == 0) {
                // Нет аргументов — просто строка (работает тернарный оператор)
                message = std::string(fmt);
            }
            else {
                bool formatted = false;
                // Если в строке есть '{', значит это std::format строка
                if (fmt.find('{') != std::string_view::npos) {
                    try {
                        message = std::vformat(fmt, std::make_format_args(args...));
                        formatted = true;
                    }
                    catch (const std::format_error&) {
                        // Если форматирование не удалось (например, одиночная '{'), 
                        // молча падаем в fallback ниже
                    }
                }

                // Fallback: старый добрый stream-стиль (склеивание)
                if (!formatted) {
                    std::ostringstream oss;
                    oss << fmt;
                    (oss << ... << args);
                    message = oss.str();
                }
            }

            write(level, loc, message);
        }

        // --- LEGACY API (для обратной совместимости) ---
        template<typename... Args>
        static void trace(Args&&... args) {
            log(LogLevel::Trace, std::source_location::current(), std::forward<Args>(args)...);
        }
        template<typename... Args>
        static void debug(Args&&... args) {
            log(LogLevel::Debug, std::source_location::current(), std::forward<Args>(args)...);
        }
        template<typename... Args>
        static void info(Args&&... args) {
            log(LogLevel::Info, std::source_location::current(), std::forward<Args>(args)...);
        }
        template<typename... Args>
        static void warning(Args&&... args) {
            log(LogLevel::Warn, std::source_location::current(), std::forward<Args>(args)...);
        }
        template<typename... Args>
        static void error(Args&&... args) {
            log(LogLevel::Error, std::source_location::current(), std::forward<Args>(args)...);
        }
        template<typename... Args>
        static void critical(Args&&... args) {
            log(LogLevel::Critical, std::source_location::current(), std::forward<Args>(args)...);
        }

    private:
        static void write(LogLevel level, const std::source_location& loc, const std::string& msg);
        static std::string_view level_to_string(LogLevel level);
        static std::string_view level_to_color(LogLevel level);

        static LogLevel s_minLevel;
        static std::ofstream s_fileStream;
        static std::mutex s_mutex;
    };

} // namespace Solace

// --- МАКРОСЫ (для нового стиля) ---
#define SOLACE_TRACE(...)    ::Solace::Logger::log(::Solace::LogLevel::Trace,    std::source_location::current(), __VA_ARGS__)
#define SOLACE_DEBUG(...)    ::Solace::Logger::log(::Solace::LogLevel::Debug,    std::source_location::current(), __VA_ARGS__)
#define SOLACE_INFO(...)     ::Solace::Logger::log(::Solace::LogLevel::Info,     std::source_location::current(), __VA_ARGS__)
#define SOLACE_WARN(...)     ::Solace::Logger::log(::Solace::LogLevel::Warn,     std::source_location::current(), __VA_ARGS__)
#define SOLACE_ERROR(...)    ::Solace::Logger::log(::Solace::LogLevel::Error,    std::source_location::current(), __VA_ARGS__)
#define SOLACE_CRITICAL(...) ::Solace::Logger::log(::Solace::LogLevel::Critical, std::source_location::current(), __VA_ARGS__)