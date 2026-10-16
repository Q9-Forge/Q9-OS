# RBF-Dateimanager (68K) — Disassemblierungs-Arbeitsstand

Ziel: einen `F$Exit`-Aufruf finden und verstehen, der im Rahmen der
Q9-Flux-68kQEMU-Portierung (eigene QEMU-basierte Emulator-Portierung,
separates Repo) einen Boot-Hänger auslöst — der Referenz-Interpreter
(`q9.exe`, Musashi-basiert) bootet an derselben Stelle erfolgreich
weiter.

**Wichtige Korrektur (selbe Sitzung):** Ursprünglich als das
`cfide`-Treibermodul (CompactFlash/ATA) fehlidentifiziert. Der
tatsächliche `cfide`-Quellcode liegt vor
(`MWOS/OS9/68030/PORTS/common/RBF/cfide/cfide.a`, Edition 42, exakt
passend zur Boot-Banner-Meldung "CompactFlash driver build 42") und
ist ein winziger, rein registerbasierter Polling-Treiber (CF_Init/
CF_Read/CF_Write/CF_GetStat/CF_SetStat/CF_Terminate, keine
Interrupts, **kein** `F$Fork`, **kein** `F$Exit`, keine
Prozess-/Tabellenverwaltung jeglicher Art). Der von diesem Ordner
untersuchte 48-KB-Codeblock kann also nicht `cfide` selbst sein.

Anhand der Modul-Kette im RAM (per Sync-Marker `$4AFC` +
`M$Size`-Feld durchlaufen) liegt direkt vor diesem 48-KB-Block ein
noch größerer, 84-KB-Block, danach folgt der 48-KB-Block, danach ein
weiterer 18-KB-Block — alle drei plausible Kandidaten für Teile des
**RBF-Dateimanagers** (der generische, blockorientierte Dateimanager,
der `cfide` als seinen Low-Level-Treiber benutzt) oder verwandter
Systemmodule. Für RBF selbst liegt im SDK-Baum **kein Quellcode** vor
(nur Deskriptor-/Definitionsdateien unter `OS9/SRC/IO/RBF/DESC/` und
`OS9/SRC/DEFS/rbfdev.a`/`rbfstat.a` — der Dateimanager-Kern selbst ist
proprietär und nur als Binärmodul vorhanden), die Disassemblierung
bleibt hier also weiterhin die einzig verfügbare Untersuchungsmethode.
Welcher der drei benachbarten Module-Blöcke exakt den `F$Exit`-Aufruf
enthält (48 KB wurde untersucht, da genau dort die passenden Adressen
lagen), ist nicht abschließend als "RBF" bestätigt — nur als
plausibelste Zuordnung anhand von Größe und Nachbarschaft zu `cfide`.

## Herkunft des Binaries

`vendor/rbf_extracted_from_qemu_ram` wurde NICHT von Disk extrahiert,
sondern aus dem laufenden Q9-Flux-68kQEMU-Emulator per `pmemsave` als
Live-RAM-Abzug gewonnen. Modulkette im RAM per Sync-Marker-Suche
(`$4AFC`) und `M$Size`-Feld (Offset `$04`, 4 Byte) durchlaufen, bis
ein Modul mit zu den beobachteten Code-Adressen passender Größe
gefunden wurde:

- Live-Basis: `0x2cede`
- Größe (`M$Size`): `0xbcee` (48366 Byte)
- Adressumrechnung: `Ghidra-Offset = Live-Adresse - 0x2cede`

Vorsicht: dieser Live-Abzug spiegelt den Zustand zu einem bestimmten
Boot-Zeitpunkt wider und wurde bisher nicht gegen eine
Disk-Extraktion desselben Moduls verifiziert.

## Ghidra-Setup

Prozessor `68000:BE:32:MC68030`, Raw-Binary-Import bei Basisadresse 0,
volle Auto-Analyse (nicht `-noanalysis` wie bei ad-hoc-Abfragen gegen
bereits bestehende Projekte). Projekt liegt **nicht** in diesem Repo,
sondern lokal unter
`/Volumes/SSD1TB/projects/Q9-OS-ghidra-cfide-driver/` (Name noch
nicht an die Korrektur angepasst — Ghidra-Projekte sind groß/binär
und gehören nicht ins Git, s. `kernel-68k/`-Konvention).

Ein früherer, viel kleinerer Analyseversuch (`Q9-OS-ghidra-cfide/`,
nur `cfide.gpr`, 1462 Byte) deckte tatsächlich das ECHTE, winzige
`cfide`-Modul ab (Größe passt gut zum oben gezeigten Quellcode) —
war also vermutlich schon vorher korrekt identifiziert, nur für
diese Boot-Hang-Untersuchung nicht mehr weiterverfolgt worden, weil
fälschlich angenommen wurde, das Modul sei zu klein für die
beobachtete Schleife.

## Fund: `F$Exit`-Aufrufkette

Live per `-d int`-Spur (QEMU) verifiziert: der (im Q9-Flux-Boot
folgenlose, dritte) `F$Exit`-Aufruf erfolgt über eine feste
Trap-Instruktion bei Modul-Offset `0xa998` (Live `0x37876`) —
eine winzige generische Trap-Wrapper-Funktion (`FUN_0000a998`,
3 Instruktionen: `link.w a5,#0; move.l d0,d1; trap #0`).

Ihr einziger Aufrufer: `FUN_00006f34` (Offset `0x6f34`–`0x6f83`,
Live `0x33e12`–`0x33e61`) — die eigentliche Shutdown-Sequenz:

```c
void FUN_00006f34(void)
{
  int iVar1 = 0x1f;              // 31, zaehlt runter
  do {
    if (*(int *)(unaff_A6 + iVar1 * 8 + -0x6d00) != 0) {
      FUN_000085ec();
      (*extraout_A1)();          // registrierten Callback aufrufen
      FUN_000085ec();
      FUN_0000020e();
    }
    iVar1 = iVar1 + -1;
  } while (-1 < iVar1);
  if (*(code **)(unaff_A6 + -0x6210) != (code *)0x0) {
    (**(code **)(unaff_A6 + -0x6210))();   // registrierte Flush-Routine
  }
  FUN_0000a998();                 // F$Exit -- UNBEDINGT, kein Fruehausstieg
}
```

Iteriert eine **32-Einträge-Tabelle** (`A6-0x6d00`, 8 Byte pro
Eintrag = Funktionszeiger + Kontext-Zeiger-Paar) und ruft jeden
registrierten Callback auf, danach optional eine registrierte
"Flush"-Routine (`FUN_00007094`, s. u.), danach **bedingungslos**
`F$Exit`. Es gibt keinen Rückweg aus dieser Funktion, der `F$Exit`
umgeht.

Die Registrierungs-API für diese 32er-Tabelle liegt direkt davor im
Modul (`0x6ed2`–`0x6f32`): nimmt einen Index `d1` (0–31), sucht den
ersten freien (`== 0`) Slot ab diesem Index, trägt dort ein
Funktionszeiger/Kontext-Paar ein. Kein Code innerhalb dieses Moduls
referenziert `FUN_00006f34`s Adresse selbst (weder als Sprungziel
noch als PC-relative `lea`/`pea`-Konstante, per Referenzsuche über
das gesamte Modul geprüft) — sie wird also nicht per normalem
Unterprogrammaufruf erreicht, sondern vermutlich über einen extern
(vom Kernel oder Modul-Header) gehaltenen Funktionszeiger.

**Arbeitshypothese (noch nicht verifiziert):** `FUN_00006f34`
entspricht der `F$Exit`-Selbstauflösung eines von diesem Modul
selbst gestarteten Hintergrund-/Service-Prozesses (RBF-artige
Dateimanager starten teils eigene Prozesse für Hintergrundarbeit;
via `F$Fork` gestartete Prozesse beenden sich typischerweise selbst
über `F$Exit`, wenn ihre Adresse dabei als Einstiegspunkt übergeben
wurde — das würde erklären, warum die Adresse nicht per normalem
Aufruf im Modul selbst auftaucht). Das würde sich mit dem länger
bekannten Kernel-Fund verbinden (s.
`kernel-68k/REVERSE_ENGINEERING.md`, Abschnitt zur `F$Exit`-
Reaktivierungsbedingung): wäre dies ein Service-Prozess, für den auf
der Referenzumgebung ein Elternprozess per `F$Wait` wartet, während er
das auf der QEMU-Portierung (aus noch unbekanntem Grund) nicht tut,
würde genau das beobachtete Verhalten (sauberer Austritt, aber keine
Reaktivierung, Scheduler findet niemanden mehr) erklären.

**Direkt vor dem `F$Exit`-Aufruf beobachtete Schleife** (Offset
`0x2c10`–`0x70ea`, `FUN_00007094`): läuft ebenfalls eine 32-Einträge-
Tabelle ab (Schrittweite `0x2c`=44 Byte, Gesamtgröße `0x580`=1408 Byte
— andere Tabelle als die 8-Byte-Callback-Tabelle oben, klassische
OS-9-Pfad-/Deskriptor-Tabellengröße), aber NUR wenn ein Flag-Bit
(Bit `0x20`, Offset `+0x14` an einem separaten Objekt) bereits gesetzt
ist — spült dann pro Tabelleneintrag ein "dirty"-Bit (`0x200`,
Offset `+0xc`). Kein fehlschlagender Suchlauf, sondern eine geordnete
Aufräum-Passage, ausgelöst durch ein bereits vorher gesetztes Flag.

## Offen

- Endgültig klären, ob der untersuchte 48-KB-Block tatsächlich RBF
  ist (oder ein anderes Nachbarmodul) — z. B. durch Vergleich
  charakteristischer Fehlermeldungs-Strings oder Funktionsmuster mit
  bekannten RBF-Beschreibungen im Technical Reference Manual.
- Wo/wodurch wird `FUN_00006f34`s Adresse tatsächlich als
  Funktionszeiger hinterlegt (Modul-interne Tabelle? `F$Fork`-
  Parameter, live am Aufrufzeitpunkt zurückverfolgt?) — nicht mehr
  innerhalb dieses Moduls auffindbar.
- Wo wird das Flag-Bit gesetzt, das `FUN_00007094`s Flush-Schleife
  überhaupt erst scharf schaltet?
- Byte-für-Byte-CF-Zugriffsvergleich zwischen Q9-Flux-68kQEMU und der
  Referenzumgebung (Musashi-basiert) zeigt bis zum Divergenzpunkt
  identische Daten — die Ursache liegt also nicht im Datenpfad,
  sondern in einer Ablaufentscheidung, deren genauer Auslöser noch
  nicht gefunden ist.
