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
    Q9K_EventSignal(id, &err);
    Q9K_EventRead(id, &value, &err);
    check("Ev$Signl addiert das Signal-Inkrement", value, 15);
    Q9K_EventSignal(id, &err);
    Q9K_EventRead(id, &value, &err);
    check("und tut das jedes Mal", value, 20);

    /* --- Setzen, absolut und relativ --- */
    check("Ev$Set setzt den Wert",
          (Q9_u32)Q9K_EventSet(id, 100, &prev, &err), 1);
    check("und meldet den vorherigen", prev, 20);
    Q9K_EventRead(id, &value, &err);
    check("der neue Wert steht drin", value, 100);

    Q9K_EventSetRelative(id, -30, &prev, &err);
    Q9K_EventRead(id, &value, &err);
    check("Ev$SetR rechnet relativ", value, 70);
    check("und meldet ebenfalls den vorherigen", prev, 100);

    /* Saettigung statt Umlauf: "overflows are set to $7fffffff". Ein
     * umlaufender Zaehler kippt von "sehr viel frei" auf "voll". */
    Q9K_EventSet(id, 0x7FFFFFF0UL, &prev, &err);
    Q9K_EventSetRelative(id, 1000, &prev, &err);
    Q9K_EventRead(id, &value, &err);
    check("ein Ueberlauf saettigt bei 0x7fffffff", value, 0x7FFFFFFFUL);
    Q9K_EventSet(id, 0x80000010UL, &prev, &err);
    Q9K_EventSetRelative(id, -1000, &prev, &err);
    Q9K_EventRead(id, &value, &err);
    check("ein Unterlauf bei 0x80000000", value, 0x80000000UL);

    /* --- Ev$Pulse stellt den Wert wieder her --- */
    reset();
    setName("puls");
    Q9K_EventCreate(NAME, 7, 0, 1, &id, &err);
    check("Ev$Pulse meldet Erfolg", (Q9_u32)Q9K_EventPulse(id, 99, &err), 1);
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

    /* Ev$Wait fehlt noch und muss das SAGEN -- ein stiller Erfolg liesse
     * den Aufrufer weiterlaufen, als haette er gewartet. */
    Q9K_SetU32(Q9K_EVENT_SCRATCH_FUNC, 4);          /* Ev$Wait */
    Q9K_SysEventImpl();
    check("Bruecke Ev$Wait meldet, dass es fehlt",
          Q9K_GetU32(Q9K_EVENT_SCRATCH_OK), 0);
    check("mit E_UNKSVC", Q9K_GetU32(Q9K_EVENT_SCRATCH_ERROR), 0xD0);

    Q9K_SetU32(Q9K_EVENT_SCRATCH_FUNC, 12);         /* es gibt nur 0..11 */
    Q9K_SysEventImpl();
    check("ein unbekannter Funktionscode wird abgewiesen",
          Q9K_GetU32(Q9K_EVENT_SCRATCH_OK), 0);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
