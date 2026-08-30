/*
 * q9kernel_arena.c -- Q9-OS eigener Kernel: Speicherallokator (Arena).
 *
 * Setzt Q9_D_ARENA (Kernel-Global-Offset $3FC) von seinem beim Boot
 * initialisierten leeren Zustand (Kopf=Schwanz=sich selbst, s.
 * q9kernel_cinit.c/Q9K_InitEmptyQueue) auf einen echten, benutzbaren
 * Freispeicher-Zustand -- exakt dasselbe Zwei-Phasen-Muster wie beim
 * REALEN Kernel (Thema 01: "Kopf/Ende-Zeiger bei +0x8/+0xC selbst-
 * referenzierend beim Boot initialisiert", die eigentliche Registrierung
 * echten freien Speichers passiert erst spaeter, sobald die RAM-Groesse
 * bekannt ist -- Q9_D_TOTRAM steht schon seit dem Assembler-Einstieg
 * fest, s. q9kernel_entry.a).
 *
 * BEWUSST vereinfacht gegenueber dem Original: der reale Kernel hat eine
 * "farbklassifizierte" (nach Groessenklasse segmentierte) Freispeicher-
 * liste (s. Q9_D_FREEMEM-Kommentar in q9sysglob.h) -- das ist eine
 * interne Kernel-Implementierungsdetail, KEIN Kompatibilitaetserfordernis
 * (kein reales Modul haengt von der internen Arena-Struktur ab, nur von
 * den Syscall-Ergebnissen F$SRqMem/F$SRtMem liefern -- die existieren
 * hier noch nicht). Deshalb hier eine einfache, einzelne First-Fit-
 * Freiliste statt Groessenklassen -- funktional ausreichend, deutlich
 * weniger Code.
 *
 * Nur die Q9K_ALLOC_STANDARD-Variante implementiert (16-Byte-Aufloesung,
 * First-Fit). Q9K_ALLOC_BUDDY ist in q9kernel_config.h als gueltige
 * Wahl vorgesehen, hat aber noch KEINE Implementierung -- schlaegt
 * bewusst mit #error fehl statt still falschen/fehlenden Code zu
 * erzeugen (gleiche Disziplin wie ueberall in diesem Projekt: kein
 * stiller Fehldefault).
 *
 * Freiblock-Format (rein intern, keine Kompatibilitaetsanforderung --
 * kein reales Modul sieht diese Struktur je): Header aus zwei Q9_u32-
 * Feldern {next (0=Listenende), size (inkl. dieses Headers)} direkt am
 * Blockanfang, First-Fit-Suche, Aufteilen bei ausreichendem Restplatz.
 * WICHTIG (per Host-Test gefunden, nicht nur vermutet, 2026-08-18): der
 * Feldabstand ist ueberall sizeof(Q9_u32), NICHT hartkodiert "4" --
 * sizeof(Q9_u32) ist auf dem echten 32-Bit-Zielsystem 4 Byte, auf einem
 * 64-Bit-Host beim Testen aber 8 Byte. Ein erster Entwurf mit
 * hartkodiertem "+4" liess next/size auf dem Host ueberlappen (echter,
 * durch den Host-Test aufgedeckter Bug -- auf dem 32-Bit-Ziel waere er
 * unbemerkt geblieben, weil sizeof(Q9_u32)==4 dort zufaellig stimmt).
 * Kein Koaleszieren beim Freigeben (TODO fuer spaeter, s. Q9K_FreeMem)
 * -- fuer den aktuellen Bedarf (wenige, grosse Allokationen frueh im
 * Bootstrap, kaum Freigaben) ausreichend.
 */

#include "q9kernel_config.h"

#if !defined(Q9K_ALLOC_STANDARD)
#error "q9kernel_arena.c implementiert bisher nur Q9K_ALLOC_STANDARD -- Q9K_ALLOC_BUDDY fehlt noch"
#endif

typedef unsigned long Q9_u32;

#ifndef Q9_D_ARENA
#define Q9_D_ARENA      0x3FC   /* s. q9sysglob.h -- Kontrollblock-Basis; per
                                  * #ifndef ueberschreibbar, damit
                                  * test_q9kernel_arena.c auf einen echten,
                                  * beschreibbaren Testpuffer statt der festen
                                  * Zieladresse umbiegen kann -- aendert am
                                  * echten Kernel-Build nichts. */
#endif
/* Feldabstand hier bewusst hartkodiert "0x08"/"0x0C" (NICHT sizeof-
 * basiert wie beim internen Freiblock-Header oben) -- das sind ECHTE,
 * an der realen Kernel-Disassemblierung verifizierte Offsets (Thema 01,
 * Q9_D_FREEMEM = Q9_D_ARENA+8), auf dem echten 32-Bit-Zielsystem korrekt
 * (Felder dort genuin 4 Byte breit). Per #ifndef ueberschreibbar, weil
 * ein Host-Test mit einem breiteren Q9_u32 (s. Q9_D_ARENA-Kommentar
 * oben) hier sonst dieselbe Ueberlappung erzeugt wie beim Freiblock-
 * Header (per Host-Test gefunden, 2026-08-18) -- fuer den Test zaehlt
 * nur, dass Kopf/Schwanz zwei GETRENNTE, gueltige Speicherstellen sind,
 * nicht die exakte reale Byte-Distanz. */
#ifndef Q9K_ARENA_HEAD
#define Q9K_ARENA_HEAD  (Q9_D_ARENA + 0x08)  /* = Q9_D_FREEMEM, Kopf der Freiliste */
#endif
#ifndef Q9K_ARENA_TAIL
#define Q9K_ARENA_TAIL  (Q9_D_ARENA + 0x0C)
#endif

#define Q9K_ALLOC_GRANULARITY 16   /* Standard-Allocator, s. vendor/README.md */
#define Q9K_MIN_SPLIT_REMAINDER 16 /* kleinster Rest, der noch als eigener Freiblock lohnt */

static Q9_u32 Q9K_GetU32(Q9_u32 addr)
{
    return *(volatile Q9_u32 *)addr;
}

static void Q9K_SetU32(Q9_u32 addr, Q9_u32 value)
{
    *(volatile Q9_u32 *)addr = value;
}

static Q9_u32 Q9K_RoundUp16(Q9_u32 n)
{
    return (n + (Q9K_ALLOC_GRANULARITY - 1)) & ~(Q9_u32)(Q9K_ALLOC_GRANULARITY - 1);
}

/* Registriert EINEN freien Speicherblock in der Arena -- ersetzt den
 * selbstreferenzierenden Leerzustand. freeBase/freeSize muessen
 * mindestens Q9K_MIN_SPLIT_REMAINDER gross sein und selbst schon
 * 16-Byte-ausgerichtet sein (Aufrufers Verantwortung, keine automatische
 * Rundung hier -- die RAM-Grenzen sind Bootzeit-Konstanten, keine
 * Laufzeitwerte, die gerundet werden muessten). Darf nur EINMAL, frueh
 * im Bootstrap aufgerufen werden (kein Aufruf-Zaehler/Schutz dagegen --
 * TODO falls das je ein Problem wird). */
void Q9K_ArenaInit(Q9_u32 freeBase, Q9_u32 freeSize)
{
    if (freeSize < Q9K_MIN_SPLIT_REMAINDER)
        return; /* zu klein, um ueberhaupt einen Freiblock-Header aufzunehmen */

    Q9K_SetU32(freeBase, 0);          /* next = Listenende */
    Q9K_SetU32(freeBase + sizeof(Q9_u32), freeSize);
    Q9K_SetU32(Q9K_ARENA_HEAD, freeBase);
    Q9K_SetU32(Q9K_ARENA_TAIL, freeBase);
}

/* First-Fit-Allokation, rundet requestedSize auf 16 Byte auf. Gibt die
 * Adresse des allozierten Blocks zurueck, oder 0 bei Fehlschlag (kein
 * ausreichend grosser Block gefunden). */
Q9_u32 Q9K_AllocMem(Q9_u32 requestedSize)
{
    Q9_u32 needed = Q9K_RoundUp16(requestedSize);
    Q9_u32 prevAddr = 0;
    Q9_u32 curAddr = Q9K_GetU32(Q9K_ARENA_HEAD);

    while (curAddr != 0) {
        Q9_u32 curSize = Q9K_GetU32(curAddr + sizeof(Q9_u32));
        Q9_u32 curNext = Q9K_GetU32(curAddr);

        if (curSize >= needed) {
            Q9_u32 remainder = curSize - needed;

            if (remainder >= Q9K_MIN_SPLIT_REMAINDER) {
                /* Block aufteilen: hinterer Teil bleibt frei, vorderer
                 * Teil wird zurueckgegeben. */
                Q9_u32 newFreeAddr = curAddr + needed;
                Q9K_SetU32(newFreeAddr, curNext);
                Q9K_SetU32(newFreeAddr + sizeof(Q9_u32), remainder);
                if (prevAddr == 0)
                    Q9K_SetU32(Q9K_ARENA_HEAD, newFreeAddr);
                else
                    Q9K_SetU32(prevAddr, newFreeAddr);
                if (curNext == 0)
                    Q9K_SetU32(Q9K_ARENA_TAIL, newFreeAddr);
            } else {
                /* Rest zu klein fuer einen eigenen Freiblock -- ganzen
                 * Block hergeben (interne Fragmentierung, akzeptiert). */
                if (prevAddr == 0)
                    Q9K_SetU32(Q9K_ARENA_HEAD, curNext);
                else
                    Q9K_SetU32(prevAddr, curNext);
                if (curNext == 0)
                    Q9K_SetU32(Q9K_ARENA_TAIL, prevAddr);
            }

            return curAddr;
        }

        prevAddr = curAddr;
        curAddr = curNext;
    }

    return 0; /* kein ausreichend grosser Freiblock gefunden */
}

/* NACHTRAG 2026-08-30 (Abschnitt "F$SRqMem/F$SRtMem") -- Best-Fit-
 * Variante fuer die reale "d0.l=-1"-Sonderform von F$SRqMem ("the
 * largest block of free memory ... is allocated", 68k_tech.pdf S. 503):
 * durchsucht die GESAMTE Freiliste (nicht First-Fit wie Q9K_AllocMem),
 * gibt den GROESSTEN gefundenen Block KOMPLETT heraus (keine Aufteilung
 * -- das Manual erwaehnt fuer diesen Sonderfall keine, und der Sinn der
 * Anfrage ist ja gerade "so viel wie moeglich"). *outSize traegt danach
 * die echte, unaufgerundete Blockgroesse (kann groesser sein als jede
 * konkrete Anforderung). Rueckgabe 0 (mit *outSize=0) = Arena leer. */
Q9_u32 Q9K_AllocLargest(Q9_u32 *outSize)
{
    Q9_u32 prevAddr = 0;
    Q9_u32 curAddr = Q9K_GetU32(Q9K_ARENA_HEAD);
    Q9_u32 bestAddr = 0, bestSize = 0, bestPrev = 0, bestNext = 0;

    while (curAddr != 0) {
        Q9_u32 curSize = Q9K_GetU32(curAddr + sizeof(Q9_u32));
        Q9_u32 curNext = Q9K_GetU32(curAddr);

        if (curSize > bestSize) {
            bestSize = curSize;
            bestAddr = curAddr;
            bestPrev = prevAddr;
            bestNext = curNext;
        }

        prevAddr = curAddr;
        curAddr = curNext;
    }

    if (bestAddr == 0) {
        *outSize = 0;
        return 0;
    }

    if (bestPrev == 0)
        Q9K_SetU32(Q9K_ARENA_HEAD, bestNext);
    else
        Q9K_SetU32(bestPrev, bestNext);
    if (bestNext == 0)
        Q9K_SetU32(Q9K_ARENA_TAIL, bestPrev);

    *outSize = bestSize;
    return bestAddr;
}

/* Gibt einen zuvor per Q9K_AllocMem erhaltenen Block zurueck -- der
 * Aufrufer muss addr/size selbst im Kopf behalten (kein verstecktes
 * Zuteilungs-Header). Haengt den Block einfach vorn an die Freiliste --
 * KEIN Koaleszieren mit benachbarten Freibloecken (TODO fuer spaeter,
 * s. Kopfkommentar). */
void Q9K_FreeMem(Q9_u32 addr, Q9_u32 size)
{
    Q9_u32 oldHead;

    if (size < Q9K_MIN_SPLIT_REMAINDER)
        return; /* zu klein, um selbst als Freiblock zu dienen -- verloren, TODO */

    oldHead = Q9K_GetU32(Q9K_ARENA_HEAD);
    Q9K_SetU32(addr, oldHead);
    Q9K_SetU32(addr + sizeof(Q9_u32), size);
    Q9K_SetU32(Q9K_ARENA_HEAD, addr);
    if (oldHead == 0)
        Q9K_SetU32(Q9K_ARENA_TAIL, addr);
}
