/*
 * q9trace.h -- Satzformat und Filterlogik des Syscall-Trace
 * (Debug-Konzept Abschnitt 2.2-2.4). Reiner, freistehender C-Code,
 * host-testbar; benutzt q9ringbuf.c als Speicher, kennt aber selbst
 * keine Kernel-Adressen (Tick-Zaehler/Prozess-ID werden dem Aufrufer
 * uebergeben, nicht hier gelesen).
 *
 * Satzkopf (12 Byte, danach n Byte Nutzdaten), big-endian,
 * 2-Byte-ausgerichtet -- Feldtabelle aus dem Konzeptdokument:
 *
 *   +0  1  Satztyp
 *   +1  1  Satzlaenge in Byte (einschliesslich Kopf)
 *   +2  1  Callcode bzw. Funktions-ID
 *   +3  1  Bits 0-3 Tiefe, Bit 4 Carry (Rueckkehr), Bit 5 gekuerzt
 *   +4  2  Prozess-ID
 *   +6  4  Tick-Zaehler seit Boot
 *   +10 2  Feinzeit innerhalb des Ticks
 *   +12 n  Nutzdaten nach Satztyp
 */
#ifndef Q9TRACE_H
#define Q9TRACE_H

#include "q9ringbuf.h"

#define Q9TRACE_HDR_LEN 12

/* Satztypen (Konzeptdokument 2.2) */
#define Q9TRACE_REC_ENTRY         1
#define Q9TRACE_REC_RETURN        2
#define Q9TRACE_REC_INTERN_ENTRY  3
#define Q9TRACE_REC_INTERN_RETURN 4
#define Q9TRACE_REC_LOST          5
#define Q9TRACE_REC_PROCINFO      6
#define Q9TRACE_REC_TIMEBASE      7

/* Flagsbyte (+3) */
#define Q9TRACE_FLAG_DEPTH_MASK 0x0f  /* Bits 0-3 */
#define Q9TRACE_FLAG_CARRY      0x10  /* Bit 4 */
#define Q9TRACE_FLAG_TRUNC      0x20  /* Bit 5 */

/* Obergrenze fuer einen einzelnen Satz -- Name/Pfad-Nutzdaten werden auf
 * 15 Zeichen gekappt (Konzeptdokument 2.2: "Namen ... werden mit bis zu
 * 15 Zeichen kopiert"), macht mit Kopf + Laengen-Vorspann bequem Platz
 * in einem Byte-Laengenfeld (max 255). */
#define Q9TRACE_MAX_NAME 15
#define Q9TRACE_MAX_REC  64

/* Filterarten fuer F$Q9Dbg Unterfunktion 4 (Konzeptdokument 2.5) --
 * dieselben Zahlen dienen als "Filterart" (d1) des Syscalls UND als
 * Feldbezeichner hier. */
#define Q9TRACE_FILTER_PID       1
#define Q9TRACE_FILTER_USER      2
#define Q9TRACE_FILTER_PATH      3
#define Q9TRACE_FILTER_FILEID    4
#define Q9TRACE_FILTER_ERRORONLY 5
#define Q9TRACE_FILTER_DETAIL    6

/* Syscall-Maske: 256 Callcodes, ein Bit je Code -- 32 Byte (Konzept-
 * dokument 2.4/2.5: "a0 = 32 Byte Maske"). Bit N von Byte N/8, LSB
 * zuerst innerhalb des Byte (willkuerliche, aber feste Konvention --
 * s. Q9TraceMaskTest/-Clear/-Set unten, immer darueber benutzen statt
 * das Bit selbst auszurechnen). */
#define Q9TRACE_MASK_BYTES 32

typedef struct Q9TraceFilter {
    Q9_u8 syscallMask[Q9TRACE_MASK_BYTES];
    Q9_u8 internalMask[Q9TRACE_MASK_BYTES]; /* Funktions-IDs, dieselbe Form */
    Q9_u16 pid;        /* 0 = alle */
    Q9_u16 user;       /* Gruppe.Benutzer gepackt (high=Gruppe,low=Benutzer), 0 = alle */
    Q9_u16 path;       /* prozesslokale Pfadnummer, 0xFFFF = kein Filter */
    Q9_u32 fileId;     /* Datei-ID (Geraet+Sektor), 0 = kein Filter.
                        * UMSETZUNG (Konzeptdokument Abschnitt 7, offener
                        * Punkt): welche I$GetStt-Unterfunktion bei RBF
                        * die FD-Sektornummer liefert, ist NICHT geklaert
                        * -- das Feld wird hier nur gespeichert/verglichen,
                        * niemand befuellt es bisher aus einem echten
                        * I$Open-Satz (das ist Werkzeugarbeit, Paket 4).
                        * Bewusster Platzhalter, nicht geraten. */
    Q9_u8  errorOnly;  /* 0/1 */
    Q9_u8  detail;     /* 1..3 */
} Q9TraceFilter;

void Q9TraceFilterInit(Q9TraceFilter *f); /* alles aus/alle (kein Filter, Detail 1) */

void Q9TraceMaskSet(Q9_u8 mask[Q9TRACE_MASK_BYTES], unsigned code);
void Q9TraceMaskClear(Q9_u8 mask[Q9TRACE_MASK_BYTES], unsigned code);
int  Q9TraceMaskTest(const Q9_u8 mask[Q9TRACE_MASK_BYTES], unsigned code);

/* Eingangskontext fuer die Filterentscheidung -- fuer Satztyp 1/2
 * (Syscall) ist "code" der Callcode, fuer 3/4 (intern) die Funktions-ID.
 * "isReturn"/"carrySet" betreffen nur die Filterregel "nur Fehler". */
typedef struct Q9TraceEvent {
    Q9_u8  recType;    /* Q9TRACE_REC_* */
    Q9_u8  code;       /* Callcode bzw. Funktions-ID */
    Q9_u16 pid;
    Q9_u16 user;
    Q9_u16 path;       /* 0xFFFF = keine Pfadnummer bekannt (z.B. interner Aufruf) */
    Q9_u32 fileId;     /* 0 = keine Datei-ID bekannt */
    int    isReturn;
    int    carrySet;
} Q9TraceEvent;

/* 1 = Satz wird gespeichert, 0 = durch Filter unterdrueckt. Wirkt VOR
 * dem Speichern (Konzeptdokument 2.4: "Alle Filter wirken vor dem
 * Speichern"). */
int Q9TraceShouldLog(const Q9TraceFilter *f, const Q9TraceEvent *ev);

/* Baut einen Eintritts-/Rueckkehr- bzw. intern-Eintritts-/Rueckkehr-Satz
 * in "out" (mind. Q9TRACE_MAX_REC Byte). "payload"/"payloadLen" sind die
 * schon fertig kodierten Nutzdaten (Paket 2: nur rohe Byte, z.B. bis zu
 * zwei 32-Bit-Werte fuer Q9K_TRACE_FN -- die Syscall-Beschreibungstabelle
 * aus 2.3 ist Paket 4 und entscheidet SPAETER, was genau hineingeht).
 * Kuerzt automatisch auf Q9TRACE_MAX_REC und setzt dabei Bit 5 (gekuerzt).
 * Rueckgabe: tatsaechliche Satzlaenge (>0) oder 0 bei ungueltigem Typ. */
Q9_u32 Q9TraceBuildRecord(Q9_u8 *out, Q9_u8 recType, Q9_u8 code,
                           unsigned depth, int carrySet,
                           Q9_u16 pid, Q9_u32 tick, Q9_u16 fineTime,
                           const Q9_u8 *payload, Q9_u32 payloadLen);

/* Satz Typ 5 ("verlorene Saetze") -- Nutzdaten: 4 Byte Anzahl. */
Q9_u32 Q9TraceBuildLostRecord(Q9_u8 *out, Q9_u16 pid, Q9_u32 tick,
                               Q9_u16 fineTime, Q9_u32 lostCount);

/* Satz Typ 6 (Prozess-Info) -- Nutzdaten: 2 Byte Benutzer (Gruppe.Nutzer
 * gepackt). */
Q9_u32 Q9TraceBuildProcInfoRecord(Q9_u8 *out, Q9_u16 pid, Q9_u32 tick,
                                   Q9_u16 fineTime, Q9_u16 user);

/* Satz Typ 7 (Zeitbasis) -- Nutzdaten: 4 Byte Datum/Uhrzeit (F$Time-
 * Rohformat, hier nur durchgereicht, nicht interpretiert) + 4 Byte Tick
 * + 2 Byte Feinzeit zum selben Moment (dasselbe tick/fineTime wie die
 * Kopf-Felder, absichtlich redundant -- das Tool braucht diese Angaben
 * als REFERENZPAAR, nicht als laufende Kopf-Zeit). */
Q9_u32 Q9TraceBuildTimeBaseRecord(Q9_u8 *out, Q9_u32 tick, Q9_u16 fineTime,
                                   Q9_u32 timeRaw);

/* Zerlegt den 12-Byte-Kopf eines Satzes an "rec" zurueck in die
 * Einzelfelder (fuer Hosttests UND spaeter den Host-Dekoder, Paket 4). */
typedef struct Q9TraceRecHdr {
    Q9_u8  recType;
    Q9_u8  recLen;
    Q9_u8  code;
    Q9_u8  flags;
    Q9_u16 pid;
    Q9_u32 tick;
    Q9_u16 fineTime;
} Q9TraceRecHdr;

void Q9TraceDecodeHeader(const Q9_u8 *rec, Q9TraceRecHdr *hdr);

#endif /* Q9TRACE_H */
