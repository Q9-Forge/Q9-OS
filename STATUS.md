# Q9-OS status

This file describes the current implementation status of the OS-9 system
calls in Q9-OS.  The status refers to the Q9-OS kernel and its current
IOMan integration, not to the original OS-9 implementation.

| Status | Meaning |
|---|---|
| ✅ | Implemented and exercised in the current kernel test path |
| 🟡 | Partially implemented, limited, or not yet completely verified |
| ❌ | Not implemented or still routed to the unimplemented-service stub |

The call-code names and the complete call-code set are based on
`Q9-KERNEL/.os9-original/SYSCALL_MODULE_MAP.md`.  A green status does not
mean that every OS-9 corner case or every hardware device is already
supported.

## F$ system calls

| Status | Code | Command | Current Q9-OS status |
|---|---:|---|---|
| ✅ | `0x00` | F$Link | Kernel module lookup/link path implemented and exercised |
| 🟡 | `0x01` | F$Load | IOMan path exists, full module-loader coverage remains open |
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
| 🟡 | `0x0F` | F$PErr | IOMan-owned; Q9-OS integration not complete |
| ✅ | `0x10` | F$PrsNam | Path-name parsing implemented |
| ❌ | `0x11` | F$CmpNam | Not implemented |
| 🟡 | `0x12` | F$SchBit | IOMan-owned; not complete |
| 🟡 | `0x13` | F$AllBit | IOMan-owned; not complete |
| 🟡 | `0x14` | F$DelBit | IOMan-owned; not complete |
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
| 🟡 | `0x2B` | F$IOQu | IOMan-owned; not complete |
| ❌ | `0x2C` | F$AProc | Not implemented |
| ❌ | `0x2D` | F$NProc | Not implemented |
| 🟡 | `0x2E` | F$VModul | Validation path exists; complete loader integration remains open |
| ❌ | `0x2F` | F$FindPD | Not implemented |
| 🟡 | `0x30` | F$AllPD | Basic descriptor allocation path exists; full OS-9 semantics remain open |
| 🟡 | `0x31` | F$RetPD | Basic descriptor return path exists; full validation remains open |
| 🟡 | `0x32` | F$SSvc | Service registration path exists; broader service semantics remain open |
| ❌ | `0x33` | F$IODel | IOMan-owned; not complete |
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
| 🟡 | `0x80` | I$Attach | IOMan/device-manager path is present but incomplete |
| 🟡 | `0x81` | I$Detach | IOMan/device-manager path is present but incomplete |
| 🟡 | `0x82` | I$Dup | IOMan path is present but incomplete |
| 🟡 | `0x83` | I$Create | IOMan path is present but incomplete |
| 🟡 | `0x84` | I$Open | Kernel bridge and console path implemented; broader device support remains open |
| 🟡 | `0x85` | I$MakDir | IOMan path is present but incomplete |
| 🟡 | `0x86` | I$ChgDir | IOMan path is present but incomplete |
| 🟡 | `0x87` | I$Delete | IOMan path is present but incomplete |
| 🟡 | `0x88` | I$Seek | IOMan path is present but incomplete |
| 🟡 | `0x89` | I$Read | IOMan path is present but incomplete |
| 🟡 | `0x8A` | I$Write | IOMan path is present but incomplete |
| 🟡 | `0x8B` | I$ReadLn | IOMan path is present but incomplete |
| 🟡 | `0x8C` | I$WritLn | IOMan path is present but incomplete |
| 🟡 | `0x8D` | I$GetStt | IOMan path is present but incomplete |
| 🟡 | `0x8E` | I$SetStt | IOMan path is present but incomplete |
| 🟡 | `0x8F` | I$Close | IOMan path is present but incomplete |
| 🟡 | `0x92` | I$SGetSt | IOMan path is present but incomplete |

## Current interpretation

The kernel bootstrap and the basic process/memory path are usable.  The
largest remaining groups are module loading, complete trap lifetime
management, IOMan/device integration, and the many supervisor services that
are still deliberately left as future work.

This file should be updated whenever a syscall gains a real implementation
or a new emulator regression test.
