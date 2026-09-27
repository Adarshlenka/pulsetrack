#pragma once
#include <string>
#include <cstddef>
#include "SmartMeter.h"

namespace pulsetrack {

struct Options {
    MeterType meterType = MeterType::Residential;
    int durationSeconds = 20;
    std::size_t windowSize = 5;
    std::string logPath = "pulsetrack.log";
};

// Parses argv into validated Options. This is the single place untrusted
// external input (command-line arguments) enters the program, and nothing
// here is allowed to reach the rest of the code unvalidated: unknown flags,
// non-numeric values, and out-of-range values all throw
// InvalidArgumentException with a clear message rather than crashing,
// silently clamping, or being silently ignored.
class ArgParser {
public:
    static Options parse(int argc, char** argv);

    // Checked by main() before parse(), so --help/-h short-circuits to a
    // normal, successful (exit 0) usage print instead of being treated as
    // an error.
    static bool hasHelpFlag(int argc, char** argv);
    static std::string usageText();

    // Exposed separately so unit tests can exercise the validation rules
    // directly without needing a real argv.
    static int parsePositiveInt(const std::string& valueText, const std::string& argName,
                                 int minValue, int maxValue);
};

} // namespace pulsetrack
