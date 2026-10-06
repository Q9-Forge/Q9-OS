/*
 * q9ringbuf.h -- Ringpuffer als allgemeiner Baustein (Debug-Konzept
 * Abschnitt 2.8). Satzorientiert (Saetze variabler Laenge mit
 * Laengenfeld, nie halbe Saetze lesen), Modus ueberschreiben (neueste
 * behalten) oder anhalten-wenn-voll (aelteste behalten), mit Zaehlern
 * fuer geschriebene/verlorene Saetze und Lesestand.
 *
 * Bewusst reiner, freistehender C-Code ohne Kernel-Abhaengigkeiten --
 * host-testbar wie q9kernel_bitmap.c & Co. Der Puffer selbst liegt beim
 * Aufrufer (fester Speicherbereich im Kernel ODER ein Testarray); dieses
 * Modul kennt nur Anfang+Groesse+Lese-/Schreibzeiger, keine Kernel-
 * Adressen.
 *
 * "Jetzt (Paket 2) nur einfach" (Konzeptdokument 2.8): EIN Puffer fuer
 * den Trace, aber mit vollem Beschreibungsblock, sodass spaetere weitere
 * Instanzen (z.B. syslog) ohne Umbau dazukommen koennen.
 */
#ifndef Q9RINGBUF_H
#define Q9RINGBUF_H

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#define Q9RB_MODE_OVERWRITE 0  /* voll -> aelteste Saetze werden ueberschrieben */
#define Q9RB_MODE_HALT      1  /* voll -> neue Saetze werden verworfen, Anfang bleibt erhalten */

/* Beschreibungsblock (Konzeptdokument 2.8, Tabelle "Eigenschaft/Bedeutung").
 * "id"/"name" identifizieren die Instanz (Trace = Puffernummer 0), sind
 * hier aber reine Nutzdaten des Aufrufers -- dieses Modul vergleicht sie
 * nie, es adressiert ueber den uebergebenen Q9RingBuf* direkt. */
typedef struct Q9RingBuf {
    Q9_u8  *data;        /* Puffer-Rohspeicher, vom Aufrufer bereitgestellt */
    Q9_u32  size;         /* Groesse in Byte, beim Anlegen festgelegt */
    Q9_u32  writeOff;     /* naechster Schreib-Offset, 0..size-1 */
    Q9_u32  readOff;      /* aeltester noch lesbarer Offset (Lesestand), 0..size-1 */
    Q9_u32  used;         /* aktuell belegte Byte (writeOff-readOff modulo size) */
    Q9_u32  writtenCount; /* Zaehler: insgesamt geschriebene Saetze */
    Q9_u32  lostCount;    /* Zaehler: verlorene (ueberschriebene bzw. verworfene) Saetze */
    Q9_u8   mode;         /* Q9RB_MODE_OVERWRITE oder Q9RB_MODE_HALT */
    Q9_u8   id;           /* Puffernummer (Trace = 0) */
    char    name[16];     /* Anzeigename, z.B. "trace" -- 15 Zeichen + NUL */
} Q9RingBuf;

/* Anlegen (nur Feldzuweisung, kein malloc -- data/size kommen vom
 * Aufrufer, meist ein fest allozierter Kernelbereich). */
void Q9RingBufInit(Q9RingBuf *rb, Q9_u8 *data, Q9_u32 size,
                    Q9_u8 id, const char *name, Q9_u8 mode);

/* Schreibt einen Satz variabler Laenge. "recLen" ist die GESAMTE Satz-
 * laenge inklusive eines etwaigen Laengenfelds, das der AUFRUFER schon
 * in "rec" kodiert hat (dieses Modul kodiert kein eigenes Format, das
 * ist Sache von q9trace.c -- der Ringpuffer selbst ist formatlos und
 * kopiert nur Byte, s. Kopfkommentar "allgemeiner Baustein").
 * Rueckgabe: 1 = geschrieben, 0 = verworfen (Modus HALT und Puffer voll,
 * oder recLen > size). Bei Erfolg im OVERWRITE-Modus werden ggf. alte
 * Saetze am Lesekopf uebersprungen (deren Laenge steht an ihrem eigenen
 * Offset 0 -- s. Q9RingBufRecLenAt) und als "verloren" gezaehlt. */
int Q9RingBufWrite(Q9RingBuf *rb, const Q9_u8 *rec, Q9_u32 recLen);

/* Liest ab dem aktuellen Lesestand bis zu "maxBytes" in "dst", liefert
 * die Anzahl tatsaechlich kopierter Byte und ruckt den Lesestand genau
 * um VOLLE Saetze vor (nie ein halber Satz). "recLenFn" bestimmt, wie
 * dieses Modul die Laenge eines Satzes an einer gegebenen Position
 * erfaehrt -- bleibt NULL, liest es selbst Byte 0 jedes Satzes als
 * Laengenfeld (das von q9trace.c vorgesehene Format, s. dort: Offset +1
 * traegt die Laenge, +0 den Satztyp -- das Laengenbyte sitzt also bei
 * +1, nicht +0; dieses Modul liest deshalb IMMER ueber den von
 * Q9RINGBUF_LENOFF angegebenen Offset). */
#define Q9RINGBUF_LENOFF 1  /* Byte-Offset des Laengenfelds innerhalb eines Satzes */

Q9_u32 Q9RingBufRead(Q9RingBuf *rb, Q9_u8 *dst, Q9_u32 maxBytes,
                      Q9_u32 *lostOut);

/* Leert den Puffer (Lese-/Schreibzeiger zuruecksetzen, Zaehler NICHT
 * zuruecksetzen -- "geschriebene Saetze insgesamt" bleibt eine
 * monotone Grosse ueber die Kernel-Laufzeit, nur der Inhalt wird
 * verworfen). */
void Q9RingBufClear(Q9RingBuf *rb);

#endif /* Q9RINGBUF_H */
