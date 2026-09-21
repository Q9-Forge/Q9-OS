/*
 * q9kernel_chain.c -- Q9-OS eigener Kernel: F$Chain (Callcode $05,
 *                     2026-09-18).
 *
 * Verifizierte ABI (68k_tech.pdf S.383):
 *
 *   EIN  d0.w = gewuenschter Modultyp/Sprache (Programm/Objekt oder 0)
 *        d1.l = zusaetzlicher Speicher
 *        d2.l = Parametergroesse
 *        d3.w = Zahl der zu uebernehmenden E/A-Pfade
 *        d4.w = Prioritaet
 *        (a0) = Modulname, (a1) = Parameter
 *   AUS  keine -- "F$Chain does not return to the calling process".
 *   Fehler: Carry und d1.w; der Aufrufer laeuft dann UNVERAENDERT weiter.
 *
 * F$Chain fuehrt ein neues Programm aus, ohne einen Prozess zu erzeugen --
 * "similar to a Fork command followed by an Exit", aber im selben Prozess.
 * Das Handbuch nennt vier Schritte:
 *
 *   * das alte Primaermodul unlinken,
 *   * das neue suchen (erst im Modulverzeichnis, sonst als Datei laden),
 *   * den Datenbereich auf die Groesse aus dem neuen Modulkopf bringen,
 *   * Intercepts und anstehende Signale loeschen.
 *
 * Offene Pfade bleiben ausdruecklich erhalten ("Open paths are not closed
 * or otherwise affected") -- das ist der Sinn der Sache: ein Programm
 * kann ein anderes an seiner Stelle weiterlaufen lassen, mit denselben
 * Ein- und Ausgaben. Die Registeruebergabe an das neue Programm ist
 * dieselbe wie bei F$Fork (Figure D-2: "these are identical to F$Fork").
 *
 * ZWEI DINGE, DIE DIE REIHENFOLGE DIKTIEREN -- beide sind der Grund,
 * warum diese Datei so aussieht, wie sie aussieht:
 *
 * 1. DER FEHLERFALL MUSS DEN AUFRUFER HEIL LASSEN. Findet sich das neue
 *    Modul nicht oder reicht der Speicher nicht, laeuft der alte Prozess
 *    weiter und bekommt nur ein Carry. Deshalb wird ALLES NEUE zuerst
 *    beschafft (Modul linken, Speicher holen, Parameter kopieren, Rahmen
 *    aufsetzen) und erst danach etwas Altes abgebaut. Ein Entwurf, der
 *    zuerst aufraeumt, hinterlaesst bei jedem Fehlschlag einen Prozess
 *    ohne Programm -- und der Fehlschlag ist der wahrscheinlichere Fall
 *    (Tippfehler im Namen).
 *
 * 2. DER ALTE SPEICHERBLOCK DARF NICHT FREIGEGEBEN WERDEN, SOLANGE DER
 *    KERNEL DARIN LAEUFT. Der Stack des Aufrufers liegt in eben diesem
 *    Block (s. F$Sleep, gleiches Muster), und dieser C-Code laeuft auf
 *    ihm. Die Freigabe steht deshalb in einer EIGENEN Funktion
 *    (Q9K_SysChainReleaseImpl), die die ASM-Seite erst aufruft, NACHDEM
 *    sie auf den Stack im neuen Block umgeschaltet hat. Beides in einem
 *    Rutsch zu erledigen ginge fast immer gut -- bis die naechste
 *    Allokation den freigegebenen Stack ueberschreibt, und dann
 *    unreproduzierbar.
 *
 * NICHT UMGESETZT: das Nachladen von der Platte, wenn das Modul nicht im
 * Verzeichnis steht. Das Handbuch nennt es als zweiten Schritt, aber
 * F$Load haengt in diesem Kernel noch im externen RBF-Pfad (s. STATUS.md,
 * `0x01`). Ein Modul, das nicht im Speicher steht, meldet hier deshalb
 * E$MNF statt geladen zu werden -- dieselbe Grenze wie bei F$Fork, und
 * sie verschwindet mit F$Load von selbst.
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

/* Alles aus q9kernel_firstproc.c -- F$Chain baut denselben Prozessrahmen
 * auf wie F$Fork und benutzt dieselben Bausteine. */
extern Q9_u32 Q9K_ModDirLinkByName(Q9_u16 typeLang, const char *name);
extern void   Q9K_ModDirUnlinkByHeader(Q9_u32 hdrAddr);
extern Q9_u32 Q9K_AllocMem(Q9_u32 size);
extern void   Q9K_FreeMem(Q9_u32 addr, Q9_u32 size);
extern void   Q9K_ApplyInitializedData(Q9_u32 hdrAddr, Q9_u32 block);
extern Q9_u32 Q9K_ReadHdrU32BE(Q9_u32 addr);
extern void   Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 index, Q9_u32 value);
extern Q9_u16 Q9K_ProcIdForDesc(Q9_u32 desc);
extern void   Q9K_AlarmCleanupProcess(Q9_u32 desc); /* native F$UAcct lifecycle */

#ifndef Q9K_E_MNF
#define Q9K_E_MNF    0xDDU
#endif
#ifndef Q9K_E_MEMFUL
#define Q9K_E_MEMFUL 0xCFU
#endif
#ifndef Q9K_E_BPADDR
#define Q9K_E_BPADDR 0xD2U
#endif

#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Modulkopf-Felder -- die Werte sind aus q9kernel_firstproc.c UEBERNOMMEN,
 * nicht nachgeschlagen: ein erster Entwurf hatte sie geraten (0x30/0x34/
 * 0x28), was Groessen und Einstiegspunkt verschob und den gechainten
 * Prozess mit Vektor 4 abstuerzen liess. Wer sie aendert, aendert sie an
 * beiden Stellen. */
#ifndef Q9K_MH_EXEC
#define Q9K_MH_EXEC  0x30UL
#define Q9K_MH_MEM   0x38UL
#define Q9K_MH_STACK 0x3CUL
#endif

#ifndef Q9K_PROCDESC_SAVEDSP_OFF
#define Q9K_PROCDESC_SAVEDSP_OFF   0x08UL
#endif
#ifndef Q9K_PROCDESC_SIGNAL_OFF
#define Q9K_PROCDESC_SIGNAL_OFF    0x26UL   /* P$Signal  */
#define Q9K_PROCDESC_SIGVEC_OFF    0x28UL   /* P$SigVec  */
#define Q9K_PROCDESC_SIGDAT_OFF    0x2CUL   /* P$SigDat  */
#endif
#ifndef Q9K_PROCDESC_MODHDR_OFF
#define Q9K_PROCDESC_MODHDR_OFF    0x38UL
#endif
#ifndef Q9K_PROCDESC_PRIORITY_OFF
#define Q9K_PROCDESC_PRIORITY_OFF  0x18UL
#endif
#ifndef Q9K_PROCDESC_ALLOCBASE_OFF
#define Q9K_PROCDESC_ALLOCBASE_OFF 0x1B0UL
#define Q9K_PROCDESC_ALLOCSIZE_OFF 0x1B4UL
#endif
#ifndef Q9K_PROCDESC_ENTRYPC_OFF
#define Q9K_PROCDESC_ENTRYPC_OFF   0x1C8UL
#endif

/* Rahmengroesse und Exception-Frame-Felder, wie bei F$Fork. */
#ifndef Q9K_PROCDESC_REGSAVE_SIZE
#define Q9K_PROCDESC_REGSAVE_SIZE 60UL    /* d0-d7/a0-a6 */
#endif
#ifndef Q9K_EXCFRAME_SR_OFF
#define Q9K_EXCFRAME_SR_OFF     0UL
#define Q9K_EXCFRAME_PC_OFF     2UL
#define Q9K_EXCFRAME_FMTVEC_OFF 6UL
#endif
#ifndef Q9K_FAKEFRAME_SIZE
#define Q9K_FAKEFRAME_SIZE (Q9K_PROCDESC_REGSAVE_SIZE + 8UL)
#endif
/* Supervisor, IPL=0 -- der Board-Timer muss den neuen Prozess sofort
 * unterbrechen koennen. Ebenfalls uebernommen: ein geratenes 0x0000
 * liefert einen User-Mode-Start, in dem der erste privilegierte Befehl
 * die Maschine anhaelt. */
#ifndef Q9K_INITIAL_SR
#define Q9K_INITIAL_SR 0x2000U
#endif
#ifndef Q9K_A6_BIAS
#define Q9K_A6_BIAS 0x8000UL
#endif

/* Scratch-Bruecke, hinter F$Sema ($1B80-$1B97). */
#ifndef Q9K_CHAIN_SCRATCH_TYPELANG
#define Q9K_CHAIN_SCRATCH_TYPELANG 0x1BA0UL /* Q9_u32, d0.w EIN */
#define Q9K_CHAIN_SCRATCH_ADDMEM   0x1BA4UL /* Q9_u32, d1.l EIN */
#define Q9K_CHAIN_SCRATCH_PARAMSZ  0x1BA8UL /* Q9_u32, d2.l EIN */
#define Q9K_CHAIN_SCRATCH_NUMPATHS 0x1BACUL /* Q9_u32, d3.w EIN */
#define Q9K_CHAIN_SCRATCH_PRIORITY 0x1BB0UL /* Q9_u32, d4.w EIN */
#define Q9K_CHAIN_SCRATCH_NAME     0x1BB4UL /* Q9_u32, (a0) EIN */
#define Q9K_CHAIN_SCRATCH_PARAM    0x1BB8UL /* Q9_u32, (a1) EIN */
#define Q9K_CHAIN_SCRATCH_ERROR    0x1BBCUL /* Q9_u32, d1.w AUS bei Fehler */
#define Q9K_CHAIN_SCRATCH_OK       0x1BC0UL /* Q9_u32, 0/1 */
#define Q9K_CHAIN_SCRATCH_OLDBLOCK 0x1BC4UL /* Q9_u32, alter Block, spaeter freizugeben */
#define Q9K_CHAIN_SCRATCH_OLDSIZE  0x1BC8UL /* Q9_u32, dessen Groesse */
#define Q9K_CHAIN_SCRATCH_OLDMOD   0x1BCCUL /* Q9_u32, altes Primaermodul */
#endif

#ifndef Q9K_CELL_ACCESSORS_PROVIDED
static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }
#endif
#ifndef Q9K_CHAIN_BYTE_ACCESSORS
static Q9_u8  Q9K_GetU8(Q9_u32 addr) { return *(volatile Q9_u8 *)addr; }
static void   Q9K_SetU8(Q9_u32 addr, Q9_u8 value) { *(volatile Q9_u8 *)addr = value; }
static Q9_u16 Q9K_GetU16(Q9_u32 addr) { return *(volatile Q9_u16 *)addr; }
static void   Q9K_SetU16(Q9_u32 addr, Q9_u16 value) { *(volatile Q9_u16 *)addr = value; }
#endif

/* Q9K_ProcChain -- der erste, aufbauende Teil.
 *
 * Baut Modul, Speicher und Prozessrahmen des neuen Programms vollstaendig
 * auf und traegt sie in den Deskriptor des AUFRUFERS ein. Was danach vom
 * alten Programm noch uebrig ist (Speicherblock und Modulverweis), bleibt
 * in den OLD*-Zellen liegen und wird erst von Q9K_SysChainReleaseImpl
 * abgeraeumt -- Begruendung im Kopfkommentar.
 *
 * Rueckgabe 1 = Erfolg; dann steht in SavedSP der Rahmen des neuen
 * Programms und der Aufrufer darf dorthin wechseln. */
int Q9K_ProcChain(Q9_u16 typeLang, Q9_u32 addMem, Q9_u32 paramSize,
                  Q9_u32 namePtr, Q9_u32 paramPtr, Q9_u16 priorityIn,
                  Q9_u16 numPaths, Q9_u16 *outError)
{
    Q9_u32 desc = Q9K_GetU32(Q9_D_PROC);
    Q9_u32 hdrAddr, dataSize, stackSize, execOff, totalSize, block;
    Q9_u32 blockTop, spBoundary, frameBase, entryPC;
    Q9_u16 priority;
    Q9_u32 i;

    if (desc == 0UL) {
        /* Ohne laufenden Prozess gibt es niemanden, dessen Programm
         * ersetzt werden koennte. */
        *outError = (Q9_u16)Q9K_E_BPADDR;
        return 0;
    }

    /* --- Schritt 1: alles Neue beschaffen. Bis hierher bleibt der
     *     Aufrufer in jedem Fehlerfall unversehrt. --- */
    hdrAddr = Q9K_ModDirLinkByName(typeLang, (const char *)(unsigned long)namePtr);
    if (hdrAddr == 0UL) {
        *outError = (Q9_u16)Q9K_E_MNF;
        return 0;
    }

    dataSize  = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_MEM);
    stackSize = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_STACK);
    execOff   = Q9K_ReadHdrU32BE(hdrAddr + Q9K_MH_EXEC);
    totalSize = dataSize + stackSize + addMem + paramSize;

    block = Q9K_AllocMem(totalSize);
    if (block == 0UL) {
        Q9K_ModDirUnlinkByHeader(hdrAddr);   /* Linkzaehler zuruecknehmen */
        *outError = (Q9_u16)Q9K_E_MEMFUL;
        return 0;
    }

    Q9K_ApplyInitializedData(hdrAddr, block);

    blockTop   = block + totalSize;
    spBoundary = blockTop - paramSize;

    /* Die Parameter werden JETZT kopiert, solange der alte Block noch
     * steht: (a1) zeigt fast immer in den Parameterbereich des alten
     * Programms, und der verschwindet gleich. */
    for (i = 0UL; i < paramSize; i++)
        Q9K_SetU8(spBoundary + i, Q9K_GetU8(paramPtr + i));

    frameBase = spBoundary - Q9K_FAKEFRAME_SIZE;

    /* Die Prioritaet behaelt der Prozess, wenn der Aufrufer keine nennt --
     * bei F$Fork erbt das Kind sie vom Elternprozess, hier ist es
     * derselbe Prozess. */
    priority = (priorityIn != 0U)
             ? priorityIn
             : (Q9_u16)Q9K_GetU8(desc + Q9K_PROCDESC_PRIORITY_OFF);

    entryPC = hdrAddr + execOff;

    /* Registeruebergabe exakt wie bei F$Fork -- das Handbuch sagt zu
     * Figure D-2 ausdruecklich "these are identical to F$Fork". Die
     * Prozess-ID bleibt dieselbe: es ist derselbe Prozess. */
    Q9K_SetFrameReg(frameBase, 0, Q9K_ProcIdForDesc(desc)); /* d0.w = Process ID  */
    Q9K_SetFrameReg(frameBase, 1, 0);                       /* d1.l = Group/user  */
    Q9K_SetFrameReg(frameBase, 2, priority);                /* d2.w = Priority    */
    Q9K_SetFrameReg(frameBase, 3, numPaths);                /* d3.w = Pfadzahl    */
    Q9K_SetFrameReg(frameBase, 4, 0);                       /* d4.l = Undefiniert */
    Q9K_SetFrameReg(frameBase, 5, paramSize);               /* d5.l = Parametergroesse */
    Q9K_SetFrameReg(frameBase, 6, totalSize);               /* d6.l = Gesamtspeicher   */
    Q9K_SetFrameReg(frameBase, 7, 0);                       /* d7.l = Undefiniert */
    Q9K_SetFrameReg(frameBase, 8, 0);                       /* a0 = Undefiniert   */
    Q9K_SetFrameReg(frameBase, 9, blockTop);                /* a1 = Speicherobergrenze */
    Q9K_SetFrameReg(frameBase, 10, 0);                      /* a2 = Undefiniert   */
    Q9K_SetFrameReg(frameBase, 11, hdrAddr);                /* a3 = neues Primaermodul */
    Q9K_SetFrameReg(frameBase, 12, 0);                      /* a4 = Undefiniert   */
    Q9K_SetFrameReg(frameBase, 13, blockTop);               /* a5 = Parametergrenze */
    Q9K_SetFrameReg(frameBase, 14, block + Q9K_A6_BIAS);    /* a6 = Datenbereich, $8000-Bias */

    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SR_OFF,
               Q9K_INITIAL_SR);
    Q9K_SetU32(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF,
               entryPC);
    Q9K_SetU16(frameBase + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_FMTVEC_OFF, 0);

    /* --- Schritt 2: das Alte merken, aber noch nicht anfassen. --- */
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OLDBLOCK, Q9K_GetU32(desc + Q9K_PROCDESC_ALLOCBASE_OFF));
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OLDSIZE,  Q9K_GetU32(desc + Q9K_PROCDESC_ALLOCSIZE_OFF));
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OLDMOD,   Q9K_GetU32(desc + Q9K_PROCDESC_MODHDR_OFF));

    /* --- Schritt 3: den Deskriptor auf das neue Programm umstellen. ---
     *
     * Die Pfadtabelle bleibt unberuehrt: "Open paths are not closed or
     * otherwise affected". Ebenso Eltern, Benutzer und Prozess-ID -- es
     * ist derselbe Prozess, er fuehrt nur etwas anderes aus. */
    Q9K_SetU32(desc + Q9K_PROCDESC_MODHDR_OFF,    hdrAddr);
    Q9K_SetU32(desc + Q9K_PROCDESC_ALLOCBASE_OFF, block);
    Q9K_SetU32(desc + Q9K_PROCDESC_ALLOCSIZE_OFF, totalSize);
    Q9K_SetU32(desc + Q9K_PROCDESC_SAVEDSP_OFF,   frameBase);
    Q9K_SetU32(desc + Q9K_PROCDESC_ENTRYPC_OFF,   entryPC);
    Q9K_SetU8(desc + Q9K_PROCDESC_PRIORITY_OFF,   (Q9_u8)priority);

    /* "Erase intercepts and any pending signals." Beides gehoerte zum
     * alten Programm: ein Intercept zeigt auf Code, den es gleich nicht
     * mehr gibt, und ein anstehendes Signal war an das alte Programm
     * gerichtet. Beides stehenzulassen waere ein Sprung ins Leere beim
     * naechsten Signal. */
    Q9K_SetU16(desc + Q9K_PROCDESC_SIGNAL_OFF, 0U);
    Q9K_SetU32(desc + Q9K_PROCDESC_SIGVEC_OFF, 0UL);
    Q9K_SetU32(desc + Q9K_PROCDESC_SIGDAT_OFF, 0UL);

    return 1;
}

/* Q9K_SysChainImpl -- Bruecke, aufbauender Teil. */
void Q9K_SysChainImpl(void)
{
    Q9_u16 err = 0U;
    int ok;

    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OK, 0UL);
    ok = Q9K_ProcChain((Q9_u16)Q9K_GetU32(Q9K_CHAIN_SCRATCH_TYPELANG),
                       Q9K_GetU32(Q9K_CHAIN_SCRATCH_ADDMEM),
                       Q9K_GetU32(Q9K_CHAIN_SCRATCH_PARAMSZ),
                       Q9K_GetU32(Q9K_CHAIN_SCRATCH_NAME),
                       Q9K_GetU32(Q9K_CHAIN_SCRATCH_PARAM),
                       (Q9_u16)Q9K_GetU32(Q9K_CHAIN_SCRATCH_PRIORITY),
                       (Q9_u16)Q9K_GetU32(Q9K_CHAIN_SCRATCH_NUMPATHS),
                       &err);
    if (!ok) {
        Q9K_SetU32(Q9K_CHAIN_SCRATCH_ERROR, (Q9_u32)err);
        return;
    }
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OK, 1UL);
}

/* Q9K_SysChainReleaseImpl -- der abbauende Teil.
 *
 * Wird von der ASM-Seite erst aufgerufen, NACHDEM sie auf den Stack im
 * neuen Speicherblock umgeschaltet hat. Vorher waere die Freigabe ein
 * Schuss ins eigene Knie: der Kernel laeuft bis dahin auf dem Stack im
 * ALTEN Block (s. Kopfkommentar). */
void Q9K_SysChainReleaseImpl(void)
{
    Q9_u32 oldBlock = Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDBLOCK);
    Q9_u32 oldSize  = Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDSIZE);
    Q9_u32 oldMod   = Q9K_GetU32(Q9K_CHAIN_SCRATCH_OLDMOD);

    /* F$UAcct is an optional OS9P2 callback on Chain.  Q9 owns alarms
     * directly, so perform the same process-lifecycle cleanup natively. */
    Q9K_AlarmCleanupProcess(Q9K_GetU32(Q9_D_PROC));

    if (oldMod != 0UL)
        Q9K_ModDirUnlinkByHeader(oldMod);
    if (oldBlock != 0UL && oldSize != 0UL)
        Q9K_FreeMem(oldBlock, oldSize);

    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OLDBLOCK, 0UL);
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OLDSIZE, 0UL);
    Q9K_SetU32(Q9K_CHAIN_SCRATCH_OLDMOD, 0UL);
}
