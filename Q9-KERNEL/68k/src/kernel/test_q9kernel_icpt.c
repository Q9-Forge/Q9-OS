/*
 * test_q9kernel_icpt.c -- Hosttest fuer die Ausfuehrung von
 *                         Intercept-Routinen, F$RTE und F$SigReset
 *                         (q9kernel_icpt.c, 2026-09-18).
 *
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_icpt test_q9kernel_icpt.c && ./test_icpt
 *
 * Geprueft wird das Stapeln und Zurueckholen der Prozessrahmen -- also
 * genau das, was ueber Wohl und Wehe entscheidet: ein Rahmen zu wenig
 * oder zu viel schickt den Prozess an eine beliebige Stelle.
 */

#include <stdio.h>
#include <string.h>

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

static unsigned char g_cells[0x200];
#define CELL(off) ((unsigned long)(g_cells + (off)))

#define Q9_D_PROC              CELL(0x000)
#define Q9K_ICPT_SCRATCH_OK    CELL(0x020)
#define Q9K_ICPT_SCRATCH_ERROR CELL(0x040)
#define Q9K_ICPT_SCRATCH_DEPTH CELL(0x060)

/* Deskriptor-Abstaende auf Testbreite gezogen. */
#define Q9K_PROCDESC_SAVEDSP_OFF   0x08UL
#define Q9K_PROCDESC_SIGNAL_OFF    0x20UL
#define Q9K_PROCDESC_SIGVEC_OFF    0x28UL
#define Q9K_PROCDESC_SIGDAT_OFF    0x30UL
#define Q9K_PROCDESC_ICPTDEPTH_OFF 0x38UL

/* Der Rahmen selbst behaelt seine ECHTEN Masse: 15 Register zu 4 Byte
 * plus 8 Byte Ausnahmerahmen. Q9K_Set/GetFrameReg rechnen byteweise und
 * sind deshalb auf jedem Host massgerecht -- anders als die
 * Deskriptorfelder oben, die ueber Q9K_SetU32 laufen. */
static unsigned long g_frameRegs[32][15];
static unsigned char g_stack[2048];

#define STACKTOP ((Q9_u32)(unsigned long)(g_stack + sizeof g_stack))

/* Nachbau der beiden Rahmenzugriffe aus q9kernel_firstproc.c -- byteweise,
 * damit der Test dieselbe Darstellung benutzt wie der Kernel. */
void Q9K_SetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex, Q9_u32 value)
{
    volatile unsigned char *p = (volatile unsigned char *)(frameBase + regIndex * 4UL);
    p[0] = (unsigned char)(value >> 24); p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);  p[3] = (unsigned char)value;
}

Q9_u32 Q9K_GetFrameReg(Q9_u32 frameBase, Q9_u32 regIndex)
{
    volatile unsigned char *p = (volatile unsigned char *)(frameBase + regIndex * 4UL);
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16)
         | ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

#include "q9kernel_icpt.c"

static int failures;
static unsigned char g_desc[256];
#define DESC ((Q9_u32)(unsigned long)g_desc)
#define VECTOR 0x12340000UL
#define SIGDAT 0x55660000UL

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

static void checkHex(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-62s = 0x%lx\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-62s = 0x%lx (erwartet 0x%lx)\n", label,
               (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

/* Ein Prozess mit gueltigem Rahmen, eingetragener Routine und bekannten
 * Registerwerten im Hauptprogramm. */
static Q9_u32 setup(void)
{
    Q9_u32 frame = STACKTOP - Q9K_FAKEFRAME_SIZE;
    Q9_u32 i;

    memset(g_cells, 0, sizeof g_cells);
    memset(g_desc, 0, sizeof g_desc);
    memset(g_stack, 0, sizeof g_stack);
    memset(g_frameRegs, 0, sizeof g_frameRegs);

    for (i = 0UL; i < 15UL; i++)
        Q9K_SetFrameReg(frame, i, 0xA0000000UL + i);
    Q9K_SetU16(frame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_SR_OFF, 0x2000);
    Q9K_SetU32(frame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF, 0xDEAD0000UL);

    Q9K_SetU32(Q9_D_PROC, DESC);
    Q9K_SetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF, frame);
    Q9K_SetU32(DESC + Q9K_PROCDESC_SIGVEC_OFF, VECTOR);
    Q9K_SetU32(DESC + Q9K_PROCDESC_SIGDAT_OFF, SIGDAT);
    return frame;
}

int main(void)
{
    Q9_u32 mainFrame, icptFrame;
    Q9_u16 err;

    printf("== Intercept-Ausfuehrung, F$RTE und F$SigReset ==\n");

    /* --- Zustellung --- */
    mainFrame = setup();
    check("ein Signal wird zugestellt",
          (Q9_u32)Q9K_IcptDeliver(DESC, 3), 1);
    icptFrame = Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF);
    check("der neue Rahmen liegt genau einen Rahmen tiefer",
          mainFrame - icptFrame, Q9K_FAKEFRAME_SIZE);
    check("die Tiefe ist 1", Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 1);

    /* Was die Routine vorfindet: Signalcode und Datenbereich. */
    check("d1 traegt den Signalcode", Q9K_GetFrameReg(icptFrame, 1), 3);
    checkHex("a6 traegt den Datenbereich", Q9K_GetFrameReg(icptFrame, 14), SIGDAT);
    checkHex("der Einstieg ist die eingetragene Routine",
             Q9K_GetU32(icptFrame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF),
             VECTOR);

    /* Der Rahmen des Hauptprogramms bleibt unangetastet -- sonst gaebe es
     * nichts mehr, wohin F$RTE zurueckkehren koennte. */
    checkHex("der Rahmen des Hauptprogramms bleibt unveraendert",
             Q9K_GetU32(mainFrame + Q9K_PROCDESC_REGSAVE_SIZE + Q9K_EXCFRAME_PC_OFF),
             0xDEAD0000UL);
    checkHex("samt seiner Register", Q9K_GetFrameReg(mainFrame, 5), 0xA0000005UL);

    /* Die uebrigen Register uebernimmt die Routine vom Hauptprogramm --
     * undefiniert heisst nicht "Muell". */
    checkHex("nicht belegte Register sind uebernommen, nicht zufaellig",
             Q9K_GetFrameReg(icptFrame, 5), 0xA0000005UL);

    /* Das Signal ist entgegengenommen: bliebe es stehen, liefe die
     * Routine beim naechsten F$RTE noch einmal fuer dasselbe Signal. */
    check("das anstehende Signal ist abgeraeumt",
          (Q9_u32)Q9K_GetU16(DESC + Q9K_PROCDESC_SIGNAL_OFF), 0);

    /* --- F$RTE --- */
    err = 0;
    check("F$RTE kehrt zurueck", (Q9_u32)Q9K_IcptReturn(DESC, &err), 1);
    check("und stellt den Rahmen des Hauptprogramms wieder her",
          Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF), mainFrame);
    check("die Tiefe ist wieder 0",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 0);

    /* --- Verschachtelung: ein Signal waehrend der Routine --- */
    mainFrame = setup();
    Q9K_IcptDeliver(DESC, 3);
    icptFrame = Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF);
    Q9K_IcptDeliver(DESC, 4);
    check("ein zweites Signal stapelt einen weiteren Rahmen",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 2);
    check("wieder genau einen Rahmen tiefer",
          icptFrame - Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF),
          Q9K_FAKEFRAME_SIZE);
    check("mit dem neuen Signalcode",
          Q9K_GetFrameReg(Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF), 1), 4);

    err = 0;
    Q9K_IcptReturn(DESC, &err);
    check("das erste F$RTE fuehrt zur aeusseren Routine zurueck",
          Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF), icptFrame);
    Q9K_IcptReturn(DESC, &err);
    check("das zweite ins Hauptprogramm",
          Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF), mainFrame);

    /* --- F$RTE mit noch anstehendem Signal --- */
    mainFrame = setup();
    Q9K_IcptDeliver(DESC, 3);
    Q9K_SetU16(DESC + Q9K_PROCDESC_SIGNAL_OFF, 9);   /* waehrenddessen eingetroffen */
    err = 0;
    Q9K_IcptReturn(DESC, &err);
    check("bei anstehendem Signal laeuft die Routine erneut",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 1);
    check("mit dem neuen Signalcode",
          Q9K_GetFrameReg(Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF), 1), 9);
    check("statt ins Hauptprogramm zurueckzukehren",
          (Q9_u32)(Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF) != mainFrame), 1);

    /* --- Wann NICHT zugestellt wird --- */
    setup();
    Q9K_SetU32(DESC + Q9K_PROCDESC_SIGVEC_OFF, 0);
    check("ohne eingetragene Routine wird nicht zugestellt",
          (Q9_u32)Q9K_IcptDeliver(DESC, 3), 0);
    check("und nichts gestapelt",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 0);

    setup();
    Q9K_SetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF, 0);
    check("ohne gesicherten Zustand wird nicht zugestellt",
          (Q9_u32)Q9K_IcptDeliver(DESC, 3), 0);

    check("ein Nulldeskriptor wird abgewiesen",
          (Q9_u32)Q9K_IcptDeliver(0, 3), 0);

    /* --- F$RTE ohne Intercept ---
     *
     * Das ist der gefaehrliche Fall: wuerde hier ein Rahmen abgeraeumt,
     * verloere das Hauptprogramm seinen eigenen Zustand. */
    mainFrame = setup();
    err = 0;
    check("F$RTE ohne Intercept wird abgewiesen",
          (Q9_u32)Q9K_IcptReturn(DESC, &err), 0);
    check("mit E_BPADDR", (Q9_u32)err, 0xD2);
    check("und laesst den Rahmen des Hauptprogramms stehen",
          Q9K_GetU32(DESC + Q9K_PROCDESC_SAVEDSP_OFF), mainFrame);

    /* --- F$SigReset --- */
    setup();
    Q9K_IcptDeliver(DESC, 3);
    Q9K_IcptDeliver(DESC, 4);
    Q9K_IcptReset(DESC);
    check("F$SigReset raeumt den Kontextstapel ab",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 0);
    err = 0;
    check("danach wird ein F$RTE abgewiesen",
          (Q9_u32)Q9K_IcptReturn(DESC, &err), 0);

    /* Das Handbuch nennt fuer F$SigReset keinen Fehlerfall. */
    setup();
    Q9K_IcptReset(DESC);
    check("F$SigReset ohne offenen Intercept ist in Ordnung",
          Q9K_GetU32(DESC + Q9K_PROCDESC_ICPTDEPTH_OFF), 0);

    /* --- Die Bruecken --- */
    setup();
    Q9K_IcptDeliver(DESC, 3);
    Q9K_SysRTEImpl();
    check("Bruecke F$RTE meldet Erfolg", Q9K_GetU32(Q9K_ICPT_SCRATCH_OK), 1);
    check("und die verbliebene Tiefe", Q9K_GetU32(Q9K_ICPT_SCRATCH_DEPTH), 0);

    Q9K_SysRTEImpl();
    check("ein zweites F$RTE wird abgewiesen", Q9K_GetU32(Q9K_ICPT_SCRATCH_OK), 0);
    check("mit E_BPADDR", Q9K_GetU32(Q9K_ICPT_SCRATCH_ERROR), 0xD2);

    setup();
    Q9K_IcptDeliver(DESC, 3);
    Q9K_SysSigResetImpl();
    check("Bruecke F$SigReset meldet Erfolg", Q9K_GetU32(Q9K_ICPT_SCRATCH_OK), 1);
    check("und meldet Tiefe 0", Q9K_GetU32(Q9K_ICPT_SCRATCH_DEPTH), 0);

    printf("\n%s\n", failures ? "TESTS FEHLGESCHLAGEN" : "ALLE TESTS BESTANDEN");
    return failures ? 1 : 0;
}
