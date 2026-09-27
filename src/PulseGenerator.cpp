#include "PulseGenerator.h"
#include <random>
#include <chrono>

namespace pulsetrack {

PulseGenerator::PulseGenerator(PulseCounter& counter, std::pair<int, int> intervalRangeMs)
    : counter_(counter),
      minIntervalMs_(intervalRangeMs.first),
      maxIntervalMs_(intervalRangeMs.second),
      running_(false) {
    // Defensive normalization: never let a zero/negative or inverted range
    // reach the random distribution in run() below.
    if (minIntervalMs_ < 1) {
        minIntervalMs_ = 1;
    }
    if (maxIntervalMs_ < minIntervalMs_) {
        maxIntervalMs_ = minIntervalMs_;
    }
}

PulseGenerator::~PulseGenerator() {
    stop();
}

void PulseGenerator::start() {
    if (running_.exchange(true)) {
        return; // already running; do not spawn a second thread
    }
    worker_ = std::thread(&PulseGenerator::run, this);
}

void PulseGenerator::stop() {
    if (running_.exchange(false)) {
        if (worker_.joinable()) {
            worker_.join();
        }
    }
}

void PulseGenerator::run() {
    // Each thread gets its own random engine; std::mt19937 is not
    // thread-safe to share, and there is only ever one generator thread
    // here so a thread-local engine is both correct and simple.
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(minIntervalMs_, maxIntervalMs_);

    while (running_.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rng)));
        // Re-check after sleeping so stop() during the sleep is honored
        // immediately instead of registering one extra pulse.
        if (running_.load(std::memory_order_relaxed)) {
            counter_.increment();
        }
    }
}

} // namespace pulsetrack
