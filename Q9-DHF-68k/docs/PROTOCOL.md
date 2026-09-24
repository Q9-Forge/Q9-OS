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

Opcode Übersicht
- 0x01: FM_CALL — allgemeiner FileManager-Aufruf (Open/Close/Read/Write/GetStat/SetStat/...) mit FuncID im Subcommand
- 0x02: FM_CTRL — Steuerbefehle (Init, Terminate, Sync/Flush)
- 0x03: FM_DESC — Descriptor/Config-Operationen (GetBasePath, SetBasePath)
- 0xFF: FM_PING — Test/Ping

Beispiel: GetStat
Request:
- Opcode=0x01 (FM_CALL)
- Subcommand=0x10 (FUNC_GETSTAT)
- Flags=0x00
- PayloadLength=2 + N
- Payload: [PathLength(2)][Path UTF-8 bytes]

Response:
- Status(1)
- DataLength(2)
- Data: serialized stat structure (mode, uid, gid, size, atime, mtime, ctime) in defined order, big-endian

SetStat
- Subcommand=0x11 (FUNC_SETSTAT)
- Payload: serialized target fields bitmap + corresponding values
- Driver validates fields, applies changes, returns Status

FuncID Mapping (erste Version)
- 0x10: FUNC_GETSTAT
- 0x11: FUNC_SETSTAT
- 0x20: FUNC_OPEN
- 0x21: FUNC_CLOSE
- 0x22: FUNC_READ
- 0x23: FUNC_WRITE
- 0x30: FUNC_OPENDIR
- 0x31: FUNC_READDIR
- 0x40: FUNC_MKDIR
- 0x41: FUNC_RMDIR
- 0x42: FUNC_UNLINK
- 0x43: FUNC_RENAME
- 0x50: FUNC_CHDIR
- 0x60: FUNC_STAT64 (extended)

Errors
- Status bytes non-zero map to errno-like codes; reserve 0x80..0xFF for driver-specific codes.

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
- 0x00 (1): BYTE Command
  - 0 = Idle
  - 1 = Create
  - 2 = Open
  - 3 = Seek
  - 4 = Read
  - 5 = Write
  - 6 = ReadLn
  - 7 = WriteLn
  - 8 = GetStt (GetStat)
  - 9 = SetStt (SetStat)
  - 10 = Close
  - 11 = Delete
  - 12 = MkDir
  - 13 = ChDir
  - 255 = Return/Response

- 0x01 (4): LONG A0 — Dateiname / Verzeichnisname (Offset/Pointer semantics: Manager kopiert Name in NAME-Bereich)
- 0x05 (4): LONG A1 — Buffer (Offset/Pointer semantics: Manager kopiert Buffer in BUFFER-Bereich)
- 0x09 (4): LONG D0 — Pfadnummer / Descriptor
- 0x0D (4): LONG D1 — Statuscode / Byte-Anzahl / Max-Länge / Seek-Offset (semantisch je nach Command)
- 0x11 (4): LONG D2 — Attribute / Flags

- 0x15 (256): BYTE ARRAY[256] NAME — Null-terminierter UTF-8 Name (falls Name kürzer, mit \0 auffüllen)
- 0x115 (512): BYTE ARRAY[512] BUFFER — Datapuffer für Read/Write/Line-Operationen

Gesamtgröße (empfohlen): 0x315 (789) Bytes

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

