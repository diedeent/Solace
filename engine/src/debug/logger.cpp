#include <engine/debug/logger.hpp>

namespace Solace {

    LogLevel Logger::s_minLevel = LogLevel::Trace;
    std::ofstream Logger::s_fileStream;
    std::mutex Logger::s_mutex;

    void Logger::init(const std::string& logFilePath, LogLevel minLevel) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_minLevel = minLevel;

        std::filesystem::path path(logFilePath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }

        s_fileStream.open(logFilePath, std::ios::out | std::ios::trunc);
        if (!s_fileStream.is_open()) {
            std::cerr << "[WARNING] Failed to open log file: " << logFilePath << "\n";
        }
        else {
            s_fileStream << "=== Solace Engine Log Started ===\n";
        }
    }

    void Logger::shutdown() {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_fileStream.is_open()) {
            s_fileStream << "=== Solace Engine Log Closed ===\n";
            s_fileStream.close();
        }
    }

    void Logger::write(LogLevel level, const std::source_location& loc, const std::string& msg) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        struct tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time_t);
#else
        localtime_r(&time_t, &tm_buf);
#endif

        std::string_view filePath = loc.file_name();
        size_t lastSlash = filePath.find_last_of("/\\");
        std::string_view fileName = (lastSlash != std::string_view::npos) ? filePath.substr(lastSlash + 1) : filePath;

        std::string logEntry = std::format(
            "[{:02}:{:02}:{:02}.{:03}] [{}] [{}:{}] {}\n",
            tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec, (int)ms.count(),
            level_to_string(level), fileName, loc.line(), msg
        );

        std::lock_guard<std::mutex> lock(s_mutex);

        // Консоль (с цветами)
        std::ostream& out = (level >= LogLevel::Error) ? std::cerr : std::cout;
        out << level_to_color(level) << logEntry << "\033[0m";

        // Файл (без цветов, с flush для сохранения при краше)
        if (s_fileStream.is_open()) {
            s_fileStream << logEntry;
            s_fileStream.flush();
        }
    }

    std::string_view Logger::level_to_string(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:    return "TRACE";
        case LogLevel::Debug:    return "DEBUG";
        case LogLevel::Info:     return "INFO";
        case LogLevel::Warn:     return "WARN";
        case LogLevel::Error:    return "ERROR";
        case LogLevel::Critical: return "CRITICAL";
        default:                 return "UNKNOWN";
        }
    }

    std::string_view Logger::level_to_color(LogLevel level) {
        switch (level) {
        case LogLevel::Trace:    return "\033[90m"; // Серый
        case LogLevel::Debug:    return "\033[36m"; // Голубой
        case LogLevel::Info:     return "\033[32m"; // Зелёный
        case LogLevel::Warn:     return "\033[33m"; // Жёлтый
        case LogLevel::Error:    return "\033[31m"; // Красный
        case LogLevel::Critical: return "\033[41;97m"; // Белый на красном фоне
        default:                 return "\033[0m";
        }
    }

} // namespace Solace