/* Regressionstest fuer q9kernel_procapi.c (Host-gcc, kein Cross-Build).
 *
 * gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *     -o test_q9kernel_procapi test_q9kernel_procapi.c && ./test_q9kernel_procapi
 */

#include <stdio.h>
#include <string.h>

static unsigned char g_globals[0x400];
/* Vier Pool-Slots. Die Groesse MUSS >= 4 * Q9K_PROCDESC_SIZE sein; die
 * Konstante selbst ist hier noch nicht sichtbar (sie kommt erst mit dem
 * #include unten), deshalb der bewusst grosszuegige Literalwert plus die
 * Kompilierzeit-Pruefung direkt nach dem Include. Frueher stand hier
 * 4 * 0x200 -- beim Vergroessern des Deskriptors auf 0x400 (2026-09-09)
 * fielen dadurch drei Tests still um genau einen halben Slot daneben. */
static unsigned char g_pool[4 * 0x400];

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

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
