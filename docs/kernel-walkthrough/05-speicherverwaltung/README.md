# Thema 05: Speicherverwaltung — der eigentliche Allokator

Thema 01 hatte gezeigt, wie der Kernel beim Boot einen ersten Speicher-
bereich reserviert (Arena-**Setup**). Dieses Thema geht einen Schritt
weiter: was passiert, wenn zur **Laufzeit** Speicher angefordert oder
freigegeben wird (`F$SRqMem`/`F$SRtMem`)? Wie wird ein freier Block
gefunden, wie werden freigegebene Blöcke wieder verschmolzen? Wie
Thema 04 erweitert dieses Thema einen Bereich, der für 68K schon **vor**
dieser Session sehr tief erforscht wurde, aber nie mit x86 verglichen.

## Quellenlage

- **68K**: reine Zusammenfassung bereits vollständig gelesener Funktionen
  aus `docs/REVERSE_ENGINEERING.md` — nichts neu disassembliert.
- **x86**: neue, gezielte Untersuchung im bestehenden Kernel-Ghidra-
  Projekt (`/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/`).
  Gefunden über eine Fehlercode-Suche (s. u.), zwei Funktionen benannt
  und komplett dekompiliert.

## Label-Konvention

- **68K**: bereits etablierte Namen aus `src/kernel/kernel.r`.
- **x86**: Präfix `Q9X_`, in dieser Runde neu vergeben
  (`Q9X_arena_lookup_or_create`, `Q9X_arena_reserve_space`).

## Die zentrale Erkenntnis: identische Fehlercodes, identische Technik — über zwei Architekturen hinweg

Die x86-Funktionen wurden nicht durch Zufall gefunden, sondern gezielt:
alle Funktionen im Kernel wurden nach den fünf Fehlercode-Konstanten
durchsucht, die `docs/REVERSE_ENGINEERING.md` bereits für den 68K-
Allokator dokumentiert hatte. Treffer — **alle fünf Werte identisch**:

| Fehlercode | Bedeutung (68K, bereits dokumentiert) | x86-Fundstelle (diese Runde) |
|---|---|---|
| `0xDB` | "Adresse gehört nicht zu diesem Pool" | `Q9X_arena_lookup_or_create`, Prüfung der Alignment-Bits |
| `0xD2` | "Adresse gehört zu keiner bekannten Region" | `Q9X_arena_lookup_or_create`, zweimal (Kandidatenlisten-Suche + Größenprüfung) |
| `0xAB` | "keine Arena mit ausreichend freiem Speicher" | `Q9X_arena_lookup_or_create`, Freilisten-Traversierung |
| `0xED` | "Pool/Arena-Liste voll bzw. leer" | `Q9X_arena_reserve_space`, zweimal |
| `0xE1` | "angeforderte Größe ist 0" | `Q9X_arena_reserve_space` |

**Das ist über die reinen Typ-Codes (Thema 00) und Dreiklang-Filterwerte
(Thema 02) hinaus ein weiterer, unabhängiger Beleg** für dieselbe
Erkenntnis: OS-9000 hat beim C-Neuschrieb nicht nur Konzepte, sondern
**konkrete Konstanten** aus dem 68K-Original übernommen — hier sogar
interne Fehlercodes, die kein Nutzer je zu Gesicht bekommt, also keinen
Kompatibilitätsdruck von außen hatten. Das spricht dafür, dass Microware
intern eine gemeinsame Fehlercode-Tabelle über beide Produktlinien
hinweg pflegte.

## 68K-Zusammenfassung (aus `docs/REVERSE_ENGINEERING.md`, nicht neu hergeleitet)

Zweistufiges Schema: **Pool → Arena (nach Adressbereich) → nach Größe/
Klasse sortierte Freiliste innerhalb der Arena.**

- **`Q9_mem_alloc_5440`** (Allozieren, vollständig gelesen): First-Fit
  innerhalb der ersten passenden Arena (Klassen-Tag-Filter, Aktiviert-
  Flag, Sperrbit). Bei Größentreffer: exakter Treffer → Block komplett
  aus der Freiliste aushängen; Restfläche übrig → **Split von hinten**
  (ungewöhnlich: der Freilisten-Eintrag behält seine Adresse und wird nur
  verkleinert, der allozierte Bereich ist das *hintere* Ende — erspart
  Neusortierung).
- **`Q9_mem_free_5a22`** (Freigeben, vollständig gelesen): probiert zwei
  Pools nacheinander (`(0x3fc,A6)`, dann `(0x50,A6)+0x390`), **Boundary-
  Tag-Coalescing** mit angrenzenden freien Nachbarblöcken statt neuem
  Freilisten-Eintrag, sauber geklammerte Interrupt-Maskierung an jedem
  Ausstiegspunkt.
- **`Q9_arena_lookup_5bac`** (Arena finden-oder-erzeugen, vollständig
  gelesen): durchsucht die Arena-Liste des Pools nach der Adresse; bei
  Nichttreffer eine zweite Kandidatenliste ab `(0x404,A6)`; bei Treffer
  dort: neuer **42-Byte-Arena-Deskriptor** wird per Kopierschleife aus
  der Kandidatenregion **als Vorlage übernommen** (Template-Mechanismus).
- **`Q9_freelist_bysize_5712`** (größensortierte Freiliste auf
  Arena-Ebene, vollständig gelesen): pflegt eine sekundäre, nach Größe/
  Klasse sortierte Sicht auf die Freiflächen einer Arena.
- **Syscall-Zuordnung nicht abschließend geklärt** (ehrliche Lücke schon
  in der Originalrecherche): Registerkonvention passt thematisch zu
  `F$SRqMem`/`F$SRtMem`, aber kein direkter `TRAP #0`-Handler gefunden —
  die Aufrufer von `Q9_mem_alloc_5440`/`Q9_mem_free_5a22` sind selbst
  schon interne Verwaltungsroutinen, nicht der öffentliche Einstiegspunkt.

## x86-Funde (neu in dieser Runde)

**`Q9X_arena_lookup_or_create`** (781 Byte, komplett dekompiliert, siehe
[`decompiled-x86.c`](decompiled-x86.c)): zwei Teile in einer Funktion.

1. **Arena-Lookup-oder-Erzeugen** (erster Teil): durchsucht eine sortierte
   Arena-Liste nach der passenden Adresse (Alignment-Prüfung über
   `param_3-1`-Bitmaske — dieselbe Zweierpotenz-Rundungslogik wie beim
   68K). Bei Nichttreffer: Kandidatenliste bei `unaff_EBX+0xc4`
   durchsuchen; bei Treffer: neuer **64-Byte-Arena-Deskriptor** über
   `Q9X_arena_reserve_space` reserviert, dann per **16-Dword-Kopierschleife
   als Vorlage aus der Kandidatenregion übernommen** — strukturell
   identisch zum 68K-Template-Mechanismus (`0x5bac`), nur mit 64 statt
   42 Byte Deskriptorgröße.
2. **Freilisten-Insert mit Boundary-Tag-Coalescing** (zweiter Teil, ab
   `LAB_00222d2a`): First-Fit-artige Traversierung mit Verschmelzung
   angrenzender freier Blöcke — entspricht inhaltlich 68Ks
   `Q9_mem_free_5a22`, nur in derselben Funktion untergebracht statt
   einer eigenen.

**`Q9X_arena_reserve_space`** (530 Byte, komplett dekompiliert): rundet
die angeforderte Größe (`neg`/`and`-Muster, **identisches Idiom** zum
68K-Alignment-Runden), ruft eine tiefere, in dieser Runde nicht mehr
umbenannte Primitive (`FUN_002228ac`) — vermutliches Äquivalent zu 68Ks
eigentlichem First-Fit-Allokator `Q9_mem_alloc_5440`, aber nicht
verifiziert — und fällt bei Erschöpfung auf eine erneute Arena-Erzeugung
über `Q9X_arena_lookup_or_create` zurück (gegenseitiger Aufruf zwischen
beiden Funktionen).

## Vergleichstabelle

| # | Was passiert | 68K | x86 |
|---|---|---|---|
| 1 | Speicher anfordern (First-Fit-Suche) | `Q9_mem_alloc_5440` | `FUN_002228ac` (vermutet, nicht verifiziert) über `Q9X_arena_reserve_space` |
| 2 | Größen-Rundung (Zweierpotenz-Alignment) | `neg.l`/`and.l`-Idiom | identisches `neg`/`and`-Idiom in `Q9X_arena_reserve_space` |
| 3 | Speicher freigeben mit Nachbar-Verschmelzung | `Q9_mem_free_5a22` (Boundary-Tag-Coalescing) | zweiter Teil von `Q9X_arena_lookup_or_create` |
| 4 | Arena finden oder neu erzeugen | `Q9_arena_lookup_5bac` (42-Byte-Deskriptor, Template-Kopie) | erster Teil von `Q9X_arena_lookup_or_create` (64-Byte-Deskriptor, Template-Kopie) |
| 5 | Größensortierte Freiliste auf Arena-Ebene | `Q9_freelist_bysize_5712` | in dieser Runde nicht separat identifiziert (evtl. Teil von `FUN_002228ac`) |
| 6 | Fehlercode-Konvention | `0xDB`/`0xD2`/`0xAB`/`0xED`/`0xE1` | **identisch**, alle fünf Werte bestätigt |
| 7 | Öffentlicher Syscall-Einstiegspunkt (`F$SRqMem`/`F$SRtMem`) | nicht gefunden (offene Lücke schon vor dieser Session) | nicht untersucht in dieser Runde |

## Offene Punkte

- **`FUN_002228ac`** (der vermutliche eigentliche First-Fit-Allokator)
  nicht disassembliert/benannt — nächster Kandidat für eine Vertiefung.
- **Öffentlicher Syscall-Einstiegspunkt** für `F$SRqMem`/`F$SRtMem` weder
  bei 68K noch x86 gefunden (68K-Lücke schon vor dieser Session bekannt,
  x86 in dieser Runde nicht gesucht).
- **Größensortierte Freiliste auf x86-Seite** (Äquivalent zu
  `Q9_freelist_bysize_5712`) nicht separat identifiziert — möglicherweise
  in `FUN_002228ac` verborgen.
- Zwei Pools beim 68K (`(0x3fc,A6)` und `(0x50,A6)+0x390`) — auf x86 nur
  ein Pool-Kontext in den gelesenen Funktionen sichtbar; ob x86 ebenfalls
  mehrere Pools kennt, nicht geklärt.

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 2b — kurz zusammengefasst: das zweistufige
Pool→Arena→Freiliste-Schema mit Boundary-Tag-Coalescing und Template-
basierter Arena-Deskriptor-Erzeugung ist ein **über zwei unabhängige
Architekturen hinweg identisch bewährtes** Muster (bis auf die Fehler-
codes hinunter) — klare Übernahme-Empfehlung. Der ungewöhnliche
"Split von hinten"-Trick beim 68K-Allocator ist eine hübsche
Detail-Optimierung, aber kein Muss.

## Quellen

- 68K: [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitte "Fund: `FUN_00005a22`", "Fund: `0x5bac`", "Fund: `0x5440`", "Fund: `0x5712`", "Fund: Versuch, weitere Syscalls zu lokalisieren (`F$SRqMem`)"
- x86: [`asm-x86.txt`](asm-x86.txt), [`decompiled-x86.c`](decompiled-x86.c), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/`, Skript `../../../modules/os9000-x86/ghidra_scripts/kernel_RenameMemAlloc.java`
- Vorherige Themen: [Thema 01](../01-kernel-bootstrap/) (Arena-Setup, `Q9_D_ARENA`), [Thema 04](../04-scheduler-prozesslebenszyklus/) (Prozess-Terminierung ruft `Q9_mem_free_5a22` auf)

**Erstellt**: 2026-08-14
