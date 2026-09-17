# Kernel: Analyse und nächste Arbeitspakete

Stand: 14.09.2026. Analysierter Stand: `main`, `3681010`.
Basis: Quellcode, Buildskript, aktuelle Übergabe in
`OWN_KERNEL_STATUS.md` und erneuter Lauf aller 16 Hosttests (alle bestanden).
Am 14.09.2026 wurde zusätzlich ein frischer 68k-Build im Emulator gestartet;
die F$TLink-Spur wurde um Modul-, Init-, Exec-, Static- und Fehlerwerte
erweitert. Die Testauswahl steht im Quellbaum wieder auf dem kombinierten
Fall `echo` gefolgt von `date`.

## Einschätzung

## Messstand 17.09.2026

Der direkte Rücksprung aus dem externen `I$ChgDir("/dd")` ist jetzt live
belegt: der Aufruf erreicht den Rücksprung-PC `0x0000caa6` wieder. Im
gemessenen Lauf standen dabei `D0=0x00000005` und `D1=0x00008200`; der
Aufruf hängt somit nicht im externen IOMan-Pfad. Die zuvor verwendete
Testauswahl war jedoch nicht vollständig reproduzierbar: der
Sysgo-Startup-Versuch wurde unabhängig von `Q9K_TestPrograms` immer
gestartet und wartete auf die Shell. Dafür gibt es jetzt in
`q9kernel_entry.a` den Compiletime-Schalter `Q9K_TestStartup` (Standard
`0`), sodass `echo`/`date`-Regressionen den Startup-Versuch gezielt
überspringen können; für die separate Startup-Untersuchung kann er auf `1`
gesetzt werden. Im isolierten Modus wurde anschließend
`F$Load("/dd/CMDS/date")` erreicht; innerhalb des 18-Sekunden-Fensters war
noch kein Rücksprung aus diesem Ladevorgang sichtbar. Eine Exception trat
dabei nicht auf. Der nächste Messpunkt ist daher die interne Rückkehrbilanz
von IOMans `F$Load` für `date`, nicht erneut `I$ChgDir`.
Mit aktivierter CF-Sektorspur wiederholt sich dabei derselbe Verzeichnis-
bzw. Dateisektor (`LBA 65`), ohne dass ein weiterer Datenbereich gelesen
oder der Aufruf zurückkehrt. Dasselbe fehlende Rückkehrsignal wurde im
aktuellen Kombitest bereits beim `echo`-Laden beobachtet; `csl` ist in dieser
Bootkette resident und daher kein gleichwertiger Dateilade-Gegentest. Das ist
der derzeit stärkste Hinweis auf einen RBF-/Verzeichnis-Weiterlauf im
externen `F$Load`-Pfad.

Ein kontrollierter Gegenversuch am 17.09. hat die Zuständigkeit weiter
getrennt: Wenn der per `F$SSvc` registrierte IOMan testweise auch den
`I$Open`-Slot übernehmen darf, verschwindet der alte Root-Sektor-Loop, aber
IOMan scheitert bereits beim Konsolen-Open mit `Error $0000`. Die einfache
Freigabe der I/O-Slots ist deshalb kein Fix. Der Kernel-Fallback und der
externe IOMan-Aufruf müssen über denselben Registerrahmen, A4/A6-Kontext,
Pfaddeskriptor und Fehler-CCR integriert werden. Die Änderung wurde nicht
übernommen; sie dient als reproduzierbare Abgrenzung für den nächsten
Schritt.

Der 68k-Kernel ist ein laufender Integrationsprototyp mit Modulverwaltung,
Prozessverwaltung, Scheduler, Speicherverwaltung und Anbindung an fremde
OS-9-Komponenten. Laut letzter Übergabe läuft `echo` mit `csl`; `date`
endet mit `csl traphandler mismatch`. Das ist der aktuelle Engpass.
Die vermutete Arena-Überlappung wurde im letzten Commit zurückgezogen:
Sie darf nicht als bestätigte Ursache weiterverwendet werden.
Der x86-Bereich enthält vor allem Analyse- und Bootreferenzen; Schwerpunkt
der nächsten Sitzung sollte der eigene 68k-Kernel bleiben.

Die bisherigen grünen Tests sind nützlich, beweisen aber nicht den realen
Trap-/Interrupt-Ablauf. Besonders `test_q9kernel_traplink.c` testet die
C-Buchhaltung mit Stubs; es führt `M$Init` und den Assemblerübergang nicht aus.
Außerdem verwendet dieser Hosttest bewusst 64-Bit-`unsigned long` für
32-Bit-Zielfelder und kompensiert die Abweichung mit anderen Abständen.

## Kleine Roadmap, in dieser Reihenfolge

| Priorität | Paket | Fertig, wenn … |
|---|---|---|
| 1 | Reproduzierbaren Boot-Test und Symbolzuordnung herstellen | Frischer Build, Kernel-/Emulator-Commit, Modul- und Image-Hashes, Link-Map und vollständiges Log gehören zu demselben Lauf. `echo` allein, `date` allein, beide nacheinander und beide gleichzeitig sind getrennte Fälle. |
| 2 | `date` / `F$TLink` / `M$Init` untersuchen und korrigieren | Der erste abweichende Register- oder Speicherwert gegenüber dem funktionierenden Fall ist belegt; `date` liefert nachvollziehbare Ausgabe und Exitstatus ohne mismatch. |
| 3 | Trap-/Interrupt-Grundlage stabilisieren | Der dokumentierte Schutz durch 12 Byte Totraum ist durch eine ursächliche Korrektur ersetzt; wiederholte Zwei-Prozess-Läufe funktionieren unabhängig von Codeverschiebungen. |
| 4 | Tests und ABI-Abbildung stärken | 32-Bit-Werte, Hostzeiger und Big-Endian-Zielspeicher sind explizit getrennt; Trap-Aufruf/Rückkehr, Fehlerpfade und getrennte Trap-Daten zweier Prozesse sind abgedeckt. |
| 5 | Kleine echte Programmsuite und Ressourcenprüfung | Mehrere reale Programme laufen wiederholt mit geprüfter Ausgabe/Exitstatus; Modulreferenzen, Trap-Slots und Speicherverbrauch bleiben nachvollziehbar. Erst danach weitere Syscalls oder Shell-Integration ausbauen. |

## Konkreter Beginn der nächsten Sitzung

1. In einem neuen Buildverzeichnis bauen. `build.sh` ignoriert momentan
   `mwos-build`-Fehler und prüft danach nur, ob Objektdateien existieren.
   Alte Objekte dürfen deshalb keinen vermeintlich erfolgreichen Neubau
   vortäuschen. Symboltabellen aus `r68 -s` mit der tatsächlichen Link-Map
   verbinden: Objekt-Offsets sind noch keine Laufzeitadressen.
2. Isolierte Imagekopie verwenden und das wirklich gestartete Modul prüfen.

### Nachtrag 15.09.2026: Startup-Fehler auf den Rückgabecode eingegrenzt

Der bisherige `mshell`-Fehlertext lautet:

    **** can't call csl ****

Der Trap-Trace zeigt unmittelbar davor `F$TLink(13,"csl")` mit `E$MNF`
(`$DD`, Modul nicht gefunden). Es handelt sich an dieser Stelle nicht um
einen Trap-Handler-Mismatch. Ein frisches Image enthält zwar ein großes
residentes `csl`-Bootmodul, dessen Modulname ist im aktuellen
Boot-/Modulverzeichnispfad jedoch nicht zuverlässig für `F$TLink`
auffindbar. Das frühere isolierte `echo`-Ergebnis war deshalb nicht
widersprüchlich: dort wurde `csl` zuvor erfolgreich per `F$Load` aus dem
Dateisystem geladen.

Ein Vorab-`F$Load` im neuen Startup-Prozess wurde probeweise getestet,
blockiert aber in diesem frühen Pfad und bleibt deshalb nicht im Kernel.
Der nächste sinnvolle Fix ist die gemeinsame Ursache im Bootmodul- bzw.
Modulverzeichnispfad: ein großes residentes Trapmodul muss nach dem Boot
mit seinem echten Namen auffindbar sein, bevor `mshell` gestartet wird.

Die anschließende A6-Spur zeigt nach dem Arena-Fix die nächste Ursache:
`csl` wird gefunden und `F$TLink` erfolgreich abgeschlossen, danach wechselt
A6 im externen Trap-Rückweg mehrfach korrekt zwischen der `csl`-Statik
(`0x56240`) und der `mshell`-Datenbasis (`0x591c0`). Beim letzten Rückweg
entsteht jedoch `0x591b9` (ungerade und sieben Bytes zu klein), worauf der
Adressfehler in `mshell` folgt. Das testweise überschriebenе `R$a7`-Feld im
externen Registerrahmen wurde entfernt; der Fehler bleibt bestehen. Als
nächster Prüfkandidat bleibt damit die Rückgabe-/Rahmenbehandlung des
installierten `csl`-Trap-Handlers selbst.

Dieser Fix ist inzwischen umgesetzt: Die Arena-Basis wird beim Bootstrap
auf das Ende der höchsten belegten RAM-Bootregion angehoben und anschließend
16-Byte-ausgerichtet. Im frischen Emulatorlauf sind `csl` und `mshell` nun
vollständig im Modulverzeichnis sichtbar; `F$TLink(13,"csl")` endet mit
`ok=1` und `err=0`. Der nächste Fehler liegt daher erst danach im echten
`mshell`-Lauf: Vektor 11 (Adressfehler), mit einem PC in einem Textbereich.
   Einen vollständigen Prozessabschluss abwarten; kein vorzeitiger Erfolg
   beim ersten Prompt-ähnlichen Zeichen oder allein bei fehlender Exception.
3. Bei `date` den Sentinel an der vom Aufrufer übergebenen A3-Adresse
   verfolgen: vor `F$TLink`, unmittelbar vor `M$Init`, unmittelbar danach
   und an der mismatch-Prüfung. Dazu PID, PC, SR, A3/A4/A6 und Trap-Slot
   erfassen. Kleiner Ringpuffer statt unbeschränkter Konsolenausgabe.
4. Besonders den Vertrag in `Q9K_SysFTLink` prüfen: A4 wird nach der
   Registerwiederherstellung mit der Init-Adresse überschrieben; nach
   `M$Init` wird Carry bedingungslos gelöscht. Ferner erzeugt
   `Q9K_ProcTLink` auch bei Init-Offset 0 zunächst `hdr + 0`, während
   der Assembler nur auf eine absolute Nulladresse prüft. Das sind
   konkrete Prüfkandidaten, keine nachgewiesene Ursache des date-Fehlers.
5. Erst nach dem ersten belegten Unterschied einen kleinen Fix vornehmen
   und denselben Test wiederholen. Eine neue Hypothese muss einen messbaren
   Unterschied vorhersagen; widerlegte Hypothesen nicht als Befund übernehmen.

## Technische Schulden, die nicht verloren gehen dürfen

- `Q9K_PatchCslFreelistBug` verändert die geladene fremde Runtime im RAM.
  Den Patch als Kompatibilitätsmaßnahme kennzeichnen und gegen den
  Referenzkernel prüfen; ein erfolgreicher gepatchter Lauf allein beweist
  keinen Fehler in der originalen Runtime.
- Globale Scratch-Adressen und die IRQ-Sperre im Trap-Pfad erfordern eine
  klare Regel für Verschachtelung und Kontextwechsel. Bekannte
  Adresskollisionen sprechen für eine zentral geprüfte Speicherbelegung.
- `Q9K_ApplyInitializedData` braucht Grenzenprüfungen für Modul und
  Zielspeicher. Aktuell wird unter anderem ein IRefs-Gruppenwort verworfen.
  Den unterstützten Relokationsumfang explizit festlegen und testen.
- Fehlerpfade von `F$TLink` auf Rückgabe bereits erworbener Modulreferenzen,
  statischen Speichers und Trap-Slots prüfen.
- Primärer Prozessspeicher wird bei `F$Exit` bereits freigegeben;
  prozessbezogenes Tracking zusätzlicher Speicheranforderungen bleibt offen.
- Aktuellen Status kurz halten und historische Untersuchungen separat
  archivieren. Die große Statusdatei enthält überholte und korrigierte
  Aussagen; ihr Kopf und die letzten Korrekturen sind maßgeblich.

## Zwischenstand 14.09.2026

## Nachtrag 16.09.2026

- Der TrapEnt-Stackaufbau wurde erneut gegen die OS-9-Dokumentation geprüft:
  bei `A7` liegt das Caller-A6, danach folgen Funktionscode, Vektor und
  Rücksprungadresse. `Q9K_TCallDispatch` erzeugt diese Reihenfolge korrekt.
- Der verbleibende Adressfehler ist reproduzierbar: `csl` setzt A6 auf
  `0x56539` statt `0x56540`. Ein Schreibzugriff auf das betroffene
  Kontextfeld wurde bei `PC=0x17c38` innerhalb von `csl` gefunden; der
  Kernel schreibt an dieser Stelle nicht.
- Der vorhandene Laufzeitpatch `Q9K_PatchCslFreelistBug` greift auf dem
  aktuell gebooteten `csl`-Modul nicht, weil dessen erwartetes Bytemuster an
  `hdr+$56bc` fehlt. Vor einer Anpassung muss der entsprechende Pfad in
  dieser konkreten `csl`-Version identifiziert werden.
- Der breitere Registerschutz in `Q9K_SysFSRqMem` wurde gebaut und getestet,
  verändert den Fehler jedoch nicht. Es gibt deshalb noch keinen neuen
  Kernel-Fix oder Commit aus dieser Untersuchung.

## Nachtrag 15.09.2026

- Das Testabbild verwendet jetzt einen Extended-Boot: `tools/mkbootfile.sh`
  ruft `os9 gen -e -b=` auf. Damit wird die vollstaendige Segmentliste von
  `OS9Boot` verwendet; der bisherige feste Bootbereich von nur `$5264` Bytes
  hatte den vergroesserten Kernel-/IOMan-Bereich abgeschnitten.
- Ein frisches Image mit Kernel, IOMan, RBF, CF-Treibern, `math`, `csl`,
  `mshell` und `shell` wurde im Emulator gestartet. Der Dump bestaetigt die
  vollstaendige Bootregion (`$670da` Bytes) und alle genannten Module.
- Der Emulatorlauf endet weiterhin im Startup-/Shell-Test ohne erwarteten
  Erfolgsmarker; die Bootfile-Uebergabe ist damit repariert, der verbleibende
  Fehler liegt jetzt im Laufzeitpfad von Startup/Shell und nicht mehr in der
  abgeschnittenen Bootregion.

- `Q9-KERNEL/68k/src/kernel/build.sh <verzeichnis>` erzeugt jetzt einen
  reproduzierbaren Wegwerf-Build; `tools/mkbootfile.sh` kann ihn über
  `Q9K_BUILD_DIR` verwenden.
- `tools/mkbootfile.sh` ermittelt die Grenzen von Kernel, `init` und
  `forkchild` jetzt direkt aus den Modulköpfen. Die vorher fest verdrahteten
  Offsets stammten aus einem älteren Kernelstand und schnitten die aktuelle
  Referenz-Bootdatei mitten im Kernel auf.
- Ein frischer Emulatorlauf mit der korrigierten Bootkette findet wieder ein
  gültiges OS-9-Bootfile und startet `hellosvc` als echtes Programm. Der
  separate IOMan-Hinweis `can't chgdir to system device: $00DD` verschwindet,
  sobald `rbf`, `cfide`, `dd` und `c0` ergänzt werden.
- `date` benötigt zusätzlich die Runtime-Module `math` und `csl`; beide
  müssen als residente Bootmodule mit `Q9_BOOT_MODULES` in die Bootkette
  aufgenommen werden. Wird `csl` nur per `F$Load` geladen, aber nicht per
  `F$TLink(13,"csl")` initialisiert, endet `date` reproduzierbar mit einer
  Illegal-Instruction-Exception. Mit residentem `csl` läuft der isolierte
  `date`-Test ohne Exception bis zum normalen Test-Timeout.
- Dafür gibt es jetzt `tools/boot_modules_68k.conf`. Nach dem Laden der
  Microware-Toolchain kann `tools/mkbootfile.sh --config
  tools/boot_modules_68k.conf` verwendet werden. Die Datei verwendet
  `MWOS_ROOT` und ergänzt `math`/`csl`/`mshell` reproduzierbar. `mshell`
  wird für den Startup-Versuch resident aufgenommen, weil der aktuelle
  große Modul-Ladepfad `F$Load("/dd/CMDS/mshell")` noch mit `E$MNF` endet.
- `tools/run_kernel_test.exp` beendet den zugehörigen Emulator nach Marker
  oder Timeout und fordert davor einen Dump an. Für einen Dump muss der
  Harness aus dem Q9-Flux-Verzeichnis gestartet werden, weil Q9-Flux den
  relativen Pfad `local_images/q9dbg_dump.txt` verwendet.
- Der vollständige `echo`/`date`-Lauf ist noch nicht abschließend bewertet;
  er muss mit `math` und `csl` in derselben Bootkette erneut geprüft werden.
  Der künstliche manuelle
  Doppelaufruf von `F$TLink("csl")` ist im Standardtest deaktiviert, weil der
  geforkte Prozess den installierten Trap erbt.
- Der erste sysgo-artige Startup-Versuch ist inzwischen reproduzierbar:
  `F$Load("/dd/CMDS/shell")`, `I$Open("/dd/SYS/startup")` und
  `F$Fork("shell")` erreichen den Kindprozess ohne Exception. `F$Wait`
  blockiert zunächst; nach Korrektur der A4-Wiederherstellung liest die
  Shell tatsächlich, erhält aber wiederholt `Read I/O error $0201`. Der
  Startup-Text erscheint deshalb noch nicht. Die
  Übergabe von `d3=3` wurde an die F$Fork-Konvention angepasst, ändert das
  Verhalten aber nicht. Der aktuelle `I$Open`-Stub reserviert zwar einen
  Kernel-Pfadpool-Eintrag, erzeugt aber noch keinen vollständig an IOMan/
  den File-Manager gebundenen Dateipfad. Als nächstes muss dieser reale
  Pfaddeskriptor-/DBT-Vertrag hergestellt werden.

Für die nächste Sitzung reichen Paket 1 und der belegte Fix aus Paket 2.
Weitere Architekturports und neue Funktionsgruppen sind dafür nicht nötig.

## Nachtrag 16.09.2026 – Prozessblock-Zuordnung geklärt

- Die frühere Zuordnung des Schreibzugriffs zu `csl` bei `PC=0x17c38` war
  falsch. Der relevante Watch-Treffer liegt bei `PC=0x096fa` im Kernel-
  Kopierloop für `M$IData`.
- Der konkrete Kopiervorgang lautet: Quelle `0x30c14`, Ziel `0x4ff42`,
  Länge mindestens vier Bytes. Quelle und Ziel enthalten dabei Nullen.
- Der Prozesspool zeigt: Slot 1 (`D_Proc=0x32940`) gehört zu `mshell` und
  besitzt den Block `0x4e540` mit `0x5072` Bytes. `0x4f5b2` ist dessen
  Blockende und zugleich die Basis des gerade neu allokierten Kindblocks;
  der Schreibzugriff liegt somit im neuen Kindblock, nicht im laufenden
  Elternblock.
- Die Modulwerte von `mshell` sind konsistent: `M$Mem=0x1c3a`,
  `M$Stack=0x3400`, ergänzt um angeforderten Speicher und Parameter ergibt
  die beobachtete Allokationsgröße `0x5072`. Der `M$IData`-Kopiervorgang
  überschreibt daher nicht außerhalb der angeforderten Kind-Allokation.
- Ergebnis dieser Zwischenmessung: Dieser konkrete Watch-Treffer ist ein
  normaler Initialisierungsschreibzugriff und kein neuer Beleg für die
  A6-/csl-Korruption. Der damals noch offene Adressfehler wurde im folgenden
  Nachtrag durch die Korrektur des F$TLink-A6-Bias behoben.

## Nachtrag 16.09.2026 – F$TLink-A6-Bias korrigiert

- Die direkte Analyse von `csl`'s `M$Init` zeigt den Zugriff
  `movea.l -$7ffc(a6),a6`. Der F$TLink-Trampolin übergab bislang den rohen
  statischen Speicherzeiger als A6.
- Das ist für ein reentrant gelinktes OS-9-Modul falsch: `M$Init` erwartet
  den statischen Datenzeiger mit dem OS-9-Bias von `$8000`. Der Trampolin
  addiert nun vor dem Sprung zu `M$Init` explizit `$8000`.
- Mit einem neu gebauten Kernel und einem neu erzeugten Bootfile verschwindet
  der bisher reproduzierbare Adressfehler vollständig: kein `Vektor=11`, kein
  `A6=0x56539`. Der Lauf bleibt danach ohne Exception im noch offenen
  Startup-/Shell-Pfad hängen; das ist ein nachgelagerter, separater Testpunkt.

## Nachtrag 16.09.2026 – Startup-/Shell-Parameter korrigiert

- Der verbleibende `mshell`-Fehler `E$MNF` lag nicht an einem weiteren
  fehlenden Kommandomodul. Die Parameterstruktur unseres `F$Fork`-Aufrufs
  wich von der Referenz `sysgo_smart.a` ab: verwendet wurde `-npxt` statt
  `-npt` sowie ein eigener, inkompatibler Parameterblock.
- Der Startup-Prozess richtet jetzt vor dem Shell-Aufruf die Datenbasis
  `/dd` und das Ausführungsverzeichnis `/dd/CMDS` ein und verwendet den
  vollständigen, von `sysgo_smart` belegten Parameter-/Environment-/argv-
  Block.
- Verifiziert im Emulator mit neu gebautem Kernel und Bootfile:
  `W00000000` – `mshell` startet und beendet den Startup-Aufruf ohne
  Fehler. Der anschließende `S`-Loop ist der absichtliche aktuelle
  `Q9K_StartupIdle`, keine Exception. Kein `Vektor=11` und kein
  `E$MNF` mehr.
