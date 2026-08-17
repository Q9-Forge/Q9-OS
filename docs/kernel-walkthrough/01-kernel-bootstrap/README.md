# Thema 01: Kernel-Bootstrap — vom Einsprung bis zur Übergabe an den Scheduler

Dieses Thema setzt direkt an [Thema 00](../00-modul-aufbau-und-header/) an:
der Kernel ist jetzt am echten Einsprungpunkt angekommen (68K: `0x67A0`,
x86: `0x21E4C0`). Was folgt, bis beide Kernel die Kontrolle an den
Scheduler/ersten Prozess übergeben.

## Label-Konvention

- **68K**: Präfix `Q9_` — bereits etablierte Namen aus `src/kernel/kernel.r`.
- **x86**: Präfix `Q9X_` — in dieser Runde neu vergeben (vorher nur rohe
  Ghidra-Namen wie `FUN_0021eb26`). Zwei Hilfsfunktionen (`FUN_0021e5a0`,
  `FUN_0021f680`) bleiben unbenannt, weil ihre genaue Rolle erst am Ende
  dieses Themas geklärt wird (s. u.) — ein aussagekräftiger Name wäre
  jetzt verfrüht.

## Die zwei offenen Fragen — beide geklärt

### 1. Woher bekommt der 68K-Kernel den Speicher für die Exception-Tabelle?

**Antwort: Er alloziert ihn gar nicht — er bekommt ihn vom Boot-ROM über
Register A5 fertig übergeben.** Byte-genaue Verfolgung des Stack- und
Registerinhalts an der entscheidenden Stelle (`0x68e8`-`0x68ec` in
`asm-68k.r`) zeigt: `A0 = $34(sp)` liest exakt die Stelle im
gesicherten Registersatz, an der zu Funktionsbeginn (`movem.l
a6/a5/.../d0,-(sp)`) das Register **A5** gesichert wurde. Der Aufruf von
`Q9_const_init_4978` direkt davor (der frühere Verdächtige, siehe
`docs/REVERSE_ENGINEERING.md`) ist ein reiner Zufallsnachbar — er setzt
drei unabhängige Konstanten und hat mit der Exception-Tabelle nichts zu
tun. Das erklärt auch, warum bisher keine Allokationsfunktion dafür
gefunden wurde: **es gibt keine.** Der 68K-Kernel bekommt beim Start über
Register A5 einen 2.560 Byte großen, vorbereiteten Speicherblock direkt
vom Boot-ROM.

**Bonus-Fund dabei — eine vollständige Boot-Register-Konvention:** beim
Verfolgen von A5 fielen mehrere weitere Boot-Zeit-Register auf, die der
Kernel beim Start direkt in die Kernel-Globals kopiert:

| Register | Bedeutung | Ziel in Kernel-Globals |
|---|---|---|
| `D0` | Gesamt-RAM-Größe | `Q9_D_TOTRAM` (`0x6C`) — bestätigt bereits dokumentierten Wert |
| `D1` | erkannter CPU-Typ | `Q9_D_MPUTYP` (`0x3C8`) — bestätigt bereits dokumentierten Wert |
| `A1` | Boot-ROM-Einsprungpunkt | `Q9_D_SYSROM` (`0x64`) — bestätigt bereits dokumentierten Wert |
| `A5` | Speicherblock für die Exception-Tabelle | `D_ExcJmp` (`0x68`) — **neu geklärt, s. o.** |
| `D3` | Boot-Zeit-Flags — **neu, 2026-08-17, s. u.** | `Q9_D_BOOTFLAGS` (`0x93C`) |
| `SP` (beim Einsprung) | Zeiger auf eine Boot-Zeit-Speicherregion-Liste — **neu, 2026-08-17, s. u.** | wird in eine eigene Stack-Kopie übernommen, kein fester Kernel-Global-Slot |

Damit sind vier der bisher als „PLATZHALTER“ markierten Felder in
`q9sysglob.h`/`.a` jetzt auf `VERIFIZIERT` hochstufbar (nicht in dieser
Runde nachgetragen, aber vorgemerkt für die nächste Aktualisierung dieser
beiden Dateien).

**Zweiter Bonus-Fund, jetzt vollständig aufgelöst (2026-08-14):** die alte,
als „KONFLIKT“ markierte Frage nach der echten Position von
`Q9_D_ACTIVQ`/`Q9_D_SLEEPQ`/`Q9_D_WAITQ` ist beantwortet, und alle sechs
beim Boot initialisierten leeren Listen sind jetzt identifiziert. Der
komplette Init-Block liegt bei `0x6856`-`0x689e` in `Q9_kernel_init_67a0`
(sechs `lea`/`move.l`-Dreiergruppen direkt hintereinander, siehe
[`asm-68k.r`](asm-68k.r)):

| Adresse | Verkettung bei | Bedeutung | Beleg |
|---|---|---|---|
| `0x37C` | `+0x30`/`+0x34` | `Q9_D_ACTIVQ` — Bereitschaftswarteschlange | bereits in Thema 01 (erste Runde) geklärt |
| `0x384` | `+0x30`/`+0x34` | `Q9_D_SLEEPQ` — Schlafwarteschlange | dito |
| `0x38C` | `+0x30`/`+0x34` | `Q9_D_WAITQ` — Wartewarteschlange | dito |
| `0x3FC` | `+0x8`/`+0xC` | **neu:** Basis des Arena-/Freispeicher-Kontrollblocks (`Q9_D_ARENA`) — das schon bekannte `Q9_D_FREEMEM` (`0x404`) ist exakt `Basis+8`, also das Kopf-Feld dieser Struktur, nicht separat | `Q9_arena_alloc_526c` referenziert `$3fc(a6)` mehrfach (`lea`/`pea`, u. a. `src/kernel/kernel.r` Zeile 17285 im Kontext); die Falschannahme "eigenständige Struktur" korrigiert |
| `0x774` | `+0xC`/`+0x10` | **neu:** `Q9_D_ALMQ1` — F$Alarm-Warteschlange 1 (sofortige/D1=0-Variante) | `Q9_alarm_set_157e` lädt `lea $774(a6),a0` direkt vor dem Sprung in `Q9_alarm_insert_15c4` (dem sortierten Einfüge-/Verkettungs-Code über genau `+0xC`/`+0x10`, verifiziert per Volltextlesen) |
| `0x77C` | `+0xC`/`+0x10` | **neu:** `Q9_D_ALMQ2` — F$Alarm-Warteschlange 2 (intervallbasierte Variante) | `Q9_alarm_set_1580` lädt `lea $77c(a6),a0` genauso vor `Q9_alarm_insert_15c4` |

**Wichtiger Nebenbefund:** die alten `q9sysglob.h`/`.a`-Felder
`Q9_D_THREAD` (`0x438`) und `Q9_D_ALARTH` (`0x440`) beschreiben fast
wortgleich "Kopf der System-Thread-/Alarm-Warteschlange" — aber an einer
ganz anderen Adresse als die jetzt nachweislich echten Alarm-Warteschlangen
`Q9_D_ALMQ1`/`Q9_D_ALMQ2`. Beide alten Felder waren nie mehr als
`PLATZHALTER` (nie selbst verifiziert) und sind jetzt als `KONFLIKT`
markiert, nicht gelöscht — möglich, dass sie ein anderes, noch unbekanntes
Konzept beschreiben, oder schlicht falsch geraten waren. `Q9_D_ALMQ1`/
`Q9_D_ALMQ2` (und `Q9_D_ARENA`) wurden neu und mit Status `VERIFIZIERT` in
`src/q9sysglob.h`/`.a` ergänzt.

Die `0x0777`-Freilisten-Sentinel, die schon beim x86-Kernel auftauchte
(Fund 3/5 in `KERNEL_INIT.md`), ist hier interessanterweise NICHT
wiederzufinden — die 68K-Alarm-Warteschlangen nutzen eine reine
Ringlisten-Terminierung (Kopf==Schwanz bei leerer Liste) statt eines
Tag-Sentinels. Kein Widerspruch, nur eine andere Implementierungswahl für
dasselbe Grundproblem "leere Liste erkennen".

### 2. Wohin springt der x86-Kernel am Ende von `Q9X_kernel_globals_init` (`pcVar2`)?

**Antwort: über ein manuelles Stack-Switch-Sprung-Primitiv — mechanisch
dasselbe Prinzip wie beim 68K, nur anders umgesetzt. Das Sprungziel selbst
lässt sich jetzt als Formel auflösen, wenn auch nicht als fester
Zahlenwert.** Die Funktion, die Ghidra als Rückgabewert („pcVar2“) an den
Aufrufer weiterreicht, ist in Wirklichkeit kein normaler Rückgabewert —
Ghidra scheitert hier an einem handgeschriebenen Kontextwechsel. Die
entscheidende Funktion (unbenannt gelassen, s. o.) besteht nur aus drei
Instruktionen:

```asm
XCHG EAX,ESP        ; tauscht EAX und ESP
MOV EAX,[EAX]       ; liest von der (jetzt alten) Stack-Adresse
JMP EAX
```

Das ist ein **manueller Stack-Wechsel**: EAX enthält vorher die Adresse
eines anderswo vorbereiteten neuen Stack-Rahmens; nach dem Tausch läuft
die CPU auf diesem neuen Stack weiter, und der `JMP` springt zu einer
Adresse, die dort abgelegt wurde. Ghidra dekompiliert das als normalen
Funktionsaufruf mit Rückgabewert — daher die verwirrende `pcVar2`-Zeile
in `Q9X_kernel_init`.

**Nachtrag (2026-08-14): das Sprungziel als Formel aufgelöst.** Direkt vor
dem Aufruf (`asm-x86.txt`, `0x21f08b`-`0x21f093`) steht:

```asm
0021f08b: MOV EAX,0x4140
0021f090: ADD EAX,dword ptr [EBP + -0x4]
0021f093: CALL 0x0021e5a0
```

`[EBP-4]` wurde weiter vorne in derselben Funktion (`0x21ece6`, direkt nach
`CALL Q9X_query_memsize`) mit dem Rückgabewert der zweiten
Speichergrößen-Abfrage belegt (`iVar2` in `decompiled-x86.c`) und bis
hierhin unverändert durchgereicht. **Der neue Stack-Zeiger (und damit die
Adresse, an der der eigentliche Sprungzielwert abgelegt ist) ist also
exakt `iVar2 + 0x4140`** — ein fester Offset in den gerade erst per
`Q9X_query_memsize` ermittelten, frisch reservierten Speicherbereich
hinein (der `0x4140` liegt innerhalb des schon bekannten "festen
`0x464A`-Byte-Bereichs" aus Fund 3/`KERNEL_INIT.md`, also noch vor den
Prozess-/Pfad-Deskriptor-Tabellen). Der **numerische** Wert lässt sich aus
dem Binary allein nicht mehr bestimmen, weil `iVar2` von der tatsächlichen
RAM-Größe zur Laufzeit abhängt (keine Konstante im Modul) — das wäre nur
noch per Live-Speicher-Inspektion im laufenden Emulator zu ermitteln, nicht
mehr per reiner Disassemblierung. Damit ist die Frage so weit aufgelöst,
wie es ohne einen laufenden Gast überhaupt geht.

Funktional ist klar: **das ist der x86-Moment, der dem 68K-Sprung in
`Q9_reschedule_trampolin_3140` entspricht** — beide Kernel bauen sich
sozusagen einen künstlichen Ausführungskontext und springen hinein, nur
der 68K-Kernel tut das direkt auf seinem eigenen Stack, der x86-Kernel
über einen echten Stack-Wechsel in einen frisch berechneten, dynamischen
Speicherbereich hinein.

## Neuer Fund (2026-08-17): Wie findet der Kernel das Init-Modul?

Bisher offene Lücke (`Q9_D_INIT` nur `[HANDBUCH]`, kein Schritt in der
chronologischen Übersicht unten) — Anlass war Andreas' Frage zur Boot-
Reihenfolge des eigenen Kernels ("kommt zuerst die Init einlesen?").
Gezielt in `dker030s` nachgesucht, Fund bei `0x6986`-`0x6a06`:

**Der Kernel sucht das Init-Modul über einen eigenen Speicher-Scan,
nicht über eine vom Boot-ROM übergebene Adresse und nicht über
Typ-Code-Filterung:**

1. `0x698c`-`0x69f6`: eine Liste von Speicherregionen wird durchlaufen
   (Paare aus Basisadresse/Länge, nullterminiert, Quellregister `D6`).
   Innerhalb jeder Region wird an aufsteigenden geraden Adressen nach
   einem gültigen Modul-Header gesucht (Aufruf von `0x4410`, der bekannten
   Sync-/Prüfsummen-Validierung aus Thema 00/10 entsprechend).

   **Herkunft der Liste, nachgetragen 2026-08-17** (Andreas' Frage: "wird
   das nicht vorher vom Bootloader in die Register geladen?"): **teilweise
   ja** — sie kommt vom Boot-ROM, aber über den **Stack**, nicht über ein
   Register. Ganz am Anfang von `Q9_kernel_init_67a0` (`0x67aa`):
   `movea.l SP,A0` — der Stackpointer zeigt beim Einsprung bereits auf
   diese Liste. Der Kernel kopiert sie sofort in einen frisch reservierten
   eigenen Stack-Bereich (`0x67b0`-`0x67b6`, `move.l (A0)+,(A2)+`-Schleife,
   nullterminiert per `clr.l (A2)`), **bevor** er irgendetwas anderes tut
   — vermutlich um die Kopie vor späteren eigenen Stack-Operationen
   (Register-Sichern, weitere Aufrufe) zu schützen. `D6` wird direkt danach
   (`0x67b8`) auf diese **eigene Kopie** gesetzt, nicht auf die
   Originaladresse — deshalb wirkte die Herkunft beim ersten Blick
   unklar. Dieselbe kopierte Liste wird zweimal durchlaufen: einmal früh
   (`0x68d0`-`0x68de`, offenbar allgemeine Bereichs-Buchführung) und
   erneut hier bei der Init-Modul-Suche — eine gemeinsame Quelle für
   beide Zwecke.

   **Bonus-Fund dabei — ein fünftes Boot-Register:** `btst.l #0x4,D3`
   ist die allererste Instruktion der Funktion (`0x67a0`), noch vor dem
   Stack-Zugriff. `D3` wird bei `0x681e` komplett nach `Q9_D_BOOTFLAGS`
   (`0x93C`) gesichert — ein Boot-Zeit-Flags-Wort, neben D0/D1/A1/A5 das
   fünfte vom Boot-ROM übergebene Register. Zwei Bits mit beobachtbarer
   Wirkung: Bit 4 (bei `0x67a0`) steuert, ob der Kernel selbst die
   Interrupts maskiert (`ori #$700,SR`) oder ob der Aufrufer das schon
   erledigt hat; Bit 3 (bei `0x69dc`, s. u.) überspringt während der
   Init-Modul-Suche den Namensvergleich für den aktuellen Kandidaten und
   springt direkt zum Erfolgspfad — genaue Absicht (z. B. "Init-Modul-
   Adresse ist dem Bootloader schon bekannt") nicht abschließend
   verifiziert. `D3` wird ab `0x698a` mehrfach als gewöhnliches
   Scratch-Register wiederverwendet, die Flags-Bedeutung gilt nur bis
   dahin (gleiches Registerrecycling-Muster wie bei `A5`, s. o.).
2. Für jeden gefundenen, gültigen Kandidaten (`0x69b0`-`0x69d8`): der
   Name wird über `M$Name` (`+0xC`-Offset) gelesen und **Byte für Byte,
   groß-/kleinschreibungsunabhängig** (XOR + `andi.b #$DF`-Maskierung von
   Bit 5) gegen den festen String `"init"` verglichen — kein Typ-Code-
   Filter, reiner Namensvergleich.
3. Bei Treffer (`0x6a06`): `move.l A5,(0x20,A6)` — die gefundene
   Modul-Adresse wird in `Q9_D_INIT` gespeichert. **Wichtig:** `A5` hält
   an dieser Stelle NICHT mehr den Exception-Tabellen-Zeiger aus Zeile 22
   oben — das Register wird zwischen `0x6986` und `0x6a06` für die
   Namenssuche umgebogen und danach neu belegt. Reines Register-Recycling
   in dichtem Assembler, kein Widerspruch zum ersten Fund.
4. Direkt danach (`0x6a0a`-`0x6a4a`): mehrere Init-Modul-Felder werden
   gelesen und in Kernel-Globals kopiert — drei davon eindeutig
   identifiziert, weil sie exakt zu bereits bekannten `q9sysglob.h`-Feldern
   passen:

   | Init-Modul-Offset (rel. A5) | Ziel in Kernel-Globals | Breite |
   |---|---|---|
   | `0x68` | `Q9_D_COMPAT` (`0x2E`) | 1 Byte |
   | `0x69` | `Q9_D_COMPAT2` (`0x3E0`) | 1 Byte |
   | `0x7A` | `Q9_D_SYSCONF` (`0x38`) — deckt sich mit dem unabhängig in `Q9-Flux/docs/MMU_SSM_WORKFLOW_de.md` gefundenen `M$SysConf`/`SSM_NoProt` | 2 Byte |
   | `0x5E` | **neu, noch unbenannt** (`Q9_D_UNKN8A6` = `0x8A6`) | 2 Byte |
   | `0x60` | **neu, noch unbenannt** (`Q9_D_UNKN8A8` = `0x8A8`) | 2 Byte |

5. Falls **kein** Modul namens "init" gefunden wird: der Kernel gibt eine
   fest einprogrammierte Meldung aus — wörtlich `"kernel: can't find Init
   module"` (Fundort `0x64c5`, direkt neben dem Vergleichs-String `"init"`
   bei `0x64c0`) — und bricht vermutlich ab (Fortsetzung des Fehlerpfads
   nicht weiter verfolgt).

**Konsequenz für den eigenen Kernel** (Andreas' eigentliche Frage): das
Init-Modul-Einlesen ist kein Sonderfall vor dem Bootstrap, sondern ein
regulärer, eigenständiger Namenssuchschritt **mitten im** Kernel-
Bootstrap — nach der Exception-Tabellen-Übernahme (Zeile 22ff.), vor dem
Sprung in den Scheduler (Schritt 15 unten). Für Q9-OS' eigenen Kernel
spricht das dafür, denselben Grundablauf zu übernehmen (Namenssuche über
denselben Sync-/Prüfsummen-Scanner, den auch der Boot-Vorketten-Fund aus
Thema 10 beschreibt — ein einziger Mechanismus für beide Zwecke, wie dort
schon empfohlen), statt eine feste Adresse oder ein separates
Discovery-Protokoll zu erfinden.

`Q9_D_INIT`, `Q9_D_COMPAT`, `Q9_D_COMPAT2` und `Q9_D_SYSCONF` in
`q9sysglob.h`/`.a` sind mit diesem Fund von `[HANDBUCH]`/`[PLATZHALTER]`
auf `[VERIFIZIERT]` hochgestuft (bereits nachgetragen). Zwei neue,
bisher unbekannte Felder (`Q9_D_UNKN8A6`/`Q9_D_UNKN8A8`) sind als
`[PLATZHALTER]` ergänzt.

## Die chronologische Übersicht

Eine Zeile pro Schritt, in der tatsächlichen Ausführungsreihenfolge. Wo
ein Schritt nur bei einer Architektur vorkommt, steht das explizit dabei
— das ist keine Lücke, sondern ein echter struktureller Unterschied.

| # | Was passiert | 68K | x86 |
|---|---|---|---|
| 1 | Einsprung: kurzer Sprung überspringt den eingebetteten ID-/Copyright-String | `Q9_kernel_init_67a0` (via `M$Exec`-`BRA.W`) | `Q9X_entry_trampolin` → `Q9X_kernel_init` |
| 2 | *(nur x86)* Capability-Bit aus der Boot-Parameter-Struktur prüfen | — | `Q9X_kernel_init` |
| 3 | *(nur x86)* Speichergröße abfragen, eigenen Modul-Header prüfen + relozieren | — | `Q9X_query_memsize` → `Q9X_module_check_reloc` (68K braucht keine Relozierung, s. Thema 00) |
| 4 | Boot-Zeit-Parameter in die Kernel-Globals übernehmen (RAM-Größe, CPU-Typ, ROM-Adresse, Speicherblock für Exception-Tabelle, ...) | `Q9_kernel_init_67a0` — direkt aus CPU-Registern (s. Tabelle oben) | `Q9X_kernel_globals_init` — aus einer Boot-Parameter-Zeigerstruktur |
| 5 | *(nur 68K)* Großflächiges Nullen des Kernel-Global-Bereichs (fast der komplette `0x0000`-`0x1000`-Bereich) | `Q9_kernel_init_67a0` (3× Zero-Fill-Aufruf) | — (im x86-Bootstrap nicht gefunden — vermutlich beim Laden bereits genullt) |
| 6 | *(nur 68K)* CPU-Typ-Selbstcheck: Boot-ROM-Angabe gegen den eigenen, im Modulkopf eingebetteten Namen ("6803") prüfen, sonst Panik | `Q9_kernel_init_67a0` | — |
| 7 | Leere zirkuläre Listen (Bereitschafts-/Wartelisten) initialisieren | `Q9_kernel_init_67a0` — **sechs** Listen (s. o.) | `Q9X_kernel_globals_init` — **vier** Listen |
| 8 | Speicher für die Exception-/Trap-Tabelle bereitstellen | vom Boot-ROM über Register A5 übergeben (s. o.) | selbst aus der eigenen Speichergrößen-Abfrage berechnet — **echter Unterschied** |
| 9 | Exception-/Trap-Dispatch-Tabelle aus kompakter Quelltabelle befüllen | `Q9_kernel_init_67a0` (256 Einträge aus Quelle bei `0x3802`) | `Q9X_dispatch_table_build` (2× aufgerufen) |
| 10 | *(nur 68K)* Syscall-Dispatch-Tabellen (`D_SysDis`/`D_UsrDis`) einrichten | `Q9_kernel_init_67a0` | — (x86-Äquivalent nicht in diesem Bootstrap-Abschnitt identifiziert) |
| 10a | Init-Modul per Namenssuche ("init", Speicherregionen scannen, Sync-/Prüfsummen-Check) finden, Adresse in `D_Init` speichern, Konfigurationsfelder in Kernel-Globals kopieren — **neu, 2026-08-17, s. o.** | `Q9_kernel_init_67a0` (`0x6986`-`0x6a4a`) | nicht in dieser Runde auf der x86-Seite gesucht |
| 11 | *(nur x86)* Weitere Module im Speicher suchen (Modul-Scanner) | — | `Q9X_kernel_globals_init` |
| 12 | *(nur x86)* Prozess-/Pfad-Deskriptor-Tabellen mit Freiliste einrichten | — (68K macht das vermutlich an anderer Stelle, nicht Teil dieses Bootstraps) | `Q9X_kernel_globals_init` |
| 13 | *(nur x86)* Geräte-/Modul-Init-Schleife | — | `Q9X_device_module_init_loop` |
| 14 | *(nur 68K)* Prozess-ID validieren, zwei Deskriptorfelder aufräumen, bedingter `TRAP #0`-Modulaufruf | `Q9_boot_finalize_6de4` (ruft `Q9_proc_id_lookup_2cee`) | — |
| 15 | **Künstlichen Ausführungskontext aufbauen und hineinspringen — Übergabe an Scheduler/ersten Prozess** | `Q9_boot_finalize_6de4` → direkter Sprung in `Q9_reschedule_trampolin_3140` | `Q9X_kernel_globals_init` → Stack-Switch-Primitiv (s. o.) |

**Das Wichtigste in einem Satz:** beide Kernel folgen demselben
Grundmuster — Boot-Parameter übernehmen, Dispatch-Tabellen aus
kompakten Quelltabellen aufbauen, Warteschlangen vorbereiten, dann
künstlich in den ersten Ausführungskontext springen — aber sie tun das
in unterschiedlicher Reihenfolge und mit unterschiedlichen Mitteln
(68K: direkte CPU-Register vom Boot-ROM, harte Sprünge; x86: eine
Boot-Parameter-Zeigerstruktur, ein echter Stack-Wechsel).

## C-Übersetzung

Nur für die x86-Seite sinnvoll versucht (kein 68K-Decompiler in diesem
Projekt im Einsatz) — siehe [`decompiled-x86.c`](decompiled-x86.c).
**Wichtig:** das ist Ghidras eigene Pseudo-C-Ausgabe, keine
handgeschriebene, tatsächlich kompilierbare Quelle — aber sie ist
syntaktisch sauber (mit Typedefs für Ghidras Pseudo-Typen und
Vorwärtsdeklarationen ergänzt) und zeigt genau die Stelle, an der
Ghidras eigene Typinterpretation an ihre Grenzen stößt (`pcVar2`, s. o.).
Fazit zu Andreas' ursprünglicher Vermutung (aus der Architektur-
Diskussion, siehe `KERNEL_INIT.md` Fund 2): C-kompilierter Code lässt
sich tatsächlich leichter automatisch in Funktionen zerlegen als
68K-Assembler — aber bei echten, handgeschriebenen Kontrollfluss-Tricks
(wie diesem Stack-Switch) hilft auch der C-Decompiler nicht automatisch
weiter, man muss trotzdem die rohe Disassemblierung lesen.

## Weitere Dateien in diesem Thema

- [`asm-68k.r`](asm-68k.r) — wortwörtlicher Auszug aus `src/kernel/kernel.r`, `Q9_kernel_init_67a0` bis zum Sprung in `Q9_reschedule_trampolin_3140`
- [`asm-x86.txt`](asm-x86.txt) — frische Ghidra-Disassemblierung mit den neuen `Q9X_`-Namen
- [`decompiled-x86.c`](decompiled-x86.c) — Ghidra-Pseudo-C-Dekompilierung, s. o.

## Quellen

- 68K: [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitte "Fund: Trap-/Exception-Tabellen-Initialisierung..." (Zeile 149) und der `Q9_boot_finalize_6de4`-Abschnitt (Zeile 1643) — dort steht jetzt nur noch ein Verweis auf dieses Thema, die Details stehen hier.
- x86: [`../../../modules/os9000-x86/docs/KERNEL_INIT.md`](../../../modules/os9000-x86/docs/KERNEL_INIT.md) — ebenfalls per Verweis auf dieses Thema aktualisiert.
- Ghidra-Projekte: `/Volumes/SSD1TB/projects/Q9-OS-ghidra/` (68K), `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/` (x86, Funktionen in dieser Runde umbenannt).

**Erstellt**: 2026-08-13, **aktualisiert**: 2026-08-14 (beide offenen Punkte
aus diesem Thema geklärt — 68K-Warteschlangen `Q9_D_ARENA`/`Q9_D_ALMQ1`/
`Q9_D_ALMQ2` und x86-Sprungziel-Formel)
