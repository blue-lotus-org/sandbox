/**
 * @file Logger.h
 * @brief Thread-safe logging facility for the sandbox platform.
 *
 * This header defines the Logger class that provides structured,
 * thread-safe logging capabilities throughout the sandbox platform.
 * It supports multiple log levels and output destinations.
 */

#ifndef SANDBOX_LOGGER_H
#define SANDBOX_LOGGER_H

#include <string>
#include <chrono>
#include <mutex>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>

namespace sandbox {

/**
 * @enum LogLevel
 * @brief Enumeration of supported log levels.
 */
enum class LogLevel {
    DEBUG,      ///< Detailed information for debugging
    INFO,       ///< General informational messages
    WARNING,    ///< Warning conditions
    ERROR,      ///< Error conditions
    CRITICAL    ///< Critical conditions requiring immediate attention
};

/**
 * @brief Convert LogLevel to string representation.
 * @param level The log level to convert.
 * @return String representation of the log level.
 */
inline std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:    return "DEBUG";
        case LogLevel::INFO:     return "INFO";
        case LogLevel::WARNING:  return "WARNING";
        case LogLevel::ERROR:    return "ERROR";
        case LogLevel::CRITICAL: return "CRITICAL";
        default:                 return "UNKNOWN";
    }
}

/**
 * @brief Convert string to LogLevel.
 * @param str The string representation of the log level.
 * @return The corresponding LogLevel, or INFO if not recognized.
 */
inline LogLevel stringToLogLevel(const std::string& str) {
    if (str == "DEBUG") return LogLevel::DEBUG;
    if (str == "INFO") return LogLevel::INFO;
    if (str == "WARNING") return LogLevel::WARNING;
    if (str == "ERROR") return LogLevel::ERROR;
    if (str == "CRITICAL") return LogLevel::CRITICAL;
    return LogLevel::INFO;
}

/**
 * @class Logger
 * @brief Provides thread-safe logging functionality.
 *
 * The Logger class implements a singleton pattern to provide
 * global logging access throughout the application. It supports
 * console and file output with configurable log levels.
 */
class Logger {
public:
    /**
     * @brief Get the singleton instance of the Logger.
     * @return Reference to the Logger instance.
     */
    static Logger& getInstance();

    /**
     * @brief Deleted copy constructor to prevent copying.
     */
    Logger(const Logger&) = delete;

    /**
     * @brief Deleted assignment operator to prevent copying.
     */
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Initialize the logger with configuration.
     * @param level Minimum log level to output.
     * @param output Output destination ("stdout", "stderr", or file path).
     * @param logFile Path to log file (optional).
     */
    void initialize(LogLevel level, const std::string& output, const std::string& logFile = "");

    /**
     * @brief Log a message with the specified level.
     * @param level The log level for this message.
     * @param message The message to log.
     * @param file Source file name (optional, auto-filled by macro).
     * @param line Source line number (optional, auto-filled by macro).
     */
    void log(LogLevel level, const std::string& message,
             const std::string& file = "", int line = 0);

    /**
     * @brief Log a debug message.
     * @param message The message to log.
     */
    void debug(const std::string& message, const std::string& file = "", int line = 0);

    /**
     * @brief Log an info message.
     * @param message The message to log.
     */
    void info(const std::string& message, const std::string& file = "", int line = 0);

    /**
     * @brief Log a warning message.
     * @param message The message to log.
     */
    void warning(const std::string& message, const std::string& file = "", int line = 0);

    /**
     * @brief Log an error message.
     * @param message The message to log.
     */
    void error(const std::string& message, const std::string& file = "", int line = 0);

    /**
     * @brief Log a critical message.
     * @param message The message to log.
     */
    void critical(const std::string& message, const std::string& file = "", int line = 0);

    /**
     * @brief Set the minimum log level.
     * @param level The new minimum log level.
     */
    void setLevel(LogLevel level);

    /**
     * @brief Get the current minimum log level.
     * @return The current log level.
     */
    LogLevel getLevel() const;

    /**
     * @brief Flush any pending log output.
     */
    void flush();

    /**
     * @brief Shutdown the logger and close file handles.
     */
    void shutdown();

private:
    /**
     * @brief Private constructor for singleton pattern.
     */
    Logger();

    /**
     * @brief Private destructor.
     */
    ~Logger();

    /**
     * @brief Format a log message with timestamp and level.
     * @param level The log level.
     * @param message The message content.
     * @param file Source file name.
     * @param line Source line number.
     * @return Formatted log message.
     */
    std::string formatMessage(LogLevel level, const std::string& message,
                              const std::string& file, int line);

    LogLevel minLevel_;              ///< Minimum log level to output
    std::string output_;             ///< Output destination
    std::string logFile_;            ///< Path to log file
    std::mutex mutex_;               ///< Mutex for thread safety
    std::ofstream fileStream_;       ///< File output stream
    bool initialized_;               ///< Initialization flag
};

/**
 * @brief Convenience macro for logging with source location.
 */
#define SANDBOX_LOG(level, message) \
    Logger::getInstance().log(level, message, __FILE__, __LINE__)

/**
 * @brief Convenience macro for debug logging.
 */
#define SANDBOX_DEBUG(message) \
    Logger::getInstance().debug(message, __FILE__, __LINE__)

/**
 * @brief Convenience macro for info logging.
 */
#define SANDBOX_INFO(message) \
    Logger::getInstance().info(message, __FILE__, __LINE__)

/**
 * @brief Convenience macro for warning logging.
 */
#define SANDBOX_WARNING(message) \
    Logger::getInstance().warning(message, __FILE__, __LINE__)

/**
 * @brief Convenience macro for error logging.
 */
#define SANDBOX_ERROR(message) \
    Logger::getInstance().error(message, __FILE__, __LINE__)

/**
 * @brief Convenience macro for critical logging.
 */
#define SANDBOX_CRITICAL(message) \
    Logger::getInstance().critical(message, __FILE__, __LINE__)

} // namespace sandbox

#endif // SANDBOX_LOGGER_H
