# Q9-DHF Mini-Protocol (Manager -> Driver)

Zweck
- Übergabe von Filemanager-Aufrufen vom Q9-DHF Manager an den Treiber (dhfdrv-68k).
- Ein kompaktes Set/GetStat-basiertes Kommandomodell, das Funktions-IDs und strukturierte Payloads kapselt.

Design-Grundsätze
- Einfaches, klar strukturiertes Binary-ähnliches Kommandoformat, aber als Text/JSON in ersten Implementationen möglich.
- Jede Nachricht enthält: Opcode (1 byte), Subcommand/FuncID (1 byte), Flags (1 byte), PayloadLength (2 bytes, big endian), Payload (variable).
- Antworten: Status (1 byte: 0 OK, non-zero Fehler), DataLength (2 bytes), Data.
- Für SetStat/GetStat wird das Subcommand als Funktions-Selector genutzt.
- Pfadnamen UTF-8, null-terminated im Payload, bei relativem Pfad vor Konfinezungsprüfung (absolut vs. relativ) durch Manager.

Commands
- Single flat command list (WORD values). No nested opcodes.
- 0x0000: Idle
- 0x0001 - 0x000D: Manager commands (Create, Open, Seek, Read, Write, ReadLn, WriteLn, GetStt, SetStt, Close, Delete, MkDir, ChDir)
- 0x0020 - 0x002F: Control commands
  - 0x0021: Descriptor Init
  - 0x0022: Test / Ping
- 0x0080 - 0x00FF: Return/Response codes
  - 0x0080: OK (no error)
  - 0x0081-0x00FF: command-specific error codes

Parameter semantics
- param[0..4] are 5 32-bit values with command-dependent meaning. Typical usages:
  - For operations involving strings/buffers provided by the user program (in emulator memory):
    - param[0] = pointer/address to null-terminated name/path in emulator memory
    - param[1] = pointer/address to buffer in emulator memory (read/write data)
    - param[2] = descriptor/handle (if applicable)
    - param[3] = length / maxlen / seek-offset (signed)
    - param[4] = attributes / flags
  - Manager MUST populate these params with the *addresses* that point into emulator memory (not copy the data into the shared area). The hardware simulator reads the emulator memory at those addresses and performs host operations.

Errors
- result_code maps to host errno-like values; 0 = OK. Command-specific return codes are in 0x0081..0x00FF.

Example: Write flow
1) Manager (in emulated 68k) wants to write 100 bytes from buffer at emu address 0x2000 to file handle 5.
2) Manager sets shared area:
   - param[0] = 0 (no name)
   - param[1] = 0x2000 (pointer in emulator memory to data)
   - param[2] = 5      (file descriptor/handle)
   - param[3] = 100    (length)
   - param[4] = 0      (flags)
   - seq incremented
   - command = 0x0005  (WRITE)
3) Hardware Simulator polls the shared area, sees command=0x0005 with new seq, reads param[1] (emu pointer 0x2000) via emulator memory API, copies 100 bytes from emulator memory to a host buffer, performs write(5, hostbuf, 100) on the host, sets result_code=0 or errno, result_len=bytes_written, and sets command=0x0080 (OK) or 0x0081..0x00FF on error.
4) Manager polls the shared area, sees response (command >= 0x0080), reads result_code/result_len and continues.



Security / Confinement
- Manager MUST normalize and resolve paths and enforce confinement: resolved_path must start with basepath.
- Driver SHOULD double-check (realpath) to prevent symlink escape, but manager is primary enforcer.
- chdir/cd must only affect per-descriptor CWD tracked by manager; global CWD not allowed.

Extension
- Future: binary-packed payloads, TLV encoding for SetStat fields, async call IDs for long ops.

Notes aus RBF-Analyse
- RBF verwendet GetStat/SetStat hooks. Das Protokoll kodiert spezielle GetStat subcommands, so dass der Treiber SetStat/Call intern auf die richtigen Host-APIs mappt (chmod/chown/utimes/...).
- Bei Bedarf können bestimmte RBF-internen control-codes als FM_CTRL Subcommands abgebildet werden.


## Universeller Kommando-Bereich (Shared Command Area)

Für Operationen mit Pfadnamen oder großen Buffern wird ein einheitlicher Kommando-Bereich definiert, der vom Manager gefüllt und vom Treiber gelesen werden kann. Die Struktur ist "packed" (keine Einfüge-Padding-Bytes); alle LONG-Felder sind 32-bit big-endian.

Layout (Offsets, bytes):
- 0x00 (1): uint8_t version
- 0x01 (1): uint8_t status
- 0x02 (2): uint16_t command (WORD, big-endian)
  - 0 = Idle
  - 0x0001..0x000D: Manager commands (1..13 = Create..ChDir)
  - 0x0020..0x002F: Control commands
    - 0x0021: Descriptor Init
    - 0x0022: Test / Ping
  - 0x0080..0x00FF: Return/Response codes (0x0080 = OK, 0x008x command-specific errors)

- 0x04 (4): uint32_t seq (BE)
- 0x08 (20): uint32_t param[5] - five 32-bit parameters (A0/A1/D0/D1/D2 semantics)
- 0x1C (4): uint32_t result_code - errno-like (0 == OK)
- 0x20 (4): uint32_t result_len  - valid bytes in external buffer or TLV area
- 0x24 (4): uint32_t crc32       - CRC32 over header+payload (optional, 0 = disabled)

- NAME and BUFFER are not embedded by default; manager should provide an external data block when param[0]/param[1] indicate presence.
- Following: optional TLV extension area for complex payloads (SetStat, extended attributes)

Gesamtgröße (empfohlen): header + external areas (variable)

Hinweise zur Nutzung
- Manager füllt die Felder und setzt das COMMAND-Byte auf den gewünschten Wert; Treiber verarbeitet und schreibt Antwort in die gleichen Felder (z. B. COMMAND=255 für Return) und setzt D1/D2 bzw. Status-Felder.
- Bei Pfad- oder Namenfeldern: Manager kopiert den Pfad in NAME und schreibt A0 so, dass Treiber weiß, dass NAME zu verwenden ist (z. B. A0 = 0x00000001 als Flag). Alternativ kann A0 als Offset in die Shared-Area (0x15) interpretiert werden.
- Für Read/Write: Manager setzt D1 = Max-Länge / Anzahl-der-Bytes; Treiber schreibt tatsächliche gelesene/geschriebene Länge zurück in D1.
- SetStat/GetStat: relevante Felder (z. B. Attribute in D2, weitere Werte im BUFFER) werden an vereinbarten Offsets serialisiert; Versionierung kann über ein spezielles Flag in D2 erfolgen.
- Sicherheitsanforderung: Manager darf vor dem Setzen in den Shared-Area Pfad-Normalisierung und Confinement durchführen; Treiber muss zusätzliche Prüfung (realpath, symlink check) durchführen, bevor Host-Operationen ausgeführt werden.

Zukünftige Schritte
- Definition eines präzisen Binary-Serialisierungsformats für SetStat-Payloads (TLV), einschließlich Feld-IDs und Längen.
- Mapping-Regeln (wie A0/A1 interpretiert werden) klar dokumentieren (Offset-vs-Flag vs. Pointer) und Version-Feld hinzufügen.
- Implementierung eines SDK-Helper (C-Struct) zur Arbeit mit der Shared-Area.

OS-9 Manager Memory allocation pattern
- If the manager needs per-path or per-handle private buffers in the emulated process memory, use OS-9 system pool allocations so the memory is visible to the emulator and stable across calls:
  - Request system memory from the manager using F$SRqMem (exposed via a C wrapper e.g. _os9_f_srqmem()). The kernel may round the size up to page boundaries.
  - Store the returned pointer in the path descriptor (path->pd_opt) or another manager-provided slot so the driver and manager can share it across calls.
  - When the path/handle is closed, free the memory with F$SRtMem (e.g. _os9_f_srtmem()).

Example (OS-9 68k C sketch):

```c
#include <types.h>
#include <io.h>

// Command structure used by manager in emu memory
typedef struct {
    int command_id;
    int param1;
    char buffer[256];
} MyCmdStruct;

// Open: allocate system memory for per-path command area
error_code _sysio_open(path_desc *path, ...) {
    u_int32 size = sizeof(MyCmdStruct);
    MyCmdStruct *cmd_ptr;
    error_code err;

    // 1. request system memory (F$SRqMem)
    err = _os9_f_srqmem(&size, (void **)&cmd_ptr);
    if (err != 0) return err;

    // 2. initialize and store in path descriptor
    cmd_ptr->command_id = 0;
    cmd_ptr->param1 = 100;
    path->pd_opt = (char *)cmd_ptr;
    return 0;
}

// Close: release system memory
error_code _sysio_close(path_desc *path) {
    u_int32 size = sizeof(MyCmdStruct);
    MyCmdStruct *cmd_ptr = (MyCmdStruct *)path->pd_opt;
    if (cmd_ptr) _os9_f_srtmem(size, cmd_ptr);
    return 0;
}
```

- This approach lets the manager provide stable emulator-memory buffers whose addresses can be passed to the hardware simulator via the shared command area (param[0..4]).



