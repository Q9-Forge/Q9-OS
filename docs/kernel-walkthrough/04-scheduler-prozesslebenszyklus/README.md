# Thema 04: Scheduler & Prozess-Lebenszyklus

Wie wählt der Kernel den nächsten laufenden Prozess aus, wie läuft ein
Kontextwechsel ab, und was passiert bei `F$Fork`/`F$Exit`? Dieses Thema
erweitert einen Bereich, der für 68K schon **vor** dieser Session sehr
tief erforscht wurde (`docs/REVERSE_ENGINEERING.md`), aber nie mit x86
verglichen wurde — genau das holt dieses Thema nach.

## Quellenlage

- **68K**: reine Zusammenfassung bereits vollständig gelesener Funktionen
  aus `docs/REVERSE_ENGINEERING.md` — nichts neu disassembliert.
- **x86**: neue, gezielte Untersuchung im bestehenden Kernel-Ghidra-
  Projekt (`/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/`, aus
  Thema 01), ausgehend von den dort schon bekannten Ready-Queue-Feldern.
  Zwei Funktionen neu benannt und teilweise gelesen — **nicht** das
  gesamte 743-Byte-Herzstück Byte für Byte, siehe "Offene Punkte".

## Label-Konvention

- **68K**: bereits etablierte Namen aus `src/kernel/kernel.r`.
- **x86**: Präfix `Q9X_`, in dieser Runde neu vergeben (`Q9X_scheduler_insert`,
  `Q9X_first_context_finalize`).

## Die zentrale Erkenntnis: Themas 01 letzte Lücke schließt sich weiter

Thema 01 hatte gezeigt, dass `Q9X_kernel_globals_init` über ein manuelles
Stack-Switch-Primitiv (`FUN_0021e5a0`) irgendwohin springt, und dass
Ghidra danach fälschlich einen Aufruf von `FUN_0021f680` anhängt — als
wäre das ein normaler Rückgabewert. Diese Runde zeigt: **derselbe Trick
passiert ein zweites Mal**, direkt am Ende von `FUN_0021f680` (jetzt
`Q9X_first_context_finalize`) — und **diesmal zeigt Ghidras
"fälschlich angehängter Aufruf" tatsächlich auf eine sinnvolle Funktion**:
`Q9X_scheduler_insert`.

```c
/* Ende von Q9X_first_context_finalize (Ghidra-Dekompilierung) */
FUN_0021e5a0();          /* Stack-Switch-Trampolin, Sprungziel-Formel wie in Thema 01 */
Q9X_scheduler_insert();  /* <- das eigentliche Sprungziel, kein normaler Aufruf */
```

Damit ist die Kette jetzt vollständig sichtbar: **`Q9X_kernel_init` →
`Q9X_kernel_globals_init` → (Trampolin) → `Q9X_first_context_finalize` →
(Trampolin) → `Q9X_scheduler_insert`.** Das ist der x86-Moment, der
exakt dem 68K-Sprung in `Q9_reschedule_trampolin_3140` entspricht — beide
Kernel enden ihre Boot-Kette mit der Übergabe an die Prozessverwaltung.

## 68K-Zusammenfassung (aus `docs/REVERSE_ENGINEERING.md`, nicht neu hergeleitet)

**`Q9_scheduler_183a`** (Ready-Queue-Einfügen, vollständig gelesen):
Prozess wird nur eingefügt, wenn er nicht schon aktiv ist. Enthält
**Priority Aging** (globaler Countdown bei `$3c4(a6)`, bei Ablauf werden
alle wartenden Prozesse in ihrem Sortier-Schlüssel angehoben — klassischer
Schutz gegen Verhungern niedrigpriorer Prozesse) und eine mehrstufige
Sortier-Schlüssel-Berechnung (Echtzeit-/Boost-Flag, Systemschwellen).
Zirkuläre Doppel-Verkettung mit Sentinel-Kopf bei `$37c(a6)` (bestätigt
in Thema 01 als `Q9_D_ACTIVQ`).

**Prozess-Terminierung**, dreistufige Kette (alle vollständig gelesen):
1. `Q9_proc_slot_cleanup_25f8` — Deskriptor-Slot aufräumen: schließt
   offene Pfade **über einen echten `TRAP #0`-Aufruf** (`I$Close`-artig),
   räumt zwei Ressourcenlisten auf, setzt `D_Proc` währenddessen
   **temporär auf den sterbenden Prozess** ("der Kernel wird kurzzeitig
   der sterbende Prozess", damit normale prozessbezogene Mechanismen
   greifen).
2. `Q9_proc_resource_free_62da` — gibt zwei pro-Prozess-Ressourcenlisten
   frei (Speicherblock-Chunks, Fixgrößen-Einträge).
3. `Q9_mem_free_5a22` — die eigentliche Speicherfreigabe-Primitive
   (Boundary-Tag-Coalescing, siehe Thema 05).

**Syscall-Zuordnung** (bereits laufzeitverifiziert, siehe
`docs/REVERSE_ENGINEERING.md`, "Alle laufzeitverifizierten
Syscall-Einstiegspunkte"): `F$AProc` = `Q9_scheduler_183a`, `F$RetPD` =
`Q9_proc_id_free_3370`. `F$Fork`/`F$DFork` teilen sich denselben
Deskriptor-Allocator (`Q9_procdesc_alloc_16b6`) und dieselbe
Init-Routine (`0x28aa`) — einziger Unterschied: ob am Ende in den
Scheduler eingereiht wird oder die Debug-Kontrolle übernommen wird.
**`Q9_procdesc_alloc_16b6`/`0x28aa` selbst sind nur benannt, nicht im
Detail gelesen** — ehrliche Lücke schon in der ursprünglichen 68K-Recherche,
nicht in dieser Runde nachgeholt.

## x86-Funde (neu in dieser Runde)

**`Q9X_scheduler_insert`** (743 Byte, teilweise gelesen — Anfang und
mehrere Kernabschnitte, nicht die komplette Funktion):

- Liest die aus Thema 01 bekannten Queue-Felder (`EBX+0x94`, `EBX+0xa4`,
  `EBX+0x50`, `EBX+0x48`).
- **Zeitbasierter Queue-Walk**: vergleicht einen laufenden Tick-Zähler
  (`EBX+0x58`) gegen einen in jedem Queue-Eintrag gespeicherten
  Wake-Zeitpunkt (`Eintrag+0x24`) — bei Fälligkeit wird der Eintrag per
  klassischem Doppel-Verkettungs-Unlink (`next->prev=prev; prev->next=next`)
  aus der Liste entfernt.
- **EFLAGS-basierte kritische Abschnitte** (`PUSHFD`/`CLI`/.../`POPFD`) —
  dasselbe Grundprinzip wie 68K, das Interrupts während der
  Queue-Manipulation maskiert (dort über `ori #$700,sr`/`move d4,sr`).
- Bittest-Muster (`BT ... ,0x6`) auf einem Flags-Feld im Queue-Eintrag —
  strukturell vergleichbar mit 68K's Echtzeit-/Boost-Flag-Prüfung in
  `Q9_scheduler_183a`, nicht bis zur exakten Bit-Bedeutung verifiziert.

**Einordnung:** Die gelesenen Ausschnitte zeigen eindeutig eine
**"wecke fällige, wartende Prozesse"**-Funktion (Timer-Vergleich +
Unlink), nicht eindeutig auch das reine "füge neuen Prozess ein"
(68Ks `Q9_scheduler_183a` macht beides in einer Funktion, abhängig vom
Prozesszustand). Ob x86 das in **einer** Funktion vereint oder auf
mehrere aufteilt, ist mit dem hier gelesenen Ausschnitt nicht
abschließend geklärt — ehrlich als offen markiert.

## Vergleichstabelle

| # | Was passiert | 68K | x86 |
|---|---|---|---|
| 1 | Prozess in Ready-Queue einfügen (mit Prioritäts-Sortierung/Aging) | `Q9_scheduler_183a` | `Q9X_scheduler_insert` (Einfüge-Anteil nicht separat verifiziert, s. o.) |
| 2 | Wartende/schlafende Prozesse zeitgesteuert aufwecken | `Q9_scheduler_183a`, aufgerufen aus Timer-/Alarm-Kontext (10 charakterisierte Aufrufer, `docs/REVERSE_ENGINEERING.md`) | `Q9X_scheduler_insert` — Timer-Vergleich + Unlink direkt im gelesenen Code sichtbar |
| 3 | Kritischer Abschnitt beim Queue-Zugriff | Interrupts maskiert (`ori #$700,sr`) | Interrupts maskiert (`CLI`/`PUSHFD`/`POPFD`) — gleiches Prinzip, andere CPU-Mechanik |
| 4 | Prozess erzeugen (`F$Fork`) | `Q9_procdesc_alloc_16b6` + Init-Routine `0x28aa` (nur benannt, nicht im Detail gelesen — **68K-seitige Lücke**) | **nicht untersucht in dieser Runde** |
| 5 | Prozess beenden (`F$Exit`) | `Q9_proc_slot_cleanup_25f8` → `Q9_proc_resource_free_62da` → `Q9_mem_free_5a22` (komplette Kette gelesen) | **nicht untersucht in dieser Runde** |
| 6 | Übergabe vom Boot-Bootstrap an die Prozessverwaltung | Direkter Sprung `Q9_boot_finalize_6de4` → `Q9_reschedule_trampolin_3140` (Thema 01) | Doppelter Stack-Switch-Trampolin, endet in `Q9X_scheduler_insert` (**dieser Fund**, schließt Thema 01s offene Frage weiter) |

## Offene Punkte

- **`Q9X_scheduler_insert` nicht komplett gelesen** (743 Byte, nur
  Anfang + Kernabschnitte) — insbesondere der Teil, der einen NEUEN
  Prozess einfügt (statt einen fälligen aufweckt), wurde nicht
  eindeutig lokalisiert.
- **`F$Fork`/`F$Exit` auf x86-Seite komplett unbekannt** — in dieser
  Runde nicht gesucht, da der Fokus auf dem Scheduler-Kern lag. Guter
  nächster Schritt für eine Vertiefung, falls gewünscht.
- **68K-seitige Lücke, nicht neu geschlossen**: `Q9_procdesc_alloc_16b6`
  und die Fork-Init-Routine `0x28aa` sind nur benannt, ihr Inhalt wurde
  in der ursprünglichen Recherche nicht gelesen.
- Die genaue Bit-Bedeutung des `BT ...,0x6`-Tests in `Q9X_scheduler_insert`
  (vermuteter Echtzeit-/Boost-Flag-Analog) nicht verifiziert.

## Für den eigenen Kernel (Ergänzung zu `docs/OWN_KERNEL_INIT_PLAN.md`)

Siehe dortige neue Sektion 2a — kurz zusammengefasst: Ready-Queue als
zirkuläre Doppel-Verkettung mit Sentinel-Kopf, Prioritäts-Aging gegen
Verhungern, und Interrupt-maskierte kritische Abschnitte bei jeder
Queue-Manipulation sind bei **beiden** Architekturen unabhängig
voneinander vorhanden — klare Übernahme-Empfehlung. Die 68K-Terminierungs-
Kette (Slot aufräumen → Ressourcen freigeben → Speicher freigeben, mit
echten Syscall-Aufrufen für offene Pfade) ist ein bewährtes, sauber
geschichtetes Muster, das sich auch ohne x86-Vergleich empfiehlt.

## Quellen

- 68K: [`../../REVERSE_ENGINEERING.md`](../../REVERSE_ENGINEERING.md), Abschnitte "Fund: `Q9_scheduler_183a`", "Fund: `FUN_000025f8`", "Fund: `FUN_000062da`", "Fund: die 10 Aufrufer von `Q9_scheduler_183a` charakterisiert", "Fund: Alle laufzeitverifizierten Syscall-Einstiegspunkte"
- x86: [`asm-x86.txt`](asm-x86.txt), Ghidra-Projekt `/Volumes/SSD1TB/projects/Q9-OS-ghidra-os9000-kernel/`, Skript `../../../modules/os9000-x86/ghidra_scripts/kernel_RenameScheduler.java`
- Vorherige Themen: [Thema 01](../01-kernel-bootstrap/) (Boot-Bootstrap, Ready-Queue-Setup), [Thema 00](../00-modul-aufbau-und-header/)

**Erstellt**: 2026-08-14
