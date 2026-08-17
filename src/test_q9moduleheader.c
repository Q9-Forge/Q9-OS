/*
 * test_q9moduleheader.c -- Regressionstest fuer Q9_DetectModuleHeaderFormat/
 *                          Q9_ReadModuleName.
 *
 * Erster Baustein einer Testkonvention fuer Q9-OS-Host-Tooling (gab es
 * bisher nicht, nur Build-Artefakte unter build/). Prueft gegen echte,
 * bereits im Repo liegende Vendor-Dateien (modules/os9000-x86/vendor-live/,
 * vendor/68020/) UND synthetische Grenzfaelle.
 *
 * Aufruf: test_q9moduleheader (kein Argument -- Pfade sind repo-relativ
 * fest verdrahtet, ueber REPO_ROOT ueberschreibbar falls das Skript aus
 * einem anderen Arbeitsverzeichnis laeuft)
 *
 * Exit-Code 0 = alle Tests bestanden, 1 = mindestens ein Fehlschlag.
 *
 * Bauen und laufen lassen (Host-gcc, aus src/ heraus):
 *   gcc -Wall -Wextra -o test_q9moduleheader test_q9moduleheader.c q9moduleheader.c
 *   ./test_q9moduleheader ..
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "q9moduleheader.h"

static int failures = 0;

static const char *fmtName(Q9_ModHeadFormat f)
{
    switch (f) {
    case Q9_MHFMT_UNKNOWN:   return "UNKNOWN";
    case Q9_MHFMT_6809:      return "6809";
    case Q9_MHFMT_68K:       return "68K";
    case Q9_MHFMT_OS9000_BE: return "OS9000_BE";
    case Q9_MHFMT_OS9000_LE: return "OS9000_LE";
    case Q9_MHFMT_Q9OWN:     return "Q9OWN";
    }
    return "?";
}

static void checkFormat(const char *label, Q9_ModHeadFormat got, Q9_ModHeadFormat want)
{
    if (got == want) {
        printf("[OK]   %-55s -> %s\n", label, fmtName(got));
    } else {
        printf("[FAIL] %-55s -> %s (erwartet %s)\n", label, fmtName(got), fmtName(want));
        failures++;
    }
}

static void checkName(const char *label, const char *got, const char *want)
{
    if (strcmp(got, want) == 0) {
        printf("[OK]   %-55s Name = \"%s\"\n", label, got);
    } else {
        printf("[FAIL] %-55s Name = \"%s\" (erwartet \"%s\")\n", label, got, want);
        failures++;
    }
}

static int testRealFile(const char *repoRoot, const char *relPath,
                         Q9_ModHeadFormat wantFmt, const char *wantName,
                         int littleEndian, uint32_t nameOffsetFieldOffset)
{
    char fullPath[1024];
    FILE *f;
    static unsigned char buf[4 * 1024 * 1024];
    size_t n;
    Q9_ModHeadFormat got;
    char name[64];
    uint32_t nameOff;

    snprintf(fullPath, sizeof(fullPath), "%s/%s", repoRoot, relPath);
    f = fopen(fullPath, "rb");
    if (!f) {
        printf("[SKIP] %s nicht gefunden (%s) -- Vendor-Datei fehlt lokal?\n", relPath, fullPath);
        return 0; /* kein harter Fehler -- Vendor-Dateien sind bewusst nicht ueberall verfuegbar */
    }
    n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    got = Q9_DetectModuleHeaderFormat(buf, (uint32_t)n);
    checkFormat(relPath, got, wantFmt);

    if (got == wantFmt && n > nameOffsetFieldOffset + 3) {
        if (littleEndian)
            nameOff = (uint32_t)buf[nameOffsetFieldOffset] | ((uint32_t)buf[nameOffsetFieldOffset + 1] << 8) |
                      ((uint32_t)buf[nameOffsetFieldOffset + 2] << 16) | ((uint32_t)buf[nameOffsetFieldOffset + 3] << 24);
        else
            nameOff = ((uint32_t)buf[nameOffsetFieldOffset] << 24) | ((uint32_t)buf[nameOffsetFieldOffset + 1] << 16) |
                      ((uint32_t)buf[nameOffsetFieldOffset + 2] << 8) | buf[nameOffsetFieldOffset + 3];
        Q9_ReadModuleName(buf, (uint32_t)n, nameOff, littleEndian, name, sizeof(name));
        checkName(relPath, name, wantName);
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *repoRoot = (argc > 1) ? argv[1] : "..";

    printf("== Echte Vendor-Dateien ==\n");
    testRealFile(repoRoot, "vendor/68020/dker030s", Q9_MHFMT_68K, "kernel",
                 0, Q9_MH68K_NAME);
    testRealFile(repoRoot, "modules/os9000-x86/vendor-live/kernel", Q9_MHFMT_OS9000_LE, "kernel",
                 1, Q9_MH9K_NAME);
    testRealFile(repoRoot, "modules/os9000-x86/vendor-live/ioman", Q9_MHFMT_OS9000_LE, "ioman",
                 1, Q9_MH9K_NAME);
    testRealFile(repoRoot, "modules/os9000-x86/vendor-live/rbf", Q9_MHFMT_OS9000_LE, "rbf",
                 1, Q9_MH9K_NAME);
    testRealFile(repoRoot, "modules/os9000-x86/vendor-live/ssm", Q9_MHFMT_OS9000_LE, "ssm",
                 1, Q9_MH9K_NAME);

    printf("\n== Synthetische Grenzfaelle ==\n");
    {
        unsigned char q9be[4] = { 0x51, 0x39, 0, 0 };
        unsigned char q9le[4] = { 0x39, 0x51, 0, 0 };
        unsigned char garbage[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
        unsigned char tooShort[1] = { 0x4A };
        unsigned char sync6809[2] = { 0x87, 0xCD };

        checkFormat("Q9-eigen, Big-Endian", Q9_DetectModuleHeaderFormat(q9be, 4), Q9_MHFMT_Q9OWN);
        checkFormat("Q9-eigen, Little-Endian", Q9_DetectModuleHeaderFormat(q9le, 4), Q9_MHFMT_Q9OWN);
        checkFormat("6809-Sync (HANDBUCH, kein Live-Sample)", Q9_DetectModuleHeaderFormat(sync6809, 2), Q9_MHFMT_6809);
        checkFormat("Zufallsbytes", Q9_DetectModuleHeaderFormat(garbage, 4), Q9_MHFMT_UNKNOWN);
        checkFormat("Zu kurz (1 Byte)", Q9_DetectModuleHeaderFormat(tooShort, 1), Q9_MHFMT_UNKNOWN);
        checkFormat("NULL-Pointer", Q9_DetectModuleHeaderFormat(NULL, 10), Q9_MHFMT_UNKNOWN);
    }

    printf("\n%s\n", failures == 0 ? "ALLE TESTS BESTANDEN" : "FEHLSCHLAEGE VORHANDEN");
    return failures == 0 ? 0 : 1;
}
