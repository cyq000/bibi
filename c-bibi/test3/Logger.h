#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <mutex>

class Logger {
public:
    enum Level {
        INFO,
        WARNING,
        ERROR,
        DEBUG
    };

    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (ofs_.is_open()) {
            ofs_.close();
        }
        ofs_.open(filename, std::ios::app);
    }

    void log(Level level, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ostringstream oss;
        oss << "[" << timestamp() << "] "
            << levelToString(level) << " "
            << msg << "\n";

        // 输出到终端
        std::cout << oss.str();

        // 输出到文件
        if (ofs_.is_open()) {
            ofs_ << oss.str();
            ofs_.flush();
        }
    }

private:
    std::ofstream ofs_;
    std::mutex mutex_;

    Logger() {}
    ~Logger() {
        if (ofs_.is_open()) ofs_.close();
    }

    std::string timestamp() {
        std::time_t now = std::time(nullptr);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return buf;
    }

    std::string levelToString(Level level) {
        switch (level) {
            case INFO: return "[INFO]";
            case WARNING: return "[WARNING]";
            case ERROR: return "[ERROR]";
            case DEBUG: return "[DEBUG]";
        }
        return "[UNKNOWN]";
    }
};

// 宏定义简化调用
#define LOG_INFO(msg) Logger::instance().log(Logger::INFO, msg)
#define LOG_WARN(msg) Logger::instance().log(Logger::WARNING, msg)
#define LOG_ERROR(msg) Logger::instance().log(Logger::ERROR, msg)
#define LOG_DEBUG(msg) Logger::instance().log(Logger::DEBUG, msg)

#endif // LOGGER_H
