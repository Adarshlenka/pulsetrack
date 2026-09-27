#pragma once
#include <memory>
#include <string>
#include "SmartMeter.h"

namespace pulsetrack {

// Factory Pattern (point 7): centralizes meter construction so the rest of
// the program only ever asks for "a meter of this type" and never needs to
// know, or spell out, the concrete subclass name.
class MeterFactory {
public:
    static std::unique_ptr<SmartMeter> create(MeterType type);

    // Parses a case-insensitive name ("residential", "Commercial", ...).
    // Throws InvalidMeterTypeException for anything else -- never returns
    // a null pointer.
    static std::unique_ptr<SmartMeter> create(const std::string& typeName);
};

} // namespace pulsetrack
