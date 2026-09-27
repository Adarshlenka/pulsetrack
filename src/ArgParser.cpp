#include "ArgParser.h"
#include "MeterFactory.h"
#include "Exceptions.h"
#include <stdexcept>

namespace pulsetrack {

namespace {

std::string extractValue(const std::string& arg, const std::string& key) {
    // arg looks like "--key=value"; caller has already checked the prefix.
    return arg.substr(key.size());
}

} // namespace

bool ArgParser::hasHelpFlag(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            return true;
        }
    }
    return false;
}

std::string ArgParser::usageText() {
    return "Usage: pulsetrack [--meter=residential|commercial|industrial] "
           "[--duration=SECONDS] [--window=N] [--log=PATH]";
}

int ArgParser::parsePositiveInt(const std::string& valueText, const std::string& argName,
                                 int minValue, int maxValue) {
    if (valueText.empty()) {
        throw InvalidArgumentException(argName + " requires a value");
    }

    int parsed = 0;
    try {
        std::size_t consumed = 0;
        parsed = std::stoi(valueText, &consumed);
        if (consumed != valueText.size()) {
            // Rejects trailing garbage like "20abc" that std::stoi would
            // otherwise silently accept by parsing only the numeric prefix.
            throw InvalidArgumentException(argName + " must be a whole number, got '" + valueText + "'");
        }
    } catch (const std::invalid_argument&) {
        throw InvalidArgumentException(argName + " must be a whole number, got '" + valueText + "'");
    } catch (const std::out_of_range&) {
        throw InvalidArgumentException(argName + " value is out of range: '" + valueText + "'");
    }

    if (parsed < minValue || parsed > maxValue) {
        throw InvalidArgumentException(
            argName + " must be between " + std::to_string(minValue) + " and " + std::to_string(maxValue) +
            ", got " + std::to_string(parsed));
    }
    return parsed;
}

Options ArgParser::parse(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg.rfind("--meter=", 0) == 0) {
            const std::string value = extractValue(arg, "--meter=");
            // Reuses the factory's own validation so there is exactly one
            // place in the codebase that knows which meter names are valid.
            auto meter = MeterFactory::create(value);
            options.meterType = meter->type();
        } else if (arg.rfind("--duration=", 0) == 0) {
            options.durationSeconds =
                parsePositiveInt(extractValue(arg, "--duration="), "--duration", 1, 3600);
        } else if (arg.rfind("--window=", 0) == 0) {
            options.windowSize = static_cast<std::size_t>(
                parsePositiveInt(extractValue(arg, "--window="), "--window", 1, 100));
        } else if (arg.rfind("--log=", 0) == 0) {
            std::string value = extractValue(arg, "--log=");
            if (value.empty()) {
                throw InvalidArgumentException("--log requires a file path");
            }
            options.logPath = value;
        } else if (arg == "--help" || arg == "-h") {
            // Already handled by hasHelpFlag() before parse() is called in
            // normal use; this branch only matters if parse() is called
            // directly (e.g. from a test) without that check first.
            throw InvalidArgumentException(usageText());
        } else {
            throw InvalidArgumentException("Unknown argument: '" + arg + "'");
        }
    }

    return options;
}

} // namespace pulsetrack
