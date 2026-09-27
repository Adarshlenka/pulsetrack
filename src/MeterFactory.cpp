#include "MeterFactory.h"
#include "Exceptions.h"
#include <algorithm>
#include <cctype>

namespace pulsetrack {

std::unique_ptr<SmartMeter> MeterFactory::create(MeterType type) {
    switch (type) {
        case MeterType::Residential: return std::make_unique<ResidentialMeter>();
        case MeterType::Commercial:  return std::make_unique<CommercialMeter>();
        case MeterType::Industrial:  return std::make_unique<IndustrialMeter>();
    }
    // Defensive: only reachable if MeterType is ever extended without
    // updating this switch. Throwing here (rather than returning null)
    // keeps "MeterFactory::create never returns null" an invariant callers
    // can rely on.
    throw InvalidMeterTypeException("Unhandled MeterType enum value");
}

std::unique_ptr<SmartMeter> MeterFactory::create(const std::string& typeName) {
    std::string lower = typeName;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "residential") return create(MeterType::Residential);
    if (lower == "commercial")  return create(MeterType::Commercial);
    if (lower == "industrial")  return create(MeterType::Industrial);

    throw InvalidMeterTypeException(
        "Unknown meter type '" + typeName + "'. Valid options: residential, commercial, industrial.");
}

} // namespace pulsetrack
