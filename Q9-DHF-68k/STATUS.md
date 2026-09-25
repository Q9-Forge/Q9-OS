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

> Legend: ❌ Not started, 🟡 Partially/rudimentary, ✅ Done

## Architecture note

This directory now contains two coexisting layers that must not be confused:

- **Kernel layer** (`manager/dhf_manager.c`, `manager/dhf.h`, `include/dhf_shared_api.h`, `docs/PROTOCOL.md`): the real OS-9 68k target code. The manager calls `_os9_f_viread()` on a `struct path_desc`, which is the genuine OS-9 IOMan mechanism to reach the registered driver module.
- **Host-simulator layer** (`host_simulator/`, `examples/`, `manager/dhf_manager_hostsim.c`, `include/dhf_shared_hostsim.h`): a native-compiled test harness that exercises the same driver logic (`driver/dhfdrv-68k.c`, `descriptor/`) via direct function calls, without requiring the real 68k target or emulator. Useful for fast iteration on filesystem logic.

The driver (`driver/dhfdrv-68k.c`) and descriptor (`descriptor/`) are shared between both layers and are target-agnostic C.
