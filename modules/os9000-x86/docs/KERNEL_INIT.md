# OS-9000/x86-Kernel: Ghidra-Analyse, Entry-Point und Init-Sequenz — im Vergleich zum OS-9/68K-Kernel

Untersuchtes Modul: [`../vendor-live/kernel`](../vendor-live/kernel) (76.944 Byte,
live aus dem laufenden System extrahiert, siehe
[`../vendor-live/PROVENANCE.md`](../vendor-live/PROVENANCE.md)) — bewusst
diese Kopie statt [`../vendor/kernel`](../vendor/kernel) (aus `mw86.tar`),
weil sie den tatsächlich laufenden Build enthält.

Vergleichspunkt: der bereits vollständig analysierte OS-9/68K-Kernel
(`dker030s`), dokumentiert in [`../../docs/REVERSE_ENGINEERING.md`](../../docs/REVERSE_ENGINEERING.md),
insbesondere der Abschnitt "Fund: Trap-/Exception-Tabellen-Initialisierung
und Dispatch-Ziele" (Zeile ~149, zentrale Boot-Init-Funktion `FUN_000067a0`).

## Ghidra-Setup

- Projekt: `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/` (außerhalb
  des Repos, gleiche Konvention wie bei den 68K-Modul-Projekten).
- Import: `analyzeHeadless ... -import vendor-live/kernel -processor x86:LE:32:default -loader BinaryLoader -loader-baseAddr 0x21e400`
  — Basisadresse bewusst auf `0x21E400` gesetzt (die physische RAM-Adresse,
  an der das Modul lief, siehe PROVENANCE.md), damit etwaige absolute
  Adressverweise korrekt aufgelöst werden. Compiler-Spec wurde beim Import
  automatisch auf `windows` gesetzt (Ghidra-Default für `x86:LE:32:default`
  ohne explizite `-cspec`); für reine Disassemblierung/Dekompilierung ohne
  Aufrufkonventions-Feinschliff war das ausreichend, könnte bei einer
  Vertiefung aber gegen `gcc` getauscht werden.
- Skripte: [`../ghidra_scripts/kernel_ParseHeaderAndAnalyze.java`](../ghidra_scripts/kernel_ParseHeaderAndAnalyze.java),
  [`kernel_SetEntryAndAnalyze.java`](../ghidra_scripts/kernel_SetEntryAndAnalyze.java),
  [`kernel_DecompileInitChain.java`](../ghidra_scripts/kernel_DecompileInitChain.java),
  [`kernel_DumpAllFunctions.java`](../ghidra_scripts/kernel_DumpAllFunctions.java).
- Rohausgaben: [`../disasm/kernel_header_dump.txt`](../disasm/kernel_header_dump.txt),
  [`../disasm/kernel_full_disasm.txt`](../disasm/kernel_full_disasm.txt) (alle
  229 Funktionen, reines Assembler-Listing), [`../disasm/kernel_init_chain_decompiled.txt`](../disasm/kernel_init_chain_decompiled.txt)
  (Pseudo-C der Init-Aufrufkette, Tiefe 3).

## Fund 1: Entry-Point empirisch gefunden — Header-Feld `0x24` = x86-Analogon von `M$Exec`

Der Bereich `0x14`–`0x58` war laut [`FINDINGS.md`](FINDINGS.md) noch nicht
Feld-für-Feld zugeordnet. Rohbyte-Sichtung ergab mehrere Kandidatenwerte
(alle < Modulgröße, also plausible In-Modul-Offsets); disassembliert wurde
nur einer davon zu einem eindeutigen, sauberen Treffer:

| Header-Offset | Wert (LE) | Disassemblierung am Zieloffset |
|---|---|---|
| `0x24` | `0x000000A4` | `JMP 0x0021E4C0` — sauberer, direkter Sprung |
| `0x2C` | `0x00001B60` | landet 2 Byte hinter einem echten `push ebp; mov ebp,esp`-Prolog woanders im Modul — vermutlich Zufallstreffer in dichtem Code, kein Hinweis auf dieses Feld |
| `0x30`, `0x34`, `0x38` | diverse | keine erkennbaren Funktionsanfänge an den Zieladressen |

**Feld `0x24` = die x86-Entsprechung von `M$Exec`.** Es zeigt nicht direkt
auf die Init-Funktion, sondern auf einen 5-Byte-Trampolin
(`E9 17 00 00 00` = `JMP rel32`) unmittelbar nach dem Copyright-String im
Modulkopf — der Sprung landet bei `0x21E4C0`, dem tatsächlichen
Init-Funktionskörper (96 Byte, sauberer C-Compiler-Prolog `PUSH EAX; MOV
EAX,[EAX+0x90]; ...`). Dasselbe Trampolin-Prinzip wie bei 68K-Modulen
(`M$Exec` zeigt dort ebenfalls oft auf einen kurzen Sprung-Stub statt
direkt auf den Code), nur mit 4-Byte- statt 2-Byte-Namensfeld-Offset davor.

**Bonus-Fund dabei:** direkt nach dem Trampolin, bei Offset `0xB4`/`0xB6`,
steht zweimal hintereinander das 16-Bit-Wort `0xB0BD` — **derselbe
"Struktur-Signatur"-Musterwert**, den die 68K-Analyse bei Modulkopf-Offset
`0x40`–`0x43` fand und in `REVERSE_ENGINEERING.md` ("Fund: `0x1424`
... löst das `0xB0BD`-Rätsel") als Validierungs-Signatur für verlinkte
Modul-/Deskriptor-Listen identifizierte. Dass exakt derselbe Magic-Wert an
einer strukturell ähnlichen Stelle (kurz nach dem Modul-Entry) im
komplett neu geschriebenen x86-Kernel wieder auftaucht, ist ein starkes
Indiz für **Code-/Konventions-Kontinuität über den kompletten C-Neuschrieb
hinweg**, nicht nur für die öffentlich dokumentierten Header-Felder.

## Fund 2: Auto-Analyse-Ergebnis — deutlich bessere Ghidra-Abdeckung als beim 68K-Kernel

Nach Setzen des Entry-Points und `analyzeAll`:

| | 68K-Kernel (`dker030s`, vor manueller Nacharbeit) | x86-Kernel (`kernel`, reine Ghidra-Autoanalyse) |
|---|---|---|
| Code-Abdeckung | ~55 % (vor manueller Dispatch-Ziel-Verfolgung) | **70,0 %** |
| Funktionen erkannt | vergleichsweise wenige, viele mussten per Hand (`createFunction`, Kontrollfluss-Verfolgung) nachträglich erschlossen werden | **229**, komplett automatisch |
| Undefiniert | 40 % (vor Nacharbeit) | 29,1 % |

**Andreas' Vermutung aus der Architektur-Diskussion bestätigt sich damit
konkret**: der C-kompilierte x86-Code lässt sich von Ghidra spürbar
zuverlässiger automatisch in Funktionen zerlegen als der handoptimierte
68K-Assembler, wo Ghidras Funktions-Heuristiken deutlich häufiger scheitern
(z. B. weil handgeschriebener Code ungewöhnliche Sprung-/Rücksprung-Muster
nutzt, die kein Standard-Compiler-Prolog sind). Ob der **Dekompilierte
Pseudo-C-Code** am Ende auch leichter zu *verstehen* ist, ist differenzierter
zu beantworten (siehe Fund 3/4) — die reine Funktionserkennung ist aber
eindeutig einfacher.

## Fund 3: Die Init-Funktion selbst — Speichergrößen-Anfrage, Arena-Aufbau, manuell konstruierter Prozesskontext

Pseudo-C (volle Version in `disasm/kernel_init_chain_decompiled.txt`,
Tiefe 0). Ghidras Dekompiler verliert hier streckenweise die Kalling-
Konvention (viele `extraout_`/`unaff_`-Variablen — der Kernel übergibt
offenbar Parameter/Rückgabewerte teils in Registern statt auf dem Stack,
was der `windows`-Compiler-Spec nicht korrekt modelliert; siehe "Grenzen"
unten), der grobe Ablauf ist aber klar erkennbar:

1. Prüft ein Bit (`& 0x10`) in einer über mehrere Zeiger verketteten
   Struktur, die letztlich vom **Boot-Parameter, der der Init-Funktion in
   `EAX` übergeben wird**, abhängt — bei Bedarf wird eine triviale
   1-Byte-Stub-Funktion (`FUN_0022e9b0`, reines `RET`) aufgerufen. Dieselbe
   Zeigerkette (`[[[EBX+0x6c]+0x90]+0x2c]+0x10`) taucht später in mehreren
   anderen Funktionen wieder auf, immer zur Abfrage einzelner Bits — ein
   **Capability-/Konfigurations-Flags-Wort**, das aus einer separaten,
   zielspezifischen Beschreibungsstruktur stammt (siehe Fund 5).
2. Ruft `FUN_0021f17e(param)` — fragt über einen **Funktionszeiger-Aufruf**
   (`(**(code**)(param+0x34))(...)`, also einen virtuellen Tabellenaufruf,
   kein fester Sprung) eine **verfügbare Speichergröße** von einer
   externen Beschreibungsstruktur ab, rundet das Ergebnis auf **16 Byte**
   auf (`+0xfU & 0xfffffff0`) — dieselbe 16-Byte-Granularität, die
   `vendor/README.md` für den klassischen 68K-"Standard-Allocator"
   dokumentiert.
3. Baut daraus eine Zieladresse (`iVar4`) und ruft
   `FUN_0022246c(iVar4)` — siehe Fund 4, ein **Modul-Lade-/Relozierer**.
4. Ruft `FUN_0021eb26(iVar4)` (1405 Byte, größte direkte Aufrufer) — siehe
   Fund 5, der eigentliche "Kernel-Globals aufbauen"-Block.
5. Konstruiert danach **von Hand einen Stack-Rahmen** mit fest einprogram-
   mierten Rücksprungadressen (`0x21E51E`, `0x21E536` — beides Adressen
   *innerhalb dieser selben Init-Funktion*) und ruft den von `FUN_0021eb26`
   zurückgegebenen Funktionszeiger `pcVar2` darüber auf. Dieses Muster
   — der Kernel baut sich selbst einen künstlichen Rücksprungkontext, statt
   normal zurückzukehren — ist ein klarer Fingerabdruck für **"hier wird der
   allererste Prozess/Kontext von Hand angelegt"**, konzeptionell dieselbe
   Aufgabe wie beim 68K-Kernel, dort aber über Register/Systempool-
   Strukturen statt über einen manipulierten x86-Stack gelöst.

## Fund 4: `FUN_0022246c` — Modul-Header-CRC-Prüfung + Relozierer (nicht Teil des 68K-Vergleichs, aber zentral fürs Boot-Verständnis)

Vollständig lesbar, weil sauber kompiliert:

```c
if (*in_EAX == 0x4afc) {                 // Modul-Sync prüfen
    if (FUN_00227a2e() == 0) {           // Header-Checksumme über 44 Worte (88 Byte = 0x58)
        // ... läuft zwei Tabellen ab in_EAX+0x1a durch, addiert die
        // Modul-Basisadresse auf jeden Eintrag -> klassische Relozierung
        if (*(code**)(unaff_EBX+0xa50) != 0)
            (**(code**)(unaff_EBX+0xa50))(param_1, ...);   // Post-Load-Hook
        return 0;                        // Erfolg
    }
    return 0xEC;                         // Checksummenfehler
}
return 0xCD;                             // kein gültiges Modul (falscher Sync)
```

`FUN_00227a2e` XORt exakt **44 Worte (88 Byte = `0x58`)** — das deckt sich
mit dem uns bereits bekannten Namens-Offset `0x58` und bestätigt: die
x86-Kopfprüfsumme läuft über den kompletten Standard-Header **bis
einschließlich der Namensfeld-Grenze**, dieselbe Prüfsummen-Idee wie beim
68K ("Header parity XOR(0x00-0x2E) = expect 0xFFFF", nur über eine breitere
Kopfgröße, passend zum breiteren Namensfeld).

**Wichtiger struktureller Unterschied zum 68K:** klassische OS-9/68K-Module
sind bewusst **positionsunabhängiger Code** und benötigen beim Laden KEINE
Relozierung. Diese x86-Funktion belegt, dass OS-9000/x86-Module (zumindest
manche) **tatsächlich eine Relozierungstabelle mitbringen und beim Laden
patchen lassen** — ein echter Architekturunterschied, keine reine
Implementierungsdetail-Variante. Passt zu einer C-Toolchain, für die
vollständig PIC-fähiger Code aufwändiger zu erzeugen ist als für
handgeschriebenen 68K-Assembler.

`FUN_0021f8e6` (Aufrufer-Kette von `FUN_0021f0a4`, das direkt am Anfang von
`FUN_0021eb26` läuft) ist der dazugehörige **Modul-Scanner**: durchsucht
einen Speicherbereich Wort für Wort nach `0x4AFC`, validiert jeden Treffer
mit derselben Checksummenfunktion, überspringt bei Erfolg exakt `M$Size`
Bytes zum nächsten Kandidaten (klassisches "Module hintereinander im
Speicher aufreihen"-Scanverfahren) und wählt unter mehreren Namens-
Treffern den mit der höchsten Prioritäts-/Typ-Kennung. Das erklärt
plausibel, **wie der x86-Kernel weitere Module (IOMan, RBF, ...) zur
Boot-Zeit im Speicher findet**, ohne dass dafür schon ein Dateisystem
verfügbar sein müsste — passend zum bereits in [`FINDINGS.md`](FINDINGS.md)
dokumentierten Bild, dass `sysboot` mehrere Module gebündelt (wenn auch
komprimiert) mitbringt.

## Fund 5: `FUN_0021eb26` — der eigentliche "Kernel-Globals aufbauen"-Block (x86-Analogon zur 68K-Boot-Init `FUN_000067a0`)

Das ist der inhaltlich dichteste Teil, 1405 Byte. Wichtigste erkennbare
Schritte (Register `unaff_EBX` verhält sich hier durchgehend wie ein
**Kernel-Global-Basiszeiger** — das x86-Analogon zum 68K-System-Global-
Bereich, der dort über VBR erreicht wird):

- Kopiert mehrere Felder aus der übergebenen, zielspezifischen
  Beschreibungsstruktur (`in_EAX`, vermutlich der "Low-Level-System"/
  Boot-Parameter-Block) in die Kernel-Globals, **inklusive eines
  Rückzeigers auf diese Struktur selbst** (`unaff_EBX+0x36 = in_EAX`) —
  wird später für die Capability-Flags-Abfragen wiederverwendet (Fund 3/5).
- Initialisiert **vier leere zirkuläre Doppel-Listen** (Kopf/Schwanz zeigen
  jeweils auf sich selbst) — plausibel Bereitschafts-/Wartelisten für
  Prozesse, direkt vergleichbar mit dem 68K-Scheduler-Fund
  (`Q9_scheduler_183a`, Ready-Queue-Einfügeroutine).
- Fragt erneut die Speichergröße ab (`FUN_0021f17e`, s. Fund 3) und legt
  danach **drei aufeinanderfolgende feste Speicherbereiche** an: einen
  Bereich fester Größe (`+0x464A` Byte), gefolgt von zwei Arrays aus
  16-Byte-Einträgen (Anzahl aus einem zuvor gelesenen Konfigurationswert),
  jeweils mit einem Freiliste-Sentinel (`0x0777`) markiert — plausibel die
  **Prozess- und Pfad-Deskriptor-Tabellen** (die feste Kappungsgrenze
  `0x80` = 128 taucht mehrfach in der unmittelbaren Umgebung auf, passend
  zu klassischen OS-9-Limits wie `NPROC`/`NPD`).
- Markiert alle Slots dieser beiden Tabellen als **frei/unbenutzt**
  (Sentinel-Zeiger + `0xFFFF`-Markierung je Eintrag) und baut daraus eine
  **verkettete Freiliste über getaggte Indizes** (`(index*2)|1` — Bit 0
  als Gültigkeits-Tag, ein klassischer OS-9-Kniff) — direktes Gegenstück
  zum 68K-Fund `FUN_000025f8` ("Prozessdeskriptor-Slot zurücksetzen/
  freigeben") und `0x5712` ("größensortierte Freiliste auf Arena-Ebene").
- **Baut eine Interrupt-/Exceptions-Dispatch-Tabelle auf** — über
  `FUN_00221540`, zweimal aufgerufen (für zwei getrennte Tabellenbereiche,
  Parameter `1` und `2`). Diese Funktion kopiert **14-Byte-Quelleinträge**
  (Vektornummer + Handler-Zeiger + zwei weitere Felder) aus einer
  kompakten, im Modul mitgelieferten Quelltabelle in **16-Byte-Zielslots**,
  indiziert nach Vektornummer — **strukturell identisch zum 68K-Fund**
  (kompakte Quelltabelle im Modul → zur Boot-Zeit in eine feste
  Ziel-Dispatch-Tabelle expandiert), nur mit anderer Eintragsgröße/-form.
  **Wichtiger Unterschied:** kein `LIDT`/`SIDT` in dieser Funktion — die
  echte x86-Hardware-IDT wird hier **nicht** direkt programmiert. Das
  spricht dafür, dass das eigentliche `lidt` (Hardware-Vektortabelle) in
  der zielspezifischen "Low-Level System"/romcore-Schicht passiert
  (außerhalb dieses Kernelmoduls, passend zur bereits aus dem Technical
  Manual bekannten Portierungsgrenze), während der Kernel selbst nur seine
  **eigene, softwareseitige** Dispatch-Tabelle pflegt — vermutlich springt
  ein von der LLS-Schicht in die echte Hardware-IDT eingetragener
  gemeinsamer Trampolin am Ende in diese kernel-eigene Tabelle. Das wäre
  ein lohnender nächster Schritt, ist hier aber nicht mehr verifiziert
  worden (siehe "Grenzen" unten).
- Durchläuft eine Liste von Geräte-/Modul-Einträgen aus der Boot-Parameter-
  Struktur (`*(in_EAX+0x24)`) und ruft für jeden Eintrag `FUN_0021f4bc`
  auf — plausibel eine **Geräte-/Low-Level-Treiber-Initialisierungsschleife**
  (im Detail nicht weiter verfolgt).
- Ruft am Ende einen weiteren Funktionszeiger aus der Boot-Parameter-
  Struktur auf (Slot `0x18`) — plausibel "Interrupts aktivieren"/"Timer
  starten", ebenfalls Teil der Low-Level-System-Schicht.

## Vergleich: 68K-Kernel-Bootstrap vs. x86-Kernel-Init

| | OS-9/68K (`FUN_000067a0`, `dker030s`) | OS-9000/x86 (`kernel_init_0021e4c0` + `FUN_0021eb26`) |
|---|---|---|
| Entry-Point-Fund | `M$Exec` @ Header `0x30` (4 Byte), zeigt direkt auf Assembler-Code | `M$Exec`-Analogon @ Header `0x24`, zeigt auf einen 5-Byte-`JMP`-Trampolin, dieser dann auf den echten Code |
| Exception-/Trap-Tabelle | Hardware-VBR (`MOVEC`) + 256-Eintrags-Tabelle (10 Byte/Eintrag), aus (count,offset)-Quelltabelle im Modul befüllt, muss exakt 256 erreichen sonst Panic | Software-Dispatch-Tabelle (16 Byte/Eintrag), aus 14-Byte-Quelleinträgen befüllt, **kein direkter Hardware-`LIDT`** in diesem Modul — echte IDT vermutlich von der Low-Level-System-Schicht gesetzt |
| Speicher-/Arena-Setup | Allokator mit 16-Byte-Auflösung, Systempool-Anfrage über festen Konstantenaufruf | Verfügbare Größe über **virtuellen Funktionszeiger-Aufruf** an eine zielspezifische Beschreibungsstruktur abgefragt, ebenfalls auf 16 Byte gerundet — dieselbe Granularität, aber der Zugriffsweg selbst ist bereits Teil der Portierungsschicht statt fest im Kernel verdrahtet |
| Prozess-/Deskriptor-Tabellen | Slot-Reset/Freigabe (`FUN_000025f8`), größensortierte Freilisten auf Arena-Ebene (`0x5712`) | Zwei feste Arrays fester Größe (Limit `0x80`=128), Freiliste über getaggte Indizes (`(i*2)\|1`) statt Zeigerliste — funktional dasselbe Ziel, andere Kodierung |
| Panic-/Fehlerausgabe | Feste Adressfolge (`0x7f6`, `0x850`, `0x84a`, `0x868`) | Ebenfalls über **Funktionszeiger-Tabelle** (virtueller Aufruf, Slot `0xa04` der Kernel-Globals) statt fester Adressen — konsistent mit der generellen x86-Tendenz, Hardware-/Ausgabe-Spezifika über Zeiger statt feste Sprünge zu entkoppeln |
| Modul-Kopf-Prüfsumme | XOR über `0x00`–`0x2E` (68K-Standardheader), erwartet `0xFFFF` | XOR über `0x00`–`0x57` (44 Worte, breiterer x86-Header), erwartet `0` |
| Modul-Relozierung | **Keine** — 68K-Module sind bewusst positionsunabhängig | **Vorhanden** — dedizierte Relozierungsfunktion (`FUN_0022246c`) patcht Zeigertabellen anhand der Ladeadresse |
| Magic-Konstante `0xB0BD` | Bei Modulkopf-Offset `0x40`–`0x43`, Validierungs-Signatur für verkettete Modul-/Deskriptor-Listen | Direkt nach dem Entry-Trampolin bei Offset `0xB4`/`0xB6` (zweimal) — dieselbe Konstante, andere Fundstelle, exakte Bedeutung an dieser Stelle nicht verifiziert |
| Ghidra-Auto-Analyse | ~55 % Coverage vor manueller Nacharbeit, viele Funktionsgrenzen mussten per Hand ergänzt werden | 70 % Coverage, 229 Funktionen, komplett automatisch — **bestätigt Andreas' Vermutung, dass C-kompilierter Code für Ghidra deutlich leichter automatisch zu erschließen ist** |

**Gesamteinschätzung:** Die **konzeptionelle Reihenfolge** der Kernel-
Initialisierung ist über beide Architekturen hinweg klar wiedererkennbar
— Speicher/Arena einrichten, Exception-/Trap-Dispatch-Tabelle aus einer
kompakten Quelltabelle aufbauen, Prozess-/Deskriptor-Tabellen mit
Freilisten vorbereiten, dann den ersten Prozess-/Ausführungskontext von
Hand konstruieren und in ihn hineinspringen. Was sich zwischen den
Architekturen ändert, ist fast durchgängig **die Art der Kopplung**: der
68K-Kernel verdrahtet Hardware (VBR, feste Sprungadressen) und
Systemlogik im selben Modul, der x86-Kernel entkoppelt beides konsequent
über Funktionszeiger/virtuelle Aufrufe in eine externe, zielspezifische
Beschreibungsstruktur — exakt das im Technical Manual dokumentierte
**"Low-Level System"-Portierungskonzept**, hier erstmals nicht nur aus der
Dokumentation, sondern direkt im disassemblierten Code bestätigt.

## Grenzen / offen geblieben

- Die Ghidra-Dekompilierung verliert bei mehreren Funktionen die
  Aufrufkonvention (`extraout_`/`unaff_`-Variablen) — der x86-Kernel
  scheint an mehreren Stellen Werte in Registern statt über den Stack zu
  übergeben/zurückzugeben, was die generische `windows`-Compiler-Spec
  nicht vollständig modelliert. Eine gezielte `-cspec gcc`- oder
  benutzerdefinierte Kalling-Konventions-Definition könnte das verbessern,
  wurde hier aber nicht mehr ausprobiert.
- Nicht verifiziert: ob/wo die echte x86-Hardware-IDT (`LIDT`) tatsächlich
  gesetzt wird — vermutlich außerhalb dieses Moduls in der Low-Level-
  System-Schicht, aber diese Schicht selbst wurde nicht separat aus dem
  RAM extrahiert oder untersucht.
- `FUN_0021f4bc` (Geräte-/Modul-Init-Schleife), `FUN_0021f680`/
  `FUN_0021e5a0` (letzte zwei Aufrufe am Ende von `FUN_0021eb26`) und die
  genaue Bedeutung der beiden `FUN_0022264a`-Aufrufe wurden nicht mehr im
  Detail gelesen — Kandidaten für eine weitere Runde, falls gewünscht.
- Die exakte Bedeutung der beiden Tabellen aus Fund 5 (Prozess- vs.
  Pfad-Deskriptoren?) ist plausibel, aber nicht durch einen Cross-Check
  (z. B. Laufzeit-Vergleich mit `ident`/`procs`-Ausgabe im Gast) bestätigt
  — anders als bei der 68K-Syscall-Tabelle, die per Live-Verifikation im
  Emulator abgesichert wurde.
- Die eingebaute 96-Byte-Init-Funktion selbst ruft **keine sichtbare
  Prozess-Erzeugung** (kein offensichtliches `F$Fork`-Äquivalent) auf —
  vermutlich passiert das erst NACH dem Rücksprung über den manuell
  gebauten Stack-Rahmen, in vom aufgerufenen Funktionszeiger `pcVar2`
  erreichtem Code, der hier (Tiefe-3-Grenze der automatischen Verfolgung)
  nicht mehr aufgelöst wurde.

**Erstellt**: 2026-08-13
