# SysPulse — Stage 1: Project Introduction

**Project:** SysPulse: Embedded-Style System Resource Monitor & Control Daemon
**Areas covered:** Linux Device Drivers (via procfs/sysfs interfacing), System Programming, C++
**Author:** Shreyash Kar | **Date:** October 5, 2026 | **Repo:** https://github.com/iamyash5/syspulse

---

## 1. Project idea

SysPulse is a lightweight C++ daemon and CLI pair that monitors CPU and memory usage on a Linux system by reading kernel-exposed interfaces (`/proc/stat`, `/proc/meminfo`), applies a configurable threshold policy, and exposes live status and control to a command-line client over a UNIX domain socket.

## 2. Objective

Build a small but complete systems-programming stack: a background daemon that continuously reads kernel-exposed resource data, applies simple decision logic (threshold-based alerting), and exposes that state to userspace tools through standard Linux IPC (UNIX sockets) — the same architectural pattern used by real embedded and server monitoring agents.

## 3. Problem statement

Embedded and server systems constantly need to know their own resource state (CPU load, memory pressure, temperature) and react to it — for example, throttling work, raising an alert, or toggling an output when a threshold is crossed. Linux exposes this data through the `/proc` and `/sys` virtual filesystems rather than requiring every application to write its own kernel driver. Learning to read, parse and act on these interfaces correctly, from a long-running daemon with proper concurrency and IPC, is a core systems-programming skill.

## 4. Scope

**In scope**
- Daemon (`syspulsed`): background thread polling `/proc/stat` and `/proc/meminfo`, CPU percentage calculation (delta-based), memory usage percentage, configurable alert threshold.
- UNIX domain socket server for control/query, with a simple text command protocol (`STATUS`, `SET_THRESHOLD <value>`).
- CLI tool (`syspulsectl`): sends commands to the daemon and prints responses (`status`, `set-threshold <value>`).
- Clean shutdown on SIGINT/SIGTERM.
- Testing with manual load generation and automated checks.

**Out of scope (future work)**
- Writing a custom kernel module or character device (avoided deliberately — see note below).
- Historical data storage / graphing.
- Multi-host monitoring or networked control.
- GPIO/LED hardware output (would be a natural next step on real embedded hardware via `/sys/class/leds` or `/sys/class/gpio`).

## 5. Expected outcome

- A daemon that accurately reports live CPU and memory usage.
- A working alert mechanism that changes state when CPU usage crosses a user-configurable threshold.
- A CLI that can query status and change the threshold at runtime.
- Complete documentation, UML diagrams, and test results.

## 6. Applications

- **Embedded monitoring agents**: resource-aware daemons on embedded Linux boards that throttle or signal based on system load — this project mirrors that pattern directly.
- **Server/container health checks**: the same procfs-polling + threshold + socket-control pattern underlies many lightweight monitoring agents.
- **Educational**: demonstrates how userspace software interacts with the Linux kernel through its standard exposed interfaces, without needing to write or load kernel code.

## 7. A note on architecture choice

An earlier iteration of this project explored writing a custom kernel character-device driver and a uinput-based virtual GPIO device. Both were abandoned after environment-specific issues (a WSL2 kernel header/version mismatch preventing module loading) made them unreliable within the project timeline. SysPulse instead uses the standard, production-realistic pattern of interacting with the kernel through its existing procfs/sysfs interfaces — this is how the large majority of real embedded Linux userspace software (not just this project) actually reads sensors and system state, making it an equally valid demonstration of Linux systems-programming and device-interfacing skill.

## 8. Next stage

Stage 2: Project Requirements Document and development plan.
