/*
 * q9ident.c -- Host-seitiges "ident"-Werkzeug fuer OS-9-Module aller vier
 *              bekannten Generationen (6809/68K/OS-9000/Q9-eigen).
 *
 * Aufruf: q9ident <datei> [<datei> ...]
 *
 * Reines ANSI-C, laeuft auf dem Entwickler-Host (nicht auf OS-9 selbst) --
 * anders als src/mbr.c, das fuer den OS-9-Cross-Build gedacht ist. Liest
 * jede Datei komplett ein, erkennt das Format ueber Q9_DetectModuleHeaderFormat
 * und gibt Sync-Wert, Format, Groesse, Name (soweit vorhanden) und bei
 * 68K/OS-9000 Typ/Sprache aus.
 *
 * Bewusst noch schmal gehalten: erster Baustein einer Reihe, kein
 * vollstaendiger Feldabzug aller Header-Varianten. Erweiterung (CRC-Pruefung,
 * volle Q9-eigene abiClass-Auswertung) folgt bei Bedarf.
 *
 * Bauen (Host-gcc, NICHT der OS-9-Cross-Compiler aus dem Makefile daneben):
 *   gcc -Wall -Wextra -o q9ident q9ident.c q9moduleheader.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "q9moduleheader.h"

static const char *formatName(Q9_ModHeadFormat fmt)
{
    switch (fmt) {
    case Q9_MHFMT_6809:      return "OS-9/6809";
    case Q9_MHFMT_68K:       return "OS-9/68K";
    case Q9_MHFMT_OS9000_BE: return "OS-9000 (Big-Endian, z.B. PowerPC/ARM)";
    case Q9_MHFMT_OS9000_LE: return "OS-9000 (Little-Endian, z.B. x86)";
    case Q9_MHFMT_Q9OWN:     return "Q9-eigenes Format";
    case Q9_MHFMT_UNKNOWN:
    default:                 return "unbekannt";
    }
}

static uint32_t readU32(const unsigned char *p, int littleEndian)
{
    if (littleEndian)
        return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void identOne(const char *path)
{
    FILE *f;
    unsigned char *buf;
    long fileSize;
    size_t n;
    Q9_ModHeadFormat fmt;

    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "%s: kann Datei nicht oeffnen\n", path);
        return;
    }
    fseek(f, 0, SEEK_END);
    fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fileSize <= 0) {
        fprintf(stderr, "%s: leer oder Groesse nicht ermittelbar\n", path);
        fclose(f);
        return;
    }

    buf = (unsigned char *)malloc((size_t)fileSize);
    if (!buf) {
        fprintf(stderr, "%s: zu wenig Speicher (%ld Byte)\n", path, fileSize);
        fclose(f);
        return;
    }
    n = fread(buf, 1, (size_t)fileSize, f);
    fclose(f);

    fmt = Q9_DetectModuleHeaderFormat(buf, (uint32_t)n);

    printf("%s:\n", path);
    printf("  Format:  %s\n", formatName(fmt));
    printf("  Groesse: %lu Byte (gelesen)\n", (unsigned long)n);

    if (fmt == Q9_MHFMT_68K && n > Q9_MH68K_NAME + 3) {
        uint32_t nameOff = readU32(buf + Q9_MH68K_NAME, 0);
        uint8_t type = buf[Q9_MH68K_TYPE];
        uint8_t lang = buf[Q9_MH68K_LANG];
        char name[64];
        Q9_ReadModuleName(buf, (uint32_t)n, nameOff, 0, name, sizeof(name));
        printf("  Name:    %s\n", name[0] ? name : "(nicht im Puffer)");
        printf("  Typ/Lang: 0x%02X / 0x%02X\n", type, lang);
    } else if (fmt == Q9_MHFMT_OS9000_LE && n > Q9_MH9K_NAME + 3) {
        uint32_t nameOff = readU32(buf + Q9_MH9K_NAME, 1);
        uint16_t tyLang = (uint16_t)(buf[Q9_MH9K_TYLANG] | ((uint16_t)buf[Q9_MH9K_TYLANG + 1] << 8));
        char name[64];
        Q9_ReadModuleName(buf, (uint32_t)n, nameOff, 1, name, sizeof(name));
        printf("  Name:    %s\n", name[0] ? name : "(nicht im Puffer)");
        printf("  Typ/Lang: 0x%02X / 0x%02X\n", (tyLang >> 8) & 0xFF, tyLang & 0xFF);
    } else if (fmt == Q9_MHFMT_Q9OWN && n >= 4) {
        uint8_t hdrVersion = buf[2];
        uint8_t abiClass = buf[3];
        static const char *widthNames[4] = { "16-Bit", "32-Bit", "64-Bit", "reserviert" };
        printf("  hdrVersion: %u\n", hdrVersion);
        printf("  abiClass:   0x%02X (%s, %s-Endian)\n", abiClass,
               widthNames[abiClass & Q9_ABICLASS_WIDTH_MASK],
               (abiClass & Q9_ABICLASS_ENDIAN_MASK) ? "Little" : "Big");
    }

    free(buf);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2) {
        fprintf(stderr, "Aufruf: %s <datei> [<datei> ...]\n", argv[0]);
        return 1;
    }
    for (i = 1; i < argc; i++) {
        identOne(argv[i]);
        if (i + 1 < argc)
            printf("\n");
    }
    return 0;
}
