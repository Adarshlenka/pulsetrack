#pragma once
#include <atomic>
#include <thread>
#include <utility>
#include "PulseCounter.h"

namespace pulsetrack {

// Simulates a meter's electrical pulses (point 1) on a background thread, at
// randomized intervals within a [min, max] millisecond range. RAII: the
// destructor always stops and joins the worker thread, so a PulseGenerator
// can never outlive or leak its thread, even if an exception unwinds the
// stack while it is running.
class PulseGenerator {
public:
    PulseGenerator(PulseCounter& counter, std::pair<int, int> intervalRangeMs);
    ~PulseGenerator();

    // Not copyable/movable: it owns a running thread tied to `counter_`.
    PulseGenerator(const PulseGenerator&) = delete;
    PulseGenerator& operator=(const PulseGenerator&) = delete;

    void start();
    void stop();

private:
    void run();

    PulseCounter& counter_;
    int minIntervalMs_;
    int maxIntervalMs_;
    std::atomic<bool> running_;
    std::thread worker_;
};

} // namespace pulsetrack
