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

## Where the remaining work is (2026-09-18)

Of the calls still open, the ones that are single, well-specified functions have
now been done. What remains falls into three groups, and the table marks which:

* **Whole subsystems**, not single calls: `F$Event` (event records with wait
  queues and signalling), `F$Alarm` (timed alarm queues on the tick handler),
  `F$Chain` (replacing the running program in place). Each needs its own design
  pass rather than an afternoon.
* **Blocked on one missing mechanism** — entering user code from the kernel:
  `F$Icpt` registers an intercept routine but nothing runs it, and `F$RTE`,
  `F$SigReset` and `F$STrap` all wait on that same piece. Building this one
  mechanism would unblock four calls at once, which makes it the highest-value
  next step in this area.
* **Not the kernel's to implement**: `F$SysDbg` calls a debugger that Q9-OS does
  not ship, `F$UAcct` is an extension point an OS9P2 module provides, and the
  SSM-owned calls need a memory management module. `F$Mem` and `F$SSpd` were
  withdrawn in real OS-9/68K itself.

A few calls have no page in the Technical Manual at all (`F$AllRAM`, `F$GBlkMp`,
`F$FModul`, `F$Sema`, `F$MBuf`, `F$POSK`, `F$GSPUMp`). Those stay untouched on
purpose: this kernel implements verified ABIs, and guessing one would break that
rule.

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
priority byte, and a running process is not preempted in the middle of its
current timeslice; waiting processes in the ready queue are reprioritized
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

`F$AProc` (`0x2C`), `F$GPrDBT` (`0x1F`), `F$AllPrc` (`0x4B`), `F$DelPrc`
(`0x4C`) and `F$SysID` (`0x55`) extend the process API, and `F$SSpd` (`0x0B`)
is marked withdrawn: the manual states plainly that "F$SSpd is currently not
implemented" in real OS-9/68K and points to lowering the priority instead.

`F$AProc` hands a descriptor to the existing scheduler. What the manual
describes — age the queue, set the new process' age to its priority, insert by
relative age — is exactly what `Q9K_SchedInsert` already does, and the ageing of
the others happens every tick anyway, so this call deliberately does not run a
second ageing pass over the same fields. Not implemented is the last sentence of
the description, immediate preemption when the new process outranks the running
one; that needs the same context switch out of trap state that `F$NProc` still
lacks.

`F$AllPrc` allocates and clears a descriptor. Without an MMU this is the direct
`F$AllPD` case the manual names, so the MMU-image step falls away entirely. One
detail comes from this kernel rather than the manual: a pool slot counts as
occupied only if its state byte holds one of the four known values, so a merely
cleared descriptor would be out of the free list yet invisible to the rest of
the kernel. The fresh descriptor therefore gets WAITING — exists, does not run.
`F$DelPrc` returns a descriptor to the pool and nothing else, as the manual
explicitly requires the caller to release other resources first.

`F$GPrDBT` assembles the pointer table from the process pool on demand — one
entry per slot, 0 for a free one, always 4 bytes per entry. This kernel keeps no
separate block table; the pool is the directory, and a second structure would
only be something that could drift out of step with it.

`F$SysID` is written entirely in assembler, without a C counterpart: it is
register setting plus two string copies, and the texts must be addressed
PC-relative because the kernel loads as a REENT module at a varying address. Its
values are honest rather than invented — OEM number and serial are zero because
Q9-OS is not a registered OEM and has no serial, the FPU identifier is zero
because this kernel performs no FPU detection, and the processor identification
comes from `Q9_D_MpuTyp`, the CPU type the boot ROM actually detected.

**A real defect came out of this, found only because a diagnostic dump was read
to the end for once.** The emulator run completed its whole marker sequence and
then reported `Vektor=14`, a format error. A comparison run without these calls
reported `Vektor=0`, which pinned it on the new code. Cause: the live test
handed a descriptor straight from `F$AllPrc` — cleared, therefore with
`SavedSP == 0` — to `F$AProc`. The scheduler picked it up on the next tick,
switched the stack to address 0 and executed `RTE` on an empty frame. `F$AProc`
now refuses a descriptor without a saved stack, so a process that cannot run can
no longer be made schedulable; a process becomes runnable by `F$Fork` or
`Q9K_ProcCreate` building its stack and frame first. Worth noting for future
sessions: a marker run terminates the emulator about a second after the marker,
which truncates the dump before the exception section — the failure had been
invisible for that reason. A run with an unreachable marker and a timeout writes
the dump in full.

The emulator regression extended to
`HOwWl\rLGSDCcergsIPpNnxMmUVuRYyDdBTJGjAaZzEQ@#`.

`F$FindPD` (`0x2F`) completes the descriptor trio whose other two parts were
already here: `F$AllPD` hands out a number, `F$RetPD` gives it back, `F$FindPD`
translates it into an address. It therefore uses the same DBT structure read out
of IOMan's own code and the same error codes; number 0 is invalid because offset
0 is the table header itself.

`F$SigMask` (`0x57`) is a counter rather than a switch, which is why the manual
speaks of "set/increment" and "decrement": nested critical sections can each
close and open the mask without the inner one taking the mask away from the
outer. It is kept in `P$SigLvl` (`$210`). This is not a stub — `F$Send` honours
it: a signal for a masked process is stored but the process is not woken, and
opening the mask delivers what accumulated. Exactly two signals break through,
as the manual states: `S$Kill` terminates regardless of the mask, and `S$Wake`
only ensures the process runs without queueing. Known limitation: `P$Signal`
holds exactly one pending signal, so several arriving during one masked section
overwrite each other — a real queue needs per-process memory this kernel does
not allocate yet. The common case, one signal during a short critical section,
is correct.

The emulator regression extended to
`HOwWl\rLGSDCcergsIPpNnxMmUVuRYyDdBTJGjAaZzEQFfK@#`, with a separate boot
reporting `Vektor=0`.

`F$Icpt` (`0x09`) installs a signal intercept routine. Both inputs have a fixed
place in the real process descriptor — `P$SigVec` (`$28`) and `P$SigDat`
(`$2C`) — and that is where they are stored. The registration is complete; what
is still missing is running the routine when a signal arrives, which needs the
signal path to redirect the user context, enter the routine with signals masked
and return through `F$RTE`. Until then a signal is stored in `P$Signal` as
before. The call is therefore marked partial rather than green: the half that
exists is real and holds exactly the fields a later delivery path takes its
jump address from. One assembler detail worth noting: the caller passes its data
area in `a6`, the very register this kernel uses for its own C runtime pointer,
so the incoming value has to be stored before switching — otherwise precisely
the value in question would be lost.

`F$Trans` (`0x60`) exists for systems with dual-ported memory, where the same
location appears under different addresses depending on the bus. The Q9 machine
has no second bus, so local and external address are the same and the
translation is the identity in both directions. That is not a placeholder but
the only correct answer for this machine: a caller passing the returned address
to hardware gets the one the hardware actually sees. Should Q9 ever gain a
second bus, the mapping belongs exactly here and no caller has to change.

The emulator regression extended to
`HOwWl\rLGSDCcergsIPpNnxMmUVuRYyDdBTJGjAaZzEQFfKNX@#`.

`F$DatMod` (`0x25`) creates a genuine data module in memory and enters it into
the module directory, so several processes can share it. The layout follows the
real one: a 48-byte header (the common part up to `M$Parity` — the fields from
`$30` on belong to the program header and a data module rightly lacks them),
then the cleared data area, the name, and the three CRC bytes, rounded up to the
allocation boundary. The proof needs no expected-value table: the created module
must pass every check this kernel applies to a module — sync word, size, header
parity, CRC against `CRCCon` — and be findable afterwards through `F$Link`, both
on the host and in the emulator.

Two deliberate deviations. The manual says the module is "initially created with
a CRC value of 0", but this kernel's directory validates the CRC on every entry,
so a module with CRC 0 would be rejected immediately; a valid CRC is therefore
written. That is harmless: anyone changing the data has to renew the CRC through
`F$SetCRC` anyway, and a valid initial value makes the module from the outset
what the description wants it to end up as. The memory colour (`d4.l`) is
ignored, as this kernel has a single memory area for a colour to choose between.

The emulator regression now reads
`HOwWl\rLGSDCcergsIPpNnxMmUVuRYyDdBTJGjAaZzEQFfKNX` … `W@#` — the gap is the
memory-trace diagnostic, which prints the allocation `F$DatMod` performs and
splits the marker string. Worth knowing when reading a run: filter lines
matching `^M <op> r=` before matching the sequence.

**The scheduler now honours a minimum priority**, which makes the manual's
replacement for the withdrawn `F$SSpd` actually work: a process whose priority
falls below the limit is no longer picked, while staying in the ready queue —
suspended is not forgotten, and raising the priority again makes it runnable
without any further bookkeeping. The limit is 0 by default, and at 0 no priority
can fall below it, so the boot behaviour is unchanged.

The limit deliberately lives in a Q9-owned scratch cell rather than in the
system global `D_MinPty`. The first attempt used `$55E` from `q9sysglob.h` — but
that entry is marked `[HANDBUCH]` there, meaning derived from documentation and
never verified against the binary. The live test showed why that matters: with
`$55E` as the source the system stalled during boot, IOMan never even filled its
device table. Something else lives at that address in this system, so the
scheduler read a random value as the limit and locked every process out. As long
as no foreign module sets a minimum priority — and a scan of all 455 Microware
modules found none that does — this is a purely internal notion, and an own
field is the honest answer rather than a guessed foreign address.

Related finding from the same scan, recorded so nobody repeats it: `F$Mem` is
called by no application at all, only by the Microware kernels themselves, which
never run here. `F$SSpd` appears in `cio` and `debug` purely as the library
wrapper `_os_sspd()` — present in the module, executed only if a program calls
it, and failing on real OS-9/68K just the same. `F$UAcct` appears nowhere.
`F$SysDbg`, by contrast, is genuinely used by `break`, `debug`, `sysgo` and
`pcf`, and the ROM does contain RomBug — see the note on that below.

**On F$SysDbg, a correction.** It was first marked withdrawn on the assumption
that Q9-OS ships no system debugger. That was wrong, and checking the ROM proved
it: **RomBug is in the boot ROM**, complete with breakpoint management (`b
<addr>`, `k`, `rst`) and a `NuRomBug:` prompt. It sits at ROM offset `0x2882`,
well before the first OS-9 module at `0x158AC`, so it belongs to the boot
monitor itself rather than to any module — in the mapped address space that puts
it around `$FE002882`.

The call is genuinely used, too: `break`, `debug`, `sysgo` and `pcf` all issue
it. And the mechanism is simple — real OS-9 keeps the debugger's entry point in
the system global `D_SysDbg` and F$SysDbg just jumps there. Our kernel even has
a placeholder for it, though on an invented address and filled by nobody.

What is missing is only the entry point: text location is not an entry point,
and finding the monitor's real one means disassembling the boot ROM. That is its
own task, not a side note — which is why this is now marked open rather than
withdrawn. Once the address is known, F$SysDbg is a handful of instructions.

**F$Load: one hypothesis tested and ruled out (2026-09-18).** After the boot
stack turned out to have been sitting inside the boot modules and corrupting
`cfide`, it was worth asking whether the long-standing `F$Load` hang had the
same cause — the symptom, RBF reading LBA 65 over and over, would fit corrupted
driver code just as well as a directory-advance bug. It does not: with the stack
fix in place, a normal boot still reaches "Hallo aus einem echten Programm!" and
then never emits the `l` marker that follows a successful `F$Load`, only
scheduler output. The load still does not return. The cause therefore lies
where it was originally suspected, in the external `F$Load`/RBF directory
advance, and not in memory corruption. Recorded so the cheap check is not
repeated.

`F$Alarm` (`0x56`) is a call with a function code in `d1.w`, and five of its
six functions are now implemented: **A$Set** (one signal after an interval),
**A$Cycle** (repeating), **A$Delete** (by ID, or all of the caller's own) and
the two absolute variants **A$AtJul** and **A$AtDate**. The codes come from the
real table in `funcs.a`, counted rather than guessed.

The three relative functions need no system clock — they count ticks, and the
tick handler, signal delivery and process lookup all existed already. Delivery
goes through the same `F$Send` path as any other signal, which means alarm
signals honour the signal mask for free: if the recipient is masked, the alarm
signal stays pending instead of being lost.

The absolute variants were held back in the previous round on the grounds that
there was no system clock. **That was wrong, and the note saying so has been
corrected**: the board carries an RTC72421 at `$FFFFD000`, which the emulator
mirrors from the host clock. `Q9K_RtcRead()` reads it, and each tick now
compares the stored target against it. A$AtJul takes the julian day number and
seconds after midnight directly; A$AtDate takes a calendar date in `d4.l` and a
time in `d3.l`, both field-coded the way `F$Time` returns them, and converts via
the `F$Julian` pair. A missed moment still fires — the description says "greater
than or equal", so an alarm whose time passed while the machine was busy is not
silently dropped. An impossible date or time is refused with `E$BPAddr` rather
than rounded.

**A$Reset is now implemented.** The Microware DPIO declaration
`_os_alarm_reset(alarm_id, signal_code, interval)` resolves the previously
missing semantics: the existing alarm keeps its ID while its signal and
relative interval are replaced. This also restarts a cyclic alarm and turns a
previously absolute alarm into a relative one. Invalid IDs, missing processes,
and zero intervals are rejected; the native regression suite covers all of
these paths.

**An address collision introduced by that same change, found immediately
after and fixed.** The F$Alarm scratch block sat at `$1A90`, directly behind a
table of 8 entries of 16 bytes (`$1A00`-`$1A7F`). Raising the entry size to 24
bytes for the absolute variants grew the table to `$1ABF` — slot 6 then landed
exactly on `_Func` and slot 7 on `_Date`. Nothing would have shown this until
the seventh simultaneous alarm, and then as silent corruption of the call
arguments mid-call. No test could see it either: the host test puts table and
scratch in two separate arrays and does not model the real address layout at
all.

The block now lives at `$1B00`, and a compile-time check in
`q9kernel_alarm.c` turns any future recurrence into a build error rather than
a runtime puzzle — verified by temporarily widening the stride and watching the
build fail. A sweep of the kernel's other table/scratch pairs (IRQ table at
`$1500`, memory-owner table at `$1710`) found no second instance.

On the time unit, an honest limitation: the description says an interval may be
given "in system clock ticks, or 256ths of a second" but does not say at that
point how the kernel tells the two apart. This version counts ticks throughout —
the unit the kernel's own tick handler already keeps — so a caller meaning
fractions of a second gets a different delay than expected. Documented rather
than quietly guessed.

The alarm table is a Q9-owned fixed pool of eight entries rather than the
original's two queues (`D_ALMQ1`/`D_ALMQ2`), which the boot code does create. The
inner node format of those is known only from disassembling the original kernel
and is of no use to us, since no foreign module reads our alarm nodes; the two
ring lists stay untouched.

The emulator regression now ends `…KNX` … `Wvtkbhiolw@#0003(*)>+TiyzYe` followed
by `C` from the chained-to module — the four digits being the kernel's own tick
counter.

**A contradiction between the two time sources, found while extending F$Alarm
and worth fixing before anything builds on it.** `F$Alarm`'s absolute variants
were first left out with the reason "there is no system clock". That reason was
wrong: `F$Time` reads a real RTC72421 at `$FFFFD000`, which the emulator provides
and which mirrors the host clock.

The real obstacle is worse. The two existing time sources disagree on the **date
format**. `F$Time` assembles its date as a *decimal number* (year × 10000 +
month × 100 + day, so 20260918), while `F$Julian` expects *fields* (year in the
high word, then a byte each for month and day). Feed one into the other and the
result is nonsense. Which reading of "yyyymmdd" is the real one cannot be decided
from the manual text alone — both fit the notation, and the `F$STime` remark
about "the month field in the date parameter" reads either way.

**A software clock, so that F$STime can mean anything.** `F$Time` used to
read the RTC72421 on every single call. That returns a correct time, but it
makes `F$STime` impossible: whatever a caller sets would be gone by the next
`F$Time`. The manual describes the opposite arrangement in as many words —
"The OS-9 kernel keeps track of the current date and time in software to make
clock modules small and simple" — so the clock now lives in the kernel
(`q9kernel_clock.c`) and the hardware only supplies the starting value.

`Q9K_ClockTick()` runs once per timer interrupt beside `Q9K_AlarmTick()` and
rolls ticks into seconds into days; `Q9K_ClockRead()` hands out the software
state, adopting the RTC reading on the very first read — the cold start with a
battery-backed clock that the manual describes. An unset clock deliberately
does not tick: counting up from zero would invent a moment rather than admit
an unknown one. Internally the clock holds a julian day number plus seconds
after midnight, the same form the absolute alarms compare against, and the
field encoding is converted only at the bridge.

`F$STime` sets it. The manual's battery-backed form is implemented too: a
month field of 0 means "take date and time from the hardware", and a year
supplied alongside it provides the century the RTC72421 does not store. What
is **not** implemented is the other half of the description — "starts the
system real-time clock ... and then linking the clock module". This kernel has
no loadable clock module; its timer interrupt has been running since boot.
There is nothing here to start, so the row stays 🟡 rather than claiming a
completeness the machine does not have.

One tolerance is recorded rather than tightened: a day that does not exist in
its month is not rejected but carried forward by the julian formula, so
31 September becomes 1 October. `Q9K_JulianFromDate` only checks month 1-12
and day 1-31, and the manual says of `F$STime` that "the date and time are not
checked for validity". Tightening that would be an invention beyond the
original; the host suite pins the actual behaviour so it cannot drift
unnoticed.

**THE TIMER PATH IS SOUND — and the three entries that stood here claiming
otherwise were all wrong, from one and the same measurement mistake.**

What this file said over three rounds: that the timer does not tick, that
`F$Sleep` never returns, that a process interrupted by a tick never runs again,
and finally that the context is not restored across a switch. **None of it is
true.** Measured properly, with the intercept test in place:

```
Wvtkbhiolw@#0003 ( * ) > + e … C      Vektor=0
                 │ │ │ │ │ │      └─ F$Chain into another module
                 │ │ │ │ │ └─ F$Chain refused an unknown module
                 │ │ │ │ └─ F$SigReset
                 │ │ │ └─ the software clock advanced across a 2 s sleep
                 │ │ └─ F$RTE returned into the main program behind F$Sleep
                 │ └─ the intercept routine ran, with the right signal code
                 └─ F$Icpt registered it
```

An alarm set for 5 ticks fires out of the timer interrupt, wakes the sleeping
process, its intercept routine runs and returns — and a separate two-second
sleep shows the clock moving. The timer ticks, sleeps end, alarms come due,
the clock advances, and a switched-out process resumes at exactly the PC it was
saved at (measured: `(56c94-07880)` then `(07880-56c94)`).

**The mistake, worth recording because it produced four wrong entries in a
row:** the console in the direct-attach test is flooded by `Q9K_TestProcA`, the
parent's diagnostic loop, which prints `A` continuously. Every reading was taken
from a ~40-character window after the marker string — and in that window the
flood had already buried everything that followed. Filtering the `A` out of the
full log shows the markers were there all along. A short window onto a noisy
channel is not a measurement; the check that settled it was
`''.join(c for c in log if c != 'A')`, which should have been the first thing
tried rather than the last.

The ready queue "being empty" and the process "vanishing" had the same origin:
the test program had simply run to completion, chained into `chaintgt` and
exited normally — which is why `Q9K_ProcExit` fired with status 0 on the test
program's own descriptor. That was never a fault.

**The intercept subsystem: F$Icpt now actually runs the routine.** Until now
`F$Icpt` could only register one. A signal was dropped into `P$Signal` and the
process woken; the registered routine never ran, so any program wanting to
handle a keyboard abort was reduced to polling.

The delivery needs no new memory. An interrupted process already has its whole
state on its own stack — register set plus exception frame, 68 bytes, with
`P$SavedSP` pointing at it. Delivery simply stacks a **second** such frame
below it, entering the intercept routine, and points `P$SavedSP` at that. `F$RTE`
is then almost nothing: move `P$SavedSP` back by one frame and the main program
continues as if nothing happened. This is exactly the stacking the manual
describes with "each time the intercept routine is called, 70 bytes are used on
the user's stack" — this version needs 68, its frame being precisely that size.

`F$RTE` re-enters the routine instead of returning when another signal is
pending ("until the queue is exhausted"), and refuses a call with no intercept
open — unstacking a frame there would cost the main program its own state and
resume it at an arbitrary address. `F$SigReset` discards the context instead,
for a routine left via `longjmp()`.

Delivery happens only to a process that is **not** currently running: whoever
is running has their state in the CPU registers, so `P$SavedSP` is stale and a
frame built from it would resume them anywhere. For a running process the
signal is stored as before and delivered at the next switch.

The emulator proves the whole round trip: an alarm set for 5 ticks fires from
the timer interrupt, wakes the sleeping process, its routine runs with the
right signal code, `F$RTE` returns into the main program behind the `F$Sleep`,
and execution continues normally from there.

**F$Chain** (`0x05`) runs a new program without creating a process — "similar
to a Fork command followed by an Exit", but in the same process, with the open
paths untouched. That last part is the point of the call: a program can hand
over to another one with the same input and output.

Two things dictate the shape of the implementation, and both were worth the
care:

**The failure case must leave the caller intact.** If the module is not found
or memory runs short, the old process simply continues and gets a carry. So
everything new is acquired first — module linked, memory taken, parameters
copied, frame built — and nothing old is torn down until that has succeeded. An
implementation that cleans up first leaves a process with no program behind on
every failure, and failure (a typo in the name) is the more likely case. The
live test checks exactly this: it chains to a module that does not exist and
then carries on running.

**The old memory block must not be freed while the kernel is still running in
it.** The caller's stack lives in that block, and this code runs on it. The
release therefore sits in its own function that the assembly side calls only
after switching to the stack in the new block. Doing both in one go would work
almost every time — until the next allocation overwrote the freed stack, and
then unreproducibly.

Not implemented is loading from disk when the module is not already in memory.
The manual lists it as the second step, but `F$Load` still hangs in the external
RBF path (see `0x01`), so a module that is not resident reports `E$MNF` — the
same limit F$Fork has, and it disappears with `F$Load`.

Two mistakes during this work are worth recording. First, three constants
(`Q9K_INITIAL_SR` and all three module-header offsets) were *guessed* rather
than copied from `q9kernel_firstproc.c`; the host suite went green because the
test shared the same wrong values, and only the emulator showed it, as a
`Vektor=4` crash. A test that shares the assumption under test proves nothing.
Second, a run that appeared to hang in `Q9K_AllocMem` turned out to be a
60-second timeout cutting into the memory tracer's very long output — the
allocation had succeeded all along. Both were diagnosed from the raw console
bytes rather than guessed at.

**A method worth recording, because it unblocks the rest of this list.** Several
calls are marked here as not implemented with the reason "the manual gives no
ABI" — `F$Chain`, `F$NProc`, `F$FModul`, `F$Panic`, `F$Event`, `F$FIRQ`,
`F$AllRAM` and others appear in Appendix D only as cross-references or index
entries, without input and output registers.

That obstacle is largely gone. Microware's own C bindings carry the calling
convention in compiled form, and `MWOS/OS9/68020/LIB/os_lib.l` holds a binding
for most of the table. Disassembling the few instructions before each `trap #0`
gives the register usage directly, which is how `F$Sema` below was settled. The
recipe: find the `trap #0` (`4E40`, with the call code relocated to `0000` in
the object), read the register moves immediately preceding it, and cross-check
any structure offsets against the matching header in `MWOS/OS9/SRC/DEFS`.

Checked one by one rather than assumed — the library covers `F$Chain`,
`F$NProc`, `F$Panic`, `F$Event`, `F$FIRQ`, `F$GSPUMp`, `F$SysDbg`, `F$STrap`,
`F$RTE`, `F$SigReset` and `F$DFork`, but **not** `F$FModul`, `F$AllRAM`,
`F$POSK` or `F$MBuf`. Those four have neither a manual entry nor a binding, and
for them the only remaining source is the original kernel itself.

Where a manual entry does exist it stays the primary source; the library
settles what the manual leaves open, the way the original kernel settled the
date format.

**F$NProc** (`0x2D`) is "Start Next Process": no input, no return, system
state. The manual is unusually explicit about the one thing that shapes the
whole implementation — "The process calling NProc should already be in one of
the system's process queues. If it is not, the calling process becomes unknown
to the system even though the process descriptor still exists." So this
implementation deliberately does **not** re-queue the caller. It takes the next
process off the ready list and switches into it; whoever called without having
queued themselves first is gone. That is the semantics, not an omission, and it
is why the call is system-state: it exists for drivers and managers that keep
their own queues.

The register set is still saved and `SavedSP` still written, because the normal
case is a caller that *is* on some queue and will be resumed later. Skipping
that would leave it holding a stale stack pointer — the same trap as `F$Sema`
and `Ev$Wait`.

When no process is ready the manual has OS-9 wait for an interrupt and look
again. This kernel goes through `F$Panic(K$Idle)` instead — the same path
`F$Sema` and `Ev$Wait` take, and the one the manual describes under `F$Panic`
itself: "F$Panic is called only when the kernel believes there are no processes
remaining to be executed." A panic service installed through `F$SSvc` is
therefore exactly where a system would intervene.

Proving it needs a second process, because the call by definition never returns
to whoever made it. `nproctgt` is forked, announces itself with `n`, and calls
`F$NProc`; the parent sleeps briefly so the child actually runs, then carries on
to its remaining tests. Three observations together make the proof: `n` appears
(the child ran), `N` never appears (the call did not return), and the parent's
later markers appear (the switch really happened rather than hanging).

**A limit reached while adding it, worth recording because it will come back.**
The kernel has grown past the 16-bit reach of `bsr`. Adding two dispatch entries
in `q9kernel_cinit.c` was enough to push the distance from `q9kernel_entry.a`
into the last modules of the link list over 32 KiB. The assembler accepts it
silently; `l68` then reports `operand size error` **without naming the place**,
which makes it an expensive thing to diagnose. Two rules follow, and both are
now written into the source: new C files go at the **end** of the link list in
`build.sh`, so existing distances do not move; and calls into the last modules
go through a pointer cell that `q9kernel_cinit.c` fills at boot
(`Q9K_StrapImplPtr` and the six below it), the same pattern `F$Event` already
used. Seven calls — `F$STrap`, `F$RTE`, `F$SigReset`, `F$Chain` (twice),
`F$Sema` (twice) — were converted, which buys room for several more calls before
the next one has to be.

**F$Event** (`0x53`) is the event system — "multiple-value semaphores", as the
manual puts it. Unlike a semaphore an event carries a counter, and a waiter
names the range of values it wants to be woken in. The manual's own example is
a printer pool: initial value = number of printers, wait increment −1, signal
increment +1.

All twelve functions are implemented: create, delete, link, unlink, read, set,
set-relative, signal, pulse, info, wait and wait-relative. The live test walks
a full life cycle — create with initial value 5, signal (giving 6), read it
back, `Ev$Wait` over the range 0…10 and `Ev$WaitR` over −5…+5, read again to
see both wait increments applied, unlink, delete, and confirm the event is then
gone.

`Ev$Wait` splits in two, exactly as `F$Sema` does and for the same reason. The
C side first decides whether the value lies in the requested range: if it does,
the call returns at once with the value **before** the wait increment, and the
event moves on by that increment — nobody waits. If it does not, the assembler
side saves the caller's register set on its own stack, updates `SavedSP`, and
only **then** calls back into C to enqueue it and pick the next process.
Enqueuing before the save would leave the waiter holding a stale stack pointer,
and the wake-up would land nowhere.

`Ev$Signl` walks that queue and wakes the waiters whose range now contains the
value, adding each one's wait increment as it goes; with the function word's
top bit set it wakes all of them instead of just the first. A waiter whose
range does not match is skipped rather than blocking the ones behind it.

Both paths are proven on the machine. The blocking one needs a second process
to do the signalling, so the live test forks one: `evsigtgt` links to the event
the main program created, waits a moment, and sets its value into the range the
main program is waiting for. The proof is the order of the markers — the child
writes `p` and `S`, the woken parent writes `~`, and `~` appears after `S`, so
the parent really did stand still.

That test found a defect the host suite could not see. A woken process returns
through `movem`/`rte` and takes `d1` from its saved register set, so the value
has to be written *into that saved set* before it is made runnable — the
non-blocking path gets `d1` straight from the bridge and never needed it. Until
that was added, the caller came back with whatever `d1` happened to hold when it
went to sleep. The host suite now checks it too (`Q9K_SetFrameReg` at the d1
slot of the frame `SavedSP` points at, carrying the value before the wait
increment), but it was the two-process test on real hardware that produced it.

A second lesson from the same hunt, and this one is about how the hunt itself
went wrong. When the first diagnostic markers came out incomplete, the
explanation reached for was that consecutive writes to the DUART drop
characters without a TXRDY wait. That was wrong: the missing markers were code
that genuinely never ran, because the test was calling function code 9
(`Ev$Pulse`, which restores the old value and therefore wakes nobody) instead
of 10 (`Ev$Set`). Adding the TXRDY wait changed nothing; fixing the function
code produced every marker at once. A later run on `chaintgt` pointed the other
way again — a lone TXRDY wait loop there produced no output at all until a plain
unguarded write preceded it.

So the honest state of it: character loss on this console has not been
demonstrated, the TXRDY question is unresolved, and the real lesson is the
older one — a missing marker means "this code did not run" until something
proves otherwise. `chaintgt` now writes a `c` the moment it starts, before
touching the stack or any subroutine, so that question can be answered directly
instead of inferred.

The function codes come from `MWOS/OS9/SRC/DEFS/event.a` (counted off as
`do.b 1` from zero), the 32-byte record layout from the same file, and the
error codes from `funcs.a` — whose counting base sits `$4C` away from the real
values, a shift confirmed against two codes this kernel already knows
(`E$UnkSvc` `$D0`, `E$BPAddr` `$D2`). For "table full" the manual names
`E$EvFull`, which `funcs.a` does not contain; `E$Full` (`$F8`) is reported
instead and that is noted in the source rather than inventing a code.

Two things the host suite pinned down that would otherwise have been silent
faults. **The event ID is not the table index**: it carries a serial number in
the high word, because a bare index is reused the moment an event is deleted,
and an old ID would then quietly address a different event. And **the saturating
arithmetic reads the sign bit explicitly** instead of casting: on a 64-bit test
host `$80000010` is a positive number, so the downward saturation never
triggered — the code now computes on 32 bits regardless of the compiler's word
width.

**An assembler lesson that also casts doubt on an earlier explanation.** The
handler could not reach its C function: `bsr` was out of range (16-bit
PC-relative), and an absolute `jsr` from this relocatable module never
returned. The fix is a pointer cell that C fills at boot. Notably, `F$STrap`
hit the *same symptom* — a C call from assembly that did not return — and that
was attributed there to the compiler's stack-check prologue in an exception
context. That explanation may well have been wrong too; the lookup was rewritten
in assembler and works, so the question is moot in practice, but the note there
should not be read as settled.

**F$STrap** (`0x0E`) lets a process catch its own program-error exceptions —
bus error, illegal instruction, zero divide and the rest. The call takes a
table of word pairs terminated by -1, exactly as the manual prints it:

```
ExcpTbl  dc.w  T_TRAPV,OvfError-*-4
         dc.w  T_CHK,CHKError-*-4
         dc.w  -1
```

The first word is the exception as a **byte offset** into the CPU vector table
(`T_IllIns` = 16, i.e. vector 4 — from `sysglob.a`, where the vectors are laid
out with `org 0` and one long each). The second is a **PC-relative** distance:
`Routine-*-4` means the routine sits four bytes past the start of the pair plus
that distance, which is what makes the table relocatable — it lives inside a
program module loaded at an arbitrary address. Both of those are easy to get
silently wrong, and either mistake only shows up once an exception actually
fires; the host suite pins them with 26 checks.

Handlers go into `P$Except`, ten longs in the process descriptor, indexed by
vector minus two. The field address was derived rather than guessed:
`process.a` lists the descriptor fields without gaps, and two of them are
independently known in this kernel (`P$SigVec` `$28`, `P$PModul` `$38`).
Counting from there puts `P$Except` at `$3C` and `P$ExStk` at `$64` — both
anchors match, so the derivation between them holds.

The dispatch is deliberately tiny: when the exception hits, the 68030 frame is
already on the stack with SR, PC and the format/vector word. The kernel only
replaces the PC in that frame with the handler address, and the `rte` that
follows jumps there with the register set untouched — the handler sees exactly
the state in which the fault occurred.

**It works end to end — after two wrong explanations along the way, both worth
recording.**

The live test registers a handler for Illegal Instruction, then executes one
deliberately. The emulator prints `T` (registered) and `i` (the process's own
handler ran), and the test carries on normally afterwards.

The first wrong explanation blamed the vector installation. Measured, the
vectors are fine:

```
VBR = 00000400   slot 4 = 000075da   Q9K_ExcTrap = 000075da
```

The second was closer but still not the cause. A marker on `Q9K_ExcTrap`'s
first instruction showed it **re-entering itself endlessly** — an unbroken run
of `X`. That is real, and it is a pre-existing fault: to produce a backtrace the
diagnostic section deliberately reads *past* the exception frame (`8(sp)`
through `16(sp)`, plus a 64-byte stack copy), and when the faulting process's
stack ends there, that read faults in turn. So the handler lookup was moved
ahead of any diagnostic, where it belongs — a handler asked for afterwards
would never be reached.

But the jump still did not arrive, and the marker trail said why: `X`, then the
vector number `4`, and then nothing. **The lookup was calling into C**, and the
answer was in `Q9K_ExcTrap`'s own header comment all along — that handler is
written in assembler precisely because the compiler puts a stack-check prologue
in front of every C function, and out of an exception context it does not hold.
The first version reintroduced exactly what the comment warns against.

The lookup is now a dozen assembler instructions: vector number from the frame,
minus two, range-checked, indexed into `P$Except` of the running process. The C
function stays for the host test, with a note that the kernel does not call it.

**The re-entry fault now has a guard**, though its proof is partial and that is
worth stating precisely.

`Q9K_ExcTrap` counts its own nesting. The first entry runs the full diagnostic
as before; a second one — which can only mean the diagnostic tripped over its
own read — prints a single `R` straight to the DUART and stops. No `bsr`, no
further reads: whatever state could still be fetched there belongs to the
failed save, not to the original fault, and that one is already in
`Q9K_ExcInfo_*` as far as the first round got.

**Both cases are now measured, including the one that produced the endless
chain.** Registering a handler for a *different* vector (`T_TRAPV`) and then
executing an Illegal Instruction gives it no handler at all — and the result is
a single `E`, no `R`, no chain. The diagnostic completes, the faulting process
stops, and the rest of the system keeps running.

So the exception path now behaves correctly in both directions: with a
registered handler the process handles its own fault (`i`), without one the
kernel reports and halts that process alone (`E`).

An earlier note here said the Illegal-Instruction case "simply falls silent"
and left it unresolved. That was a broken measurement, not a finding: the test
jumped past the F$STrap block with a `bra` that skipped considerably more than
intended, so the exception under observation was never the one being reached.
Registering for a different vector isolates the case without moving any code.

Two further limits, both stated rather than hidden: the separate exception
stack from `(a0)` is recorded in `P$ExStk` but not switched to — `(a0) = 0`,
the documented normal case, works fully. And there is no way back to the code
that faulted; the manual offers none, and a handler is expected to exit or
`longjmp()` away.

**F$Sema** (`0x62`), and an ABI that had to be recovered. The manual describes
semaphores at length — the structure, the states, the P/V operation codes — but
never says which registers the call expects. That was settled by disassembling
Microware's own library, `MWOS/OS9/68020/LIB/os_lib.l`: `_os_sema_p` at `$9310`
and `_os_sema_v` at `$9386` give the whole convention — `d0.l` = pointer to the
semaphore, `d1.w` = 1 for P or 2 for V. The structure offsets visible in that
code (`s_value` +0, `s_lock` +4, the `s_flags` byte tested at +`$1f`, `s_sync`
+32) match `semaphore.h` exactly, which independently confirms header and code
belong together.

The same disassembly explains why this call is small: **the uncontended case
never reaches the kernel at all.** User code takes the semaphore itself with
`tas` and returns; only when that fails does it call F$Sema(P), and only then
must the kernel suspend the caller. Likewise it releases the semaphore itself
and calls F$Sema(V) only when `s_lock` shows someone is still waiting. And
after waking, the user code retries the `tas` on its own — so the kernel never
has to promise that the woken process actually gets the semaphore, only that it
gets woken. The manual says exactly this: the first process in the queue "is
activated and retries the reserve operation".

Waiters are queued in the semaphore itself and chained through the same
descriptor field the ready queue uses. A waiting process is deliberately **not**
put on the sleep list: both chain through that one field, and a process in both
at once would destroy both lists. The original has a separate state for this
(`'p'`, per `process.a`), and so does this implementation.

P is split across two calls into C, which matters: the queueing happens only
*after* the assembly side has saved the caller's register set and updated
`SavedSP`. Queueing first would leave the waiting process with a stale stack
pointer, and the wake-up would land nowhere.

The live test covers V and both refusals. **P is host-tested only** — it blocks
the caller by design until another process releases, so proving it in the
emulator needs a second process. That gap is named rather than papered over
with a marker that would suggest otherwise.

**F$GBlkMp** (`0x19`) is the status report that `mfree` and similar tools use:
the addresses and sizes of the free RAM blocks, copied into the caller's buffer
as `{address, size}` pairs and closed with a null entry, plus the minimum
allocation size, the fragment count, the total RAM found at startup and the
currently free total. The manual is emphatic that the reported blocks must
never be used directly — `F$SRqMem` is for that — so this call only copies out.

It needed one thing the kernel did not have: the total RAM size. The boot ROM
passes it in `d0` on entry, but that value only ever became the stack pointer.
It is now kept in `Q9K_RamSize`, written **after** the zero-fill (which clears
the first 32K and would otherwise erase it) and taken from `sp`, which at that
point still holds exactly the value the boot ROM supplied. Taking it from `sp`
rather than rescuing a register across the zero-fill avoids a second assumption
about the boot ROM's register usage.

Two limits are stated rather than papered over. The reported total is the
machine's RAM, not the part available to the arena — kernel, boot chain and
stack live inside it. And the arena does not yet coalesce adjacent blocks on
free, so the fragment count can exceed the number of genuinely separate free
regions; the free total is still correct, but "fragments" here means "list
nodes".

Unlike the bit-map calls below, this one **is** proven on the machine: the live
test gets `d0 = 16`, which is this allocator's granularity and therefore proof
that the kernel's own handler ran rather than a foreign one.

**The three bit-map calls, and what measuring them turned up.** `F$SchBit`
(`0x12`), `F$AllBit` (`0x13`) and `F$DelBit` (`0x14`) form one cycle: search a
free run, allocate it, hand it back. RBF manages disk cluster allocation with
them. All three now have a kernel-native implementation in
`q9kernel_bitmap.c`, covered by 49 host checks.

`F$SchBit`'s failure case is unusual and is reproduced exactly: when no run is
large enough it sets carry but returns **no error code** — `d0.w`/`d1.w` carry
the start and size of the largest run found instead, so the caller can decide
whether less will do. Reading `d1.w` as an error number there yields silent
nonsense, which is why it is spelled out at length in the source.

Two things were measured rather than assumed, and both are recorded because
they cost time:

- **These calls currently do nothing on the running system.** `F$AllBit` on a
  zeroed buffer reports success and leaves the buffer at zero. A control probe
  with a deliberately unassigned call code returned carry correctly, so the
  measurement setup was sound.
- **The reason is that the loaded Microware IOMan claims them.** Its F$SSvc
  marker table (`$1400` + call code) reads 1 for both `$12` and `$13`, which
  matches the manual ("The IOMan module implements F$SchBit"). While that IOMan
  is loaded its handler runs, not this kernel's — and, as measured, without
  touching the bit map.

So the new implementation is the kernel's own path for running **without** a
foreign IOMan, which is where this project is headed. It cannot be proven in
the emulator as things stand: a live test there would measure IOMan's handler,
not this one. The rows therefore stay 🟡 with the reason named, and no live
marker was added — a green marker that measures something other than what it
claims is worse than none.

The bit order is an open point, stated as such: the manual only says "bit
numbers range from 0 to n-1". This implementation puts bit 0 at the most
significant bit of byte 0. Deciding it needs IOMan's `F$AllBit` handler
disassembled, the same way the date format was settled. Until then all three
calls share one order, so any caller going exclusively through them gets
consistent results; the host suite pins the order so it cannot drift.

**The question has since been decided, by disassembly rather than by taste.**
Microware's own kernel `aker000b` unpacks the date at `0x2316` with
`move.b d1,d3` / `asr.l #8,d1` / `move.b d1,d2` / `asr.l #8,d1` — byte-wise
field extraction, not decimal division. The same routine contains `cmpi.w
#$62e` (1582, the Gregorian changeover) and `muls.w #$5b5` (1461, the four-year
cycle), which identifies it as the julian-day conversion beyond doubt. So
"yyyymmdd" means **fields**: `F$Julian` was right and `F$Time` was wrong.

`Q9K_SysFTime` now builds its date the same way (`swap` for the year, then
`lsl.w #8` / `or.l` for month and day). With both sides agreeing, the absolute
alarm variants were implemented in the same pass; see the `F$Alarm` section
above. This was a genuine defect, not a cosmetic one: any program feeding
`F$Time`'s output into `F$Julian` — the ordinary way to compute a date
difference — would have got nonsense back.

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
| 🟡 | `0x05` | F$Chain | Replaces the caller's program in place, emulator-verified in both the success and the refusal path; loading from disk when the module is not in memory waits on `F$Load` |
| ✅ | `0x06` | F$Exit | Process exit, primary memory and tracked user allocations released |
| ⛔ | `0x07` | F$Mem | Withdrawn in real OS-9/68K ("F$Mem is no longer available. Use F$SRqMem instead."); deliberately not implemented |
| ✅ | `0x08` | F$Send | Signal path implemented and tested at kernel level |
| ✅ | `0x09` | F$Icpt | Registers the routine and really **runs** it on delivery, by stacking a second process frame; emulator-verified end to end (alarm → routine → `F$RTE`) |
| ✅ | `0x0A` | F$Sleep | Sleeps end on time and on an early signal; emulator-verified with a 2 s sleep across which the clock advanced |
| ⛔ | `0x0B` | F$SSpd | "F$SSpd is currently not implemented" in real OS-9/68K; the manual points to lowering the priority instead, and that route now works here (see the scheduler note) |
| ✅ | `0x0C` | F$ID | Process identity path implemented |
| ✅ | `0x0D` | F$SPrior | Priority change on a live process descriptor, implemented and verified in the emulator; see the note on scheduler re-queue timing below |
| ✅ | `0x0E` | F$STrap | Registers per-process handlers in P$Except and really dispatches into them; emulator-verified end to end on a deliberate Illegal Instruction |
| 🟡 | `0x0F` | F$PErr | Microware path exists; current Q9 compatibility is not fully verified |
| ✅ | `0x10` | F$PrsNam | Path-name parsing implemented |
| ✅ | `0x11` | F$CmpNam | Name comparison with `?`/`*` wildcards and case folding, implemented and verified in the emulator |
| 🟡 | `0x12` | F$SchBit | Kernel-native implementation, host-tested, including the unusual carry case; the loaded Microware IOMan claims the call via F$SSvc, so it is not reachable in the emulator — see the note |
| 🟡 | `0x13` | F$AllBit | Kernel-native implementation, host-tested; same IOMan claim as `0x12` |
| 🟡 | `0x14` | F$DelBit | Kernel-native implementation, host-tested; same IOMan claim as `0x12` |
| ✅ | `0x15` | F$Time | Reads the kernel's software clock, which takes its starting value from the RTC72421 at `$FFFFD000` |
| 🟡 | `0x16` | F$STime | Sets the software clock, including the battery-backed form (month field 0); the clock module it would otherwise link does not exist on this machine |
| ✅ | `0x17` | F$CRC | 24-bit module CRC, accumulated across calls; verified against a real module and the documented CRCCon constant |
| ✅ | `0x18` | F$GPrDsc | Read-only copy of a process descriptor, length-capped at the descriptor size |
| ✅ | `0x19` | F$GBlkMp | Reports the free-memory map from the kernel's own free list, with the fragment count and totals; emulator-verified |
| ✅ | `0x1A` | F$GModDr | Copies the module directory out in whole entries; the format is this kernel's own, as the manual allows |
| ✅ | `0x1B` | F$CpyMem | Copy with owner-PID validation; no address translation is needed while all processes share one flat address space |
| ✅ | `0x1C` | F$SUser | Changes the caller's own group/user ID in the process descriptor; only the documented "user 0.0 may change freely" case is implemented |
| ✅ | `0x1D` | F$UnLoad | Same lookup rule as F$Link and the same counter as F$UnLink, keyed by module name |
| ✅ | `0x1E` | F$RTE | Unstacks the intercept frame and re-enters the routine when another signal is pending; emulator-verified |
| ✅ | `0x1F` | F$GPrDBT | Pointer table assembled from the process pool, one entry per slot, 0 for a free one |
| ✅ | `0x20` | F$Julian | Packed date/time to OS-9 Julian day; zero point anchored on JULBASE from time.h, 1582 changeover implemented |
| ✅ | `0x21` | F$TLink | Links trap modules, initializes their state, supports `namePtr=0` removal, and releases references/owned memory on process exit |
| ❌ | `0x22` | F$DFork | Not implemented |
| ❌ | `0x23` | F$DExec | Not implemented |
| ❌ | `0x24` | F$DExit | Not implemented |
| ✅ | `0x25` | F$DatMod | Creates a real data module: header, cleared data area, name, parity and CRC, entered into the module directory |
| ✅ | `0x26` | F$SetCRC | Updates header parity and module CRC; verified by re-checking the module against CRCCon afterwards |
| 🟡 | `0x27` | F$SetSys | Known csl memory-increment variable `$7C` is now persistent and host-tested; the broader system-variable catalogue remains open |
| ✅ | `0x28` | F$SRqMem | Allocation, rounding, process tracking and emulator test complete |
| ✅ | `0x29` | F$SRtMem | Explicit return and process cleanup complete |
| 🟡 | `0x2A` | F$IRQ | Kernel path exists; complete interrupt-device coverage remains open |
| 🟡 | `0x2B` | F$IOQu | Microware path exists; current Q9 compatibility is not fully verified |
| ✅ | `0x2C` | F$AProc | Makes a runnable descriptor schedulable; refuses one without a saved stack; immediate preemption still open |
| ✅ | `0x2D` | F$NProc | Takes the next process off the ready list and switches into it. The caller is deliberately not re-queued — that is the manual's own semantics. Emulator-verified with a forked process that calls it and correctly never comes back |
| 🟡 | `0x2E` | F$VModul | Header parity/CRC validation, size-bound handling, return-buffer ABI, and host tests are complete; full external loader integration remains open |
| ✅ | `0x2F` | F$FindPD | Path/process number to descriptor address, same DBT structure as F$AllPD/F$RetPD |
| 🟡 | `0x30` | F$AllPD | DBT allocation, descriptor clearing, host tests, and the real IOMan lifecycle probe are implemented; emulator confirmation remains open |
| 🟡 | `0x31` | F$RetPD | DBT return with descriptor-number validation, host tests, and the real IOMan lifecycle probe are implemented; emulator confirmation remains open |
| 🟡 | `0x32` | F$SSvc | Service-table registration, SysTrap routing, data pointers, kernel-slot protection, and empty-table handling are host-tested; live external-service dispatch remains open |
| 🟡 | `0x33` | F$IODel | Microware path exists; current Q9 compatibility is not fully verified |
| ✅ | `0x37` | F$GProcP | PID-to-process-descriptor lookup implemented and host-tested; broader process-property APIs are tracked separately |
| ✅ | `0x38` | F$Move | Memory move path implemented |
| ❌ | `0x39` | F$AllRAM | Not implemented |
| 🟡 | `0x3A` | F$Permit | Q9 flat-address-space compatibility handler is wired and succeeds; per-process MMU permission maps remain open |
| 🟡 | `0x3B` | F$Protect | Q9 flat-address-space compatibility handler is wired and succeeds; denying access requires the future MMU layer |
| 🟡 | `0x3F` | F$AllTsk | Q9 flat-address-space compatibility handler is wired for supervisor calls; hardware task-image setup remains open |
| 🟡 | `0x40` | F$DelTsk | Q9 flat-address-space compatibility handler is wired for user and supervisor calls; task-image release remains open |
| ✅ | `0x4B` | F$AllPrc | Allocates and clears a process descriptor; without an MMU this is the documented direct F$AllPD case |
| ✅ | `0x4C` | F$DelPrc | Returns a descriptor to the pool only, as documented; other resources stay the caller's duty |
| 🟡 | `0x4E` | F$FModul | Side-effect-free module-directory lookup is implemented with type/language filtering, highest-revision selection, result registers, and name-pointer advancement; emulator regression remains to be added |
| ❌ | `0x52` | F$SysDbg | RomBug **is** present in the boot ROM; what is missing is the entry point for `D_SysDbg` — see the note below |
| ✅ | `0x53` | F$Event | All twelve functions — create, delete, link, unlink, read, set, set-relative, signal, pulse, info, wait, wait-relative. Emulator-verified on both wait paths, the blocking one with a forked second process doing the signalling |
| ✅ | `0x54` | F$Gregor | Exact inverse of F$Julian, verified over every day from 1582-10-15 to 2200-12-31 |
| ✅ | `0x55` | F$SysID | Version and copyright text plus processor identification; OEM and serial are honestly zero |
| 🟡 | `0x56` | F$Alarm | A$Set, A$Cycle, A$Delete, A$AtJul, A$AtDate and A$Reset are implemented; A$Set is emulator-verified, while the complete alarm matrix remains host-tested |
| ✅ | `0x57` | F$SigMask | Nesting-safe signal mask counter; F$Send honours it, S$Kill and S$Wake break through |
| 🟡 | `0x58` | F$ChkMem | Flat-address-space handler rejects 32-bit range wraparound; MMU/permission validation remains open |
| ⛔ | `0x59` | F$UAcct | A user-defined call an OS9P2 module claims through F$SSvc, not a kernel service; what is missing is the cold-start scan of `M$Extens`, not this call |
| 🟡 | `0x5A` | F$CCtl | Handler is wired and live dispatch-verified; on the cacheless Q9 target every control request is a successful no-op, while real CACR/cache maintenance remains hardware-specific |
| 🟡 | `0x5B` | F$GSPUMp | Flat-address-space compatibility handler is wired; processor/task-map reporting remains hardware-specific and open |
| 🟡 | `0x5C` | F$SRqCMem | Shares the working memory-allocation path; color semantics remain limited |
| ❌ | `0x5D` | F$POSK | Not implemented |
| 🟡 | `0x5E` | F$Panic | Default panic path is implemented: emits the panic code and halts; optional OS9P2-installed service hook remains open |
| ❌ | `0x5F` | F$MBuf | Not implemented |
| ✅ | `0x60` | F$Trans | Identity mapping, which is the correct answer on a machine without a second bus |
| ❌ | `0x61` | F$FIRQ | Not implemented |
| 🟡 | `0x62` | F$Sema | P and V implemented against an ABI recovered by disassembly; V and the refusals are emulator-verified, P is host-tested only (it blocks by design) |
| ✅ | `0x63` | F$SigReset | Discards the saved intercept context, for a routine left via `longjmp()`; emulator-verified |

## I$ input/output system calls

The I$ dispatch is built into the 68k kernel: `q9kernel_cinit.c` installs the
native handlers in both `D_UsrDis` and `D_SysDis`, and `q9kernel_entry.a`
contains the corresponding callcode trampolines.  The external Microware
IOMan is also connected and reaches these system-dispatch entries; this is
verified end-to-end for the I/O path through RBF/CF and for the current IOMan
boot path.  The live module-address classification assigns every I$ entry
point below to Microware IOMan; the native userland regression additionally
passes real create/open/read/write/mkdir/delete/close round-trips on FAT16.
“Native” below means Q9's own minimal console/path implementation, whereas
the Microware route supplies the real device and filesystem semantics.  A
diamond therefore marks the working Microware route even when the independent
Q9-native replacement is still incomplete.  The current external `F$Load`
trace still stalls in RBF directory/position advancement, which is tracked
separately from I$ dispatch ownership.

| Status | Code | Command | Current Q9-OS status |
|---|---:|---|---|
| 🔷 | `0x80` | I$Attach | Verified through Microware IOMan/RBF/CF with `iattachsvc`; Q9-native device semantics remain open |
| 🔷 | `0x81` | I$Detach | Verified through Microware IOMan/RBF/CF with `iattachsvc`; broader lifetime semantics remain open |
| 🔷 | `0x82` | I$Dup | Microware IOMan dispatch is present; Q9-native path-table duplication is implemented and emulator-verified, while native file-manager parity remains open |
| 🔷 | `0x83` | I$Create | Microware IOMan/RBF/CF path is present and covered by the create/write round-trip; Q9-native implementation remains open |
| 🔷 | `0x84` | I$Open | Microware IOMan/RBF/CF path is present and live-traced; Q9-native console/path handling remains minimal |
| 🔷 | `0x85` | I$MakDir | Microware IOMan/RBF/CF path is present and covered by the FAT16 mkdir regression; Q9-native implementation remains open |
| 🔷 | `0x86` | I$ChgDir | Microware IOMan path is present; Q9-native data/execution directory storage is emulator-tested, while device resolution remains open |
| 🔷 | `0x87` | I$Delete | Microware IOMan/RBF/CF path is present and covered by the FAT16 delete regression; Q9-native implementation remains open |
| 🔷 | `0x88` | I$Seek | Microware IOMan path is present; Q9-native paths retain the requested position, while native backing-file repositioning remains open |
| 🔷 | `0x89` | I$Read | Microware IOMan/RBF/CF path is live-traced and covered by read-back tests; Q9-native console input remains minimal |
| 🔷 | `0x8A` | I$Write | Microware IOMan/RBF/CF path is covered by the create/write round-trip; Q9-native console output remains minimal |
| 🔷 | `0x8B` | I$ReadLn | Microware IOMan path is live-traced; Q9-native line input remains minimal |
| 🔷 | `0x8C` | I$WritLn | Microware IOMan path is covered by userland output tests; Q9-native console output remains minimal |
| 🔷 | `0x8D` | I$GetStt | Microware IOMan path is present; Q9-native `SS_Opt`/`SS_Ready` support covers only direct system paths |
| 🔷 | `0x8E` | I$SetStt | Microware IOMan path is present; Q9-native `SS_Opt` support does not yet cover all status codes |
| 🔷 | `0x8F` | I$Close | Microware IOMan/RBF/CF path is covered by close/read-back tests; Q9-native path cleanup is implemented and verified |
| 🔷 | `0x92` | I$SGetSt | Microware IOMan path is present; Q9-native `SS_Opt`/`SS_Ready` support remains limited |

## Current interpretation

The kernel bootstrap and the basic process/memory path are usable.  The
largest remaining groups are module loading, complete trap lifetime
management, IOMan/device integration, and the many supervisor services that
are still deliberately left as future work.

This file should be updated whenever a syscall gains a real implementation
or a new emulator regression test.
