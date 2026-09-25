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
| Emulator device (Musashi, `Q9-Flux/Q9-Flux-68k/src/devices/dhf/q9_dhf.c`) | open/close/read/write/seek/mkdir/rmdir/unlink/rename/opendir/readdir/closedir/truncate, basepath confinement | ✅ Done | 2026-09-25: first step of the plan in `Q9-Flux-68kQEMU/docs/HOSTFS_MANAGER_de.md` ("Q9-Flux-Emulator zuerst"). MMIO window `$FFFF4000-4FFF`, CMD/HANDLE/RESULT/ERRNO/ARG1/ARG2 registers + PATH/PATH2/DATA buffers, all big-endian. The filesystem logic itself is the same as `driver/dhfdrv-68k.c`, just running on the correct side of the guest/host boundary now. Verified via `make test-dhf` (19/19, direct C-API against a real temp dir, no CPU) and `make test-io-dispatch` (confirms the address is reachable through Musashi's real dispatch path, no collision). Basepath still hardcoded at attach time in `m68krt.c` (like `rtc72421` initially) -- a config-driven `hostpath` field (like `cf`'s `image` field) is a separate follow-up. QEMU device: not started. |
| Real driver module (OS-9, Init/Read/Write/GetStat/SetStat entry points reachable via `_os9_f_viread`) | ❌ Not started | This is the actual missing link between the kernel manager and the new emulator device: `dhf_manager.c` calls the driver via `_os9_f_viread(pd->pd_dev, ...)`, i.e. a real OS-9 driver module with the standard jump table -- NOT the plain C functions in `driver/dhfdrv-68k.c` (those are only ever called by the host-simulator manager, `dhf_manager_hostsim.c`). Building this needs QCC's `driver` calling-convention/MODHEADER support, which is mid-flight on the `QCC-DEFMODUL` branch (not merged into `Q9-QCC` main, parser still incomplete as of 2026-09-25). |

> Legend: ❌ Not started, 🟡 Partially/rudimentary, ✅ Done

## Architecture note

This directory now contains two coexisting layers that must not be confused:

- **Kernel layer** (`manager/dhf_manager.c`, `manager/dhf.h`, `include/dhf_shared_api.h`, `docs/PROTOCOL.md`): the real OS-9 68k target code. The manager calls `_os9_f_viread()` on a `struct path_desc`, which is the genuine OS-9 IOMan mechanism to reach the registered driver module.
- **Host-simulator layer** (`host_simulator/`, `examples/`, `manager/dhf_manager_hostsim.c`, `include/dhf_shared_hostsim.h`): a native-compiled test harness that exercises the same driver logic (`driver/dhfdrv-68k.c`, `descriptor/`) via direct function calls, without requiring the real 68k target or emulator. Useful for fast iteration on filesystem logic.

The driver (`driver/dhfdrv-68k.c`) and descriptor (`descriptor/`) are shared between both layers and are target-agnostic C.

**Third layer, added 2026-09-25** (`Q9-Flux/Q9-Flux-68k/src/devices/dhf/`, separate repo): the Musashi-emulator-side device. It is reachable from real/emulated 68k code via ordinary MMIO reads/writes at `$FFFF4000`, and internally does the same host filesystem calls as `driver/dhfdrv-68k.c` -- but nothing in this repo talks to it yet. The still-missing piece is the real OS-9 driver module described in the STATUS table above (`_os9_f_viread` target); once that exists, its entry points are what would poke this MMIO window.
