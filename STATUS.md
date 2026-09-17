# Q9-OS status

This file describes the current implementation status of the OS-9 system
calls in Q9-OS.  The status refers to the Q9-OS kernel and its current
IOMan integration, not to the original OS-9 implementation.

| Status | Meaning |
|---|---|
| ✅ | Fully implemented in Q9-OS and exercised without Microware dependencies |
| 🟡 | Partially implemented, limited, or not yet completely verified |
| 🔷 | Kernel-level path is usable with Microware components in a current emulator test; Q9-native replacement is still open |
| ❌ | Not implemented or still routed to the unimplemented-service stub |
| ⛔ | Withdrawn in real OS-9/68K itself; deliberately not implemented here |

The call-code names and the complete call-code set are based on
`Q9-KERNEL/.os9-original/SYSCALL_MODULE_MAP.md`.  A green status does not
mean that every OS-9 corner case or every hardware device is already
supported.

## Latest kernel verification (2026-09-17)

The external `F$SSvc` trap return path was corrected: the 72-byte service
register frame is now removed with the correct stack adjustment before the
original exception frame is returned with `RTE`. The previous six-byte
overrun caused sporadic returns to `PC=$6c`.

The development kernel and CF bootfile were rebuilt and exercised in Q9-Flux.
A subsequent approximately 90-second stability run produced about 560,000
scheduler output characters without an exception, illegal instruction, or
`PC=$6c`; the diagnostic dump reported `Vektor=0`. This validates the current
external trap return path with the present Microware I/O modules. Native Q9
I/O replacements remain future work.

The current `I$ChgDir("/dd")` measurement reaches its return PC again with
`D0=$00000005` and `D1=$00008200`; it does not hang in the external IOMan
call. The isolated `date` test reaches `F$Load("/dd/CMDS/date")` without an
exception, but the load does not return in the current test window. CF
tracing shows repeated reads of the same directory/file sector (`LBA 65`),
so the remaining issue is currently classified as an RBF directory/position
advance problem in the external `F$Load` path.

The native `I$Open` path now also publishes each allocated path number in
the current process' `P$Path` table. Host regression tests cover the first
two process-local entries, and a fresh 68k build plus emulator boot completed
without an exception. This fixes the process-table bookkeeping gap, but it
does not yet provide full pathname, device, or file-manager semantics.

The path lifecycle tests now distinguish the process-local path index from
the global descriptor number. Two simulated processes can both use local
path 3 while referring to different descriptors; the host tests pass and a
fresh emulator boot again reaches the CF driver and the Q9 test programs
without an exception.

`I$Dup` now rejects stale or malformed native descriptors before increasing
their reference count. `I$Close` validates the descriptor before removing
the process-table entry, preventing a failed close from silently losing the
path. The host regression suite and a fresh 68k emulator boot pass with
these changes.

The native `I$Dup` and `I$Close` handlers now resolve the process path table
through `Q9_D_Proc`, rather than relying on the caller's A4 value. `I$Close`
also preserves the process-local path index while calculating the global
descriptor address. This fixes the handler-side ambiguity and was rebuilt
successfully for 68k; emulator startup reaches the native duplicate path
without entering its bad-descriptor branch. The dedicated child-process
regression now passes in Q9-Flux as the marker sequence `ODCc@#`: native
`I$Open`, `I$Dup`, both native `I$Close` calls, followed by Microware
`I$Attach` and `I$Detach`. The direct-attach startup switch is compile-time
gated and remains disabled in normal development boots.

The native `I$ChgDir` baseline is now included in the same child-process
regression. Marker sequence `HODCc@#` confirms that a data-directory change
(`d0=3`, `/dd`) stores successfully before the native path lifecycle and
Microware attach/detach checks. Execution-directory storage (`d0=1`) uses the
parallel `P$DIO` slot; full device and file-manager directory resolution is
still open.

The native console I/O regression now also exercises `I$Write`: marker
sequence `HOwWDCce@#` shows `I$ChgDir`, a real one-byte write (`wW`), the
native path lifecycle, and Microware attach/detach. An attempted write on
unused path 31 is rejected (`e`) before any output is emitted. The shared
native path validator now checks process-local path bounds, descriptor
liveness, and read/write mode bits.

The same validator is now used by `I$WritLn` and `I$ReadLn`. The emulator
regression produced `HOwWl\rLDCcer@#`: valid write and line-write operations
complete, an invalid `I$ReadLn` path is rejected without blocking, and the
existing native lifecycle plus Microware attach/detach checks still pass.

`I$GetStt` and `I$SetStt` now use the same current-process validation before
accessing the native path descriptor. The emulator regression extended to
`HOwWl\rLGSDCcergs@#`: valid `SS_Opt` get/set operations succeed, invalid
path 31 is rejected for both calls, and the previous I/O lifecycle remains
green.

`F$SPrior` (call code `0x0D`) is the first writing process-API call. It
resolves the process ID through the existing descriptor lookup and stores the
new priority, rejecting an unknown or free process ID with `E$IPrcID`. The
emulator regression extended to `HOwWl\rLGSDCcergsIPp@#`: `F$ID` returns the
caller's own PID, `F$SPrior` on that PID succeeds, and `F$SPrior` on an
invalid PID is rejected. A separate boot with the regression switch disabled
reported `Vektor=0`, so the normal boot path is unaffected. Two deliberate
limitations remain: the priority is truncated to the descriptor's single
priority byte, and the scheduler applies a changed priority only on the next
ready-queue entry, so raising a running process' priority does not preempt
immediately.

`F$CmpNam` (call code `0x11`) completes the pair that RBF uses to walk a
directory: `F$PrsNam` already returns the start and length of one name inside
a pathlist, and both now go straight into `F$CmpNam`. The pattern is therefore
taken by length rather than by null termination, while the target name is null
terminated, as documented. Upper and lower case match, `?` matches one
character and `*` matches any string. The emulator regression extended to
`HOwWl\rLGSDCcergsIPpNnx@#`: an exact match, a wildcard match, and a correctly
rejected mismatch, with a second boot again reporting `Vektor=0`.

One deliberate deviation: the documented second error `E$StkOvf` ("pattern too
complex") reflects the original implementation, which recurses on the stack for
every `*`. This version compares iteratively with a single backtracking point
and constant memory, so no pattern can overflow it and `E$StkOvf` is never
reported. Every pattern the original accepts is accepted here too.

`F$SUser` (call code `0x1C`) and `F$CpyMem` (`0x1B`) round out the process API.
`F$SUser` required a real group/user field first: `P$User` at descriptor offset
`$14` now exists, is set to 0.0 on process creation, inherited across `F$Fork`,
and returned by `F$ID`, which previously reported a hardcoded zero. Of the three
documented cases in which a change is permitted, only the first ("user 0.0 may
change freely") is implemented; the other two need the owning module's user
number, a field this kernel does not yet carry, so they return `E$Permit` rather
than pretending to allow the change.

`F$CpyMem` validates the owning process ID and then copies. Its real purpose in
OS-9 is translating an address out of a foreign address space; this kernel runs
without address separation, so the translation is simply not needed yet and the
PID check is the only load-bearing semantics. When address spaces arrive, the
translation belongs exactly there and callers stay unchanged.

The emulator regression extended to `HOwWl\rLGSDCcergsIPpNnxMmUVu@#`: a copy
whose bytes arrive, a rejected invalid PID, an accepted ID change, `F$ID`
reporting that new ID, and a second change correctly refused. A separate boot
with the regression switch disabled reported `Vektor=0`.

That live test caught a real bridge bug that the host tests had missed: the
assembler reads success and error as the low word of a 32-bit cell (`+2`), so
the C side must write them at full width. Both bridges used a 16-bit write,
which lands in the high word, and every call therefore looked like a failure to
the assembler. The host suite now covers the bridges themselves, not just the
comparison logic underneath them.

`F$CRC` (call code `0x17`) computes the 24-bit module CRC, accumulated across
any number of calls from an accumulator initialised to -1. The bit steps were
taken from the project's own reference implementation, which has long been
checked against real modules, rather than re-derived from a polynomial. The
strongest available proof is built into both test levels: running the CRC over
a complete, toolchain-built module *including* its own three CRC bytes must
yield the documented constant `CRCCon` (`$00800FE3`). The host test does this
over an embedded real module; the emulator test does it over a module actually
resident in memory, reached through `F$Link`.

`F$UnLoad` (`0x1D`) differs from `F$UnLink` only in its input: a module name
instead of a header address. It therefore picks its target with exactly the
same rule as `F$Link` (including the revision tie-break) and decrements with
exactly the same routine as `F$UnLink`, rather than introducing a third
variant. The shared lookup was factored out of `F$Link` so that `F$UnLoad` does
not have to raise the link count only to take it straight back down; a host
test asserts that the counter drops by exactly one.

The emulator regression extended to `HOwWl\rLGSDCcergsIPpNnxMmUVuRYy@#`, with a
separate boot reporting `Vektor=0`.

`F$Mem` (`0x07`) is marked withdrawn rather than missing. The manual states
plainly that "F$Mem is no longer available. Use F$SRqMem instead.", so
implementing it would mean building something real OS-9/68K itself removed.
`F$SRqMem` already covers the need and is green.

`F$GPrDsc` (`0x18`), `F$GModDr` (`0x1A`) and `F$SetCRC` (`0x26`) build on the
three calls above. `F$GPrDsc` copies a process descriptor out for inspection and
is strictly read-only, as documented; it never copies past the descriptor size,
because everything beyond that already belongs to the next pool slot.

`F$GModDr` copies the module directory in whole entries, never a partial one.
The format is this kernel's own 16-byte entry layout: the manual states that the
directory format may differ between OS-9 releases and that the call exists for
tools like `mdir`, so imitating a foreign layout would be a pretence that fits
nothing.

`F$SetCRC` updates both check values of a module image, in order: first the
header parity, then the CRC across everything but the three CRC bytes, so the
CRC covers the parity just written. It is checked without an expected-value
table: after the call, `F$CRC` over the whole image must yield `CRCCon` again
and the XOR of all header words must be `$FFFF` — exactly the two conditions
real OS-9 tools measure a module by.

The emulator regression extended to `HOwWl\rLGSDCcergsIPpNnxMmUVuRYyDdBT@#`,
with a separate boot reporting `Vektor=0`. The documented even-address
requirement for `F$SetCRC` proved real rather than theoretical: the test image
first landed on an odd address behind an odd-length string table and was
correctly refused, which is what the alignment check is there for. The live test
deliberately runs `F$SetCRC` over a private module image, never over a resident
module — the manual warns that altering a known module's header makes it
inaccessible to every other process.

`F$Julian` (`0x20`) and `F$Gregor` (`0x54`) convert between field-packed
date/time (`yyyymmdd`, `00hhmmss`) and the OS-9 Julian day number. The zero
point is the crux and is not guessed: `MWOS/SRC/DEFS/time.h` defines `JULBASE
2440587` as the Julian date for 1970-01-01, one below the astronomical day
number for that date — consistent with the manual's note that OS-9 changes
Julian dates at midnight rather than noon. The manual's weekday formula
`MOD(Julian+2, 7)` confirms this zero point for 1970-01-01, 2000-01-01 and the
1582-10-15 changeover, and fails for the astronomical value on all three. The
changeover is implemented as documented, with the Julian leap rule before it.
The host test proves `F$Gregor` is the exact inverse by round-tripping every one
of the 225798 days from 1582-10-15 to 2200-12-31. The emulator regression
extended to `HOwWl\rLGSDCcergsIPpNnxMmUVuRYyDdBTJGj@#`.

These two calls needed the first real 32-bit division in the kernel. Every
other C file divides only by powers of two, which the compiler turns into
shifts, so `__udivide` and `__umodulo` never existed — plain 68000 has no
instruction for it. Both now live in `q9kernel_entry.a` next to `__multiply`,
with the same register convention; the live `F$Julian` check against JULBASE
doubles as the proof of that convention, since swapped operands could never
produce 2440587.

**A latent boot-layout bug surfaced and was fixed on the way.** The new code
grew the kernel by about 2k, and the emulator then crashed before the first
marker (`Vektor=4`/`11`, PC inside the `cfide` CompactFlash driver, on a data
table rather than code). An unmodified `main` kernel plus 2k of pure padding
crashed identically, which ruled out the new code and identified a pure size
effect. Root cause: the boot/supervisor stack sat at a fixed `$10000..$18000`
under a comment promising the boot file could grow to `$10000` — but the boot
file has long since grown to about `$56000`. The stack had therefore been
sitting in the middle of the boot modules for weeks, harmless only because it
grows down from `$18000` and uses a few KB; once the kernel grew, `cfide`
landed at `$17b82..$18138`, directly under the stack top, and the first pushes
during boot overwrote its code. The arena had already been made dynamic on
2026-09-15; the stack had not. The stack now sits at the end of RAM, taken
from the total-RAM value the boot ROM passes in `D0` — the only two
instructions before it are the SR write and saving the old SP — and the arena
reserves the last 32k for it. The zones are now Globals / boot file / arena /
stack, and no fixed address remains that could go stale again.

A related finding is recorded for whoever fixes the underlying A4-origin bug
one day: the 12-byte dead zone that was meant to cover file offset `$3ac`
(where RBF/SCF blindly `addq.l #1,$3ac(a4)` with `a4` pointing at this kernel)
has been sitting about 612 bytes further back for some time, in kernels that
boot perfectly well. The symptom guard is therefore displaced and inert; the
kernel survives because `addq`/`subq` cancel and nothing critical happens to
live at `$3ac`. `build.sh` now prints the bytes at `$3ac` after every link as a
reminder, without failing the build.

The host regression suite was repaired in the same pass. Four of the sixteen
suites had silently stopped building or running: `q9kernel_sysmem.c` and
`q9kernel_moddir.c` gained memory-trace calls whose stubs were missing from
their tests, `q9kernel_procend.c` gained a `Q9K_ProcMemReleaseAll` dependency,
and `test_q9kernel_sysmem.c` plus `test_q9kernel_firstproc.c` reached real
kernel addresses (`$1710`, `$004C`, `$0404`, `$1284`) because those constants
were not redirectable. The constants now follow the `#ifndef` convention used
elsewhere in the kernel, and the tests redirect them into their own fake
globals. All sixteen suites build and pass again.

## F$ system calls

| Status | Code | Command | Current Q9-OS status |
|---|---:|---|---|
| ✅ | `0x00` | F$Link | Kernel module lookup/link path implemented and exercised |
| 🟡 | `0x01` | F$Load | External path is entered, but current `echo` and `date` file loads do not return; `csl` is resident |
| ✅ | `0x02` | F$UnLink | Kernel module unlink path implemented |
| ✅ | `0x03` | F$Fork | Process creation and memory ownership implemented and tested |
| ✅ | `0x04` | F$Wait | Child/zombie handling implemented and tested |
| ❌ | `0x05` | F$Chain | Not implemented |
| ✅ | `0x06` | F$Exit | Process exit, primary memory and tracked user allocations released |
| ⛔ | `0x07` | F$Mem | Withdrawn in real OS-9/68K ("F$Mem is no longer available. Use F$SRqMem instead."); deliberately not implemented |
| ✅ | `0x08` | F$Send | Signal path implemented and tested at kernel level |
| ❌ | `0x09` | F$Icpt | Not implemented |
| 🟡 | `0x0A` | F$Sleep | Scheduler sleep path exists; complete timing coverage remains open |
| ❌ | `0x0B` | F$SSpd | Not implemented |
| ✅ | `0x0C` | F$ID | Process identity path implemented |
| ✅ | `0x0D` | F$SPrior | Priority change on a live process descriptor, implemented and verified in the emulator; see the note on scheduler re-queue timing below |
| ❌ | `0x0E` | F$STrap | Not implemented |
| 🟡 | `0x0F` | F$PErr | Microware path exists; current Q9 compatibility is not fully verified |
| ✅ | `0x10` | F$PrsNam | Path-name parsing implemented |
| ✅ | `0x11` | F$CmpNam | Name comparison with `?`/`*` wildcards and case folding, implemented and verified in the emulator |
| 🟡 | `0x12` | F$SchBit | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x13` | F$AllBit | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x14` | F$DelBit | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x15` | F$Time | Handler exists; clock source and full validation remain open |
| ❌ | `0x16` | F$STime | Not implemented |
| ✅ | `0x17` | F$CRC | 24-bit module CRC, accumulated across calls; verified against a real module and the documented CRCCon constant |
| ✅ | `0x18` | F$GPrDsc | Read-only copy of a process descriptor, length-capped at the descriptor size |
| ❌ | `0x19` | F$GBlkMp | Not implemented |
| ✅ | `0x1A` | F$GModDr | Copies the module directory out in whole entries; the format is this kernel's own, as the manual allows |
| ✅ | `0x1B` | F$CpyMem | Copy with owner-PID validation; no address translation is needed while all processes share one flat address space |
| ✅ | `0x1C` | F$SUser | Changes the caller's own group/user ID in the process descriptor; only the documented "user 0.0 may change freely" case is implemented |
| ✅ | `0x1D` | F$UnLoad | Same lookup rule as F$Link and the same counter as F$UnLink, keyed by module name |
| ❌ | `0x1E` | F$RTE | Not implemented |
| ❌ | `0x1F` | F$GPrDBT | Not implemented |
| ✅ | `0x20` | F$Julian | Packed date/time to OS-9 Julian day; zero point anchored on JULBASE from time.h, 1582 changeover implemented |
| 🟡 | `0x21` | F$TLink | Trap linking works; unlink and complete lifetime handling remain open |
| ❌ | `0x22` | F$DFork | Not implemented |
| ❌ | `0x23` | F$DExec | Not implemented |
| ❌ | `0x24` | F$DExit | Not implemented |
| ❌ | `0x25` | F$DatMod | Not implemented |
| ✅ | `0x26` | F$SetCRC | Updates header parity and module CRC; verified by re-checking the module against CRCCon afterwards |
| 🟡 | `0x27` | F$SetSys | Basic handler exists; full system configuration semantics remain open |
| ✅ | `0x28` | F$SRqMem | Allocation, rounding, process tracking and emulator test complete |
| ✅ | `0x29` | F$SRtMem | Explicit return and process cleanup complete |
| 🟡 | `0x2A` | F$IRQ | Kernel path exists; complete interrupt-device coverage remains open |
| 🟡 | `0x2B` | F$IOQu | Microware path exists; current Q9 compatibility is not fully verified |
| ❌ | `0x2C` | F$AProc | Not implemented |
| ❌ | `0x2D` | F$NProc | Not implemented |
| 🟡 | `0x2E` | F$VModul | Validation path exists; complete loader integration remains open |
| ❌ | `0x2F` | F$FindPD | Not implemented |
| 🟡 | `0x30` | F$AllPD | Basic descriptor allocation path exists; full OS-9 semantics remain open |
| 🟡 | `0x31` | F$RetPD | Basic descriptor return path exists; full validation remains open |
| 🟡 | `0x32` | F$SSvc | Service registration path exists; broader service semantics remain open |
| 🟡 | `0x33` | F$IODel | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x37` | F$GProcP | Basic process-property path exists; full property set remains open |
| ✅ | `0x38` | F$Move | Memory move path implemented |
| ❌ | `0x39` | F$AllRAM | Not implemented |
| ❌ | `0x3A` | F$Permit | SSM-owned; not implemented in Q9-OS |
| ❌ | `0x3B` | F$Protect | SSM-owned; not implemented in Q9-OS |
| ❌ | `0x3F` | F$AllTsk | SSM-owned; not implemented in Q9-OS |
| ❌ | `0x40` | F$DelTsk | SSM-owned; not implemented in Q9-OS |
| ❌ | `0x4B` | F$AllPrc | Not implemented |
| ❌ | `0x4C` | F$DelPrc | Not implemented |
| ❌ | `0x4E` | F$FModul | Not implemented |
| ❌ | `0x52` | F$SysDbg | Not implemented |
| ❌ | `0x53` | F$Event | Not implemented |
| ✅ | `0x54` | F$Gregor | Exact inverse of F$Julian, verified over every day from 1582-10-15 to 2200-12-31 |
| ❌ | `0x55` | F$SysID | Not implemented |
| ❌ | `0x56` | F$Alarm | Not implemented |
| ❌ | `0x57` | F$SigMask | Not implemented |
| 🟡 | `0x58` | F$ChkMem | Basic memory-check path exists; full protection semantics remain open |
| ❌ | `0x59` | F$UAcct | Not implemented |
| 🟡 | `0x5A` | F$CCtl | Handler/dispatch path exists; cache-control implementation remains open |
| ❌ | `0x5B` | F$GSPUMp | Not implemented |
| 🟡 | `0x5C` | F$SRqCMem | Shares the working memory-allocation path; color semantics remain limited |
| ❌ | `0x5D` | F$POSK | Not implemented |
| ❌ | `0x5E` | F$Panic | Diagnostic stub exists, but it is not a completed panic service |
| ❌ | `0x5F` | F$MBuf | Not implemented |
| ❌ | `0x60` | F$Trans | Not implemented |
| ❌ | `0x61` | F$FIRQ | Not implemented |
| ❌ | `0x62` | F$Sema | Not implemented |
| ❌ | `0x63` | F$SigReset | Not implemented |

## I$ input/output system calls

| Status | Code | Command | Current Q9-OS status |
|---|---:|---|---|
| 🔷 | `0x80` | I$Attach | Verified through Microware IOMan/RBF/CF with `iattachsvc`; Q9-native device semantics remain open |
| 🔷 | `0x81` | I$Detach | Verified through Microware IOMan/RBF/CF with `iattachsvc`; broader lifetime semantics remain open |
| 🟡 | `0x82` | I$Dup | Native path-table duplication, descriptor validation, and reference counting are implemented and verified in the emulator; full file-manager semantics remain open |
| ❌ | `0x83` | I$Create | No Q9-OS implementation; only the Microware path exists |
| 🟡 | `0x84` | I$Open | Minimal Q9-native console path is implemented; process-local `P$Path` publication is tested, while full pathname/device semantics remain open |
| ❌ | `0x85` | I$MakDir | No Q9-OS implementation; only the Microware path exists |
| 🟡 | `0x86` | I$ChgDir | Native data/execution directory storage is implemented and emulator-tested; device and file-manager resolution remain open |
| ❌ | `0x87` | I$Delete | No Q9-OS implementation; only the Microware path exists |
| 🟡 | `0x88` | I$Seek | Sequential Q9-native paths accept seek as a validated no-op; random-access semantics remain open |
| 🟡 | `0x89` | I$Read | Minimal Q9-native blocking console input with path and read-mode validation is implemented; full device semantics remain open |
| 🟡 | `0x8A` | I$Write | Minimal Q9-native console output with path and write-mode validation is implemented and emulator-tested; full device semantics remain open |
| 🟡 | `0x8B` | I$ReadLn | Minimal Q9-native blocking line input with path and read-mode validation is implemented; editing and device semantics remain open |
| 🟡 | `0x8C` | I$WritLn | Minimal Q9-native console output with path and write-mode validation is implemented and emulator-tested; full device semantics remain open |
| 🟡 | `0x8D` | I$GetStt | Native `SS_Opt` support now validates the current process path; other status codes remain open |
| 🟡 | `0x8E` | I$SetStt | Native `SS_Opt` support now validates the current process path; other status codes remain open |
| 🟡 | `0x8F` | I$Close | Native path-table removal, descriptor validation, and release with reference counting are implemented and verified in the emulator; full file-manager close semantics remain open |
| 🟡 | `0x92` | I$SGetSt | Minimal Q9-native `SS_Opt` support for direct system paths; permission, device-name, and file-manager semantics remain open |

## Current interpretation

The kernel bootstrap and the basic process/memory path are usable.  The
largest remaining groups are module loading, complete trap lifetime
management, IOMan/device integration, and the many supervisor services that
are still deliberately left as future work.

This file should be updated whenever a syscall gains a real implementation
or a new emulator regression test.
