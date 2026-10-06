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
    rb->mode = mode;
    rb->id = id;

    for (i = 0; i < sizeof(rb->name) - 1 && name[i] != '\0'; i++)
        rb->name[i] = name[i];
    rb->name[i] = '\0';
}

/* Laenge des Satzes, dessen ERSTES Byte an "off" liegt (zirkulaer). */
static Q9_u32 RecLenAt(const Q9RingBuf *rb, Q9_u32 off)
{
    Q9_u32 lenOff = off + Q9RINGBUF_LENOFF;
    if (lenOff >= rb->size)
        lenOff -= rb->size;
    return (Q9_u32)rb->data[lenOff];
}

int Q9RingBufWrite(Q9RingBuf *rb, const Q9_u8 *rec, Q9_u32 recLen)
{
    Q9_u32 free;
    Q9_u32 i;

    if (recLen == 0 || recLen > rb->size)
        return 0;

    free = rb->size - rb->used;

    if (recLen > free) {
        if (rb->mode == Q9RB_MODE_HALT) {
            rb->lostCount++;
            return 0;
        }
        /* OVERWRITE: am Lesekopf ganze (alte) Saetze opfern, bis genug
         * Platz frei ist. Jeder geopferte Satz zaehlt einzeln als
         * verloren -- ein teilweise ueberschriebener Satz waere ohnehin
         * nie wieder vollstaendig lesbar. */
        while (recLen > free) {
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
        }
    }

    for (i = 0; i < recLen; i++) {
        Q9_u32 off = rb->writeOff + i;
        if (off >= rb->size)
            off -= rb->size;
        rb->data[off] = rec[i];
    }
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
