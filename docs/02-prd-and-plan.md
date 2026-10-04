# SysPulse — Stage 2: Requirements (PRD) & Development Plan

**Author:** Shreyash Kar | **Version:** 1.0 | **Date:** October 5, 2026

---

## 1. Purpose

Defines what SysPulse must do, how it is delivered, and the remaining timeline to submission.

## 2. Functional requirements

### Daemon (`syspulsed`)
| ID | Requirement |
|---|---|
| FR-D1 | Read `/proc/stat` and compute CPU usage percentage from successive samples. |
| FR-D2 | Read `/proc/meminfo` and compute memory usage percentage. |
| FR-D3 | Run a background monitor loop, sampling once per second. |
| FR-D4 | Maintain a configurable CPU alert threshold (default 80%). |
| FR-D5 | Track and log alert state transitions (entering/leaving high-CPU state). |
| FR-D6 | Serve a UNIX domain socket (`/tmp/syspulse.sock`) accepting text commands. |
| FR-D7 | Support `STATUS` command returning current CPU%, memory%, threshold, alert state. |
| FR-D8 | Support `SET_THRESHOLD <value>` command to update the threshold at runtime. |
| FR-D9 | Shut down cleanly on SIGINT/SIGTERM, removing the socket file. |

### CLI (`syspulsectl`)
| ID | Requirement |
|---|---|
| FR-C1 | `syspulsectl status` — query and print current daemon state. |
| FR-C2 | `syspulsectl set-threshold <value>` — update the alert threshold. |
| FR-C3 | Print a clear error if the daemon is not running / socket unavailable. |

## 3. Non-functional requirements

| ID | Requirement |
|---|---|
| NFR-1 | **Correctness**: CPU percentage must be computed from deltas between two samples, not a single snapshot. |
| NFR-2 | **Concurrency safety**: shared state between monitor thread and socket server thread protected by a mutex. |
| NFR-3 | **Resource use**: daemon should have negligible CPU/memory footprint itself. |
| NFR-4 | **Code quality**: C++17, RAII where applicable, compiled clean with `-Wall -Wextra`. |
| NFR-5 | **Portability**: runs on any standard Linux system (including WSL2) with no root privileges and no kernel module. |
| NFR-6 | **Testability**: behaviour verifiable by generating CPU load and observing alert state change. |

## 4. Modules and deliverables

| Module | Language | Deliverable |
|---|---|---|
| `daemon/syspulsed.cpp` | C++17 | Background daemon: procfs readers, monitor thread, socket server |
| `cli/syspulsectl.cpp` | C++17 | Command-line control tool |
| `tests/` | C++ / shell | Verification scripts and/or unit tests for parsing logic |
| `docs/` | Markdown | Stage documents, UML diagrams, progress log, final report |

## 5. Assumptions and constraints

- Development and testing performed in WSL2 (Ubuntu) on Windows.
- Single developer, fixed deadline.
- `/proc/stat` and `/proc/meminfo` are assumed present and readable, as guaranteed on any standard Linux kernel.

## 6. Risks and mitigation

| Risk | Mitigation |
|---|---|
| Kernel module approach unreliable in WSL2 (encountered during development) | Pivoted to procfs/sysfs-based design requiring no custom kernel code |
| Short remaining timeline | Scope limited to CPU/memory monitoring and a two-command CLI protocol; no persistence or networking |
| Race conditions between monitor and server threads | Single mutex guarding all shared state, held only for short critical sections |

## 7. Development plan and timeline

| Day | Work |
|---|---|
| Day 1-2 | Explored kernel character-device and uinput approaches; encountered WSL2 environment blockers |
| Day 3 | Designed and implemented SysPulse: daemon (procfs readers, monitor thread, socket server), CLI, verified working end-to-end |
| Day 4 | Documentation (Stage 1-2, UML diagrams), testing, progress log, final report, submission |

## 8. Git strategy

- `main`: stable branch, each milestone committed directly given the compressed timeline.
- Commits made per completed feature (daemon core, CLI, docs) rather than one bulk commit.
- Repository: https://github.com/iamyash5/syspulse

## 9. Acceptance criteria

1. `syspulsed` runs continuously and reports plausible CPU/memory percentages.
2. `syspulsectl status` returns live values matching observed system load.
3. Generating CPU load causes the alert state to flip to active once the threshold is crossed, and back once load drops.
4. `syspulsectl set-threshold <value>` changes daemon behaviour at runtime without restart.
5. Daemon shuts down cleanly (no leftover socket file) on Ctrl+C.
6. All documentation, diagrams, and source code are committed and submitted.
