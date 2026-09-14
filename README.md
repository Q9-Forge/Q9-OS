# Q9-OS

Q9-OS is the operating-system project for Q9: an independent, modular
kernel designed for compatibility with OS-9/68K and OS-9000 modules while
remaining open to new Q9-specific components.

German version: [README_de.md](README_de.md)

## Repository layout

- `Q9-KERNEL/` — kernel sources, currently focused on the 68k target
  (`Q9-KERNEL/68k/`), with shared code under `Q9-KERNEL/common/`
- `docs/` — design notes and development status reports
- `Scheduler.md` — scheduler/timer design notes
- `tools/` — supporting build and diagnostic scripts

Architecture-independent sources live in `common/`; architecture-specific
sources are kept below `68k/` or `x86/`.

## Status

The project is under active development. The 68k kernel work is the current
focus; the x86 and I/O-manager areas are prepared for future implementation.

## Background

Q9 draws inspiration from Microware OS-9/68K and OS-9000. The goal is not to
redistribute proprietary source code, but to build an independent
implementation informed by public documentation and independent study of
the system's observable, documented behavior.

## License

See the repository files for the applicable license and the documentation for
the status of reference material.
