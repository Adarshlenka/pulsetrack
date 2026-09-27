#pragma once

namespace pulsetrack {

// Flags whether a given average power reading counts as "high consumption"
// for a meter's threshold (point 6). Deliberately simple: a direct
// comparison with no hidden state, so its behavior is fully described by
// one line and one boundary rule: exactly-at-threshold is NOT high, only
// strictly above it is.
class ConsumptionMonitor {
public:
    explicit ConsumptionMonitor(double thresholdWatts) : thresholdWatts_(thresholdWatts) {}

    bool isHighConsumption(double averagePowerWatts) const {
        return averagePowerWatts > thresholdWatts_;
    }

    double threshold() const { return thresholdWatts_; }

private:
    double thresholdWatts_;
};

} // namespace pulsetrack
