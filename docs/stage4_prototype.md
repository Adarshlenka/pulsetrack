# Stage 4 — Initial Implementation & Prototype

## Core modules implemented

Commit `88ee9ec` (`Add core PulseTrack C++ implementation: ...`) is the
initial working prototype, built in the order set out in Stage 3:

1. `SmartMeter` hierarchy + `MeterFactory` — the three meter types and the
   Factory Pattern that constructs them.
2. `PulseCounter` + `PulseGenerator` — thread-safe pulse counting and the
   background thread that simulates pulses.
3. `EnergyCalculator` — pulses → kWh → Watts conversion math.
4. `SlidingWindowAnalyzer<T>` — the templated rolling-average window.
5. `ConsumptionMonitor` — threshold comparison.
6. `Logger` — timestamped file logging.
7. `ArgParser` + `main.cpp` — CLI validation and wiring everything together
   into one runnable program.

Each module was integrated progressively rather than written all at once
and wired together at the end: the counter and generator were built and
manually exercised before the energy math was added on top, and so on up
the pipeline, so a mistake in an earlier layer would surface immediately
rather than being masked by a later one.

## Demonstrating the initial functionality

A real captured run from this stage (`--meter=residential --duration=5
--window=3`):

```
==================================================
 PulseTrack - Smart Meter Pulse Counter & Analytics
==================================================
  Meter type      : Residential
  Pulse constant  : 1000 imp/kWh
  Alert threshold : 6500 W
  Duration        : 5 s
  Sliding window  : 3 samples
--------------------------------------------------
Sec   Pulses   Power(W)     AvgPower(W)    Energy(kWh) Status
--------------------------------------------------
1     1        3600.0       3600.0         0.0010      normal
2     3        10800.0      7200.0         0.0040      HIGH CONSUMPTION
3     3        10800.0      8400.0         0.0070      HIGH CONSUMPTION
4     1        3600.0       8400.0         0.0080      HIGH CONSUMPTION
5     3        10800.0      8400.0         0.0110      HIGH CONSUMPTION
--------------------------------------------------
Session complete. Total pulses: 11  Total energy: 0.0110 kWh
```

This confirms, from the very first working version, that every functional
requirement in Stage 2 (FR1–FR9) was already observably working: pulses
generated and counted, energy and power computed, the sliding window
producing a recent average distinct from the instantaneous reading, high
consumption correctly flagged, and every line mirrored into the log file.

## Issues found and solved during this stage

- **Meter constants needed tuning to be a meaningful demo.** The first pass
  at each meter type's pulse-interval range and threshold either almost
  never triggered "HIGH CONSUMPTION" or triggered it constantly, which
  makes FR6 hard to actually observe. Fixed by deriving the expected power
  range analytically from each meter's pulse interval and pulse constant
  (`power_W = (pulses/pulsesPerKwh) × 3600000 / seconds`) and picking a
  threshold that sits inside that range, so a real run naturally shows a
  mix of `normal` and `HIGH CONSUMPTION` rows rather than always one or
  the other.
- **`--help` exiting with an error code.** Caught during manual
  verification in Stage 5: `--help` was implemented by throwing the usage
  text as an exception, which made it exit with code 1 like a real error.
  Fixed by checking for `--help`/`-h` before validation and returning 0
  directly — documented in Stage 5 as part of the testing/improvement
  loop, since that is where it was actually caught.

## Roadmap into Stage 5

With the prototype working end-to-end, the next stage adds: the automated
test suite (55 cases covering the functional behavior above plus edge
cases), memory/thread-safety verification (Valgrind, ThreadSanitizer,
AddressSanitizer/UBSan), and the kernel-space driver component.
