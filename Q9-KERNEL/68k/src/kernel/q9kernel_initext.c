/*
 * q9kernel_initext.c -- Q9-OS eigener Kernel: optionale Init-Modul-
 *                       Erweiterung (SMP-CPU-Anzahl u.a.).
 *
 * Vorgabe, 2026-08-18: "Das muesste aber optional sein. Wenn die fehlen
 * nehmen wir sinnige Default-Werte an... vielleicht definieren wir eine
 * zusaetzliche Groesse (z.B. 64 Byte), wenn es das ist dann haben wir
 * dafuer eine feste Struktur, bis die 64 Byte voll sind... dann koennen
 * wir ggf. eine zweite Erweiterung definieren."
 *
 * Mechanismus: nutzt den STANDARD-68K-Modulheader-Erweiterungspunkt
 * (M$HdExt/M$HdExtSz, Offset 0x28/0x2C -- s. Q9_MH68K_HDEXT/HDEXTSZ in
 * src/q9moduleheader.h), NICHTS Neues erfunden. Das Init-Modul ist
 * selbst ein normales Systm-Modul mit Standard-Header, hat diese Felder
 * also schon. Ein unveraendertes, klassisches Init-Modul hat dort 0 --
 * "keine Erweiterung", Defaults gelten. Erkennung zweistufig, gleiches
 * Muster wie ueberall sonst in diesem Projekt: M$HdExtSz muss GENAU
 * Q9K_INITEXT_SIZE sein (Groesse selbst ist der Versions-Diskriminator
 * -- ein kuenftiges "V2" bekaeme eine andere Groesse), UND das
 * eingebettete Magic+Version muss passen (schuetzt vor einem zufaellig
 * gleich grossen, aber inhaltlich fremden Block). Passt eines von
 * beiden nicht: Default verwenden, NICHT hart fehlschlagen -- "optional"
 * im eigentlichen Sinn.
 *
 * Bewusst noch minimal (nur cpuCount) -- 64 Byte lassen viel Raum fuer
 * spaetere Felder, ohne die Groesse (und damit die Versionserkennung)
 * zu aendern. Was passiert, wenn die 64 Byte irgendwann voll sind,
 * bewusst noch nicht entschieden ("dann koennen wir ggf. eine
 * zweite Erweiterung definieren") -- spaeter klaeren, wenn der Bedarf
 * wirklich da ist, nicht spekulativ vorwegnehmen.
 *
 * Braucht q9kernel_config.h (s. dort) -- inhaltlich hier aber wirklich
 * unabhaengig von Atomic/Development bzw. Standard/Buddy, da CPU-Anzahl
 * beide Achsen gleichermassen betrifft.
 *
 * Doppelt getestet (2026-08-18): echte xcc-Pipeline (alle Stufen exit
 * status = 0) UND Logik-Selbsttest mit synthetischen Byte-Arrays gegen
 * Host-gcc (test_q9kernel_initext.c, 7 Faelle inkl. klassisches Modul/
 * gueltige Erweiterung/falsches Magic/falsche Groesse/cpuCount=0/
 * ausserhalb availableLen/NULL-Pointer -- alle bestehen).
 */

#include "q9kernel_config.h"

typedef unsigned long  Q9_u32;
typedef unsigned short Q9_u16;
typedef unsigned char  Q9_u8;

/* Q9_MH68K_HDEXT/HDEXTSZ aus q9moduleheader.h -- lokal dupliziert wie in
 * den anderen Kernel-Dateien (kein getesteter Include-Mechanismus fuer
 * eine Host-stdint.h-Datei in dieser Toolchain, s. q9kernel_entry.a). */
#define Q9K_HDREXT_OFF      0x28
#define Q9K_HDREXTSZ_OFF    0x2C

#define Q9K_INITEXT_SIZE    64
#define Q9K_INITEXT_MAGIC   0x51394945UL   /* ASCII "Q9IE" -- Q9 Init Extension */
#define Q9K_INITEXT_VERSION 1

#define Q9K_DEFAULT_CPU_COUNT 1             /* kein SMP angenommen, sicherste Annahme */

/* Liest ein Big-Endian-32-/16-Bit-Wort (68K ist immer Big-Endian) --
 * gleiche Vorsicht wie in q9kernel_modcheck.c, kein Pointer-Cast. */
static Q9_u32 Q9K_ReadU32BE(const Q9_u8 *p)
{
    return ((Q9_u32)p[0] << 24) | ((Q9_u32)p[1] << 16) |
           ((Q9_u32)p[2] << 8) | (Q9_u32)p[3];
}

static Q9_u16 Q9K_ReadU16BE(const Q9_u8 *p)
{
    return (Q9_u16)(((Q9_u16)p[0] << 8) | p[1]);
}

/* Liefert die tatsaechlich zu verwendende CPU-Anzahl. initModAddr =
 * Basis des gefundenen Init-Moduls (Q9_D_INIT), availableLen = wie
 * viele Byte ab dort sicher lesbar sind. Bounds-Check vor jedem
 * Lesezugriff (gleiche Disziplin wie q9kernel_modcheck.c). Bei jeder
 * Unklarheit/Ungueltigkeit: Default zurueckgeben, nie raten. */
Q9_u32 Q9K_GetCpuCount(const Q9_u8 *initModAddr, Q9_u32 availableLen)
{
    Q9_u32 hdExtOffset;
    Q9_u16 hdExtSize;
    const Q9_u8 *ext;

    if (initModAddr == 0 || availableLen < Q9K_HDREXTSZ_OFF + 2)
        return Q9K_DEFAULT_CPU_COUNT;

    hdExtOffset = Q9K_ReadU32BE(initModAddr + Q9K_HDREXT_OFF);
    hdExtSize = Q9K_ReadU16BE(initModAddr + Q9K_HDREXTSZ_OFF);

    if (hdExtOffset == 0 || hdExtSize != Q9K_INITEXT_SIZE)
        return Q9K_DEFAULT_CPU_COUNT; /* keine oder unbekannte Erweiterung */

    if (hdExtOffset + Q9K_INITEXT_SIZE > availableLen)
        return Q9K_DEFAULT_CPU_COUNT; /* wuerde ausserhalb des gelesenen Bereichs liegen */

    ext = initModAddr + hdExtOffset;

    if (Q9K_ReadU32BE(ext) != Q9K_INITEXT_MAGIC)
        return Q9K_DEFAULT_CPU_COUNT;
    if (Q9K_ReadU16BE(ext + 4) != Q9K_INITEXT_VERSION)
        return Q9K_DEFAULT_CPU_COUNT;

    /* Layout: magic(4) + version(2) + reserved0(2) + cpuCount(4) + Rest offen */
    {
        Q9_u32 cpuCount = Q9K_ReadU32BE(ext + 8);
        if (cpuCount == 0)
            return Q9K_DEFAULT_CPU_COUNT; /* 0 waere unsinnig, lieber Default */
        return cpuCount;
    }
}
