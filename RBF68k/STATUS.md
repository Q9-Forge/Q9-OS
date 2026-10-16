# RBF68k – Status und Aufgabenliste

**Projekt:** Eigener Random Block File Manager (RBF) für OS-9/68K  
**Pfad:** `/Volumes/SSD1TB/projects/Q9-Forge/Q9-SDK/SRC/SYSTEM/Q9-OS/RBF68k`  
**Letzte Aktualisierung:** 2026-10-15  

Dieses Dokument wird fortlaufend nach Erledigung von Teilaufgaben aktualisiert.

---

## 1. Übersicht der Phasen

```
Phase 1: Vorbereitung & Referenzen (Erledigt)
   │
Phase 2: Modul-Gerüst & Dispatcher (In Arbeit)
   │
Phase 3: Dateisystem-Strukturen & Treiber-Anbindung (LSN0, FD, LSN-Mapping)
   │
Phase 4: Lese-Pfad / Read-Only Meilenstein (Open, Seek, Read, Close, GetStt)
   │
Phase 5: Verzeichnis-Navigation (ChgDir, Dir-Scan)
   │
Phase 6: Schreib-Pfad / Read-Write Meilenstein (Bitmap, Create, MakDir, Write, Delete, SetStt)
   │
Phase 7: Puffer-Caching & Performance
   │
Phase 8: Integration & Testsuite (Q9-Flux-Boottest, Regressionstests)
```

---

## 2. Detaillierte Aufgabenliste

### Phase 1: Arbeitsumgebung & Referenzen bereitstellen
- [x] Verzeichnisstruktur unter `Q9-OS/RBF68k` angelegt (`defs/`, `docs/`, `ref_bin/`, `ref_rea/`, `driver/`, `descriptor/`, `src/`, `tools/`).
- [x] Offizielle OS-9 Definitionsdateien kopiert: `rbf.h`, `rbfdev.a`, `rbfstat.a`, `rbfdesc.a`, `rbf.des`, `rbf.lb8`, `q9defs.d`.
- [x] Referenz-Binaries gesichert: `ref_bin/rbf_os9_68000.bin` (9638 Byte Originalmodul) und QEMU-RAM-Extraktion.
- [x] Disassembler-Toolchain eingerichtet: `dis68` auf dem Mac gebaut und vollständiges Disassembly mit Symbolen nach `docs/rbf_disassembly.a` erzeugt.
- [x] C-Referenzarchitektur: Decompilierte Quellen aus REA (`ref_rea/rbf_01.c` bis `rbf_08.c`) für Bitmap-, Cache- und FD-Algorithmen hinterlegt.
- [x] Treiber & Deskriptoren: `driver/cfide_v42.a`, `driver/cfidef.a` sowie Beispiel-Deskriptoren hinterlegt.

---

### Phase 2: Modul-Gerüst & Dispatcher (M$Exec)
- [x] **Modul-Header definieren**:
  - Typ: `FlMgr` ($0D), Objekt-Code ($01), Re-entrant ($80) + Supervisor-Mode ($20) = Attr `$A0`.
  - Modulname: `rbf` (sauberer Lookup für `I$Attach`).
- [x] **13-Slot Sprungtabelle (`M$Exec`)**:
  - Exakte relative Offsets (`Label - RbfEnt`) zur Tabellenbasis (ab Byte `$3C`).
  - Alle 13 I$-Callcodes ($83 bis $8F: Create, Open, MakDir, ChgDir, Delete, Seek, Read, Write, ReadLn, WritLn, GetStt, SetStt, Close) initial verdrahtet.
- [x] **ABI-Rahmen**:
  - Fehlerbehandlung: Fehlercode in `d1.w` und Carry-Bit im CCR setzen (`Rbf_UnkSvc`); Erfolg: Carry löschen.
  - `I$Close`: `PD_COUNT(a1)` Prüfung implementiert (nur schließen wenn Count == 0).
- [x] **Build-Skript verifizieren**:
  - `src/build.sh` assembliert mit `qr68k` und linkt mit `ql68k` zu `rbf.mod` (Header, Parity, Checksumme und relative Sprünge binär verifiziert).

---

### Phase 3: Dateisystem-Strukturen & Treiber-Anbindung
- [x] **Treiber-Aufruf (`rbf_drv.a`)**:
  - `Rbf_CallDriver`: Anbindung an den Low-Level-Treiber (z. B. `cfide`) über die Gerätetabelle (`PD_DEV` $\rightarrow$ `DT_DRIV`/`DT_STAT`), Treiber-Locking via `V_BUSY` und Sprung über `M$Exec` Entry-Table.
  - `Rbf_ReadSector` & `Rbf_WriteSector`: Lesen und Schreiben einzelner 512-Byte-Sektoren.
- [x] **LSN0 (Identification Sector) Parser (`rbf_fs.a`)**:
  - `Rbf_ParseLSN0`: Extraktion und Validierung von `DD_TOT` (Gesamtsektoren), `DD_LSNSize` (Sektorgröße, Standard 512), `DD_DIR` (Root-Verzeichnis FD LSN), `DD_MapLSN` (Bitmap-Startsektor) und `DD_BIT` (Cluster-Verhältnis).
- [x] **File Descriptor (FD) Parser & LSN-Mapping (`rbf_fs.a`)**:
  - `Rbf_MapOffsetToLSN`: Rechnet einen beliebigen logischen Dateioffset (z. B. `PD_CP`) anhand der bis zu 48 Segment-Einträge (`FDSL_A`/`FDSL_B`) des FDs in die exakte physische Sektornummer (LSN), den Sektor-Byte-Offset und die verbleibenden Sektoren um; EOF-Prüfung gegen `FD_SIZ`.
  - `Rbf_CompareFilename`: OS-9 Dateinamen-Vergleich (High-Bit-Terminierung `$80` oder NUL).

---

### Phase 4: Lese-Pfad (Read-Only Meilenstein)
- [x] **`I$Open` (`rbf_ops.a: Rbf_Open`)**:
  - Sektor 0 einlesen, Root-Verzeichnis FD lokalisieren.
  - Verzeichnis-Sektoren durchsuchen, Dateinamen matchen (`Rbf_CompareFilename`).
  - Ziel-FD einlesen, Rechte prüfen, Pfaddeskriptor initialisieren (`PD_FD`, `PD_DFD`, `PD_SIZ`, `PD_ATT`, `PD_CP=0`, `PD_DVT`).
- [x] **`I$Seek` (`rbf_ops.a: Rbf_Seek`)**:
  - Dateizeiger `PD_CP` absolut setzen.
- [x] **`I$Read` / `I$ReadLn` (`rbf_ops.a: Rbf_ReadCommon`)**:
  - Dynamisches Segment-Mapping (`Rbf_MapOffsetToLSN`), Block-weises Einlesen über den Treiber, Kopieren in User-Puffer, Zeilenende-Erkennung bei `ReadLn`.
- [x] **`I$GetStt` (`rbf_ops.a: Rbf_GetStt`)**:
  - `SS_Size` (Dateigröße in `REG_D2`), `SS_Pos` (aktuelle Position in `REG_D2`), `SS_EOF` (Dateiende prüfen), `SS_Ready` (Datenbereit-Status `d1=1`), `SS_FD` (FD-Sektor für `attr` auslesen).
- [x] **`I$Close` (`rbf_68k.a: Rbf_Close`)**:
  - `PD_COUNT`-Nutzungszähler prüfen, bei 0 sauber abschließen.
- [x] **Verifikation**:
  - Host-Side Unit-Test [`tools/test_rbf_readonly.py`](tools/test_rbf_readonly.py) verifiziert alle Read-Only Operationen erfolgreich gegen das reale Datenträger-Image `q9data-rbf.img`.

---

### Phase 5: Verzeichnis-Operationen
- [x] **`I$ChgDir` (`rbf_ops.a: Rbf_ChgDir`)**:
  - Arbeitsverzeichnis (`P$DIO + DIO_FM`) und Ausführungsverzeichnis (`P$DIO + DIO_EXEC + DIO_FM`) im Prozessdeskriptor `a4` setzen, Prüfung auf `DIR_`-Attribut.
- [x] **Verzeichnis-Iteration**:
  - Vollständige Unterstützung von 32-Byte-Verzeichniseinträgen (`DIR_NM` + `DIR_FD`) über `I$Read` und `I$GetStt SS_Size` für Utilities wie `dir` und `list`.

---

### Phase 6: Schreib-Pfad (Read-Write Meilenstein)
- [x] **Sektor-Allokation / Bitmap (`rbf_write.a: Rbf_AllocCluster`)**:
  - Freie Cluster in der Bitmap suchen (Bit-Test abwärts 7..0), belegen (`bset`), Bitmap auf Disk zurückschreiben.
- [x] **`I$Create` / `I$MakDir` (`rbf_write.a: Rbf_Create`, `Rbf_MakDir`)**:
  - Neuen FD-Sektor allozieren, initialisieren (`FD_ATT`, `FD_LNK=1`, `FD_SIZ=0`), Eintrag in Verzeichnis anlegen (`DIR_NM` + `DIR_FD`), Pfaddeskriptor initialisieren.
- [x] **`I$Write` / `I$WritLn` (`rbf_write.a: Rbf_Write`, `Rbf_WritLn`)**:
  - Bei Bedarf neue Segmente allozieren (`Rbf_AllocCluster`), Segmente in FD eintragen, Sektoren über Treiber schreiben, Dateigröße im FD aktualisieren.
- [x] **`I$Delete` (`rbf_write.a: Rbf_Delete`)**:
  - Verzeichniseintrag im Elternverzeichnis nullen, FD invalidieren.
- [x] **`I$SetStt` (`rbf_write.a: Rbf_SetStt`)**:
  - `SS_Attr` (Rechte im FD ändern), `SS_Size` (Dateigröße anpassen).
- [x] **Verifikation**:
  - Unit-Test [`tools/test_rbf_readwrite.py`](tools/test_rbf_readwrite.py) verifiziert Create, Write, Read-Back und Delete erfolgreich gegen ein Duplikat des echten Datenträgers.

---

### Phase 7: Puffer-Caching & Optimierung
- [x] **Sektor-Pufferung**:
  - Pufferung von LSN0-, FD- und Verzeichnisblöcken (`Rbf_LSN0Buf`, `Rbf_FDBuf`, `Rbf_SectorBuf`, `Rbf_CompBuf`) vollständig deterministisch und statisch ohne dynamische Allokation (`malloc`/`free`).
- [x] **Größen-Optimierung**:
  - Modulgröße auf 5.992 Bytes optimiert (unter 6.000-Byte-Limit), sodass `OS9Boot` mit 310.782 Bytes exakt in die 6 vorallokierten Disksegmente (310.784 Bytes) ohne Bitmap-Überlauf passt.

---

### Phase 8: Integration & Testsuite
- [x] Einbinden in Test-Bootbaum (`OS9Boot` mit eigenem `rbf` via `tools/mk_rbfboot.py`).
- [x] Start in `Q9-Flux` mit CompactFlash-Image (`local_images/work/rbf68k-session/OS9SYS.hda`).
- [x] Bootloader $\rightarrow$ Kernel $\rightarrow$ `init` $\rightarrow$ `rtclock` $\rightarrow$ `sysgo` $\rightarrow$ `SYS/startup` $\rightarrow$ `mshell` Login erfolgreich.
- [x] Funktionstests mit Standard-Utilities vollständig bestanden:
  - `dir`, `dir -e`, `dir CMDS`, `dir .`, `dir ..`, `dir ...` (relative Pfade & Multi-Dot)
  - `pd`, `pd -x`, `dirs`, `chd`, `chx`
  - `list startup`, `list /dd/TESTDIR/TESTFILE`
  - `makdir /dd/TESTDIR`
  - `copy /dd/startup /dd/TESTDIR/TESTFILE` (inkl. `SS_FD` File-Descriptor-Kopie)
  - `del /dd/TESTDIR/TESTFILE`
  - `deldir /dd/TESTDIR`
  - `logout`

---

## 3. Änderungsprotokoll

| Datum | Phase / Aufgabe | Beschreibung | Status |
|---|---|---|---|
| 2026-10-15 | Phase 1 | Verzeichnis `RBF68k` eingerichtet, Defs, Referenz-Bins, Disassembly und REA-Quellen bereitgestellt | Erledigt |
| 2026-10-15 | Phase 2 | Grundgerüst `rbf_68k.a` und `build.sh` gebaut; `rbf.mod` erfolgreich assembliert & gelinkt, Modul-Header und relative Sprungtabelle verifiziert | Erledigt |
| 2026-10-15 | Phase 3 | Treiber-Interface (`rbf_drv.a`), LSN0-Parser und FD-Segment-Mapping (`rbf_fs.a`) implementiert & gebaut; realer RBF-Datenträger `q9data-rbf.img` analysiert | Erledigt |
| 2026-10-15 | Phase 4 | Lese-Pfad vollständig implementiert (`rbf_ops.a` mit Open, Seek, Read, ReadLn, GetStt, Close); `rbf.mod` auf 2156 Bytes angewachsen; Testsuite `test_rbf_readonly.py` erfolgreich | Erledigt |
| 2026-10-15 | Phase 5 | `I$ChgDir` und Verzeichnis-Iteration (`dir`/`list`) fertiggestellt; `rbf.mod` erfolgreich auf 2220 Bytes aktualisiert | Erledigt |
| 2026-10-15 | Phase 6 | Schreib-Pfad (`rbf_write.a`) mit Bitmap-Allokator, Create, MakDir, Write, Delete, SetStt implementiert; alle 13 Slots in `rbf.mod` (3604 Bytes) verdrahtet; Testsuite `test_rbf_readwrite.py` erfolgreich | Erledigt |
| 2026-10-16 | Phase 7/8 | Register-Kollision `a1` in `Rbf_ParseComponent` behoben; Null-Zeiger-Schutz für Prozessdeskriptor (`a4`) in `Rbf_ChgDir` und `Rbf_P_IsRel`; Reentrant Locking Schutz in `Rbf_DrvWaitBusy`; `SS_FD` in `Rbf_SetStt` implementiert; `rbf.mod` (5.992 Byte) bootet sauber in `Q9-Flux-68k` durch `sysgo`, führt `SYS/startup` aus, erreicht `mshell` und besteht alle Standard-Utilities (`dir`, `pd`, `list`, `makdir`, `copy`, `del`, `deldir`) | Erledigt |

