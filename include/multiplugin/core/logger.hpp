/**
 * @file logger.hpp
 * @brief Lightweight, dependency-free logging utilities with stream syntax
 *
 * Usage examples:
 *   LOG_DEBUG << "Processing frame: " << frameNumber;
 *   LOG_INFO << "Plugin loaded";
 *   LOG_ERR << "Failed to allocate memory";
 */

#ifndef MULTIPLUGIN_CORE_LOGGER_HPP
#define MULTIPLUGIN_CORE_LOGGER_HPP

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace mp {
namespace logger {

/// Log levels
enum class lvl : std::uint8_t {
    trace,
    debug,
    info,
    warn,
    err,
    critical,
    off
};

/// Default log level based on build type
#ifdef NDEBUG
constexpr lvl kDefaultLogLevel = lvl::info;    // Release: info and above
#else
constexpr lvl kDefaultLogLevel = lvl::trace;   // Debug: everything
#endif

namespace detail {
    struct Settings {
        std::string folder = "TiM";                   // Folder name in AppData/Library
        std::string base   = "MultiPlugin";           // Log file base name
        lvl         min_level = kDefaultLogLevel;     // Minimum level to write
    };

    inline Settings &settings() {
        static Settings s;
        return s;
    }

    inline std::mutex &settings_mutex() {
        static std::mutex m;
        return m;
    }

    /// Get platform-specific app data directory
    inline std::filesystem::path roaming_base() {
#ifdef _WIN32
        if (const char *p = std::getenv("APPDATA")) return std::filesystem::path(p);
        if (const char *t = std::getenv("TEMP")) return std::filesystem::path(t);
        return std::filesystem::temp_directory_path();
#else
        if (const char *home = std::getenv("HOME")) {
#  ifdef __APPLE__
            return std::filesystem::path(home) / "Library" / "Application Support";
#  else
            return std::filesystem::path(home) / ".config";
#  endif
        }
        return std::filesystem::temp_directory_path();
#endif
    }

    /// Get logs directory, creating it if needed
    inline std::filesystem::path logs_dir() {
        const auto &s = settings();
        auto base = roaming_base() / s.folder;
        std::error_code ec;
        std::filesystem::create_directories(base, ec);
        return base;
    }

    /// Get full path to log file
    inline std::filesystem::path log_path() {
        const auto &s = settings();
        return logs_dir() / (s.base + ".log");
    }

    inline bool level_enabled(lvl level) {
        const auto min_level = settings().min_level;
        return static_cast<std::uint8_t>(level) >= static_cast<std::uint8_t>(min_level)
               && min_level != lvl::off && level != lvl::off;
    }

    inline const char *level_tag(lvl level) {
        switch (level) {
            case lvl::trace:    return "trace";
            case lvl::debug:    return "debug";
            case lvl::info:     return "info";
            case lvl::warn:     return "warn";
            case lvl::err:      return "err";
            case lvl::critical: return "critical";
            case lvl::off:      return "off";
        }
        return "unknown";
    }

    inline void write_line(lvl level, const std::string &message) {
        if (!level_enabled(level)) return;

        // Optimized timestamp with thread-local caching
        static thread_local std::chrono::system_clock::time_point last_time;
        static thread_local std::string cached_time_str;

        const auto now = std::chrono::system_clock::now();
        if (cached_time_str.empty() ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() >= 10) {
            last_time = now;
            const auto t = std::chrono::system_clock::to_time_t(now);
            const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()).count() % 1000000;

            std::tm tm_time = *std::localtime(&t);
            cached_time_str.clear();
            cached_time_str.reserve(32);
            cached_time_str = '[';
            cached_time_str += std::to_string(1900 + tm_time.tm_year) + '-';
            cached_time_str += (tm_time.tm_mon + 1 < 10 ? "0" : "") + std::to_string(tm_time.tm_mon + 1) + '-';
            cached_time_str += (tm_time.tm_mday < 10 ? "0" : "") + std::to_string(tm_time.tm_mday) + ' ';
            cached_time_str += (tm_time.tm_hour < 10 ? "0" : "") + std::to_string(tm_time.tm_hour) + ':';
            cached_time_str += (tm_time.tm_min < 10 ? "0" : "") + std::to_string(tm_time.tm_min) + ':';
            cached_time_str += (tm_time.tm_sec < 10 ? "0" : "") + std::to_string(tm_time.tm_sec);
            cached_time_str += '.' + std::string(6 - std::to_string(micros).length(), '0') + std::to_string(micros) + ']';
        }

        // Keep file open for better performance
        static std::mutex file_mutex;
        static std::ofstream file;
        static bool initialized = false;

        {
            std::lock_guard<std::mutex> lk(file_mutex);
            if (!initialized) {
                const auto path = log_path();
                file.open(path, std::ios::out | std::ios::app);
                if (file.is_open()) {
                    file.rdbuf()->pubsetbuf(nullptr, 0); // Unbuffered for immediate writes
                }
                initialized = true;
            }

            if (file.is_open()) {
                file << cached_time_str << " [" << level_tag(level) << "] [thread "
                     << std::this_thread::get_id() << "] " << message << '\n';
            }
        }

#ifndef NDEBUG
        // In debug builds, also output to stderr/debugger
        static std::mutex debug_mutex;
        std::lock_guard<std::mutex> lk(debug_mutex);

        std::ostringstream debug_stream;
        debug_stream << "[" << settings().base << "] "
                     << "[" << level_tag(level) << "] [thread "
                     << std::this_thread::get_id() << "] " << message << '\n';
        const std::string debug_msg = debug_stream.str();

#  ifdef _WIN32
        OutputDebugStringA(debug_msg.c_str());
#  endif
        // Debugging to stderr just in case
        std::cerr << debug_msg;
        std::cerr.flush();
#endif
    }
} // namespace detail

/// Set folder name for logs (e.g., "TiM" or "MyCompany")
inline void set_folder(const std::string &name) {
    std::lock_guard<std::mutex> lk(detail::settings_mutex());
    detail::settings().folder = name;
}

/// Set log file base name (e.g., "MultiPlugin" becomes "MultiPlugin.log")
inline void set_log_base(const std::string &name) {
    std::lock_guard<std::mutex> lk(detail::settings_mutex());
    detail::settings().base = name;
}

/// Set minimum log level
inline void set_min_level(lvl level) {
    std::lock_guard<std::mutex> lk(detail::settings_mutex());
    detail::settings().min_level = level;
}

/// Get logs directory path
inline std::string logs_directory() { return detail::logs_dir().string(); }

/// Get full log file path
inline std::string log_file_path() { return detail::log_path().string(); }

/// Stream-style logger object
class stream_logger {
public:
    explicit stream_logger(lvl level) : level_(level) {}
    ~stream_logger() {
        try {
            const std::string msg = stream_.str();
            if (!msg.empty()) detail::write_line(level_, msg);
        } catch (...) {
            // Swallow exceptions in destructor
        }
    }

    template <typename T>
    stream_logger &operator<<(const T &value) {
        stream_ << value;
        return *this;
    }

private:
    std::ostringstream stream_{};
    lvl                 level_;
};

inline stream_logger log(lvl level = lvl::info) { return stream_logger(level); }

} // namespace logger
} // namespace mp

// Macros for convenient logging

// Get filename without path
#ifdef _WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define FILE_INFO (std::string(__FILENAME__) + ":" + std::to_string(__LINE__))
#define DEBUG_INFO "[" << FILE_INFO << ":" << __FUNCTION__ << "] "

// Stream-style logging macros
#define LOG_TRACE    (::mp::logger::log(::mp::logger::lvl::trace) << DEBUG_INFO)
#define LOG_DEBUG    (::mp::logger::log(::mp::logger::lvl::debug) << DEBUG_INFO)
#define LOG_INFO     ::mp::logger::log(::mp::logger::lvl::info)
#define LOG_WARN     ::mp::logger::log(::mp::logger::lvl::warn)

#ifndef NDEBUG
#define LOG_ERR      (::mp::logger::log(::mp::logger::lvl::err) << DEBUG_INFO)
#else
#define LOG_ERR      ::mp::logger::log(::mp::logger::lvl::err)
#endif

#define LOG_CRITICAL ::mp::logger::log(::mp::logger::lvl::critical)

// Backward compatibility alias
#define LOG LOG_INFO

// Timing helpers
#define LOG_TIME_START(name) const auto name = std::chrono::high_resolution_clock::now();
#define LOG_TIME_END_MSG(name, message) \
    LOG_DEBUG << message << " took " \
              << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - name).count() \
              << "ms";
#define LOG_TIME_END(name) LOG_TIME_END_MSG(name, #name)

#endif // MULTIPLUGIN_CORE_LOGGER_HPP