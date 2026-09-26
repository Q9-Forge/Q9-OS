# DHF-Protokoll (Manager → Treiber → Emulator-Gerät)

Stand 26.09.2026. Beschreibt, was **tatsächlich** implementiert ist: `manager/dhfmgr_68k.a`,
`driver/dhfdrv_68k.a` und das Gerät im Emulator (`Q9-Flux/Q9-Flux-68k/src/devices/dhf/`).
Die frühere Fassung dieser Datei beschrieb ein nie umgesetztes Nachrichtenformat.

DHF gibt es nur im Emulator: Das „Gerät“ ist ein MMIO-Fenster, hinter dem der Emulator
direkt auf ein Verzeichnis des Hosts zugreift.

## Ablauf eines Aufrufs

1. IOMan ruft den FileManager `dhfmgr` auf (a1 = Pfaddeskriptor, a4 = Prozessdeskriptor,
   a5 = Registersatz des Aufrufers).
2. Der Manager füllt seinen Kommandoblock (`CmdBlk`, Layout wie das Fenster) und ruft den
   Treiber immer über dessen `D_WRIT`-Eintrag auf.
3. Der Treiber kopiert Byte 4–31 ins Fenster (`V_PORT` aus `M$Port` des Deskriptors) und
   schreibt **zuletzt** das Kommandobyte. Dieser Schreibzugriff löst im Emulator die
   Verarbeitung synchron aus.
4. Danach kopiert der Treiber Status und a0/a1/d0/d1/d2 (Byte 2 und 8–27) zurück. Ist der
   Status ungleich 0, kehrt er mit Carry und d1.w = Status zurück.

Das ist nur deshalb sicher, weil OS-9/68k im System-State nicht unterbrochen wird und das
Gerät synchron arbeitet. Für mehrere CPUs wäre ein anderes Protokoll nötig (nicht geplant).

## Fenster (`struct dhf_shared`, 32 Byte, big-endian)

| Offset | Feld | Bedeutung |
|---:|---|---|
| 0 | version | 1 |
| 1 | command | Kommando (s. u.); Schreiben löst die Verarbeitung aus |
| 2 | status | Ergebnis: 0 = OK, sonst **OS-9-Fehlercode** (geht unverändert an den Aufrufer) |
| 3 | flags | unbenutzt |
| 4 | seq | **aktuelles Verzeichnis des aufrufenden Prozesses** (Pseudo-Sektornummer aus P$DIO+4, bei Exec-Modus P$DIO+$14); relative Pfade gelten ab dort |
| 8 | a0 | meist Zeiger auf den Pfadnamen (Gast-RAM) |
| 12 | a1 | meist Zeiger auf einen Puffer |
| 16 | d0 | meist Pfadnummer (= Handle im Gerät) |
| 20 | d1 | Byteanzahl / Position / Attribute |
| 24 | d2 | Modus / whence / Rückgabe |
| 28 | pid | Prozess-ID des Aufrufers (für Sperren: Pfade desselben Prozesses sperren sich nicht) |

Zwei Geräte-Instanzen: `$FFFF4000` (Deskriptor `d0`) und `$FFFF4100` (Deskriptor `d1`), je mit
eigenem Basispfad, eigener Handle- und Nummerntabelle.

## Kommandos

| Nr | Kommando | Eingaben | Ausgaben | I$-Aufruf |
|---:|---|---|---|---|
| 1 | CREATE | a0 Pfad, d0 Pfadnr, d1 OS-9-Attribute, d2 Modus, a1 Anfangsgröße (bei ISize_ $20) | a0 pd_fd, a1 pd_dfd (Byteadressen) | I$Create |
| 2 | OPEN | a0 Pfad, d0 Pfadnr, d2 Modus | a0 pd_fd, a1 pd_dfd, d2 = 1 wenn Verzeichnis | I$Open |
| 3 | SEEK | d0, d1 Position, d2 whence (Manager: immer 0) | d1 Position | I$Seek |
| 4 | READ | d0, a1 Puffer, d1 Anzahl | d1 gelesen | I$Read |
| 5 | WRITE | d0, a1 Puffer, d1 Anzahl | d1 geschrieben | I$Write |
| 6 | READLN | d0, a1, d1 Maximum | d1 gelesen (bis CR/LF) | I$ReadLn |
| 7 | WRITELN | d0, a1, d1 | d1 geschrieben (bis einschl. CR) | I$WritLn |
| 8 | GETSTT | d0, a1 Puffer | 16 Byte {Größe, Modus, mtime, atime} | SS_Size |
| 9 | SETSTT | d0, d1 neue Größe | – | SS_Size (SetStt) |
| 10 | CLOSE | d0 | – | I$Close (nur beim letzten Abbild, PD_COUNT) |
| 11 | DELETE | a0 Pfad | – | I$Delete |
| 12 | MKDIR | a0 Pfad, d1 Attribute | – | I$MakDir |
| 13 | CHDIR | a0 Pfad | a0 Verzeichnisnummer (→ P$DIO+4) | I$ChgDir |
| 18 | INIT | a0 Basispfad (0 = nur `[dhfN] hostpath`, sonst E$NotRdy), d1 Flags (Bit 0 = nur lesbar) | – | iniz (Treiber-Init), ROM-Booter |
| 19 | TERM | – | – | Treiber-Term |
| 20 | GETFD | d0, d1 Anzahl, a1 Puffer | FD-Abbild (Figure 7-2) | SS_FD |
| 21 | SETATTR | d0, d1 Attributbyte | – | SS_Attr |
| 22 | GETPOS | d0 | d1 Position | SS_Pos |
| 23 | ISEOF | d0 | Status E$EOF am Ende | SS_EOF |
| 24 | RENAMEAT | d0 Verzeichnis-Pfadnr, a0 alter Name, a1 neuer Name | – | SS_Rename |
| 25 | GETFREE | d0 | d1 freie Bytes (32 Bit gekappt) | SS_Free |
| 26 | FDINF | d2 Sektornummer, d1 Anzahl, a1 Puffer | FD-Abbild | SS_FDInf |
| 27 | VOLSTORE | a1 Puffer | 16 Byte {Bytes/Sektor, gesamt, frei, größter Block} | SS_VolStore |
| 28 | SETFD | d0, a1 FD-Abbild | – (nur FD_DAT wirkt) | SS_FD (SetStt) |
| 29 | LOCK | d0, d1 Größe (0 = freigeben, $FFFFFFFF = ganze Datei) | – | SS_Lock |
| 254 | PING | – | – | – |

14–17 (RMDIR, RENAME, OPENDIR, READDIR) sind Reste der frühen Unix-artigen Planung und von
keinem I$-Aufruf erreichbar. SS_Ready, SS_DevNm, SS_Opt und SS_Ticks beantwortet der
Manager selbst.

## RBF-Semantik, die DHF nachbildet

Alles per Ablaufverfolgung der echten Utilities ermittelt (s. STATUS.md):

- **Verzeichnisse** sind virtuelle RBF-Verzeichnisdateien aus 32-Byte-Einträgen: Eintrag 0 = „..“,
  Eintrag 1 = „.“, Name mit Bit 7 am letzten Zeichen, Byte 29–31 = Pseudo-Sektornummer.
  Einträge haben feste Plätze (gelöscht = freier Platz), frei positionierbar per Seek.
- **Pseudo-Sektornummern:** je Host-Pfad fest vergeben, bis zum nächsten `iniz`.
- **Pfaddeskriptor, Optionsteil** (rbf.h `struct rbf_opt`): pd_att $B5, pd_fd $B6 und
  pd_dfd $BA als **Byteadresse** (Sektornummer × 256), pd_dvt $C2.
- **Aktuelles Verzeichnis je Prozess** in P$DIO ($148 im Prozessdeskriptor; +0 Gerät von
  IOMan, +4 DHF-Verzeichnisnummer; Ausführungsverzeichnis ab +$10). Wird vererbt.
- **Pfadnamen** enden wie bei OS-9 am ersten Zeichen <= $20 (NUL, CR, Leerzeichen) – `sysgo`
  übergibt CR-terminierte Namen. Nur der INIT-Basispfad (Host) wird bis NUL gelesen.
- **Pfade:** Gerätename entfällt (Basispfad = Wurzel), „...“ = zwei Ebenen hoch usw.,
  „..“ nie über die Wurzel hinaus.
- **Öffnen:** Verzeichnis ohne Dir-Bit ($80) → E$FNA, Datei mit Dir-Bit → E$FNA
  (Ausnahme: Verzeichnis, dessen Dir-Bit per SS_Attr entfernt wurde → `deldir`).
- **Create** auf eine vorhandene Datei → E$CEF.
- **Rohgerät** `/<gerät>@`: nur lesbar, liefert ein virtuelles LSN0 und eine Bitmap.
- **Namen**, die OS-9 nicht darstellen kann, werden ausgeblendet: `.DS_Store`, `._*`, mehr
  als 28 Zeichen, Bytes ab $80.
- **Zeiten** in Ortszeit.
- **Nur lesbares Laufwerk:** jede verändernde Operation → E$WP.
- **Sperren** (Kap. 7 „Record Locking“): Open mit Share_ ($40) → nicht teilbar, ein anderer
  Prozess bekommt E$Share. Im Update-Modus sperrt jedes Read den gelesenen Bereich bis zum
  nächsten Read/Write/Close; SS_Lock sperrt explizit. Zugriffe anderer Prozesse auf einen
  gesperrten Bereich meldet das Gerät mit E$Lock; der **Manager wartet** dann (F$Sleep, je ein
  Tick) und wiederholt, bis SS_Ticks erreicht ist (0 = unbegrenzt). Timeout und Zähler liegen
  im FileManager-Bereich des Pfaddeskriptors (PD_FST $2A/$2E). Nicht nachgebildet:
  EOF-Sperre, Deadlock-Erkennung.

## Konfiguration im Emulator

Abschnitte `[dhf0]`/`[dhf1]` in der `.q9`-Datei haben Vorrang vor dem Deskriptor:

```
[dhf1]
hostpath = ../cf_images/OS9SYS   ; relativ zur .q9-Datei
readonly = yes                   ; yes|no
```

Ohne Abschnitt gelten Basispfad und Flag aus dem Deskriptor (`DevCon`: Wort 0 Basispfad,
Wort 1 Flags, Bit 0 = nur lesbar).

## Diagnose

`Q9_DHF_DEBUG=1` in der Umgebung des Emulators: Jeder Open/Create wird mit aufgelöstem
Host-Pfad und pd_fd/pd_dfd protokolliert. `Q9_DHF_DEBUG=2` zusätzlich jedes Kommando mit
seq, d0/d1/d2, a1, Pfad und Status.

## Booten

Der ROM-Booter `bootdhf` (Q9-Port `ROM_CBOOT/io_dhf.c`) benutzt Instanz 0 ohne OS-9: INIT mit
a0 = 0, OPEN `OS9Boot` (Pfadnummer 1, Modus Read), GETSTT, READ der ganzen Datei nach
`bootram`, CLOSE. Zeichenketten legt er auf den Stack (das Gerät liest nur Gast-RAM).
