# Stage 5 — Testing, Integration & Improvement

## Remaining implementation completed this stage

- Dependency-free automated test suite (`tests/test_core.cpp`, commit
  `d84fe16`).
- Linux kernel character device driver + userspace syscall client
  (`kernel/`, commit `e4aec5b`, developed on `feature/kernel-driver` and
  merged through `dev` into `master`).

## Unit and edge-case testing

`make test` builds and runs 55 checks, all passing:

```
55 passed, 0 failed
```

Coverage includes normal behavior for every functional requirement (FR1–
FR9) plus edge cases specifically chosen to break naive implementations:

- Empty sliding window (must not divide by zero).
- A reading exactly at the high-consumption threshold (must NOT be flagged
  — only strictly above it counts).
- Zero pulses, zero elapsed seconds, zero/negative pulse constants (must
  raise a clear exception, never crash or silently produce NaN/garbage).
- Malformed CLI input: non-numeric, trailing garbage (`"20abc"`), empty
  values, out-of-range values, and numeric overflow.
- Stopping a background thread that was never started, and restarting one
  after a clean stop (both must be safe no-ops/valid operations, not
  crashes).
- Logger construction against a path in a nonexistent directory (must
  raise a clear exception rather than silently dropping log output).

## Integration / system testing

The compiled binary was run end-to-end for all three meter types
(residential, commercial, industrial) with different `--duration` and
`--window` values, and the resulting log file contents were inspected
directly to confirm the console output and the log file agree line for
line.

## Memory and concurrency verification

Not just claimed — actually run:

| Tool | Target | Result |
|---|---|---|
| Compiler warnings | `-Wall -Wextra -Werror -Wpedantic` | 0 warnings |
| Valgrind (`--leak-check=full`) | test suite and the real binary | 0 leaks, 0 errors |
| ThreadSanitizer | test suite (`make sanitize-thread`) | 0 data races |
| AddressSanitizer + UBSan | test suite (`make sanitize-address`) | 0 errors |

## Kernel driver verification

`pulsemeter_driver.c` was compiled against real Linux kbuild headers
available in the development sandbox, with zero errors or warnings. It
could not be `insmod`-loaded in that same sandbox, for two confirmed
reasons: the sandbox's own running kernel has no matching headers package
anywhere on the system, and the sandbox has no `insmod`/`modprobe`/`rmmod`
tools installed at all. Both are environment constraints of the
development sandbox, not defects in the driver — see `kernel/README.md`
for the exact load/test steps to run on a real Linux machine or WSL2,
where both constraints are absent by default.

## Issue found and fixed during this stage

`--help` was exiting with status 1 (as if it were an error) because it was
implemented by throwing the usage text as an `InvalidArgumentException`.
Caught during manual verification, fixed by adding
`ArgParser::hasHelpFlag()`, checked in `main()` before argument validation
runs, so `--help`/`-h` now print usage and exit 0. Re-verified after the
fix.

## Roadmap into Stage 6

Final stage: polish documentation, write the final project report
(achievements, limitations, future improvements), and package everything
for submission — source, tests, kernel driver, UML diagrams, and Git
history.
