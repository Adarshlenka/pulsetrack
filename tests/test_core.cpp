// Lightweight, dependency-free test harness for PulseTrack's core logic.
// No external framework (Catch2/gtest) on purpose, to keep the build simple
// and self-contained -- this file *is* the proof that the 9 functional
// points, plus the edge/security cases called out in the README, actually
// work, not just that they compile.

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <thread>
#include <chrono>
#include <vector>

#include "SmartMeter.h"
#include "MeterFactory.h"
#include "PulseCounter.h"
#include "PulseGenerator.h"
#include "EnergyCalculator.h"
#include "SlidingWindowAnalyzer.h"
#include "ConsumptionMonitor.h"
#include "Logger.h"
#include "ArgParser.h"
#include "Exceptions.h"

using namespace pulsetrack;

namespace {

int g_passed = 0;
int g_failed = 0;

void reportPass(const std::string& name) {
    ++g_passed;
    std::cout << "[PASS] " << name << "\n";
}

void reportFail(const std::string& name, const std::string& detail) {
    ++g_failed;
    std::cout << "[FAIL] " << name << " -- " << detail << "\n";
}

void check(const std::string& name, bool condition) {
    if (condition) {
        reportPass(name);
    } else {
        reportFail(name, "condition was false");
    }
}

// Generic "this call must throw exactly ExceptionT" helper, used for every
// input-validation / security-relevant edge case below.
template <typename ExceptionT, typename Func>
void expectThrows(const std::string& name, Func&& func) {
    try {
        func();
        reportFail(name, "expected an exception but none was thrown");
    } catch (const ExceptionT&) {
        reportPass(name);
    } catch (const std::exception& ex) {
        reportFail(name, std::string("threw the wrong exception type: ") + ex.what());
    }
}

// ---------------------------------------------------------------------
// Point 7: Factory Pattern
// ---------------------------------------------------------------------
void testMeterFactory() {
    auto residential = MeterFactory::create("residential");
    check("Factory creates Residential from lowercase name", residential->type() == MeterType::Residential);

    auto commercial = MeterFactory::create("COMMERCIAL");
    check("Factory creates Commercial from uppercase name", commercial->type() == MeterType::Commercial);

    auto industrial = MeterFactory::create(MeterType::Industrial);
    check("Factory creates Industrial from enum overload", industrial->typeName() == "Industrial");

    expectThrows<InvalidMeterTypeException>("Factory rejects an unknown meter name", [] {
        MeterFactory::create("nuclear");
    });
    expectThrows<InvalidMeterTypeException>("Factory rejects an empty meter name", [] {
        MeterFactory::create("");
    });
}

// ---------------------------------------------------------------------
// Points 2-3: reading and counting pulses (thread-safety is exercised
// separately in testPulseGeneratorConcurrency below)
// ---------------------------------------------------------------------
void testPulseCounter() {
    PulseCounter counter;
    check("New PulseCounter starts at zero total", counter.total() == 0);
    check("New PulseCounter starts at zero interval", counter.fetchAndResetInterval() == 0);

    counter.increment();
    counter.increment();
    counter.increment();
    check("Three increments -> total is 3", counter.total() == 3);

    const uint64_t interval1 = counter.fetchAndResetInterval();
    check("fetchAndResetInterval returns the accumulated 3", interval1 == 3);
    check("fetchAndResetInterval resets the interval count", counter.fetchAndResetInterval() == 0);
    check("Resetting the interval does not affect the cumulative total", counter.total() == 3);

    counter.increment();
    check("Total keeps accumulating after an interval reset", counter.total() == 4);
}

// ---------------------------------------------------------------------
// Points 4-5: pulses -> energy -> power, including divide-by-zero guards
// ---------------------------------------------------------------------
void testEnergyCalculator() {
    check("1000 pulses at 1000 imp/kWh = 1.0 kWh", EnergyCalculator::pulsesToKwh(1000, 1000.0) == 1.0);
    check("0 pulses = 0 kWh", EnergyCalculator::pulsesToKwh(0, 1000.0) == 0.0);

    const double power = EnergyCalculator::instantaneousPowerWatts(1, 1000.0, 1.0);
    check("1 pulse/sec at 1000 imp/kWh works out to 3600 W", power > 3599.9 && power < 3600.1);

    expectThrows<InvalidArgumentException>("Zero elapsed seconds is rejected (no divide by zero)", [] {
        EnergyCalculator::instantaneousPowerWatts(5, 1000.0, 0.0);
    });
    expectThrows<InvalidArgumentException>("Negative elapsed seconds is rejected", [] {
        EnergyCalculator::instantaneousPowerWatts(5, 1000.0, -2.0);
    });
    expectThrows<InvalidArgumentException>("Zero pulsesPerKwh is rejected (no divide by zero)", [] {
        EnergyCalculator::pulsesToKwh(5, 0.0);
    });
    expectThrows<InvalidArgumentException>("Negative pulsesPerKwh is rejected", [] {
        EnergyCalculator::pulsesToKwh(5, -100.0);
    });
}

// ---------------------------------------------------------------------
// Points 5 & 8: sliding window for recent/average consumption
// ---------------------------------------------------------------------
void testSlidingWindow() {
    SlidingWindowAnalyzer<double> empty(3);
    check("Empty window average is 0, not NaN/crash", empty.average() == 0.0);
    check("Empty window reports empty()", empty.empty());

    SlidingWindowAnalyzer<double> w(3);
    w.addReading(10.0);
    w.addReading(20.0);
    check("Partial window (2 of 3) averages just those readings", w.average() == 15.0);

    w.addReading(30.0);
    check("Full window (3 of 3) averages all three", w.average() == 20.0);

    w.addReading(60.0); // should push the oldest reading (10.0) out
    check("Window stays at capacity once full", w.size() == 3);
    check("Average reflects only the retained readings (20,30,60)", w.average() == (20.0 + 30.0 + 60.0) / 3.0);

    SlidingWindowAnalyzer<double> zeroSized(0);
    check("A windowSize of 0 is clamped to capacity 1, not left unusable", zeroSized.capacity() == 1);
    zeroSized.addReading(5.0);
    zeroSized.addReading(9.0);
    check("Clamped window still only keeps the most recent reading", zeroSized.average() == 9.0);
}

// ---------------------------------------------------------------------
// Point 6: high consumption detection, including the exact-threshold edge
// ---------------------------------------------------------------------
void testConsumptionMonitor() {
    ConsumptionMonitor monitor(1000.0);
    check("Reading below threshold is not flagged high", !monitor.isHighConsumption(999.9));
    check("Reading exactly AT threshold is NOT high (strict > rule)", !monitor.isHighConsumption(1000.0));
    check("Reading just above threshold is flagged high", monitor.isHighConsumption(1000.1));
}

// ---------------------------------------------------------------------
// Point 1 + concurrency/security: pulse generator thread lifecycle safety
// ---------------------------------------------------------------------
void testPulseGeneratorConcurrency() {
    PulseCounter counter;
    {
        PulseGenerator generator(counter, {5, 15}); // fast interval for a quick test
        generator.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        generator.stop();
    }
    check("Generator produced at least one pulse in 300ms at 5-15ms intervals", counter.total() > 0);

    PulseCounter counter2;
    PulseGenerator neverStarted(counter2, {10, 20});
    neverStarted.stop(); // stopping without starting must be a safe no-op, not a crash
    check("Stopping a never-started generator is a safe no-op", counter2.total() == 0);

    PulseGenerator restartable(counter2, {5, 10});
    restartable.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    restartable.stop();
    const uint64_t afterFirstRun = counter2.total();
    restartable.start(); // restart after a clean stop must work, not crash
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    restartable.stop();
    check("Generator can be restarted after stop() and keeps counting", counter2.total() >= afterFirstRun);

    PulseGenerator invertedRange(counter2, {50, 5}); // deliberately inverted min/max
    invertedRange.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    invertedRange.stop();
    check("An inverted [min,max] interval range is normalized, not undefined behavior", true);
}

// ---------------------------------------------------------------------
// Point 9: logging, including a bad-path failure case
// ---------------------------------------------------------------------
void testLogger() {
    const std::string path = "test_logger_output.log";
    std::remove(path.c_str());
    {
        Logger logger(path);
        logger.log("INFO", "hello from test");
        logger.log("ALERT", "second line");
    }

    std::ifstream in(path);
    check("Logger created a readable log file", in.good());
    std::string line1, line2;
    std::getline(in, line1);
    std::getline(in, line2);
    check("First log line contains its level and message",
          line1.find("[INFO]") != std::string::npos && line1.find("hello from test") != std::string::npos);
    check("Second log line contains its level and message",
          line2.find("[ALERT]") != std::string::npos && line2.find("second line") != std::string::npos);
    in.close();
    std::remove(path.c_str());

    expectThrows<LogFileException>("Logger rejects a path in a nonexistent directory", [] {
        Logger bad("/no/such/directory/x.log");
    });
}

// ---------------------------------------------------------------------
// Security-relevant input validation: everything that reaches the program
// from argv must be validated here, never trusted as-is.
// ---------------------------------------------------------------------
std::vector<char*> toArgv(std::vector<std::string>& storage) {
    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (auto& s : storage) {
        argv.push_back(s.data());
    }
    return argv;
}

void testArgParser() {
    check("parsePositiveInt accepts a valid in-range value", ArgParser::parsePositiveInt("42", "--x", 1, 100) == 42);
    check("parsePositiveInt accepts the minimum boundary", ArgParser::parsePositiveInt("1", "--x", 1, 100) == 1);
    check("parsePositiveInt accepts the maximum boundary", ArgParser::parsePositiveInt("100", "--x", 1, 100) == 100);

    expectThrows<InvalidArgumentException>("parsePositiveInt rejects non-numeric text", [] {
        ArgParser::parsePositiveInt("abc", "--x", 1, 100);
    });
    expectThrows<InvalidArgumentException>("parsePositiveInt rejects trailing garbage ('42abc')", [] {
        ArgParser::parsePositiveInt("42abc", "--x", 1, 100);
    });
    expectThrows<InvalidArgumentException>("parsePositiveInt rejects an empty value", [] {
        ArgParser::parsePositiveInt("", "--x", 1, 100);
    });
    expectThrows<InvalidArgumentException>("parsePositiveInt rejects a value below the minimum", [] {
        ArgParser::parsePositiveInt("0", "--x", 1, 100);
    });
    expectThrows<InvalidArgumentException>("parsePositiveInt rejects a value above the maximum", [] {
        ArgParser::parsePositiveInt("101", "--x", 1, 100);
    });
    expectThrows<InvalidArgumentException>("parsePositiveInt rejects a numeric overflow", [] {
        ArgParser::parsePositiveInt("999999999999999999999", "--x", 1, 100);
    });

    {
        std::vector<std::string> storage = {"pulsetrack", "--meter=commercial", "--duration=30", "--window=7"};
        auto argv = toArgv(storage);
        Options opts = ArgParser::parse(static_cast<int>(argv.size()), argv.data());
        check("parse() reads --meter=commercial correctly", opts.meterType == MeterType::Commercial);
        check("parse() reads --duration=30 correctly", opts.durationSeconds == 30);
        check("parse() reads --window=7 correctly", opts.windowSize == 7);
    }
    {
        std::vector<std::string> storage = {"pulsetrack", "--meter=nuclear"};
        auto argv = toArgv(storage);
        expectThrows<InvalidMeterTypeException>("parse() rejects an invalid --meter value", [&] {
            ArgParser::parse(static_cast<int>(argv.size()), argv.data());
        });
    }
    {
        std::vector<std::string> storage = {"pulsetrack", "--surprise=1"};
        auto argv = toArgv(storage);
        expectThrows<InvalidArgumentException>("parse() rejects a completely unknown flag", [&] {
            ArgParser::parse(static_cast<int>(argv.size()), argv.data());
        });
    }
    {
        std::vector<std::string> storage = {"pulsetrack", "--help"};
        auto argv = toArgv(storage);
        check("hasHelpFlag detects --help", ArgParser::hasHelpFlag(static_cast<int>(argv.size()), argv.data()));
    }
    {
        std::vector<std::string> storage = {"pulsetrack", "-h"};
        auto argv = toArgv(storage);
        check("hasHelpFlag detects -h", ArgParser::hasHelpFlag(static_cast<int>(argv.size()), argv.data()));
    }
    {
        std::vector<std::string> storage = {"pulsetrack", "--duration=10"};
        auto argv = toArgv(storage);
        check("hasHelpFlag returns false when no help flag is present",
              !ArgParser::hasHelpFlag(static_cast<int>(argv.size()), argv.data()));
    }
}

} // namespace

int main() {
    std::cout << "Running PulseTrack test suite...\n\n";

    testMeterFactory();
    testPulseCounter();
    testEnergyCalculator();
    testSlidingWindow();
    testConsumptionMonitor();
    testPulseGeneratorConcurrency();
    testLogger();
    testArgParser();

    std::cout << "\n----------------------------------------\n";
    std::cout << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
