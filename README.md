# PulseTrack

A small, self-contained C++ program that simulates how a real electricity
meter works: it generates pulses the way a meter's blinking LED or spinning
disc does, counts them, converts them into energy usage, and watches for
abnormally high consumption — all from a single terminal command.

## What it does

Every point below is implemented and testable independently:

1. **Generates/simulates electricity meter pulses** — a background thread
   emits pulses at randomized, meter-realistic intervals.
2. **Reads those pulses** — the main thread reads them every second.
3. **Counts them** — a thread-safe atomic counter, both per-second and
   cumulative.
4. **Converts pulses into energy consumption** — using each meter type's
   pulse constant (imp/kWh).
5. **Calculates recent/average power usage** — a sliding window over the
   last N one-second samples.
6. **Detects high consumption** — compares the recent average against the
   meter's threshold and flags it.
7. **Supports different meter types via the Factory Pattern** —
   Residential, Commercial, Industrial, each with different pulse
   constants, thresholds, and pulse rates.
8. **Uses a sliding window for recent consumption analysis** — a generic,
   templated `SlidingWindowAnalyzer<T>` backed by `std::deque`.
9. **Logs the results** — every sample and every alert is timestamped and
   written to a log file.

Additionally, a Linux kernel character device driver (`kernel/`) simulates
the same pulse output in kernel space, with a userspace client
demonstrating the syscalls used to read it — see the Kernel driver section
below.

## Project stages

This project was built and documented in six stages, each with its own
write-up in `docs/`:

| Stage | Document |
|---|---|
| 1 — Introduction | [`docs/stage1_introduction.md`](docs/stage1_introduction.md) |
| 2 — Requirements & Plan (PRD) | [`docs/stage2_requirements_and_plan.md`](docs/stage2_requirements_and_plan.md) |
| 3 — Design & Architecture (incl. UML) | [`docs/stage3_design_and_architecture.md`](docs/stage3_design_and_architecture.md) |
| 4 — Initial Implementation & Prototype | [`docs/stage4_prototype.md`](docs/stage4_prototype.md) |
| 5 — Testing, Integration & Improvement | [`docs/stage5_testing_and_improvement.md`](docs/stage5_testing_and_improvement.md) |
| 6 — Final Implementation & Presentation | [`docs/stage6_final_summary.md`](docs/stage6_final_summary.md) |

The Git history mirrors these stages via a real branching strategy — see
"Git repository and branching strategy" in Stage 3, and run
`git log --oneline --graph --all` to see it directly.

## Real-world use case

A real electricity meter counts usage by pulses: a small LED blinks (or a
disc spins) once for every fixed amount of energy consumed. Utility
companies and smart-meter gateways count these blinks to know how much
power a building is using right now, to bill accurately, and to catch
unusual spikes (like a faulty appliance left running). PulseTrack is
software that reproduces exactly that pipeline — generate pulses, count
them, turn them into kWh, track a recent average, flag abnormal usage, log
everything — the same core logic behind a consumer "smart meter" app that
shows live usage and sends high-usage alerts.

## Architecture

```
 you
  │  ./pulsetrack --meter=residential --duration=60
  ▼
[Factory Pattern] → builds the requested SmartMeter
  │
  ▼
[PulseGenerator]  (background thread) → simulates pulses
  │
  ▼
[PulseCounter]    (main thread, thread-safe)  → reads/counts pulses
  │
  ▼
[EnergyCalculator] → pulses → energy (kWh) → instantaneous power (W)
  │
  ▼
[SlidingWindowAnalyzer<double>] → recent average power
  │
  ▼
[ConsumptionMonitor] → flags HIGH CONSUMPTION vs normal
  │
  ├──► console output (live, formatted table)
  │
  ▼
[Logger] → timestamped log file
```

A second, independent component lives in `kernel/`: a Linux character
device driver that generates and exposes pulses in kernel space, plus a
userspace client that reads it via `open`/`read`/`close`. The two are not
wired together at runtime in this submission — see
[`docs/stage3_design_and_architecture.md`](docs/stage3_design_and_architecture.md)
for the full two-component diagram and UML (class, sequence, state).

## Building and running

### On macOS (Apple Silicon / M1, or Intel)

You only need Xcode's Command Line Tools — no Docker, no VM, no WSL. This
project is plain, portable C++17 that compiles natively on macOS's own
POSIX-compliant Terminal.

```bash
xcode-select --install      # one-time, gives you clang++, make, git
git clone <your-repo-url>
cd pulsetrack
make            # or: scripts/build.sh
./pulsetrack --meter=residential --duration=20
```

### On Linux (including WSL2 on Windows)

```bash
sudo apt install build-essential   # g++, make (if not already present)
git clone <your-repo-url>
cd pulsetrack
make
./pulsetrack --meter=residential --duration=20
```

### CLI options

```
--meter=residential|commercial|industrial   (default: residential)
--duration=SECONDS                          (default: 20, range 1-3600)
--window=N                                  (default: 5,  range 1-100)
--log=PATH                                  (default: pulsetrack.log)
--help, -h                                  (prints usage, exits 0)
```

Example:

```bash
./pulsetrack --meter=industrial --duration=30 --window=8 --log=session.log
```

### Running the test suite

```bash
make test
```

This builds and runs a self-contained, dependency-free suite of 55 checks
covering the 9 functional points above plus edge cases (see "Verification"
below).

### Kernel driver

```bash
cd kernel
make                                    # needs linux-headers-$(uname -r)
sudo insmod pulsemeter_driver.ko
cat /dev/pulsemeter
sudo rmmod pulsemeter_driver
```

See [`kernel/README.md`](kernel/README.md) for what was actually verified
where (compiled clean against real kbuild headers; not `insmod`-loaded in
the development sandbox, for two specific, confirmed reasons given there).

## Verification (trust but verify)

Nothing in this repo is asserted without having actually been run:

- **Compiler warnings**: builds clean with `-Wall -Wextra -Werror
  -Wpedantic` — zero warnings tolerated.
- **Functional + edge-case tests**: 55/55 passing (`make test`), covering
  normal operation, empty/zero inputs, boundary values (e.g. a reading
  exactly at the alert threshold), malformed CLI input (non-numeric,
  trailing garbage, out-of-range, integer overflow, empty values),
  restart/no-op safety of the background thread, and log-file failure
  handling.
- **Memory safety**: `valgrind --leak-check=full` on both the test suite
  and the real binary — 0 leaks, 0 errors.
- **Data-race safety**: rebuilt and re-run under **ThreadSanitizer**
  (`make sanitize-thread`) — 0 races reported, verifying the atomic
  pulse-counting design is actually race-free, not just assumed to be.
- **Undefined behavior / memory errors**: rebuilt and re-run under
  **AddressSanitizer + UndefinedBehaviorSanitizer**
  (`make sanitize-address`) — clean.
- **Input validation as a security boundary**: all external input (CLI
  arguments) is parsed and validated in one place (`ArgParser`) before it
  can reach the rest of the program; malformed input always fails with a
  clear message and exit code 1, never a crash or unhandled exception.

## Concepts from the training covered here

| Topic (from the 20-day syllabus) | Where it shows up |
|---|---|
| C++ (OOP, design patterns) | `SmartMeter` hierarchy + `MeterFactory` (Factory Pattern) |
| C++ (STL, templates) | `SlidingWindowAnalyzer<T>` (templated, `std::deque`-backed) |
| C++ (multithreading, RAII) | `PulseGenerator` (background thread, exception-safe stop/join) |
| C++ (smart pointers) | `std::unique_ptr<SmartMeter>` ownership throughout |
| C++ (exceptions) | A typed exception hierarchy (`PulseTrackException` and subtypes) instead of error codes |
| Linux (build toolchain) | Plain `Makefile` + `g++`/`clang++`, no IDE-specific project files |
| Linux (shell scripting) | `scripts/build.sh`, `scripts/run.sh` |
| Linux + Git | Real, incremental commit history + branching strategy (see `git log --oneline --graph --all`) |
| Linux Device Drivers | `kernel/pulsemeter_driver.c` — char device, kernel timer, interrupt-context-safe spinlock, device model registration |
| Linux System Programming | `kernel/test/read_meter.c` — raw `open`/`read`/`close` syscalls against a device node |
| Computer Architecture / hardware-software interaction | Discussed below |

**Computer Architecture / hardware-software boundary, honestly**: the
userspace pipeline's contact with hardware is indirect but real —
fixed-width atomic integers (`std::atomic<uint64_t>`) for lock-free,
hardware-level-safe concurrent counting, and `std::this_thread::sleep_for`
for OS-scheduler-backed timing. The kernel driver engages with it directly:
a spinlock with IRQ save/restore (`spin_lock_irqsave`) is used specifically
*because* its timer callback runs in softirq/interrupt context, where a
mutex would be unsafe (it can sleep; interrupt context can't). Note on
scope: the natural, minimal way to read a *real* meter's pulse signal on
Linux is to poll it from userspace via the kernel's existing `libgpiod`
GPIO interface, not a custom driver — this project includes a real
character device driver anyway because Linux Device Drivers is an
explicitly required, separately graded topic for this submission, not
because the meter itself demands kernel-space code. That tradeoff is
stated here rather than left implicit.

## Pushing to GitHub

Since building this already requires Xcode Command Line Tools (which
includes `git`), you can push directly rather than uploading through the
browser:

```bash
git remote add origin https://github.com/<your-username>/pulsetrack.git
git push -u origin master
git push origin dev feature/kernel-driver feature/stage-docs
```

Pushing all branches (not just `master`) is what makes the branching
strategy in Stage 3 actually visible on GitHub — the `feature/* → dev →
master` merge history, not just the final state.


AUTHOR
Adarsh Lenka
B.Tech Computer Science Engineering 
Institute of Technical Educational & Research (ITER), SOA University

License
This project is intended for academic and educational purposes.
