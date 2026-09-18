/*
 * test_q9kernel_sema.c -- Hosttest fuer F$Sema (q9kernel_sema.c,
 *                         2026-09-18).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_sema test_q9kernel_sema.c && ./test_sema
 *
 * Geprueft wird die Warteschlangenverwaltung und die Arbeitsteilung mit
 * dem Nutzercode -- nicht der Prozesswechsel selbst: Q9K_ProcAProc steht
 * hier als zaehlender Stub, das Schlafenlegen passiert ohnehin auf der
 * ASM-Seite (s. Kopfkommentar der Kerneldatei).
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;

static unsigned char g_cells[0x400];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9_D_PROC              CELL(0x000)
#define Q9K_SEMA_SCRATCH_PTR   CELL(0x020)
#define Q9K_SEMA_SCRATCH_OP    CELL(0x040)
#define Q9K_SEMA_SCRATCH_ERROR CELL(0x060)
#define Q9K_SEMA_SCRATCH_OK    CELL(0x080)
#define Q9K_SEMA_SCRATCH_SLEEP CELL(0x0A0)
#define Q9K_SEMA_SCRATCH_NEXT  CELL(0x0C0)
#define Q9K_PROCDESC_STATE_OFF 56UL

/* Feldabstaende auf Testbreite gezogen -- auf diesem Host ist Q9_u32 acht
 * Byte breit, die echten 4-Byte-Abstaende wuerden ineinanderlaufen. */
#define Q9K_SEMA_OFF_VALUE   0UL
#define Q9K_SEMA_OFF_LOCK    8UL
#define Q9K_SEMA_OFF_QNEXT  16UL
#define Q9K_SEMA_OFF_QPREV  24UL
#define Q9K_SEMA_OFF_LENGTH 32UL
#define Q9K_SEMA_OFF_OWNER  40UL
#define Q9K_READYQ_NEXT_OFF 48UL

/* Prozesswechsel gibt es auf dem Host nicht -- Stubs, die mitzaehlen. */
static int g_aprocCalls;
static unsigned long g_aprocLast;
static int g_aprocFails;

int Q9K_ProcAProc(Q9_u32 desc, Q9_u16 *outError)
{
    g_aprocCalls++;
    g_aprocLast = desc;
    if (g_aprocFails) { *outError = 0xE4; return 0; }
    return 1;
}

/* Der Scheduler waehlt hier einen festen "naechsten" Prozess -- geprueft
 * wird, DASS gewechselt wird, nicht wohin. */
static unsigned long g_nextPick;
static int g_pickCalls;
Q9_u32 Q9K_SchedFirstPick(void) { g_pickCalls++; return g_nextPick; }

#include "q9kernel_sema.c"

static int failures;
static unsigned char g_sema[128];
static unsigned char g_procs[4][128];

#define SEMA    ((Q9_u32)(unsigned long)g_sema)
#define PROC(i) ((Q9_u32)(unsigned long)g_procs[i])

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
    memset(g_sema, 0, sizeof g_sema);
    memset(g_procs, 0, sizeof g_procs);
    g_aprocCalls = 0; g_aprocLast = 0; g_aprocFails = 0;
    g_pickCalls = 0; g_nextPick = 0;
}

static Q9_u32 qlen(void)  { return Q9K_GetU32(SEMA + Q9K_SEMA_OFF_LENGTH); }
static Q9_u32 qhead(void) { return Q9K_GetU32(SEMA + Q9K_SEMA_OFF_QNEXT); }
static Q9_u32 qtail(void) { return Q9K_GetU32(SEMA + Q9K_SEMA_OFF_QPREV); }

int main(void)
{
    printf("== F$Sema ==\n");

    /* --- Die Warteschlange --- */
    reset();
    check("eine frische Schlange ist leer", qlen(), 0);
    check("und hat keinen Kopf", qhead(), 0);

    Q9K_SemaEnqueue(SEMA, PROC(0));
    check("nach dem ersten Einreihen ist die Laenge 1", qlen(), 1);
    check("Kopf ist der erste Prozess", qhead(), PROC(0));
    check("Schwanz ebenfalls", qtail(), PROC(0));

    Q9K_SemaEnqueue(SEMA, PROC(1));
    Q9K_SemaEnqueue(SEMA, PROC(2));
    check("nach drei Einreihungen ist die Laenge 3", qlen(), 3);
    check("der Kopf bleibt der erste", qhead(), PROC(0));
    check("der Schwanz ist der zuletzt eingereihte", qtail(), PROC(2));

    /* Die Reihenfolge ist der Punkt: "placed at the end of the
     * semaphore's wait queue" -- wer zuerst wartet, kommt zuerst dran.
     * Eine Schlange, die von vorn einreiht, laesst den ersten Warter
     * beliebig lange hungern. */
    check("entnommen wird der zuerst Eingereihte", Q9K_SemaDequeue(SEMA), PROC(0));
    check("dann der zweite", Q9K_SemaDequeue(SEMA), PROC(1));
    check("dann der dritte", Q9K_SemaDequeue(SEMA), PROC(2));
    check("danach ist die Schlange leer", qlen(), 0);
    check("und liefert 0", Q9K_SemaDequeue(SEMA), 0);
    check("der Schwanz ist mit zurueckgesetzt", qtail(), 0);

    /* Eine geleerte Schlange muss wieder benutzbar sein -- ein
     * haengengebliebener Schwanzzeiger haenge den naechsten Warter an
     * einen Prozess, der laengst weg ist. */
    Q9K_SemaEnqueue(SEMA, PROC(3));
    check("nach dem Leeren nimmt sie wieder auf", qhead(), PROC(3));
    check("mit richtigem Schwanz", qtail(), PROC(3));
    check("und Laenge 1", qlen(), 1);

    /* --- P ueber die Bruecke --- */
    reset();
    Q9K_SetU32(Q9_D_PROC, PROC(0));
    Q9K_SetU32(Q9K_SEMA_SCRATCH_PTR, SEMA);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 1);
    Q9K_SysSemaImpl();
    check("P meldet Erfolg", Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 1);
    check("und fordert das Schlafenlegen an", Q9K_GetU32(Q9K_SEMA_SCRATCH_SLEEP), 1);
    check("niemand wurde dabei geweckt", (Q9_u32)g_aprocCalls, 0);
    /* Der erste Teil entscheidet nur -- eingereiht wird erst, wenn die
     * ASM-Seite den Registersatz gesichert hat (sonst wartet der Prozess
     * mit veraltetem Stackzeiger). */
    check("eingereiht ist noch niemand", qlen(), 0);
    check("und noch kein Prozesswechsel gewaehlt", (Q9_u32)g_pickCalls, 0);

    Q9K_SysSemaWaitImpl();
    check("der zweite Teil reiht den Aufrufer ein", qhead(), PROC(0));
    check("sein Zustand ist 'p' (Semaphor-Warteschlange)",
          (Q9_u32)*(unsigned char *)(PROC(0) + Q9K_PROCDESC_STATE_OFF), 'p');
    check("und ein Prozesswechsel wurde gewaehlt", (Q9_u32)g_pickCalls, 1);

    /* Ein zweiter Warter reiht sich dahinter ein. */
    Q9K_SetU32(Q9_D_PROC, PROC(1));
    Q9K_SysSemaImpl();
    Q9K_SysSemaWaitImpl();
    check("ein zweiter Warter kommt dahinter", qtail(), PROC(1));
    check("die Laenge stimmt", qlen(), 2);

    /* --- V ueber die Bruecke --- */
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 2);
    Q9K_SysSemaImpl();
    check("V meldet Erfolg", Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 1);
    check("und weckt genau einen Prozess", (Q9_u32)g_aprocCalls, 1);
    check("naemlich den zuerst Wartenden", (Q9_u32)g_aprocLast, PROC(0));
    check("der aus der Schlange genommen ist", qhead(), PROC(1));
    check("V legt den Aufrufer NICHT schlafen",
          Q9K_GetU32(Q9K_SEMA_SCRATCH_SLEEP), 0);

    Q9K_SysSemaImpl();
    check("ein zweites V weckt den naechsten", (Q9_u32)g_aprocLast, PROC(1));
    check("die Schlange ist danach leer", qlen(), 0);

    /* V ohne Warter ist KEIN Fehler: zwischen dem s_lock-Test des
     * Aufrufers und dem Aufruf kann sich die Lage geaendert haben. Ein
     * Fehler hier wuerde eine korrekte Freigabe als Fehlschlag melden. */
    Q9K_SysSemaImpl();
    check("V ohne Warter ist kein Fehler", Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 1);
    check("und weckt niemanden zusaetzlich", (Q9_u32)g_aprocCalls, 2);

    /* Scheitert das Aktivieren, ist das ein echter Fehler -- der Warter
     * bliebe sonst fuer immer liegen, ohne dass es jemand erfaehrt. */
    reset();
    Q9K_SetU32(Q9_D_PROC, PROC(0));
    Q9K_SetU32(Q9K_SEMA_SCRATCH_PTR, SEMA);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 1);
    Q9K_SysSemaImpl();
    Q9K_SysSemaWaitImpl();
    g_aprocFails = 1;
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 2);
    Q9K_SysSemaImpl();
    check("ein fehlgeschlagenes Wecken wird gemeldet",
          Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 0);
    check("mit dem Fehlercode des Weckversuchs",
          Q9K_GetU32(Q9K_SEMA_SCRATCH_ERROR), 0xE4);

    /* --- Abweisungen --- */
    reset();
    Q9K_SetU32(Q9_D_PROC, PROC(0));
    Q9K_SetU32(Q9K_SEMA_SCRATCH_PTR, 0);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 1);
    Q9K_SysSemaImpl();
    check("ein Nullzeiger wird abgewiesen", Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 0);
    check("mit E_BPADDR", Q9K_GetU32(Q9K_SEMA_SCRATCH_ERROR), 0xD2);

    Q9K_SetU32(Q9K_SEMA_SCRATCH_PTR, SEMA);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 3);
    Q9K_SysSemaImpl();
    check("eine unbekannte Operation wird abgewiesen",
          Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 0);
    check("mit E_UNKSVC", Q9K_GetU32(Q9K_SEMA_SCRATCH_ERROR), 0xD0);

    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 0);
    Q9K_SysSemaImpl();
    check("Operation 0 ebenso -- init/term brauchen keinen Kernel",
          Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 0);

    /* P ohne laufenden Prozess: es gibt niemanden zum Schlafenlegen. Ein
     * stiller Erfolg hiesse, der Aufrufer wartet auf ein Wecken, das nie
     * kommt. */
    reset();
    Q9K_SetU32(Q9_D_PROC, 0);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_PTR, SEMA);
    Q9K_SetU32(Q9K_SEMA_SCRATCH_OP, 1);
    Q9K_SysSemaImpl();
    check("P ohne laufenden Prozess wird abgewiesen",
          Q9K_GetU32(Q9K_SEMA_SCRATCH_OK), 0);
    check("statt einen Warter ins Leere zu haengen", qlen(), 0);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
