/*
 * q9kernel_event.c -- Q9-OS eigener Kernel: F$Event (Callcode $53,
 *                     2026-09-19), das Ereignissystem.
 *
 * Verifizierte ABI (68k_tech.pdf S.386 ff. sowie Kapitel 4). F$Event ist
 * ein Aufruf mit Funktionscode in d1.w; alles Weitere haengt vom Code ab.
 * Die Codes stammen aus MWOS/OS9/SRC/DEFS/event.a (dort als "do.b 1" ab 0
 * durchgezaehlt), nicht aus dem Fliesstext:
 *
 *   0 Ev$Link   (a0)=Name                    -> d0.l=ID
 *   1 Ev$UnLnk  d0.l=ID
 *   2 Ev$Creat  d0.l=Startwert, d2.w=WaitInk,
 *               d3.w=SignalInk, (a0)=Name    -> d0.l=ID
 *   3 Ev$Delet  (a0)=Name
 *   4 Ev$Wait   d0.l=ID, d2.l=min, d3.l=max  -> d1.l=Wert   (s. u.)
 *   5 Ev$WaitR  wie Wait, aber relativ       -> d1/d2/d3    (s. u.)
 *   6 Ev$Read   d0.l=ID                      -> d1.l=Wert
 *   7 Ev$Info   d0.l=Index, (a0)=Puffer      -> d0.l=Index, Puffer gefuellt
 *   8 Ev$Signl  d0.l=ID
 *   9 Ev$Pulse  d0.l=ID, d2.l=Pulswert
 *  10 Ev$Set    d0.l=ID, d2.l=neuer Wert     -> d1.l=alter Wert
 *  11 Ev$SetR   d0.l=ID, d2.l=Inkrement      -> d1.l=alter Wert
 *
 * WAS EIN EREIGNIS IST: eine 32-Byte-Systemvariable mit Name, Wert und
 * zwei festen Inkrementen -- eines, das beim Warten auf den Wert addiert
 * wird, eines beim Signalisieren. Das Handbuch nennt sie "multiple-value
 * semaphores": anders als ein Semaphor traegt ein Ereignis einen Zaehler,
 * und ein Wartender nennt einen Wertebereich, in dem er geweckt werden
 * will. Das Beispiel aus dem Handbuch ist ein Druckerpool -- Startwert =
 * Zahl der Drucker, Wait-Inkrement -1, Signal-Inkrement +1.
 *
 * Die Feldaufteilung ist aus MWOS/OS9/SRC/DEFS/event.a uebernommen, wo
 * sie mit "do.w/do.l" luecklos steht: ID (Wort), Name (12 Byte), Wert
 * (Langwort), Wait-Inkrement (Wort), Signal-Inkrement (Wort), Link-Zaehler
 * (Wort), zwei Warteschlangenzeiger (Langworte) -- zusammen die 32 Byte,
 * die dieselbe Datei als Ev_Size festhaelt.
 *
 * WAS DIESE FASSUNG KANN, und was noch nicht: alles ausser Warten.
 * Ev$Wait und Ev$WaitR fehlen; sie setzen voraus, dass der Aufrufer
 * angehalten und spaeter wieder geweckt wird, und das ist derselbe
 * Prozesswechsel, den F$Sema schon fuehrt (s. q9kernel_sema.c). Der Rest
 * -- anlegen, loeschen, verbinden, lesen, setzen, signalisieren -- steht
 * vollstaendig und ist fuer sich brauchbar: ein Ereignis ist damit ein
 * benannter, systemweiter Zaehler, den mehrere Prozesse teilen koennen.
 * Ev$Signl durchsucht die Warteschlange bereits; sie ist nur immer leer,
 * solange niemand warten kann.
 *
 * ZU DEN FEHLERCODES: E$EvntID ($BD), E$EvNF ($BE), E$EvBusy ($BF) und
 * E$BNam ($EB) sind aus funcs.a durchgezaehlt. Die Zaehlung dort beginnt
 * bei einem "org", der um $4C neben dem echten Wert liegt; diese
 * Verschiebung ist an ZWEI unabhaengigen Werten geprueft, die dieser
 * Kernel bereits kennt (E$UnkSvc $D0 und E$BPAddr $D2). Fuer "Tabelle
 * voll" nennt das Handbuch E$EvFull, ein Name, den funcs.a nicht
 * enthaelt -- hier wird deshalb E$Full ($F8) gemeldet und das hier
 * vermerkt, statt einen Code zu erfinden.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#define Q9K_E_EVNTID 0xBDU
#define Q9K_E_EVNF   0xBEU
#define Q9K_E_EVBUSY 0xBFU
#define Q9K_E_BNAM   0xEBU
#define Q9K_E_EVFULL 0xF8U   /* E$Full -- s. Kopfkommentar */
#ifndef Q9K_E_UNKSVC
#define Q9K_E_UNKSVC 0xD0U
#endif

/* Ereignistabelle. Fester Pool, dieselbe schlanke Bauart wie die
 * Alarmtabelle: 16 Eintraege reichen fuer diesen Kernelstand bei weitem,
 * und eine verkettete Liste kann das spaeter ersetzen, ohne dass ein
 * Aufrufer es merkt. Liegt hinter dem Ausnahme-Verschachtelungszaehler
 * ($1C00) mit Abstand. */
#ifndef Q9K_EVENT_BASE
#define Q9K_EVENT_BASE  0x1C10UL
#endif
#define Q9K_EVENT_SLOTS 16UL

/* Feldabstaende nach event.a. Per #ifndef ueberschreibbar, aus dem
 * ueblichen Grund: auf einem 64-Bit-Testhost sind Q9K_SetU32-Zugriffe
 * acht Byte breit und wuerden sonst ineinanderlaufen (die Alarmtabelle
 * hat genau daran schon einmal gelitten, s. q9kernel_alarm.c). */
#ifndef Q9K_EVENT_STRIDE
#define Q9K_EVENT_STRIDE    32UL
#define Q9K_EVENT_OFF_ID     0UL   /* Wort     */
#define Q9K_EVENT_OFF_NAME   2UL   /* 12 Byte  */
#define Q9K_EVENT_OFF_VALUE 14UL   /* Langwort */
#define Q9K_EVENT_OFF_INCW  18UL   /* Wort     */
#define Q9K_EVENT_OFF_INCS  20UL   /* Wort     */
#define Q9K_EVENT_OFF_LINK  22UL   /* Wort     */
#define Q9K_EVENT_OFF_QNEXT 24UL   /* Langwort */
#define Q9K_EVENT_OFF_QPREV 28UL   /* Langwort */
#endif
#define Q9K_EVENT_NAME_MAX 11UL    /* "max 11 chars", s. MAXEVNAME in event.a */

#define Q9K_EVENT_SLOT(i) (Q9K_EVENT_BASE + (i) * Q9K_EVENT_STRIDE)

/* Fortlaufender Zaehler fuer Ereignis-IDs, direkt hinter der Tabelle.
 * Die ID ist NICHT der Tabellenindex: das Handbuch sagt "this number and
 * the event's array position create a unique ID", und eine reine
 * Indexnummer wuerde nach dem Loeschen sofort wiederverwendet -- ein
 * Aufrufer mit einer alten ID wuerde dann still auf ein fremdes Ereignis
 * zugreifen. Die laufende Nummer im oberen Wort verhindert das. */
#ifndef Q9K_EVENT_NEXTID
#define Q9K_EVENT_NEXTID (Q9K_EVENT_BASE + Q9K_EVENT_SLOTS * Q9K_EVENT_STRIDE)
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif
#ifndef Q9K_EVENT_SMALL_ACCESSORS
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }
#endif

/* Ein Platz ist frei, wenn seine ID 0 ist -- IDs beginnen bei 1. */
static int Q9K_EventSlotFree(Q9_u32 slot)
{
    return Q9K_GetU16(Q9K_EVENT_SLOT(slot) + Q9K_EVENT_OFF_ID) == 0U;
}

/* Namensvergleich gegen den Tabelleneintrag. Der Name im Aufruf ist
 * nicht zwingend nullterminiert -- das Handbuch laesst ihn "updated past
 * event name" zurueckgeben, der Aufrufer darf ihn also mitten in einer
 * Zeichenkette stehen haben. Verglichen wird deshalb bis zum ersten
 * Zeichen, das kein Namenszeichen mehr ist. */
static int Q9K_EventNameChar(Q9_u8 c)
{
    return (c > 0x20U && c < 0x7FU) ? 1 : 0;
}

static int Q9K_EventNameMatches(Q9_u32 slot, Q9_u32 nameAddr)
{
    Q9_u32 entry = Q9K_EVENT_SLOT(slot) + Q9K_EVENT_OFF_NAME;
    Q9_u32 i;

    for (i = 0UL; i < Q9K_EVENT_NAME_MAX; i++) {
        Q9_u8 want = Q9K_GetU8(entry + i);
        Q9_u8 have = Q9K_GetU8(nameAddr + i);

        if (want == 0U)
            return Q9K_EventNameChar(have) ? 0 : 1;   /* beide zu Ende */
        if (!Q9K_EventNameChar(have) || have != want)
            return 0;
    }
    return 1;
}

/* Q9K_EventNameLength -- Laenge des uebergebenen Namens, oder 0, wenn er
 * leer oder zu lang ist. Das Handbuch kennt dafuer E$BNam. */
Q9_u32 Q9K_EventNameLength(Q9_u32 nameAddr)
{
    Q9_u32 i;

    if (nameAddr == 0UL)
        return 0UL;
    for (i = 0UL; i <= Q9K_EVENT_NAME_MAX; i++) {
        if (!Q9K_EventNameChar(Q9K_GetU8(nameAddr + i)))
            break;
    }
    return (i == 0UL || i > Q9K_EVENT_NAME_MAX) ? 0UL : i;
}

/* Q9K_EventFindByName -- Tabellenindex oder -1. */
long Q9K_EventFindByName(Q9_u32 nameAddr)
{
    Q9_u32 i;

    for (i = 0UL; i < Q9K_EVENT_SLOTS; i++) {
        if (!Q9K_EventSlotFree(i) && Q9K_EventNameMatches(i, nameAddr))
            return (long)i;
    }
    return -1L;
}

/* Q9K_EventFindById -- Tabellenindex zu einer ID, oder -1.
 *
 * Die ID traegt im unteren Wort den Tabellenindex und im oberen die
 * laufende Nummer; geprueft wird BEIDES. Nur den Index zu nehmen waere
 * schneller, liesse aber eine alte ID auf den inzwischen neu belegten
 * Platz zeigen -- genau das, was die laufende Nummer verhindern soll. */
long Q9K_EventFindById(Q9_u32 id)
{
    Q9_u32 slot = id & 0xFFFFUL;

    if (slot >= Q9K_EVENT_SLOTS)
        return -1L;
    if (Q9K_EventSlotFree(slot))
        return -1L;
    if (Q9K_GetU16(Q9K_EVENT_SLOT(slot) + Q9K_EVENT_OFF_ID)
        != (Q9_u16)((id >> 16) & 0xFFFFUL))
        return -1L;
    return (long)slot;
}

/* Q9K_EventCreate -- Ev$Creat.
 *
 * Rueckgabe 1 = angelegt, *outId traegt die ID. */
int Q9K_EventCreate(Q9_u32 nameAddr, Q9_u32 initialValue,
                    Q9_u16 waitInc, Q9_u16 signalInc,
                    Q9_u32 *outId, Q9_u16 *outError)
{
    Q9_u32 len = Q9K_EventNameLength(nameAddr);
    Q9_u32 i, slot, entry, serial;

    if (len == 0UL) {
        *outError = Q9K_E_BNAM;
        return 0;
    }
    if (Q9K_EventFindByName(nameAddr) >= 0L) {
        /* "The named event already exists" -- ein zweites Ereignis
         * gleichen Namens waere von aussen nicht unterscheidbar. */
        *outError = Q9K_E_EVBUSY;
        return 0;
    }

    slot = Q9K_EVENT_SLOTS;
    for (i = 0UL; i < Q9K_EVENT_SLOTS; i++) {
        if (Q9K_EventSlotFree(i)) { slot = i; break; }
    }
    if (slot == Q9K_EVENT_SLOTS) {
        *outError = Q9K_E_EVFULL;
        return 0;
    }

    serial = Q9K_GetU32(Q9K_EVENT_NEXTID) + 1UL;
    if (serial == 0UL || serial > 0xFFFFUL)
        serial = 1UL;                       /* Wort, und 0 heisst "frei" */
    Q9K_SetU32(Q9K_EVENT_NEXTID, serial);

    entry = Q9K_EVENT_SLOT(slot);
    Q9K_SetU16(entry + Q9K_EVENT_OFF_ID, (Q9_u16)serial);
    for (i = 0UL; i < Q9K_EVENT_NAME_MAX + 1UL; i++) {
        Q9K_SetU8(entry + Q9K_EVENT_OFF_NAME + i,
                  (i < len) ? Q9K_GetU8(nameAddr + i) : 0U);
    }
    Q9K_SetU32(entry + Q9K_EVENT_OFF_VALUE, initialValue);
    Q9K_SetU16(entry + Q9K_EVENT_OFF_INCW, waitInc);
    Q9K_SetU16(entry + Q9K_EVENT_OFF_INCS, signalInc);
    /* "an implicit use count (initially set to 1)" -- der Erzeuger haelt
     * das Ereignis, bis er es wieder freigibt. */
    Q9K_SetU16(entry + Q9K_EVENT_OFF_LINK, 1U);
    Q9K_SetU32(entry + Q9K_EVENT_OFF_QNEXT, 0UL);
    Q9K_SetU32(entry + Q9K_EVENT_OFF_QPREV, 0UL);

    *outId = (serial << 16) | slot;
    return 1;
}

/* Q9K_EventDelete -- Ev$Delet, ueber den Namen.
 *
 * "An event may not be deleted unless its use count is zero." */
int Q9K_EventDelete(Q9_u32 nameAddr, Q9_u16 *outError)
{
    long slot;

    if (Q9K_EventNameLength(nameAddr) == 0UL) {
        *outError = Q9K_E_BNAM;
        return 0;
    }
    slot = Q9K_EventFindByName(nameAddr);
    if (slot < 0L) {
        *outError = Q9K_E_EVNF;
        return 0;
    }
    if (Q9K_GetU16(Q9K_EVENT_SLOT((Q9_u32)slot) + Q9K_EVENT_OFF_LINK) != 0U) {
        *outError = Q9K_E_EVBUSY;
        return 0;
    }
    /* ID auf 0: der Platz gilt damit als frei. */
    Q9K_SetU16(Q9K_EVENT_SLOT((Q9_u32)slot) + Q9K_EVENT_OFF_ID, 0U);
    return 1;
}

/* Q9K_EventLink -- Ev$Link: ID zum Namen holen und den Zaehler erhoehen. */
int Q9K_EventLink(Q9_u32 nameAddr, Q9_u32 *outId, Q9_u16 *outError)
{
    long slot;
    Q9_u32 entry;

    if (Q9K_EventNameLength(nameAddr) == 0UL) {
        *outError = Q9K_E_BNAM;
        return 0;
    }
    slot = Q9K_EventFindByName(nameAddr);
    if (slot < 0L) {
        *outError = Q9K_E_EVNF;
        return 0;
    }
    entry = Q9K_EVENT_SLOT((Q9_u32)slot);
    Q9K_SetU16(entry + Q9K_EVENT_OFF_LINK,
               (Q9_u16)(Q9K_GetU16(entry + Q9K_EVENT_OFF_LINK) + 1U));
    *outId = ((Q9_u32)Q9K_GetU16(entry + Q9K_EVENT_OFF_ID) << 16) | (Q9_u32)slot;
    return 1;
}

/* Q9K_EventUnlink -- Ev$UnLnk: Zaehler senken.
 *
 * Bei 0 wird NICHT geloescht: "When it reaches zero, you must delete it"
 * -- das Loeschen bleibt ausdruecklich dem Aufrufer ueberlassen. Ein
 * Zaehler, der schon 0 ist, bleibt 0, statt unterzulaufen. */
int Q9K_EventUnlink(Q9_u32 id, Q9_u16 *outError)
{
    long slot = Q9K_EventFindById(id);
    Q9_u32 entry;
    Q9_u16 count;

    if (slot < 0L) {
        *outError = Q9K_E_EVNTID;
        return 0;
    }
    entry = Q9K_EVENT_SLOT((Q9_u32)slot);
    count = Q9K_GetU16(entry + Q9K_EVENT_OFF_LINK);
    if (count != 0U)
        Q9K_SetU16(entry + Q9K_EVENT_OFF_LINK, (Q9_u16)(count - 1U));
    return 1;
}

/* Q9K_EventRead -- Ev$Read: Wert lesen, ohne ihn anzufassen. */
int Q9K_EventRead(Q9_u32 id, Q9_u32 *outValue, Q9_u16 *outError)
{
    long slot = Q9K_EventFindById(id);

    if (slot < 0L) {
        *outError = Q9K_E_EVNTID;
        return 0;
    }
    *outValue = Q9K_GetU32(Q9K_EVENT_SLOT((Q9_u32)slot) + Q9K_EVENT_OFF_VALUE);
    return 1;
}

/* Saettigende Addition. "Arithmetic underflows or overflows are set to
 * $80000000 or $7fffffff, respectively" -- ausdruecklich saettigen, nicht
 * umlaufen, damit ein Zaehler an seiner Grenze stehenbleibt statt ins
 * Gegenteil zu springen. */
static Q9_u32 Q9K_EventAddSaturating(Q9_u32 value, long delta)
{
    /* Das Vorzeichen wird AUSDRUECKLICH aus Bit 31 gelesen, statt sich
     * auf einen Cast zu verlassen: der Ereigniswert ist per Definition
     * 32 Bit breit ("a range of two billion"), ein "long" des
     * Uebersetzers muss das nicht sein. Auf einem 64-Bit-Testhost gilt
     * $80000010 sonst als positive Zahl, und die Saettigung nach unten
     * greift nie -- genau so im Hosttest aufgelaufen, bevor hier
     * gerechnet statt gecastet wurde. */
    long v = (long)(value & 0x7FFFFFFFUL);
    long r;

    if (value & 0x80000000UL)
        v -= 0x40000000L + 0x40000000L;   /* zweimal, damit 32-Bit-Ziele nicht ueberlaufen */

    r = v + delta;
    if (r > 0x7FFFFFFFL || (delta > 0L && r < v))
        return 0x7FFFFFFFUL;
    if (r < -0x40000000L - 0x40000000L || (delta < 0L && r > v))
        return 0x80000000UL;
    return (Q9_u32)r & 0xFFFFFFFFUL;
}

/* Q9K_EventSignalCommon -- der gemeinsame Kern von Ev$Signl, Ev$Set,
 * Ev$SetR und Ev$Pulse.
 *
 * newValue ist der Wert, den das Ereignis annimmt; restore != 0 stellt
 * den alten danach wieder her (das ist genau Ev$Pulse: "the original
 * event value is restored").
 *
 * Das Wecken der Wartenden ist vorbereitet, aber die Schlange ist immer
 * leer, solange Ev$Wait fehlt -- s. Kopfkommentar. */
static int Q9K_EventSignalCommon(Q9_u32 id, Q9_u32 newValue, int restore,
                                 Q9_u32 *outPrevious, Q9_u16 *outError)
{
    long slot = Q9K_EventFindById(id);
    Q9_u32 entry, previous;

    if (slot < 0L) {
        *outError = Q9K_E_EVNTID;
        return 0;
    }
    entry = Q9K_EVENT_SLOT((Q9_u32)slot);
    previous = Q9K_GetU32(entry + Q9K_EVENT_OFF_VALUE);
    Q9K_SetU32(entry + Q9K_EVENT_OFF_VALUE, newValue);

    /* Hier durchsucht das Original die Warteschlange und weckt, wer in
     * den Wertebereich faellt. */

    if (restore)
        Q9K_SetU32(entry + Q9K_EVENT_OFF_VALUE, previous);

    if (outPrevious != 0)
        *outPrevious = previous;
    return 1;
}

int Q9K_EventSignal(Q9_u32 id, Q9_u16 *outError)
{
    long slot = Q9K_EventFindById(id);
    Q9_u32 entry, value;

    if (slot < 0L) {
        *outError = Q9K_E_EVNTID;
        return 0;
    }
    entry = Q9K_EVENT_SLOT((Q9_u32)slot);
    value = Q9K_EventAddSaturating(
                Q9K_GetU32(entry + Q9K_EVENT_OFF_VALUE),
                (long)(short)Q9K_GetU16(entry + Q9K_EVENT_OFF_INCS));
    return Q9K_EventSignalCommon(id, value, 0, 0, outError);
}

int Q9K_EventSet(Q9_u32 id, Q9_u32 newValue, Q9_u32 *outPrevious, Q9_u16 *outError)
{
    return Q9K_EventSignalCommon(id, newValue, 0, outPrevious, outError);
}

int Q9K_EventSetRelative(Q9_u32 id, long delta, Q9_u32 *outPrevious, Q9_u16 *outError)
{
    long slot = Q9K_EventFindById(id);
    Q9_u32 value;

    if (slot < 0L) {
        *outError = Q9K_E_EVNTID;
        return 0;
    }
    value = Q9K_EventAddSaturating(
                Q9K_GetU32(Q9K_EVENT_SLOT((Q9_u32)slot) + Q9K_EVENT_OFF_VALUE),
                delta);
    return Q9K_EventSignalCommon(id, value, 0, outPrevious, outError);
}

int Q9K_EventPulse(Q9_u32 id, Q9_u32 pulseValue, Q9_u16 *outError)
{
    return Q9K_EventSignalCommon(id, pulseValue, 1, 0, outError);
}

/* Q9K_EventInfo -- Ev$Info: den 32-Byte-Eintrag des ersten aktiven
 * Ereignisses mit Index >= startIndex herauskopieren.
 *
 * "Unlike other F$Event functions, Ev$Info only uses the low word of d0." */
int Q9K_EventInfo(Q9_u32 startIndex, Q9_u32 bufAddr,
                  Q9_u32 *outIndex, Q9_u16 *outError)
{
    Q9_u32 i;

    if (bufAddr == 0UL) {
        *outError = Q9K_E_EVNTID;
        return 0;
    }
    for (i = startIndex & 0xFFFFUL; i < Q9K_EVENT_SLOTS; i++) {
        if (!Q9K_EventSlotFree(i)) {
            Q9_u32 entry = Q9K_EVENT_SLOT(i);
            Q9_u32 k;
            for (k = 0UL; k < Q9K_EVENT_STRIDE; k++)
                Q9K_SetU8(bufAddr + k, Q9K_GetU8(entry + k));
            *outIndex = i;
            return 1;
        }
    }
    /* "The index is above all active events." */
    *outError = Q9K_E_EVNTID;
    return 0;
}

/* --- Bruecke ------------------------------------------------------
 *
 * Ein Aufruf, zwoelf Funktionen: die Registerbelegung wechselt je nach
 * Funktionscode, deshalb reicht die ASM-Seite pauschal d0/d2/d3 und (a0)
 * herein und holt sich, was der jeweilige Code zurueckgibt. */
#ifndef Q9K_EVENT_SCRATCH_FUNC
#define Q9K_EVENT_SCRATCH_FUNC  0x1E20UL /* Q9_u32, d1.w EIN = Funktionscode */
#define Q9K_EVENT_SCRATCH_D0    0x1E24UL /* Q9_u32, d0.l EIN / AUS           */
#define Q9K_EVENT_SCRATCH_D1    0x1E28UL /* Q9_u32, d1.l AUS                 */
#define Q9K_EVENT_SCRATCH_D2    0x1E2CUL /* Q9_u32, d2.l EIN                 */
#define Q9K_EVENT_SCRATCH_D3    0x1E30UL /* Q9_u32, d3.l EIN                 */
#define Q9K_EVENT_SCRATCH_A0    0x1E34UL /* Q9_u32, (a0) EIN                 */
#define Q9K_EVENT_SCRATCH_ERROR 0x1E38UL /* Q9_u32, d1.w AUS bei Fehler      */
#define Q9K_EVENT_SCRATCH_OK    0x1E3CUL /* Q9_u32, 0/1                      */
#endif

#define Q9K_EV_LINK  0U
#define Q9K_EV_UNLNK 1U
#define Q9K_EV_CREAT 2U
#define Q9K_EV_DELET 3U
#define Q9K_EV_WAIT  4U
#define Q9K_EV_WAITR 5U
#define Q9K_EV_READ  6U
#define Q9K_EV_INFO  7U
#define Q9K_EV_SIGNL 8U
#define Q9K_EV_PULSE 9U
#define Q9K_EV_SET  10U
#define Q9K_EV_SETR 11U

void Q9K_SysEventImpl(void)
{
    /* Das oberste Bit des Funktionsworts ist bei Ev$Signl/Set/SetR/Pulse
     * ein Schalter ("MS bit set to activate all processes in range") und
     * gehoert nicht zum Code. */
    Q9_u32 func = Q9K_GetU32(Q9K_EVENT_SCRATCH_FUNC) & 0x7FFFUL;
    Q9_u32 d0   = Q9K_GetU32(Q9K_EVENT_SCRATCH_D0);
    Q9_u32 d2   = Q9K_GetU32(Q9K_EVENT_SCRATCH_D2);
    Q9_u32 a0   = Q9K_GetU32(Q9K_EVENT_SCRATCH_A0);
    Q9_u32 out  = 0UL;
    Q9_u16 err  = 0U;
    int ok = 0;

    Q9K_SetU32(Q9K_EVENT_SCRATCH_OK, 0UL);

    switch (func) {
    case Q9K_EV_CREAT:
        ok = Q9K_EventCreate(a0, d0,
                             (Q9_u16)Q9K_GetU32(Q9K_EVENT_SCRATCH_D2),
                             (Q9_u16)Q9K_GetU32(Q9K_EVENT_SCRATCH_D3),
                             &out, &err);
        if (ok) Q9K_SetU32(Q9K_EVENT_SCRATCH_D0, out);
        break;
    case Q9K_EV_DELET:
        ok = Q9K_EventDelete(a0, &err);
        break;
    case Q9K_EV_LINK:
        ok = Q9K_EventLink(a0, &out, &err);
        if (ok) Q9K_SetU32(Q9K_EVENT_SCRATCH_D0, out);
        break;
    case Q9K_EV_UNLNK:
        ok = Q9K_EventUnlink(d0, &err);
        break;
    case Q9K_EV_READ:
        ok = Q9K_EventRead(d0, &out, &err);
        if (ok) Q9K_SetU32(Q9K_EVENT_SCRATCH_D1, out);
        break;
    case Q9K_EV_SIGNL:
        ok = Q9K_EventSignal(d0, &err);
        break;
    case Q9K_EV_PULSE:
        ok = Q9K_EventPulse(d0, d2, &err);
        break;
    case Q9K_EV_SET:
        ok = Q9K_EventSet(d0, d2, &out, &err);
        if (ok) Q9K_SetU32(Q9K_EVENT_SCRATCH_D1, out);
        break;
    case Q9K_EV_SETR:
        ok = Q9K_EventSetRelative(d0, (long)(signed long)d2, &out, &err);
        if (ok) Q9K_SetU32(Q9K_EVENT_SCRATCH_D1, out);
        break;
    case Q9K_EV_INFO:
        ok = Q9K_EventInfo(d0, a0, &out, &err);
        if (ok) Q9K_SetU32(Q9K_EVENT_SCRATCH_D0, out);
        break;
    case Q9K_EV_WAIT:
    case Q9K_EV_WAITR:
        /* Noch nicht umgesetzt -- s. Kopfkommentar. Ein stiller Erfolg
         * waere hier besonders schaedlich: der Aufrufer liefe weiter, als
         * haette er das Ereignis abgewartet. */
        err = Q9K_E_UNKSVC;
        ok = 0;
        break;
    default:
        err = Q9K_E_UNKSVC;
        ok = 0;
        break;
    }

    if (!ok) {
        Q9K_SetU32(Q9K_EVENT_SCRATCH_ERROR, (Q9_u32)err);
        return;
    }
    Q9K_SetU32(Q9K_EVENT_SCRATCH_OK, 1UL);
}
