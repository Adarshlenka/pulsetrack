#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <memory>
#include <cstdint>

#include "ArgParser.h"
#include "MeterFactory.h"
#include "PulseCounter.h"
#include "PulseGenerator.h"
#include "EnergyCalculator.h"
#include "SlidingWindowAnalyzer.h"
#include "ConsumptionMonitor.h"
#include "Logger.h"
#include "Exceptions.h"

using namespace pulsetrack;

namespace {

void printHeader(const SmartMeter& meter, const Options& options) {
    std::cout << "==================================================\n";
    std::cout << " PulseTrack - Smart Meter Pulse Counter & Analytics\n";
    std::cout << "==================================================\n";
    std::cout << "  Meter type      : " << meter.typeName() << "\n";
    std::cout << "  Pulse constant  : " << meter.pulsesPerKwh() << " imp/kWh\n";
    std::cout << "  Alert threshold : " << meter.highConsumptionThresholdWatts() << " W\n";
    std::cout << "  Duration        : " << options.durationSeconds << " s\n";
    std::cout << "  Sliding window  : " << options.windowSize << " samples\n";
    std::cout << "  Log file        : " << options.logPath << "\n";
    std::cout << "--------------------------------------------------\n";
    std::cout << std::left
              << std::setw(6)  << "Sec"
              << std::setw(9)  << "Pulses"
              << std::setw(13) << "Power(W)"
              << std::setw(15) << "AvgPower(W)"
              << std::setw(12) << "Energy(kWh)"
              << "Status\n";
    std::cout << "--------------------------------------------------\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        if (ArgParser::hasHelpFlag(argc, argv)) {
            std::cout << ArgParser::usageText() << "\n";
            return 0;
        }

        // 1. Parse and validate all external input up front.
        const Options options = ArgParser::parse(argc, argv);

        // 7. Factory Pattern: build the requested meter type.
        const std::unique_ptr<SmartMeter> meter = MeterFactory::create(options.meterType);

        // 9. Logger (fails fast if the log path can't be opened).
        Logger logger(options.logPath);

        PulseCounter counter;
        SlidingWindowAnalyzer<double> window(options.windowSize);
        ConsumptionMonitor monitor(meter->highConsumptionThresholdWatts());

        printHeader(*meter, options);
        logger.log("INFO", "Session started: meter=" + meter->typeName() +
                                " duration=" + std::to_string(options.durationSeconds) + "s" +
                                " window=" + std::to_string(options.windowSize));

        // 1. Start simulating pulses on a background thread.
        PulseGenerator generator(counter, meter->pulseIntervalRangeMs());
        generator.start();

        for (int second = 1; second <= options.durationSeconds; ++second) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // 2-3. Read and count the pulses generated this second.
            const uint64_t pulsesThisSecond = counter.fetchAndResetInterval();

            // 4-5. Convert pulses -> instantaneous power, feed the sliding window.
            const double powerWatts =
                EnergyCalculator::instantaneousPowerWatts(pulsesThisSecond, meter->pulsesPerKwh(), 1.0);
            window.addReading(powerWatts);
            const double avgPowerWatts = window.average();

            // 6. Detect high consumption.
            const bool high = monitor.isHighConsumption(avgPowerWatts);

            // 4. Cumulative energy so far.
            const double totalKwh = EnergyCalculator::pulsesToKwh(counter.total(), meter->pulsesPerKwh());

            std::cout << std::left
                      << std::setw(6) << second
                      << std::setw(9) << pulsesThisSecond
                      << std::setw(13) << std::fixed << std::setprecision(1) << powerWatts
                      << std::setw(15) << std::fixed << std::setprecision(1) << avgPowerWatts
                      << std::setw(12) << std::fixed << std::setprecision(4) << totalKwh
                      << (high ? "HIGH CONSUMPTION" : "normal")
                      << "\n";

            std::ostringstream line;
            line << "t=" << second << "s pulses=" << pulsesThisSecond << " power_W="
                 << std::fixed << std::setprecision(1) << powerWatts
                 << " avg_power_W=" << avgPowerWatts
                 << " total_kwh=" << std::setprecision(4) << totalKwh
                 << " status=" << (high ? "HIGH" : "normal");
            logger.log(high ? "ALERT" : "INFO", line.str());
        }

        generator.stop();

        const double finalKwh = EnergyCalculator::pulsesToKwh(counter.total(), meter->pulsesPerKwh());
        std::cout << "--------------------------------------------------\n";
        std::cout << "Session complete. Total pulses: " << counter.total()
                  << "  Total energy: " << std::fixed << std::setprecision(4) << finalKwh << " kWh\n";
        std::cout << "Full log written to: " << options.logPath << "\n";

        logger.log("INFO", "Session complete. total_pulses=" + std::to_string(counter.total()) +
                                " total_kwh=" + std::to_string(finalKwh));

        return 0;
    } catch (const PulseTrackException& ex) {
        // All expected, "this is bad input/state" failures land here with a
        // clean, specific message.
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        // Safety net: no exception of any kind is allowed to escape main()
        // and trigger an unhandled-exception abort.
        std::cerr << "Unexpected error: " << ex.what() << "\n";
        return 1;
    }
}
