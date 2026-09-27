#include "Logger.h"
#include "Exceptions.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace pulsetrack {

Logger::Logger(const std::string& filePath) : filePath_(filePath) {
    stream_.open(filePath_, std::ios::out | std::ios::app);
    if (!stream_.is_open()) {
        throw LogFileException("Could not open log file for writing: " + filePath_);
    }
}

void Logger::log(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTime);
#else
    localtime_r(&nowTime, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S") << " [" << level << "] " << message;

    stream_ << oss.str() << std::endl;
}

} // namespace pulsetrack
