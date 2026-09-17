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
| ❌ | `0x07` | F$Mem | Not implemented |
| ✅ | `0x08` | F$Send | Signal path implemented and tested at kernel level |
| ❌ | `0x09` | F$Icpt | Not implemented |
| 🟡 | `0x0A` | F$Sleep | Scheduler sleep path exists; complete timing coverage remains open |
| ❌ | `0x0B` | F$SSpd | Not implemented |
| ✅ | `0x0C` | F$ID | Process identity path implemented |
| ❌ | `0x0D` | F$SPrior | Not implemented |
| ❌ | `0x0E` | F$STrap | Not implemented |
| 🟡 | `0x0F` | F$PErr | Microware path exists; current Q9 compatibility is not fully verified |
| ✅ | `0x10` | F$PrsNam | Path-name parsing implemented |
| ❌ | `0x11` | F$CmpNam | Not implemented |
| 🟡 | `0x12` | F$SchBit | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x13` | F$AllBit | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x14` | F$DelBit | Microware path exists; current Q9 compatibility is not fully verified |
| 🟡 | `0x15` | F$Time | Handler exists; clock source and full validation remain open |
| ❌ | `0x16` | F$STime | Not implemented |
| ❌ | `0x17` | F$CRC | Not implemented |
| ❌ | `0x18` | F$GPrDsc | Not implemented |
| ❌ | `0x19` | F$GBlkMp | Not implemented |
| ❌ | `0x1A` | F$GModDr | Not implemented |
| ❌ | `0x1B` | F$CpyMem | No registered syscall implementation |
| ❌ | `0x1C` | F$SUser | Not implemented |
| ❌ | `0x1D` | F$UnLoad | Not implemented |
| ❌ | `0x1E` | F$RTE | Not implemented |
| ❌ | `0x1F` | F$GPrDBT | Not implemented |
| ❌ | `0x20` | F$Julian | Not implemented |
| 🟡 | `0x21` | F$TLink | Trap linking works; unlink and complete lifetime handling remain open |
| ❌ | `0x22` | F$DFork | Not implemented |
| ❌ | `0x23` | F$DExec | Not implemented |
| ❌ | `0x24` | F$DExit | Not implemented |
| ❌ | `0x25` | F$DatMod | Not implemented |
| ❌ | `0x26` | F$SetCRC | Not implemented |
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
| ❌ | `0x54` | F$Gregor | Not implemented |
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
