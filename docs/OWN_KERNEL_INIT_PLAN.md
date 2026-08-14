# Erster Entwurf: was der eigene Q9-Kernel initialisieren muss

**Statushinweis, bewusst nicht versteckt:** Die Haupt-`README.md` dieses
Projekts sagt aktuell explizit "Kein Neubau eines eigenen Betriebssystems"
und verweist auf `Q9RESUME-Kernel` als archivierten, separaten früheren
Versuch dafür — Q9-OS versteht sich offiziell als **Port** von echtem
Microware OS-9/68K, nicht als Neuentwicklung. Dieses Dokument entstand,
weil Andreas in dieser Session wiederholt nach den Voraussetzungen für
einen **eigenen** Kernel gefragt hat ("dann werden wir mit dem Kernel
anfangen... was muss initialisiert werden"). Das kann heißen: der
README-Satz ist inzwischen veraltet, oder die Frage zielte auf ein
anderes/zukünftiges Projekt (evtl. eine Wiederbelebung von
`Q9RESUME-Kernel`), oder es ist bewusst nur Recherche ohne
Bau-Verpflichtung. **Nicht selbst entschieden** — deshalb bewusst NICHT
in die Haupt-`README.md` verlinkt, bis das geklärt ist. Der Inhalt selbst
bleibt trotzdem gültig (reine Synthese bereits verifizierter Funde), nur
der Rahmen "was ist das für ein Projekt" ist offen.

**Was das hier ist:** eine grobe Anforderungsliste, kein Bauplan und kein
Quellcode. Grundlage sind die vier Themen des
[Kernel-Walkthroughs](kernel-walkthrough/) — dem systematischen Vergleich
des OS-9/68K-Kernels (`dker030s`, dieses Projekts Referenz) mit dem
OS-9000/x86-Kernel (Microwares spätere, portable C-Neufassung). Ziel
dieses Dokuments: aus dem, was **beide** Referenzkerne tun, herausfiltern,
was für einen eigenen Kernel sinnvoll übernehmbar ist — nicht als 1:1-
Kopie einer Architektur, sondern als geprüfte Anforderung.

**Wichtiger Rahmen:** Q9-Flux emuliert 68K-Hardware (Musashi-Interpreter).
Ein eigener Kernel läuft also auf 68K. Die x86-Vergleichsseite ist deshalb
**nicht** die Zielplattform — sie dient dazu, zu erkennen, welche Teile
des 68K-Designs "OS-9-Konvention" sind (über zwei Architekturen und ~30
Jahre stabil, also vermutlich bewusst so gewählt) und welche Teile bloße
68K-CPU-Eigenheiten sind (austauschbar, ohne dass man die OS-9-Idee
verlässt).

## Die Grundsatzentscheidung, die dieses Dokument NICHT trifft

Aus einer früheren Diskussion (noch nicht formal entschieden): entweder

- **(A) Eigenständiger Kernel, eigenes Modulformat** — volle Freiheit,
  aber jeder Treiber/File-Manager muss selbst geschrieben werden. Andreas'
  eigene Einschätzung dazu: "bis wir einen eigenen RBF beschrieben haben,
  können sicher Jahre vergehen."
- **(B) ABI-kompatibel zu echten OS-9/68K-Binaries** an der Descriptor/
  Driver/File-Manager-Grenze — dann lassen sich reale Microware-Treiber
  (`cfide`, `rbf`, ...) direkt weiterverwenden (Q9-Flux hat mit Musashi
  bereits einen 68K-Interpreter eingebaut), zumindest übergangsweise, für
  Module ohne eigenen Quellcode. Lizenzrechtlich bleibt das auf internen
  Gebrauch beschränkt (siehe `vendor/README.md`).

Diese Liste ist **für beide Wege nützlich** — Pfad (B) macht mehrere
Punkte unten zur Pflicht (exakte Byte-Kompatibilität bei Header/Callcode-
Tabellen), Pfad (A) macht sie zur freien Wahl (Konzept übernehmen, Format
selbst bestimmen). Wo das einen Unterschied macht, steht es dabei.

## 1. Modulformat — was ein Modul-Header mindestens braucht

Aus [Thema 00](kernel-walkthrough/00-modul-aufbau-und-header/): drei
untersuchte Header-Generationen (6809/68K/OS-9000), aber ein Kern, der
über alle drei ~45 Jahre hinweg gleich blieb:

| Feld | Zweck | Für Pfad (B) zwingend byte-genau | Für Pfad (A) frei wählbar, aber Konzept behalten |
|---|---|---|---|
| Sync-Konstante | Modul im Speicher/auf Platte erkennen | Ja, `$4AFC` (68K-Wert) | Ja — eigener Wert erlaubt, aber ein fester Wert bleibt sinnvoll für den Boot-ROM-Scan |
| Modulgröße | Wie viele Bytes gehören zum Modul | Ja | Ja |
| Name-Offset + Namensstring | Modul über Namen linken (`F$Link`) | Ja | Ja, falls überhaupt namensbasiertes Linken gewünscht ist |
| **Typ-Code** | Welche Rolle hat das Modul (System/Fmgr/Driver/Descriptor) | **Ja, exakt `0x0C`/`0x0D`/`0x0E`/`0x0F`** — das ist die am längsten unveränderte Konstante der ganzen OS-9-Familie (6809→68K→x86, ~28 Jahre identisch) | Frei, aber: das Konzept "Typ-Byte, mit dem IOMan beim Linken filtert" ist der Kern des Dreiklangs (s. Abschnitt 3) — ohne das geht Pfad-B-Kompatibilität ohnehin nicht, für Pfad A trotzdem sinnvoll |
| Einsprungoffset (`M$Exec`) | Wo beginnt der Modulcode | Ja (Position im Header darf variieren, Konzept nicht) | Ja |
| Prüfsumme | Beschädigte Module erkennen | Ja, falls echte Module gelinkt werden sollen | Empfehlenswert, aber optional |

**Nicht übernehmenswert:** Weder 68Ks 46-Byte-Minimalheader noch OS-9000s
88-Byte-Universalheader sind zwingend die richtige Wahl für einen neuen
Entwurf — das war eine Design-Entscheidung ihrer jeweiligen Epoche
(68K: Speicher war knapp, nur das Nötigste im Standard-Header; OS-9000:
ein C-Compiler kann eine größere feste Struktur billig verwalten). Für
Pfad (A) lohnt sich eher OS-9000s Ansatz (ein Feld pro Konzept, keine
"nur bei bestimmten Typen vorhandene" Erweiterung) — einfacher zu parsen,
kein Sonderfall-Code nötig.

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

## 3. Der Dreiklang — Pflicht, falls Pfad (B) gewählt wird

Aus [Thema 02](kernel-walkthrough/02-io-manager-syscall-dispatch/) und
[Thema 03](kernel-walkthrough/03-dreiklang/): der auffälligste Fund der
ganzen Serie — **`Q9X_ioman_attach` (x86) linkt mit exakt denselben drei
Filterwerten `0xF00`/`0xE00`/`0xD00` wie das 68K-`I$Attach`**, um
nacheinander Descriptor→Driver→File-Manager zu linken. Über zwei komplett
unabhängige Implementierungen hinweg identisch.

**Falls Pfad (B) (echte Binaries weiterverwenden):** dieser Mechanismus
muss **exakt** nachgebaut werden — `F$Link` mit typgefiltertem Namens-
Lookup, in dieser Reihenfolge, mit diesen Filterwerten. Ohne das lassen
sich reale `cfide`/`rbf`-Module nicht ansprechen.

**Falls Pfad (A):** das Konzept (ein Gerät wird durch DREI verlinkte
Module beschrieben, nicht durch eine monolithische Treiberdatei) ist
trotzdem wertvoll — es trennt "was für ein Gerät ist das" (Descriptor) von
"wie spreche ich die Hardware an" (Driver) von "welche Dateisystem-Semantik
gilt" (File-Manager). Diese Trennung erlaubt z. B., denselben Treiber mit
verschiedenen File-Managern zu kombinieren (SCF für seriell, RBF für
Blockgeräte) — ein Freiheitsgrad, den man beim Neuentwurf nicht
leichtfertig aufgeben sollte, auch mit eigenem Format.

**Callcode-Dispatch innerhalb eines File-Managers** (Thema 03): beide
Architekturen lösen das über eine **kompakte, callcode-indizierte
Sprungtabelle** — 68K mit 13 Slots (`I$Create`…`I$Close`, Basis `0x83`)
direkt an der `M$Exec`-Adresse, x86 mit 16 Slots (3 neue, unidentifizierte
dazu) im `m_idata`-Bereich, indiziert mit `Callcode-0x95`. **Übernahme-
Empfehlung unabhängig vom Pfad**: eine File-Manager-interne
Sprungtabelle, indiziert über `(Callcode - Basiswert)`, ist ein simples,
bei beiden Architekturen bewährtes Muster — deutlich einfacher als eine
Kette von `if`/`switch`-Vergleichen, und offen für Erweiterung (x86 zeigt,
dass man die Tabelle bei Bedarf problemlos vergrößern kann, 13→16).

## 4. Was NICHT übernommen werden sollte

- **Der x86-"RET-Trampolin"-Sprungtrick** (`CALL $+5`/`POP`/`LEA`/zwei
  `PUSH`/`RET` statt normalem `CALL`) — taucht in den x86-Referenzkernen
  gleich **dreimal** auf (Kernel-Bootstrap, IOMan-Dispatch), ist aber
  vermutlich eine Notlösung des jeweiligen C-Compilers/der Toolchain
  (Ghidra scheitert jedes Mal daran, es als normalen Aufruf zu erkennen —
  ein Hinweis, dass es kein bewusst gewähltes, sauberes Sprachmittel war).
  Für einen von Hand geschriebenen oder aus C kompilierten eigenen Kernel
  gibt es keinen Grund, diesen Trick nachzubauen — ein normaler indirekter
  `JMP`/`CALL` über einen Funktionszeiger reicht.
- **x86s 88-Byte-Universalheader 1:1** — wie in Abschnitt 1 erwähnt, das
  Konzept (ein Feld pro Sache, keine typabhängige Sonderbehandlung) ist
  gut, die exakte Feldreihenfolge/-breite ist reine OS-9000-Historie.
- **68Ks feste `0x1000`-Byte-Größe des Kernel-Global-Bereichs** (falls sie
  sich als zu knapp/zu großzügig erweist) — eine Konstante, keine
  architektonische Notwendigkeit.

## 5. Offene Entscheidungen, die nur Andreas treffen kann

Diese Liste löst NICHTS davon auf, macht die Entscheidungen aber
konkreter:

1. **Pfad (A) oder (B)?** (s. o.) — das bestimmt, wie viel von Abschnitt 1
   und 3 zur Pflicht statt zur Empfehlung wird.
2. **Eigenes Syscall-Nummerierungsschema oder 68K-`F$`/`I$`-Codes
   übernehmen?** Bei Pfad (B) zwingend die 68K-Codes (sonst keine
   Kompatibilität zu echten Treibern/File-Managern); bei Pfad (A) frei.
3. **Wie groß soll der Kernel-Global-Bereich sein, und wo liegt er?** (68K
   nutzt `0x1000` Byte ab einer über VBR erreichten Adresse) — abhängig
   von Q9-Flux' RAM-Layout.
4. **Modul-Scanner beim Boot: ja oder nein?** Empfehlung oben war "ja,
   x86-Ansatz übernehmen" — aber das ist eine echte Design-Entscheidung
   mit Aufwandsfolgen (Prüfsummen-Logik, Namenskollisions-Handling bei
   mehreren Revisionen), keine reine Formsache.
5. **Reicht ein Descriptor+Driver+File-Manager-Dreiklang, oder wird eine
   vereinfachte Zwei-Ebenen-Struktur gewünscht** (z. B. Descriptor+Driver
   verschmolzen, wenn ohnehin nur eigene, für Q9 geschriebene Treiber
   zum Einsatz kommen)?

## Quellen

Alles hier basiert ausschließlich auf bereits verifizierten Funden aus
dem Kernel-Walkthrough — keine neuen Behauptungen, nur Synthese:

- [`kernel-walkthrough/00-modul-aufbau-und-header/`](kernel-walkthrough/00-modul-aufbau-und-header/README.md)
- [`kernel-walkthrough/01-kernel-bootstrap/`](kernel-walkthrough/01-kernel-bootstrap/README.md)
- [`kernel-walkthrough/02-io-manager-syscall-dispatch/`](kernel-walkthrough/02-io-manager-syscall-dispatch/README.md)
- [`kernel-walkthrough/03-dreiklang/`](kernel-walkthrough/03-dreiklang/README.md)

**Erstellt**: 2026-08-14
