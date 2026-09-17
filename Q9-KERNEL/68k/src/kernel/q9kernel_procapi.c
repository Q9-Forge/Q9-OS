/*
 * q9kernel_procapi.c -- Q9-OS eigener Kernel: Prozess-API (lesend +
 * F$SPrior als erster schreibender Call, 2026-09-17)
 *
 * Implementiert die gemeinsame, kleine Grundlage fuer F$GProcP, F$ID und
 * F$SPrior.  Alle drei Calls sind vollstaendig kernel-intern: kein
 * Treiber, kein Dateisystem und kein Kontextwechsel sind daran beteiligt.
 * Die Register-Bruecken liegen in q9kernel_entry.a; diese Datei enthaelt
 * nur die testbare Prozess-ID<->Deskriptor-Abbildung und die Scratch-
 * Ergebnisse.
 *
 * Verifizierte ABI (68k_bls.pdf, S. 443/451; 68k_tech.pdf S. 499f fuer
 * F$SPrior):
 *
 *   F$GProcP: d0.w=PID -> (a1)=Prozessdeskriptor, E$PrcID ($E0)
 *              bei ungueltiger PID.
 *   F$ID:     -> d0.w=PID, d1.l=Gruppe/Benutzer, d2.w=Prioritaet.
 *   F$SPrior: d0.w=PID, d1.w=gewuenschte Prioritaet (0=niedrigste,
 *              65535=hoechste) -> keine Ausgabe bei Erfolg, sonst Carry
 *              + d1.w=E$IPrcID ($E0) bei ungueltiger PID.
 *
 * Der aktuelle eigene Prozessdeskriptor hat noch NICHT das komplette
 * Microware-P$-Layout.  Diese Calls stellen deshalb nur die dokumentierte
 * Call-ABI bereit; Gruppen-/Benutzerkennung ist bis zum Security-Block
 * bewusst 0.  Die PID entspricht wie bei Q9K_ProcFork dem 1-basierten
 * Prozesspool-Slot.  Das ist bereits stabil genug fuer IOMan, das
 * F$GProcP als Kernelprimitive nutzt.
 *
 * F$SPriors reale Benutzer-/Gruppenpruefung ("nur derselbe Benutzer oder
 * Gruppe 0 darf eine fremde Prioritaet aendern") entfaellt hier bewusst:
 * jeder Prozess traegt ohnehin Gruppe/Benutzer 0 (s. Q9K_SysIDImpl oben)
 * -- das entspricht in der realen Konvention bereits dem Superuser-Fall,
 * der IMMER darf.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif
#ifndef Q9K_PROCPOOL_BASE_ADDR
#define Q9K_PROCPOOL_BASE_ADDR 0x1204UL
#endif
#ifndef Q9K_PROCPOOL_COUNT_ADDR
#define Q9K_PROCPOOL_COUNT_ADDR 0x1220UL
#endif
#ifndef Q9K_PROCDESC_SIZE
#define Q9K_PROCDESC_SIZE 0x400UL
#endif
/* Schiebeweite statt Multiplikation/Division (kein 32-Bit-Helfer im
 * fruehen Kernelpfad, s. Q9K_ProcLookup unten). FRUEHER waren hier drei
 * hartkodierte "9" verstreut -- beim Vergroessern von 0x200 auf 0x400
 * (2026-09-09, Fortsetzung 27) waeren sie beinahe stehengeblieben und
 * haetten jede PID<->Slot-Umrechnung still halbiert. Deshalb jetzt EINE
 * abgeleitete Konstante plus Kompilierzeit-Kopplung: passt der Shift
 * nicht mehr zur Groesse, schlaegt der Build mit einer negativen
 * Array-Groesse fehl (C89-tauglich, kein static_assert noetig). */
#ifndef Q9K_PROCDESC_SHIFT
#define Q9K_PROCDESC_SHIFT 10UL   /* 0x400 = 1 << 10 */
#endif
typedef char Q9K_ProcDescShiftMatchesSize[
    ((1UL << Q9K_PROCDESC_SHIFT) == Q9K_PROCDESC_SIZE) ? 1 : -1];
#ifndef Q9K_PROCDESC_STATE_OFF
#define Q9K_PROCDESC_STATE_OFF 0x1DUL
#endif
#ifndef Q9K_PROCDESC_PRIORITY_OFF
#define Q9K_PROCDESC_PRIORITY_OFF 0x19UL
#endif

/* Direkt hinter den I$Open-Scratch-Feldern.  $1370/$1374 sind noch
 * temporaere Diagnosefelder, daher beginnt die dauerhafte Prozess-API
 * bewusst erst bei $1380. */
#ifndef Q9K_GPROCP_SCRATCH_PID
#define Q9K_GPROCP_SCRATCH_PID     0x1380UL /* d0.w EIN */
#endif
#ifndef Q9K_GPROCP_SCRATCH_DESC
#define Q9K_GPROCP_SCRATCH_DESC    0x1384UL /* (a1) AUS */
#endif
#ifndef Q9K_GPROCP_SCRATCH_ERROR
#define Q9K_GPROCP_SCRATCH_ERROR   0x1388UL /* d1.w AUS bei Fehler */
#endif
#ifndef Q9K_GPROCP_SCRATCH_SUCCESS
#define Q9K_GPROCP_SCRATCH_SUCCESS 0x138CUL /* 0/1 */
#endif
#ifndef Q9K_ID_SCRATCH_PID
#define Q9K_ID_SCRATCH_PID         0x1390UL /* d0.w AUS */
#endif
#ifndef Q9K_ID_SCRATCH_GROUPUSER
#define Q9K_ID_SCRATCH_GROUPUSER   0x1394UL /* d1.l AUS */
#endif
#ifndef Q9K_ID_SCRATCH_PRIORITY
#define Q9K_ID_SCRATCH_PRIORITY    0x1398UL /* d2.w AUS */
#endif
#ifndef Q9K_ID_SCRATCH_ERROR
#define Q9K_ID_SCRATCH_ERROR       0x139CUL /* d1.w AUS bei Fehler */
#endif
#ifndef Q9K_ID_SCRATCH_SUCCESS
#define Q9K_ID_SCRATCH_SUCCESS     0x13A0UL /* 0/1 */
#endif

/* F$SPrior-Scratch (2026-09-17): erster schreibender Prozess-API-Call in
 * dieser Datei.  Hinter dem gesamten bisher belegten Kernel-Scratchbereich
 * (hoechste bekannte Adresse $1894, Ende der F$SRqMem-Eigentuemertabelle
 * in q9kernel_sysmem.c) -- bewusst mit Abstand, gleiche Konvention wie
 * ueberall in diesem Kernel (kleine Luecken zwischen Bloecken). */
#ifndef Q9K_SPRIOR_SCRATCH_PID
#define Q9K_SPRIOR_SCRATCH_PID      0x18A0UL /* Q9_u32, d0.w EIN */
#endif
#ifndef Q9K_SPRIOR_SCRATCH_PRIORITY
#define Q9K_SPRIOR_SCRATCH_PRIORITY 0x18A4UL /* Q9_u32, d1.w EIN */
#endif
#ifndef Q9K_SPRIOR_SCRATCH_ERROR
#define Q9K_SPRIOR_SCRATCH_ERROR    0x18A8UL /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_SPRIOR_SCRATCH_SUCCESS
#define Q9K_SPRIOR_SCRATCH_SUCCESS  0x18ACUL /* Q9_u32, 0/1 */
#endif

#define Q9K_E_PRCID 0x00E0U /* errno.h: invalid process ID (E$IPrcID) */

#define Q9K_PROCDESC_STATE_ACTIVE   'a'
#define Q9K_PROCDESC_STATE_WAITING  'w'
#define Q9K_PROCDESC_STATE_SLEEPING 's'
#define Q9K_PROCDESC_STATE_ZOMBIE   'z'

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static Q9_u8 Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

/* Freie Pool-Slots enthalten an Offset 0 den Freilistenzeiger.  Daher darf
 * eine PID nicht allein aus ihrem Bereich abgeleitet werden: nur einer der
 * vier bekannten internen Zustandswerte markiert einen belegten Slot. */
static int Q9K_ProcIsAllocated(Q9_u32 desc)
{
    Q9_u8 state = Q9K_GetU8(desc + Q9K_PROCDESC_STATE_OFF);
    return state == Q9K_PROCDESC_STATE_ACTIVE
        || state == Q9K_PROCDESC_STATE_WAITING
        || state == Q9K_PROCDESC_STATE_SLEEPING
        || state == Q9K_PROCDESC_STATE_ZOMBIE;
}

/* Prozess-ID -> belegter Pool-Slot, 0 bei ungueltiger oder freier ID.
 * Q9K_PROCDESC_SIZE ist bewusst eine Zweierpotenz (1024), deshalb kein
 * 32-Bit-Multiplikationshelper im fruehen Kernelpfad erforderlich. */
Q9_u32 Q9K_ProcLookup(Q9_u16 pid)
{
    Q9_u32 base = Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR);
    Q9_u32 count = Q9K_GetU32(Q9K_PROCPOOL_COUNT_ADDR);
    Q9_u32 desc;

    if (pid == 0 || (Q9_u32)pid > count || base == 0)
        return 0;

    desc = base + (((Q9_u32)pid - 1UL) << Q9K_PROCDESC_SHIFT);
    return Q9K_ProcIsAllocated(desc) ? desc : 0;
}

/* Gibt die zu einem belegten Pool-Slot gehoerende 1-basierte PID zurueck.
 * Der Bereichs-/Ausrichtungstest verhindert, dass ein fremder Zeiger in
 * Q9_D_PROC als scheinbar gueltige Prozess-ID zurueckkehrt. */
Q9_u16 Q9K_ProcIdForDesc(Q9_u32 desc)
{
    Q9_u32 base = Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR);
    Q9_u32 count = Q9K_GetU32(Q9K_PROCPOOL_COUNT_ADDR);
    Q9_u32 delta;

    if (base == 0 || count == 0 || desc < base)
        return 0;

    delta = desc - base;
    if ((delta & (Q9K_PROCDESC_SIZE - 1UL)) != 0
        || (delta >> Q9K_PROCDESC_SHIFT) >= count
        || !Q9K_ProcIsAllocated(desc))
        return 0;

    return (Q9_u16)((delta >> Q9K_PROCDESC_SHIFT) + 1UL);
}

void Q9K_SysGProcPImpl(void)
{
    Q9_u16 pid = Q9K_GetU16(Q9K_GPROCP_SCRATCH_PID);
    Q9_u32 desc = Q9K_ProcLookup(pid);

    if (desc == 0) {
        Q9K_SetU16(Q9K_GPROCP_SCRATCH_ERROR, Q9K_E_PRCID);
        Q9K_SetU16(Q9K_GPROCP_SCRATCH_SUCCESS, 0);
        return;
    }

    Q9K_SetU32(Q9K_GPROCP_SCRATCH_DESC, desc);
    Q9K_SetU16(Q9K_GPROCP_SCRATCH_SUCCESS, 1);
}

void Q9K_SysIDImpl(void)
{
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 pid = Q9K_ProcIdForDesc(desc);

    if (pid == 0) {
        Q9K_SetU16(Q9K_ID_SCRATCH_ERROR, Q9K_E_PRCID);
        Q9K_SetU16(Q9K_ID_SCRATCH_SUCCESS, 0);
        return;
    }

    Q9K_SetU16(Q9K_ID_SCRATCH_PID, pid);
    Q9K_SetU32(Q9K_ID_SCRATCH_GROUPUSER, 0); /* Security-/User-Modell folgt separat. */
    Q9K_SetU16(Q9K_ID_SCRATCH_PRIORITY,
                (Q9_u16)Q9K_GetU8(desc + Q9K_PROCDESC_PRIORITY_OFF));
    Q9K_SetU16(Q9K_ID_SCRATCH_SUCCESS, 1);
}

/* Q9K_ProcSPrior -- echte F$SPrior-Kernlogik (Callcode $0D, "Set Process
 * Priority"), s. Kopfkommentar fuer die verifizierte ABI.
 *
 * Dieser Kernel legt die Prioritaet nur im unteren Byte von P$Prior ab
 * (Q9K_PROCDESC_PRIORITY_OFF, s. q9kernel_sched.c) -- die uebergebene,
 * real wortbreite Prioritaet wird deshalb auf 0..255 abgeschnitten,
 * dieselbe bewusste Vereinfachung wie bei Q9K_ProcFork/Q9K_ProcCreate.
 *
 * BEKANNTE EINSCHRAENKUNG: nur das Prioritaetsfeld wird geaendert. Der
 * Scheduler (q9kernel_sched.c, Q9K_SchedInsert) setzt Age=Prioritaet nur
 * BEIM Einhaengen in die Ready-Queue -- ein bereits eingehaengter Prozess
 * wirkt sich also erst beim naechsten Ready-Queue-Eintritt (z.B. nach
 * Schlaf/Block) mit der neuen Prioritaet aus, nicht sofort. Reale
 * Sofortwirkung (Preemption bei Prioritaetserhoehung) bleibt TODO.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt).
 */
int Q9K_ProcSPrior(Q9_u16 pid, Q9_u16 priority, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_ProcLookup(pid);

    *outError = 0;

    if (desc == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF, (Q9_u8)(priority & 0xFFU));
    return 1;
}

void Q9K_SysSPriorImpl(void)
{
    Q9_u16 pid      = Q9K_GetU16(Q9K_SPRIOR_SCRATCH_PID);
    Q9_u16 priority = Q9K_GetU16(Q9K_SPRIOR_SCRATCH_PRIORITY);
    Q9_u16 err      = 0;

    if (Q9K_ProcSPrior(pid, priority, &err)) {
        Q9K_SetU16(Q9K_SPRIOR_SCRATCH_SUCCESS, 1);
    } else {
        Q9K_SetU16(Q9K_SPRIOR_SCRATCH_ERROR, err);
        Q9K_SetU16(Q9K_SPRIOR_SCRATCH_SUCCESS, 0);
    }
}
