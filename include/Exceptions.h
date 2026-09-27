#pragma once
#include <stdexcept>
#include <string>

namespace pulsetrack {

// Base for every exception this project throws. Deriving everything from
// std::exception (via std::runtime_error) guarantees a single catch(const
// std::exception&) at the top of main() can never miss one -- no uncaught
// exception should ever be able to reach std::terminate.
class PulseTrackException : public std::runtime_error {
public:
    explicit PulseTrackException(const std::string& message) : std::runtime_error(message) {}
};

// Thrown when a requested meter type name/enum isn't recognized.
class InvalidMeterTypeException : public PulseTrackException {
public:
    explicit InvalidMeterTypeException(const std::string& message) : PulseTrackException(message) {}
};

// Thrown for any malformed, missing, or out-of-range command-line input.
class InvalidArgumentException : public PulseTrackException {
public:
    explicit InvalidArgumentException(const std::string& message) : PulseTrackException(message) {}
};

// Thrown when the log file cannot be opened for writing.
class LogFileException : public PulseTrackException {
public:
    explicit LogFileException(const std::string& message) : PulseTrackException(message) {}
};

} // namespace pulsetrack
