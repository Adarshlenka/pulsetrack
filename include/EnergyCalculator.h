#pragma once
#include <cstdint>
#include "Exceptions.h"

namespace pulsetrack {

// Pure conversion math (point 4-5: pulses -> energy -> power). Stateless, so
// it is trivially safe to call from any thread.
class EnergyCalculator {
public:
    // Converts a pulse count into energy in kWh for a given pulse constant.
    static double pulsesToKwh(uint64_t pulses, double pulsesPerKwh) {
        if (pulsesPerKwh <= 0.0) {
            throw InvalidArgumentException("pulsesPerKwh must be greater than zero");
        }
        return static_cast<double>(pulses) / pulsesPerKwh;
    }

    // Instantaneous power in Watts implied by `pulses` observed over
    // `elapsedSeconds`. elapsedSeconds must be > 0: a zero-length sample
    // window has no well-defined power, so this guards the division rather
    // than letting it silently produce infinity/NaN.
    static double instantaneousPowerWatts(uint64_t pulses, double pulsesPerKwh, double elapsedSeconds) {
        if (elapsedSeconds <= 0.0) {
            throw InvalidArgumentException("elapsedSeconds must be greater than zero");
        }
        const double kwh = pulsesToKwh(pulses, pulsesPerKwh);
        const double kwhPerHour = kwh * (3600.0 / elapsedSeconds);
        return kwhPerHour * 1000.0;
    }
};

} // namespace pulsetrack
