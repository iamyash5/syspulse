# SysPulse

Embedded-style system resource monitor & control daemon, written in C++.

SysPulse reads CPU and memory usage from Linux's `/proc` interface, applies a
configurable threshold-based alert, and exposes live status and control to a
CLI tool over a UNIX domain socket — the same architectural pattern used by
real embedded/server monitoring agents.

## Project structure

```
syspulse/
├── daemon/      syspulsed.cpp   - background monitor + socket server
├── cli/         syspulsectl.cpp - command-line control tool
├── docs/        Stage 1-6 documentation, UML diagrams, progress log
└── tests/       verification notes / scripts
```

## Build

Requires a C++17 compiler (g++) and pthreads (standard on Linux).

```bash
cd daemon
g++ -std=c++17 -o syspulsed syspulsed.cpp -lpthread

cd ../cli
g++ -std=c++17 -o syspulsectl syspulsectl.cpp
```

## Run

**Terminal 1 — start the daemon:**
```bash
cd daemon
./syspulsed
```

**Terminal 2 — use the CLI:**
```bash
cd cli
./syspulsectl status
./syspulsectl set-threshold 70
```

Stop the daemon with `Ctrl+C` — it shuts down cleanly and removes its socket
file at `/tmp/syspulse.sock`.

## How it works

- A background thread samples `/proc/stat` and `/proc/meminfo` once per
  second, computing CPU usage as a delta between two samples (not a single
  snapshot) and memory usage from total vs. available memory.
- Shared state (current readings, threshold, alert flag) is protected by a
  mutex, since it is read and written from two threads: the monitor loop and
  the socket server.
- The socket server accepts simple text commands (`STATUS`,
  `SET_THRESHOLD <value>`) from CLI clients over a UNIX domain socket.
- When CPU usage crosses the configured threshold, the daemon logs an alert
  state change — a simplified stand-in for the kind of automatic response
  (throttling, signalling a GPIO/LED) a real embedded monitoring agent would
  perform.

## Why procfs/sysfs instead of a custom kernel module

An earlier iteration of this project explored a custom character-device
kernel module and a `uinput`-based virtual GPIO device. Both were dropped
after a WSL2 kernel header/version mismatch made module loading unreliable
within the project timeline. Reading and reacting to kernel-exposed
interfaces (`/proc`, `/sys`) is itself the standard way the large majority of
real embedded Linux userspace software interacts with the kernel and with
hardware state, so this remains a direct demonstration of the same
systems-programming and device-interfacing skills.

## Documentation

See `docs/` for the full Stage 1-6 documentation, UML diagrams, and the
project progress log.