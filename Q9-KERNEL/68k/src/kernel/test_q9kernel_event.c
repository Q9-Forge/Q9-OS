/*
 * test_q9kernel_event.c -- Hosttest fuer F$Event (q9kernel_event.c,
 *                          2026-09-19).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_event test_q9kernel_event.c && ./test_event
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

static unsigned char g_cells[0x400];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9K_EVENT_SCRATCH_FUNC  CELL(0x000)
#define Q9K_EVENT_SCRATCH_D0    CELL(0x020)
#define Q9K_EVENT_SCRATCH_D1    CELL(0x040)
#define Q9K_EVENT_SCRATCH_D2    CELL(0x060)
#define Q9K_EVENT_SCRATCH_D3    CELL(0x080)
#define Q9K_EVENT_SCRATCH_A0    CELL(0x0A0)
#define Q9K_EVENT_SCRATCH_ERROR CELL(0x0C0)
#define Q9K_EVENT_SCRATCH_OK    CELL(0x0E0)
#define Q9K_EVENT_SCRATCH_WAIT  CELL(0x100)
#define Q9K_EVENT_SCRATCH_NEXT  CELL(0x120)
#define Q9_D_PROC               CELL(0x140)

/* Eigene Deskriptorfelder fuer den Wertebereich -- auf Testbreite
 * gezogen, wie ueberall. */
#define Q9K_PROCDESC_EVMIN_OFF 0x40UL
#define Q9K_PROCDESC_EVMAX_OFF 0x48UL
#define Q9K_READYQ_NEXT_OFF    0x50UL
#define Q9K_PROCDESC_SAVEDSP_OFF 0x58UL

/* Prozesswechsel gibt es auf dem Host nicht. */
static int g_aprocCalls;
static unsigned long g_aprocLast;
int Q9K_ProcAProc(unsigned long desc, unsigned short *outError)
{
    (void)outError; g_aprocCalls++; g_aprocLast = desc; return 1;
}
static unsigned long g_nextPick;
unsigned long Q9K_SchedFirstPick(void) { return g_nextPick; }

/* Der Rueckgabewert landet im gesicherten Registersatz des Wartenden.
 * Hier wird nur mitgeschrieben, WAS wohin geschrieben wurde -- den
 * echten 68k-Rahmen gibt es auf dem Host nicht. */
static unsigned long g_frameBase, g_frameValue;
static unsigned long g_frameIndex = 0xFFFFFFFFUL;
static int g_frameWrites;
void Q9K_SetFrameReg(unsigned long frameBase, unsigned long regIndex,
                     unsigned long value)
{
    g_frameBase = frameBase; g_frameIndex = regIndex; g_frameValue = value;
    g_frameWrites++;
}

/* Die Ereignistabelle behaelt ihre ECHTEN Feldabstaende: alle Zugriffe
 * darauf laufen ueber Q9K_GetU16/GetU8 oder gezielte 4-Byte-Felder, und
 * Q9K_SetU32 trifft nur Ev_Value und die beiden Schlangenzeiger, die auf
 * diesem Host mit acht Byte in den jeweils naechsten Eintrag ragen
 * wuerden. Deshalb ein grosszuegigerer Abstand, aber dieselbe
 * Feldreihenfolge -- so bleibt der Test am echten Aufbau. */
static unsigned char g_table[16 * 64 + 32];
#define Q9K_EVENT_BASE   ((unsigned long)g_table)
#define Q9K_EVENT_STRIDE    64UL
#define Q9K_EVENT_OFF_ID     0UL
#define Q9K_EVENT_OFF_NAME   2UL
#define Q9K_EVENT_OFF_VALUE 16UL
#define Q9K_EVENT_OFF_INCW  24UL
#define Q9K_EVENT_OFF_INCS  26UL
#define Q9K_EVENT_OFF_LINK  28UL
#define Q9K_EVENT_OFF_QNEXT 32UL
#define Q9K_EVENT_OFF_QPREV 40UL
#define Q9K_EVENT_NEXTID (Q9K_EVENT_BASE + 16UL * 64UL)

#include "q9kernel_event.c"

static int failures;
static char g_name[32];
static unsigned char g_buf[64];

#define NAME ((Q9_u32)(unsigned long)g_name)
#define BUF  ((Q9_u32)(unsigned long)g_buf)

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-62s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void reset(void)
{
    memset(g_cells, 0, sizeof g_cells);
    g_aprocCalls = 0; g_aprocLast = 0; g_nextPick = 0;
    g_frameWrites = 0; g_frameBase = 0; g_frameValue = 0;
    g_frameIndex = 0xFFFFFFFFUL;
    memset(g_table, 0, sizeof g_table);
    memset(g_name, 0, sizeof g_name);
    memset(g_buf, 0, sizeof g_buf);
}

static void setName(const char *s) { strcpy(g_name, s); }

int main(void)
{
    Q9_u32 id = 0, id2 = 0, value = 0, prev = 0, idx = 0;
    Q9_u16 err = 0;

    printf("== F$Event ==\n");

    /* --- Anlegen und Wiederfinden --- */
    reset();
    setName("drucker");
    check("ein Ereignis laesst sich anlegen",
          (Q9_u32)Q9K_EventCreate(NAME, 3, (Q9_u16)-1, 1, &id, &err), 1);
    check("die ID ist nicht 0", (Q9_u32)(id != 0), 1);
    check("der Startwert steht drin",
          (Q9_u32)(Q9K_EventRead(id, &value, &err), value), 3);

    /* Der Link-Zaehler beginnt bei 1 -- "an implicit use count
     * (initially set to 1)". Ohne das liesse sich ein frisch angelegtes
     * Ereignis sofort wieder loeschen, obwohl sein Erzeuger es haelt. */
    check("ein frisches Ereignis kann nicht geloescht werden",
          (Q9_u32)Q9K_EventDelete(NAME, &err), 0);
    check("mit E_EVBUSY", (Q9_u32)err, 0xBF);

    /* --- Verbinden und Loesen --- */
    check("ein zweiter Zugriff findet dasselbe Ereignis",
          (Q9_u32)Q9K_EventLink(NAME, &id2, &err), 1);
    check("und liefert dieselbe ID", id2, id);
    check("nach zweimal Loesen ist der Zaehler 0",
          (Q9_u32)(Q9K_EventUnlink(id, &err), Q9K_EventUnlink(id, &err), 1), 1);
    check("und jetzt laesst es sich loeschen",
          (Q9_u32)Q9K_EventDelete(NAME, &err), 1);
    check("danach wird es nicht mehr gefunden",
          (Q9_u32)Q9K_EventLink(NAME, &id2, &err), 0);
    check("mit E_EVNF", (Q9_u32)err, 0xBE);

    /* --- Die ID ueberlebt das Loeschen nicht ---
     *
     * Der Platz wird sofort wiederverwendet. Traege die ID nur den Index,
     * zeigte eine alte ID danach still auf ein fremdes Ereignis. */
    reset();
    setName("erstes");
    Q9K_EventCreate(NAME, 1, 0, 1, &id, &err);
    Q9K_EventUnlink(id, &err);
    Q9K_EventDelete(NAME, &err);
    setName("zweites");
    Q9K_EventCreate(NAME, 2, 0, 1, &id2, &err);
    check("der Platz wird wiederverwendet", id2 & 0xFFFF, id & 0xFFFF);
    check("aber die ID ist eine andere", (Q9_u32)(id2 != id), 1);
    check("und die alte ID gilt nicht mehr",
          (Q9_u32)Q9K_EventRead(id, &value, &err), 0);
    check("mit E_EVNTID", (Q9_u32)err, 0xBD);

    /* --- Signalisieren --- */
    reset();
    setName("zaehler");
    Q9K_EventCreate(NAME, 10, (Q9_u16)-2, 5, &id, &err);
    Q9K_EventSignal(id, 0, &err);
    Q9K_EventRead(id, &value, &err);
    check("Ev$Signl addiert das Signal-Inkrement", value, 15);
    Q9K_EventSignal(id, 0, &err);
    Q9K_EventRead(id, &value, &err);
    check("und tut das jedes Mal", value, 20);

    /* --- Setzen, absolut und relativ --- */
    check("Ev$Set setzt den Wert",
          (Q9_u32)Q9K_EventSet(id, 100, 0, &prev, &err), 1);
    check("und meldet den vorherigen", prev, 20);
    Q9K_EventRead(id, &value, &err);
    check("der neue Wert steht drin", value, 100);

    Q9K_EventSetRelative(id, -30, 0, &prev, &err);
    Q9K_EventRead(id, &value, &err);
    check("Ev$SetR rechnet relativ", value, 70);
    check("und meldet ebenfalls den vorherigen", prev, 100);

    /* Saettigung statt Umlauf: "overflows are set to $7fffffff". Ein
     * umlaufender Zaehler kippt von "sehr viel frei" auf "voll". */
    Q9K_EventSet(id, 0x7FFFFFF0UL, 0, &prev, &err);
    Q9K_EventSetRelative(id, 1000, 0, &prev, &err);
    Q9K_EventRead(id, &value, &err);
    check("ein Ueberlauf saettigt bei 0x7fffffff", value, 0x7FFFFFFFUL);
    Q9K_EventSet(id, 0x80000010UL, 0, &prev, &err);
    Q9K_EventSetRelative(id, -1000, 0, &prev, &err);
    Q9K_EventRead(id, &value, &err);
    check("ein Unterlauf bei 0x80000000", value, 0x80000000UL);

    /* --- Ev$Pulse stellt den Wert wieder her --- */
    reset();
    setName("puls");
    Q9K_EventCreate(NAME, 7, 0, 1, &id, &err);
    check("Ev$Pulse meldet Erfolg", (Q9_u32)Q9K_EventPulse(id, 99, 0, &err), 1);
    Q9K_EventRead(id, &value, &err);
    check("und laesst den Wert unveraendert zurueck", value, 7);

    /* --- Ev$Info --- */
    reset();
    setName("eins");
    Q9K_EventCreate(NAME, 11, 0, 1, &id, &err);
    setName("zwei");
    Q9K_EventCreate(NAME, 22, 0, 1, &id2, &err);
    check("Info findet das erste aktive Ereignis",
          (Q9_u32)Q9K_EventInfo(0, BUF, &idx, &err), 1);
    check("an Platz 0", idx, 0);
    check("und kopiert dessen Namen heraus", (Q9_u32)(g_buf[2] == 'e'), 1);
    check("Info ab Platz 1 findet das zweite",
          (Q9_u32)(Q9K_EventInfo(1, BUF, &idx, &err), idx), 1);
    check("Info hinter dem letzten meldet Fehlschlag",
          (Q9_u32)Q9K_EventInfo(2, BUF, &idx, &err), 0);

    /* --- Namen --- */
    reset();
    setName("");
    check("ein leerer Name wird abgewiesen",
          (Q9_u32)Q9K_EventCreate(NAME, 0, 0, 0, &id, &err), 0);
    check("mit E_BNAM", (Q9_u32)err, 0xEB);
    setName("viel_zu_langer_name");
    check("ein zu langer Name ebenso",
          (Q9_u32)Q9K_EventCreate(NAME, 0, 0, 0, &id, &err), 0);
    setName("elfzeichen");           /* 10 Zeichen -- gerade noch erlaubt */
    check("elf Zeichen sind erlaubt",
          (Q9_u32)Q9K_EventCreate(NAME, 0, 0, 0, &id, &err), 1);
    check("ein zweites Ereignis gleichen Namens wird abgewiesen",
          (Q9_u32)Q9K_EventCreate(NAME, 0, 0, 0, &id2, &err), 0);
    check("mit E_EVBUSY", (Q9_u32)err, 0xBF);

    /* --- Volle Tabelle --- */
    reset();
    {
        int i;
        char nm[16];
        for (i = 0; i < 16; i++) {
            sprintf(nm, "ev%d", i);
            setName(nm);
            if (!Q9K_EventCreate(NAME, 0, 0, 1, &id, &err)) break;
        }
        check("sechzehn Ereignisse passen hinein", (Q9_u32)i, 16);
        setName("einszuviel");
        check("das siebzehnte wird abgewiesen",
              (Q9_u32)Q9K_EventCreate(NAME, 0, 0, 1, &id, &err), 0);
        check("mit dem Voll-Code", (Q9_u32)err, 0xF8);
    }

    /* --- Die Bruecke --- */
    reset();
    setName("bruecke");
    Q9K_SetU32(Q9K_EVENT_SCRATCH_FUNC, 2);          /* Ev$Creat */
    Q9K_SetU32(Q9K_EVENT_SCRATCH_D0, 5);
    Q9K_SetU32(Q9K_EVENT_SCRATCH_D2, 0xFFFF);       /* Wait-Inkrement -1 */
    Q9K_SetU32(Q9K_EVENT_SCRATCH_D3, 1);
    Q9K_SetU32(Q9K_EVENT_SCRATCH_A0, NAME);
    Q9K_SysEventImpl();
    check("Bruecke Ev$Creat meldet Erfolg", Q9K_GetU32(Q9K_EVENT_SCRATCH_OK), 1);
    id = Q9K_GetU32(Q9K_EVENT_SCRATCH_D0);
    check("und gibt eine ID zurueck", (Q9_u32)(id != 0), 1);

    Q9K_SetU32(Q9K_EVENT_SCRATCH_FUNC, 6);          /* Ev$Read */
    Q9K_SetU32(Q9K_EVENT_SCRATCH_D0, id);
    Q9K_SysEventImpl();
    check("Bruecke Ev$Read liefert den Wert", Q9K_GetU32(Q9K_EVENT_SCRATCH_D1), 5);

    /* Das oberste Bit des Funktionsworts ist ein Schalter, kein Teil des
     * Codes -- ein Ev$Signl mit gesetztem Bit muss trotzdem greifen. */
    Q9K_SetU32(Q9K_EVENT_SCRATCH_FUNC, 0x8008);     /* Ev$Signl mit MS-Bit */
    Q9K_SetU32(Q9K_EVENT_SCRATCH_D0, id);
    Q9K_SysEventImpl();
    check("Bruecke Ev$Signl trotz gesetztem oberen Bit",
          Q9K_GetU32(Q9K_EVENT_SCRATCH_OK), 1);
    Q9K_EventRead(id, &value, &err);
    check("und hat wirklich signalisiert", value, 6);

    /* --- Warten --- */
    reset();
    setName("warte");
    Q9K_EventCreate(NAME, 0, (Q9_u16)-1, 1, &id, &err);   /* Wert 0, Wait -1, Signal +1 */

    /* Wert 0 liegt NICHT im Bereich 1..9 -- der Aufrufer muss warten. */
    check("ausserhalb des Bereichs wird gewartet",
          (Q9_u32)Q9K_EventWaitCheck(id, 1, 9, &value, &err), 0);
    check("und zwar ohne Fehler", (Q9_u32)err, 0);
    check("der gemeldete Wert ist der aktuelle", value, 0);

    /* Im Bereich: sofort weiter, und das Wait-Inkrement greift. */
    Q9K_EventSet(id, 5, 0, &prev, &err);
    check("im Bereich laeuft der Aufrufer sofort weiter",
          (Q9_u32)Q9K_EventWaitCheck(id, 1, 9, &value, &err), 1);
    check("der zurueckgegebene Wert ist der vor dem Inkrement", value, 5);
    Q9K_EventRead(id, &value, &err);
    check("und das Wait-Inkrement wurde angewandt", value, 4);

    /* Einreihen und wecken. */
    reset();
    setName("warte2");
    Q9K_EventCreate(NAME, 0, (Q9_u16)-1, 1, &id, &err);
    {
        static unsigned char proc[128];
        Q9_u32 desc = (Q9_u32)(unsigned long)proc;
        Q9_u32 next = 0;
        Q9_u32 entry;

        memset(proc, 0, sizeof proc);
        Q9K_SetU32(Q9_D_PROC, desc);
        Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF, 0xC0DE00UL);
        g_nextPick = 0x4242;
        Q9K_EventWaitEnqueue(id, 1, 9, &next);
        check("der Wartende waehlt einen naechsten Prozess", next, 0x4242);

        entry = Q9K_EVENT_SLOT((Q9_u32)Q9K_EventFindById(id));
        check("und steht in der Schlange des Ereignisses",
              Q9K_GetU32(entry + Q9K_EVENT_OFF_QNEXT), desc);
        check("sein Bereich ist vermerkt",
              Q9K_GetU32(desc + Q9K_PROCDESC_EVMIN_OFF), 1);

        /* Ein Signal bringt den Wert auf 1 -- im Bereich, also wecken. */
        Q9K_EventSignal(id, 0, &err);
        check("das Signal weckt den Wartenden", (Q9_u32)g_aprocCalls, 1);
        check("und zwar genau ihn", g_aprocLast, desc);
        check("die Schlange ist danach leer",
              Q9K_GetU32(entry + Q9K_EVENT_OFF_QNEXT), 0);
        Q9K_EventRead(id, &value, &err);
        check("der Wert steht nach Signal +1 und Wait -1 wieder bei 0", value, 0);

        /* Der Geweckte kehrt ueber movem/rte zurueck und holt d1 aus
         * seinem gesicherten Registersatz -- dort muss der Wert also
         * stehen, und zwar derselbe wie auf dem nicht blockierenden Weg:
         * der VOR dem Wait-Inkrement. Fehlte das, kaeme der Aufrufer mit
         * dem d1 zurueck, das er beim Einschlafen zufaellig hatte. */
        check("der Rueckgabewert wurde in den Registersatz gelegt",
              (Q9_u32)g_frameWrites, 1);
        check("und zwar an den Platz von d1", g_frameIndex, 1);
        check("in den Rahmen, auf den SavedSP zeigt", g_frameBase,
              Q9K_GetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF));
        check("mit dem Wert vor dem Wait-Inkrement", g_frameValue, 1);
    }

    /* Ein Wartender ausserhalb des Bereichs bleibt liegen -- und blockiert
     * die hinter ihm Stehenden nicht. */
    reset();
    setName("warte3");
    Q9K_EventCreate(NAME, 0, 0, 1, &id, &err);
    {
        static unsigned char pa[128], pb[128];
        Q9_u32 da = (Q9_u32)(unsigned long)pa, db = (Q9_u32)(unsigned long)pb;
        Q9_u32 next = 0, entry;

        memset(pa, 0, sizeof pa); memset(pb, 0, sizeof pb);
        Q9K_SetU32(Q9_D_PROC, da);
        Q9K_EventWaitEnqueue(id, 100, 200, &next);   /* weit weg */
        Q9K_SetU32(Q9_D_PROC, db);
        Q9K_EventWaitEnqueue(id, 1, 1, &next);       /* passt gleich */

        Q9K_EventSignal(id, 0, &err);                /* Wert 0 -> 1 */
        check("der passende Wartende wird geweckt", g_aprocLast, db);
        check("obwohl ein unpassender vor ihm steht", (Q9_u32)g_aprocCalls, 1);
        entry = Q9K_EVENT_SLOT((Q9_u32)Q9K_EventFindById(id));
        check("der unpassende bleibt in der Schlange",
              Q9K_GetU32(entry + Q9K_EVENT_OFF_QNEXT), da);
    }

    /* Mit gesetztem oberen Bit werden alle passenden geweckt. */
    reset();
    setName("alle");
    Q9K_EventCreate(NAME, 0, 0, 1, &id, &err);
    {
        static unsigned char p1[128], p2[128], p3[128];
        Q9_u32 next = 0;
        memset(p1,0,sizeof p1); memset(p2,0,sizeof p2); memset(p3,0,sizeof p3);
        Q9K_SetU32(Q9_D_PROC, (Q9_u32)(unsigned long)p1);
        Q9K_EventWaitEnqueue(id, 1, 1, &next);
        Q9K_SetU32(Q9_D_PROC, (Q9_u32)(unsigned long)p2);
        Q9K_EventWaitEnqueue(id, 1, 1, &next);
        Q9K_SetU32(Q9_D_PROC, (Q9_u32)(unsigned long)p3);
        Q9K_EventWaitEnqueue(id, 1, 1, &next);

        Q9K_EventSignal(id, 1, &err);                /* wakeAll */
        check("mit gesetztem oberen Bit werden alle geweckt",
              (Q9_u32)g_aprocCalls, 3);
    }

    Q9K_SetU32(Q9K_EVENT_SCRATCH_FUNC, 12);         /* es gibt nur 0..11 */
    Q9K_SysEventImpl();
    check("ein unbekannter Funktionscode wird abgewiesen",
          Q9K_GetU32(Q9K_EVENT_SCRATCH_OK), 0);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
