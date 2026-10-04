# SysPulse — Progress Log

## Day 1
- Set up development environment (WSL2 Ubuntu), build tools, Git repository.
- Explored initial project direction: a userspace network stack with packet
  filtering (NetStack Lab), then pivoted to an embedded-focused design
  (ArduLink) using a simulated Arduino over a virtual serial link, after
  evaluating relevance to embedded-systems hiring.
- Wrote Stage 1 and Stage 2 documentation for the ArduLink design.

## Day 2
- Began kernel character-device driver implementation for a virtual GPIO
  framework (VirtGPIO).
- Hit a WSL2-specific blocker: kernel headers for the running WSL2 kernel
  were not available via `apt`, since WSL2 uses a Microsoft-maintained
  kernel rather than the distribution's own.
- Built the WSL2 kernel source tree to obtain matching headers; verified
  with a hello-world kernel module (successful load/unload).
- Wrote the VirtGPIO character-device driver (`ioctl`-based pin control).
  Build succeeded, but module load failed with "Invalid module format" —
  diagnosed as a version mismatch between the running kernel
  (6.18.33.2) and the built kernel source (6.18.40.1+).

## Day 3
- Evaluated options to resolve the kernel version mismatch; given the
  remaining timeline, decided against further custom kernel module work.
- Pivoted to a `uinput`-based virtual GPIO approach (no custom module
  required, using Linux's built-in input subsystem) and verified a working
  proof-of-concept.
- Given continued time pressure, made a final architecture decision:
  SysPulse — a daemon/CLI pair interacting with the kernel through its
  existing procfs interfaces (`/proc/stat`, `/proc/meminfo`) instead of a
  custom or virtual device. This removed all WSL2-specific environment
  risk.
- Implemented and tested `syspulsed` (monitor thread, threshold alerting,
  UNIX socket server) and `syspulsectl` (CLI). Verified end-to-end:
  status queries, runtime threshold changes, and alert state transitions
  under real CPU load. Committed and pushed working code.

## Day 4
- Wrote Stage 1 (Introduction) and Stage 2 (PRD & Development Plan)
  documentation for the final SysPulse design.
- Created Stage 3 UML diagrams: architecture, class, and sequence
  diagrams.
- Wrote README with build/run instructions and this progress log.
- Finalized testing, Stage 6 report, and submission.