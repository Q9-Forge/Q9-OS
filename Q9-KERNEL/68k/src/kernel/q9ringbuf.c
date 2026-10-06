/*
 * q9ringbuf.c -- Implementierung, s. q9ringbuf.h fuer das Konzept.
 */
#include "q9ringbuf.h"

void Q9RingBufInit(Q9RingBuf *rb, Q9_u8 *data, Q9_u32 size,
                    Q9_u8 id, const char *name, Q9_u8 mode)
{
    Q9_u32 i;

    rb->data = data;
    rb->size = size;
    rb->writeOff = 0;
    rb->readOff = 0;
    rb->used = 0;
    rb->writtenCount = 0;
    rb->lostCount = 0;
    rb->lostFmt = 0;
    rb->mode = mode;
    rb->id = id;

    for (i = 0; i < sizeof(rb->name) - 1 && name[i] != '\0'; i++)
        rb->name[i] = name[i];
    rb->name[i] = '\0';
}

void Q9RingBufSetLostFormatter(Q9RingBuf *rb, Q9RingBufLostFmt fmt)
{
    rb->lostFmt = fmt;
}

/* Laenge des Satzes, dessen ERSTES Byte an "off" liegt (zirkulaer). */
static Q9_u32 RecLenAt(const Q9RingBuf *rb, Q9_u32 off)
{
    Q9_u32 lenOff = off + Q9RINGBUF_LENOFF;
    if (lenOff >= rb->size)
        lenOff -= rb->size;
    return (Q9_u32)rb->data[lenOff];
}

/* Kopiert "len" Byte aus "src" zirkulaer ab "off" in rb->data. */
static void CopyIn(Q9RingBuf *rb, Q9_u32 off, const Q9_u8 *src, Q9_u32 len)
{
    Q9_u32 i;
    for (i = 0; i < len; i++) {
        Q9_u32 at = off + i;
        if (at >= rb->size)
            at -= rb->size;
        rb->data[at] = src[i];
    }
}

int Q9RingBufWrite(Q9RingBuf *rb, const Q9_u8 *rec, Q9_u32 recLen)
{
    Q9_u32 free;
    Q9_u32 lostThisOp = 0;
    Q9_u32 markerLen = 0;
    Q9_u8 markerBuf[Q9RINGBUF_LOSTMARKER_MAX];

    if (recLen == 0 || recLen > rb->size)
        return 0;

    free = rb->size - rb->used;

    if (recLen > free) {
        if (rb->mode == Q9RB_MODE_HALT) {
            rb->lostCount++;
            return 0;
        }
        /* OVERWRITE (Semantik von Andreas praezisiert, 06.10.2026): am
         * Lesekopf alte Saetze opfern, bis genug Platz frei ist -- UND
         * genug fuer den Verlust-Marker-Satz, falls ein Formatierer
         * gesetzt ist. Der Marker hat fuer ein gegebenes Format IMMER
         * dieselbe Laenge unabhaengig vom Zaehlerwert (Vertrag in
         * q9ringbuf.h), deshalb genuegt es, sie EINMAL vorab zu messen
         * (Aufruf mit lostCount=0 nur zur Laengenermittlung -- der
         * tatsaechliche, richtige Zaehlerwert wird erst GANZ am Ende
         * hineingeschrieben, s.u.). Jeder geopferte Satz zaehlt einzeln
         * als verloren -- ein teilweise ueberschriebener Satz waere
         * ohnehin nie wieder vollstaendig lesbar. */
        if (rb->lostFmt != 0) {
            markerLen = rb->lostFmt(markerBuf, 0);
            if (markerLen > rb->size)
                markerLen = 0; /* Puffer zu klein fuer JEDEN Marker -- Zaehler-only-Rueckfall */
        }

        while (recLen + markerLen > free) {
            Q9_u32 oldLen = RecLenAt(rb, rb->readOff);
            if (oldLen == 0 || oldLen > rb->used) {
                /* Kaputter/leerer Zustand (sollte nie vorkommen) --
                 * Puffer hart leeren statt endlos zu drehen. */
                rb->readOff = rb->writeOff;
                rb->used = 0;
                free = rb->size;
                break;
            }
            rb->readOff += oldLen;
            if (rb->readOff >= rb->size)
                rb->readOff -= rb->size;
            rb->used -= oldLen;
            free += oldLen;
            rb->lostCount++;
            lostThisOp++;
        }

        if (lostThisOp > 0 && rb->lostFmt != 0 && markerLen > 0) {
            /* WICHTIG: der Marker darf NICHT an der (neuen) Lesekopf-
             * Position selbst beginnen -- dort faengt bereits der
             * aelteste UEBERLEBENDE Satz an (falls einer ueberlebt
             * hat), ihn dort hineinzuschreiben wuerde genau DIESEN
             * Satz zerstoeren, den wir gerade NICHT opfern wollten.
             * Stattdessen belegt der Marker die LETZTEN "markerLen"
             * Byte der soeben insgesamt frei gewordenen Spanne --
             * d.h. er liegt UNMITTELBAR VOR der neuen Lesekopf-
             * Position, und genau dorthin wird der Lesekopf
             * zurueckgesetzt. Die Spanne ist dank der Schleifen-
             * bedingung oben ("recLen+markerLen > free") immer
             * mindestens recLen+markerLen Byte gross, sodass Marker
             * UND der nachfolgend neu geschriebene Satz (an writeOff,
             * s.u.) nie denselben Platz beanspruchen, unabhaengig
             * davon, wie die freie Spanne physisch im Ring liegt. */
            Q9_u32 finalLen = rb->lostFmt(markerBuf, lostThisOp);
            Q9_u32 markerPos = rb->readOff + rb->size - markerLen;
            if (markerPos >= rb->size)
                markerPos -= rb->size;

            CopyIn(rb, markerPos, markerBuf, finalLen);
            rb->readOff = markerPos;
            rb->used += finalLen;
            free -= finalLen;
            /* Der Marker selbst zaehlt NICHT nochmal als "geschrieben"
             * im Sinne eines Nutzsatzes -- er ist Buchfuehrung ueber
             * bereits gezaehlte Verluste, kein neuer Trace-Eintrag. */
        }
    }

    CopyIn(rb, rb->writeOff, rec, recLen);
    rb->writeOff += recLen;
    if (rb->writeOff >= rb->size)
        rb->writeOff -= rb->size;
    rb->used += recLen;
    rb->writtenCount++;
    return 1;
}

Q9_u32 Q9RingBufRead(Q9RingBuf *rb, Q9_u8 *dst, Q9_u32 maxBytes,
                      Q9_u32 *lostOut)
{
    Q9_u32 copied = 0;
    Q9_u32 off = rb->readOff;
    Q9_u32 remaining = rb->used;

    if (lostOut != 0)
        *lostOut = rb->lostCount;

    while (remaining > 0) {
        Q9_u32 recLen = RecLenAt(rb, off);
        Q9_u32 i;

        if (recLen == 0 || recLen > remaining)
            break;              /* kaputter Zustand -- lieber abbrechen als Muell liefern */
        if (copied + recLen > maxBytes)
            break;              /* naechster Satz passt nicht mehr GANZ hinein */

        for (i = 0; i < recLen; i++) {
            Q9_u32 src = off + i;
            if (src >= rb->size)
                src -= rb->size;
            dst[copied + i] = rb->data[src];
        }
        copied += recLen;
        off += recLen;
        if (off >= rb->size)
            off -= rb->size;
        remaining -= recLen;
    }

    rb->readOff = off;
    rb->used = remaining;
    return copied;
}

void Q9RingBufClear(Q9RingBuf *rb)
{
    rb->writeOff = 0;
    rb->readOff = 0;
    rb->used = 0;
    /* writtenCount/lostCount bewusst NICHT zuruecksetzen, s. Header. */
}
