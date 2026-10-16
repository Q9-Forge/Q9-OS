# Q9-PIPE (pipeman) – Implementierungsstatus

Stand: **2026-10-17**  
Status: 🟢 **100% Implementiert, Gebaut und Live auf Q9-Flux-68k Verifiziert**

---

## 1. Übersicht

`pipeman` ist der native OS-9/68k Pipe File Manager für Q9-Flux-68k. Er implementiert das inter-prozessuale Datenstreaming für anonyme Shell-Pipes (`cmd1 ! cmd2`) sowie Named Pipes über das `/pipe`-Gerät.

### Modul-Spezifikation
- **Modulname:** `pipeman`
- **Typ/Sprache:** File Manager (`$0D01` / Type 13, Lang 1)
- **Attribute:** Reentrant / Supervisor (`$8000`)
- **Edition:** 1
- **Größe:** 2.448 Bytes (`$0990`)
- **Parität:** `$1D43` (Good)
- **CRC:** `$F8E0B2` (Good)

---

## 2. Architektur & Komponenten

| Datei | Funktion / Verantwortung |
|---|---|
| `src/pipe_defs.a` | Datenstrukturen (`PD_RBUF`, `PD_RCNT`, `PD_RHEAD`, `PD_RTAIL`, Pufferzeiger, Statusflags) |
| `src/pipe_68k.a` | Modul-Header, Exec-Tabelle (13 I/O-Einstiegspunkte) |
| `src/pipe_open.a` | Pfaderzeugung, Ringpuffer-Initialisierung, Open/Create/Attach-Verhalten |
| `src/pipe_io.a` | Read/Write/ReadLn/WritLn mit Ringpuffer-Streaming, Blockierung, EOF-Signalisation |
| `src/pipe_stat.a` | GetStt / SetStt (SS_Ready, SS_Size, SS_EOF) |
| `src/pipe_sub.a` | Pufferallokation, Task-Scheduling / Wakeup (`F$Sleep`, `F$Send`) |
| `src/build.sh` | Build-Skript mit `qr68k`, `ql68k` und `fixmod.py` |

---

## 3. Live-Verifikation auf Q9-Flux-68k

Die Verifikation erfolgte live unter der nativen OS-9 `mshell` in Q9-Flux-68k via `tools/test_pipe_q9flux.exp`:

| Test | Befehl | Ergebnis | Bedeutung |
|---|---|---|---|
| 1. Modul-Integrität | `ident -m pipeman` | ✅ PASSED | Header, Edition 1, Parität `$1D43` und CRC `$F8E0B2` valide |
| 2. Anonyme Pipe | `echo "Q9_NATIVE_PIPE_TEST_SUCCESS" ! grep Q9_NATIVE_PIPE_TEST_SUCCESS` | ✅ PASSED | Single-Block Transfer & Filterung via Shell-Pipe |
| 3. Multi-Block Streaming | `dir /dd ! grep CMDS` | ✅ PASSED | Multi-Block Streaming von Verzeichniseinträgen |
| 4. Prozess-Pipe | `procs ! grep shell` | ✅ PASSED | Systemzustand/Prozesstabelle via Pipe gestreamt |
| 5. Wortzählung | `echo "alpha beta gamma" ! count -w` | ✅ PASSED | Formatierter Texttransfer und Zählung |
| 6. Modusprüfung | `makdir /pipe/testdir` | ✅ PASSED | Verzeichnis-Operationen korrekt abgewiesen (`Error #000:214` = `$D6` / `E$BMode`) |

Alle 5 Integrationsprüfungen wurden fehlerfrei abgeschlossen.
