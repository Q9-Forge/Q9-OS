/*
 * test_q9kernel_initext.c -- Regressionstest fuer Q9K_GetCpuCount
 *                            (q9kernel_initext.c).
 *
 * Reines portables C (Host-gcc), kein OS-9-Cross-Build noetig -- die
 * Logik selbst haengt an keiner Toolchain-Eigenheit, nur an Byte-
 * Arithmetik. Bindet q9kernel_initext.c direkt ein (nicht neu
 * abgeschrieben), damit Test und echte Implementierung nie auseinander-
 * laufen koennen.
 *
 * Bauen und laufen lassen (aus src/kernel/ heraus):
 *   gcc -Wall -Wextra -DQ9K_KERNEL_DEVELOPMENT -DQ9K_ALLOC_STANDARD \
 *       -o test_q9kernel_initext test_q9kernel_initext.c && \
 *       ./test_q9kernel_initext
 *
 * Exit-Code 0 = alle Tests bestanden, 1 = mindestens ein Fehlschlag.
 */

#include "q9kernel_initext.c"
#include <string.h>
#include <stdio.h>

static int failures = 0;

static void check(const char *label, Q9_u32 got, Q9_u32 want)
{
    if (got == want) {
        printf("[OK]   %-45s = %lu\n", label, (unsigned long)got);
    } else {
        printf("[FAIL] %-45s = %lu (erwartet %lu)\n", label, (unsigned long)got, (unsigned long)want);
        failures++;
    }
}

static void putU32BE(Q9_u8 *p, Q9_u32 v)
{
    p[0] = (Q9_u8)((v >> 24) & 0xFF);
    p[1] = (Q9_u8)((v >> 16) & 0xFF);
    p[2] = (Q9_u8)((v >> 8) & 0xFF);
    p[3] = (Q9_u8)(v & 0xFF);
}

static void putU16BE(Q9_u8 *p, Q9_u16 v)
{
    p[0] = (Q9_u8)((v >> 8) & 0xFF);
    p[1] = (Q9_u8)(v & 0xFF);
}

int main(void)
{
    Q9_u8 mod[256];

    /* Fall 1: klassisches, unveraendertes Init-Modul (HdExt = 0) -> Default */
    memset(mod, 0, sizeof(mod));
    check("Klassisch, HdExt=0", Q9K_GetCpuCount(mod, sizeof(mod)), Q9K_DEFAULT_CPU_COUNT);

    /* Fall 2: gueltige Erweiterung, cpuCount=4 */
    memset(mod, 0, sizeof(mod));
    putU32BE(mod + Q9K_HDREXT_OFF, 0x80);
    putU16BE(mod + Q9K_HDREXTSZ_OFF, Q9K_INITEXT_SIZE);
    putU32BE(mod + 0x80, Q9K_INITEXT_MAGIC);
    putU16BE(mod + 0x80 + 4, Q9K_INITEXT_VERSION);
    putU32BE(mod + 0x80 + 8, 4);
    check("Gueltig, cpuCount=4", Q9K_GetCpuCount(mod, sizeof(mod)), 4);

    /* Fall 3: falsches Magic -> Default (nicht abstuerzen/fremden Block missverstehen) */
    memset(mod, 0, sizeof(mod));
    putU32BE(mod + Q9K_HDREXT_OFF, 0x80);
    putU16BE(mod + Q9K_HDREXTSZ_OFF, Q9K_INITEXT_SIZE);
    putU32BE(mod + 0x80, 0xDEADBEEFUL);
    putU16BE(mod + 0x80 + 4, Q9K_INITEXT_VERSION);
    putU32BE(mod + 0x80 + 8, 4);
    check("Falsches Magic", Q9K_GetCpuCount(mod, sizeof(mod)), Q9K_DEFAULT_CPU_COUNT);

    /* Fall 4: falsche Groesse (Groesse selbst ist der Versions-Diskriminator) -> Default */
    memset(mod, 0, sizeof(mod));
    putU32BE(mod + Q9K_HDREXT_OFF, 0x80);
    putU16BE(mod + Q9K_HDREXTSZ_OFF, 32);
    putU32BE(mod + 0x80, Q9K_INITEXT_MAGIC);
    putU16BE(mod + 0x80 + 4, Q9K_INITEXT_VERSION);
    putU32BE(mod + 0x80 + 8, 4);
    check("Falsche HdExtSz (32 statt 64)", Q9K_GetCpuCount(mod, sizeof(mod)), Q9K_DEFAULT_CPU_COUNT);

    /* Fall 5: cpuCount=0 in einer sonst gueltigen Erweiterung -> Default (0 waere unsinnig) */
    memset(mod, 0, sizeof(mod));
    putU32BE(mod + Q9K_HDREXT_OFF, 0x80);
    putU16BE(mod + Q9K_HDREXTSZ_OFF, Q9K_INITEXT_SIZE);
    putU32BE(mod + 0x80, Q9K_INITEXT_MAGIC);
    putU16BE(mod + 0x80 + 4, Q9K_INITEXT_VERSION);
    putU32BE(mod + 0x80 + 8, 0);
    check("cpuCount=0 in gueltiger Erweiterung", Q9K_GetCpuCount(mod, sizeof(mod)), Q9K_DEFAULT_CPU_COUNT);

    /* Fall 6: Erweiterung wuerde ausserhalb des sicher gelesenen Bereichs liegen */
    memset(mod, 0, sizeof(mod));
    putU32BE(mod + Q9K_HDREXT_OFF, 0x80);
    putU16BE(mod + Q9K_HDREXTSZ_OFF, Q9K_INITEXT_SIZE);
    putU32BE(mod + 0x80, Q9K_INITEXT_MAGIC);
    putU16BE(mod + 0x80 + 4, Q9K_INITEXT_VERSION);
    putU32BE(mod + 0x80 + 8, 8);
    check("Ausserhalb availableLen (nur 0x80 Byte verfuegbar)", Q9K_GetCpuCount(mod, 0x80), Q9K_DEFAULT_CPU_COUNT);

    /* Fall 7: NULL-Pointer */
    check("NULL-Pointer", Q9K_GetCpuCount(0, 100), Q9K_DEFAULT_CPU_COUNT);

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
