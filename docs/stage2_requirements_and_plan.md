# Stage 2 — Project Requirements & Development Plan (PRD)

## Functional requirements

| # | Requirement |
|---|---|
| FR1 | Generate/simulate electricity meter pulses. |
| FR2 | Read those pulses. |
| FR3 | Count them (per-second interval count and cumulative total). |
| FR4 | Convert pulses into energy consumption (kWh). |
| FR5 | Calculate recent/average power usage over a sliding window. |
| FR6 | Detect and flag high consumption against a per-meter threshold. |
| FR7 | Support different meter types (Residential/Commercial/Industrial) via a Factory Pattern. |
| FR8 | Use a sliding window for recent consumption analysis. |
| FR9 | Log every reading and alert to a timestamped log file. |
| FR10 | Provide a Linux kernel character device (`/dev/pulsemeter`) that generates and exposes pulse events the way real interrupt-driven metering hardware does. |
| FR11 | Provide a userspace client demonstrating standard syscalls (`open`/`read`/`close`) against that device. |

## Non-functional requirements

| # | Requirement | How it's met |
|---|---|---|
| NFR1 | Thread safety | Pulse generation and counting run on separate threads with no data races — verified with ThreadSanitizer. |
| NFR2 | Memory safety | No leaks, no use-after-free/UB — verified with Valgrind and AddressSanitizer+UBSan. |
| NFR3 | Input validation / security | All CLI input validated in one place (`ArgParser`); malformed input never crashes the program. |
| NFR4 | Portability | Plain C++17/POSIX; builds natively on macOS and Linux (including WSL2) with no OS-specific code in the userspace layer. |
| NFR5 | Build simplicity | One `Makefile`, `g++`/`clang++`, no external dependencies or package manager needed. |
| NFR6 | Testability | 55 automated tests covering functional behavior and edge cases, runnable with one command (`make test`). |
| NFR7 | Maintainability | Small, single-responsibility classes (one concept per header/source pair) rather than one monolithic file. |

## Scope, modules, and deliverables

**Modules**
- `core` — C++ meter/analytics library (`include/`, `src/`).
- `kernel` — Linux character device driver + userspace syscall test client.
- `tests` — dependency-free automated test suite.
- `scripts` — build/run helpers.
- `docs` — this staged documentation set.

**Deliverables**
- Working C++ userspace application (`pulsetrack`).
- Working (compile-verified) Linux kernel module (`pulsemeter_driver.ko` source).
- Automated test suite with a passing run recorded in Stage 5.
- UML diagrams (class, sequence, state) — Stage 3.
- Git repository with a real branching history — Stage 3.
- Final project report — Stage 6.

## Development plan and timeline

| Day | Focus |
|---|---|
| 1 | Stage 1–2: define the problem, scope, requirements, and this plan. |
| 2 | Stage 3: architecture, UML diagrams, dev environment, Git branching strategy. |
| 3–4 | Stage 4: implement the core C++ library and the kernel driver; get an initial working prototype running end-to-end. |
| 5 | Stage 5: build the automated test suite, run memory/thread-safety verification, fix anything found. |
| 6 | Stage 6: final polish, documentation, and submission packaging. |

This mirrors the actual commit history in this repository (`git log
--oneline --graph --all`), which is the real evidence of the plan being
followed rather than written after the fact.
