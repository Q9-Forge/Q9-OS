/* Regressionstest fuer q9kernel_procapi.c (Host-gcc, kein Cross-Build).
 *
 * gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *     -o test_q9kernel_procapi test_q9kernel_procapi.c && ./test_q9kernel_procapi
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_globals[0x600];
/* Vier Pool-Slots. Die Groesse MUSS >= 4 * Q9K_PROCDESC_SIZE sein; die
 * Konstante selbst ist hier noch nicht sichtbar (sie kommt erst mit dem
 * #include unten), deshalb der bewusst grosszuegige Literalwert plus die
 * Kompilierzeit-Pruefung direkt nach dem Include. Frueher stand hier
 * 4 * 0x200 -- beim Vergroessern des Deskriptors auf 0x400 (2026-09-09)
 * fielen dadurch drei Tests still um genau einen halben Slot daneben. */
static unsigned char g_pool[4 * 0x400];
/* Groesser als ein Deskriptor, damit die Laengenbegrenzung von
 * F$GPrDsc nachweisbar ist. */
static unsigned char g_bigBuf[0x400 + 16];

/* Auf dem 64-Bit-Host brauchen die Q9_u32-Felder getrennte Abstaende. */
#define Q9_D_PROC                    ((unsigned long)(g_globals + 0x000))
#define Q9K_PROCPOOL_BASE_ADDR       ((unsigned long)(g_globals + 0x020))
#define Q9K_PROCPOOL_COUNT_ADDR      ((unsigned long)(g_globals + 0x040))
#define Q9K_GPROCP_SCRATCH_PID       ((unsigned long)(g_globals + 0x080))
#define Q9K_GPROCP_SCRATCH_DESC      ((unsigned long)(g_globals + 0x0A0))
#define Q9K_GPROCP_SCRATCH_ERROR     ((unsigned long)(g_globals + 0x0C0))
#define Q9K_GPROCP_SCRATCH_SUCCESS   ((unsigned long)(g_globals + 0x0E0))
#define Q9K_ID_SCRATCH_PID           ((unsigned long)(g_globals + 0x100))
#define Q9K_ID_SCRATCH_GROUPUSER     ((unsigned long)(g_globals + 0x120))
#define Q9K_ID_SCRATCH_PRIORITY      ((unsigned long)(g_globals + 0x140))
#define Q9K_ID_SCRATCH_ERROR          ((unsigned long)(g_globals + 0x160))
#define Q9K_ID_SCRATCH_SUCCESS        ((unsigned long)(g_globals + 0x180))
#define Q9K_SPRIOR_SCRATCH_PID        ((unsigned long)(g_globals + 0x1A0))
#define Q9K_SPRIOR_SCRATCH_PRIORITY   ((unsigned long)(g_globals + 0x1C0))
#define Q9K_SPRIOR_SCRATCH_ERROR      ((unsigned long)(g_globals + 0x1E0))
#define Q9K_SPRIOR_SCRATCH_SUCCESS    ((unsigned long)(g_globals + 0x200))
#define Q9K_SUSER_SCRATCH_GROUPUSER   ((unsigned long)(g_globals + 0x220))
#define Q9K_SUSER_SCRATCH_ERROR       ((unsigned long)(g_globals + 0x240))
#define Q9K_SUSER_SCRATCH_SUCCESS     ((unsigned long)(g_globals + 0x260))
#define Q9K_CPYMEM_SCRATCH_PID        ((unsigned long)(g_globals + 0x280))
#define Q9K_CPYMEM_SCRATCH_COUNT      ((unsigned long)(g_globals + 0x2A0))
#define Q9K_CPYMEM_SCRATCH_SRC        ((unsigned long)(g_globals + 0x2C0))
#define Q9K_CPYMEM_SCRATCH_DST        ((unsigned long)(g_globals + 0x2E0))
#define Q9K_CPYMEM_SCRATCH_ERROR      ((unsigned long)(g_globals + 0x300))
#define Q9K_CPYMEM_SCRATCH_SUCCESS    ((unsigned long)(g_globals + 0x320))
#define Q9K_GPRDSC_SCRATCH_PID        ((unsigned long)(g_globals + 0x340))
#define Q9K_GPRDSC_SCRATCH_COUNT      ((unsigned long)(g_globals + 0x360))
#define Q9K_GPRDSC_SCRATCH_BUF        ((unsigned long)(g_globals + 0x380))
#define Q9K_GPRDSC_SCRATCH_ERROR      ((unsigned long)(g_globals + 0x3A0))
#define Q9K_GPRDSC_SCRATCH_SUCCESS    ((unsigned long)(g_globals + 0x3C0))
#define Q9K_APROC_SCRATCH_DESC        ((unsigned long)(g_globals + 0x400))
#define Q9K_APROC_SCRATCH_ERROR       ((unsigned long)(g_globals + 0x420))
#define Q9K_APROC_SCRATCH_SUCCESS     ((unsigned long)(g_globals + 0x440))
#define Q9K_APROC_SCRATCH_PREEMPT     ((unsigned long)(g_globals + 0x4A0))
#define Q9K_GPRDBT_SCRATCH_BUF        ((unsigned long)(g_globals + 0x460))
#define Q9K_GPRDBT_SCRATCH_COUNT      ((unsigned long)(g_globals + 0x480))
/* P$User liegt real auf $14 und ist dort genau 4 Byte breit. Auf diesem
 * Host ist Q9_u32 aber 8 Byte breit, ein Zugriff wuerde also bis $1B
 * reichen und die Nachbarfelder P$Prior ($19) und P$Age ($1A)
 * ueberschreiben -- gleiche Grosszuegigkeit wie in
 * test_q9kernel_firstproc.c, betrifft NUR diesen Test. */
#define Q9K_PROCDESC_USER_OFF         0x300UL

/* Q9K_SchedInsert lebt in q9kernel_sched.c (Ready-Queue). Hier ein
 * aufrufzaehlender Stub -- dieser Test prueft nur, DASS F$AProc den
 * Scheduler mit dem richtigen Deskriptor beauftragt; das Einhaengen
 * selbst ist in test_q9kernel_sched.c eigenstaendig abgedeckt. */
static int g_schedInsertCalls;
static unsigned long g_schedInsertLast;
void Q9K_SchedInsert(unsigned long desc)
{
    g_schedInsertCalls++;
    g_schedInsertLast = desc;
}
void Q9K_SchedSetPriority(unsigned long desc, unsigned short priority)
{
    *(unsigned char *)(desc + 0x19UL) = (unsigned char)(priority & 0xFFU);
}

#include "q9kernel_procapi.c"

/* Kopplung des Fake-Pools an die echte Deskriptorgroesse: waechst
 * Q9K_PROCDESC_SIZE ueber das hier reservierte Viertel hinaus, bricht der
 * Build (negative Array-Groesse) statt still danebenzuliegen. */
typedef char Q9K_TestPoolFitsFourSlots[
    (sizeof(g_pool) >= 4 * Q9K_PROCDESC_SIZE) ? 1 : -1];

static int failures;

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want)
        printf("[OK]   %-62s = %lu\n", label, (unsigned long)got);
    else {
        printf("[FAIL] %-62s = %lu (erwartet %lu)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

int main(void)
{
    Q9_u32 base = (Q9_u32)(unsigned long)g_pool;
    Q9_u32 first = base;
    Q9_u32 second = base + Q9K_PROCDESC_SIZE;
    Q9_u32 third = base + 2UL * Q9K_PROCDESC_SIZE;

    memset(g_bigBuf, 0xEE, sizeof(g_bigBuf));
    memset(g_globals, 0, sizeof(g_globals));
    memset(g_pool, 0, sizeof(g_pool));
    Q9K_SetU32(Q9K_PROCPOOL_BASE_ADDR, base);
    Q9K_SetU32(Q9K_PROCPOOL_COUNT_ADDR, 4);

    *(Q9_u8 *)(first + Q9K_PROCDESC_STATE_OFF) = Q9K_PROCDESC_STATE_ACTIVE;
    *(Q9_u8 *)(first + Q9K_PROCDESC_PRIORITY_OFF) = 5;
    *(Q9_u8 *)(second + Q9K_PROCDESC_STATE_OFF) = Q9K_PROCDESC_STATE_WAITING;
    *(Q9_u8 *)(second + Q9K_PROCDESC_PRIORITY_OFF) = 17;
    *(Q9_u8 *)(third + Q9K_PROCDESC_STATE_OFF) = Q9K_PROCDESC_STATE_ZOMBIE;

    check("PID 1 loest auf den ersten aktiven Slot auf", Q9K_ProcLookup(1), first);
    check("PID 2 akzeptiert einen wartenden Prozess", Q9K_ProcLookup(2), second);
    check("PID 3 akzeptiert einen Zombie bis zum Reap", Q9K_ProcLookup(3), third);
    check("PID 0 wird abgewiesen", Q9K_ProcLookup(0), 0);
    check("PID ausserhalb des Prozesspools wird abgewiesen", Q9K_ProcLookup(5), 0);
    check("Freier Slot wird nicht als Prozess ausgegeben", Q9K_ProcLookup(4), 0);
    check("Slot 2 ergibt wieder die 1-basierte PID 2", Q9K_ProcIdForDesc(second), 2);
    check("Fremder Zeiger ergibt keine PID", Q9K_ProcIdForDesc(base + 1), 0);

    Q9K_SetU16(Q9K_GPROCP_SCRATCH_PID, 2);
    Q9K_SysGProcPImpl();
    check("F$GProcP-Bridge meldet Erfolg", Q9K_GetU16(Q9K_GPROCP_SCRATCH_SUCCESS), 1);
    check("F$GProcP-Bridge liefert den Deskriptor", Q9K_GetU32(Q9K_GPROCP_SCRATCH_DESC), second);

    Q9K_SetU16(Q9K_GPROCP_SCRATCH_PID, 4);
    Q9K_SysGProcPImpl();
    check("F$GProcP-Bridge meldet freien Slot als Fehler", Q9K_GetU16(Q9K_GPROCP_SCRATCH_SUCCESS), 0);
    check("F$GProcP-Bridge nutzt E$PrcID", Q9K_GetU16(Q9K_GPROCP_SCRATCH_ERROR), Q9K_E_PRCID);

    Q9K_SetU32(Q9_D_PROC, first);
    Q9K_SysIDImpl();
    check("F$ID-Bridge meldet Erfolg", Q9K_GetU16(Q9K_ID_SCRATCH_SUCCESS), 1);
    check("F$ID-Bridge liefert die aktuelle PID", Q9K_GetU16(Q9K_ID_SCRATCH_PID), 1);
    check("F$ID-Bridge liefert Gruppe/Benutzer 0", Q9K_GetU32(Q9K_ID_SCRATCH_GROUPUSER), 0);
    check("F$ID-Bridge liefert die aktuelle Prioritaet", Q9K_GetU16(Q9K_ID_SCRATCH_PRIORITY), 5);

    Q9K_SetU32(Q9_D_PROC, 0);
    Q9K_SysIDImpl();
    check("F$ID ohne aktuellen Prozess meldet Fehler", Q9K_GetU16(Q9K_ID_SCRATCH_SUCCESS), 0);
    check("F$ID ohne aktuellen Prozess nutzt E$PrcID", Q9K_GetU16(Q9K_ID_SCRATCH_ERROR), Q9K_E_PRCID);

    /* F$SPrior (Callcode 0x0D): gueltige PID -> neue Prioritaet im
     * Deskriptor sichtbar. */
    Q9K_SetU16(Q9K_SPRIOR_SCRATCH_PID, 2);
    Q9K_SetU16(Q9K_SPRIOR_SCRATCH_PRIORITY, 42);
    Q9K_SysSPriorImpl();
    check("F$SPrior-Bridge meldet Erfolg", Q9K_GetU16(Q9K_SPRIOR_SCRATCH_SUCCESS), 1);
    check("F$SPrior setzt die neue Prioritaet im Deskriptor",
          (Q9_u32)Q9K_GetU8(second + Q9K_PROCDESC_PRIORITY_OFF), 42);

    /* Ungueltige/freie PID muss E$IPrcID melden, kein Deskriptor betroffen. */
    Q9K_SetU16(Q9K_SPRIOR_SCRATCH_PID, 4);
    Q9K_SetU16(Q9K_SPRIOR_SCRATCH_PRIORITY, 99);
    Q9K_SysSPriorImpl();
    check("F$SPrior-Bridge meldet freien Slot als Fehler", Q9K_GetU16(Q9K_SPRIOR_SCRATCH_SUCCESS), 0);
    check("F$SPrior-Bridge nutzt E$PrcID", Q9K_GetU16(Q9K_SPRIOR_SCRATCH_ERROR), Q9K_E_PRCID);

    /* Reale Prioritaet ist wortbreit (bis 65535); dieser Kernel schneidet
     * bewusst auf ein Byte ab (s. Q9K_ProcSPrior-Kopfkommentar). */
    Q9K_SetU16(Q9K_SPRIOR_SCRATCH_PID, 1);
    Q9K_SetU16(Q9K_SPRIOR_SCRATCH_PRIORITY, 0x1234);
    Q9K_SysSPriorImpl();
    check("F$SPrior-Bridge meldet Erfolg trotz Wort-Prioritaet", Q9K_GetU16(Q9K_SPRIOR_SCRATCH_SUCCESS), 1);
    check("F$SPrior schneidet die Prioritaet auf ein Byte ab",
          (Q9_u32)Q9K_GetU8(first + Q9K_PROCDESC_PRIORITY_OFF), 0x34);

    /* F$SUser (Callcode 0x1C): nur Benutzer 0.0 darf die eigene ID
     * beliebig aendern, danach ist der Weg zurueck versperrt. */
    {
        Q9_u16 err = 0xFFFF;

        Q9K_SetU32(first + Q9K_PROCDESC_USER_OFF, 0);
        check("F$SUser als 0.0 wird angenommen",
              (Q9_u32)Q9K_ProcSUser(first, 0x00030007UL, &err), 1);
        check("F$SUser legt Gruppe/Benutzer im Deskriptor ab",
              Q9K_GetU32(first + Q9K_PROCDESC_USER_OFF), 0x00030007UL);

        err = 0;
        check("F$SUser als Nicht-0.0 wird abgelehnt",
              (Q9_u32)Q9K_ProcSUser(first, 0, &err), 0);
        check("F$SUser meldet dabei E$Permit", (Q9_u32)err, Q9K_E_PERMIT);
        check("F$SUser laesst die ID bei Ablehnung unveraendert",
              Q9K_GetU32(first + Q9K_PROCDESC_USER_OFF), 0x00030007UL);

        err = 0;
        check("F$SUser ohne aktuellen Prozess meldet Fehlschlag",
              (Q9_u32)Q9K_ProcSUser(0, 1, &err), 0);
        check("F$SUser ohne aktuellen Prozess meldet E$PrcID", (Q9_u32)err, Q9K_E_PRCID);

        /* F$ID muss jetzt das echte Feld liefern, nicht mehr fest 0. */
        Q9K_SetU32(Q9_D_PROC, first);
        Q9K_SysIDImpl();
        check("F$ID liefert die per F$SUser gesetzte Gruppe/Benutzer",
              Q9K_GetU32(Q9K_ID_SCRATCH_GROUPUSER), 0x00030007UL);

        Q9K_SetU32(first + Q9K_PROCDESC_USER_OFF, 0);
    }

    /* F$CpyMem (Callcode 0x1B): PID pruefen, dann kopieren. */
    {
        static unsigned char srcBuf[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        static unsigned char dstBuf[8];
        Q9_u16 err = 0xFFFF;

        memset(dstBuf, 0, sizeof(dstBuf));
        check("F$CpyMem mit gueltiger PID meldet Erfolg",
              (Q9_u32)Q9K_ProcCpyMem(1, 4, (Q9_u32)(unsigned long)srcBuf,
                                     (Q9_u32)(unsigned long)dstBuf, &err), 1);
        check("F$CpyMem kopiert genau die angeforderte Byteanzahl",
              (Q9_u32)dstBuf[3], 4);
        check("F$CpyMem laesst das Byte dahinter unberuehrt", (Q9_u32)dstBuf[4], 0);

        err = 0;
        memset(dstBuf, 0, sizeof(dstBuf));
        check("F$CpyMem mit freier PID meldet Fehlschlag",
              (Q9_u32)Q9K_ProcCpyMem(4, 4, (Q9_u32)(unsigned long)srcBuf,
                                     (Q9_u32)(unsigned long)dstBuf, &err), 0);
        check("F$CpyMem meldet dabei E$PrcID", (Q9_u32)err, Q9K_E_PRCID);
        check("F$CpyMem kopiert bei abgelehnter PID nichts", (Q9_u32)dstBuf[0], 0);

        err = 0xFFFF;
        check("F$CpyMem mit Laenge 0 ist ein gueltiger Leerlauf",
              (Q9_u32)Q9K_ProcCpyMem(1, 0, 0, 0, &err), 1);
        check("F$CpyMem meldet dabei keinen Fehler", (Q9_u32)err, 0);
    }

    /* Die Scratch-Bruecken selbst. Der Assembler liest Erfolg und Fehler
     * als unteres Wort einer 32-Bit-Zelle ("+2"), die C-Seite MUSS sie
     * deshalb mit der vollen Breite schreiben. Ein SetU16 traefe das
     * obere Wort, und jeder Aufruf saehe fuer den Assembler wie ein
     * Fehlschlag aus -- genau dieser Fehler war live zu sehen, bevor es
     * diese beiden Testbloecke gab. */
    {
        static unsigned char srcBuf[4] = { 'Q', '9', 'O', 'S' };
        static unsigned char dstBuf[4];

        Q9K_SetU32(first + Q9K_PROCDESC_USER_OFF, 0);
        Q9K_SetU32(Q9_D_PROC, first);
        Q9K_SetU32(Q9K_SUSER_SCRATCH_GROUPUSER, 0x00010002UL);
        Q9K_SysSUserImpl();
        check("F$SUser-Bridge meldet Erfolg in voller Zellbreite",
              Q9K_GetU32(Q9K_SUSER_SCRATCH_SUCCESS), 1);
        check("F$SUser-Bridge hat die ID wirklich gesetzt",
              Q9K_GetU32(first + Q9K_PROCDESC_USER_OFF), 0x00010002UL);

        Q9K_SysSUserImpl();   /* jetzt nicht mehr 0.0 */
        check("F$SUser-Bridge meldet den zweiten Versuch als Fehlschlag",
              Q9K_GetU32(Q9K_SUSER_SCRATCH_SUCCESS), 0);
        check("F$SUser-Bridge legt E$Permit in voller Zellbreite ab",
              Q9K_GetU32(Q9K_SUSER_SCRATCH_ERROR), Q9K_E_PERMIT);
        Q9K_SetU32(first + Q9K_PROCDESC_USER_OFF, 0);

        memset(dstBuf, 0, sizeof(dstBuf));
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_PID, 1);
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_COUNT, 4);
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_SRC, (Q9_u32)(unsigned long)srcBuf);
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_DST, (Q9_u32)(unsigned long)dstBuf);
        Q9K_SysCpyMemImpl();
        check("F$CpyMem-Bridge meldet Erfolg in voller Zellbreite",
              Q9K_GetU32(Q9K_CPYMEM_SCRATCH_SUCCESS), 1);
        check("F$CpyMem-Bridge hat wirklich kopiert", (Q9_u32)dstBuf[3], 'S');

        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_PID, 4);   /* freier Slot */
        Q9K_SysCpyMemImpl();
        check("F$CpyMem-Bridge meldet freie PID als Fehlschlag",
              Q9K_GetU32(Q9K_CPYMEM_SCRATCH_SUCCESS), 0);
        check("F$CpyMem-Bridge legt E$PrcID in voller Zellbreite ab",
              Q9K_GetU32(Q9K_CPYMEM_SCRATCH_ERROR), Q9K_E_PRCID);
    }


    /* F$GPrDsc (Callcode 0x18): liest einen Deskriptor heraus, veraendert
     * ihn nie und kopiert nie ueber seine Groesse hinaus. */
    {
        static unsigned char buf[64];
        Q9_u16 err;
        unsigned i;

        for (i = 0; i < sizeof(buf); ++i)
            buf[i] = 0xEE;
        *(Q9_u8 *)(second + 0) = 0x5A;
        *(Q9_u8 *)(second + 1) = 0xA5;

        err = 0xFFFF;
        check("F$GPrDsc mit gueltiger PID meldet Erfolg",
              (Q9_u32)Q9K_ProcGPrDsc(2, 2, (Q9_u32)(unsigned long)buf, &err), 1);
        check("F$GPrDsc kopiert das erste Deskriptorbyte", (Q9_u32)buf[0], 0x5A);
        check("F$GPrDsc kopiert das zweite Deskriptorbyte", (Q9_u32)buf[1], 0xA5);
        check("F$GPrDsc kopiert kein Byte zu viel", (Q9_u32)buf[2], 0xEE);

        err = 0;
        check("F$GPrDsc mit freier PID schlaegt fehl",
              (Q9_u32)Q9K_ProcGPrDsc(4, 2, (Q9_u32)(unsigned long)buf, &err), 0);
        check("F$GPrDsc meldet dabei E$PrcID", (Q9_u32)err, Q9K_E_PRCID);

        err = 0;
        check("F$GPrDsc mit Null-Puffer schlaegt fehl",
              (Q9_u32)Q9K_ProcGPrDsc(2, 2, 0, &err), 0);

        /* Ueber die Deskriptorgroesse hinaus wird nie kopiert -- alles
         * dahinter gehoert schon dem naechsten Pool-Slot. */
        err = 0xFFFF;
        check("F$GPrDsc begrenzt eine zu grosse Anforderung",
              (Q9_u32)Q9K_ProcGPrDsc(2, 0xFFFFUL, (Q9_u32)(unsigned long)g_bigBuf, &err), 1);
        check("F$GPrDsc laesst das Byte hinter dem Deskriptor unberuehrt",
              (Q9_u32)g_bigBuf[Q9K_PROCDESC_SIZE], 0xEE);

        /* Die Bridge schreibt Erfolg/Fehler in voller Zellbreite. */
        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_PID, 2);
        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_COUNT, 2);
        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_BUF, (Q9_u32)(unsigned long)buf);
        Q9K_SysGPrDscImpl();
        check("F$GPrDsc-Bridge meldet Erfolg in voller Zellbreite",
              Q9K_GetU32(Q9K_GPRDSC_SCRATCH_SUCCESS), 1);

        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_PID, 4);
        Q9K_SysGPrDscImpl();
        check("F$GPrDsc-Bridge meldet freie PID als Fehlschlag",
              Q9K_GetU32(Q9K_GPRDSC_SCRATCH_SUCCESS), 0);
        check("F$GPrDsc-Bridge legt E$PrcID in voller Zellbreite ab",
              Q9K_GetU32(Q9K_GPRDSC_SCRATCH_ERROR), Q9K_E_PRCID);
    }


    /* F$AProc (Callcode 0x2C): beauftragt den Scheduler, aber nur mit
     * einem Deskriptor, der wirklich zu einem belegten Slot gehoert. */
    {
        Q9_u16 err;

        g_schedInsertCalls = 0;
        g_schedInsertLast = 0;
        /* Ein lauffaehiger Prozess hat einen gesicherten Stack -- ohne den
         * wuerde der Scheduler auf Adresse 0 umschalten (s. Q9K_ProcAProc). */
        Q9K_SetU32(second + Q9K_PROCDESC_SAVEDSP_OFF, 0x2000UL);
        err = 0xFFFF;
        check("F$AProc nimmt einen belegten, lauffaehigen Deskriptor an",
              (Q9_u32)Q9K_ProcAProc(second, &err), 1);
        check("F$AProc reicht genau diesen Deskriptor weiter",
              (Q9_u32)g_schedInsertLast, second);
        check("F$AProc ruft den Scheduler genau einmal", (Q9_u32)g_schedInsertCalls, 1);
        check("F$AProc meldet dabei keinen Fehler", (Q9_u32)err, 0);

        err = 0;
        check("F$AProc weist den Nullzeiger ab",
              (Q9_u32)Q9K_ProcAProc(0, &err), 0);
        check("F$AProc meldet dabei E$PrcID", (Q9_u32)err, Q9K_E_PRCID);

        /* Ein Zeiger, der nicht auf einen belegten Pool-Slot zeigt, darf
         * nicht in die Ready-Queue -- sonst verkettet sie sich in den
         * freien Speicher hinein. */
        err = 0;
        check("F$AProc weist einen poolfremden Zeiger ab",
              (Q9_u32)Q9K_ProcAProc(base + 1UL, &err), 0);
        check("F$AProc weist einen freien Slot ab",
              (Q9_u32)Q9K_ProcAProc(base + 3UL * Q9K_PROCDESC_SIZE, &err), 0);
        check("F$AProc hat den Scheduler dabei nie erneut gerufen",
              (Q9_u32)g_schedInsertCalls, 1);

        /* Der eigentliche Fund vom 2026-09-18: ein belegter, aber noch
         * nicht lauffaehiger Deskriptor (SavedSP == 0, wie ihn F$AllPrc
         * liefert) darf NICHT in die Ready-Queue -- sonst schaltet der
         * Scheduler beim naechsten Tick auf Adresse 0 um und faellt in
         * einen Format Error. */
        Q9K_SetU32(third + Q9K_PROCDESC_SAVEDSP_OFF, 0);
        err = 0;
        check("F$AProc weist einen Deskriptor ohne gesicherten Stack ab",
              (Q9_u32)Q9K_ProcAProc(third, &err), 0);
        check("F$AProc meldet auch dafuer E$PrcID", (Q9_u32)err, Q9K_E_PRCID);
        check("F$AProc hat den Scheduler dafuer nicht gerufen",
              (Q9_u32)g_schedInsertCalls, 1);

        /* Bridge: volle Zellbreite. */
        Q9K_SetU32(Q9K_APROC_SCRATCH_DESC, second);
        Q9K_SysAProcImpl();
        check("F$AProc-Bridge meldet Erfolg in voller Zellbreite",
              Q9K_GetU32(Q9K_APROC_SCRATCH_SUCCESS), 1);
        Q9K_SetU32(Q9K_APROC_SCRATCH_DESC, 0);
        Q9K_SysAProcImpl();
        check("F$AProc-Bridge meldet den Nullzeiger als Fehlschlag",
              Q9K_GetU32(Q9K_APROC_SCRATCH_SUCCESS), 0);
        check("F$AProc-Bridge legt E$PrcID in voller Zellbreite ab",
              Q9K_GetU32(Q9K_APROC_SCRATCH_ERROR), Q9K_E_PRCID);
    }

    /* F$GPrDBT (Callcode 0x1F): Zeigertabelle aus dem Pool, ein Eintrag
     * je Slot, 0 fuer einen freien -- immer 4 Byte je Eintrag, auch auf
     * diesem Host mit 8 Byte breitem Q9_u32. */
    {
        static unsigned char buf[64];
        Q9_u32 copied;
        unsigned i;

        for (i = 0; i < sizeof(buf); ++i)
            buf[i] = 0xEE;

        copied = Q9K_ProcGPrDBT((Q9_u32)(unsigned long)buf, sizeof(buf));
        check("F$GPrDBT liefert einen Eintrag je Pool-Slot", copied, 4UL * 4UL);

        /* Slot 4 ist frei (s. Testaufbau oben) und muss als 0 erscheinen. */
        check("F$GPrDBT traegt den freien Slot als 0 ein",
              (Q9_u32)((buf[12] << 24) | (buf[13] << 16) | (buf[14] << 8) | buf[15]), 0UL);
        /* Die belegten Slots tragen die unteren 32 Bit ihrer Adresse. */
        check("F$GPrDBT traegt den ersten belegten Slot ein",
              (Q9_u32)((buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]),
              first & 0xFFFFFFFFUL);

        copied = Q9K_ProcGPrDBT((Q9_u32)(unsigned long)buf, 6);
        check("F$GPrDBT schneidet auf ganze Eintraege ab", copied, 4UL);
        copied = Q9K_ProcGPrDBT((Q9_u32)(unsigned long)buf, 0);
        check("F$GPrDBT mit Puffergroesse 0 kopiert nichts", copied, 0UL);
        copied = Q9K_ProcGPrDBT(0, sizeof(buf));
        check("F$GPrDBT mit Null-Puffer kopiert nichts", copied, 0UL);

        Q9K_SetU32(Q9K_GPRDBT_SCRATCH_BUF, (Q9_u32)(unsigned long)buf);
        Q9K_SetU32(Q9K_GPRDBT_SCRATCH_COUNT, sizeof(buf));
        Q9K_SysGPrDBTImpl();
        check("F$GPrDBT-Bridge legt die Byteanzahl in der Zelle ab",
              Q9K_GetU32(Q9K_GPRDBT_SCRATCH_COUNT), 16UL);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
