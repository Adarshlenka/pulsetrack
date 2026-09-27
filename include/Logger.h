#pragma once
#include <string>
#include <fstream>
#include <mutex>

namespace pulsetrack {

// Appends timestamped lines to a log file (point 9). Throws LogFileException
// at construction if the file cannot be opened, so a bad path fails fast and
// loudly instead of silently dropping every log line. log() is guarded by an
// internal mutex so it is safe to call from multiple threads, even though
// this project only ever logs from the main thread today.
class Logger {
public:
    explicit Logger(const std::string& filePath);
    ~Logger() = default;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void log(const std::string& level, const std::string& message);

    const std::string& filePath() const { return filePath_; }

private:
    std::string filePath_;
    std::ofstream stream_;
    std::mutex mutex_;
};

} // namespace pulsetrack
