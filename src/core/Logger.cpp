/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class.
 */

#include "core/Logger.h"
#include <ctime>
#include <iomanip>
#include <iostream>

namespace sandbox {

// Static instance pointer for singleton
static Logger* instance = nullptr;

Logger& Logger::getInstance() {
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        instance = new Logger();
    });
    return *instance;
}

Logger::Logger()
    : minLevel_(LogLevel::DEBUG)
    , initialized_(false)
{
}

Logger::~Logger() {
    shutdown();
}

void Logger::initialize(LogLevel level, const std::string& output, const std::string& logFile) {
    std::lock_guard<std::mutex> lock(mutex_);

    minLevel_ = level;
    output_ = output;
    logFile_ = logFile;

    if (!logFile_.empty()) {
        // Ensure parent directory exists
        std::filesystem::path filePath(logFile_);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }

        fileStream_.open(logFile_, std::ios::app | std::ios::out);
        if (!fileStream_.is_open()) {
            std::cerr << "Failed to open log file: " << logFile_ << std::endl;
            output_ = "stdout"; // Fallback to stdout
        }
    }

    initialized_ = true;
    info("Logger initialized with level: " + logLevelToString(level));
}

void Logger::log(LogLevel level, const std::string& message,
                 const std::string& file, int line) {
    if (!initialized_) {
        std::cerr << "[UNINITIALIZED] " << message << std::endl;
        return;
    }

    if (level < minLevel_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::string formatted = formatMessage(level, message, file, line);

    if (output_ == "stdout") {
        std::cout << formatted << std::endl;
    } else if (output_ == "stderr") {
        std::cerr << formatted << std::endl;
    } else {
        if (fileStream_.is_open()) {
            fileStream_ << formatted << std::endl;
            fileStream_.flush();
        } else {
            std::cerr << formatted << std::endl;
        }
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message,
                                  const std::string& file, int line) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << "["
        << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms.count() << "]"
        << " [" << std::setw(8) << logLevelToString(level) << "]";

    if (!file.empty()) {
        oss << " [" << file << ":" << line << "]";
    }

    oss << " " << message;

    return oss.str();
}

void Logger::debug(const std::string& message, const std::string& file, int line) {
    log(LogLevel::DEBUG, message, file, line);
}

void Logger::info(const std::string& message, const std::string& file, int line) {
    log(LogLevel::INFO, message, file, line);
}

void Logger::warning(const std::string& message, const std::string& file, int line) {
    log(LogLevel::WARNING, message, file, line);
}

void Logger::error(const std::string& message, const std::string& file, int line) {
    log(LogLevel::ERROR, message, file, line);
}

void Logger::critical(const std::string& message, const std::string& file, int line) {
    log(LogLevel::CRITICAL, message, file, line);
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    minLevel_ = level;
}

LogLevel Logger::getLevel() const {
    return minLevel_;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fileStream_.is_open()) {
        fileStream_.flush();
    }
    std::cout.flush();
    std::cerr.flush();
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fileStream_.is_open()) {
        fileStream_.close();
    }
    initialized_ = false;
}

} // namespace sandbox
