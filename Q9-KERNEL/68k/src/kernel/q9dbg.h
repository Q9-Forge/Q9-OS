/*
 * q9dbg.h -- C-Header fuer das Debug-/Trace-Konzept
 * (docs/DEBUG_KONZEPT_de.md Abschnitt 2.5), Arbeitspaket 2.
 *
 * Enthaelt die Konstanten (Callcode, Unterfunktionen, Filterarten) und
 * ist bewusst so geschrieben, dass er GEFAHRLOS in jede Uebersetzungs-
 * einheit eingebunden werden kann (reine #define/typedef, kein Code,
 * keine Annahme ueber die Speicheradresse des Trace-Puffers) -- die
 * eigentliche Kernel-Anbindung (F$Q9Dbg-Handler, Trace-Puffer-Lage,
 * Dispatcher-Haken) ist NOCH NICHT Teil dieser Runde, s. Kopfkommentar
 * q9kernel_dbg.c (noch nicht angelegt) bzw. STATUS.md/
 * docs/OWN_KERNEL_STATUS.md fuer den Grund.
 *
 * Kleine Hilfsfunktionen (q9dbg_on() usw., Konzeptdokument 2.5
 * "Makros: ... ein C-Header (q9dbg.h) mit kleinen Funktionen") folgen,
 * sobald der echte Syscall registriert ist -- bisher waeren sie toter
 * Code ohne Gegenstelle.
 */
#ifndef Q9DBG_H
#define Q9DBG_H

/* Konfigurations-Syscall (Konzeptdokument 2.5). $71-$7F sind in MWOS
 * nirgends vergeben; $7F liegt am weitesten von kuenftigen Microware-
 * Codes entfernt. */
#define Q9DBG_CALLCODE 0x7f

/* Unterfunktionen (d0.w) */
#define Q9DBG_FN_VERSION      0  /* Version/Faehigkeiten abfragen */
#define Q9DBG_FN_TRACE_ONOFF  1  /* Trace an/aus, d1=0/1 */
#define Q9DBG_FN_MASK_SET     2  /* Syscall-Maske setzen, a0=32 Byte */
#define Q9DBG_FN_MASK_GET     3  /* Syscall-Maske lesen, a0=Ziel 32 Byte */
#define Q9DBG_FN_FILTER_SET   4  /* Filter setzen, d1=Filterart, d2=Wert */
#define Q9DBG_FN_LEVEL_SET    5  /* Detailstufe/Modus setzen, d1=Stufe, d2=Ausgabe/Modus */
#define Q9DBG_FN_RING_READ    6  /* Ringpuffer lesen, d3=Puffernummer, d1=ab Satznummer, d2=max Byte, a0=Ziel */
#define Q9DBG_FN_RING_CLEAR   7  /* Ringpuffer leeren, d3=Puffernummer */
#define Q9DBG_FN_IFUNC_MASK   8  /* Interne-Funktionen-Maske setzen, a0=Maske */

/* Puffernummern (Konzeptdokument 2.8: "0 = trace, spaeter syslog") */
#define Q9DBG_RING_TRACE 0

/* Filterarten fuer Q9DBG_FN_FILTER_SET (d1), identisch zu den
 * Q9TRACE_FILTER_*-Konstanten in q9trace.h -- hier noch einmal
 * eigenstaendig benannt, weil dieser Header absichtlich OHNE
 * q9trace.h auskommt (reiner Syscall-ABI-Header, keine Host-Logik-
 * Abhaengigkeit). */
#define Q9DBG_FILTER_PID       1
#define Q9DBG_FILTER_USER      2
#define Q9DBG_FILTER_PATH      3
#define Q9DBG_FILTER_FILEID    4
#define Q9DBG_FILTER_ERRORONLY 5
#define Q9DBG_FILTER_DETAIL    6

/* Ausgabeziel/Modus fuer Q9DBG_FN_LEVEL_SET (d2), Konzeptdokument 2.6 */
#define Q9DBG_OUT_RINGBUF     0  /* Standard: nur Ringpuffer */
#define Q9DBG_OUT_CONSOLE     1  /* nur Konsole (DUART) */
#define Q9DBG_OUT_BOTH        2  /* beides */

#endif /* Q9DBG_H */
