#pragma once
#include <deque>
#include <numeric>
#include <type_traits>
#include <cstddef>

namespace pulsetrack {

// Generic fixed-size sliding window with a running average (points 5 and 8:
// recent/average usage via a sliding window). Templated over the reading
// type to demonstrate C++ templates + STL containers, guarded so it can only
// ever be instantiated for arithmetic types.
template <typename T>
class SlidingWindowAnalyzer {
    static_assert(std::is_arithmetic<T>::value, "SlidingWindowAnalyzer requires an arithmetic type");

public:
    // A windowSize of 0 would make the window permanently empty and every
    // average() call meaningless, so it is clamped to at least 1.
    explicit SlidingWindowAnalyzer(std::size_t windowSize)
        : windowSize_(windowSize == 0 ? 1 : windowSize) {}

    void addReading(T value) {
        buffer_.push_back(value);
        while (buffer_.size() > windowSize_) {
            buffer_.pop_front();
        }
    }

    // Average of the readings currently in the window. Returns 0 when the
    // window is empty rather than dividing by zero, so callers never need
    // to special-case a fresh/empty window themselves.
    double average() const {
        if (buffer_.empty()) {
            return 0.0;
        }
        const double sum = std::accumulate(buffer_.begin(), buffer_.end(), 0.0);
        return sum / static_cast<double>(buffer_.size());
    }

    std::size_t size() const { return buffer_.size(); }
    std::size_t capacity() const { return windowSize_; }
    bool empty() const { return buffer_.empty(); }

private:
    std::size_t windowSize_;
    std::deque<T> buffer_;
};

} // namespace pulsetrack
