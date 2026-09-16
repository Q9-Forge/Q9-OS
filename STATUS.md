# Q9-OS status

This file describes the current implementation status of the OS-9 system
calls in Q9-OS.  The status refers to the Q9-OS kernel and its current
IOMan integration, not to the original OS-9 implementation.

| Status | Meaning |
|---|---|
| ✅ | Implemented and exercised in the current kernel test path |
| 🟡 | Partially implemented, limited, or not yet completely verified |
| 🔷 | Works with Microware components in a current emulator test; Q9-native replacement is still open |
| ❌ | Not implemented or still routed to the unimplemented-service stub |

The call-code names and the complete call-code set are based on
`Q9-KERNEL/.os9-original/SYSCALL_MODULE_MAP.md`.  A green status does not
mean that every OS-9 corner case or every hardware device is already
supported.

## Latest kernel verification (2026-09-16)

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

## F$ system calls

| Status | Code | Command | Current Q9-OS status |
|---|---:|---|---|
| ✅ | `0x00` | F$Link | Kernel module lookup/link path implemented and exercised |
| 🟡 | `0x01` | F$Load | Microware path exists, but no complete current Q9 emulator verification |
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
| 🟡 | `0x80` | I$Attach | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x81` | I$Detach | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x82` | I$Dup | Minimal Q9-native path duplication is implemented and exercised during boot; full path-lifetime semantics remain open |
| 🟡 | `0x83` | I$Create | Microware path exists; no complete current Q9 emulator verification |
| 🔷 | `0x84` | I$Open | Microware console path exercised; Q9-native support remains open |
| 🟡 | `0x85` | I$MakDir | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x86` | I$ChgDir | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x87` | I$Delete | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x88` | I$Seek | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x89` | I$Read | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x8A` | I$Write | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x8B` | I$ReadLn | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x8C` | I$WritLn | Minimal Q9-native console output is implemented and exercised by `hellosvc`; full device semantics remain open |
| 🟡 | `0x8D` | I$GetStt | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x8E` | I$SetStt | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x8F` | I$Close | Microware path exists; no complete current Q9 emulator verification |
| 🟡 | `0x92` | I$SGetSt | Microware path exists; no complete current Q9 emulator verification |

## Current interpretation

The kernel bootstrap and the basic process/memory path are usable.  The
largest remaining groups are module loading, complete trap lifetime
management, IOMan/device integration, and the many supervisor services that
are still deliberately left as future work.

This file should be updated whenever a syscall gains a real implementation
or a new emulator regression test.
