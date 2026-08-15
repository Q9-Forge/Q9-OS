# Erster Entwurf: was der eigene Q9-Kernel initialisieren muss

**Status (2026-08-14, von Andreas bestätigt):** Die frühere "kein Neubau"-
Beschränkung ist aufgehoben — es sollte erst eine längere Planungs-/
Recherchephase geben, die jetzt zu Ende geht. Ziel ist ein **eigener
Kernel**. Dabei gilt eine **verbindliche, nicht verhandelbare Anforderung**:
**echte, bestehende OS-9/68K- UND OS-9000-Module müssen weiterhin
laufen** — Andreas' Formulierung: "Eine Kompatibilität zu echten
OS-9/9000 Modulen will ich auf jeden Fall haben." Zusätzliche, eigene
Modularten sind ausdrücklich willkommen ("da bin ich offen für") — es
geht also nicht um "eigenes Format ODER Kompatibilität", sondern um
"Kompatibilität ALS Fundament, eigene Erweiterungen obendrauf".

**Damit ist dieses Dokument** eine grobe Anforderungsliste, kein Bauplan
und kein Quellcode. Grundlage sind die vier Themen des
[Kernel-Walkthroughs](kernel-walkthrough/) — dem systematischen Vergleich
des OS-9/68K-Kernels (`dker030s`, dieses Projekts bisherige Referenz) mit
dem OS-9000/x86-Kernel (Microwares spätere, portable C-Neufassung).

**Offener technischer Punkt, der aus Andreas' Antwort folgt und noch zu
klären ist:** "Kompatibilität zu echten Modulen" kann zwei sehr
unterschiedliche Dinge bedeuten:

1. **Strukturelle Kompatibilität** — der eigene Kernel versteht das
   Header-/Link-/Dispatch-Format (dieses Dokument), kann also echte
   Module *identifizieren, linken, ihre Metadaten lesen* — aber der
   eigentliche Modul-**Code** müsste trotzdem für die Zielarchitektur des
   eigenen Kernels vorliegen (neu kompiliert oder per Interpreter
   ausgeführt).
2. **Binäre Ausführungskompatibilität** — der eigene Kernel führt den
   **unveränderten Maschinencode** echter Module tatsächlich aus. Für
   68K-Module ist das machbar (Q9-Flux hat mit Musashi bereits einen
   68K-Interpreter). **Für OS-9000/x86-Module braucht das zusätzlich
   einen x86-Interpreter/-Emulator** — den gibt es in Q9-Flux aktuell
   nicht (Musashi emuliert ausschließlich 68K). Andreas' Formulierung
   "alte OS-9000 Module ausführen können" deutet auf Bedeutung 2, nicht
   nur 1 — das wäre eine substanzielle zusätzliche Komponente (x86-CPU-
   Emulation neben der bestehenden 68K-Emulation), keine Kleinigkeit.

Dieses Dokument geht im Folgenden von Bedeutung 1 als **Minimalziel**
aus (das ist ohnehin nötig für 68K-Kompatibilität) und markiert an den
relevanten Stellen, wo Bedeutung 2 zusätzliche Arbeit bedeuten würde.

## 1. Modulformat — was ein Modul-Header mindestens braucht

Aus [Thema 00](kernel-walkthrough/00-modul-aufbau-und-header/): drei
untersuchte Header-Generationen (6809/68K/OS-9000), aber ein Kern, der
über alle drei ~45 Jahre hinweg gleich blieb. **Diese Felder muss der
eigene Kernel byte-genau verstehen können, um echte Module zu erkennen
und zu linken** — das ist jetzt keine Empfehlung mehr, sondern die
technische Grundlage der zugesagten Kompatibilität:

| Feld | Zweck | Byte-genaue Kompatibilität nötig? |
|---|---|---|
| Sync-Konstante | Modul im Speicher/auf Platte erkennen | Ja — beide bekannten Werte (`$4AFC` 68K, `$FC4A`/`$4AFC` x86-gespiegelt) müssen erkannt werden, um Module beider Familien zu finden |
| Modulgröße | Wie viele Bytes gehören zum Modul | Ja |
| Name-Offset + Namensstring | Modul über Namen linken (`F$Link`) | Ja |
| **Typ-Code** | Welche Rolle hat das Modul (System/Fmgr/Driver/Descriptor) | **Ja, exakt `0x0C`/`0x0D`/`0x0E`/`0x0F`** — das ist die am längsten unveränderte Konstante der ganzen OS-9-Familie (6809→68K→x86, ~28 Jahre identisch) und die Grundlage des Dreiklangs (Abschnitt 3) |
| Einsprungoffset (`M$Exec`/`m_exec`) | Wo beginnt der Modulcode | Ja — Position im Header unterscheidet sich zwischen 68K (`$30`) und OS-9000 (`$24`), der Parser muss beide Layouts kennen |
| Prüfsumme | Beschädigte Module erkennen | Empfehlenswert für eigene neue Module, für das reine Erkennen/Linken echter Module nicht zwingend |

**Für eigene, zusätzliche Modularten** (von Andreas ausdrücklich gewünscht):
freie Wahl, mit einer Einschränkung — der Sync-Wert-Scan beim Boot (Punkt
"Modul-Scanner" in Abschnitt 2) muss zwischen "das ist ein 68K-Modul",
"das ist ein OS-9000-Modul" und "das ist ein eigenes Q9-Modul"
unterscheiden können, sich also über einen eigenen, vierten Sync-Wert
identifizieren, der nicht mit den beiden bestehenden kollidiert.

Weder 68Ks 46-Byte-Minimalheader noch OS-9000s 88-Byte-Universalheader
müssen dabei als internes Modell übernommen werden — der Kernel kann
intern EIN gemeinsames Verständnis (z. B. angelehnt an `src/q9moduleheader.h`,
das schon alle drei Layouts als Parser-Definitionen enthält) nutzen und
beim Lesen eines Moduls anhand des Sync-Werts entscheiden, welches der
bekannten Layouts gilt.

**Eigenes Q9-Header-Format entworfen (2026-08-16, 💡, in
`src/q9moduleheader.h` als Layout 4 festgehalten):** eigener Sync-Wert
(`$5139`, ASCII "Q9", kollisionsfrei zu `$87CD`/`$4AFC`), `type`-Feld auf
16 Bit erweitert (`0x00`-`0x0F` wortidentisch zu den Legacy-Typcodes,
`0x0C`-`0x0F` weiterhin zwingend für den Dreiklang reserviert, `0x10`+
frei für eigene Modularten), und ein selbstbeschreibendes `abiClass`-Byte
direkt nach einem `hdrVersion`-Byte (noch vor jedem breitenabhängigen
Feld, nach ELF-`EI_CLASS`-Vorbild) — kodiert Pointerbreite (16/32/64 Bit)
UND Endianness in einem Feld, fünf real genutzte Kombinationen
(16BE/32BE/32LE/64BE/64LE; 16-Bit fürs 6809-Erbe, absichtlich nur
Big-Endian). Ein optionaler Erweiterungsblock (über `hdExtOffset`/
`hdExtSize`, dasselbe Prinzip, das der 68K-Header schon selbst mitbringt)
trägt erweiterte Owner/Group/World-Rechte, SMP-Metadaten
(`cpuAffinityMask`, `smpFlags`), Zielarchitektur-Kennung und das
Adressierungsmodell-Flag (Bezug: Abschnitt 2d/6). Details, Bitlayouts und
die drei Struct-Varianten (`Q9_ModHeadOwn16/32/64`): siehe
`src/q9moduleheader.h`. Noch offen: konkrete Feldbelegung für
Abschnitt-5-Punkt-5 (welche eigenen Modularten `0x10`+ bekommen).

## 2. Boot-Init-Reihenfolge — was beide Kernel in derselben Grundform tun

Aus [Thema 01](kernel-walkthrough/01-kernel-bootstrap/) (komplette
15-Schritte-Tabelle dort) — hier verdichtet auf die Schritte, die **in
beiden Kernen unabhängig voneinander vorkommen** (das ist das eigentliche
Signal: zwei komplett unabhängige Implementierungen kamen auf dieselbe
Reihenfolge):

1. **Einsprung über einen kurzen Stub**, der einen eingebetteten ID-/
   Copyright-String überspringt (nicht funktional notwendig, aber beide
   tun es — reine Konvention, kein Muss für einen eigenen Kernel).
2. **Kernel-Globals-Basis etablieren** — ein fester Ort (bzw. bei x86: ein
   fester Zugriffsweg über ein Segmentregister), über den der gesamte
   restliche Kernelcode auf seinen globalen Zustand zugreift. **Muss-
   Anforderung**, unabhängig vom Weg: irgendein Mechanismus, der diesen
   Zeiger für den kompletten Kernel und alle Systemmodule einheitlich
   verfügbar macht (68K: Register A6 fest reserviert; x86: `FS`-Segment).
   Für 68K/Q9-Flux ist "festes Adressregister" die naheliegende Wahl, weil
   die Zielplattform sowieso 68K ist.
3. **Speichergröße ermitteln, Arena/Freispeicher-Verwaltung aufsetzen** —
   beide runden auf **16-Byte-Granularität**. Das ist wahrscheinlich kein
   Zufall (68K-Alignment-Anforderungen, aber x86 hätte das nicht gebraucht
   und tut es trotzdem) — spricht für "als bewusste Design-Konstante
   übernehmen", nicht neu erfinden.
4. **Exception-/Trap-Dispatch-Tabelle aus einer kompakten Quelltabelle
   aufbauen.** Beide Kernel tragen NICHT die volle Dispatch-Tabelle direkt
   im Modul — sie tragen eine kompakte Liste (68K: `(count,offset)`-Paare;
   x86: 14-Byte-Quelleinträge) und expandieren sie erst beim Boot in eine
   größere, direkt indizierbare Tabelle. **Klare Übernahme-Empfehlung**:
   dieses Muster spart Modulgröße und ist bei beiden Architekturen absicht-
   lich so gewählt.
5. **Leere, zirkuläre Bereitschafts-/Warteschlangen anlegen** (68K: sechs
   — aktiv/schlafend/wartend plus Arena-Kontrollblock plus zwei Alarm-
   Warteschlangen; x86: vier, ähnliches Konzept). Kopf=Schwanz=sich selbst
   bei leerer Liste ist bei beiden das Terminierungsmuster.
6. **Prozess-/Pfad-Deskriptor-Tabellen mit einer Freiliste einrichten** —
   beide markieren freie Slots mit einem Sentinel-Muster (x86: `0xFFFF`
   plus ein Zeiger auf einen gemeinsamen "leer"-Handler; 68K: ähnliches
   Tag-Prinzip an anderer Stelle im Code). Feste Obergrenze (`0x80`=128
   bei x86) statt dynamischem Wachstum — für einen eigenen, eher
   ressourcenbeschränkten Zielsystem-Kontext ebenfalls die pragmatischere
   Wahl.
7. **Ersten Ausführungskontext von Hand konstruieren und hineinspringen**
   — beide Kernel bauen sich am Ende einen künstlichen Rücksprung-/
   Stack-Rahmen (68K: direkter Sprung in den Scheduler-Trampolin; x86: ein
   manueller Stack-Wechsel über `XCHG`/`JMP`). Das ist der Moment, an dem
   der Kernel "sich selbst verlässt" und in Prozess-Kontext weiterläuft.
   **Für einen eigenen Kernel:** dieser Schritt ist konzeptionell zwingend
   (irgendwie muss der erste Prozess starten), der genaue CPU-Trick ist
   Implementierungsdetail.

**Was NUR beim x86-Kernel vorkommt** (aus Thema 01, Zeilen 2/3/11/12/13
der dortigen Tabelle) und **wahrscheinlich eine echte Verbesserung ist,
die man übernehmen sollte**: ein **Modul-Scanner**, der beim Boot den
Speicher nach weiteren Modulen absucht (Sync-Byte-Muster, Prüfsumme,
Namensvergleich bei Mehrfachtreffern die höchste Revision behält) — das
ist die Grundlage dafür, dass der x86-Kernel zur Boot-Zeit selbstständig
findet, was sonst noch geladen ist, ohne dass eine feste Adresse verdrahtet
werden muss. Der 68K-Kernel bekommt seine Module dagegen off-Rekord
(vermutlich vom Boot-ROM/Init-Modul vorverlinkt) — für einen eigenen
Kernel ist der x86-Ansatz (Scanner statt Festverdrahtung) die robustere
Wahl, unabhängig von der Zielarchitektur.

**Was NUR beim 68K-Kernel vorkommt** und eher CPU-spezifisch als
übernehmenswert ist: der CPU-Typ-Selbstcheck (Boot-ROM-Angabe gegen
Modulkopf-String) und das großflächige Nullen des Kernel-Global-Bereichs
(bei x86 offenbar schon beim Laden erledigt) — beides plausibel
notwendig, weil 68K-Boot-ROMs keine Garantie über den Anfangszustand des
RAM geben; für Q9-Flux (wo der Bootvorgang bekannt/kontrolliert ist)
prüfenswert, ob das überhaupt nötig ist.

## 2a. Scheduler & Prozess-Lebenszyklus — nach dem Boot

Aus [Thema 04](kernel-walkthrough/04-scheduler-prozesslebenszyklus/):
Thema 01 endet mit der Übergabe an den Scheduler — dieser Abschnitt
beschreibt, was danach dauerhaft läuft. Auch hier: zwei unabhängige
Implementierungen, dieselben Grundmuster.

- **Ready-Queue als zirkuläre Doppel-Verkettung mit Sentinel-Kopf** —
  bei beiden Architekturen. **Klare Übernahme-Empfehlung.**
- **Prioritäts-Aging**: ein globaler Countdown, der bei Ablauf alle
  wartenden Prozesse in ihrem Sortier-Schlüssel anhebt — Schutz gegen
  Verhungern niedrigpriorer Prozesse (68K vollständig gelesen, x86 nur
  in Ansätzen bestätigt: ein Timer-Vergleich pro Queue-Eintrag existiert
  nachweislich). **Übernahme-Empfehlung**, da ein einfacher, bewährter
  Mechanismus gegen ein reales Scheduling-Problem.
- **Jede Queue-Manipulation läuft in einem interrupt-maskierten
  kritischen Abschnitt** (68K: `ori #$700,sr`/Wiederherstellen; x86:
  `PUSHFD`/`CLI`/.../`POPFD`) — **Muss-Anforderung**, unabhängig von der
  Zielarchitektur: ohne das sind Ready-Queue-Operationen nicht
  interrupt-sicher.
- **Prozess-Terminierung als geschichtete Kette** (68K vollständig
  gelesen, x86 auf dieser Ebene noch nicht untersucht): Deskriptor-Slot
  aufräumen (inkl. **echtem Syscall-Aufruf** zum Schließen offener Pfade
  — der Kernel nutzt hier seinen eigenen öffentlichen `I$Close`-Pfad,
  keine interne Abkürzung) → pro-Prozess-Ressourcenlisten freigeben →
  eigentliche Speicherfreigabe (Thema 05). Diese Schichtung (jede Ebene
  kennt nur die nächsttiefere, nicht die Speicherverwaltung direkt) ist
  ein sauberes Vorbild, unabhängig vom exakten Byte-Layout.
- **Offen für beide Architekturen**: wie `F$Fork` im Detail abläuft
  (Deskriptor-Allocator und Init-Routine sind beim 68K nur benannt, nicht
  gelesen; auf x86 komplett unbekannt) — falls das für den eigenen
  Entwurf relevant wird, lohnt sich hier eine weitere Vertiefungsrunde
  auf beiden Seiten, bevor man sich auf eine Prozesserzeugungs-Reihenfolge
  festlegt.

## 2b. Speicherverwaltung — der eigentliche Allokator

Aus [Thema 05](kernel-walkthrough/05-speicherverwaltung/): über die
Boot-Zeit-Arena aus Abschnitt 2 hinaus die Laufzeit-Logik für
`F$SRqMem`/`F$SRtMem`. Stärkster Einzelbefund der ganzen Serie: **alle
fünf 68K-Fehlercodes für Speicherverwaltung (`0xDB`/`0xD2`/`0xAB`/`0xED`/
`0xE1`) tauchen unverändert im x86-Kernel wieder auf** — interne Codes,
die kein Nutzer je sieht, also ohne jeden externen Kompatibilitätsdruck
übernommen. **Klare Übernahme-Empfehlung, unabhängig vom Pfad**: diese
Werte sind offenbar eine über Jahrzehnte gepflegte interne Konvention,
kein Zufall.

- **Zweistufiges Schema Pool → Arena (nach Adressbereich) → Freiliste**
  (nach Größe/Klasse sortiert) — bei beiden Architekturen unabhängig
  vorhanden. **Übernahme-Empfehlung.**
- **Arena-Deskriptoren werden per Template-Kopie erzeugt**: eine
  Kandidatenregion wird byteweise in einen neuen, festgrößigen
  Deskriptor kopiert (68K: 42 Byte; x86: 64 Byte) statt ihn von Grund
  auf neu zu berechnen — ein einfacher, an beiden Architekturen
  gefundener Mechanismus.
- **Boundary-Tag-Coalescing beim Freigeben**: angrenzende freie Blöcke
  werden verschmolzen statt neue Freilisten-Einträge anzulegen — Standard-
  technik, aber konkret bei beiden Kernen bestätigt.
- **Größen-Rundung per Zweierpotenz-Bitmaske** (`neg`/`and`-Idiom) — bei
  beiden Architekturen identisch, unabhängig davon ob 16-Byte- (Thema 01)
  oder größere Alignments gefragt sind.
- **Offen für beide Architekturen**: der öffentliche `F$SRqMem`/
  `F$SRtMem`-Einstiegspunkt selbst wurde nie gefunden (68K-Lücke schon
  vor dieser Session bekannt) — für den eigenen Kernel kein Hindernis
  (der eigene Einstiegspunkt kann frei gewählt werden), aber ein Hinweis,
  dass die Syscall-Tabellen-Befüllung beim Boot noch nicht vollständig
  verstanden ist, falls das für Pfad (B) relevant wird.

## 2c. Exception-/Trap-Handler — wichtige Design-Lehre aus einem Fehlschlag

Aus [Thema 06](kernel-walkthrough/06-exception-handler/): das Prinzip
"kompakte Quelltabelle → beim Boot zur vollen Dispatch-Tabelle
expandiert" (schon in Abschnitt 2, Punkt 4 als Übernahme-Empfehlung
genannt) wurde hier ein zweites Mal bestätigt — der x86-Tabellen-
**Builder** arbeitet exakt nach demselben Schema wie beim 68K.

**Eine echte, für den eigenen Entwurf relevante Design-Lehre** ergibt
sich aber aus dem, was in dieser Runde NICHT gefunden werden konnte: die
x86-Quelltabelle liegt **zur Laufzeit in dynamisch allozierten
Kernel-Globals**, nicht **statisch im Moduldateiabbild** wie beim 68K
(Offset `0x3802`) oder wie die RBF-Callcode-Tabelle aus Thema 03
(`m_idata`-Bereich). Das machte die x86-Quelltabelle mit reiner
Datei-Analyse praktisch unmöglich nachzuvollziehen — nur mit einem
laufenden Emulator und Live-Speicher-Auslesen wäre das möglich gewesen.

**Für den eigenen Kernel:** eine **statische** Quelltabelle im Modul
(68K-Stil) ist eindeutig die bessere Wahl — sie bleibt offline mit
einem Disassembler/Hex-Editor nachvollziehbar und debugbar, ohne einen
laufenden Emulator zu brauchen. Das ist ein direkter, aus einem eigenen
Rechercheergebnis abgeleiteter Grund, hier NICHT dem x86-Muster zu
folgen, obwohl x86 in anderen Punkten (z. B. Modul-Scanner, Abschnitt 2)
die bessere Vorlage war — nicht jede x86-Design-Entscheidung ist
automatisch die modernere/bessere.

Auch bemerkenswert: das komplette x86-Kernelmodul enthält **keine
einzige `INT`- oder `IRET`-Instruktion** — der tatsächliche Auslöse-
Mechanismus für einen Syscall (Software-Interrupt? Call-Gate? etwas
anderes?) bleibt für x86/OS-9000 ungeklärt. Für den eigenen 68K-Kernel
ist das ohnehin irrelevant (`TRAP #0` ist die naheliegende, bereits vom
68K-Vorbild bewährte Wahl), aber ein Hinweis darauf, dass die x86-
Vergleichsseite hier an eine echte Erkenntnisgrenze gestoßen ist.

**Nachtrag (Thema 09):** der Auslöser wurde inzwischen doch gefunden —
nur nicht im Kernel-Modul selbst, sondern auf der aufrufenden Seite in
einem Treiber-Modul: ein echter `INT 0xFF`. Details in Abschnitt 3b.

## 2d. MMU/Speicherschutz — SSM als eigenständiges, optionales Modul

Aus [Thema 07](kernel-walkthrough/07-ssm-mmu/), direkte Antwort auf
Andreas' Frage "wann wird die MMU initialisiert": **weder vom Kernel
selbst noch von Treibern/Deskriptoren** — bei beiden Referenzarchitekturen
übernimmt das ein **eigenständiges, separat geladenes Systemmodul**
(68K/OS-9000-Konvention: `SSM`, "System Security Module"), laut Manual
vom Init-Modul geladen, nicht Teil des Kernel-Bootstraps aus Thema 01.

**Klare Übernahme-Empfehlung für den eigenen Kernel**: MMU-/Speicherschutz-
Verwaltung als **optionales, nachladbares Modul** behandeln, nicht als
Kernel-Pflichtbestandteil — das deckt sich mit der bereits dokumentierten
Atomic-/Development-Kernel-Unterscheidung (Atomic kommt laut Manual ganz
ohne SSM aus, nur Development nutzt es für User-State-Speicherschutz).
Für Q9-Flux (68K) heißt das konkret: ein eigenes SSM-artiges Modul wäre
zuständig für Function-Code-basierte Zugriffssteuerung (`MOVEC SFC`,
User-Data vs. Super-Data) und würde eine seitengrößenartige Konstante in
den Kernel-Global-Bereich schreiben (68K-Vorbild: `0x1000` in denselben
Offset, den `src/q9sysglob.h` bereits als `Q9_D_BLKSIZ` führt).

**Ehrlich offen**: weder beim 68K- noch beim x86-Vorbild wurde in dieser
Runde das eigentliche Programmieren der Übersetzungstabellen vollständig
verifiziert (68K: keine `PMOVE`-Instruktion in `ssm851` gefunden; x86: nur
`CR3`-Zugriffe in einer vermuteten Adressraum-Freigabe-Routine, nicht in
einer erkennbaren Erst-Initialisierung) — für eine konkrete Umsetzung
bräuchte es eine weitere Vertiefungsrunde, keine reine Übernahme aus den
hier gefundenen Ausschnitten.

**Ergänzung 2026-08-15** (Quelle: `Q9-Flux/docs/MMU_SSM_WORKFLOW_de.md` +
`68k_tech.pdf`, kein Kernel-Walkthrough-Thema, sondern ein Planungsgespräch):
zwei bis dahin unklare Punkte konkretisiert.

1. **SSM baut echte Übersetzungstabellen, keine reine Schutzprüfung ohne
   Umsetzung.** Laut `MMU_SSM_WORKFLOW_de.md` legt SSM einen vollständigen
   68030-PMMU-Deskriptorbaum an (Root → Table A → B → C → D) mit getrennten
   Supervisor- und User-Root-Pointern (`SRP`/`CRP`). Für den eigenen Kernel
   heißt das: eine **portable Seitentabellen-Abstraktionsschicht** ist
   nötig (68K-Kompatibilitätspfad SSM-bytegetreu, native Ziele mit
   äquivalenter Semantik) — kein freier Entwurf zwischen "flach" und
   "virtuell", die Altlast ist bereits virtuell. Relevante Syscalls dafür,
   früh zu priorisieren zusammen mit dem Modul-Laden: `F$ChkMem`,
   `F$SRqMem`, `F$MapBlk`, `F$Trans`.
2. **Geräte-Fenster werden NICHT beim Boot vorab eingerichtet, sondern
   verzögert beim ersten `I$Attach`.** Zitat `68k_tech.pdf`: "OS-9 links to
   its file manager and device driver... the driver's initialization
   routine is called to initialize the hardware... **The kernel attaches
   all devices at open and detaches them at close.**" Die Init-Routine
   eines Treibers (Dispatch-Tabellen-Slot 0) läuft also nur beim ersten
   `I$Open`/`I$Create` auf dieses Gerät — dort würde `F$MapBlk` das
   physische Fenster bei SSM anmelden, nicht der Boot-ROM und nicht der
   Kernel-Bootstrap. Für ein paar einzelne I/O-Register ist trotzdem kein
   gesondertes `F$MapBlk` nötig: Treiber-Routinen laufen innerhalb eines
   `TRAP` (S-Bit gesetzt, Supervisor-Root-Tabelle), die laut Adresskarte in
   `MMU_SSM_WORKFLOW_de.md` den ganzen I/O-Cluster-Bereich pauschal als
   System-State abdeckt. Explizites `F$MapBlk` in eine *User*-Tabelle
   braucht nur, was auch direkt aus User-Code heraus beschreibbar sein
   soll (z. B. ein Framebuffer für performanten Direktzugriff ohne
   Syscall pro Pixel). **Praktische Konsequenz für Q9-Flux:** die
   bisherige Praxis, ROM/RAM/IO-Bereiche im Emulator/Bootprozess pauschal
   vorzumappen, entspricht nicht diesem Modell — Andreas hat das bereits
   gestoppt; nächster Schritt ist, den vorhandenen Framebuffer-Treiber so
   zu erweitern, dass er sein Speicherfenster selbst per `F$MapBlk` in
   seiner Init-Routine anmeldet, statt sich auf Emulator-seitiges
   Vor-Mapping zu verlassen (noch nicht umgesetzt).

## 2e. Boot-Vorkette — was vor dem Kernel-Einsprung passiert (aus Thema 10)

Aus [Thema 10](kernel-walkthrough/10-boot-vorkette/): alle bisherigen
Abschnitte (2-2d) setzen voraus, dass der System-Global-Bereich beim
Kernel-Einsprung bereits existiert — dieser Abschnitt klärt, wer ihn
anlegt. Antwort, gefunden durch Disassemblierung der echten Q9-Flux-
Boot-ROM-Datei: **der Boot-ROM selbst**, in einer eigenen Routine vor
jedem Kernel-Code — eine `DBF`-Schleife nullt einen ca. 19,5-KByte-
Bereich in **16-Byte-Schritten** (dieselbe Granularität wie Abschnitt 2,
Punkt 3 — jetzt ein drittes Mal, eine Ebene *unter* dem Kernel,
bestätigt).

**Klare Übernahme-Empfehlung für den eigenen Kernel:** diese
Verantwortung sauber trennen — ein eigener Q9-Bootlader (nicht der
Kernel selbst) legt den System-Global-Bereich an und nullt ihn, bevor er
den Kernel überhaupt anspringt. Das entlastet den Kernel-Bootstrap
(Abschnitt 2) von dieser Aufgabe und spiegelt exakt, was das 68K-Vorbild
tut.

**Zweiter Fund, ebenfalls klare Übernahme-Empfehlung:** der Boot-ROM
validiert Boot-Kandidaten (Module in der Bootdatei) mit **demselben**
Sync-Wort- und Prüfsummen-Verfahren, das auch beim regulären Modul-Linken
zur Laufzeit gilt (Sync `$4AFC`, 24-Word-XOR-Prüfsumme über Offset
`0x00`-`0x2F`, muss `0xFFFF` ergeben) — kein separates Bootfile-Format
nötig, ein einziges Validierungsverfahren für beide Fälle.

**Dritter Fund, eine Design-Lehre:** die Fehlerbehandlung des 68K-Boot-
ROMs ist bewusst simpel — bei jedem grundlegenden Fehler (RAM-Test
fehlgeschlagen, Bootfile ungültig) wird **kein** selektives Recovery
versucht, sondern die komplette ROM-Logik neu gestartet. Für die
allerfrüheste Boot-Phase (bevor irgendein Dateisystem, Scheduler oder
Fehlerbehandlungs-Infrastruktur existiert) ist das plausibel die
robustere Wahl als ein komplexer Fehlerbehandlungspfad, der selbst
fehlerhaft sein könnte.

**x86-Seite, neuer Baustein:** `vectx86` — ein bisher unbekanntes,
eigenständiges Modul (nicht Teil des Kernels), das feste Handler-Adressen
in Kernel-Globals-Felder installiert. Bestätigt außerdem ein zweites Mal,
unabhängig von `scllio` (Abschnitt 3b), dass `INT 0xFF` der generische
x86-Systemaufruf-Mechanismus ist — kein treiberspezifischer Einzelfall.
Kein echtes BIOS-/IPL-Äquivalent zum 68K-Boot-ROM wurde auf der x86-Seite
gefunden (liegt vermutlich außerhalb des gesamten Microware-Modulfundus,
in QEMUs eigener Firmware) — für den eigenen Kernel kein Hindernis, aber
ein Hinweis, dass die x86-Vorkette strukturell anders organisiert sein
könnte als die 68K-Vorkette (mehrere kleine Bootstrap-Module statt eines
monolithischen ROM-Codes).

## 3. Der Dreiklang — jetzt eine Pflichtanforderung

Aus [Thema 02](kernel-walkthrough/02-io-manager-syscall-dispatch/) und
[Thema 03](kernel-walkthrough/03-dreiklang/): der auffälligste Fund der
ganzen Serie — **`Q9X_ioman_attach` (x86) linkt mit exakt denselben drei
Filterwerten `0xF00`/`0xE00`/`0xD00` wie das 68K-`I$Attach`**, um
nacheinander Descriptor→Driver→File-Manager zu linken. Über zwei komplett
unabhängige Implementierungen hinweg identisch — und genau deshalb jetzt
kein "netter Fund mehr", sondern **Pflichtmechanismus**: ohne exakten
Nachbau von `F$Link` mit typgefiltertem Namens-Lookup, in dieser
Reihenfolge, mit diesen Filterwerten, lassen sich weder reale
`cfide`/`rbf`-Module (68K) noch reale OS-9000-Äquivalente ansprechen —
und genau das hat Andreas als nicht verhandelbar bezeichnet.

Der Dreiklang selbst (Descriptor beschreibt "was für ein Gerät",
Driver spricht die Hardware an, File-Manager definiert die
Dateisystem-Semantik) bleibt auch für **eigene, neue** Modularten ein
sinnvolles Muster — erlaubt z. B., einen Treiber mit verschiedenen
File-Managern zu kombinieren (SCF für seriell, RBF für Blockgeräte).

**Callcode-Dispatch innerhalb eines File-Managers** (Thema 03): beide
Architekturen lösen das über eine **kompakte, callcode-indizierte
Sprungtabelle** — 68K mit 13 Slots (`I$Create`…`I$Close`, Basis `0x83`)
direkt an der `M$Exec`-Adresse, x86 mit 16 Slots (3 neue, unidentifizierte
dazu) im `m_idata`-Bereich, indiziert mit `Callcode-0x95`. Für echte
Kompatibilität muss der eigene Kernel **beide** Tabellen-Layouts (Position
im Modul, Basiswert der Indizierung) unterstützen — für eigene neue
File-Manager ist das Muster selbst (`(Callcode-Basiswert)` als Tabellen-
Index) eine klare Übernahme-Empfehlung, unabhängig vom exakten Layout.

## 3a. RBF-Handler-Körper: File-Manager → Treiber-Übergabe (aus Thema 08)

Aus [Thema 08](kernel-walkthrough/08-rbf-handler/): eine Ebene tiefer als
Sektion 3 (Dispatch-Tabelle) — wie kommt ein File-Manager wie RBF beim
eigentlichen `I$Write` zum zuständigen Treiber? Die x86-Seite liefert
hier ein klares, gut belegtes Muster: **`Q9X_rbf_driver_dispatch`**
durchläuft eine **Laufzeit-Liste aus (Geräte-/Treiber-ID,
Handler-Funktionszeiger)-Paaren** und ruft den passenden Handler per
**indirektem `CALL`** auf — kein fest verdrahteter Sprung, sondern ein
generischer, zur Laufzeit aufgebauter Dispatcher. `Q9X_rbf_i_write`
selbst enthält dafür **keine eigene Zugriffslogik**, sondern delegiert
komplett an diese eine Funktion.

Der 68K-Vergleich fällt hier anders aus als sonst: `Q9_rbf_i_read` ruft
stattdessen **~10 interne Hilfsroutinen** direkt per `bsr` auf (nicht
über eine Liste), und `I$Write` teilt sich sogar Code mit `I$WritLn`
(mehrere Einsprungpunkte in dieselbe Routine — dasselbe Prinzip wie
beim 68K-Kernel selbst, siehe Thema 01). Die genaue Aufteilung der 68K-
Hilfsroutinen ließ sich in dieser Runde nicht mehr rekonstruieren.

**Übernahme-Empfehlung für den eigenen Kernel:** das x86-Muster (Laufzeit-
Liste aus ID/Funktionszeiger-Paaren mit indirektem Aufruf) ist flexibler
als starre `bsr`-Ketten und passt zum bereits in Thema 01 gefundenen
x86-Modul-Scanner-Prinzip (Geräte/Module werden zur Laufzeit entdeckt,
nicht fest verdrahtet einprogrammiert) — für **eigene, neue**
File-Manager-Implementierungen ein sinnvolles Vorbild. Für die
**Kompatibilität** mit echten alten Modulen ändert das nichts an Sektion
3: die Dispatch-Tabellen-Layouts selbst (68K `M$Exec`, x86 `m_idata`)
bleiben die Pflichtanforderung, unabhängig davon, wie der eigene Kernel
intern vom Tabellen-Eintrag zum Treiber kommt.

## 3b. Treiber-Hardware-Zugriff und der x86-Syscall-Auslöser (aus Thema 09)

Aus [Thema 09](kernel-walkthrough/09-treiber-hardware/): der letzte
Schritt der Kette Descriptor→Driver→Hardware, und nebenbei die Antwort
auf die in Abschnitt 2c offen gelassene Frage nach dem x86-Syscall-
Auslöser.

**68K (`cfide`):** ein direkter, lehrbuchmäßiger ATA/IDE-PIO-Treiber —
Statusregister pollen, Drive/Head- und LBA-Register setzen, echte
ATA-Kommandobytes (`0x20`=READ, `0x30`=WRITE), 512 Byte byteweise per PIO
transferieren. **Kein** Systemaufruf im heißen Lese-/Schreibpfad — nur
zwei `trap #0`-Aufrufe an ganz anderer Stelle (Init, GetStat-Formatierung).
Das bestätigt: ein Treiber, der echte Hardware bedient, braucht dafür
keinen einzigen Kernelaufruf — reiner Register-Zugriff reicht.

**x86 (`scllio`):** genau umgekehrt — **kein** `IN`/`OUT` im ganzen Modul,
dafür ein zehn Byte kleiner Trampolin, der `INT 0xFF` mit einem Zeiger auf
einen Callcode-Parameterblock in `ECX` ausführt. Das ist der bislang
gesuchte x86-Systemaufruf-Mechanismus (das x86-Gegenstück zu `TRAP #0`) —
gefunden auf der **aufrufenden** Seite, nicht im Kernel-Modul selbst (der
`IDT`-Vektor-0xFF-Handler bleibt weiterhin unlokalisiert). Ob `scllio`
darüber hinaus selbst Hardware anspricht (hinter einem der drei
gefundenen indirekten `CALL`s), wurde nicht geklärt.

**Für den eigenen Kernel:**
- **Muss-Anforderung** (für x86-Kompatibilität, falls Bedeutung 2 aus dem
  Statusabschnitt oben gilt): ein `IDT`-Gate für Vektor `0xFF`, das echte
  x86-Module unverändert weiter nutzen können — analog zum 68K-`TRAP #0`-
  Handler aus Thema 06.
- **Übernahme-Empfehlung, architekturunabhängig**: der Parameterblock-
  Kontrakt (Callcode-Wort + Flag-Wort + Größenfeld + Nutzdaten, komplett
  auf dem Stack aufgebaut, Zeiger im Trap-Register übergeben) ist ein
  einfaches, klar spezifizierbares Muster für eigene neue Systemaufrufe —
  unabhängig davon, ob als 68K-`TRAP` oder x86-`INT` ausgelöst.
- **Bestätigt erneut** das Dispatch-Muster aus Sektion 3a: `scllio`s drei
  indirekten `CALL`s über eine verkettete Liste sind derselbe Mechanismus
  wie `Q9X_rbf_driver_dispatch` aus Thema 08 — kein Einzelfall, sondern
  ein durchgängiges x86-Konstruktionsprinzip.

## 3c. Programm-Modul-Laden: `F$Load` (aus Thema 11)

Aus [Thema 11](kernel-walkthrough/11-programm-laden/): `docs/KERNEL.md`
beschreibt `F$Fork`s ersten Schritt seit Beginn dieses Projekts in
Theorie ("Modul lokalisieren/laden — erst im Speicher suchen, sonst von
Mass-Storage nachladen") — dieser Abschnitt bestätigt das jetzt mit
echtem, disassembliertem 68K-Code (`F$Load`, physisch im IOMan-Modul,
nicht im Kernel).

**Klare Übernahme-Empfehlung:** die dreistufige Suchreihenfolge — (1)
In-Memory-Modulverzeichnis per Namensvergleich durchsuchen, (2) bei
Nichttreffer eine der beiden Suchlisten anhand eines Präfix-Tests
(`'/'` = vollständiger Pfad, sonst Standard-Suchliste) auswählen, (3)
erst dann tatsächlich von Mass-Storage laden — spart bei mehrfach
genutzten Modulen (Compiler-Läufe, wiederholte Programmstarts)
vollständig wiederholte Plattenzugriffe. Für den eigenen Kernel ein
einfaches, klar spezifizierbares Muster, unabhängig vom exakten
Byte-Layout des Modulverzeichnisses.

**Ebenfalls Übernahme-Empfehlung:** neu geladene Module werden in eine
**typspezifische verkettete Liste** eingehängt (nicht eine einzige globale
Liste) — passt zur bereits in Abschnitt 1 erwähnten Typ-Code-Zentralität
des ganzen Formats und macht typgefilterte Suchen (wie sie `F$Link`s
Dreiklang-Mechanismus, Abschnitt 3, ohnehin braucht) effizient.

**x86-Seite offen:** kein eigenständiger x86-`F$Load`-Fund in dieser
Runde — nur der bereits aus Abschnitt 2 bekannte Boot-Zeit-Modul-Scanner
als naheliegende, aber nicht bestätigte Parallele.

## 4. Was NICHT übernommen werden sollte

**Wichtige Klarstellung vorab:** Diese Punkte betreffen nur Code, den
**wir selbst neu schreiben** (eigene Kernel-Interna, eigene neue Module) —
sie haben **nichts** mit der Fähigkeit zu tun, echte alte Module
auszuführen. Ein reales Modul, das intern einen der unten genannten
Tricks nutzt, läuft trotzdem unverändert weiter, wenn wir seinen
Maschinencode ausführen (68K direkt, x86 nur mit einem eigenen x86-
Interpreter, s. Statusabschnitt oben) — wir müssen diese Tricks nicht
selbst nachbauen, nur nicht kaputt machen, was schon drinsteht.

- **Der x86-"RET-Trampolin"-Sprungtrick** (`CALL $+5`/`POP`/`LEA`/zwei
  `PUSH`/`RET` statt normalem `CALL`) — taucht in den x86-Referenzkernen
  gleich **dreimal** auf (Kernel-Bootstrap, IOMan-Dispatch), ist aber
  vermutlich eine Notlösung des jeweiligen C-Compilers/der Toolchain
  (Ghidra scheitert jedes Mal daran, es als normalen Aufruf zu erkennen —
  ein Hinweis, dass es kein bewusst gewähltes, sauberes Sprachmittel war).
  Für **eigenen, neuen** Kernel-/Modulcode gibt es keinen Grund, diesen
  Trick nachzubauen — ein normaler indirekter `JMP`/`CALL` über einen
  Funktionszeiger reicht.
- **x86s 88-Byte-Universalheader 1:1** — wie in Abschnitt 1 erwähnt, das
  Konzept (ein Feld pro Sache, keine typabhängige Sonderbehandlung) ist
  gut, die exakte Feldreihenfolge/-breite ist reine OS-9000-Historie.
- **68Ks feste `0x1000`-Byte-Größe des Kernel-Global-Bereichs** (falls sie
  sich als zu knapp/zu großzügig erweist) — eine Konstante, keine
  architektonische Notwendigkeit.

## 5. Offene Entscheidungen, die nur Andreas treffen kann

Diese Liste löst NICHTS davon auf, macht die Entscheidungen aber
konkreter:

1. **Strukturelle Kompatibilität oder tatsächliche Binärausführung für
   OS-9000/x86-Module?** (s. Statusabschnitt oben) — Bedeutung 2 braucht
   einen x86-Interpreter zusätzlich zu Musashi, ein substanzielles neues
   Stück Technik, kein Nebeneffekt der übrigen Punkte hier.
2. **Eigenes Syscall-Nummerierungsschema oder 68K-`F$`/`I$`-Codes
   übernehmen?** Für Kompatibilität zu echten 68K-Treibern/File-Managern
   ohnehin zwingend die 68K-Codes — die Frage ist eher, ob eigene, neue
   Syscalls eine getrennte Nummerierung bekommen oder in dieselbe Tabelle
   einsortiert werden. **Teilweise entschieden (2026-08-15, s. Abschnitt
   6):** eigene Syscalls bekommen einen von den echten `F$`/`I$`-Codes
   komplett getrennten Nummernraum — ein erster Entwurf (`F_MUTEX_INIT`
   `0x30` u.a.) kollidierte real mit `F$AllPD`/`F$RetPD`/`F$SSvc`/
   `F$DelTsk`. Offen bleibt nur noch die exakte Lage/Größe dieses neuen
   Raums.
3. **Wie groß soll der Kernel-Global-Bereich sein, und wo liegt er?** (68K
   nutzt `0x1000` Byte ab einer über VBR erreichten Adresse) — abhängig
   von Q9-Flux' RAM-Layout.
4. **Modul-Scanner beim Boot: ja oder nein?** Empfehlung oben war "ja,
   x86-Ansatz übernehmen" — aber das ist eine echte Design-Entscheidung
   mit Aufwandsfolgen (Prüfsummen-Logik, Namenskollisions-Handling bei
   mehreren Revisionen), keine reine Formsache.
5. **Wie viele/welche eigenen Modularten sollen zusätzlich zum Dreiklang
   definiert werden?** Andreas ist dafür offen, aber ohne Eingrenzung
   bleibt das komplett unbestimmt — auch nur ein paar Stichworte würden
   reichen, um die Modulformat-Erweiterung (Abschnitt 1) konkreter zu
   planen. **Teilweise entschieden (2026-08-15):** ein eigenes Q9-Modul-/
   Header-Format ist für Phase 1 (s. Abschnitt 6) grundsätzlich in Scope —
   u.a. weil erweiterte Zugriffsrechte (mehr als Owner/Public) auf den
   echten Legacy-Headern strukturell nicht nachrüstbar sind (klassisches
   68K-`FD_ATT`/`PD_ATT` ist nur 8 Bit breit, kein Platz für eine
   Gruppen-Ebene — die kam erst mit OS-9000). Konkrete Feldliste des
   eigenen Headers noch offen.

## 6. Phasenplan und SMP-Grundsatzentscheidungen (2026-08-15, 💡 Vorschlag)

Planungsgespräch, keine neuen Ghidra-Funde — Ergebnis eines Gesprächs über
"was soll der eigene Kernel zusätzlich zur OS-9/9000-Vorgabe können, und
was müssen wir *jetzt* entscheiden, weil es sich später nicht mehr sauber
nachrüsten lässt". Noch nicht mit Andreas' finalem Go versehen, daher 💡.

### Phasenplan (Andreas' eigene Formulierung)

- **Phase 1**: reale OS-9/68K-Module laden und ausführen können (zuerst
  Original-Microware-Manager/IO/Driver/Descriptor-Module) — plus SMP-
  Fähigkeit (mindestens architektonisch vorgesehen), 64-Bit-taugliche
  Schnittstellen inkl. Big-/Little-Endian-Bewusstsein (mindestens
  vorgesehen), eigene neue Syscalls (Umsetzung kann später kommen:
  Multiprozessor-Verwaltung, Mutex, interne Pipes) und ein eigenes
  Q9-Modul-/Header-Format als Planungsgegenstand.
- **Phase 2**: reale OS-9000/x86-Module laden und ausführen können, plus
  alle Zusatzfähigkeiten aus Phase 1.
- **Phase 3 (Nice-to-have, wächst erwartungsgemäß weiter)**: CPU-Hotplug &
  Deep Sleep, UNIX-artiges `fork()` über A6-Register-Umbiegen (nur für
  Q9-eigenen, flach-adressierten Code sinnvoll), volles Demand Paging mit
  Swap.

### Sortiermethode: One-Way-Door vs. Two-Way-Door

Kriterium: lässt sich eine Erweiterung später noch sauber nachrüsten, oder
zieht sie sich unumkehrbar durch den ganzen Code? Ergebnis dieser Runde:

**Jetzt entscheiden (🔴 Phase 1):**
- SMP-fähige Kernel-Globals (pro-CPU statt fixem A6/FS-Zeiger), TAS-
  Spinlock-Unterbau mit `#ifdef`-No-Op auf Single-Core, CPU-Zahl dynamisch
  aus einem Init-Modul gelesen.
- Adressierungsmodell (s. Abschnitt 2d, Ergänzung 2026-08-15) — Portable
  Seitentabellen-Abstraktion, weil die Legacy-Seite bereits real
  PMMU-basiert virtuell adressiert, nicht flach/physisch.
- 64-Bit-taugliche Syscall-Struct-/Zeigergrößen-Konventionen, auch wenn
  68K/RISC-V32 vorerst 32-Bit bleiben.
- Eigener, von `F$`/`I$` komplett getrennter Syscall-Nummernraum (s.
  Fund unten).
- Grundexistenz von Kernel-Mutex-Primitiven (nicht die exakte
  Nummer/der Name — die ist 🟡).

**Später leicht nachrüstbar (🟡), heute nur Erweiterungspunkt vormerken:**
Priority Inheritance/Ceiling für Locks, Modul-Hot-Reload, Futexes,
Lockless-Ringpuffer/Named-Pipe-Systemmonitoring, `/proc`, Dateisystem-
Journaling. Netzwerk-Stack bleibt grundsätzlich File-Manager-Ebene, nie
Kernel — bestätigt über `os9k_tech.pdf`: `DT_NFM`/`DT_SOCK`/`DT_RTNFM`
sind File-Manager-Gerätetypen, kein Kernel-Bestandteil.

### Konkreter, verifizierter Fund: Syscall-Nummernkollision

Ein erster Entwurf eigener Syscalls (`F_MUTEX_INIT` `0x30`, `F_MUTEX_LOCK`
`0x31`, `F_MUTEX_UNLOCK` `0x32`, `F_CPU_COUNT` `0x40`) wurde gegen
[`SYSCALL_MODULE_MAP.md`](SYSCALL_MODULE_MAP.md) geprüft — alle vier
kollidieren mit real existierenden OS-9-Syscalls (`F$AllPD`, `F$RetPD`,
`F$SSvc`, `F$DelTsk`). Regel daraus: jeder neue Syscall-Vorschlag muss
zuerst gegen diese Tabelle geprüft werden, bevor eine Nummer vergeben
wird.

### Namenskonvention: `SMP_`, nicht `MP_`

Multiprozessor-bezogene Syscalls/Bezeichner bekommen das Präfix `SMP_`
(Symmetric Multi-Processing, Industriestandard). **Nicht** `MP_` — das ist
im echten OS-9000-Spec bereits *Module Permission* (`MP_OWNER_READ` etc.,
Modul-Zugriffsrechte im Header) — Wiederverwendung würde dieselbe Art von
Kollision reproduzieren wie oben. Geprüft: `SMP` kommt in `68k_tech.pdf`
und `os9k_tech.pdf` nirgends vor, frei verwendbar.

## Quellen

Alles hier basiert ausschließlich auf bereits verifizierten Funden aus
dem Kernel-Walkthrough — keine neuen Behauptungen, nur Synthese:

- [`kernel-walkthrough/00-modul-aufbau-und-header/`](kernel-walkthrough/00-modul-aufbau-und-header/README.md)
- [`kernel-walkthrough/01-kernel-bootstrap/`](kernel-walkthrough/01-kernel-bootstrap/README.md)
- [`kernel-walkthrough/02-io-manager-syscall-dispatch/`](kernel-walkthrough/02-io-manager-syscall-dispatch/README.md)
- [`kernel-walkthrough/03-dreiklang/`](kernel-walkthrough/03-dreiklang/README.md)
- [`kernel-walkthrough/04-scheduler-prozesslebenszyklus/`](kernel-walkthrough/04-scheduler-prozesslebenszyklus/README.md)
- [`kernel-walkthrough/05-speicherverwaltung/`](kernel-walkthrough/05-speicherverwaltung/README.md)
- [`kernel-walkthrough/06-exception-handler/`](kernel-walkthrough/06-exception-handler/README.md)
- [`kernel-walkthrough/07-ssm-mmu/`](kernel-walkthrough/07-ssm-mmu/README.md)
- [`kernel-walkthrough/08-rbf-handler/`](kernel-walkthrough/08-rbf-handler/README.md)
- [`kernel-walkthrough/09-treiber-hardware/`](kernel-walkthrough/09-treiber-hardware/README.md)
- [`kernel-walkthrough/10-boot-vorkette/`](kernel-walkthrough/10-boot-vorkette/README.md)
- [`kernel-walkthrough/11-programm-laden/`](kernel-walkthrough/11-programm-laden/README.md)

Konkrete Modul-/Syscall-Stückliste, die auf diesem Dokument aufbaut:
[`OWN_KERNEL_MODULES_OVERVIEW.md`](OWN_KERNEL_MODULES_OVERVIEW.md).

Abschnitt 2d (Ergänzung) und Abschnitt 6 basieren zusätzlich auf einem
Planungsgespräch (kein Kernel-Walkthrough-Thema) sowie auf
`Q9-Flux/docs/MMU_SSM_WORKFLOW_de.md`, `68k_tech.pdf` (Kapitel zu
`I$Attach`, `FD_ATT`/`PD_ATT`) und `os9k_tech.pdf` (`MP_*`/`PERM_*`/
`DT_*`-Konstanten).

**Erstellt**: 2026-08-14, zuletzt ergänzt 2026-08-15
