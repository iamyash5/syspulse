# SysPulse — Stage 6: Final Report

**Project:** SysPulse: Embedded-Style System Resource Monitor & Control Daemon
**Author:** Shreyash Kar | **Date:** October 5, 2026
**Repository:** https://github.com/iamyash5/syspulse

---

## 1. Summary

SysPulse is a C++ daemon/CLI system that monitors CPU and memory usage on Linux by reading the kernel's procfs interface (`/proc/stat`, `/proc/meminfo`), applies a configurable threshold-based alert policy, and exposes live status and runtime control through a UNIX domain socket. It demonstrates core Linux systems-programming skills — concurrent background processing, IPC, and kernel-interface interaction — in the style used by real embedded and server monitoring agents.

## 2. Final architecture

- **`syspulsed` (daemon)**
  - A monitor thread samples `/proc/stat` and `/proc/meminfo` once per second, computing CPU usage from the delta between two successive samples and memory usage from total vs. available memory.
  - Shared state (current readings, configurable threshold, alert flag) is protected by a `std::mutex`, since it is accessed by both the monitor thread and the socket-server thread.
  - A UNIX domain socket server (`/tmp/syspulse.sock`) accepts simple text commands from clients: `STATUS` and `SET_THRESHOLD <value>`.
  - The daemon logs alert state transitions when CPU usage crosses the threshold, and shuts down cleanly on SIGINT/SIGTERM, removing its socket file.

- **`syspulsectl` (CLI)**
  - Connects to the daemon's UNIX socket, sends a command, prints the response.
  - Supports `status` and `set-threshold <value>`.

See `docs/diagrams/` for the architecture, class, and sequence diagrams.

## 3. Implementation and testing results

- The daemon was verified to report plausible, live CPU and memory percentages that track actual system load.
- `syspulsectl status` correctly reflects daemon-side state at query time.
- `syspulsectl set-threshold <value>` was confirmed to update daemon behaviour at runtime, without restarting the daemon.
- Alert state was verified to flip to active when CPU usage was pushed above a (deliberately low) threshold using a CPU-bound background process, and to clear once load dropped.
- Clean shutdown (Ctrl+C) was confirmed to terminate the daemon and remove the socket file, leaving no stale state.
- An automated integration test script (`tests/test_syspulse.sh`) exercises all of the above — binary presence, daemon startup, socket creation, `STATUS`/`SET_THRESHOLD` correctness, alert behaviour under load, and clean shutdown — and reports a pass/fail summary.

## 4. Development process and challenges

Development did not follow a single straight path, and that process is itself part of the engineering record (see `docs/progress-log.md` for full day-by-day detail):

1. The project began as **NetStack Lab**, a userspace network stack and packet-filtering framework, then pivoted to **ArduLink**, a simulated-Arduino serial-protocol framework, after evaluating relevance to embedded-systems hiring criteria.
2. Implementation then moved to **VirtGPIO**, a custom Linux character-device kernel module (`ioctl`-based virtual GPIO pins). This required building the WSL2 kernel source tree to obtain matching headers, since WSL2 ships a Microsoft-maintained kernel not covered by standard `apt` header packages. A hello-world module confirmed the toolchain worked, and the full VirtGPIO driver was written and compiled successfully.
3. Loading the compiled module failed with `Invalid module format`, diagnosed as a version mismatch between the actually-running WSL2 kernel (`6.18.33.2`) and the kernel source tree available to build against (`6.18.40.1+`). This is an environment-specific limitation of WSL2, not a defect in the driver code itself.
4. A `uinput`-based virtual GPIO approach was prototyped next, using Linux's built-in input subsystem to avoid compiling a custom module. This worked as a proof of concept.
5. With the deadline close, a final decision was made to consolidate around **SysPulse**: an architecture that interacts with the kernel exclusively through its already-exposed procfs interfaces, eliminating all WSL2-specific environment risk while preserving the core learning objective — correct, concurrent, well-structured interaction between userspace C++ code and kernel-exposed system state.

This process reflects a realistic engineering constraint: environment limitations outside the code's control (a development-platform kernel/header mismatch) required a scoped, judged pivot under time pressure, rather than continued investment in an approach that could not be reliably verified within the available time.

## 5. Limitations

- SysPulse does not write to a custom or virtual device; it reads existing kernel interfaces rather than exposing a new one.
- No persistent history or graphing of resource data over time.
- Single-host only; no networked or multi-host monitoring.
- The alert mechanism logs state transitions but does not yet drive an external output (e.g. an LED or GPIO), though the architecture is positioned to add this.

## 6. Future improvements

- On real embedded hardware (or a Linux environment without WSL2's kernel-header constraints), extend the alert mechanism to drive an actual output via `/sys/class/leds` or `/sys/class/gpio`, closing the loop from "detect" to "act."
- Revisit the custom kernel character-device driver (VirtGPIO) in an environment with matching kernel headers, since the driver code itself compiled successfully and only failed at load time due to the environment mismatch.
- Add persistent logging/history and a simple time-series view of resource usage.
- Extend the CLI protocol with authentication or access control for multi-user systems.

## 7. Conclusion

SysPulse meets its stated requirements: a working daemon that monitors system resources via kernel-exposed interfaces, applies configurable threshold logic, and exposes status and control to a CLI over standard Linux IPC, built and tested entirely in C++17. The project also documents a realistic instance of adapting scope and architecture in response to environment constraints discovered during development, while preserving the core systems-programming and kernel-interfacing learning objectives of the assignment.