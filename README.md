# Q9-OS

Q9-OS is the operating-system project for Q9: an independent, modular
kernel designed for compatibility with OS-9/68K and OS-9000 modules while
remaining open to new Q9-specific components.

German version: [README_de.md](README_de.md)

## Repository layout

- `Q9-KERNEL/` — common and architecture-specific kernel work
- `Q9-IOMAN/` — I/O manager work
- `Q9-Manager/` — system managers such as RBF, SCF, SBF and PIPE
- `docs/` — design notes, walkthroughs and reverse-engineering records
- `vendor/` — ignored/reference material where applicable

Architecture-independent sources live in `common/`; architecture-specific
sources are kept below `68k/` or `x86/`.

## Status

The project is under active development. The 68k kernel work is the current
focus; the x86 and I/O-manager areas are prepared for future implementation.

## Background

Q9 draws inspiration from Microware OS-9/68K and OS-9000. The goal is not to
redistribute proprietary source code, but to build an independent
implementation informed by public documentation, compatible module formats
and documented observations.

## License

See the repository files for the applicable license and the documentation for
the status of reference material.
