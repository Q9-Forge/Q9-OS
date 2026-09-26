# Q9-DHF-68k Status

| Component | Function / Item | Status | Notes |
|---|---|---:|---|
| Kernel Manager (dhf_manager.c) | create/open/close/read/write/seek | ✅ Done (logic only, not yet wired to a real ABI) | Forwarding logic uses the DHF_CMD_* protocol in dhf.h, but the call it forwards *through* (`_os9_f_viread`) **does not exist anywhere in the real Microware SDK** (verified 2026-09-25 by grepping the full local MWOS source tree -- zero hits). This was a placeholder invented early in the project and never corrected until now; see "Real driver-call mechanism" below and the new `manager/dhfmgr_68k.a` row for the real replacement path. |
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
| Real driver module (OS-9, Init/Read/Write/GetStat/SetStat entry points, `driver/dhfdrv_68k.a`) | ✅ Done, runtime-verified | 2026-09-25: hand-written directly in 68k assembly using the `psect` pseudo-op (bypassing the still-incomplete QCC `QCC-DEFMODUL` driver/MODHEADER support), assembled+linked with our own `qr68k`/`ql68k`. Standard 7-word jump table (Init/Read/Write/GetStat/SetStat/Term/0); Read/Write/GetStat/SetStat share one body that copies the caller's path-descriptor fields (`SH_SEQ..SH_D2`, 24 bytes starting at offset 4) into the DHF MMIO block at `$FFFF4000` (matching `struct dhf_shared`/A0-A1/D0-D2 zero-copy protocol from `Q9-DHFDRV-68k`, i.e. the **same** protocol the Musashi emulator device above already speaks), writes `SH_COMMAND`, reads back `SH_STATUS`, copies `SH_D0..SH_D2` back out, and returns via carry+d1.w on error. Static validity confirmed with `os9 ident`/`os9 dump` (good CRC/header parity, correctly identified as "Dev Drv, 68000 obj, Sharable"). **Same day, additionally runtime-verified end-to-end**: `Q9-Flux/Q9-Flux-68k/test/14_test_dhf_driver.c` (`make test-dhf-driver`) loads the real `.mod` into guest RAM and runs its Init/Read/Write/GetStat/SetStat entry points on the actual Musashi CPU core against the real `q9_devtype_dhf` MMIO device and a throwaway host directory -- CREATE/WRITE/CLOSE/OPEN/READ/CLOSE/DELETE (with content roundtrip) and the confinement error path (rejecting `..`, carry+d1 both correct) all pass. |
| Real driver-call mechanism (how a FileManager reaches this driver module) | ✅ Researched, documented | 2026-09-25: replaces the false `_os9_f_viread` assumption above with facts read directly from Microware's own shipped headers (`MWOS/OS9/SRC/DEFS/{sysio,path,module}.h`, confidential-but-locally-present SDK source) and cross-checked against the **real, booted** kernel FileManager binary (`Q9-Images/cf_images/OS9SYS/CMDS/BOOTOBJS/rbf`, hex-dumped directly). Path descriptor (`union pathdesc`, `path.h`) has `pd_dev` -> `Devicetbl` (`sysio.h`): `{ V_driv (driver module ptr), V_stat (driver static storage base), V_desc, V_fmgr, V_usrs, ... }`. The driver module's header extension (`module.h` `mod_driver`, laid out right after the common 48-byte header) has **named, fixed-offset fields** `_mdinit/_mdread/_mdwrite/_mdgetstat/_mdsetstt/_mdterm/_mderror` -- exactly the same order/meaning as the `D_INIT=0/D_READ=2/D_WRIT=4/D_GSTA=6/D_PSTA=8/D_TERM=10/D_TRAP=12` byte-offset constants also defined in `sysio.h`. A caller therefore reads a 16-bit routine offset from `V_driv + 0x3C + D_xxx` (0x3C = `offsetof(mod_driver,_mdinit)`, confirmed against our own `dhfdrv_68k.mod`'s real header bytes -- its own DhfEnt table happens to land at exactly that same fixed offset, since `qr68k`/`ql68k`'s `psect Q9Drivr` output already matches the standard `mod_driver` layout byte-for-byte), adds it to `V_driv` to get the absolute routine address, and `jsr`s there with `a1=path descriptor, a2=V_stat, a4=process descriptor, a6=system global` -- the same convention already used (and runtime-verified) for `test-dhf-driver` above, just now with a documented, non-invented source. |
| FileManager module (OS-9, `manager/dhfmgr_68k.a`) | 🟡 6 of 13 calls wired via one shared body, structurally verified, not yet runtime-tested | 2026-09-25/26/27, three iterations (see file header for the full history). Module type `Q9FlMgr=13` (`MT_FILEMAN`), assembled+linked with `qr68k`/`ql68k`. Jump table has **13 entries** in ascending I$-callcode order (`funcs.h`: `I_CREATE=$83 .. I_CLOSE=$8f`), verified against a hex-dump of the real, booted `rbf` kernel module (13-word table at its `_mexec` offset). Register-level conventions sourced from the real Microware "OS-9 v2.4 Technical I-O Manual" (`a1`=path descriptor, `a4`=process descriptor, `a5`=pointer to the **caller's saved REGISTERS struct**, `reg.h`: `d[8]` then `a[8]`, so caller's `d0`/`d1`/`a0` live at `a5+0`/`a5+4`/`a5+32`, `a6`=system global) and "OS-9 System Calls" Chapter 2 (exact register tables per I$ call), not guessed. **2026-09-27 redesign (user-directed simplification):** rather than one bespoke routine per call, all 13 jump-table slots are now tiny 6-byte thunks (`moveq #<DHF_CMD_x>,d3 / bra MgrCommon`) that fall into **one shared body**. `MgrCommon` uniformly copies the caller's `a0`/`d0`/`d1` (from the `a5`-pointed register struct) into a 28-byte `dhf_shared` command block's `a0`/`a1`/`d2`/`d1` fields (both `a0` and `a1` get the same value -- pathname and data-buffer calls both source it from the caller's `a0`), sets `d0` = the path's own `pd_pd` number, and `command` = the tagged `DHF_CMD_*` -- then calls the driver via the `Devicetbl`/`mod_driver` mechanism (documented in the row above), writes the result back to the caller's `d1`, and returns. This mapping is exact for `Create`/`Open`/`Read`/`Write`/`Close`/`Delete` (the six now wired) but does **not** fit `Seek` (needs a "whence" the caller's registers don't carry this way), `GetStt`/`SetStt` (function-code-dependent parameter shapes), or `MakDir`/`ChgDir` (register convention not yet verified against a real source) -- those seven remain `E$UnkSvc` stubs, sharing one address, by design rather than oversight. **The command-block memory itself is no longer static/persistent**: `MgrCommon` obtains it fresh from the kernel every call (`F$SRqMem`, `MWOS/SRC/DEFS/funcs.h`: callcode `$28`) and returns it before returning to the caller (`F$SRtMem`, callcode `$29`) -- register conventions (`d0.l`=byte count, `a2`=block address, carry+`d1.w`=error) again from "OS-9 System Calls" Chapter 1, not guessed. This eliminates the earlier design's fixed `MAXPATHS`-sized static table entirely (module needs zero static storage now, `psect` static-size parameter back to 0, matching the driver). **The cross-call persistence problem this raises (Read/Write/Close need to find the same open file as a prior Create/Open) is solved on the simulator side, not the manager side** (user's insight: "der Simulator macht das... dort können wir ein Array anlegen"): `d0` now always carries the caller's own OS-9 path number (`pd_pd`) instead of a driver-invented handle, and `Q9-Flux-68k/src/devices/dhf/dhf_host_fs.c` / `Q9-OS/Q9-DHFDRV-68k/emulator/dhf_host_fs.c` gained `dhf_host_fs_open_at()`/`create_at()` (2026-09-26) which use a caller-supplied index into the already-existing `DHF_MAX_HANDLES=64` array instead of auto-allocating one -- the path number already lives exactly as long as IOMan keeps the path open, so nothing new needs to persist anywhere. `dhf_emu_device.c`'s `DHF_CMD_OPEN`/`DHF_CMD_CREATE` cases updated accordingly (both flattened copies kept in sync); `make test-dhf` (16/16) and `make test-dhf-driver` (all checks) still pass unchanged after this device-side change, as does `Q9-DHFDRV-68k`'s own `make test`. Manager build verified with `os9 ident`/`os9 dump`: good CRC/parity, "File Mngr, 68000 obj, Sharable", 300 bytes (down from 556 in the previous, more repetitive version); the six wired entries point at six distinct, tightly-packed 6-byte thunk addresses ($56/$5c/$62/$68/$6e/$74), the seven stubs still share one address. **2026-09-27, real-kernel load confirmed** (though not yet a live entry-point call): copied both `dhfdrv_68k.mod`/`dhfmgr_68k.mod` into `/CMDS/dhf/` of a fresh, dedicated boot image (`Q9-Images/cf_images/OS9SYS_Claude.hda`, an APFS-CoW clone of the current master, plus `Q9-Images/rom_images/Claude_cb030_BIOS.BIN`) via ToolShed, then actually booted the real Musashi emulator (`Q9-Flux-68k/test/expect/claude_dhf_load.exp`) and ran the **real OS-9 kernel's own** `load` command on both -- no error from either, and `mdir` (bare, listing all resident modules) shows `dhfdrv_68k.mod` and `dhfmgr_68k.mod` sitting right alongside `kernel`/`ioman`/`rbf`/`scf`/etc. This is a stronger check than our own `os9 ident`/`os9 dump`: it's the real kernel's own CRC/header/link-table validation accepting both modules, not just our tooling. **Still missing before a live entry-point call is possible**: a DHF *device descriptor* module (`Q9Devic` type, analogous to `rbf.des`/`scfdesc.a`) naming `dhfmgr_68k` as file manager and `dhfdrv_68k` as driver, plus an `I$Attach`/path-open from a running OS-9 program -- that's the next concrete step toward a true end-to-end test of the manager (the driver alone already has that, via `test-dhf-driver`'s synthetic Musashi-CPU harness). |
| Device descriptor (OS-9, `descriptor/dhf0.a`) + real `I$Attach` test | 🟡 Descriptor built+byte-verified, driver gained a real `Init`, but live `iniz dhf0` in the real kernel still fails -- root cause NOT yet found | 2026-09-27. Descriptor written after the real Microware sources `MWOS/OS9/SRC/IO/RBF/DESC/rbfdesc.a` and `.../SCF/DESC/scfdesc.a` (field order/names copied exactly, cross-checked against `module.a`'s `M$Port/M$Vector/M$IRQLvl/M$Prior/M$Mode/M$FMgr/M$PDev/M$DevCon/.../M$Opt` offsets). Three things are configurable without rebuilding the driver/manager (as requested): `DHF_PORT` (`M$Port`, hardware/MMIO base -- the kernel copies this into the driver's own static storage at offset 0, `V_PORT`, *before* calling Init, verified against `MWOS/OS9/SRC/IO/SCF/DRVR/sc68681.a`'s real `Init`), the descriptor's own module name (`nam`, e.g. `dhf0` -- what you `iniz` by), and the host basepath (via a custom one-word options table entry the driver's `Init` reads through `M$DevCon`, then forwards to the emulator as `DHF_CMD_INIT` -- a command that already existed in `dhf_emu_device.c` but nothing ever triggered it before). `os9 ident`/`os9 dump` on the built module confirm every field byte-exact (`Dev Descr, Data, Sharable`, `M$Port=$FFFF4000`, `M$Mode=$87`, `M$FMgr`/`M$PDev`/`M$DevCon` offsets all resolve to the right strings). **Driver/manager also fixed along the way**: (1) `driver/dhfdrv_68k.a` Read/Write/GetStat/SetStat previously read/wrote the DHF MMIO block at a *hardcoded* `$FFFF4000` -- now reads it from `(a2)` (`V_PORT`) every time, making `DHF_PORT` in the descriptor actually take effect; this also exposed a **real pre-existing bug**: the driver's `psect` declared `static-size=0`, which is what the kernel allocates for `(a2)` -- harmless while nothing read `(a2)`, but wrong now (fixed to `46`, `sizeof(sysioStatic)`, per `MWOS/OS9/SRC/DEFS/iodev.a`). (2) both `driver/dhfdrv_68k.a` and `manager/dhfmgr_68k.a` were missing the **system-state attribute bit** (`SupStat`, `$20`) that "OS-9 v2.4 Technical I-O Manual" explicitly requires for I/O system modules ("`Attr_Revs equ ((ReEnt+Supstat)<<8)+0`" in its own sample FileManager listing) -- both only had `ReEnt`; fixed, `os9 ident` now shows "System State Process" on both. All of this is real, verified progress (synthetic `test-dhf-driver` harness updated to supply a fake `V_PORT` the same way the real kernel would, and still passes all checks end-to-end). **What's still broken**: `iniz dhf0` in the actual booted emulator fails with a generic "can't attach" from the `iniz` utility. `Q9_TRAP_TRACE_ALL` instrumentation (env var already built into `m68krt.c`) shows the underlying `I$Attach` syscall (callcode `$80`) returning carry set, `d1=$dd` = `E$MNF` ("Module Not Found") -- but WHICH module is not reliably identifiable: a control test (`iniz h0`, a real, already-loaded-but-not-yet-attached CF device) succeeds fine in the same environment, ruling out anything generic/environmental. A theory that `I$Attach` truncates the `M$FMgr`/`M$PDev` name to 6 characters before its own internal `F$Load` (based on a trace line reading `name=/dd/CMDS/dhfmgr` for what should have been `dhfmgr_68k`) led to renaming both modules' internal names to `dhfmgr`/`dhfdrv` (≤6 chars) -- **this did not fix it**, and on reflection the trace evidence for that theory is suspect (the tracer's own name-reading helper, `m68krt_read_os9_name()`, stops at the first byte with its high bit set, an OS-9 pathlist convention that may not apply cleanly to this particular call site -- so that 6-char reading may itself have been an artifact of misapplying the tracer, not a real kernel limit). The rename is harmless and kept, but should **not** be read as a confirmed fix. Root cause of the `E$MNF` is still open. Next steps to consider: build a deliberately minimal descriptor+driver+filemanager from scratch to bisect against a known-working baseline, or find a way to trace `I$Attach`'s *internal* linking (which appears to bypass the trap-gate instrumentation entirely, unlike explicit user `load` commands). |

| `iniz dhf0` E$MNF root cause | ✅ Found and fixed (real bug, not the 6-char theory) | 2026-09-26 (real system date; the three rows above mislabel themselves 2026-09-27, apparently a session date-tracking slip -- not corrected here to avoid rewriting history, but don't trust those dates). Root cause: **`ql68k`'s `-O=<file>` names the module after the OUTPUT FILE NAME unless `-n=<name>` is also given** (`ql68 -n=<name>  Modulname (sonst aus dem Ausgabenamen)` -- confirmed from its own `--help`). None of the three `ql68k` invocations used so far (driver, manager, descriptor) ever passed `-n=`, so despite every source file's own `nam dhfdrv`/`nam dhfmgr`/`nam dhf0` line, the actual resident module names (confirmed by parsing the real module header's `_mname` offset, and independently by `mdir` on the booted image) were `dhfdrv_68k.mod`, `dhfmgr_68k.mod`/`dhfdrv_68k_noinit.mod` (an even older leftover build) and `dhf0.mod` -- i.e. the assembler's `nam` directive is COSMETIC for `qr68k`'s own `.r` intermediate, `ql68k` alone decides the final module name. This fully explains the E$MNF without needing the previous session's 6-char-truncation theory (which was investigating a red herring -- the tracer's `m68krt_read_os9_name()` 6-char reading was real but irrelevant, since the actual resident names weren't even close to matching regardless of length). **Fix applied**: rebuilt all three (`cd Q9-QCC/runtime/os9 && qr68k <src>.a -o=<src>.r && ql68k <src>.r -n=<shortname> -O=<src>.mod`), re-copied into `OS9SYS_Claude.hda`'s `/CMDS/dhf/` via ToolShed `os9 copy -r` + `os9 attr -e` (execute-bit workaround, see host repo's own device-descriptor doc), confirmed via `mdir` in the real booted kernel: now shows bare `dhfmgr dhfdrv dhf0` (previously the `_68k`/`_68k.mod`/`_noinit` garbage names). **Result: the error changed** from `E$MNF` (`$dd`) to `E$PERMIT` (`$a4`, "You must be super user to do that") -- confirms the descriptor/filemanager/driver names now resolve correctly and attach gets further than before. |
| `iniz dhf0` E$PERMIT (new blocker) | 🟡 Reproducible, kernel-internal, root cause NOT yet found | 2026-09-26. Ruled out: not a login/privilege issue (`super` account is `group.user=0.0` per `/SYS/password`; a same-session control test, `iniz h1` on an already-loaded-but-unattached real CF device, succeeds fine) -- so the *process* has superuser rights, something about attaching *this specific* descriptor triggers a different, permission-gated code path. Ruled out: not our own driver/manager code returning the error -- `driver/dhfdrv_68k.a`'s `Init` unconditionally clears carry before returning (never signals an error itself), and neither DHF's own error-code enum (`dhf_proto.h`, all values 205-242) nor `dhf_host_fs.c`'s errno mapping produces `0xa4`; it must come from the real OS-9 kernel's own logic. Traced with a **new diagnostic** (`Q9_ITRACE_PATH=/dhf0` env var, added this session to `src/kernel/m68krt.c`'s `m68krt_trap_trace_callback` -- arms `Q9_ITRACE_CALLCODE`'s per-instruction trace on a SPECIFIC path string match instead of the first occurrence of that callcode, needed because `/dhf0` is opened dozens of times' worth of *other* callcode-0x84 calls into the boot before the one we care about; usage: `Q9_TRAP_TRACE=<file> Q9_TRAP_TRACE_ALL=1 Q9_ITRACE_CALLCODE=84 Q9_ITRACE_PATH=/dhf0 Q9_ITRACE_N=<budget>`, NOT YET committed). With a 40000-instruction budget: **PC never once leaves the kernel-resident low-memory range (`< $20000`)** -- i.e. the real kernel's own I$Open/attach logic decides E$PERMIT entirely internally, without ever jsr-ing into our loaded `dhfmgr`/`dhfdrv`/`dhf0` modules (which load far higher, `$00e5xxxx`+). Found what looks like a **module-directory linear search / byte-by-byte name-compare loop** (bouncing between `pc=$8bd6..$8c44`, sequential `a0`/`a1` pointer increments with a per-byte compare in `d2`, distinct from the `$8c3c`/`$9902` outer loop that iterates it once per resident module) that is *still running* at the end of the 40000-instruction capture -- the actual `E$PERMIT`-setting code lies beyond that, not yet captured. No real Microware kernel/IOMan source is available locally to shortcut this (the SDK ships RBF/SCF/driver sources but not the kernel/IOMan internals) -- would need either a much larger `Q9_ITRACE_N` budget (cheap to try) or a disassembly pass (capstone, per established convention) over the `$8bd6-$9920`-ish PC range against a raw guest-RAM dump taken while paused there, to identify what the search is actually comparing and why our descriptor fails it. Next step for a future session: re-run with `Q9_ITRACE_N=200000`+ to capture past the loop into the actual branch that sets `d1=$a4`. **UPDATE, same session, minutes later**: root cause found -- see next row, this was a real bug, not a kernel mystery. |
| `E$PERMIT` root cause + fix | ✅ Found and fixed | 2026-09-26. All three of *our* modules had **module header owner `0.1`** (`os9 ident`'s "Owner:" field, the `_mowner` field baked into the module header at link time by `ql68k`), while every real Microware-shipped module (`cfide`, `rbf`, `scf`, checked directly) has owner `0.0`. `ql68k --help` has a `-gu=<gruppe>.<nutzer>` flag ("Eigentuemer des Moduls") that none of the three builds had used, so they picked up some tool default instead of `0.0`. Rebuilt all three adding `-gu=0.0` to the existing `-n=<name>` invocation, re-copied+re-`attr -e`'d into the image; `os9 ident` now shows `Owner: 0.0` on all three, matching the real kernel modules. **Result: the error changed again**, from `E$PERMIT` (`$a4`) to `E$NORAM` (`$ed`, "No Ram Available") -- confirms this really was the next gate (some check compares the descriptor/filemanager/driver module's owner field against the caller somehow -- exact kernel logic not traced, not needed since matching the real modules' convention was sufficient). |
| `E$NORAM` (current blocker) | 🟡 Reproducible, root cause NOT yet found, promising lead | 2026-09-26, same `Q9_ITRACE_PATH=/dhf0` technique (this time execution finally leaves the kernel-resident low-memory range -- 4904 distinct PCs above `$20000` reached, first time any of these traces left kernel territory). The generic error-return tail (`pc=$edae` onward, the same shared "set carry + return" dispatcher every error in this investigation has funneled through) is fed `d1=$ed` from a `moveq`-like store at `pc=$edf8`, reached via a loop at `pc=$ecb4..$ece8` that walks a table of **32-byte-spaced entries** (`a2` stepping `$ffe530 -> $ffe550 -> $ffe570 -> $ffe590...`, `d0` counting DOWN from roughly 13) while `a1` holds a constant `$ffff1080` (an MMIO-range address, though not our own `DHF_PORT=$FFFF4000` -- likely just a carried-over register, not necessarily read in this loop). Reads exactly like a **kernel memory-descriptor free-list walk** (OS-9's system memory allocator keeps free/allocated region descriptors in fixed-size table slots) that fails to find/allocate a suitable block and falls through to `E$NORAM` once the counter in `d0` is exhausted (`a0` observed going to `0` right as the loop's last iteration completes, then the code loads a different value into `a0` and falls into the error tail) -- plausibly the kernel allocating the driver's **static storage** (`V_stat`, our `psect`'s `STATIC_SIZE=46` operand) or a similar per-attach kernel structure, and failing for a reason unrelated to actual free memory (the emulated CB030 board has never shown real memory pressure in any other test). Not yet identified: which allocation call this is, why it fails despite plenty of free RAM, or whether it's alignment/pool-selection related (system-state modules sometimes must allocate from a *specific* memory pool, not just anywhere) or a subtler off-by-one in our own descriptor/driver's declared sizes. **UPDATE, same session**: did exactly that comparison (extended `m68krt_watch_pc_callback` to hex-dump 32 bytes at `(a2)` whenever `pc==$0000ecb4`, the loop's top -- also uncommitted). Findings: (1) `"iniz dhf0"` actually issues a **real `I$Attach` (callcode `$80`, path `dhf0`, no leading slash)** first, which itself already fails with the *same* error the later `I$Open("/dhf0")` retry also hits -- both funnel through the identical shared error-return tail at `$edae`, so tracing either is equally valid, and the earlier `$84`-based traces above were not wasted. (2) Compared byte-for-byte against a working `"iniz h1"` (`Q9_ITRACE_CALLCODE=80 Q9_ITRACE_PATH=h1`, no leading slash there either): **the first 22 scanned 32-byte table entries (`$ffe2d0` step `$20` up to `$ffe570`) are byte-identical between the working and failing run**, with exactly one exception -- entry `$ffe2d0` (the very first/lowest one) is all-zero for `h1` but reads `00 00 00 03` for `dhf0`. The successful `h1` run's `I$Attach` returns success (`d1=0`) by using the *next* entry, `$ffe590` (one past everything captured) -- so this table is not a "no memory left" situation at all, real free capacity clearly exists right there. (3) The failing `dhf0` run instead **repeats the entire 22-entry scan three times** (66 `slot` log lines = 3×22) before giving up with `E$NORAM` -- never seen reaching `$ffe590` at all in any of the three passes. (4) Tested whether the "3" correlates with our own 3 explicit `load` commands (dhfmgr_68k/dhfdrv_68k/dhf0) by removing them entirely and copying all three modules straight into `/CMDS` (so a bcommon name-based disk search could in principle find them) and running bare `"iniz dhf0"` with zero prior `load`s: **theory falsely predicted this would help -- instead the error reverted all the way back to the original `E$MNF`**, proving conclusively that `I$Attach`'s internal module resolution does NOT fall back to any disk/directory search by itself (confirming the STATUS.md row above) and that pre-`load`ing all three modules by explicit path really is a hard prerequisite, not an optional convenience. So the "3" in that header-like table cell is very likely a genuine link-count/reference-count for our 3 resident custom modules, but it is a *symptom*, not something we can just avoid by not loading them. Root cause of why the 3-pass search never reaches the same free slot `h1`'s single pass does remains open. Next step for a future session: this now needs actual **disassembly of the real `ioman` module** (not more black-box register tracing) around the addresses this investigation already pinpointed precisely -- the loop itself (`$8bd6`-`$8c44`), its 3x-repeated outer bound-setting code (somewhere calling into `$ecb4`), and the shared error tail (`$edae`-`$ef6x`). `ioman`'s on-disk copy is at `/CMDS/BOOTOBJS/ioman` in the image (hex-dumpable directly via the host `os9 dump` ToolShed command, no running emulator needed) -- correlating disk-file offsets to the runtime addresses above just needs `ioman`'s known load base (readable from the module directory at runtime, e.g. via `mdir -e` or an itrace `a2`/`a3` register that happens to point at a resident module's header) once, then it's ordinary capstone disassembly (established project convention, see `[[q9-xcc-toolchain-milestone]]`), not more tracing. **UPDATE, same session: DONE, root cause found and fixed -- see next row.** |
| `E$NORAM` root cause + fix -- **`iniz dhf0` NOW SUCCEEDS** | ✅✅✅ Fixed, first successful real-kernel attach ever for this project | 2026-09-26. Did the disassembly (capstone, `CS_MODE_M68K_030`, against `kernel`/`ioman` extracted straight from `/CMDS/BOOTOBJS/{kernel,ioman}` on the image -- their file sizes matched the already-documented `m68krt.c` address-range comment byte-for-byte, `kernel` at load base `$7100`, `ioman` at `$e03c`, confirming those ranges as exact file-offset bases). Pinpointed the exact two instructions (`ioman+$eca-ish`): after successfully finding+claiming a free `Devicetbl`-style slot, the routine does `movea.l $44(a7),a0 / move.l $38(a0),d0 / beq.w ->E$NORAM`, i.e. it reads the **driver module's own header field `_mdata`** (`mod_driver`, offset `$38` -- independently confirmed by the header dump itself: `_mdinit` sits right after at `$3c`, matching the already-known-correct `$3C` offset from the "Real driver-call mechanism" row above) and treats **zero as fatal** -- undocumented anywhere, but apparently the real kernel requires every attached driver to declare *some* nonzero static storage. Root cause: **`STATIC_SIZE` (46, `sizeof(sysioStatic)`) was passed as `psect`'s 5th positional parameter, which `qr68k`/`ql68k` treat as `stksz` (a generic stack-size hint, ROF field `$32`/`32`) -- an entirely different field from the module header's actual `_mdata`, which `ql68k` computes purely from a module's own `vsect` reserved-data declarations (`q9-qr68k/README.md`'s ROF field table: `"20 4 statstorage reservierte Daten (ds im vsect)"`, confirmed by reading `ql68k`'s own source, `ql68.c:1177`: `_mdata = totalUninit + totalInit + totalRemote`).** Since `driver/dhfdrv_68k.a` had no `vsect` block at all, its compiled `_mdata` was genuinely `0` regardless of the `STATIC_SIZE` constant used elsewhere in the source -- a real, confirmed bug in how the module was built, not a kernel mystery. **Fix**: added `vsect / ds.b STATIC_SIZE / ends` inside the driver's `psect`; rebuilt (`_mdata` now `48`, 46 rounded to a 4-byte multiple), re-copied+`attr -e`'d into the image. **Result: `iniz dhf0` now returns with NO error** (verified via `test/expect/claude_dhf_attach.exp`, `Q9_TRAP_TRACE_ALL` confirms `I$Attach` callcode `$80` returns `d1=0`) -- the first time this project has ever gotten a real, successful `I$Attach` for `dhf0`. **New, separate finding**: the immediately-following `dir /dhf0` now reaches actual manager/driver code (progress!) but crashes the emulator with a PMMU Table-C fault jumping to PC≈0 (`IR=4e73`, an `rte` opcode at address 2) -- almost certainly the FileManager's still-stubbed `GetStt`/`ReadLn` entries (STATUS.md above: "those seven remain `E$UnkSvc` stubs... by design") being reached with a bad/uninitialized jump rather than cleanly returning `E$UnkSvc`; this is a **new, well-scoped follow-up task** (wire up `GetStt` at minimum, since `dir` needs it), not a re-opening of the attach investigation. **This whole investigation's technique (disassembling the real, unlabeled kernel with capstone once its module load base is known) is reusable for future black-box kernel questions** -- worth remembering alongside `[[q9-os-kernel-disassembly]]` even though that Ghidra project targets the *own* reimplemented kernel, not this real Microware one. |

| MgrCommon PMMU crash after `iniz dhf0` succeeded | ✅ Fixed -- real register-clobber bug, not a kernel mystery | 2026-09-26, same session as the `E$NORAM` fix above. With `iniz dhf0` finally succeeding, the very next real test (`dir /dhf0`, which reaches the manager's `Open` for the first time ever) crashed the emulator with a PMMU Table-C fault jumping to `PC≈2` (`IR=4e73`). Traced with `Q9_ITRACE_PATH=/dhf0` + a one-off memory dump added to `m68krt.c`'s per-instruction hook (both uncommitted-then-reverted, not part of the final diagnostic set kept). Root cause: `manager/dhfmgr_68k.a`'s `MgrCommon` saved the caller's `a1` (path descriptor) and `d3` (command number) across the `F$SRqMem` syscall via the **stack** (`move.l a1,-(a7)` / `move.l d3,-(a7)` ... `move.l (a7)+,d3` / `movea.l (a7)+,a0`) -- empirically, `F$SRqMem` leaves the stack POINTER exactly where it found it (verified: SP identical before/after the trap), but the **memory content** at the saved-`a1` slot came back as `0` instead of the real pointer, and `a3` (loaded from that now-`NULL` path descriptor's `pd_dev` field) ended up reading the CPU's reset-vector-table bytes as if they were a `Devicetbl`, producing a wild jump address for the subsequent driver call. Exact mechanism inside `F$SRqMem` not further chased (would need yet more kernel disassembly) -- treated as an empirical fact to work around, not something to rely on being fixed. **Fix**: rewrote `MgrCommon` to extract everything it needs from `a1` (path number into `d2`, `pd_dev` into `d5` as a raw 32-bit value) and stash the command number in `d4` **before** calling `F$SRqMem`, using only data registers (`d2/d4/d5`) that are never documented as syscall work registers (only `d0-d3/a0-a3` are) -- no stack save/restore around the syscall at all now. **Result**: `dir /dhf0` no longer crashes; a controlled follow-up test (`list /dhf0/hello.txt` against a real file placed at the descriptor's configured host basepath, `/tmp/q9dhf_claude_descriptor_test/hello.txt`) now fails **cleanly** with `list: can't read` instead of crashing or hanging -- consistent with the **already-documented** limitation that `ChgDir`/multi-component path elaboration are still `E$UnkSvc` stubs (see `MakDir/ChgDir`... row above), not a new bug: the kernel's own pathlist elaboration for a two-segment path (`/dhf0/hello.txt`) apparently needs `ChgDir` support our manager doesn't have yet, so it likely never even reaches our `Open` with the full string (no matching trap in the trace). **`dir /dhf0` itself now hangs (timeout) instead of crashing** when actually reached with just the bare `/dhf0` root path -- also consistent with the documented gap (real directory-entry listing needs `GetStt`/`ReadLn`, still stubs) rather than a new crash to chase. **Net effect of this session's four fixes**: attach + basic single-component file operations (`Create`/`Open`/`Read`/`Write`/`Close`/`Delete`, the six wired manager calls) should now be usable end-to-end for the first time; **directory listing and multi-component paths remain the next concrete milestone** (wire real `GetStt`/`ChgDir`, not stubs) for a future session. |

| `I$GetStt` (`SS_Ready`/`SS_Size`) -- was a stub | 🟡 Implemented, found+fixed a real host-hang bug along the way | 2026-09-26, follow-up session ("wire GetStt/ReadLn/ChgDir for real"). `I$GetStt` doesn't fit `MgrCommon`'s uniform payload (confirmed against "OS-9 System Calls" Ch. 2: `d0.w`=path number, `d1.w`=function code, rest depends on the code) -- gets its own routine, `Mgr_GetStt`, dispatching on `REG_D1(a5)`. Two function codes wired so far (the ones `dir`/`list` actually probe): `SS_Ready` ($01, "RBF devices always return carry clear, d1.l=1" per the manual -- answered locally, no driver round-trip) and `SS_Size` ($02, `d2.l`=file size out -- goes through the same `F$SRqMem`/`CallDrv`/`F$SRtMem` dance as `MgrCommon`, same register-preservation fix applied). Everything else still falls through to the existing `E$UnkSvc` stub path (renamed `MgrUnkSvc`, shared by `Seek`/`ReadLn`/`WritLn`/`SetStt`/`MakDir`/`ChgDir`) -- no regression, just not yet implemented. **Found+fixed a real bug while wiring `SS_Size`**: the emulator's existing `DHF_CMD_GETSTT` handler (`dhf_emu_device.c`, already implemented before this session) looked up the target file by **pathname** (`dhf_host_fs_getstat(path, ...)`), but the manager -- like `MgrCommon` for Read/Write/Close -- has no pathname for an already-open path, only the OS-9 path number; it left `SH_A0` uninitialized (garbage). The device unconditionally resolves `SH_A0` into a `path` pointer before its command switch (harmless, lazy pointer arithmetic) but the ORIGINAL `getstat` call would then have tried to actually read/confine a bogus string -- this didn't crash the 68k CPU, it made the **host emulator process** effectively stop responding (external symptom: the whole test hung past any reasonable timeout, `Q9_TRAP_TRACE_ALL` tracing showed the trace file simply stop growing with no further syscalls -- confirmed NOT a 68k-side infinite loop, since the identical test without `SS_Size` wired completes in well under a second). **Fix**: added `dhf_host_fs_getstat_at(fs, handle, ...)` (new function, `dhf_host_fs.c`/`.h`) using `fstat()`/`stat()` on the handle's already-stored `fd`/`path` from the `handles[]` table -- exactly the same handle-indexed pattern `open_at`/`create_at` already established -- and switched `dhf_emu_device.c`'s `DHF_CMD_GETSTT` case to call it with `d0` (the OS-9 path number) instead of the unresolved pathname. Confirmed fix: `SS_Size` no longer hangs. **Not yet fixed / next step**: `list /dhf0/hello.txt` (a real file placed at the descriptor's basepath) now fails quickly and cleanly with `list: can't read` again (no hang, no crash) -- root cause not yet found for this specific error; `Q9_TRAP_TRACE_ALL` tracing itself is impractical for chasing it further because turning on the per-instruction `classify` mechanism this session's diagnostics rely on slows emulation down by roughly two-three orders of magnitude for whatever `list` does after loading (confirmed: identical test hits the 120s–300s expect timeout ONLY when `Q9_TRAP_TRACE_ALL` is set, never otherwise) -- the next session should either write a minimal, purpose-built 68k test program that calls `I$Open`/`I$Read`/`I$Close` directly against `/dhf0/hello.txt` with its own diagnostic output (sidesteps both the `list` utility's opaque internals and the tracing-overhead problem), or try wiring `I$ChgDir` next (still unverified register convention, but now documented in `/tmp/os9_tech_ref.txt`-style extracts from the local "OS-9 v2.4 Technical Reference Manual" PDF, `pdftotext -layout` -- worth remembering as a source for any further I$-call register conventions, see `SEE ALSO` cross-references in that manual for `I$MakDir`/`I$ChgDir` on pages "2-3"/"2-15"). |

| `test/dhftest_68k.a` -- minimal, purpose-built test program | ✅ Written, works, already paid for itself twice over | 2026-09-26, follow-up session. A tiny standalone OS-9 `Prgrm` module (not FileManager/driver/descriptor) that calls `I$Open`/`I$Read`/`I$Close` directly against `/dhf0/hello.txt` and prints each step's success/error (hex code) to stdout (path 1, already open by OS-9 convention at program start) -- built as a controlled replacement for the opaque `list` utility, whose failures couldn't be diagnosed further (see previous row). Two real, general (non-DHF-specific) bugs found and fixed while getting it to even run correctly, both worth remembering for any future hand-written 68k assembly in this project: **(1) qr68k treats `dc.b 'multi-char string'` (single quotes) as a NUMERIC multi-character constant, truncated to the destination size** -- for a `.b` directive that keeps only the LAST character (confirmed against `q9-qr68k/README.md`'s own documented behavior, "einfache Anführungszeichen sind eine ZAHL"; `dc.b "..."`, double quotes, is the real string form -- the existing `descriptor/dhf0.a` already used double quotes correctly, this file didn't). **(2) OS-9 modules are position-independent -- absolute label references (`lea LABEL,a0`, `movea.l #LABEL,a0`, or a bare `LABEL` as a MOVE memory operand) silently bake in the assembly-time offset as if the module were loaded at address 0**, causing a PMMU fault the moment the module runs anywhere else; the fix is PC-relative addressing (`lea LABEL(pc),a0`) for every label reference, and -- since PC-relative is a "control", not "alterable", addressing mode on real 68k hardware and cannot be a MOVE destination -- routing every read/write of a mutable variable through an address register loaded via `lea VAR(pc),An` first. (Also made the module explicitly non-`ReEnt`/non-`SupStat`, sidestepping the question of where a shared/reentrant program's own writable data area pointer comes from -- out of scope for a disposable test tool.) `dhftest`, run against a real RBF file (`/dd/README` instead of `/dhf0/hello.txt`, `test/dhftest_rbf_68k.a`, a copy with just the target path changed) completes cleanly (`OPEN: FEHLER, Code: $00D6`, no hang) -- confirming the test program itself is correct and isolating the next finding below as DHF-specific. |
| `Q9_TRAP_TRACE_ALL`'s `classify` mechanism -- was unusable for anything involving a forked child process | ✅ Fixed (tracer bug, not a DHF bug, but blocked diagnosing one) | 2026-09-26. Running `dhftest` (or, in hindsight, the earlier `list` investigation two rows up) under `Q9_TRAP_TRACE_ALL` made the emulator apparently hang for minutes past any reasonable timeout, while the identical run without tracing finished in under a second -- traced back to `classify_active`, a single **global** (not per-process) flag that `m68krt.c` opens on every trap0 once `Q9_TRAP_TRACE_ALL` is set, closed only when a specific return PC is reached by the per-instruction hook; the code's own header comment already flagged the assumption this relies on ("kein rekursiver trap#0") -- exactly what breaks the moment a **second process** (the forked child, e.g. `dhftest` or `list` itself) starts trapping while the first trap's classify window is still open, since the flag has no notion of which process is which and effectively never closes again once two processes' trap0s interleave. Fix: gated `classify_active`'s activation behind a new, independent env var (`Q9_CLASSIFY`), decoupled from `Q9_TRAP_TRACE_ALL` -- `Q9_ITRACE_PATH`/`Q9_ITRACE_CALLCODE` (the actually-useful targeted tool for cases like this) never needed `classify` and are unaffected, now usable again for anything that forks. **This fix is what made the row below possible to find at all.** |
| DHF `I$Read` from a forked process -- real, reproducible, KERNEL-internal infinite loop | 🟡 Found and precisely located via disassembly, root cause (which of our writes corrupts kernel state) NOT yet found | 2026-09-26, using the now-fixed tracer + `dhftest`. `iniz dhf0` and `I$Open("/dhf0/hello.txt")` both succeed (`OPEN: ok` printed) even from a genuinely forked child process -- but the program then hangs indefinitely (confirmed for real, not a tracing artifact: identical behavior with tracing off, using a >60s timeout). `Q9_ITRACE_PATH=/dhf0/hello.txt Q9_ITRACE_CALLCODE=84` (arms on the `I$Open` trap, then just keeps counting instructions -- covers whatever runs right after, i.e. `I$Read`) shows execution stuck bouncing between exactly two PCs (`$896e`/`$8982`-ish range) inside the **kernel** module (address range confirmed via the same `kernel.mod`-file-offset technique as the `E$NORAM` disassembly two rows up) forever, `sp`/`a3` constant, `a1` oscillating between `$00000000` and `$00ebe26a`. Disassembly of that address range (capstone, same technique) identifies it precisely: a **circular linked-list walk with interrupts disabled** (`ori.w #$700,sr` at entry, `move.w d4,sr` to restore on exit -- classic atomic-queue-manipulation pattern), checking/updating a counter-like field at each node's offset `+$2e0` and following a "next" pointer at each node's offset `+$30`, terminating only when the walk returns to its own starting node (`a3`, constant throughout, never reached). This is very likely OS-9's own per-tick event/sleep-queue or process-ring processing (disabling interrupts for atomic list manipulation matches that role) -- **something in our own code corrupts one node's `+$30` "next" pointer to `0`**, turning what should be a finite ring into an infinite 2-node bounce (address `0` behaves as a node whose own `+$30` field happens to read back `$00ebe26a`, and vice versa) that then hangs on literally the next clock tick, for the whole system, not just our process. Prime suspects, not yet checked one at a time: `MgrCommon`'s `F$SRqMem`/`F$SRtMem` pair (already known to interact with caller context in surprising ways, see the register-safety row above) now exercised from a **different process's** memory pool state than the shell-builtin-command tests that validated it earlier; or `Mgr_GetStt`'s parallel `F$SRqMem`/`F$SRtMem` pair (untested from a forked process at all so far, only via `iniz`/`list`, both shell-builtin contexts). Next step for a future session: bisect by testing `I$Read` alone (skip `I$GetStt`/`SS_Size`, not currently called by `dhftest` anyway since it only does `Open`/`Read`/`Close`) against a **freshly rebooted image** each time to rule out cross-run pool corruption accumulating across this same long-lived boot, and/or use `Q9_DUMP_MEM` (already built into `m68krt.c`) on the corrupted node's neighborhood once its address is known, to see the exact moment its `+$30` field becomes `0`. |

**UPDATE, same session -- bisection results (see `test/dhftest_bisect_68k.a`/`_rbf_68k.a`, both new, both committed):**
1. **A variant that does ONLY `I$Open`+`I$Close` (no `I$Read` at all), then `F$Sleep`s 100 ticks five times in a row, printing after each** -- **also hangs**, and on one run hung even before finishing the FIRST print (before `I$Open` even returned), on another run only after `I$Open` succeeded. Non-deterministic exact hang point, always within/shortly after touching DHF. Rules out `I$Read`'s MMIO-buffer write as the (sole) culprit -- whatever corrupts kernel state, `MgrCommon`'s shared `F$SRqMem`/`CallDrv`/`F$SRtMem` sequence (used identically by `Open`) is already enough.
2. **The identical bisection program retargeted at a real RBF file (`/dd/README`) instead of `/dhf0/...`) completes cleanly every time**, including all 5 real one-second sleeps (so 5 genuine timer ticks fire and get processed without incident) -- confirms the corruption is DHF-specific, not a pre-existing/general kernel or scheduler bug this project just happened to be the first to trip over.
3. **Raising the test program's declared stack (`M$Stack`) from 1024 to 16384 bytes made no difference** -- rules out "F$SRqMem writes below the caller's declared stack into adjacent memory because the stack is too small" as the mechanism, even though that remains a plausible-sounding theory in the abstract.
4. **Plain forking is not the trigger by itself**: attaching `dhf0` and then running the real, already-installed `date` utility (itself a fork+exec, like `dhftest`) eight times in a row via the shell -- with NO further DHF-specific activity -- never hangs. The trigger needs a forked process that itself calls into our manager (`I$Open`/`I$Read`/etc. on the `dhf0` path), not just "any external program while dhf0 is attached."
5. A second itrace capture of a *different* hang instance (same technique, `Q9_ITRACE_PATH=/dhf0/hello.txt`) landed in a *different* low address range (`$070c`-`$073e2`, below `kernel`'s own `$7100` load base -- likely boot-ROM-resident low-level code, not yet correlated to a disassembly) with **`a6` (expected to be the constant system-global pointer, `$00004a00` in every other trace this whole investigation) instead oscillating between `$26044a06` and `$00000000`/`$00004a00`** -- a second, independent symptom of the same underlying corruption, seen from a different angle. Not yet disassembled/explained (would need dumping+disassembling whatever ROM/low module occupies that range, analogous to the `kernel.mod`/`ioman.mod` extraction technique used for the two rows above, but for boot-resident code that isn't necessarily a discrete file in `/CMDS/BOOTOBJS`).

**Assessment**: this is now clearly a real, DHF-specific, timing-dependent (exact hang location and even whether it happens before or after `I$Open` returns varies between otherwise-identical runs) kernel-memory-corruption bug, most likely triggered somewhere in `MgrCommon`'s shared `F$SRqMem`/`CallDrv`/`F$SRtMem` sequence or the driver's MMIO copy-loop, but reached only when a genuinely forked process (not the shell's own built-in-command context) is the caller -- every earlier test of this exact code path (`iniz`, `dir`, `list` failing to even reach it) ran in the shell's own process, never actually exercising this combination before `dhftest` existed. Root cause NOT yet found; likely needs either instrumenting `F$SRqMem`/`F$SRtMem`'s *own* internal kernel code (disassemble+trace, same technique as the `E$NORAM` row, applied to whatever the real callcode-`$28`/`$29` handlers do) to see exactly what memory they touch relative to a forked child's context specifically, or a differently-shaped bisection (e.g. a variant that calls `F$SRqMem`/`F$SRtMem` directly, with no DHF `I$Open` at all, to check whether the memory allocator itself is already enough to corrupt something when called from a fresh child process, independent of anything DHF-specific in `CallDrv`).

**UPDATE, same session -- root cause found and fixed! `MgrCommon` no longer hangs from a forked process.**

Continued bisecting (all via temporary, throwaway edits to `manager/dhfmgr_68k.a`, reverted between steps -- the final, real fix is described below and is what's actually committed):
1. **A version of `MgrCommon` reduced to literally nothing but `F$SRqMem` immediately followed by `F$SRtMem`** (no `PD_PD`/`PD_DEV` read, no payload writes, no `CallDrv`) -- run via the real `I$Open` dispatch path (nested inside a forked process's syscall, exactly the scenario that hangs) -- **completed cleanly, no hang.**
2. **Adding back only the `PD_PD(a1)`/`PD_DEV(a1)` reads** (still no payload writes, still no `CallDrv`) -- **still clean, no hang.**
3. **Adding back the six payload writes into the `F$SRqMem`-allocated block** (`SH_A0`/`SH_A1`/`SH_D0`/`SH_D1`/`SH_D2`/`SH_COMMAND`), still with `CallDrv` skipped -- **hangs again**, confirming the payload writes are what triggers it, not `CallDrv`/the driver/the emulator device (already independently ruled out in the previous update).
4. **Replacing the payload SOURCE values with hardcoded constants** (`#1` instead of `REG_A0(a5)`/`REG_D0(a5)`/`REG_D1(a5)`) -- **made it categorically worse: a full system reboot** (the actual OS-9 boot sequence restarts mid-log), not just a hang, proving the exact VALUES written change the failure mode/severity.

**Conclusion**: `F$SRqMem`, called from within a FileManager's own `I$Open` dispatch (i.e. nested inside an already-in-progress trap, specifically when the outer caller is a freshly forked process -- never reproduced from the shell's own long-lived process, and never reproduced by exercising `F$SRqMem`/`F$SRtMem` directly from ordinary user code, however many cycles), **returns a block of memory that is already in use by some other live kernel structure**. Writing into it -- with ANY values, our own real payload or arbitrary constants -- directly corrupts whatever that structure actually is (most likely the same process/event ring found earlier, given the matching symptom), and the exact symptom (silent hang next time it's walked vs. an immediate hard crash) depends on which bytes of that structure end up overwritten and with what. Whether this is a genuine bug in the underlying Microware kernel port (plausible -- FileManagers calling `F$SRqMem` for scratch buffers is normal, documented practice, so this would be a real, narrow edge case in it) or some subtler misuse on our part that a full kernel source audit would reveal was **not** resolved -- treated as an empirical fact to design around, not a mystery left unexplained for its own sake.

**The fix, actually committed**: stopped calling `F$SRqMem`/`F$SRtMem` from `MgrCommon`/`Mgr_GetStt` entirely. Replaced the per-call dynamic allocation with `CmdBlk`, a **static 32-byte buffer embedded directly in the manager's own `psect`** (i.e. baked into the module's code image, not kernel-allocated). Two wrinkles worth remembering:
- **A `vsect` doesn't work here**: `qr68k` refused `lea CmdBlk(pc),a1` when `CmdBlk` lived in a separate `vsect` ("PC-Bezug auf einen anderen Abschnitt" -- PC-relative addressing needs a fixed, assembly-time-known displacement, which only exists within the same section). Unlike a **driver**, which gets a kernel-populated static-storage pointer handed to it in `(a2)` at every entry (`mod_driver`'s `_mdata`/`V_stat` mechanism, already exploited for `driver/dhfdrv_68k.a`'s own `vsect` fix earlier this session), a **FileManager module has no such mechanism at all** (`mod_fman` in `module.h` is just `{common header, _mexec, _mexcpt}` -- no data-size/data-pointer fields whatsoever). So `CmdBlk` had to move directly into the `psect` instead (same trick already used for the disposable `dhftest` programs' own non-reentrant variables) -- and this module IS `ReEnt` (ordinarily meaning "this code page is shared read-only across every user"), yet writing into `psect`-embedded `CmdBlk` at runtime works fine in practice -- this board apparently does not enforce write-protection on a `ReEnt` module's code section (consistent with many OS-9/68k systems never having fully wired up memory protection for this), which is convenient here but not something to rely on being true elsewhere.
- **Known, accepted limitation**: `CmdBlk` is now a single, module-wide **shared** buffer, not one per path/process. Two processes calling into the DHF manager at the exact same instant would race on it. Fine for the current single-user testing this project is at; flagged in the source as needing a real mutex (`F$Event`/`Ev$Wait`, matching the original architecture doc's own concurrency section, `[[q9-hostfs-manager-idee]]`) before any multi-process production use.

**Result, verified**: the full `dhftest` (`I$Open`+`I$Read`+`I$Close`, no bisection variant, the real thing) run from a genuinely forked process now completes end-to-end with **no hang and no crash** for the first time ever: `OPEN: ok`, then `I$Read` itself now fails with a *new, ordinary, single-call* error (`E$ILLINS`, `$68`, "illegal instruction TRAP 4 occurred" -- a real CPU-exception-turned-syscall-error, not a hang) and `CLOSE: ok`. This is real, forward progress: a mundane, single-call bug to chase next (likely somewhere in the driver's `Read`-specific MMIO handling or the emulator's `DHF_CMD_READ` case, given `Open`/`Close`/`GetStt` all work through the identical shared driver body), not a system-wide corruption issue anymore.

**UPDATE, same session -- `E$ILLINS` on `I$Read` further bisected (all via temporary,
reverted edits -- nothing below is committed except this note itself):**

1. **Confirmed fully deterministic**: ran the unmodified `dhftest` three times in a row,
   identical result every time (`OPEN: ok`, `READ: FEHLER, Code: $0068`, `CLOSE: ok`) -- not
   a residual instance of the timing-dependent corruption fixed above; a real, reproducible
   bug.
2. **Decoded the exception path precisely** (disassembly, `kernel.mod`, addresses `$78be`-
   `$78f2`): this is the kernel's generic **hardware-exception-to-errno converter** -- checks
   a saved "intercept" context (`a4+$140`/`a4+$144`, the same slots the generic trap-dispatch
   preamble sets up for every syscall) and computes `errno = (vector_number*4)>>2 + $64`;
   vector 4 (illegal instruction) -> exactly `$68`. This is **not** a DHF-specific error code
   invented anywhere in our own code -- a real 68k CPU exception (illegal instruction) fired
   somewhere during this call, and the kernel's own safety net turned it into a returned
   error instead of crashing the system. Confirms this is a genuine bug (in our code or the
   kernel's `I$Read` dispatch), not a red herring.
3. **Temporarily made `MgrCommon` skip `CallDrv` entirely** (simulate success unconditionally,
   never touch the driver/emulator at all) for `Open`/`Read`/`Close` alike -- `I$Read` **still**
   fails with the identical `$68`, proving the bug is NOT in `CallDrv`, the driver's shared
   MMIO body, or the emulator's `DHF_CMD_READ` handler (all three already independently
   verified correct via `test-dhf-driver` and by working for `Open`). Side observation: with
   `CallDrv` skipped, `I$Close` **also** started failing with `$68` afterward (it did not fail
   in earlier runs where `CallDrv` ran for real) -- most likely a knock-on effect of `I$Read`'s
   own exception leaving some kernel-internal state (process nesting depth, interrupt mask)
   not fully restored, not an independent `I$Close` bug; not pursued further since `I$Read`
   failing is the actual, primary issue.
4. **Verified the compiled module itself is correct byte-for-byte**: dumped `dhfmgr_68k.mod`'s
   header (`M$Exec`) and 13-entry jump table directly (no live emulator needed, just parsing
   the `.mod` file) and disassembled the six wired thunks + `MgrCommon` with capstone --
   `Mgr_Read`'s table entry (`$62`) lands exactly where the six sequential 6-byte thunks
   (`Create`@`$56`, `Open`@`$5c`, `Read`@`$62`, `Write`@`$68`, `Close`@`$6e`, `Delete`@`$74`)
   place it, and every thunk + `MgrCommon` disassembles to exactly the expected instructions
   matching the source 1:1. **Rules out a `qr68k`/`ql68k` jump-table-generation bug** (a real
   possibility given this session already found two other real toolchain quirks) -- the
   compiled code is exactly what the source says.

**Net conclusion so far**: the illegal instruction fires somewhere during `I$Read`'s
processing that is independent of anything our own manager code does with the call's
payload (proven by the no-op `CallDrv` test) and independent of the compiled module being
correct (proven by the byte-level disassembly check) -- pointing at IOMan's own `I$Read`-
specific dispatch/pre- or post-processing having a bug or edge case when calling a third-
party (non-RBF/SCF/PIPE) FileManager from a forked process, distinct from (but same general
flavor as) the `F$SRqMem` bug fixed above. Not yet located precisely -- would need the same
kind of kernel disassembly work that found the `F$SRqMem`/`E$NORAM` issues, this time
targeting IOMan's `I$Read`-specific code path (likely reachable the same way: arm
`Q9_ITRACE_PATH` on the *`I$Open`* call as before since `I$Read` isn't itself path-logged,
then look for where PC diverges between an `I$Open` trace and this same run's continuation
into the `I$Read` trap -- attempted this session but got lost in interleaved multi-process
trace noise before finding the exact divergence point; a cleaner approach for next time might
be a **minimal C-level bisection test in `Q9-Flux-68k/test/`** that calls IOMan's I$Read path
against a trivial always-resident FileManager (if one can be improvised) to isolate IOMan's
own behavior from ours entirely, or extending `m68krt.c`'s existing "path"-logging
(`callcode == 0x80/0x83/0x84/0x86/0x87`) to also cover `0x89` (`I$Read`) so `Q9_ITRACE_PATH`
can arm on it directly instead of needing the `I$Open`-then-continue workaround).

**UPDATE, follow-up session, 2026-09-26 -- root cause FOUND AND FIXED. `dhftest` now
completes `OPEN: ok` / `READ: ok` (real file content printed) / `CLOSE: ok` end-to-end for
the first time ever. The "IOMan bug from a forked process" theory above was WRONG -- this
was a bug in our own `manager/dhfmgr_68k.a`, hiding in plain sight.**

1. **Extended `Q9-Flux-68k/src/kernel/m68krt.c`'s `Q9_ITRACE_PATH` mechanism** exactly as
   suggested above: `I$Read` ($89) carries the OS-9 path NUMBER in `d0.w`, not a pathname in
   `a0` (unlike `I$Open`/`I$Attach`), so it can't path-match the same way. Fix: when the
   `Q9_ITRACE_PATH`-watched `I$Open` returns successfully, its returned path number is now
   remembered (`g_itrace_target_pathnum`); any later `I$Read` whose `d0.w` matches that
   number arms the instruction trace directly, no more "arm on Open, keep tracing and hope
   to catch Read in the same window" workaround. New statics `g_itrace_open_pending` /
   `g_itrace_target_pathnum_valid` / `g_itrace_target_pathnum`, committed in `Q9-Flux`.
2. **Traced a real `dhftest` run this way** and found the CPU hook fires BEFORE each
   instruction executes (confirmed from Musashi's own `m68kcpu.c`: `m68ki_instr_hook(REG_PC)`
   runs, then `m68ki_instruction_jump_table[REG_IR]()` dispatches) -- so the LAST logged PC
   before the trace jumps into the kernel's known exception-to-errno converter (`$78be`-
   `$78f2`, identified in an earlier session) is the actual faulting instruction's address.
   That address was `$00f20e1e` -- and disassembling the just-extracted `dhfmgr_68k.mod`
   (`os9 copy image,/CMDS/dhf/dhfmgr_68k /tmp/...`, capstone) showed the CPU landing exactly
   TWO BYTES into the middle of a 4-byte `lea` instruction, decoding whatever garbage bytes
   happen to follow as a bogus opcode.
3. **Reconstructed IOMan's own generic FileManager-dispatch code** (disassembled directly
   from `ioman.mod`, address `$0000f55c`-`$0000f59a`, real 68k assembly, not guessed): reads
   `PD_DEV(a1)` -> `Devicetbl*`, then `V_FMGR` (offset `$c`, matching `sysio.h`) -> the
   FileManager module's own base address, then adds the module's own `_mexec` header field
   (a stored byte offset, itself read from module-base+`$30`) to get the jump table's base
   address, then reads a **16-bit, SIGNED, TABLE-BASE-RELATIVE** word at
   `table_base + (callcode-$83)*2` and adds it to `table_base` to get the final entry point.
   This is the standard, and only sane, OS-9 convention (a table that doesn't care where in
   memory the module got loaded, exactly the point of `_mexec`), and it matches what the
   *real* Microware `rbf`/`scf` modules must do too (same generic IOMan code runs for every
   FileManager type).
4. **Root cause**: `manager/dhfmgr_68k.a`'s own `DhfMgrEnt` table was written as bare
   `dc.w Mgr_Create` / `dc.w Mgr_Open` / ... for all 13 entries -- which `qr68k`/`ql68k`
   assemble as each label's plain, ABSOLUTE offset from the MODULE'S OWN base (i.e., exactly
   what `os9 dump`/capstone show when reading the label's address directly), **not** relative
   to the table's own position. Since `_mexec` (the table's own module-relative offset) is
   `$3c` in this build, EVERY SINGLE jump-table entry pointed exactly `$3c` bytes too far into
   the module -- for `Create`/`Open` this coincidentally still landed on a valid-but-wrong
   instruction *inside* `MgrCommon` (skipping its first two setup instructions, silently using
   garbage/stale `d2` as the path number -- explains why `iniz`/`dir`/earlier ad-hoc tests
   never caught this: whatever they exercised happened to still "work" by accident), but for
   `Read` it landed mid-instruction, producing the `E$ILLINS` illegal-opcode crash that took
   this whole investigation to explain. **This is a 4th real, confirmed toolchain/authoring
   gotcha for this project's hand-written 68k modules** (after the `-n=`/`-gu=` linker-flag
   issues, the `dc.b` single-vs-double-quote string bug, and the `psect`-vs-`vsect`
   static-size bug): **any module's own jump table written as `dc.w Label` must instead be
   written `dc.w Label-TableName`** (explicit table-relative arithmetic), matching the
   universal Microware/OS-9 module convention for `_mexec`-style tables -- `qr68k`/`ql68k`
   assemble the arithmetic correctly once written this way, they just don't do it implicitly
   for a bare label reference (unlike, say, a relative branch instruction).
5. **Fix applied**: rewrote all 13 `DhfMgrEnt` entries as `dc.w LabelName-DhfMgrEnt`,
   rebuilt (`qr68k`+`ql68k -n=dhfmgr -gu=0.0`), verified the new table's computed targets
   against every real label's actual file offset (found independently via raw byte-pattern
   search for each thunk's `moveq` opcode) -- all 13 now match exactly, including the six
   wired thunks and the shared `MgrUnkSvc`-branch stub target. Redeployed to
   `OS9SYS_Claude.hda`'s `/CMDS/dhf/dhfmgr_68k` (`os9 del` + `os9 copy` + `os9 attr -e`).
6. **Result, verified end-to-end for the first time ever**: `dhftest` against
   `/dhf0/hello.txt` now prints `OPEN: ok`, `READ: ok, Inhalt folgt: <real file content>`,
   `CLOSE: ok` -- no crash, no hang, no wrong error. (One test-setup wrinkle found along the
   way, not a bug: `I$Open` receives the FULL pathname INCLUDING the device-name component
   -- `/dhf0/hello.txt`, not just `hello.txt` -- since our manager does no path parsing of
   its own and forwards `a0` verbatim to the driver/host-fs, the *test file* had to actually
   live at `<basepath>/dhf0/hello.txt` on the host, not `<basepath>/hello.txt`, for this
   particular test program's hardcoded path string. Worth revisiting for real usage: either
   document that DHF host paths always mirror the full OS-9 pathname including the device
   name, or have the driver/manager strip the leading device-name component -- not decided
   yet, flagged for later, not a defect in the fix above.)
7. **Not yet re-verified after this fix**: `Mgr_GetStt`'s `SS_Size` path (used by `dir`/
   `list`) -- it was validated against an OLDER build of this file (before the 2026-09-27
   thunk redesign), and its own table entry was equally affected by the same `_mexec`-relative
   bug (confirmed: the broken table's `GetStt` entry also pointed `$3c` bytes past its real
   label). Now fixed by the same table rewrite, but a fresh `dir /dhf0`/`list /dhf0/hello.txt`
   run has not been repeated this session to confirm.

> Legend: ❌ Not started, 🟡 Partially/rudimentary, ✅ Done

## Architecture note

This directory now contains two coexisting layers that must not be confused:

- **Kernel layer** (`manager/dhf_manager.c`, `manager/dhf.h`, `include/dhf_shared_api.h`, `docs/PROTOCOL.md`, plus the new `manager/dhfmgr_68k.a` skeleton): the real OS-9 68k target code. `dhf_manager.c`'s existing `_os9_f_viread()` call is a **fictional placeholder that was never real** (see "Real driver-call mechanism" row in the status table above for the actual, documented+verified mechanism: `Devicetbl`/`mod_driver` header fields, not a trap or library call). Reconciling `dhf_manager.c`'s C logic with the real register-based module ABI (which needs an assembly entry stub, hence `dhfmgr_68k.a`) is the next concrete step.
- **Host-simulator layer** (`host_simulator/`, `examples/`, `manager/dhf_manager_hostsim.c`, `include/dhf_shared_hostsim.h`): a native-compiled test harness that exercises the same driver logic (`driver/dhfdrv-68k.c`, `descriptor/`) via direct function calls, without requiring the real 68k target or emulator. Useful for fast iteration on filesystem logic.

The driver (`driver/dhfdrv-68k.c`) and descriptor (`descriptor/`) are shared between both layers and are target-agnostic C.

**Third layer, added 2026-09-25** (`Q9-Flux/Q9-Flux-68k/src/devices/dhf/`, separate repo): the Musashi-emulator-side device, reachable from real/emulated 68k code via ordinary MMIO reads/writes at `$FFFF4000`. Its protocol and internals actually come from the **`Q9-DHFDRV-68k`** sibling project (see below), not from this directory's `driver/dhfdrv-68k.c` -- nothing in *this* repo talks to it yet.

**Sibling project, `Q9-OS/Q9-DHFDRV-68k`** (committed 2026-09-24, independently of this directory): a second, more complete DHF implementation with its own driver (`driver/dhfdrv.c`, A0/A1/D0-D2 zero-copy register protocol) and its own emulator-side device (`emulator/dhf_emu_device.c`) plus an optional TCP-forwarding backend for a remote host. It has **no manager component** -- it was presumably meant to plug into (or replace) this directory's driver layer underneath `manager/dhf_manager.c`, but that reconciliation was never done. The 2026-09-25 Musashi device above is built from `Q9-DHFDRV-68k`'s code, not this directory's.

## 2026-09-26, Nachtrag: zwei Nutzervorgaben nachgezogen (Namenskonvention + Aufrufweg)

1. **Laufwerksname `dhf0` -> `d0`, Datei `descriptor/dhf0.a` -> `descriptor/d0_dhf.a`**
   (Nutzervorgabe: soll wie ein gewoehnliches Massenspeichergeraet aussehen). `nam`/`psect`
   im Deskriptor entsprechend auf `d0` geaendert, `test/dhftest_68k.a`s Testpfad auf
   `/d0/hello.txt`. Neu gebaut+deployed (`os9 del`+`os9 copy`+`os9 attr -e` fuer `d0` und
   `dhftest` in `/CMDS/dhf/` von `OS9SYS_Claude.hda`), End-zu-Ende-Test (`iniz d0` +
   `dhftest`) laeuft weiterhin komplett durch. Die historischen Bisektions-Testvarianten
   (`dhftest_bisect_*`, `dhftest_rbf_*`, `dhftest_srqmem_*`) und Alt-Kommentare in
   `driver/dhfdrv_68k.a`/`manager/dhfmgr_68k.a`, die noch woertlich "dhf0"/"iniz dhf0"
   zitieren, bleiben bewusst unveraendert (Protokoll vergangener Sitzungen, nicht der
   aktuelle Name).
2. **`manager/dhfmgr_68k.a`s `CallDrv` rief den Treiber ueber dessen `D_READ`-Sprungtabellen-
   eintrag auf** (Kommentar im Quelltext nannte das "stellvertretend fuer alle", eine
   eigenmaechtige Wahl ohne Rueckfrage). Nutzervorgabe: der Manager soll IMMER ueber
   `D_WRIT` gehen -- der Kommandoblock (Managerfunktion-Nummer in `SH_COMMAND` + bis zu 5
   Parameter in `SH_A0/SH_A1/SH_D0/SH_D1/SH_D2`) wird dem Treiber grundsaetzlich als
   "Write" uebergeben, unabhaengig davon, ob die urspruengliche I$-Operation selbst ein
   Read/Write/Init/... war. Da `driver/dhfdrv_68k.a`s `Read:`/`Write:`/`GetStat:`/`SetStat:`
   ohnehin alle auf denselben Code zeigen, aendert das aktuell nichts am Verhalten -- macht
   aber den Aufrufweg begrifflich korrekt und zukunftssicher (falls die vier Treiber-
   Sprungziele je auseinanderlaufen sollten). `D_READ` in `dc.w D_WRIT equ 4` umbenannt
   (`sysio.h`: `D_INIT=0,D_READ=2,D_WRIT=4,D_GSTA=6,D_PSTA=8,D_TERM=10,D_TRAP=12`).
   Neu gebaut+deployed, derselbe End-zu-Ende-Test (`OPEN`/`READ`/`CLOSE`: ok) bestaetigt
   weiterhin fehlerfrei.

## 2026-09-26, Nachtrag: MakDir/ChgDir/Seek/ReadLn/WritLn verdrahtet -- `list` laeuft jetzt

Nutzerfrage: "fehlen die Funktionen in unserer Kette?" -- Antwort: NEIN, weder Treiber
(generischer Rumpf, nimmt jedes Kommandobyte) noch Simulator (`dhf_emu_device.c`, alle 20
`DHF_CMD_*`-Faelle inkl. Seek/ReadLn/WritLn/SetStt/ChgDir/MkDir/RmDir/Rename/OpenDir/
ReadDir bereits implementiert). Die Luecke war ausschliesslich im Manager (6 von 13 echten
I$-Aufrufen noch `E$UnkSvc`-Stubs).

1. **MakDir/ChgDir/ReadLn/WritLn** -- gegen die echten Registertabellen aus "OS-9 System
   Calls" Kap. 2 (`OS-9 v2.4 Technical Reference Manual`, lokal unter
   `/Volumes/SSD1TB/Documents/`) geprueft: alle vier passen Register-fuer-Register exakt auf
   `MgrCommon`s uniformes Schema (`I$MakDir`: `d1.w`=Rechte->`SH_D1`, `a0`=Pfadname-> `SH_A0`;
   `I$ChgDir`: nur `a0`=Pfadname, vom Simulator-Handler ohnehin ignoriert; `I$ReadLn`/
   `I$WritLn`: Register-fuer-Register identisch zu `I$Read`/`I$Write`). Vier neue 6-Byte-
   Sprungbretter, genau wie die sechs bestehenden.
2. **Seek passt NICHT auf das uniforme Schema** -- eigene Routine `Mgr_Seek`. Grund: das
   dritte Nutzlastfeld (`SH_D2`) traegt beim Simulator-Kommando `DHF_CMD_SEEK` ein "whence"
   (0=SEEK_SET/1=SEEK_CUR/2=SEEK_END), `MgrCommon` wuerde dort aber `REG_D0(a5)` (=Pfad-
   nummer) hineinschreiben -- bei kleinen Pfadnummern (1 oder 2) waere das versehentlich
   SEEK_CUR/SEEK_END statt SEEK_SET. `I$Seek` ist laut Handbuch aber IMMER eine absolute
   Positionierung -- `Mgr_Seek` setzt `SH_D2` deshalb fest auf `DHF_SEEK_SET` (0).
3. **SetStt bleibt bewusst Stub** -- das darunterliegende `dhf_host_fs_setstat()`
   (`Q9-Flux-68k/src/devices/dhf/dhf_host_fs.c`) ist selbst noch ein reines No-Op (ignoriert
   `statbuf`) UND nimmt -- anders als das bereits gefixte `GetStt`/`SS_Size` -- noch einen
   Pfadnamen statt einer Pfadnummer/eines Handles entgegen: dieselbe Fallenklasse, die
   `GetStt` frueher schon einmal den Host-Emulatorprozess haengen liess (s. weiter oben,
   "Found+fixed a real host-hang bug along the way"). Erst eine echte
   `dhf_host_fs_setstat_at()`-Variante bauen (Handle-basiert, analog `getstat_at`), dann
   hier verdrahten.
4. **Gefundener, unabhaengiger EOF-Bug (echter Haenger, nicht simuliert)**: mit ReadLn
   verdrahtet, hing das ECHTE `list /d0/hello.txt`-Kommando nach korrekt ausgegebenem
   Dateiinhalt (im Gegensatz zum eigenen `dhftest`, das nur EINMAL liest und daher nie EOF
   erreicht). Ursache: `dhf_host_fs_read()`/`dhf_host_fs_readln()` (`Q9-Flux-68k`) meldeten
   am echten Dateiende `status=DHF_ERR_OK` mit 0 gelesenen Bytes statt eines Fehlers --
   "OS-9 System Calls" Kap. 2 verlangt aber ausdruecklich: "If there is no data available,
   an EOF error is returned." Ohne diesen Fehler ruft ein Aufrufer wie `list` `I$Read`
   endlos weiter (0 Bytes ohne Fehler ist fuer ihn kein Abbruchgrund). Fix: beide Funktionen
   setzen jetzt `status=DHF_ERR_EOF` (Wert `211`/`$D3`, numerisch identisch mit dem echten
   `E$EOF` aus `MWOS/SRC/DEFS/errno.h` -- die DHF-Fehlercodes sind absichtlich so gewaehlt,
   dass keine Uebersetzungstabelle noetig ist) bei echtem Dateiende. **Ergebnis, verifiziert**:
   `list /d0/hello.txt` gibt den Inhalt aus und kehrt sauber zur Shell zurueck, kein Haengen
   mehr.
5. **`dir /d0` haengt nicht mehr, liefert aber noch keine echte Verzeichnisliste** (nur der
   Geraetename selbst als einzige Zeile) -- OS-9s "dir" liest bei Multi-File-Geraeten rohe,
   fest formatierte Verzeichniseintrags-Records per gewoehnlichem `I$Open`+`I$Read` auf den
   Verzeichnispfad selbst (KEIN eigener "I$OpenDir/I$ReadDir"-Aufruf existiert in OS-9 --
   das ist ein Unix-Konzept). Die Simulator-Kommandos `DHF_CMD_OPENDIR`/`DHF_CMD_READDIR`
   sind darum von KEINEM echten I$-Aufruf aus erreichbar; sie sind ein Relikt aus einer
   frueheren, Unix-artigen Designphase. Eine echte Verzeichnisliste braeuchte ein eigenes,
   DHF-spezifisches Verzeichniseintrags-Binaerformat (Host-Verzeichniseintraege in vom
   Manager per `I$Read` konsumierbare Records serialisiert) -- noch nicht entworfen, naechster
   groesserer Entwurfsschritt, falls gewuenscht.

## 2026-09-26, Nachtrag: SetStt (SS_Size) fertig verdrahtet

Nutzerauftrag: "Bau erst mal SetStt nach". Umgesetzt:

1. **Neue Simulator-Gegenseite** `dhf_host_fs_setsize_at()` (`Q9-Flux-68k/src/devices/dhf/
   dhf_host_fs.c`/`.h`) -- Handle-basiert (`ftruncate()` auf dem gespeicherten fd), analog
   zum bereits vorhandenen `dhf_host_fs_getstat_at()`. Vermeidet dieselbe Pfadnamen-statt-
   Handle-Falle, die `GetStt` frueher den Host-Emulatorprozess haengen liess. `dhf_emu_
   device.c`s `DHF_CMD_SETSTT`-Fall auf `d0`=Pfadnummer/`d1`=gewuenschte Groesse umgestellt
   (vorher: pfadbasiert + 16-Byte-Blob ueber `a1`, an das alte, nie fertig implementierte
   `dhf_host_fs_setstat()` -- diese Funktion bleibt fuer sich bestehen, wird aber von hier
   aus nicht mehr aufgerufen).
2. **`Mgr_SetStt`** (Manager) -- wie `Mgr_GetStt` funktionscode-abhaengig (`d1.w`), passt
   nicht auf `MgrCommon`s uniformes Schema, eigene Routine. Nur `SS_Size` verdrahtet (Set
   File Size, "OS-9 System Calls" Kap. 2: `d0.w`=Pfadnummer, `d1.w`=Funktionscode, `d2.l`=
   gewuenschte Groesse) -- die einzige SetStt-Funktion mit sinnvollem 1:1-Aequivalent gegen
   ein reines Host-Verzeichnis (SS_Attr/SS_Reset/SS_RFM/... haben dort keins). Alles andere
   faellt weiterhin auf `MgrUnkSvc`/`E$UnkSvc` zurueck.
3. **Neuer Test** `test/dhfsetstt_68k.a` (Vorbild `dhftest_68k.a`): `I$Open`(Update-Modus)
   + `I$SetStt(SS_Size=5)` + `I$Close` gegen `/d0/hello.txt`. **Verifiziert**: `OPEN: ok` /
   `SETSTT: ok` / `CLOSE: ok`, echte Host-Datei danach auf 5 Byte gekuerzt (`ls -la` vorher/
   nachher verglichen). Einziger Stolperstein beim ersten Testlauf: ein zu kurz gewaehltes
   `expect`-Timeout (15s) liess es wie einen Haenger aussehen -- war keiner, mit 60s lief
   derselbe Test beim naechsten Versuch sauber durch (frisch gebooteter Emulator + Netzwerk-
   Multiterminal-Verbindung brauchen hier merklich laenger als bei bereits eingespielten
   Modulen).

**Damit sind jetzt alle 13 Sprungtabellen-Eintraege echt verdrahtet** (keiner zeigt mehr
direkt auf `MgrUnkSvc`) -- `GetStt`/`SetStt` faellt lediglich fuer Funktionscodes ausserhalb
ihrer jeweils EINEN verdrahteten Funktion (`SS_Ready`/`SS_Size` bzw. `SS_Size`) weiterhin auf
`E$UnkSvc` zurueck, das ist aber jetzt eine bewusste Fallunterscheidung innerhalb einer
echten Routine, kein pauschaler Stub mehr. Einzige verbleibende groessere Luecke ist die
echte Verzeichnislistung (`dir`, s. vorheriger Abschnitt).

## 2026-09-26, Nachtrag: RBF-Verzeichnisformat gebaut+verifiziert -- echte "dir"-Utility zeigt es noch nicht an

Nutzerauftrag: "pass das dir format an so das es beim dir kommando richtig angezeigt wird".
Der zugrundeliegende Lese-Mechanismus ist jetzt fertig, korrekt und per eigenem Testprogramm
verifiziert -- die STOCK-`/CMDS/dir`-Utility selbst zeigt trotzdem noch nichts an, Grund
gefunden aber NICHT geloest (s. Punkt 4/5 unten).

1. **Directory File Format** ("OS-9 Technical Manual" Kap. 7, lokal unter /Volumes/SSD1TB/
   Documents/OS-9 v2.4 Technical Reference Manual): 32-Byte-Eintraege, Byte 0-27=Dateiname
   (High-Bit auf dem LETZTEN Zeichen gesetzt, Byte 0=0 -> geloescht/Ende), Byte 28 unbenutzt/
   0, Byte 29-31=3-Byte "FD"-LSN-Zeiger (bei DHF ohne echtes Medium bedeutungslos, bleibt 0).
2. **`dhf_host_fs_open_at()`** (`Q9-Flux-68k/src/devices/dhf/dhf_host_fs.c`) erkennt jetzt
   per `stat()`, ob das Ziel ein Host-Verzeichnis ist, und benutzt dann `opendir()` statt
   `open()` (ein Directory-fd laesst sich unter POSIX nicht `read()`en -- `EISDIR`).
   **`dhf_host_fs_read()`** liefert fuer solche Handles ueber die neue Hilfsfunktion
   `dhf_read_dir_entries()` die echten 32-Byte-RBF-Eintraege (per `readdir()`, inkl. der vom
   Host ohnehin schon geliefertern "."/".."-Eintraege) -- am echten Verzeichnisende (kein
   `readdir()`-Ergebnis mehr) greift dieselbe `DHF_ERR_EOF`-Logik wie bei normalen Dateien.
3. **Neuer Test** `test/dhfdirtest_68k.a`: `I$Open`+Leseschleife (ein `I$Read` je 32-Byte-
   Eintrag)+`I$Close` gegen `/d0`. **Verifiziert, mehrfach reproduzierbar**: listet
   `.`/`..`/`hello.txt`/`hello_backup.txt` korrekt und beendet sauber mit `E$EOF` ($D3).
   Zwei Testlaeufe wirkten zunaechst wie ein echter Haenger (Ausgabe brach nach ~24 Byte der
   ersten Meldung ab, auch nach 150s kein Fortschritt) -- ein DRITTER Lauf (mit leichter
   `Q9_ITRACE_LINK`-Instrumentierung, aus Diagnosegruenden dazugeschaltet) lief dagegen sofort
   sauber durch, ein VIERTER Lauf ganz ohne jede Instrumentierung ebenfalls -- also doch kein
   echter Haenger, sondern Netzwerk-Multiterminal-Verzoegerung wie schon bei `dhfsetstt`
   dokumentiert (s. oben), nur diesmal mit besonders langer Verzoegerung. Kein Code-Bug.
4. **Warum die STOCK-`dir`-Utility trotzdem nichts anzeigt**: per `Q9_ITRACE_PATH`+
   Disassemblierung von `/CMDS/dir` (echtes Microware-Binary, 9302 Byte) verifiziert: `dir
   /d0` ruft `I$Open("/d0", d0.b=0)` (Modus 0 -- laut "OS-9 System Calls" Kap. 2 ein reiner
   Attribut-Probe-Open, "does not permit any actual I/O on the path"), bekommt Pfadnummer 6
   zurueck (kein Fehler), tut dann NICHTS WEITER mit Pfad 6 (kein `I$Read`, kein `I$GetStt`,
   kein `I$ChgDir` -- per vollstaendiger `Q9_TRAP_TRACE_ALL`-Spur bestaetigt: die einzigen
   `I$GetStt`-Aufrufe in diesem Fenster zielen auf Pfad 1 (Konsole), nicht auf Pfad 6) und
   schliesst Pfad 6 sofort wieder -- druckt danach nur den blossen Geraetenamen ("/d0") als
   Fallback. `dir` entscheidet also OHNE weiteren Syscall auf Pfad 6, muss also etwas direkt
   aus dem Pfaddeskriptor (Zeiger in `a2`, von `I$Open` zurueckgegeben) lesen.
5. **Versuchte, NICHT erfolgreiche Erklaerung**: `PD_ATT` (Pfaddeskriptor-Offset `$B5`, Bit 7
   = "Set if directory file", "OS-9 Technical Manual" Anhang B, "RBF Definitions of the Path
   Descriptor") ist laut Handbuch GENAU das Feld, das ein Aufrufer ohne Syscall pruefen
   koennte -- ABER dieses Feld ist ausdruecklich RBF-EIGEN ("Maintained by: File Manager",
   nur RBF fuellt es tatsaechlich). `MgrCommon` wurde entsprechend erweitert (`movea.l a1,a0`
   am Anfang rettet den urspruenglichen Pfaddeskriptor, da `a1` gleich auf `CmdBlk`
   umgebogen wird; nach erfolgreichem `I$Open`, wenn der Simulator ueber ein neues,
   sonst bei OPEN ungenutztes `SH_D2`-Ruecklauffeld "ist ein Host-Verzeichnis" meldet
   (`dhf_host_fs_open_at()`s `is_dir`, per `dhf_emu_device.c`s `DHF_CMD_OPEN`-Fall
   durchgereicht), schreibt `MgrCommon` `$80` nach `PD_ATT(a0)`). **Ergebnis: keine
   Aenderung im Verhalten von `dir`** -- entweder liest `dir` ein anderes Feld/nutzt einen
   anderen Mechanismus (z. B. Pruefung des FileManager-Modulnamens gegen "rbf", oder ein
   Feld, dessen Offset in diesem Handbuch fuer eine andere OS-9-Version/Konfiguration nicht
   exakt passt), oder mein Schreibzugriff landet aus einem noch nicht gefundenen Grund nicht
   dort, wo `dir` tatsaechlich hinschaut. Der PD_ATT-Schreibzugriff selbst ist harmlos und
   bleibt im Code (schadet nichts, hilft aber `dir` bisher nicht) -- **nicht weiter verfolgt,
   da eine vollstaendige Klaerung eine tiefere Disassemblierung des kompletten `dir`-Binaries
   braeuchte** (ueber das hinausgehend, was fuer diese Sitzung vertretbar war).
6. **Fazit**: das DHF-Verzeichnisformat selbst ist fertig, korrekt und wiederverwendbar
   (jedes eigene oder zukuenftige Werkzeug kann `/d0` wie jedes andere Verzeichnis oeffnen
   und lesen). Die STOCK-`dir`-Utility zu ueberzeugen ist ein eigenstaendiges, noch offenes
   Reverse-Engineering-Problem -- naechster Schritt waere eine vollstaendige Disassemblierung
   von `/CMDS/dir`, oder alternativ ein eigenes, einfaches Listing-Werkzeug (wie
   `dhfdirtest_68k.a`, nur benutzerfreundlicher formatiert) als praktischer Ersatz.

## 2026-09-26, Nachtrag: I$GetStt SS_FD verdrahtet -- echte "attr"-Utility funktioniert jetzt

Nutzerfrage: "gibt es vielleicht noch set-/getstat funktionen die uns noch fehlen? wie
funktioniert z.B. ein attr?" -- Antwort: ja, `SS_FD` fehlte, jetzt gefunden+behoben.

1. **`attr /d0/hello.txt` scheiterte** mit "attr: error reading FD sector." Per
   `Q9_TRAP_TRACE_ALL` verifiziert: `attr` ruft nach dem `I$Open` sofort `I$GetStt` mit
   Funktionscode `$0F` (`SS_FD`, sg_codes.h: "return file descriptor") auf Pfad 6 auf --
   **kein physischer Sektorzugriff**, sondern ein ganz gewoehnliches GetStt mit dokumentiertem
   Format ("OS-9 System Calls" Kap. 2: `d0.w`=Pfadnummer, `d1.w`=`$0F`, `d2.w`=gewuenschte
   Byteanzahl, `a0`=Zielpuffer). Das widerlegt die vorherige Vermutung, `attr`/`dir` braeuchten
   echte RBF-LSN-Sektoren -- nur `attr` (nicht `dir`) nutzt diesen Mechanismus, und er ist
   vollstaendig ohne echtes Blockmedium abbildbar.
2. **FD-Sektor-Format** ("OS-9 Technical Manual" Kap. 7, Figure 7-2): Offset `$00`(1)=FD_ATT
   (Attribute, Bit7=Verzeichnis/Bit6=exklusiv/Bit5-3=oeffentlich x-w-r/Bit2-0=Besitzer x-w-r),
   `$01`(2)=FD_OWN, `$03`(5)=FD_DAT (Jahr/Monat/Tag/Stunde/Minute), `$08`(1)=FD_LNK,
   `$09`(4)=FD_SIZ, `$0D`(3)=FD_CREAT (Jahr/Monat/Tag), `$10`(240)=FD_SEG (Segmentliste --
   bleibt genullt, "Unused segments must be zero").
3. **Neue Funktion** `dhf_host_fs_getfd_at()` (`Q9-Flux-68k/src/devices/dhf/dhf_host_fs.c`,
   Handle-basiert wie `getstat_at`/`setsize_at`) baut dieses Abbild aus `fstat()`/`stat()`
   (Unix-Rechte-Bits -> FD_ATT, `st_size` -> FD_SIZ, `st_mtime`/`st_ctime` -> FD_DAT/FD_CREAT
   per `gmtime_r()`). Neues Wire-Kommando `DHF_CMD_GETFD=20` (`dhf_proto.h`), neuer Fall in
   `dhf_emu_device.c` (liest `d0`=Handle/`d1`=gewuenschte Bytezahl, schreibt via
   `resolve_guest_ptr` direkt in den vom Aufrufer vorgegebenen Gast-RAM-Puffer -- NICHT ueber
   `CmdBlk`, da bis zu 256 Byte angefordert werden koennen, mehr als `CmdBlk`s 32 Byte fassen).
4. **`MgrGst_FD`** (Manager, neue Verzweigung in `Mgr_GetStt` neben `SS_Ready`/`SS_Size`):
   `SH_D0`=Pfadnummer, `SH_A1`=`REG_A0(a5)` (Aufrufer-Zielpuffer, bewusst NICHT `CmdBlk`s
   eigene Adresse wie bei `SS_Size`), `SH_D1`=`REG_D2(a5)` (gewuenschte Byteanzahl).
5. **Ergebnis, verifiziert**: `attr /d0/hello.txt` zeigt jetzt `----r-wr  /d0/hello.txt`
   (korrekte Unix-Rechte), `attr /d0` zeigt `d-e-rewr  /d0` (Verzeichnis-Bit korrekt
   gesetzt!). **`dir /d0` bleibt trotzdem unveraendert** (weiterhin nur der Geraetename) --
   per erneutem Test bestaetigt, dass `dir` NICHT ueber `SS_FD` entscheidet, sondern ueber
   einen anderen, noch nicht gefundenen Mechanismus (s. vorheriger Abschnitt) -- `attr` und
   `dir` loesen ihre "ist das ein Verzeichnis"-Frage also unterschiedlich.
6. **Restliche GetStt/SetStt-Funktionscodes, ueberflogen** (vollstaendige Liste: `MWOS/OS9/
   SRC/DEFS/sg_codes.h`) -- die meisten (CDFM/Sockets/UCM/Grafik/...) sind fuer ein reines
   Host-Passthrough-Dateisystem irrelevant. Plausible, noch nicht verdrahtete Kandidaten,
   FALLS spaeter gebraucht: `SS_Attr` (`$1C`, SetStt-Gegenstueck zu unserem neuen `SS_FD` --
   wuerde `attr -w`/`chmod`-artige Aenderungen erlauben, aktuell nur lesend), `SS_Pos` (`$05`,
   aktuelle Position abfragen), `SS_EOF` (`$06`, Dateiende pruefen ohne zu lesen), `SS_DevNm`
   (`$0E`, Geraetename zurueckgeben, trivial). Keiner davon wurde angefragt oder als fehlend
   beobachtet -- nur dokumentiert, nicht implementiert.

## 2026-09-26, Nachtrag: SS_Attr/SS_Pos/SS_EOF/SS_DevNm verdrahtet -- alle vier verifiziert

Nutzerauftrag: "die vier implementiere bitte alle noch". Alle vier gegen die echten
Registertabellen aus "OS-9 System Calls" Kap. 2 geprueft, gebaut und per neuem Testprogramm
`test/dhfstt2_68k.a` verifiziert (Ausgabe: `SS_DevNm: ok, Name: d0` / `SS_EOF: nicht am
Ende` (vor dem Lesen) / `SS_Pos: ok, Position: $00000020` (nach Lesen der 32-Byte-Datei) /
`SS_EOF: AM ENDE` (danach) / `SS_Attr: ok` -- Host-Datei tatsaechlich auf `r--------`
gesetzt, per `ls -la` bestaetigt und wieder zurueckgesetzt).

1. **`SS_Attr`** (SetStt, `$1C`, Gegenstueck zu `SS_FD`): `d2.w`=neues Attribut-Byte (gleiche
   Bitlage wie `FD_ATT`). Neue Simulator-Funktion `dhf_host_fs_setattr_at()` mappt die Bits
   auf Unix-Rechte (Bit0-2=Besitzer r/w/x, Bit3-5=oeffentlich r/w/x) und ruft `fchmod()`
   (bzw. `chmod()` fuer Verzeichnis-Handles) auf. Bit6 (exklusiv) und Bit7 (Verzeichnis)
   werden bewusst ignoriert (kein Host-Aequivalent bzw. per Handbuch ohnehin nicht erlaubt,
   das Verzeichnis-Bit einer normalen Datei zu setzen). Neues Wire-Kommando
   `DHF_CMD_SETATTR=21`. Manager: `MgrSst_Attr` (neue Verzweigung in `Mgr_SetStt`).
2. **`SS_Pos`** (GetStt, `$05`): keine Eingabe ausser Pfadnummer, Ausgabe `d2.l`=aktuelle
   Position. Neue Simulator-Funktion `dhf_host_fs_getpos_at()` (`lseek(fd,0,SEEK_CUR)`).
   Neues Wire-Kommando `DHF_CMD_GETPOS=22`. Manager: `MgrGst_Pos`.
3. **`SS_EOF`** (GetStt, `$06`): Erfolgsfall `d1.l=0` (nicht am Ende); am Dateiende Carry+
   `d1.w=E$EOF`. Neue Simulator-Funktion `dhf_host_fs_iseof_at()` vergleicht aktuelle
   Position (`lseek`) gegen Dateigroesse (`fstat`) und setzt `status=DHF_ERR_EOF` (== echtes
   `E$EOF`, keine Umrechnung noetig) wenn erreicht. Manager: `MgrGst_Eof` setzt `SH_D1`
   VOR dem Aufruf explizit auf 0 (sonst koennte Altzustand aus `CmdBlk` durchscheinen).
   Neues Wire-Kommando `DHF_CMD_ISEOF=23`.
4. **`SS_DevNm`** (GetStt, `$0E`): `a0`=32-Byte-Zielpuffer, Ausgabe NUL-terminiert. Braucht
   **keinen** Treiber-/Simulator-Aufruf -- der Geraetename steht bereits im Modulkopf des
   Geraetedeskriptors selbst, rein lokal lesbar: `PD_DEV(a1)` -> `Devicetbl` -> `V_desc`
   (Offset `8`, neu als `DT_DESC` benannt) -> Deskriptor-Modulbasis -> `M$Name` (Modulkopf-
   Feld, Offset `12`, ein LONGWORD-Offset auf den eigenen NUL-terminierten Namensstring,
   neu als `MOD_NAME_OFF` benannt) -> Name kopieren. Manager: `MgrGst_DevNm`.
5. **Toolchain-Vorsicht bei den neuen Verzweigungen**: `MgrGst_TrySize`s Dispatch-Kette hat
   jetzt sechs `cmpi.l`/`beq`-Paare -- die drei NEUEN Ziele (`MgrGst_Pos`/`MgrGst_Eof`/
   `MgrGst_DevNm`) liegen (nach `MgrGst_FD`s vollem Rumpf) weiter weg, als ein 8-Bit-
   Kurzsprung (`beq.s`) sicher abdeckt; bewusst `beq.w` verwendet (`qr68k` waehlt
   Sprungweiten NICHT automatisch, s. "-b"-Flag) -- sonst waere das dieselbe Fehlerklasse
   wie der `DhfMgrEnt`-Tabellenbug weiter oben gewesen. Vor dem Deployment per Capstone-
   Disassemblierung des fertigen `.mod`-Files verifiziert: alle Sprungziele landen exakt auf
   den erwarteten Instruktionsgrenzen, `.w`/`.b`-Kodierung von `qr68k` korrekt gewaehlt.

## 2026-09-26, Nachtrag: SS_Rename/SS_Free verdrahtet -- und die "Terminal-Verzoegerung" war ein Testskript-Fehler

Nutzerauftrag: "dann mach bitte noch das rename und free". Umgesetzt (von einer Sitzung
begonnen, deren Limit vor dem Commit auslief; von einer Folgesitzung nachgeprueft und
abgeschlossen):

1. **`SS_Rename`** (SetStt, `$42`): `a0`=Zeiger auf den neuen Namen. Neue Simulator-
   Funktion `dhf_host_fs_rename_at()` (Handle-basiert wie alle `_at`-Varianten: alte Seite
   ist der beim Open gespeicherte Host-Pfad, neuer Name laeuft durch dieselbe Basispfad-
   Einsperrung `resolve_confined_path()`; der gespeicherte Handle-Pfad wird danach
   nachgefuehrt). Neues Wire-Kommando `DHF_CMD_RENAMEAT=24`. Manager: `MgrSst_Rename`.
   **Registerkonvention ist eine eigene Annahme** -- "OS-9 System Calls" dokumentiert
   `SS_Rename` nicht (nur `sg_codes.h`: "0x42 rename file"), und die echte `rename`-Utility
   verweigert sich bei jedem Nicht-RBF-FileManager schon VOR jedem Syscall ("pathname not
   RBF device", per `Q9_TRAP_TRACE_ALL` gesehen). Nur fuer eigene Werkzeuge nutzbar.
2. **`SS_Free`** (GetStt, `$43`): Ausgabe `d0.l`=freier Platz in Byte (ungewoehnlich: `d0`,
   nicht `d1`/`d2` wie die anderen GetStt-Antworten). Neue Simulator-Funktion
   `dhf_host_fs_getfree()` (`statvfs()` auf den Basispfad, auf 32 Bit gekappt). Neues
   Wire-Kommando `DHF_CMD_GETFREE=25`. Manager: `MgrGst_Free`. Die echte `free`-Utility
   erreicht auch diesen Aufruf nie -- sie oeffnet immer roh `<geraet>@` und liest Bitmap-
   Sektoren, fuer ein Host-Passthrough ohne LSNs nicht abbildbar.
3. **Neuer Test** `test/dhfrenfree_68k.a` (Modul `dhfrfr`, im Image als
   `/CMDS/dhf/dhfrenfree`): `I$Open`(Update) + `SS_Free` + `SS_Rename("hello_renamed.txt")`
   + `I$Close`. **Verifiziert, 3 von 3 Laeufen ohne jede Instrumentierung**: `SS_Free: ok,
   $FFFFFFFF` (Host meldet ~40 GB frei, korrekt gekappt), `SS_Rename: ok`, `CLOSE: ok`; die
   Host-Datei heisst danach tatsaechlich `hello_renamed.txt` (nach jedem Lauf zurueck-
   benannt). Manager- und Testmodul im Image `OS9SYS_Claude.hda` byteidentisch (`cmp`) mit
   einem frischen Bau aus dem committeten Quellstand.
4. **Regression gegen den neuen Manager, ein Lauf**: `dhftest` (Open/Read/Close), `dhfstt2`
   (DevNm/EOF/Pos/Attr), `dhfdirtest` (Verzeichnis bis `E$EOF` `$D3`), `list /d0/hello.txt`,
   `attr /d0/hello.txt`, `attr /d0` (`d-e-rewr`) -- alle unveraendert gruen. (`dhfstt2`
   setzt die Host-Datei auf `r--------`; danach `chmod 644` zuruecksetzen.)

**KORREKTUR zu den frueheren "Netzwerk-Multiterminal-Verzoegerungen"** (bei `dhfsetstt` und
`dhfdirtest` oben, und zunaechst auch hier): das war KEIN Timing-Effekt des Emulators,
sondern ein Fehler in den `expect`-Skripten. Deren Prompt-Muster `set prompt {[#$] ?$}`
greift auf JEDES `$` am Ende des bisher gelesenen Puffers -- also auch auf das `$` einer
Hex-Ausgabe (`$FFFFFFFF`) oder eines Syscall-Namens im Testbanner (`I$Open`, `I$SetStt`).
Das Skript haelt den Befehl dann fuer beendet und schickt sofort `Ctrl-]` -> Emulator weg,
Ausgabe mitten im Wort abgeschnitten. Genau das zeigen die Fehllaeufe hier: `$FFFFF`,
`$FFFFF`, `$FFFF` -- je nachdem, wie viele Zeichen gerade im Puffer lagen. Dass es mit
`Q9_ITRACE_*` "sofort durchlief", war Zufall der Pufferung, kein Beleg. Bei `dhfdirtest`
("Ausgabe brach nach ~24 Byte der ersten Meldung ab") passt es exakt: das erste `$` steht
in `=== dhfdirtest: I$Open...` nach 18 Zeichen. **Fix: Prompt-Muster auf den echten Prompt
festnageln, `set prompt {ROOT# $}`** -- damit liefen alle Laeufe oben beim ersten Versuch
durch. Wer ein neues Testskript schreibt: dieses Muster nehmen, nicht `[#$]`.
