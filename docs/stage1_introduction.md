# Stage 1 — Project Introduction

## Project idea and objective

**PulseTrack** is a smart electricity meter simulator: it reproduces, in
software, how a real pulse-output meter is read, counted, converted into
energy usage, and monitored for abnormal consumption — with an additional
Linux kernel-space component that captures those pulses the way real
interrupt-driven metering hardware does.

The objective is to build and demonstrate a small but complete, correctly
engineered system spanning three layers taught in this training: C++
application logic, Linux system programming, and a Linux kernel device
driver — rather than a toy exercise in any one of them alone.

## Problem being solved

A real electricity meter counts usage by pulses: a small LED blinks (or a
disc spins) once per fixed unit of energy consumed. Software that reads a
meter needs to solve several concrete problems at once:

- Capture pulses reliably without missing or double-counting them, including
  when capture happens concurrently with the rest of the program running.
- Convert a raw pulse count into a meaningful unit (kWh) and a live power
  reading (Watts).
- Recognize *recent* behavior, not just a lifetime total, so a sudden spike
  in usage can be caught as it happens.
- Do all of the above whether the pulse source is a simulated one, or one
  based on a real interrupt/timer at the kernel level.

## Project scope

In scope:
- A C++17 userspace application implementing the full pulse-to-analytics
  pipeline for three meter classes (Residential, Commercial, Industrial).
- A Linux kernel character device driver that generates and exposes pulse
  events the way a real interrupt-driven meter peripheral would.
- A small userspace client demonstrating the standard Linux syscalls
  (`open`/`read`/`close`) used to talk to that device.
- Automated tests, memory/thread-safety verification, and documentation.

Out of scope (and why, documented honestly rather than silently omitted):
- Real GPIO/hardware wiring — no physical meter hardware is available;
  the kernel timer stands in for the hardware interrupt source.
- A GUI or web frontend — not needed to demonstrate the target concepts,
  and out of place for a systems-programming submission.
- IPC and networking between separate processes — the design is
  intentionally a single process; see Stage 3 for why that is the correct
  choice here rather than a shortcut.

## Expected outcome and application

The finished project behaves like a small, self-contained energy-monitoring
agent: run one command, watch pulses turn into a live energy/power reading
per second, get flagged the moment usage crosses a threshold, and have every
reading logged. That is the same core logic behind real embedded
energy-monitoring gateways and consumer "smart meter" apps, and it is built
to be explained and demonstrated end-to-end in an interview setting.
