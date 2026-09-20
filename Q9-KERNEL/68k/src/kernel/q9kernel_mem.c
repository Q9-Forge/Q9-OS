/*
 * q9kernel_mem.c -- Q9-OS eigener Kernel: F$Mem (Callcode $07,
 *                   2026-09-20).
 *
 * VERIFIZIERTE ABI -- doppelt belegt, einmal aus dem Handbuch und
 * einmal aus dem Originalkernel, die sich decken.
 *
 * 1) 68k_tech.pdf S.465 ("F$Mem -- Resize Data Memory Area"):
 *
 *      EIN    d0.l = Desired new memory size in bytes
 *      AUS    d0.l = Actual size of new memory area in bytes
 *             (a1) = Pointer to new end of data segment (+1)
 *      FEHLER cc = Carry, d1.w = Fehlercode
 *      Zustand: User
 *
 *    "If d0 equals 0, the call is considered an information request and
 *     the current upper bound and size is returned."
 *
 * 2) Referenzkernel `dker030s`, Einstieg Modul-Offset $133C (Laufzeit
 *    $843C, in BEIDEN Dispatchtabellen registriert -- nicht auf dem
 *    Fehler-Stub). Der Einstieg ist ein duenner Mantel:
 *
 *      133c  move.l a5,d0        * D0 = Registerabbild des Aufrufers
 *      133e  moveq  #$24,d1
 *      1340  add.l  a5,d1        * D1 = &Abbild[$24] = A1-Slot (Ausgabe)
 *      1342  bsr.w  $6102        * Hauptarbeit
 *      1346  bra.w  $129c        * gemeinsamer Abschluss
 *
 *    und der Erfolgspfad der Hauptarbeit endet mit:
 *
 *      621c  move.l $330(a4),(a0)    * d0.l AUS = Groesse des Datenbereichs
 *      6220  move.l $32c(a4),d0      * Basis des Datenbereichs
 *      6224  add.l  (a0),d0          * + Groesse
 *      6226  move.l d0,(a1)          * a1 AUS = obere Grenze
 *      6228  moveq  #0,d0            * Erfolg
 *
 *    A4 ist dort $4c(a6), also D_Proc -- in diesem Kernel als
 *    Q9_D_PROC ($04C) bereits verifiziert. $32c/$330 sind die Basis
 *    und die Groesse des Prozess-Speicherblocks; die entsprechenden
 *    Felder heissen hier Q9K_PROCDESC_ALLOCBASE_OFF/ALLOCSIZE_OFF.
 *
 * WARUM NUR DIE INFORMATIONSABFRAGE, und warum ueberhaupt:
 *
 * STATUS.md fuehrte F$Mem bis 2026-09-20 als "withdrawn in real
 * OS-9/68K" und verwies dabei auf den Satz "F$Mem is no longer
 * available. Use F$SRqMem instead." Dieser Satz steht im Handbuch
 * allerdings NICHT auf der F$Mem-Seite, sondern in der
 * Fehlerbeschreibung zu E$MemFul im Fehleranhang -- er begruendet
 * dort, warum eine VERGROESSERUNG scheitert. Die F$Mem-Seite selbst
 * sagt auf S.466 praeziser:
 *
 *   "F$Mem calls to resize the data area always fail for versions of
 *    the kernel from OS-9 for 68K V2.3 and greater. Only an
 *    information request (d0=0) works on OS-9 for 68K V2.3 and
 *    greater."
 *
 * F$Mem ist also nicht zurueckgezogen, sondern auf die
 * Informationsabfrage reduziert -- und genau die ist hier
 * implementiert. Jede Groessenaenderung wird abgelehnt. Damit
 * verhaelt sich dieser Kernel wie ein reales OS-9/68K ab V2.3, und
 * ein Programm, das seine Datenbereichsgrenze erfragt, bekommt eine
 * richtige Antwort statt E$UnkSvc.
 *
 * FEHLERCODE BEI GROESSENAENDERUNG: $ED (E$NoRAM, aus errno.h, nicht
 * geraten). Das ist derselbe Code, den der Referenzkernel an dieser
 * Stelle liefert -- bei $61d4 vergleicht er die gewuenschte mit der
 * aktuellen Groesse und geht bei Vergroesserung nach $61da:
 *
 *      61d4  cmp.l $330(a4),d7
 *      61d8  bls.b $61e2
 *      61da  move.l #$ed,d0        * E$NoRAM
 *
 * Eine VERKLEINERUNG setzt das Original dagegen noch fort ($61e2ff.,
 * inklusive der E$DelSP-Pruefung gegen den Stackzeiger). Dieser
 * Kernel lehnt sie bewusst ebenfalls ab, statt sie nachzubauen: eine
 * teilweise Rueckgabe des Prozessblocks an die Arena ist hier nicht
 * vorgesehen (Q9K_FreeMem gibt einen Block als Ganzes zurueck, s.
 * q9kernel_procend.c), und das Handbuch verlangt ab V2.3 ohnehin den
 * Fehlschlag. Ein halb umgesetztes Schrumpfen waere die schlechtere
 * Wahl als ein ehrlicher, dokumentierter Fehlschlag.
 *
 * EIGENE DATEI und nicht ein Zusatz in q9kernel_sysmem.c: eine neue
 * Funktion in einem frueh gelinkten Modul verschiebt alle spaeter
 * gelinkten, und dann reisst irgendwo ein bestehendes "bsr" die
 * 16-Bit-Reichweite (l68 meldet dann nur "operand size error" ohne
 * Stelle). Neue Module gehoeren deshalb ans ENDE der Link-Liste in
 * build.sh -- gleiche Begruendung wie in q9kernel_nproc.c.
 */

typedef unsigned long Q9_u32;

static Q9_u32 Q9K_GetU32(Q9_u32 addr) { return *(volatile Q9_u32 *)addr; }
static void   Q9K_SetU32(Q9_u32 addr, Q9_u32 value) { *(volatile Q9_u32 *)addr = value; }

/* Systemglobal: Zeiger auf den aktuellen Prozessdeskriptor.
 * [VERIFIZIERT], s. q9kernel_sched.c. */
#ifndef Q9_D_PROC
#define Q9_D_PROC 0x04CUL
#endif

/* Prozessdeskriptor: Basis und Groesse des primaeren Prozessblocks.
 * Gesetzt von Q9K_ProcFork (q9kernel_firstproc.c, totalSize =
 * dataSize + stackSize + addMem + paramSize), aktualisiert von
 * F$Chain (q9kernel_chain.c), freigegeben und genullt von F$Exit
 * (q9kernel_procend.c). Entspricht $32c/$330 im Referenzkernel. */
#ifndef Q9K_PROCDESC_ALLOCBASE_OFF
#define Q9K_PROCDESC_ALLOCBASE_OFF 0x1B0UL
#endif
#ifndef Q9K_PROCDESC_ALLOCSIZE_OFF
#define Q9K_PROCDESC_ALLOCSIZE_OFF 0x1B4UL
#endif

/* Scratch-Block fuer die Assembler-Bruecke. Naechster freier Bereich
 * hinter Q9K_SemaWaitImplPtr ($1E70, q9kernel_entry.a); der
 * Globalbereich reicht bis $8000, die Arena beginnt erst bei $18000
 * (q9kernel_cinit.c), Kollision also ausgeschlossen. Wie ueberall per
 * #define vor dem #include auf einen Testpuffer umlenkbar. */
#ifndef Q9K_MemScratch_Request
#define Q9K_MemScratch_Request  0x1E74UL /* Q9_u32, d0.l EIN            */
#endif
#ifndef Q9K_MemScratch_OutSize
#define Q9K_MemScratch_OutSize  0x1E78UL /* Q9_u32, d0.l AUS            */
#endif
#ifndef Q9K_MemScratch_OutUpper
#define Q9K_MemScratch_OutUpper 0x1E7CUL /* Q9_u32, a1 AUS              */
#endif
#ifndef Q9K_MemScratch_Error
#define Q9K_MemScratch_Error    0x1E80UL /* Q9_u32, d1.w AUS bei Fehler */
#endif
#ifndef Q9K_MemScratch_Ok
#define Q9K_MemScratch_Ok       0x1E84UL /* Q9_u32, 0/1                 */
#endif

#define Q9K_E_NORAM  0x00EDUL /* errno.h: No RAM Available              */
#define Q9K_E_PRCID  0x00E0UL /* errno.h: Illegal Process ID            */

/* Q9K_ProcMem -- echte F$Mem-Kernlogik, host-testbar.
 *
 * `desc` ist der aktuelle Prozessdeskriptor (0 = keiner).
 * `requested` ist d0.l des Aufrufers: 0 = Informationsabfrage.
 *
 * Rueckgabe 1 = Erfolg (outSize und outUpper gesetzt), 0 = Fehlschlag
 * (outError gesetzt). */
int Q9K_ProcMem(Q9_u32 desc, Q9_u32 requested,
                Q9_u32 *outSize, Q9_u32 *outUpper, Q9_u32 *outError)
{
    Q9_u32 base, size;

    *outSize  = 0UL;
    *outUpper = 0UL;
    *outError = 0UL;

    if (desc == 0UL) {
        *outError = Q9K_E_PRCID;   /* kein aktueller Prozess, s. q9kernel_procsleep.c */
        return 0;
    }

    /* Jede Groessenaenderung schlaegt fehl -- s. Kopfkommentar
     * ("always fail ... Only an information request (d0=0) works"). */
    if (requested != 0UL) {
        *outError = Q9K_E_NORAM;
        return 0;
    }

    base = Q9K_GetU32(desc + Q9K_PROCDESC_ALLOCBASE_OFF);
    size = Q9K_GetU32(desc + Q9K_PROCDESC_ALLOCSIZE_OFF);

    *outSize  = size;
    *outUpper = base + size;   /* "new end of data segment (+1)" */
    return 1;
}

/* Q9K_SysFMemImpl -- duenne, parameterlose Bruecke zwischen dem
 * Assembler-Trampolin und Q9K_ProcMem, gleiches Muster wie ueberall
 * (Q9K_SysSetSysImpl usw.). */
void Q9K_SysFMemImpl(void)
{
    Q9_u32 requested = Q9K_GetU32(Q9K_MemScratch_Request);
    Q9_u32 desc      = Q9K_GetU32(Q9_D_PROC);
    Q9_u32 size = 0UL, upper = 0UL, error = 0UL;
    int ok;

    ok = Q9K_ProcMem(desc, requested, &size, &upper, &error);

    Q9K_SetU32(Q9K_MemScratch_OutSize,  size);
    Q9K_SetU32(Q9K_MemScratch_OutUpper, upper);
    Q9K_SetU32(Q9K_MemScratch_Error,    error);
    Q9K_SetU32(Q9K_MemScratch_Ok,       (Q9_u32)ok);
}
