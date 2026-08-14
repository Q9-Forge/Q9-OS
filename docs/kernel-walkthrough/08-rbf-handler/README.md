# Thema 08: RBF-Handler-Körper — was `I$Read`/`I$Write` wirklich tun

Thema 03 hatte nur die Dispatch-**Tabelle** gefunden (68K bei `M$Exec`,
x86 im `m_idata`-Bereich). Dieses Thema geht einen Schritt weiter: was
steckt **hinter** den zwei wichtigsten Einträgen — `I$Read` und
`I$Write`? Komplett neues Thema, keine 68K-Vorarbeit dazu vorhanden
(`docs/REVERSE_ENGINEERING.md` war rein Kernel-fokussiert) — Ausgangs-
punkt ist `modules/rbf-filemanager/docs/FINDINGS.md` (bestätigte
Tabellen-Offsets).

## Label-Konvention

- **68K**: neue `Q9_`-Namen für RBF (bisher nur Rohbyte-Analyse, kein
  Ghidra-Projekt existierte — jetzt angelegt:
  `/Volumes/SSD1TB/projects/Q9-OS-ghidra-rbf68k/`).
- **x86**: Präfix `Q9X_`, im bestehenden Projekt aus Thema 03
  (`/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-rbf/`) ergänzt.

## Die zentrale Erkenntnis: `I$Write` findet den Treiber über eine Liste mit Funktionszeigern

Der wichtigste Fund dieser Runde liegt auf der x86-Seite:
**`Q9X_rbf_driver_dispatch`** — eine kleine Funktion (64 Byte), die eine
**verkettete Liste aus (Schlüssel, Funktionszeiger)-Paaren** durchläuft,
den passenden Eintrag anhand eines Schlüssels (vermutlich Geräte-/
Treiber-ID) sucht und dessen Handler **per indirektem `CALL`** aufruft:

```asm
0025251a: PUSH [EBP-4]
0025251d: MOV EAX,ESI
0025251f: MOV EDI,[EDX+4]      ; Handler-Funktionszeiger aus dem Eintrag
00252522: CALL EDI             ; <- Sprung in den Treiber-Handler
```

Das ist die x86-Antwort auf die Frage "wie kommt RBF zum Treiber" — ein
**generischer Dispatcher über eine Laufzeit-Liste**, nicht ein einzelner
fester Sprung. `Q9X_rbf_i_write` ruft genau diese Funktion als **ersten
und einzigen** Schritt auf, bevor es den Rückgabewert auswertet — RBF
selbst enthält also für den eigentlichen Schreibvorgang **keine eigene
Blockzugriffs-Logik**, sondern delegiert komplett an diesen Dispatcher.

## 68K: `I$Read` — ein Orchestrator mit ~10 internen Hilfsroutinen

`Q9_rbf_i_read` (124 Byte, sauberer eigenständiger Funktionsbeginn bei
`0x5D0`, siehe [`asm-68k.txt`](asm-68k.txt)) ruft **zehn** interne
Hilfsroutinen (`bsr`) nacheinander auf, jede mit `bcs`-Fehlerprüfung, die
alle zum selben gemeinsamen Ausstiegspunkt (`0x648`) springen. Struktur
klar erkennbar (Sektorgrößen-Berechnung → Sektor lokalisieren → Status-
Flags → eigentlicher Zugriff → Puffer-Handling → Erfolgsmeldung), aber
**die einzelnen Hilfsroutinen selbst wurden nicht gelesen** — das wäre
eine deutlich größere Vertiefung als der Rahmen dieser Runde.

**Wichtiger Fund dabei**: Hilfsroutine `0x1cd6` wird **sowohl von
`I$Read` als auch von `I$Write`** aufgerufen — echter gemeinsamer Code,
kein Zufall (siehe Vergleichstabelle unten).

## 68K: `I$Write` — kein eigener Funktionsanfang, teilt sich Code mit `I$WritLn`

Überraschung beim Disassemblieren: Ghidras kontrollflussbasierte
Funktionsgrenzen-Erkennung ordnet den Tabellenwert `0x778` (`I$Write`)
**nicht** einem eigenen Funktionsanfang zu, sondern einer größeren
Funktion, die bereits bei `0x69c` beginnt — einer Adresse, die **selbst
kein Tabelleneintrag ist**. Naheliegendste Erklärung (siehe
[`asm-68k.txt`](asm-68k.txt) für die volle Disassemblierung): `0x69c` ist
die **gemeinsame Schreiblogik**, die sowohl `I$WritLn` (Einstieg bei
`0x756`, mit vorangehender Trennzeichen-Suche) als auch `I$Write`
(Direkteinstieg bei `0x778`, überspringt die Trennzeichen-Logik) beide
ansteuern — dasselbe "eine Routine, mehrere Einsprungpunkte"-Prinzip,
das diese Session beim 68K-Kernel schon mehrfach gefunden hat (Thema 01).

## Vergleichstabelle

| # | Frage | 68K | x86 |
|---|---|---|---|
| 1 | Wie wird auf den Datenträger zugegriffen? | Über ~10 interne Hilfsroutinen, nicht im Detail gelesen — eine davon (`0x1094`) ist der wahrscheinlichste Kandidat für den echten Blockzugriff | **Über `Q9X_rbf_driver_dispatch`** — Laufzeit-Liste aus (Schlüssel, Funktionszeiger), indirekter `CALL` (**dieser Fund**) |
| 2 | Teilt sich `I$Read` mit anderen Handlern Code? | Nicht untersucht (eigener, sauberer Funktionsanfang) | Nicht zutreffend — `I$Read` ist in diesem Speicherzustand ein 5-Instruktionen-Stub |
| 3 | Teilt sich `I$Write` mit anderen Handlern Code? | **Ja** — gemeinsame Routine mit `I$WritLn` ab `0x69c` (dieser Fund) | Nein — eigenständige 70-Byte-Funktion |
| 4 | Fehlerbehandlung | `bcs`-Prüfung nach jedem Hilfsroutinen-Aufruf, gemeinsamer Ausstiegspunkt | Ergebnis von `Q9X_rbf_driver_dispatch` gegen Konstante `0xD0` geprüft, bei Treffer Sonderbehandlung über `Q9X_rbf_write_error_retry` |
| 5 | Gemeinsamer interner Code zwischen Read/Write | **Ja** — Hilfsroutine `0x1cd6` in beiden verwendet | Nicht untersucht (unterschiedliche Codepfade: Stub vs. echte Funktion) |

## `I$Read` auf x86: bestätigt trivialer Stub — ehrlich offen, warum

Thema 03 hatte bereits vermutet, dass `Q9X_rbf_i_read` ein Stub sein
könnte. Diese Runde bestätigt es vollständig: die Funktion besteht aus
genau 5 Instruktionen (`PUSH EBP; XOR EAX,EAX; MOV EBP,ESP; LEAVE; RET`)
— kein Aufruf, keine Verzweigung, nur `return 0`. **Nicht geklärt**, ob
das am extrahierten Live-Speicherzustand liegt (z. B. ein Feature-Flag,
das zur Extraktionszeit den "echten" Read-Pfad deaktiviert hatte) oder
ob dieser Slot in diesem konkreten Build grundsätzlich anders
funktioniert (z. B. über einen separaten Cache-Layer, der `I$Read` nie
direkt aufruft). Ein zweiter Live-Extraktions-Durchlauf mit aktivem
Dateizugriff während der Extraktion wäre der nächste sinnvolle Schritt,
falls das geklärt werden soll.

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 3a — kurz zusammengefasst: `Q9X_rbf_driver_dispatch`s
Muster (Datenträgerzugriff über eine **Laufzeit-Liste aus (Geräte-ID,
Handler-Zeiger)-Paaren mit indirektem Aufruf**, statt eines fest
verdrahteten Sprungs) ist eine klare Übernahme-Empfehlung — flexibler als
68Ks Ansatz (mehrere interne Hilfsroutinen, deren genaue Aufteilung nicht
mehr rekonstruierbar war) und passt zum bereits in Thema 01 gefundenen
x86-Modul-Scanner-Prinzip (zur Laufzeit statt fest verdrahtet).

## Offene Punkte

- 68K: die ~10 `I$Read`-Hilfsroutinen nicht im Detail gelesen — welche
  davon den eigentlichen Hardware-/Cache-Zugriff macht, ist eine
  begründete Vermutung (`0x1094`/`0x386`/`0xf90`), nicht verifiziert.
- x86: `Q9X_rbf_write_error_retry` nur die ersten ~13 Instruktionen
  gelesen, nicht die volle 88-Byte-Funktion.
- x86: Bedeutung des Fehlercodes `0xD0` nicht identifiziert.
- x86: warum `Q9X_rbf_i_read` ein Stub ist, bleibt offen (s. o.).
- Der letzte Schritt (Treiber → tatsächliche Hardware, `cfide`/x86-
  Äquivalent) ist Thema 09, hier nicht mehr verfolgt.

## Quellen

- 68K: [`modules/rbf-filemanager/docs/FINDINGS.md`](../../../modules/rbf-filemanager/docs/FINDINGS.md) (Tabellen-Offsets), [`asm-68k.txt`](asm-68k.txt), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-rbf68k/`, Skript `../../../modules/rbf-filemanager/ghidra_scripts/DumpReadWriteHandlers.java`
- x86: [`asm-x86.txt`](asm-x86.txt), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-rbf/`, Skripte `../../../modules/os9000-x86/ghidra_scripts/rbf_{DumpReadWriteHandlers,PeekWriteHelper,RenameWriteHelpers}.java`
- Vorheriges Thema: [Thema 03](../03-dreiklang/) (Dispatch-Tabelle, Slot-Adressen)

**Erstellt**: 2026-08-14
