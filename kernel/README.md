# pulsemeter_driver — Linux kernel character device driver

A minimal char device driver simulating a smart meter's pulse output in
kernel space (Linux Device Drivers coverage for this project). A real S0
pulse meter toggles a GPIO line on every pulse, serviced by a hardware
interrupt handler; this driver substitutes a kernel timer for that
interrupt source (no real GPIO/IRQ hardware exists in a development
sandbox), but everything else — device registration, interrupt-context-safe
locking, and the userspace-facing read() interface — is written the way a
real interrupt-driven driver would be.

## What it does

- Registers `/dev/pulsemeter` as a character device.
- A kernel timer fires every 500ms (standing in for a hardware pulse
  interrupt) and increments a counter, protected by a spinlock with
  IRQ save/restore (correct for code that can run in interrupt/softirq
  context, where a mutex would be unsafe because it can sleep).
- `read()` returns the pulse count accumulated since the last read, as an
  ASCII decimal string, then resets the counter to 0 ("read-and-clear",
  the same convention many real pulse-counting peripherals use).
- Clean, ordered teardown on module unload (`del_timer_sync` before
  destroying the device, so no timer callback can fire after the counter
  it touches has been freed).

## Build

Requires the kernel headers for the machine you are building on:

```bash
sudo apt install build-essential linux-headers-$(uname -r)   # Linux
cd kernel
make
```

On WSL2 (Windows) this is the same command — WSL2 provides a real Linux
kernel with its own matching headers package.

## Load, test, unload

```bash
sudo insmod pulsemeter_driver.ko
dmesg | tail                      # confirm: "pulsemeter: loaded, device node ..."
cat /dev/pulsemeter               # prints the pulse count since the last read
cd test && gcc -O2 -o read_meter read_meter.c && ./read_meter -w 5
cd ..
sudo rmmod pulsemeter_driver
dmesg | tail                      # confirm: "pulsemeter: unloaded"
```

## What was actually verified, and where

Being precise about this, rather than just claiming it works:

- **Compiled successfully with zero errors/warnings** against real Linux
  kbuild headers (this development sandbox happened to have a generic
  kernel's headers available at `/usr/lib/modules/6.8.0-142-generic/build`,
  which is enough to run the actual kernel build system against this
  source and catch real compile-time errors — type mismatches, wrong API
  usage, missing includes, etc.). That step was genuinely run, not assumed.
- **NOT loaded or runtime-tested in this sandbox.** Two independent reasons,
  both confirmed directly rather than guessed: (1) the sandbox's own
  running kernel (`uname -r`) has no matching headers anywhere on the
  system, so a module built for it can't even be produced here, and (2)
  the sandbox has no `insmod`/`modprobe`/`rmmod` binaries installed at all
  (it's a locked-down container, by design) — so loading a kernel module
  is categorically not possible here regardless of the module itself.
- **On your own Linux machine or in WSL2**, both of those limitations go
  away: `linux-headers-$(uname -r)` matches your actual running kernel by
  definition, and `insmod`/`rmmod` are present. That is the environment
  this driver needs to be load-tested in, and the steps above are exactly
  what to run there.

## Files

- `pulsemeter_driver.c` — the driver.
- `Makefile` — standard out-of-tree kbuild makefile (`KDIR` overridable to
  target a specific kernel's headers).
- `test/read_meter.c` — a small userspace program using raw `open()`/
  `read()`/`close()` syscalls to talk to the device (Linux System
  Programming: interacting with a device node the same way any real
  application would).
