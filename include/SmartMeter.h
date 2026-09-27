#pragma once
#include <string>
#include <utility>

namespace pulsetrack {

enum class MeterType {
    Residential,
    Commercial,
    Industrial
};

// Abstract base for all meter types (point 7: Factory Pattern support).
// Each concrete meter supplies the physical constants that make its
// simulated pulses behave differently: a household meter pulses slowly at
// low power, an industrial meter pulses fast at high power. Values below
// are illustrative simulation constants chosen to produce realistic-looking
// behavior, not a calibrated real hardware spec sheet.
class SmartMeter {
public:
    virtual ~SmartMeter() = default;

    virtual MeterType type() const = 0;
    virtual std::string typeName() const = 0;

    // Pulses that represent exactly 1 kWh of energy for this meter class.
    virtual double pulsesPerKwh() const = 0;

    // Average power (Watts) above which consumption counts as "high".
    virtual double highConsumptionThresholdWatts() const = 0;

    // Inclusive [min, max] milliseconds between simulated pulses.
    virtual std::pair<int, int> pulseIntervalRangeMs() const = 0;
};

class ResidentialMeter : public SmartMeter {
public:
    MeterType type() const override { return MeterType::Residential; }
    std::string typeName() const override { return "Residential"; }
    double pulsesPerKwh() const override { return 1000.0; }
    double highConsumptionThresholdWatts() const override { return 6500.0; }
    std::pair<int, int> pulseIntervalRangeMs() const override { return {300, 900}; }
};

class CommercialMeter : public SmartMeter {
public:
    MeterType type() const override { return MeterType::Commercial; }
    std::string typeName() const override { return "Commercial"; }
    double pulsesPerKwh() const override { return 800.0; }
    double highConsumptionThresholdWatts() const override { return 30000.0; }
    std::pair<int, int> pulseIntervalRangeMs() const override { return {80, 300}; }
};

class IndustrialMeter : public SmartMeter {
public:
    MeterType type() const override { return MeterType::Industrial; }
    std::string typeName() const override { return "Industrial"; }
    double pulsesPerKwh() const override { return 400.0; }
    double highConsumptionThresholdWatts() const override { return 120000.0; }
    std::pair<int, int> pulseIntervalRangeMs() const override { return {40, 150}; }
};

} // namespace pulsetrack
