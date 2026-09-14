# Q9 Kernel – Scheduler-, Timer- und Echtzeit-Konzept

## Ziel

Der Q9-Kernel soll grundsätzlich mit dem klassischen OS-9-Scheduling kompatibel bleiben, intern aber moderne Scheduler- und Timer-Techniken verwenden können.

Wichtig ist dabei:

- OS-9-Kompatibilität erhalten
- Echtzeitfähigkeit und Determinismus nicht verschlechtern
- SMP/Mehrkernbetrieb von Anfang an berücksichtigen
- neue Scheduling-Verfahren experimentell zuschaltbar machen
- Legacy-Verhalten jederzeit als Referenz- und Debugmodus behalten

---

## 1. Scheduler-Modi

Der Scheduler soll nicht fest auf einen einzigen Algorithmus verdrahtet werden.

Stattdessen wird eine kleine Scheduler-Abstraktionsschicht vorgesehen. Verschiedene Betriebsarten können registriert und ausgewählt werden.

Denkbare erste Modi:

- `LEGACY_TICK`
- `STAGGERED_TICK`
- `TICKLESS`

Später können weitere Scheduling-Algorithmen hinzukommen.

Der Kernel sollte dafür eine zentrale Konfiguration wie beispielsweise

`sched_mode`

besitzen.

Der klassische OS-9-kompatible Scheduler bleibt dauerhaft vorhanden und dient als Referenz- und Fallbackmodus.

Damit können neue Scheduler zunächst im Emulator getestet und bei Problemen schnell wieder deaktiviert werden.

---

## 2. Sicherer Systemstart

Q9 sollte zunächst in einem möglichst klassischen und einfachen Modus booten.

Beispielsweise:

`Boot -> LEGACY_TICK -> SMP initialisieren -> moderner Scheduler`

Der Wechsel zu einem moderneren Scheduler erfolgt erst an einem definierten Synchronisationspunkt.

Beim Umschalten müssen insbesondere

- Timer erhalten bleiben,
- Ablaufzeiten korrekt übernommen werden,
- Run-Queues konsistent sein,
- laufende Prozesse korrekt behandelt werden.

Der Schedulerwechsel darf keinen Sleep, Timeout oder Prozesszustand verlieren.

---

## 3. Tickless Scheduling / High-Resolution Timer

Der klassische Kernel erzeugt regelmäßig Timerinterrupts, beispielsweise alle 10 ms oder 1 ms, unabhängig davon, ob tatsächlich etwas zu tun ist.

Q9 soll optional tickless arbeiten können.

Statt eines regelmäßigen Interrupts wird ermittelt:

**Wann ist das nächste zeitabhängige Kernelereignis?**

Beispiele:

- Ende einer Zeitscheibe
- Prozess muss aus `sleep()` aufwachen
- I/O-Timeout
- Kernel-Timer
- Netzwerk-Timeout

Der Hardwaretimer wird direkt auf das früheste Ereignis programmiert.

Prinzip:

`next_interrupt = min(next_scheduler_event, next_timer_event, ...)`

Wenn beispielsweise erst in 37 ms etwas passieren muss, braucht der Kernel vorher keinen Timerinterrupt.

Das reduziert unnötige Interrupts und ermöglicht gleichzeitig wesentlich genauere Timer.

---

## 4. Absolute Zeitbasis

Intern sollten Timer möglichst mit absoluten Ablaufzeitpunkten arbeiten und nicht ausschließlich mit herunterzählenden Tick-Countern.

Beispiel:

`expires = monotonic_time + delay`

Dadurch kann der Kernel unabhängig davon arbeiten, ob gerade ein periodischer Tick vorhanden ist.

Für OS-9-Kompatibilität kann weiterhin ein virtueller klassischer Systemtick bereitgestellt werden.

Damit ergibt sich eine klare Trennung:

`OS-9 Zeitmodell`
↓
`Q9 Timer-Abstraktion`
↓
`Hardwaretimer / CPU-local Timer`

Alte Programme sehen weiterhin das erwartete OS-9-Verhalten.

---

## 5. SMP – lokale CPU-Timer

Bei Mehrprozessorsystemen soll möglichst jede CPU bzw. jeder Core einen eigenen lokalen Timer verwenden.

Jede CPU kann dadurch unabhängig bestimmen, wann ihr nächster Scheduler- oder Timerinterrupt notwendig ist.

Beispiel:

- CPU0: nächstes Ereignis in 3,2 ms
- CPU1: nächstes Ereignis in 17 ms
- CPU2: kein Ereignis, CPU idle
- CPU3: RT-Prozess muss in 200 µs geweckt werden

Die CPUs müssen damit keinen gemeinsamen periodischen Timerinterrupt bekommen.

Wenn eine CPU eine andere CPU wegen eines Scheduler-Ereignisses aufwecken oder zur Reschedule auffordern muss, kann dafür ein IPI verwendet werden.

---

## 6. Gestaffelte Timerinterrupts

Falls ein periodischer Tick verwendet wird, sollen die Timerinterrupts der CPUs möglichst nicht gleichzeitig auftreten.

Bei vier CPUs und beispielsweise 1 ms Tick:

- CPU0:   0 µs
- CPU1: 250 µs
- CPU2: 500 µs
- CPU3: 750 µs

Danach arbeitet jede CPU wieder mit ihrem normalen 1-ms-Rhythmus.

Vorteile:

- weniger gleichzeitige Zugriffe auf gemeinsame Kernelstrukturen
- weniger Lock-Contention
- weniger Bus-/Cache-Spitzen
- Scheduler laufen nicht unnötig gleichzeitig

Der Offset könnte automatisch berechnet werden:

`phase_offset = cpu_id * tick_period / cpu_count`

Diese Funktion sollte Teil der SMP-Timerarchitektur sein und kein späterer Workaround.

---

## 7. Run-Queues

Für SMP sind langfristig lokale Run-Queues pro CPU sinnvoll.

Beispielsweise:

`CPU0 -> run_queue[0]`
`CPU1 -> run_queue[1]`
`CPU2 -> run_queue[2]`
`CPU3 -> run_queue[3]`

Ein Prozess kann bevorzugt auf der CPU weiterlaufen, auf der er zuletzt ausgeführt wurde. Das verbessert Cache-Lokalität.

Load-Balancing kann Prozesse bei Bedarf auf andere CPUs verschieben.

Ein typischer Ablauf wäre:

`Timer läuft ab`
→ `Task wird READY`
→ `geeignete CPU bestimmen`
→ `Task in deren Run-Queue einfügen`
→ `gegebenenfalls IPI an Ziel-CPU`

CPU-Affinity sollte vorgesehen werden, insbesondere für Echtzeitprozesse.

---

## 8. Timer-Queues

Für eine erste SMP-Version kann eine gemeinsame Timerverwaltung einfacher und sicherer sein.

Später können Timer-Queues ebenfalls pro CPU geführt werden.

Eine sinnvolle Regel wäre:

**Ein Timer gehört zunächst der CPU, auf der der betreffende Prozess zuletzt gelaufen ist.**

Damit bleiben Timer und Prozess möglichst CPU-lokal.

Migration muss sauber definiert werden. Es muss jederzeit eindeutig sein, welche CPU für einen Timer verantwortlich ist.

---

## 9. OS-9-Kompatibilität

Die moderne Timerarchitektur darf die von OS-9 sichtbare Scheduler-Semantik nicht verändern.

Insbesondere müssen erhalten bleiben:

- Prioritäten
- Prozesszustände
- Sleep/Timeout-Verhalten
- Zeitscheiben
- Preemption
- OS-9-Zeitfunktionen
- erwartete Scheduler-Reihenfolge soweit kompatibilitätsrelevant

Die Implementierung unterhalb dieser Schnittstelle darf dagegen moderner sein.

Grundprinzip:

**Kompatibilität oben – moderne Mechanik unten.**

---

## 10. Echtzeitfähigkeit

Die neuen Verfahren dürfen die Echtzeitfähigkeit des Kernels nicht gefährden.

Wichtige Anforderungen:

### Begrenzte Interrupt-Latenz

Interrupts dürfen nicht über längere, unvorhersehbare Zeit deaktiviert werden.

### Deterministische Scheduler-Laufzeit

Scheduleroperationen müssen eine bekannte bzw. begrenzte maximale Laufzeit besitzen.

### Keine unbeschränkten Locks

Bei SMP dürfen Locks Echtzeitprozesse nicht beliebig lange blockieren.

Priority Inversion muss berücksichtigt werden. Denkbar ist beispielsweise Priority Inheritance.

### Keine unvorhersehbare Speicherallokation

Zeitkritische Kernelpfade sollten keine normalen dynamischen Speicherallokationen benötigen.

Geeigneter sind:

- vorbereitete Strukturen
- feste Pools
- per-CPU Pools

### Begrenzte Timerverwaltung

Timeroperationen dürfen keine unbeschränkt langen Suchvorgänge verursachen.

Für kleine typische OS-9-Systeme kann eine einfache sortierte Queue ausreichend sein. Bei größeren Systemen könnten später Heap oder Timer-Wheel verwendet werden.

### CPU-Affinity

Echtzeitprozesse sollten optional an eine CPU gebunden werden können.

Dadurch werden Migration und Cacheeffekte vermieden und die Laufzeit besser vorhersehbar.

---

## 11. Tickless und Echtzeit

Tickless Scheduling steht Echtzeit nicht entgegen.

Im Gegenteil: Ein hochauflösender lokaler Timer kann einen Prozess wesentlich genauer wecken als ein klassischer Systemtick.

Beispiel:

Ein Prozess soll in 317 µs wieder laufen.

Klassischer 1-ms-Tick:

Auflösung bzw. Verzögerung kann bis ungefähr 1 ms betragen.

High-Resolution/tickless:

Timer wird direkt auf den gewünschten Zeitpunkt programmiert.

Damit kann Q9 trotz OS-9-Kompatibilität eine bessere zeitliche Auflösung erreichen.

---

## 12. Scheduler-Plugins / experimentelle Algorithmen

Die Scheduler-Abstraktion soll später verschiedene Algorithmen ermöglichen.

Beispiele:

- klassischer OS-9-Scheduler
- Round-Robin
- Prioritätsscheduler
- Fair Scheduler
- SMP Load-Balancing
- experimentelle Algorithmen

Neue Algorithmen können dadurch im Emulator getestet werden, ohne den Legacy-Scheduler zu entfernen.

Interessant wäre insbesondere:

**denselben Workload mit verschiedenen Scheduler-Modi laufen lassen und die Kernel-Traces vergleichen.**

Damit wird der Q9-Emulator gleichzeitig zu einer Testumgebung für Schedulerentwicklung.

---

## 13. Echtzeit und experimentelle Scheduler

Echtzeitprozesse sollten nicht davon abhängig sein, ob gerade ein experimenteller normaler Scheduler aktiv ist.

Denkbare Architektur:

`RT Scheduler`
+
`Normal Scheduler`

Ein RT-Prozess mit höherer Priorität hat immer Vorrang vor normalen Prozessen.

Bei gleicher RT-Priorität kann beispielsweise FIFO oder Round-Robin verwendet werden.

Scheduler-Implementierungen könnten außerdem kennzeichnen, ob sie garantierte Echtzeiteigenschaften besitzen.

Ein experimenteller Scheduler ohne garantierte Laufzeitgrenzen darf dann nicht für harte Echtzeitanforderungen verwendet werden.

---

## 14. Kernel-Tracing und Panic-Dump

Bereits als wichtiges Q9-Feature vorgesehen:

Ein sehr leichtgewichtiges Kernel-Tracing schreibt Ereignisse in einen festen Ringbuffer.

Beispiele:

- `TRACE_IRQ`
- `TRACE_SYSCALL`
- `TRACE_SCHED`
- `TRACE_MODULE`
- `TRACE_IO`
- Timer-Ereignisse
- CPU-Migration
- IPI
- Schedulerwechsel

Tracing darf niemals einen Echtzeitprozess blockieren.

Ist der Buffer voll, werden notfalls Traceinformationen überschrieben oder verworfen.

Bei Kernel-Panic sollen die letzten Traceinformationen zusammen mit beispielsweise

- Registern
- aktuellem Prozess
- CPU
- Stack
- Schedulerzustand
- Run-Queue-Zustand

gesichert bzw. nach außen übertragen werden können.

Im Emulator können Traceinformationen zusätzlich live nach außen geleitet werden.

---

## 15. Noch zu klären: klassische OS-9-Echtzeit-/Prioritätssemantik

Es muss anhand der originalen OS-9-Dokumentation genau geprüft werden, wie der klassische Scheduler Prioritäten und Aging behandelt.

Insbesondere ist `D_MaxAge` zu untersuchen.

Zu klären:

- Welche Prozesse unterliegen dem Aging?
- Welche Prioritäten werden nicht gealtert?
- Gibt es dadurch faktisch einen Echtzeit-Prioritätsbereich?
- Wie unterscheidet sich dies zwischen OS-9/68k und OS-9000?
- Welche dieser Eigenschaften müssen für binäre bzw. API-Kompatibilität exakt erhalten bleiben?

Diese Untersuchung sollte erfolgen, **bevor die endgültige Q9-RT-Scheduler-Semantik festgelegt wird.**

---

# Vorgeschlagene Entwicklungsreihenfolge

1. Klassisches OS-9-Schedulerverhalten exakt dokumentieren.
2. Legacy-Scheduler als Referenz implementieren.
3. Einheitliche Scheduler-Abstraktionsschicht definieren.
4. Monotone hochauflösende Zeitbasis einführen.
5. Timer auf absolute Ablaufzeiten umstellen.
6. Tickless-Modus zunächst auf einer CPU implementieren.
7. Kernel-Tracing für Scheduler und Timer integrieren.
8. SMP mit per-CPU Run-Queues einführen.
9. Lokale CPU-Timer verwenden.
10. Periodische Timer gegebenenfalls phasenverschieben.
11. CPU-Affinity und IPI-Rescheduling ergänzen.
12. Echtzeitverhalten und maximale Latenzen messen.
13. Erst danach alternative Scheduler-Algorithmen ausprobieren.

## Leitgedanke

Der Q9-Kernel soll **nach außen OS-9 bleiben**, intern aber moderne Timer-, SMP- und Scheduling-Techniken verwenden können.

Der klassische Scheduler wird dabei nicht weggeworfen. Er bleibt als kompatibler Referenzmodus erhalten.

Neue Techniken werden schrittweise und abschaltbar ergänzt, sodass jederzeit zwischen einem bekannten Legacy-Verhalten und experimentellen modernen Verfahren gewechselt werden kann.

Heimfahrt begleiten

Klar — hier ist die Zusammenfassung so formuliert, dass du sie direkt an die nächsten Agenten geben kannst. Ich nehme dabei die \*\*beschlossenen Ideen und die wichtigen Designgrenzen\*\* mit hinein. :::writing{variant="document" id="48317"}

\# Q9 Kernel – Scheduler-, Timer- und Echtzeit-Konzept

\## Ziel Der Q9-Kernel soll grundsätzlich mit dem klassischen OS-9-Scheduling kompatibel bleiben, intern aber moderne Scheduler- und Timer-Techniken verwenden können. Wichtig ist dabei:

&#x20; - OS-9-Kompatibilität erhalten

&#x20; - Echtzeitfähigkeit und Determinismus nicht verschlechtern

&#x20; - SMP/Mehrkernbetrieb von Anfang an berücksichtigen

&#x20; - neue Scheduling-Verfahren experimentell zuschaltbar machen

&#x20; - Legacy-Verhalten jederzeit als Referenz- und Debugmodus behalten ---

\## 1. Scheduler-Modi Der Scheduler soll nicht fest auf einen einzigen Algorithmus verdrahtet werden. Stattdessen wird eine kleine Scheduler-Abstraktionsschicht vorgesehen. Verschiedene Betriebsarten können registriert und ausgewählt werden. Denkbare erste Modi: - `LEGACY\_TICK` - `STAGGERED\_TICK` - `TICKLESS` Später können weitere Scheduling-Algorithmen hinzukommen. Der Kernel sollte dafür eine zentrale Konfiguration wie beispielsweise `sched\_mode` besitzen. Der klassische OS-9-kompatible Scheduler bleibt dauerhaft vorhanden und dient als Referenz- und Fallbackmodus. Damit können neue Scheduler zunächst im Emulator getestet und bei Problemen schnell wieder deaktiviert werden. ---

\## 2. Sicherer Systemstart Q9 sollte zunächst in einem möglichst klassischen und einfachen Modus booten. Beispielsweise: `Boot -> LEGACY\_TICK -> SMP initialisieren -> moderner Scheduler` Der Wechsel zu einem moderneren Scheduler erfolgt erst an einem definierten Synchronisationspunkt. Beim Umschalten müssen insbesondere - Timer erhalten bleiben, - Ablaufzeiten korrekt übernommen werden, - Run-Queues konsistent sein, - laufende Prozesse korrekt behandelt werden. Der Schedulerwechsel darf keinen Sleep, Timeout oder Prozesszustand verlieren. ---

\## 3. Tickless Scheduling / High-Resolution Timer Der klassische Kernel erzeugt regelmäßig Timerinterrupts, beispielsweise alle 10 ms oder 1 ms, unabhängig davon, ob tatsächlich etwas zu tun ist. Q9 soll optional tickless arbeiten können. Statt eines regelmäßigen Interrupts wird ermittelt: \*\*Wann ist das nächste zeitabhängige Kernelereignis?\*\* Beispiele: - Ende einer Zeitscheibe - Prozess muss aus `sleep()` aufwachen - I/O-Timeout - Kernel-Timer - Netzwerk-Timeout Der Hardwaretimer wird direkt auf das früheste Ereignis programmiert. Prinzip: `next\_interrupt = min(next\_scheduler\_event, next\_timer\_event, ...)` Wenn beispielsweise erst in 37 ms etwas passieren muss, braucht der Kernel vorher keinen Timerinterrupt. Das reduziert unnötige Interrupts und ermöglicht gleichzeitig wesentlich genauere Timer. ---

\## 4. Absolute Zeitbasis Intern sollten Timer möglichst mit absoluten Ablaufzeitpunkten arbeiten und nicht ausschließlich mit herunterzählenden Tick-Countern. Beispiel: `expires = monotonic\_time + delay` Dadurch kann der Kernel unabhängig davon arbeiten, ob gerade ein periodischer Tick vorhanden ist. Für OS-9-Kompatibilität kann weiterhin ein virtueller klassischer Systemtick bereitgestellt werden. Damit ergibt sich eine klare Trennung: `OS-9 Zeitmodell` ↓ `Q9 Timer-Abstraktion` ↓ `Hardwaretimer / CPU-local Timer` Alte Programme sehen weiterhin das erwartete OS-9-Verhalten. ---

\## 5. SMP – lokale CPU-Timer Bei Mehrprozessorsystemen soll möglichst jede CPU bzw. jeder Core einen eigenen lokalen Timer verwenden. Jede CPU kann dadurch unabhängig bestimmen, wann ihr nächster Scheduler- oder Timerinterrupt notwendig ist. Beispiel: - CPU0: nächstes Ereignis in 3,2 ms - CPU1: nächstes Ereignis in 17 ms - CPU2: kein Ereignis, CPU idle - CPU3: RT-Prozess muss in 200 µs geweckt werden Die CPUs müssen damit keinen gemeinsamen periodischen Timerinterrupt bekommen. Wenn eine CPU eine andere CPU wegen eines Scheduler-Ereignisses aufwecken oder zur Reschedule auffordern muss, kann dafür ein IPI verwendet werden. ---

\## 6. Gestaffelte Timerinterrupts Falls ein periodischer Tick verwendet wird, sollen die Timerinterrupts der CPUs möglichst nicht gleichzeitig auftreten. Bei vier CPUs und beispielsweise 1 ms Tick: - CPU0: 0 µs - CPU1: 250 µs - CPU2: 500 µs - CPU3: 750 µs Danach arbeitet jede CPU wieder mit ihrem normalen 1-ms-Rhythmus. Vorteile: - weniger gleichzeitige Zugriffe auf gemeinsame Kernelstrukturen - weniger Lock-Contention - weniger Bus-/Cache-Spitzen - Scheduler laufen nicht unnötig gleichzeitig Der Offset könnte automatisch berechnet werden: `phase\_offset = cpu\_id \* tick\_period / cpu\_count` Diese Funktion sollte Teil der SMP-Timerarchitektur sein und kein späterer Workaround. ---

\## 7. Run-Queues Für SMP sind langfristig lokale Run-Queues pro CPU sinnvoll. Beispielsweise: `CPU0 -> run\_queue\[0]` `CPU1 -> run\_queue\[1]` `CPU2 -> run\_queue\[2]` `CPU3 -> run\_queue\[3]` Ein Prozess kann bevorzugt auf der CPU weiterlaufen, auf der er zuletzt ausgeführt wurde. Das verbessert Cache-Lokalität. Load-Balancing kann Prozesse bei Bedarf auf andere CPUs verschieben. Ein typischer Ablauf wäre: `Timer läuft ab` → `Task wird READY` → `geeignete CPU bestimmen` → `Task in deren Run-Queue einfügen` → `gegebenenfalls IPI an Ziel-CPU` CPU-Affinity sollte vorgesehen werden, insbesondere für Echtzeitprozesse. ---

\## 8. Timer-Queues Für eine erste SMP-Version kann eine gemeinsame Timerverwaltung einfacher und sicherer sein. Später können Timer-Queues ebenfalls pro CPU geführt werden. Eine sinnvolle Regel wäre: \*\*Ein Timer gehört zunächst der CPU, auf der der betreffende Prozess zuletzt gelaufen ist.\*\* Damit bleiben Timer und Prozess möglichst CPU-lokal. Migration muss sauber definiert werden. Es muss jederzeit eindeutig sein, welche CPU für einen Timer verantwortlich ist. ---

\## 9. OS-9-Kompatibilität Die moderne Timerarchitektur darf die von OS-9 sichtbare Scheduler-Semantik nicht verändern. Insbesondere müssen erhalten bleiben: - Prioritäten - Prozesszustände - Sleep/Timeout-Verhalten - Zeitscheiben - Preemption - OS-9-Zeitfunktionen - erwartete Scheduler-Reihenfolge soweit kompatibilitätsrelevant Die Implementierung unterhalb dieser Schnittstelle darf dagegen moderner sein. Grundprinzip: \*\*Kompatibilität oben – moderne Mechanik unten.\*\* ---

\## 10. Echtzeitfähigkeit Die neuen Verfahren dürfen die Echtzeitfähigkeit des Kernels nicht gefährden. Wichtige Anforderungen: ### Begrenzte Interrupt-Latenz Interrupts dürfen nicht über längere, unvorhersehbare Zeit deaktiviert werden. ### Deterministische Scheduler-Laufzeit Scheduleroperationen müssen eine bekannte bzw. begrenzte maximale Laufzeit besitzen. ### Keine unbeschränkten Locks Bei SMP dürfen Locks Echtzeitprozesse nicht beliebig lange blockieren. Priority Inversion muss berücksichtigt werden. Denkbar ist beispielsweise Priority Inheritance. ### Keine unvorhersehbare Speicherallokation Zeitkritische Kernelpfade sollten keine normalen dynamischen Speicherallokationen benötigen. Geeigneter sind: - vorbereitete Strukturen - feste Pools - per-CPU Pools ### Begrenzte Timerverwaltung Timeroperationen dürfen keine unbeschränkt langen Suchvorgänge verursachen. Für kleine typische OS-9-Systeme kann eine einfache sortierte Queue ausreichend sein. Bei größeren Systemen könnten später Heap oder Timer-Wheel verwendet werden. ### CPU-Affinity Echtzeitprozesse sollten optional an eine CPU gebunden werden können. Dadurch werden Migration und Cacheeffekte vermieden und die Laufzeit besser vorhersehbar. ---

\## 11. Tickless und Echtzeit Tickless Scheduling steht Echtzeit nicht entgegen. Im Gegenteil: Ein hochauflösender lokaler Timer kann einen Prozess wesentlich genauer wecken als ein klassischer Systemtick. Beispiel: Ein Prozess soll in 317 µs wieder laufen. Klassischer 1-ms-Tick: Auflösung bzw. Verzögerung kann bis ungefähr 1 ms betragen. High-Resolution/tickless: Timer wird direkt auf den gewünschten Zeitpunkt programmiert. Damit kann Q9 trotz OS-9-Kompatibilität eine bessere zeitliche Auflösung erreichen. ---

\## 12. Scheduler-Plugins / experimentelle Algorithmen Die Scheduler-Abstraktion soll später verschiedene Algorithmen ermöglichen. Beispiele: - klassischer OS-9-Scheduler - Round-Robin - Prioritätsscheduler - Fair Scheduler - SMP Load-Balancing - experimentelle Algorithmen Neue Algorithmen können dadurch im Emulator getestet werden, ohne den Legacy-Scheduler zu entfernen. Interessant wäre insbesondere: \*\*denselben Workload mit verschiedenen Scheduler-Modi laufen lassen und die Kernel-Traces vergleichen.\*\* Damit wird der Q9-Emulator gleichzeitig zu einer Testumgebung für Schedulerentwicklung. ---

\## 13. Echtzeit und experimentelle Scheduler Echtzeitprozesse sollten nicht davon abhängig sein, ob gerade ein experimenteller normaler Scheduler aktiv ist. Denkbare Architektur: `RT Scheduler` + `Normal Scheduler` Ein RT-Prozess mit höherer Priorität hat immer Vorrang vor normalen Prozessen. Bei gleicher RT-Priorität kann beispielsweise FIFO oder Round-Robin verwendet werden. Scheduler-Implementierungen könnten außerdem kennzeichnen, ob sie garantierte Echtzeiteigenschaften besitzen. Ein experimenteller Scheduler ohne garantierte Laufzeitgrenzen darf dann nicht für harte Echtzeitanforderungen verwendet werden. --- ## 14. Kernel-Tracing und Panic-Dump Bereits als wichtiges Q9-Feature vorgesehen: Ein sehr leichtgewichtiges Kernel-Tracing schreibt Ereignisse in einen festen Ringbuffer. Beispiele: - `TRACE\_IRQ` - `TRACE\_SYSCALL` - `TRACE\_SCHED` - `TRACE\_MODULE` - `TRACE\_IO` - Timer-Ereignisse - CPU-Migration - IPI - Schedulerwechsel Tracing darf niemals einen Echtzeitprozess blockieren. Ist der Buffer voll, werden notfalls Traceinformationen überschrieben oder verworfen. Bei Kernel-Panic sollen die letzten Traceinformationen zusammen mit beispielsweise - Registern - aktuellem Prozess - CPU - Stack - Schedulerzustand - Run-Queue-Zustand gesichert bzw. nach außen übertragen werden können. Im Emulator können Traceinformationen zusätzlich live nach außen geleitet werden. ---

\## 15. Noch zu klären: klassische OS-9-Echtzeit-/Prioritätssemantik Es muss anhand der originalen OS-9-Dokumentation genau geprüft werden, wie der klassische Scheduler Prioritäten und Aging behandelt. Insbesondere ist `D\_MaxAge` zu untersuchen. Zu klären: - Welche Prozesse unterliegen dem Aging? - Welche Prioritäten werden nicht gealtert? - Gibt es dadurch faktisch einen Echtzeit-Prioritätsbereich? - Wie unterscheidet sich dies zwischen OS-9/68k und OS-9000? - Welche dieser Eigenschaften müssen für binäre bzw. API-Kompatibilität exakt erhalten bleiben? Diese Untersuchung sollte erfolgen, \*\*bevor die endgültige Q9-RT-Scheduler-Semantik festgelegt wird.\*\* ---

\# Vorgeschlagene Entwicklungsreihenfolge

1\. Klassisches OS-9-Schedulerverhalten exakt dokumentieren.

2\. Legacy-Scheduler als Referenz implementieren.

3\. Einheitliche Scheduler-Abstraktionsschicht definieren.

4\. Monotone hochauflösende Zeitbasis einführen.

5\. Timer auf absolute Ablaufzeiten umstellen.

6\. Tickless-Modus zunächst auf einer CPU implementieren.

7\. Kernel-Tracing für Scheduler und Timer integrieren.

8\. SMP mit per-CPU Run-Queues einführen.

9\. Lokale CPU-Timer verwenden.

10\. Periodische Timer gegebenenfalls phasenverschieben.

11\. CPU-Affinity und IPI-Rescheduling ergänzen.

12\. Echtzeitverhalten und maximale Latenzen messen.

13\. Erst danach alternative Scheduler-Algorithmen ausprobieren.

\## Leitgedanke Der Q9-Kernel soll \*\*nach außen OS-9 bleiben\*\*, intern aber moderne Timer-, SMP- und Scheduling-Techniken verwenden können. Der klassische Scheduler wird dabei nicht weggeworfen. Er bleibt als kompatibler Referenzmodus erhalten. Neue Techniken werden schrittweise und abschaltbar ergänzt, sodass jederzeit zwischen einem bekannten Legacy-Verhalten und experimentellen modernen Verfahren gewechselt werden kann. ::: Den Abschnitt über \*\*`D\_MaxAge` und die genaue OS-9-Echtzeitdefinition\*\* habe ich bewusst als \*noch zu verifizieren\* markiert. Das sollten wir anhand der Microware-Unterlagen wirklich sauber klären, bevor ein Agent daraus eine feste Architekturannahme macht. Das war gestern Abend genau der Punkt, an dem wir aufgehört hatten.

Alle mit dem Link können diesen Chat ansehen

haben wir noch über andere wichtige Themen gesprochen ?

Ja. Neben dem Scheduler gab es in dem Gespräch noch ein paar Punkte, die ich für den Q9-Kernel festhalten würde.
Der wichtigste war Kernel-Tracing + Panic-Dump. Das hattest du ziemlich klar als „gesetzt“ betrachtet: leichter Ringbuffer für IRQs, Systemcalls, Scheduler, I/O, Moduloperationen usw.; beim Panic Register, Prozess, Stack und die letzten Trace-Ereignisse retten. Außerdem hatten wir über verschiedene Wege gesprochen, diese Daten nach einem Crash nach außen zu bekommen. Das war allerdings teilweise schon auf der Hinfahrt besprochen worden.
Außerdem kamen diese Themen auf:

- Einheitlicher Event-/Wait-Mechanismus: Eine gemeinsame Kernelabstraktion, auf der Sleep/Wakeup, I/O-Warten, Timer, Netzwerk und eventuell IPC aufbauen können. Statt vieler spezieller Wartemechanismen also ein kleines allgemeines Wait-/Event-Objekt.
- RCU-artige Mechanismen: Für SMP eventuell später lockarmes Lesen häufig gelesener, selten veränderter Kernelstrukturen. Das hatten wir ausdrücklich eher als spätere Ausbaustufe gesehen, nicht für den ersten Kernel.
- Scheduler als austauschbare Komponente: Das steckt zwar in meiner Zusammenfassung, war aber eigentlich eine größere Architekturidee: Nicht nur tick versus tickless, sondern später wirklich unterschiedliche Scheduling-Algorithmen ausprobieren können.
- Legacy als dauerhafter Fallback: Der klassische OS-9-Scheduler soll nicht nach der Entwicklung verschwinden. Er bleibt als Referenz-, Kompatibilitäts- und Debugmodus erhalten.
- RT-Sicherheit neuer Funktionen: Tracing darf beispielsweise niemals blockieren; kritische Kernelpfade sollen keine unvorhersehbaren Allokationen machen; Locks und Interrupt-Sperrzeiten brauchen begrenzte Laufzeiten.
- OS-9-Echtzeitdefinition nachforschen: Ganz am Ende kamen wir auf deine Erinnerung, dass OS-9 möglicherweise doch eine Art Prioritätsgrenze besitzt. Dabei fiel D_MaxAge. Das sollten wir noch anhand originaler Microware-Dokumentation verifizieren, denn daraus könnte sich ergeben, dass OS-9 faktisch bereits zwischen normal alternden und streng priorisierten Prozessen unterscheidet.
Und noch eine Sache würde ich als übergeordnetes Architekturprinzip aus dem Gespräch herausziehen: Wir wollten moderne Funktionen möglichst unterhalb der OS-9-Schnittstelle verstecken. Also kein „OS-9 wird jetzt Linux“, sondern OS-9-Semantik außen und moderne Implementierung innen. Das zieht sich eigentlich durch Timer, SMP, Scheduler und Tracing.
Die große Zusammenfassung von eben deckt damit fast alles Wesentliche ab. Event/Wait und RCU fehlen dort allerdings als eigene Architekturpunkte. Die würde ich noch ergänzen, wenn das Dokument wirklich als Übergabe an die Agenten dienen soll.
