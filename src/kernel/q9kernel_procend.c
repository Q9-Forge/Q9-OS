/*
 * q9kernel_procend.c -- Q9-OS eigener Kernel: F$Exit/F$Wait (Abschnitt
 *                       "F$Exit/F$Wait", 2026-08-22, im Anschluss an
 *                       F$Fork).
 *
 * Reale Register-/Verhaltenskonvention ECHT per Read gelesen (nicht
 * geraten), 68k_tech.pdf:
 *
 *   F$Exit (S. 423-424, Callcode 0x06, s. modules/SYSCALL_MODULE_MAP.md):
 *     IN  d1.w = Statuscode fuer den Elternprozess
 *     OUT: Prozess ist beendet (kehrt NIE zurueck)
 *     Verhalten laut Manual (Aufzaehlung S. 424):
 *       - alle Pfade schliessen (kein Pfadsystem hier -- entfaellt)
 *       - Speicher an das System zurueckgeben (eigene Entscheidung:
 *         NICHT implementiert, s. u.)
 *       - primaeres Modul und User-Trap-Handler entlinken (User-Trap-
 *         Handler-Konzept existiert hier nicht -- nur das primaere
 *         Modul, per Q9K_ModDirUnlinkByHeader)
 *       - Deskriptor toter Kindprozesse freigeben (Q9K_ProcExitFreeDeadChildren)
 *       - falls der Elternprozess tot ist: Deskriptor sofort freigeben
 *         (eigene Entscheidung: NICHT implementiert, s. u. -- Elternprozess
 *         wird in dieser ersten Fassung immer als lebend angenommen)
 *       - hat der Elternprozess noch kein F$Wait ausgefuehrt: Prozess
 *         "in der Schwebe" lassen (= unser Zombie-Zustand, State='z')
 *       - wartet der Elternprozess bereits: ihn in die aktive Queue
 *         verschieben, Tod/Status mitteilen, aus der Kindliste entfernen,
 *         Deskriptorspeicher freigeben (Q9K_ProcExit, Zweig "reaktivieren")
 *
 *   F$Wait (S. 535-536, Callcode 0x04):
 *     IN: keine
 *     OUT (Erfolg): d0.w = ID des beendeten Kindprozesses,
 *                   d1.w = dessen Exit-Statuscode
 *     OUT (Fehlschlag): cc=Carry gesetzt, d1.w = Fehlercode
 *     Verhalten laut Manual:
 *       - blockiert den Aufrufer, bis EIN Kindprozess per F$Exit endet
 *       - starb ein Kind schon VOR dem F$Wait-Aufruf: sofortige
 *         Reaktivierung (kein Blockieren) -- unser "Zombie-Scan zuerst,
 *         dann erst blockieren"-Ablauf
 *       - Fehler NUR, wenn der Aufrufer UEBERHAUPT keine Kindprozesse hat
 *         (E$NoChld, MWOS/SRC/DEFS/errno.h: "#define E_NOCHLD 0xe2")
 *       - hat der Aufrufer MEHRERE Kinder, wird er beim ERSTEN sterbenden
 *         aktiviert -- ein F$Wait pro Kind noetig, um alle zu erfassen
 *         (passt exakt zu unserem Pool-Scan: findet IRGENDEIN Zombie-Kind,
 *         nicht ein bestimmtes)
 *
 * EIGENE ENTSCHEIDUNGEN, Manual-Verhalten bewusst NICHT vollstaendig
 * nachgebildet (dokumentiert, nicht verschwiegen -- gleiche Vorgehensweise
 * wie ueberall in diesem Kernel):
 *   - KEIN echtes Speicher-Zurueckgeben (F$SRtMem/Arena-Freigabe existiert
 *     hier nicht, s. bereits bestehendes TODO bei Q9K_AllocMem) -- ein
 *     beendeter Prozess gibt NUR seinen Deskriptor-Pool-Slot frei
 *     (Q9K_ProcPoolFree unten), NICHT den per Q9K_AllocMem belegten
 *     Daten-/Stack-Speicherblock. Fuer die geplanten Boot-Tests
 *     unproblematisch (wenige, kurzlebige Testprozesse).
 *   - "Falls der Elternprozess tot ist, Deskriptor sofort freigeben"
 *     NICHT implementiert -- der Elternprozess wird hier immer als lebend
 *     angenommen (State-Feld eines FREIEN Slots ist wegen der
 *     absichtlichen Kollision mit dem Freilisten-"naechster frei"-Zeiger
 *     bei Offset 0 NICHT zuverlaessig lesbar, s. Kopfkommentar
 *     q9kernel_firstproc.c "Slot-Offset 0 kollidiert absichtlich..." --
 *     ein sicherer Toter-Elternprozess-Test bräuchte ein eigenes,
 *     dediziertes "belegt"-Feld, hier bewusst (noch) nicht eingefuehrt,
 *     da kein Testszenario diesen Fall aktuell ausloest). TODO fuer
 *     spaeter, falls ein Elternprozess vor seinem Kind enden soll.
 *   - Signale (F$Send) existieren hier nicht -- der im Manual erwaehnte
 *     "d0.w=0 bei Signalaktivierung"-Fall entfaellt komplett.
 *
 * Pool-Scan statt echter Sibling-Liste (F$Wait "hat der Aufrufer
 * ueberhaupt Kinder?"/"gibt es ein Zombie-Kind?"): gleiche Begruendung
 * wie ueberall in diesem Kernel ("eigener Kernel, keine Kompat-Pflicht
 * fuer interne Strukturen") -- ein O(n)-Scan ueber den kompletten
 * Deskriptor-Pool (Q9K_PROCPOOL_BASE_ADDR/COUNT_ADDR, s. q9kernel_tables.c)
 * ist fuer die hier vorgesehenen Pool-Groessen (wenige Dutzend Prozesse)
 * voellig ausreichend. Funktioniert NUR korrekt, weil q9kernel_tables.c
 * seit dem NACHTRAG 2026-08-22 den gesamten Pool-Block VOR dem Aufbau der
 * Freiliste nullt UND Q9K_ProcPoolFree (unten) das ParentDesc-Feld beim
 * Freigeben explizit zuruecksetzt -- ein freier Slot hat dadurch
 * GARANTIERT ParentDesc==0, was ihn automatisch von jedem
 * "ParentDesc==gesuchter Deskriptor"-Treffer ausschliesst (gesuchte
 * Deskriptoradressen sind selbst nie 0).
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

extern void   Q9K_SchedInsert(Q9_u32 desc);        /* q9kernel_sched.c */
extern Q9_u32 Q9K_SchedFirstPick(void);             /* q9kernel_sched.c -- "naechsten Prozess waehlen,
                                                       * kein aktueller zum Wiedereinreihen", exakt das,
                                                       * was F$Exits erzwungener Wechsel braucht */
extern void   Q9K_WaitQRemove(Q9_u32 desc);         /* q9kernel_sched.c */
extern Q9_u32 Q9K_ModDirUnlinkByHeader(Q9_u32 hdrAddr); /* q9kernel_moddir.c */

/* Deskriptor-Feldoffsets -- lokal dupliziert, gleiche schlanke Konvention
 * wie ueberall in diesem Kernel (s. q9kernel_firstproc.c Kopfkommentar
 * fuer das vollstaendige Layout). Per #ifndef ueberschreibbar (Host-
 * Tests). */
#ifndef Q9K_PROCDESC_STATE_OFF
#define Q9K_PROCDESC_STATE_OFF      0x00UL
#endif
#ifndef Q9K_PROCDESC_PARENT_OFF
#define Q9K_PROCDESC_PARENT_OFF     0x04UL
#endif
#ifndef Q9K_PROCDESC_MODHDR_OFF
#define Q9K_PROCDESC_MODHDR_OFF     0x08UL
#endif
#ifndef Q9K_PROCDESC_EXITSTATUS_OFF
#define Q9K_PROCDESC_EXITSTATUS_OFF 0x0CUL
#endif
#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF    0x38UL
#endif

#define Q9K_PROCDESC_STATE_ACTIVE  'a'
#define Q9K_PROCDESC_STATE_ZOMBIE  'z'
#define Q9K_PROCDESC_STATE_WAITING 'w'

#ifndef Q9K_PROCDESC_SIZE
#define Q9K_PROCDESC_SIZE 0x200UL  /* deckt P$Path bis 0x1A8, s. q9kernel_tables.c */
#endif
#ifndef Q9K_PROCPOOL_BASE_ADDR
#define Q9K_PROCPOOL_BASE_ADDR 0x1204UL
#endif
#ifndef Q9K_PROCPOOL_FREE_ADDR
#define Q9K_PROCPOOL_FREE_ADDR 0x120CUL
#endif
#ifndef Q9K_PROCPOOL_COUNT_ADDR
#define Q9K_PROCPOOL_COUNT_ADDR 0x1220UL
#endif

/* Q9_D_PROC -- echtes Kernel-Global (q9sysglob.h), Zeiger auf den
 * AKTUELLEN Prozessdeskriptor. KEIN gelinktes Symbol -- wie ueberall in
 * diesem Kernel eine literale Adresskonstante, per #ifndef ueberschreibbar
 * (s. q9kernel_sched.c/q9kernel_firstproc.c, gleiche Definition dort
 * lokal dupliziert). */
#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Real, MWOS/SRC/DEFS/errno.h: "#define E_NOCHLD 0xe2" -- gleiche
 * Primaerquelle wie schon fuer E_MNF/E_MEMFUL/E_PRCFUL/E_UNKSVC an
 * anderer Stelle in diesem Kernel verwendet. */
#define Q9K_E_NOCHLD 0x00E2U

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }

/* Schreibt value in Register regIndex des 60-Byte-Registersatz-Bereichs
 * ab frameBase (0-7 = D0-D7, 8-14 = A0-A6) -- LOKALE Kopie von
 * Q9K_SetFrameReg (q9kernel_firstproc.c), byteweise statt Pointer-Cast
 * aus demselben, dort ausfuehrlich dokumentierten Grund (Host-/Ziel-
 * Breitenunabhaengigkeit). Wird hier nur fuer die beiden Ausgaberegister
 * (D0=Kind-PID, D1=Exit-Status) im SavedSP-Rahmen des reaktivierten
 * Elternprozesses gebraucht. */
static void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex, Q9_u32 value)
{
    Q9_u32 addr = frameBase + regIndex * 4UL;

    Q9K_SetU8(addr + 0, (Q9_u8)(value >> 24));
    Q9K_SetU8(addr + 1, (Q9_u8)(value >> 16));
    Q9K_SetU8(addr + 2, (Q9_u8)(value >> 8));
    Q9K_SetU8(addr + 3, (Q9_u8)value);
}

/* Gibt einen Deskriptor-Pool-Slot zurueck in die Freiliste (Gegenstueck
 * zu Q9K_ProcPoolAlloc, q9kernel_firstproc.c -- dort static, deshalb hier
 * eine eigene, aber identische Umkehr-Implementierung statt eines
 * Cross-File-Zugriffs auf eine static-Funktion). WICHTIG: setzt
 * ParentDesc/ModuleHdr/ExitStatus explizit auf 0 zurueck, BEVOR der
 * Freilisten-Zeiger Offset 0 ueberschreibt -- erhaelt damit die im
 * Kopfkommentar beschriebene Invariante ("freier Slot hat immer
 * ParentDesc==0"), auf der der Pool-Scan unten beruht. */
static void Q9K_ProcPoolFree(Q9_u32 desc)
{
    Q9K_SetU32(desc + Q9K_PROCDESC_PARENT_OFF, 0);
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF, 0);
    Q9K_SetU16(desc + Q9K_PROCDESC_EXITSTATUS_OFF, 0);

    Q9K_SetU32(desc, Q9K_GetU32(Q9K_PROCPOOL_FREE_ADDR));
    Q9K_SetU32(Q9K_PROCPOOL_FREE_ADDR, desc);
}

/* "Free process descriptor of any dead child processes" (68k_tech.pdf
 * S. 424, vierter Aufzaehlungspunkt) -- eigene Zombie-Kinder von
 * callerDesc, auf die niemand mehr je per F$Wait warten kann (callerDesc
 * selbst verschwindet gleich), sofort freigeben statt sie als
 * unerreichbaren Zombie im Pool liegen zu lassen. */
static void Q9K_ProcExitFreeDeadChildren(Q9_u32 callerDesc)
{
    Q9_u32 base  = Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR);
    Q9_u32 count = Q9K_GetU32(Q9K_PROCPOOL_COUNT_ADDR);
    Q9_u32 i;

    for (i = 0; i < count; i++) {
        Q9_u32 slot = base + i * Q9K_PROCDESC_SIZE;

        if (Q9K_GetU32(slot + Q9K_PROCDESC_PARENT_OFF) == callerDesc
            && Q9K_GetU8(slot + Q9K_PROCDESC_STATE_OFF) == Q9K_PROCDESC_STATE_ZOMBIE)
            Q9K_ProcPoolFree(slot);
    }
}

/* Q9K_ProcExit -- echte F$Exit-Kernlogik (s. Kopfkommentar). Rueckgabe:
 * Deskriptoradresse des als naechstes zu startenden Prozesses, oder 0
 * falls keiner mehr bereit ist (Aufrufer -- Q9K_SysFExit, q9kernel_entry.a
 * -- faellt dann in Q9K_HaltLoop, kein Fake-Fortschritt). */
Q9_u32 Q9K_ProcExit(Q9_u32 callerDesc, Q9_u16 exitStatus)
{
    Q9_u32 modHdr     = Q9K_GetU32(callerDesc + Q9K_PROCDESC_MODHDR_OFF);
    Q9_u32 parentDesc = Q9K_GetU32(callerDesc + Q9K_PROCDESC_PARENT_OFF);

    if (modHdr != 0) {
        Q9K_ModDirUnlinkByHeader(modHdr);
        Q9K_SetU32(callerDesc + Q9K_PROCDESC_MODHDR_OFF, 0);
    }

    Q9K_ProcExitFreeDeadChildren(callerDesc);

    if (parentDesc != 0 && Q9K_GetU8(parentDesc + Q9K_PROCDESC_STATE_OFF) == Q9K_PROCDESC_STATE_WAITING) {
        /* Elternprozess wartet bereits (State=='w', s. Q9K_ProcWaitTryReap
         * Blockierpfad in Q9K_SysFWait) -- sofort reaktivieren: aus der
         * Wait-Queue entfernen, Kind-PID+Exit-Status in dessen
         * gespeicherten Registersatz eintragen (exakt die Stellen, die
         * beim Wiederaufwachen per "movem.l (sp)+,d0-d7/a0-a6" als D0/D1
         * geladen werden -- der reaktivierte Elternprozess "sieht" damit
         * genau die reale F$Wait-Erfolgs-Rueckgabe, ganz ohne
         * Sonderbehandlung beim Aufwachen selbst), State zurueck auf
         * aktiv, in die Ready-Queue einreihen. Kind-Deskriptor ist damit
         * sofort abgeholt -- kein Zombie-Zwischenzustand noetig. */
        Q9_u32 childPid = (callerDesc - Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR)) / Q9K_PROCDESC_SIZE + 1;
        Q9_u32 parentSP = Q9K_GetU32(parentDesc + Q9K_PROCDESC_SAVEDSP_OFF);

        Q9K_WaitQRemove(parentDesc);
        Q9K_SetFrameReg(parentSP, 0, childPid);
        Q9K_SetFrameReg(parentSP, 1, (Q9_u32)exitStatus);
        Q9K_SetU8(parentDesc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ACTIVE);
        Q9K_SchedInsert(parentDesc);

        Q9K_ProcPoolFree(callerDesc);
    } else {
        /* Elternprozess hat noch kein F$Wait ausgefuehrt (oder es gibt
         * keinen, s. Kopfkommentar "Elternprozess tot" -- TODO) -- Kind
         * "in der Schwebe" lassen: Zombie, Deskriptor bleibt belegt, bis
         * eine spaetere F$Wait-Pool-Suche ihn findet. */
        Q9K_SetU8(callerDesc + Q9K_PROCDESC_STATE_OFF, Q9K_PROCDESC_STATE_ZOMBIE);
        Q9K_SetU16(callerDesc + Q9K_PROCDESC_EXITSTATUS_OFF, exitStatus);
    }

    /* Aufrufer laeuft nie weiter (F$Exit kehrt laut Manual nie zurueck)
     * -- erzwungener Wechsel, kein "aktueller" Prozess zum
     * Wiedereinreihen (anders als Q9K_SchedReschedule), deshalb
     * Q9K_SchedFirstPick statt Q9K_SchedReschedule, exakt wie beim
     * allerersten Prozessstart (Q9K_SchedRun). */
    return Q9K_SchedFirstPick();
}

/* Q9K_ProcWaitTryReap -- echte F$Wait-Kernlogik, EIN Versuch (s.
 * Kopfkommentar). Durchsucht den kompletten Deskriptor-Pool nach einem
 * Kind von callerDesc:
 *   - findet ein Zombie-Kind: sofort abholen (Ergebnis in *outChildPid/
 *     *outExitStatus, Deskriptor freigeben), Rueckgabe 1.
 *   - findet KEIN Kind ueberhaupt (weder lebend noch Zombie): *outError =
 *     E_NOCHLD, Rueckgabe 0 (der Aufrufer -- Q9K_SysWaitImpl -- meldet das
 *     als echten Fehler zurueck).
 *   - findet mindestens ein LEBENDES, aber KEIN Zombie-Kind: Rueckgabe 0,
 *     *outError bleibt UNVERAENDERT (Aufrufer-Konvention: 0 = "muss
 *     blockieren", der Aufrufer initialisiert *outError vorher auf 0). */
int Q9K_ProcWaitTryReap(Q9_u32 callerDesc, Q9_u32 *outChildPid, Q9_u16 *outExitStatus, Q9_u16 *outError)
{
    Q9_u32 base  = Q9K_GetU32(Q9K_PROCPOOL_BASE_ADDR);
    Q9_u32 count = Q9K_GetU32(Q9K_PROCPOOL_COUNT_ADDR);
    Q9_u32 i;
    int hasAnyChild = 0;

    for (i = 0; i < count; i++) {
        Q9_u32 slot = base + i * Q9K_PROCDESC_SIZE;

        if (Q9K_GetU32(slot + Q9K_PROCDESC_PARENT_OFF) != callerDesc)
            continue;

        hasAnyChild = 1;

        if (Q9K_GetU8(slot + Q9K_PROCDESC_STATE_OFF) == Q9K_PROCDESC_STATE_ZOMBIE) {
            *outChildPid   = (slot - base) / Q9K_PROCDESC_SIZE + 1;
            *outExitStatus = Q9K_GetU16(slot + Q9K_PROCDESC_EXITSTATUS_OFF);
            Q9K_ProcPoolFree(slot);
            return 1;
        }
    }

    if (!hasAnyChild)
        *outError = (Q9_u16)Q9K_E_NOCHLD;

    return 0;
}

/* Eigene Kernel-Global-Erweiterungen fuer die ASM<->C-Uebergabe von
 * Q9K_SysFExit/Q9K_SysFWait (q9kernel_entry.a) -- gleiches, bereits
 * etabliertes Muster wie Q9K_FORK_SCRATCH_* (q9kernel_firstproc.c).
 * Direkt hinter Q9K_WAITQ_SENTINEL_ADDR ($12A0, q9kernel_sched.c) + dessen
 * 0x38 Byte eigenem Next/Prev-Bereich -- naechste freie Adresse $12D8. */
#ifndef Q9K_WAIT_SCRATCH_CHILDPID
#define Q9K_WAIT_SCRATCH_CHILDPID   0x12D8UL   /* Q9_u16, d0.w AUS (Erfolg) */
#endif
#ifndef Q9K_WAIT_SCRATCH_EXITSTATUS
#define Q9K_WAIT_SCRATCH_EXITSTATUS 0x12DCUL   /* Q9_u16, d1.w AUS (Erfolg) */
#endif
#ifndef Q9K_WAIT_SCRATCH_ERRORCODE
#define Q9K_WAIT_SCRATCH_ERRORCODE  0x12E0UL   /* Q9_u16, d1.w AUS (E_NOCHLD) */
#endif
#ifndef Q9K_WAIT_SCRATCH_OUTCOME
#define Q9K_WAIT_SCRATCH_OUTCOME    0x12E4UL   /* Q9_u16, 0=blockieren/1=sofort erfolgreich/2=E_NOCHLD */
#endif
#ifndef Q9K_EXIT_SCRATCH_STATUS
#define Q9K_EXIT_SCRATCH_STATUS     0x12E8UL   /* Q9_u16, d1.w EIN */
#endif
#ifndef Q9K_EXIT_SCRATCH_NEXT
#define Q9K_EXIT_SCRATCH_NEXT       0x12ECUL   /* Q9_u32, naechster Deskriptor AUS */
#endif

/* Q9K_SysWaitImpl -- duenne, PARAMETERLOSE Bruecke zwischen dem
 * Assembler-Trampolin Q9K_SysFWait (q9kernel_entry.a) und
 * Q9K_ProcWaitTryReap (echte Mehrparameter-C-Funktion, s. oben) --
 * gleiches Muster/gleiche Begruendung wie Q9K_SysForkImpl
 * (q9kernel_firstproc.c): reiner C-zu-C-Aufruf hier (kein Risiko), die
 * unverifizierte Assembler<->C-Mehrparameter-Grenze wird stattdessen
 * ueber die obigen Scratch-Adressen umgangen. F$Wait hat KEINE echten
 * Eingaberegister (s. Kopfkommentar) -- liest nur Q9_D_PROC selbst. */
void Q9K_SysWaitImpl(void)
{
    Q9_u32 callerDesc = Q9K_GetU32(Q9_D_PROC);
    Q9_u32 childPid = 0;
    Q9_u16 exitStatus = 0;
    Q9_u16 error = 0;

    if (Q9K_ProcWaitTryReap(callerDesc, &childPid, &exitStatus, &error)) {
        Q9K_SetU16(Q9K_WAIT_SCRATCH_CHILDPID, (Q9_u16)childPid);
        Q9K_SetU16(Q9K_WAIT_SCRATCH_EXITSTATUS, exitStatus);
        Q9K_SetU16(Q9K_WAIT_SCRATCH_OUTCOME, 1);
    } else if (error != 0) {
        Q9K_SetU16(Q9K_WAIT_SCRATCH_ERRORCODE, error);
        Q9K_SetU16(Q9K_WAIT_SCRATCH_OUTCOME, 2);
    } else {
        Q9K_SetU16(Q9K_WAIT_SCRATCH_OUTCOME, 0);   /* Aufrufer (Trampolin) muss blockieren */
    }
}

/* Q9K_SysExitImpl -- gleiches Bruecken-Muster wie Q9K_SysWaitImpl oben,
 * fuer Q9K_ProcExit. */
void Q9K_SysExitImpl(void)
{
    Q9_u32 callerDesc = Q9K_GetU32(Q9_D_PROC);
    Q9_u16 exitStatus = Q9K_GetU16(Q9K_EXIT_SCRATCH_STATUS);
    Q9_u32 next = Q9K_ProcExit(callerDesc, exitStatus);

    Q9K_SetU32(Q9K_EXIT_SCRATCH_NEXT, next);
}
