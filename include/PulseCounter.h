#pragma once
#include <atomic>
#include <cstdint>

namespace pulsetrack {

// Thread-safe pulse counter (points 2-3: read and count pulses). One thread
// (the generator) increments it while another (the reader) periodically
// drains it. Both operations are lock-free atomics with relaxed ordering,
// which is sufficient here because the two counters are independent tallies
// with no other memory they need to synchronize -- there is no data race.
class PulseCounter {
public:
    PulseCounter() : intervalCount_(0), totalCount_(0) {}

    void increment() {
        intervalCount_.fetch_add(1, std::memory_order_relaxed);
        totalCount_.fetch_add(1, std::memory_order_relaxed);
    }

    // Atomically reads and resets the count accumulated since the last call.
    uint64_t fetchAndResetInterval() {
        return intervalCount_.exchange(0, std::memory_order_relaxed);
    }

    // Cumulative pulses since construction; this one is never reset.
    uint64_t total() const {
        return totalCount_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<uint64_t> intervalCount_;
    std::atomic<uint64_t> totalCount_;
};

} // namespace pulsetrack
