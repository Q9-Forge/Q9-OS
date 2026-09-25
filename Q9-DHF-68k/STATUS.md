# Q9-DHF-68k Status

| Component | Function / Item | Status | Notes |
|---|---|---:|---|
| Kernel Manager (dhf_manager.c) | create/open/close/read/write/seek | ✅ Done | Forwards via `_os9_f_viread` (real OS-9 IOMan call) using the DHF_CMD_* protocol in dhf.h |
| Kernel Manager (dhf_manager.c) | getstat/setstat | ✅ Done | |
| Kernel Manager (dhf_manager.c) | delete/rename | ✅ Done | |
| Kernel Manager (dhf_manager.c) | mkdir (makdir)/chgdir/readdir | ✅ Done | Note: no separate rmdir opcode; delete is used for both files and empty dirs (OS-9 convention) |
| Driver (dhfdrv-68k.c) | open/read/write/close/seek/truncate | ✅ Done | Implemented against host filesystem with basepath confinement; merged in from host-simulator development |
| Driver (dhfdrv-68k.c) | mkdir/rmdir/unlink/rename/opendir/readdir | ✅ Done | with basepath confinement |
| Driver (dhfdrv-68k.c) | PD-based variants (read/write/close/seek) | ✅ Done | Manager writes emulated PD pointer; driver resolves and validates it |
| Driver (dhfdrv-68k.c) | error→Q9 errno mapping | 🟡 Partially | needs to align host errno with OS-9 error codes (see docs/DHF_ERRNO_MAP.md) |
| Descriptor | basepath getter/setter | ✅ Done | |
| Host Simulator | FM-command dispatch, memory-backend | ✅ Done | Used to test driver logic on the host before running on real 68k target; uses its own shared-memory layout (dhf_shared_hostsim.h), separate from the kernel wire protocol in docs/PROTOCOL.md |
| Manager (host-sim variant, dhf_manager_hostsim.c) | full forwarding incl. mkdir/rmdir/unlink/rename | ✅ Done | Test-only harness manager (direct function calls, not `_os9_f_viread`); kept separate from the real kernel manager |
| Emulator device (Musashi, `Q9-Flux/Q9-Flux-68k/src/devices/dhf/`) | open/close/create/read/write/seek/readln/writeln/getstat/setstat/chdir/mkdir/rmdir/delete/rename/opendir/readdir | ✅ Done | 2026-09-25, two iterations: first cut invented its own byte-copy-into-MMIO-buffer protocol; **replaced same day** with a thin devreg adapter around the **`Q9-OS/Q9-DHFDRV-68k`** sibling project's already-written `dhf_emu_device.c`/`dhf_host_fs.c` (copied into `Q9-Flux-68k/src/devices/dhf/`, flattened) -- zero-copy, `struct dhf_shared` (28 Byte, A0/A1 = guest RAM pointers, D0-D2 = handle/length/flags), device resolves A0/A1 directly against `board->ram`. MMIO base `$FFFF4000` (NOT `Q9-DHFDRV-68k`'s own `DHF_DEFAULT_HW_BASE=$FFFFD000` default -- that collides with this repo's `Q9_BOARD_RTC_BASE`). Verified via `make test-dhf` (16/16) and `make test-io-dispatch`. QEMU device: not started. |
| Real driver module (OS-9, Init/Read/Write/GetStat/SetStat entry points, `driver/dhfdrv_68k.a`) | ✅ Done (untested at runtime) | 2026-09-25: hand-written directly in 68k assembly using the `psect` pseudo-op (bypassing the still-incomplete QCC `QCC-DEFMODUL` driver/MODHEADER support), assembled+linked with our own `qr68k`/`ql68k`. Standard 7-word jump table (Init/Read/Write/GetStat/SetStat/Term/0); Read/Write/GetStat/SetStat share one body that copies the caller's path-descriptor fields (`SH_SEQ..SH_D2`, 24 bytes starting at offset 4) into the DHF MMIO block at `$FFFF4000` (matching `struct dhf_shared`/A0-A1/D0-D2 zero-copy protocol from `Q9-DHFDRV-68k`, i.e. the **same** protocol the Musashi emulator device above already speaks), writes `SH_COMMAND`, reads back `SH_STATUS`, copies `SH_D0..SH_D2` back out, and returns via carry+d1.w on error. Verified with `os9 ident`/`os9 dump`: good CRC ($058E01), good header parity ($3E42), correctly identified as "Dev Drv, 68000 obj, Sharable", jump table offsets plausible (Init/Term -> 0x4a, Read/Write/GetStat/SetStat -> 0x50). **Not yet run inside the actual emulator** -- only static header/CRC validity confirmed so far, no live Init/Read/Write call has been exercised against the real `q9_devtype_dhf` device. This still leaves the manager-to-driver calling convention (`_os9_f_viread`) itself unverified -- see next row. |

> Legend: ❌ Not started, 🟡 Partially/rudimentary, ✅ Done

## Architecture note

This directory now contains two coexisting layers that must not be confused:

- **Kernel layer** (`manager/dhf_manager.c`, `manager/dhf.h`, `include/dhf_shared_api.h`, `docs/PROTOCOL.md`): the real OS-9 68k target code. The manager calls `_os9_f_viread()` on a `struct path_desc`, which is the genuine OS-9 IOMan mechanism to reach the registered driver module.
- **Host-simulator layer** (`host_simulator/`, `examples/`, `manager/dhf_manager_hostsim.c`, `include/dhf_shared_hostsim.h`): a native-compiled test harness that exercises the same driver logic (`driver/dhfdrv-68k.c`, `descriptor/`) via direct function calls, without requiring the real 68k target or emulator. Useful for fast iteration on filesystem logic.

The driver (`driver/dhfdrv-68k.c`) and descriptor (`descriptor/`) are shared between both layers and are target-agnostic C.

**Third layer, added 2026-09-25** (`Q9-Flux/Q9-Flux-68k/src/devices/dhf/`, separate repo): the Musashi-emulator-side device, reachable from real/emulated 68k code via ordinary MMIO reads/writes at `$FFFF4000`. Its protocol and internals actually come from the **`Q9-DHFDRV-68k`** sibling project (see below), not from this directory's `driver/dhfdrv-68k.c` -- nothing in *this* repo talks to it yet.

**Sibling project, `Q9-OS/Q9-DHFDRV-68k`** (committed 2026-09-24, independently of this directory): a second, more complete DHF implementation with its own driver (`driver/dhfdrv.c`, A0/A1/D0-D2 zero-copy register protocol) and its own emulator-side device (`emulator/dhf_emu_device.c`) plus an optional TCP-forwarding backend for a remote host. It has **no manager component** -- it was presumably meant to plug into (or replace) this directory's driver layer underneath `manager/dhf_manager.c`, but that reconciliation was never done. The 2026-09-25 Musashi device above is built from `Q9-DHFDRV-68k`'s code, not this directory's.
