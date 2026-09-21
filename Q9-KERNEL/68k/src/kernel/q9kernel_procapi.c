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

/* Aus q9kernel_sched.c -- externe Deklaration statt gemeinsamem Header,
 * gleiche schlanke Konvention wie ueberall in diesem Verzeichnis. */
extern void Q9K_SchedInsert(unsigned long desc);
extern void Q9K_SchedSetPriority(unsigned long desc, unsigned short priority);

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
#ifndef Q9K_PROCDESC_USER_OFF
#define Q9K_PROCDESC_USER_OFF 0x14UL  /* P$User, s. q9kernel_firstproc.c */
#endif
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF 0x08UL /* P$sp, s. q9kernel_firstproc.c */
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
#define Q9K_E_PERMIT 0x00A4U /* errno.h: EOS_PERMIT, "must be super user" */

/* F$AProc-/F$GPrDBT-Scratch (2026-09-18), hinter dem F$Julian-Block
 * ($1930-$193C, q9kernel_date.c). */
#ifndef Q9K_APROC_SCRATCH_DESC
#define Q9K_APROC_SCRATCH_DESC     0x1940UL /* Q9_u32, (a0) EIN            */
#define Q9K_APROC_SCRATCH_ERROR    0x1944UL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_APROC_SCRATCH_SUCCESS  0x1948UL /* Q9_u32, 0/1                 */
#endif
#ifndef Q9K_APROC_SCRATCH_PREEMPT
#define Q9K_APROC_SCRATCH_PREEMPT  0x196CUL /* Q9_u32, 1 = sofort wechseln */
#endif
#ifndef Q9K_GPRDBT_SCRATCH_BUF
#define Q9K_GPRDBT_SCRATCH_BUF     0x194CUL /* Q9_u32, (a0) EIN            */
#define Q9K_GPRDBT_SCRATCH_COUNT   0x1950UL /* Q9_u32, d1.l EIN/AUS        */
#endif

/* F$GPrDsc-Scratch (2026-09-17), hinter dem F$GModDr-Block
 * ($1910-$1914, q9kernel_moddir.c). */
#ifndef Q9K_GPRDSC_SCRATCH_PID
#define Q9K_GPRDSC_SCRATCH_PID     0x1918UL /* Q9_u32, d0.w EIN            */
#endif
#ifndef Q9K_GPRDSC_SCRATCH_COUNT
#define Q9K_GPRDSC_SCRATCH_COUNT   0x191CUL /* Q9_u32, d1.w EIN            */
#endif
#ifndef Q9K_GPRDSC_SCRATCH_BUF
#define Q9K_GPRDSC_SCRATCH_BUF     0x1920UL /* Q9_u32, (a0) EIN            */
#endif
#ifndef Q9K_GPRDSC_SCRATCH_ERROR
#define Q9K_GPRDSC_SCRATCH_ERROR   0x1924UL /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_GPRDSC_SCRATCH_SUCCESS
#define Q9K_GPRDSC_SCRATCH_SUCCESS 0x1928UL /* Q9_u32, 0/1                 */
#endif

/* F$SUser-Scratch (2026-09-17): hinter dem F$CmpNam-Block
 * ($18B0-$18C0, q9kernel_iopath.c). */
#ifndef Q9K_SUSER_SCRATCH_GROUPUSER
#define Q9K_SUSER_SCRATCH_GROUPUSER 0x18C4UL /* Q9_u32, d1.l EIN */
#endif
#ifndef Q9K_SUSER_SCRATCH_ERROR
#define Q9K_SUSER_SCRATCH_ERROR     0x18C8UL /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_SUSER_SCRATCH_SUCCESS
#define Q9K_SUSER_SCRATCH_SUCCESS   0x18CCUL /* Q9_u32, 0/1 */
#endif

/* F$CpyMem-Scratch (2026-09-17), direkt hinter F$SUser. */
#ifndef Q9K_CPYMEM_SCRATCH_PID
#define Q9K_CPYMEM_SCRATCH_PID     0x18D0UL /* Q9_u32, d0.w EIN */
#endif
#ifndef Q9K_CPYMEM_SCRATCH_COUNT
#define Q9K_CPYMEM_SCRATCH_COUNT   0x18D4UL /* Q9_u32, d1.l EIN */
#endif
#ifndef Q9K_CPYMEM_SCRATCH_SRC
#define Q9K_CPYMEM_SCRATCH_SRC     0x18D8UL /* Q9_u32, (a0) EIN */
#endif
#ifndef Q9K_CPYMEM_SCRATCH_DST
#define Q9K_CPYMEM_SCRATCH_DST     0x18DCUL /* Q9_u32, (a1) EIN */
#endif
#ifndef Q9K_CPYMEM_SCRATCH_ERROR
#define Q9K_CPYMEM_SCRATCH_ERROR   0x18E0UL /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_CPYMEM_SCRATCH_SUCCESS
#define Q9K_CPYMEM_SCRATCH_SUCCESS 0x18E4UL /* Q9_u32, 0/1 */
#endif

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
    /* Seit F$SUser (2026-09-17) ein echtes Deskriptorfeld statt einer
     * festen 0: P$User wird bei der Prozesserzeugung auf 0.0 gesetzt,
     * von F$Fork vererbt und nur von F$SUser veraendert. */
    Q9K_SetU32(Q9K_ID_SCRATCH_GROUPUSER,
               Q9K_GetU32(desc + Q9K_PROCDESC_USER_OFF));
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
 * Ein wartender Prozess in der Ready-Queue bekommt sein Age sofort auf die
 * neue Prioritaet gesetzt. Der laufende Prozess bleibt bis zum Ende seiner
 * aktuellen Zeitscheibe aktiv; eine Repositionierung mitten im Syscall waere
 * ein unsicherer Kontextwechsel.
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

    Q9K_SchedSetPriority(desc, priority);
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

/* Q9K_ProcSUser -- echte F$SUser-Kernlogik (Callcode $1C, "Set User ID
 * Number"). Verifizierte ABI (68k_tech.pdf S. 516): d1.l = gewuenschte
 * Gruppen-/Benutzer-ID, keine Ausgabe bei Erfolg, sonst Carry +
 * d1.w = E$Permit ($A4).
 *
 * Die reale Beschreibung nennt drei Faelle, in denen der Wechsel erlaubt
 * ist: Benutzer 0.0 darf beliebig wechseln; ein Primaermodul im Besitz
 * von 0.0 darf beliebig wechseln; jedes Primaermodul darf auf die ID
 * SEINES EIGENEN Modulbesitzers wechseln. Die beiden Modulfaelle
 * brauchen den Besitzereintrag aus dem Modulkopf des laufenden
 * Primaermoduls -- ein Feld, das dieser Kernel im Prozessdeskriptor noch
 * nicht fuehrt. Umgesetzt ist deshalb NUR der erste Fall (0.0 darf
 * alles), alles andere meldet sauber E$Permit statt einen der beiden
 * anderen Faelle vorzutaeuschen. Praktisch aendert das heute nichts:
 * jeder Prozess startet als 0.0 und erbt das bei F$Fork -- erst ein
 * Prozess, der sich selbst heruntergestuft hat, sieht die Grenze.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt).
 */
int Q9K_ProcSUser(Q9_u32 desc, Q9_u32 groupUser, Q9_u16 *outError)
{
    *outError = 0;

    if (desc == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    if (Q9K_GetU32(desc + Q9K_PROCDESC_USER_OFF) != 0UL) {
        *outError = Q9K_E_PERMIT;
        return 0;
    }

    Q9K_SetU32(desc + Q9K_PROCDESC_USER_OFF, groupUser);
    return 1;
}

void Q9K_SysSUserImpl(void)
{
    Q9_u16 err = 0;

    /* Diese beiden Zellen sind 32 Bit breit und werden vom Assembler mit
     * "+2" (unteres Wort) gelesen -- deshalb SetU32, nicht SetU16.  Ein
     * SetU16 wuerde das OBERE Wort treffen, die Bruecke saehe immer 0
     * und meldete jeden Aufruf als Fehlschlag (live gefunden). */
    if (Q9K_ProcSUser(Q9K_GetU32(Q9_D_PROC),
                      Q9K_GetU32(Q9K_SUSER_SCRATCH_GROUPUSER),
                      &err)) {
        Q9K_SetU32(Q9K_SUSER_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_SUSER_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_SUSER_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_ProcCpyMem -- echte F$CpyMem-Kernlogik (Callcode $1B, "Copy
 * External Memory"). Verifizierte ABI (68k_tech.pdf S. 389): d0.w = PID
 * des Besitzers des fremden Speichers, d1.l = Anzahl Bytes, (a0) =
 * Quelladresse IM FREMDEN Prozess, (a1) = eigener Zielpuffer. Keine
 * Ausgabe bei Erfolg.
 *
 * Was dieser Aufruf im realen OS-9 leistet, ist die Uebersetzung einer
 * Adresse aus einem FREMDEN Adressraum. Dieser Kernel laeuft ohne
 * Adressraumtrennung -- alle Prozesse sehen denselben flachen Speicher
 * (kein SSM, keine MMU-Tabellen, s. F$Permit/F$Protect in STATUS.md).
 * Die Uebersetzung entfaellt hier deshalb ersatzlos; echte Semantik hat
 * nur die Pruefung, ob die angegebene PID ueberhaupt einen lebenden
 * Prozess bezeichnet. Das ist bewusst so und keine Attrappe: sobald
 * dieser Kernel Adressraeume trennt, gehoert die Uebersetzung GENAU
 * hierher, und alle Aufrufer funktionieren unveraendert weiter.
 *
 * Kopiert wird vorwaerts und ohne Ueberlappungsbehandlung -- Quelle und
 * Ziel sind laut Beschreibung fremder Speicher und eigener Puffer, also
 * getrennt (anders als bei F$Move, das ausdruecklich verschiebt).
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt).
 */
int Q9K_ProcCpyMem(Q9_u16 pid, Q9_u32 count, Q9_u32 srcAddr, Q9_u32 dstAddr,
                   Q9_u16 *outError)
{
    volatile unsigned char       *dst;
    const volatile unsigned char *src;
    Q9_u32 i;

    *outError = 0;

    if (Q9K_ProcLookup(pid) == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    if (count == 0)
        return 1;

    if (srcAddr == 0 || dstAddr == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    src = (const volatile unsigned char *)srcAddr;
    dst = (volatile unsigned char *)dstAddr;
    for (i = 0; i < count; ++i)
        dst[i] = src[i];

    return 1;
}

void Q9K_SysCpyMemImpl(void)
{
    Q9_u16 err = 0;

    if (Q9K_ProcCpyMem((Q9_u16)Q9K_GetU32(Q9K_CPYMEM_SCRATCH_PID),
                       Q9K_GetU32(Q9K_CPYMEM_SCRATCH_COUNT),
                       Q9K_GetU32(Q9K_CPYMEM_SCRATCH_SRC),
                       Q9K_GetU32(Q9K_CPYMEM_SCRATCH_DST),
                       &err)) {
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_SUCCESS, 1UL); /* 32 Bit, s. Q9K_SysSUserImpl */
    } else {
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_CPYMEM_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_ProcGPrDsc -- echte F$GPrDsc-Kernlogik (Callcode $18, "Get Copy of
 * Process Descriptor"). Verifizierte ABI (68k_tech.pdf S. 440f):
 * d0.w = angeforderte PID, d1.w = Anzahl zu kopierender Bytes,
 * (a0) = Zielpuffer. Keine Ausgabe; E$IPrcID ($E0) bei ungueltiger PID.
 *
 * Ausdruecklich NUR lesend -- das Handbuch stellt klar, dass es keinen
 * Weg gibt, ueber diesen Aufruf einen Deskriptor zu VERAENDERN. Mehr als
 * die Deskriptorgroesse wird nie kopiert, auch wenn der Aufrufer mehr
 * anfordert: alles dahinter gehoert bereits dem naechsten Pool-Slot und
 * waere fremder Inhalt.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcGPrDsc(Q9_u16 pid, Q9_u32 count, Q9_u32 bufAddr, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_ProcLookup(pid);
    const volatile Q9_u8 *src;
    volatile Q9_u8 *dst;
    Q9_u32 i;

    *outError = 0;

    if (desc == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }
    if (bufAddr == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    if (count > Q9K_PROCDESC_SIZE)
        count = Q9K_PROCDESC_SIZE;

    src = (const volatile Q9_u8 *)desc;
    dst = (volatile Q9_u8 *)bufAddr;
    for (i = 0; i < count; ++i)
        dst[i] = src[i];

    return 1;
}

void Q9K_SysGPrDscImpl(void)
{
    Q9_u16 err = 0;

    if (Q9K_ProcGPrDsc((Q9_u16)Q9K_GetU32(Q9K_GPRDSC_SCRATCH_PID),
                       Q9K_GetU32(Q9K_GPRDSC_SCRATCH_COUNT),
                       Q9K_GetU32(Q9K_GPRDSC_SCRATCH_BUF),
                       &err)) {
        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_GPRDSC_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_ProcAProc -- echte F$AProc-Kernlogik (Callcode $2C, "Enter Process
 * in Active Process Queue"). Verifizierte ABI (68k_tech.pdf S. 377):
 * (a0) = Prozessdeskriptor, keine Ausgabe; Carry + d1.w bei Fehler.
 * Systemzustand.
 *
 * Die Beschreibung nennt drei Wirkungen: alle bereits wartenden Prozesse
 * altern, das Alter des uebergebenen Prozesses wird auf seine Prioritaet
 * gesetzt, und er wird nach seinem relativen Alter eingereiht. Genau das
 * ist Q9K_SchedInsert (q9kernel_sched.c) -- Age=Prioritaet plus Einhaengen
 * in die Ready-Queue; das Altern der uebrigen erledigt der Scheduler
 * ohnehin bei jedem Tick (Q9K_SchedAgeAll in Q9K_SchedReschedule). Dieser
 * Call fuehrt deshalb bewusst keine zweite, eigene Alterungsrunde aus:
 * das waere doppelte Buchfuehrung auf denselben Feldern.
 *
 * Bei einer hoeheren Prioritaet markiert die Bridge den Deskriptor fuer die
 * sofortige Trap-Kontext-Umschaltung; der Assembler sichert den laufenden
 * Rahmen und commit-t den Wechsel nach der Rueckkehr aus C.
 *
 * Rueckgabe 1 = Erfolg, 0 = Fehlschlag (*outError gesetzt). */
int Q9K_ProcAProc(Q9_u32 desc, Q9_u16 *outError)
{
    *outError = 0;

    /* Nur ein Deskriptor, der wirklich zu einem belegten Pool-Slot
     * gehoert, darf in die Ready-Queue -- sonst verkettet ein falscher
     * Zeiger die Liste in den freien Speicher hinein. */
    if (desc == 0 || Q9K_ProcIdForDesc(desc) == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    /* Und er muss AUSFUEHRBAR sein. Der Scheduler holt sich beim
     * Umschalten den gesicherten Stackzeiger aus dem Deskriptor und
     * kehrt per RTE auf den dort liegenden Rahmen zurueck
     * (Q9K_SchedRun/Q9K_TimerIRQHandler, q9kernel_entry.a). Ein
     * Deskriptor ohne gesicherten Stack -- etwa ein frisch von F$AllPrc
     * geholter, noch vollstaendig genullter -- laesst den Scheduler
     * dadurch auf Adresse 0 umschalten und ein RTE auf einem leeren
     * Rahmen ausfuehren: Format Error (Vektor 14), und zwar erst beim
     * naechsten Zeitscheibenwechsel, also weit entfernt von der
     * Ursache. LIVE ERLEBT (2026-09-18) genau so, als der Emulatortest
     * einen frischen F$AllPrc-Deskriptor an F$AProc weiterreichte.
     * Ein Prozess wird ausfuehrbar, indem F$Fork/Q9K_ProcCreate ihm
     * einen Stack samt Rahmen aufbauen -- erst danach gehoert er in die
     * Ready-Queue. */
    if (Q9K_GetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF) == 0) {
        *outError = Q9K_E_PRCID;
        return 0;
    }

    Q9K_SchedInsert(desc);
    return 1;
}

void Q9K_SysAProcImpl(void)
{
    Q9_u16 err = 0;
    Q9_u32 desc = Q9K_GetU32(Q9K_APROC_SCRATCH_DESC);

    Q9K_SetU32(Q9K_APROC_SCRATCH_PREEMPT, 0UL);
    if (Q9K_ProcAProc(desc, &err)) {
        Q9_u32 current = Q9K_GetU32(Q9_D_PROC);
        if (current != 0 && current != desc &&
            Q9K_GetU8(desc + Q9K_PROCDESC_PRIORITY_OFF) >
            Q9K_GetU8(current + Q9K_PROCDESC_PRIORITY_OFF))
            Q9K_SetU32(Q9K_APROC_SCRATCH_PREEMPT, 1UL);
        Q9K_SetU32(Q9K_APROC_SCRATCH_SUCCESS, 1UL);
    } else {
        Q9K_SetU32(Q9K_APROC_SCRATCH_ERROR, (Q9_u32)err);
        Q9K_SetU32(Q9K_APROC_SCRATCH_SUCCESS, 0UL);
    }
}

/* Q9K_ProcGPrDBT -- echte F$GPrDBT-Kernlogik (Callcode $1F, "Get Copy of
 * Process Descriptor Block Table"). Verifizierte ABI (68k_tech.pdf
 * S. 439): d1.l = hoechstens zu kopierende Bytes, (a0) = Puffer;
 * AUS: d1.l = tatsaechlich kopierte Bytes.
 *
 * Die Tabelle ist ein Feld von Zeigern auf die Prozessdeskriptoren --
 * das reale Format, an dem sich procs orientiert. Dieser Kernel fuehrt
 * keine solche Tabelle als eigene Struktur: der Prozesspool IST das
 * Verzeichnis (fortlaufende Slots ab Q9K_PROCPOOL_BASE_ADDR). Die
 * Tabelle wird deshalb beim Aufruf aus dem Pool zusammengestellt -- ein
 * Eintrag je Slot, 0 fuer einen freien. Damit bleibt die Zusage der
 * Beschreibung erhalten (Index = Prozessnummer, Eintrag = Deskriptor
 * oder leer), ohne eine zweite, parallel zu pflegende Datenstruktur
 * einzufuehren, die mit dem Pool auseinanderlaufen koennte.
 *
 * Abgeschnitten wird auf ganze Eintraege. Rueckgabe: kopierte Bytes. */
Q9_u32 Q9K_ProcGPrDBT(Q9_u32 bufAddr, Q9_u32 maxBytes)
{
    Q9_u32 base  = Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR);
    Q9_u32 count = Q9K_GetU32(Q9K_PROCPOOL_COUNT_ADDR);
    Q9_u32 written = 0;
    Q9_u32 i;

    if (bufAddr == 0 || base == 0)
        return 0;

    for (i = 0; i < count && written + 4UL <= maxBytes; ++i) {
        Q9_u32 desc = base + (i << Q9K_PROCDESC_SHIFT);
        Q9_u32 entry = Q9K_ProcIsAllocated(desc) ? desc : 0UL;
        volatile Q9_u8 *dst = (volatile Q9_u8 *)(bufAddr + written);

        /* Byteweise und ausdruecklich Big-Endian: der Puffer gehoert dem
         * Aufrufer und muss die echte 68k-Zeigerbreite von 4 Byte
         * tragen, unabhaengig davon, wie breit Q9_u32 auf dem jeweiligen
         * Uebersetzungsziel ist (auf dem Testhost 8 Byte). */
        dst[0] = (Q9_u8)((entry >> 24) & 0xFFUL);
        dst[1] = (Q9_u8)((entry >> 16) & 0xFFUL);
        dst[2] = (Q9_u8)((entry >> 8) & 0xFFUL);
        dst[3] = (Q9_u8)(entry & 0xFFUL);
        written += 4UL;
    }

    return written;
}

void Q9K_SysGPrDBTImpl(void)
{
    Q9K_SetU32(Q9K_GPRDBT_SCRATCH_COUNT,
               Q9K_ProcGPrDBT(Q9K_GetU32(Q9K_GPRDBT_SCRATCH_BUF),
                              Q9K_GetU32(Q9K_GPRDBT_SCRATCH_COUNT)));
}
