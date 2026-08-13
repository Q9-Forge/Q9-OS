/*
 * q9moduleheader.h -- Q9-OS eigene, architekturuebergreifende Definition
 *                     des OS-9-Modul-Headers: OS-9/6809 (1980),
 *                     OS-9/68K (klassisch) und OS-9000 (universell,
 *                     x86/PowerPC/ARM/MIPS/SPARC/...).
 *
 * Ziel: Grundlage fuer ein "ident"-artiges Werkzeug, das Module aller
 * drei Generationen anhand des Sync-Worts erkennt und ausfuehrlich
 * beschreiben kann (Typ, Sprache, Groesse, Einsprungpunkt, ...) --
 * unabhaengig davon, von welcher OS-9-Generation/CPU-Architektur das
 * Modul stammt.
 *
 * Eigenstaendig aus den drei offiziellen Technical Manuals rekonstruiert
 * (OS-9 System Programmer's Manual 1983, Kapitel 4, fuer 6809; OS-9 for
 * 68K Processors Technical Manual, Table 1-6/1-7/1-8, fuer 68K; OS-9000
 * Technical Manual, Kapitel 1 (mh_com-Struct), fuer OS-9000) sowie durch
 * eigene Disassemblierung realer Kernel-Binaries verifiziert (siehe
 * docs/kernel-walkthrough/00-modul-aufbau-und-header/). KEIN Abdruck der
 * proprietaeren Microware-Quelldateien module.h/oskdefs.d (copyright-
 * geschuetzt, "Reproduction... strictly prohibited") -- nur die
 * (nicht schutzfaehigen) strukturellen Fakten Offset/Groesse/Bedeutung
 * uebernommen, alle Namen und Beschreibungstexte eigene Formulierungen.
 * Fuer die Level-2-Variante (Modul-DAT-Image etc.) siehe NitrOS-9s
 * offenen Quelltext, hier nicht benoetigt.
 *
 * Statuskennzeichnung je Feld:
 *   [VERIFIZIERT] -- an einem echten Binary geprueft (68K: dker030s,
 *                    OS-9000: live extrahierter x86-Kernel)
 *   [HANDBUCH]    -- aus dem jeweiligen offiziellen Manual, nicht an
 *                    einem eigenen Binary nachgeprueft (betrifft vor
 *                    allem 6809 -- kein Kernel-Binary vorhanden)
 *   [PLATZHALTER] -- Struktur aus dem Manual uebernommen, aber am
 *                    konkreten Kernel-Modul nicht mit Wert belegt/nicht
 *                    verifizierbar (z.B. Felder, die nur bei anderen
 *                    Modultypen benutzt werden)
 */

#ifndef Q9MODULEHEADER_H
#define Q9MODULEHEADER_H

#include <stdint.h>

/* ====================================================================
 * Sync-Werte je Generation
 * ==================================================================== */
#define Q9_MH_SYNC_6809     0x87CD  /* OS-9/6809, 1980 [HANDBUCH, cross-verifiziert: NitrOS-9-Quelltext module.d, M$ID1=$87/M$ID2=$CD] */
#define Q9_MH_SYNC_OS9      0x4AFC  /* OS-9/68K UND alle OS-9000-Ports (x86/PowerPC/ARM/...) -- derselbe Wert, Byte-Reihenfolge je nach Ziel-Endianness [VERIFIZIERT fuer 68K+x86, HANDBUCH/Portierungskonzept (v_endflag) fuer PowerPC/ARM] */

/* CRC-Polynom -- ueber 6809 UND 68K hinweg identisch dokumentiert [HANDBUCH] */
#define Q9_MH_CRC_POLY      0x800FE3L

/* ====================================================================
 * Modultyp-Codes -- ueber ALLE drei Generationen und ~28 Jahre hinweg
 * wortwoertlich unveraendert (bemerkenswerteste Konstante der ganzen
 * OS-9-Familie, siehe kernel-walkthrough Thema 00)
 * ==================================================================== */
#define Q9_MT_ANY           0x0
#define Q9_MT_PROGRAM       0x1
#define Q9_MT_SUBROUT       0x2
#define Q9_MT_MULTI         0x3
#define Q9_MT_DATA          0x4
#define Q9_MT_CSDDATA       0x5
#define Q9_MT_TRAPLIB       0xB
#define Q9_MT_SYSTEM        0xC  /* Systm -- Kernel/IOMan/SSM/SCF/... */
#define Q9_MT_FILEMAN       0xD  /* Flmgr -- RBF/PCF/CDFM/... */
#define Q9_MT_DEVDRVR       0xE  /* Drivr */
#define Q9_MT_DEVDESC       0xF  /* Devic */

/* ====================================================================
 * Sprach-Codes -- ebenfalls ueber alle drei Generationen identisch
 * ==================================================================== */
#define Q9_ML_ANY           0x0
#define Q9_ML_OBJECT        0x1  /* Maschinencode der jeweiligen CPU */
#define Q9_ML_ICODE         0x2  /* BASIC09 I-Code */
#define Q9_ML_PCODE         0x3
#define Q9_ML_CCODE         0x4
#define Q9_ML_CBLCODE       0x5
#define Q9_ML_FRTNCODE      0x6

/* ====================================================================
 * Attribut-Bits
 * ==================================================================== */
#define Q9_MA_REENT         0x80  /* reentrant/sharable -- bereits 1980 bei 6809 dokumentiert */
#define Q9_MA_SUPER         0x20  /* system-state -- erst ab 68K dokumentiert (6809 kennt nur MA_REENT) */

/* ====================================================================
 * Header-Layout 1: OS-9/6809 (1980, Motorola 6809, 8-Bit)
 * Nur 9 Byte Standard-Header, danach typabhaengige Erweiterung.
 * Quelle: OS-9 System Programmer's Manual (1983), Kapitel 4.
 * [HANDBUCH] -- kein eigenes 6809-Kernel-Binary vorhanden, Layout aber
 * durch echten NitrOS-9-Kernel-Quelltext (krn.asm) plausibilisiert.
 * ==================================================================== */
#define Q9_MH6809_SYNC      0x00  /* 2 Byte */
#define Q9_MH6809_SIZE      0x02  /* 2 Byte -- nur 16 Bit, 6809 hat 64-KB-Adressraum */
#define Q9_MH6809_NAME      0x04  /* 2 Byte, Offset zum Namen -- High-Bit-terminiert (letztes Zeichen mit Bit 7 gesetzt) */
#define Q9_MH6809_TYLANG    0x06  /* 1 Byte: oberes Nibble=Typ, unteres Nibble=Sprache */
#define Q9_MH6809_ATTREV    0x07  /* 1 Byte: oberes Nibble=Attribut (nur Bit 7 dokumentiert), unteres Nibble=Revision (0-15) */
#define Q9_MH6809_PARITY    0x08  /* 1 Byte, XOR-Pruefsumme -- Standard-Header endet hier */
#define Q9_MH6809_STDLEN    0x09  /* Laenge des Standard-Headers in Byte */
/* Typabhaengige Erweiterung (User-Module 0-9; laut Manual auch fuer
 * Systm/Flmgr/Drivr/Devic verwendet, aber nicht extra tabelliert) */
#define Q9_MH6809_EXEC      0x09  /* 2 Byte, Einsprungoffset */
#define Q9_MH6809_STORSZ    0x0B  /* 2 Byte, Speicherbedarf/Stackgroesse [PLATZHALTER fuer Systm] */

/* ====================================================================
 * Header-Layout 2: OS-9/68K (klassisch -- dieses Projekts Hauptziel,
 * dker030s). 46 Byte Standard-Header + optionale, typabhaengige
 * Erweiterung. Quelle: OS-9 for 68K Processors Technical Manual,
 * Table 1-6/1-7/1-8.
 * ==================================================================== */
#define Q9_MH68K_SYNC       0x00  /* 2 Byte [VERIFIZIERT] */
#define Q9_MH68K_SYSREV     0x02  /* 2 Byte [VERIFIZIERT] */
#define Q9_MH68K_SIZE       0x04  /* 4 Byte [VERIFIZIERT] */
#define Q9_MH68K_OWNER      0x08  /* 4 Byte [VERIFIZIERT] */
#define Q9_MH68K_NAME       0x0C  /* 4 Byte, Offset zum Namen -- NUL-terminiert (KEIN High-Bit wie bei 6809!) [VERIFIZIERT] */
#define Q9_MH68K_ACCESS     0x10  /* 2 Byte [VERIFIZIERT] */
#define Q9_MH68K_TYPE       0x12  /* 1 Byte [VERIFIZIERT] */
#define Q9_MH68K_LANG       0x13  /* 1 Byte [VERIFIZIERT] */
#define Q9_MH68K_ATTR       0x14  /* 1 Byte [VERIFIZIERT] */
#define Q9_MH68K_REVS       0x15  /* 1 Byte [VERIFIZIERT] */
#define Q9_MH68K_EDIT       0x16  /* 2 Byte [VERIFIZIERT] */
#define Q9_MH68K_USAGE      0x18  /* 4 Byte, Offset Kommentar-String (unbenutzt) [HANDBUCH] */
#define Q9_MH68K_SYMBOL     0x1C  /* 4 Byte, Offset Symboltabelle (reserviert) [HANDBUCH] */
#define Q9_MH68K_IDENT      0x20  /* 2 Byte, unbenutzt [HANDBUCH] */
/* 0x22-0x27: 6 Byte reserviert */
#define Q9_MH68K_HDEXT      0x28  /* 4 Byte, Offset Header-Erweiterung [HANDBUCH] */
#define Q9_MH68K_HDEXTSZ    0x2C  /* 2 Byte, Groesse der Header-Erweiterung [HANDBUCH] */
#define Q9_MH68K_PARITY     0x2E  /* 2 Byte -- Standard-Header endet hier [VERIFIZIERT] */
#define Q9_MH68K_STDLEN     0x30  /* Laenge des Standard-Headers in Byte */
/* Typabhaengige Erweiterung -- laut Manual NUR M$Exec/M$Excpt offiziell
 * fuer Program/TrapLib/Drivr/Flmgr/Systm definiert: */
#define Q9_MH68K_EXEC       0x30  /* 4 Byte, Einsprungoffset [VERIFIZIERT] */
#define Q9_MH68K_EXCPT      0x34  /* 4 Byte, Trap-Einsprung fuer unbehandelte User-Traps [VERIFIZIERT, Wert 0 am Kernel] */
/* Nur bei Program/TrapLib/Drivr (laut Manual NICHT bei Systm/Flmgr): */
#define Q9_MH68K_MEM        0x38  /* 4 Byte, Speicherbedarf [PLATZHALTER -- Systm-Module nutzen dieses Feld laut Manual nicht] */
#define Q9_MH68K_STACK      0x3C  /* 4 Byte, Stackgroesse [PLATZHALTER] */

/* ====================================================================
 * Header-Layout 3: OS-9000 universell (x86/PowerPC/ARM/MIPS/SPARC/...)
 * 88 Byte, EIN gemeinsamer Header fuer JEDEN Modultyp (mh_com-Konzept).
 * Quelle: OS-9000 Technical Manual, Kapitel 1.
 * Byte-Reihenfolge der Mehrbyte-Felder folgt der nativen Endianness
 * des Ziels (Little-Endian bei x86, Big-Endian bei PowerPC/ARM).
 * ==================================================================== */
#define Q9_MH9K_SYNC        0x00  /* 2 Byte [VERIFIZIERT, x86] */
#define Q9_MH9K_SYSREV      0x02  /* 2 Byte [VERIFIZIERT, x86] */
#define Q9_MH9K_SIZE        0x04  /* 4 Byte [VERIFIZIERT, x86] */
#define Q9_MH9K_OWNER       0x08  /* 4 Byte [VERIFIZIERT, x86] */
#define Q9_MH9K_NAME        0x0C  /* 4 Byte, Offset zum Namen -- NUL-terminiert [VERIFIZIERT, x86] */
#define Q9_MH9K_ACCESS      0x10  /* 2 Byte [VERIFIZIERT, x86] */
#define Q9_MH9K_TYLANG      0x12  /* 2 Byte: konzeptionell (Typ<<8)|Sprache, Byte-Reihenfolge endian-abhaengig [VERIFIZIERT, x86] */
#define Q9_MH9K_ATTREV      0x14  /* 2 Byte: konzeptionell (Attribut<<8)|Revision, ebenso endian-abhaengig [VERIFIZIERT, x86] */
#define Q9_MH9K_EDIT        0x16  /* 2 Byte [VERIFIZIERT, x86] */
#define Q9_MH9K_NEEDS       0x18  /* 4 Byte, Hardware-Anforderungsflags [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_SHARE       0x1C  /* 4 Byte, Offset Shared-Data [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_SYMBOL      0x20  /* 4 Byte, Offset Symboltabelle [VERIFIZIERT, x86, Wert 0 = kein Symbol-Build] */
#define Q9_MH9K_EXEC        0x24  /* 4 Byte, Einsprungoffset -- fuer JEDEN Modultyp vorhanden [VERIFIZIERT, x86] */
#define Q9_MH9K_EXCPT       0x28  /* 4 Byte, Trap-Einsprung [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_DATA        0x2C  /* 4 Byte, Datenbereichsgroesse [VERIFIZIERT, x86] */
#define Q9_MH9K_STACK       0x30  /* 4 Byte, Stackgroesse [VERIFIZIERT, x86] */
#define Q9_MH9K_IDATA       0x34  /* 4 Byte, Offset initialisierte Daten [VERIFIZIERT, x86] */
#define Q9_MH9K_IDREF       0x38  /* 4 Byte, Offset Datenreferenzlisten [VERIFIZIERT, x86] */
#define Q9_MH9K_INIT        0x3C  /* 4 Byte [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_TERM        0x40  /* 4 Byte [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_DBIAS       0x44  /* 4 Byte [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_CBIAS       0x48  /* 4 Byte [VERIFIZIERT, x86, Wert 0 am Kernel] */
#define Q9_MH9K_IDENT       0x4C  /* 2 Byte, unbenutzt [VERIFIZIERT, x86] */
/* 0x4E-0x55: 8 Byte reserviert */
#define Q9_MH9K_PARITY      0x56  /* 2 Byte -- Header endet hier [VERIFIZIERT, x86] */
#define Q9_MH9K_STDLEN      0x58  /* Laenge des Standard-Headers in Byte */

/* ====================================================================
 * Struct-Varianten fuer host-seitiges C-Tooling (z.B. ident-artige
 * Scanner). Feldreihenfolge entspricht exakt den Offset-Defines oben --
 * ACHTUNG: reines memcpy/Pointer-Casting funktioniert nur auf einem
 * Host mit passender Endianness und ohne Struct-Padding-Ueberraschungen;
 * fuer produktiven Einsatz einzelne Felder ueber die Offset-Defines und
 * eine explizite Lese-Funktion (Big-/Little-Endian) auslesen, nicht
 * direkt ueberlagern.
 * ==================================================================== */

typedef struct {
    uint16_t sync;
    uint16_t size;
    uint16_t nameOffset;
    uint8_t  tyLang;     /* oberes Nibble Typ, unteres Nibble Sprache */
    uint8_t  attRev;     /* oberes Nibble Attribut, unteres Nibble Revision */
    uint8_t  parity;
} Q9_ModHead6809;

typedef struct {
    uint16_t sync;
    uint16_t sysRev;
    uint32_t size;
    uint32_t owner;
    uint32_t nameOffset;
    uint16_t access;
    uint8_t  type;
    uint8_t  lang;
    uint8_t  attr;
    uint8_t  revs;
    uint16_t edit;
    uint32_t usageOffset;
    uint32_t symbolOffset;
    uint16_t ident;
    uint8_t  spare[6];
    uint32_t hdExtOffset;
    uint16_t hdExtSize;
    uint16_t parity;
    /* ab hier optional, nur bei Program/TrapLib/Drivr/Flmgr/Systm vorhanden: */
    uint32_t execOffset;
    uint32_t excptOffset;
} Q9_ModHead68K;

typedef struct {
    uint16_t sync;
    uint16_t sysRev;
    uint32_t size;
    uint32_t owner;
    uint32_t nameOffset;
    uint16_t access;
    uint16_t tyLang;      /* endian-abhaengig gespeichert, s.o. */
    uint16_t attRev;      /* endian-abhaengig gespeichert, s.o. */
    uint16_t edit;
    uint32_t needs;
    uint32_t shareOffset;
    uint32_t symbolOffset;
    uint32_t execOffset;
    uint32_t excptOffset;
    uint32_t dataSize;
    uint32_t stackSize;
    uint32_t idataOffset;
    uint32_t idrefOffset;
    uint32_t initOffset;
    uint32_t termOffset;
    uint32_t dbias;
    uint32_t cbias;
    uint16_t ident;
    uint8_t  spare[8];
    uint16_t parity;
} Q9_ModHeadOS9000;

/* ====================================================================
 * Format-Erkennung -- liest die ersten 2 Byte roh (ohne Endian-Annahme)
 * und vergleicht gegen alle bekannten Sync-Werte in beiden moeglichen
 * Byte-Reihenfolgen. Reine Deklaration hier; Implementierung folgt in
 * einem spaeteren Schritt (q9moduleheader.c), sobald das eigentliche
 * ident-Werkzeug darauf aufbaut.
 * ==================================================================== */
typedef enum {
    Q9_MHFMT_UNKNOWN = 0,
    Q9_MHFMT_6809,       /* Sync $87CD */
    Q9_MHFMT_68K,        /* Sync $4AFC, Big-Endian, 46-Byte-Standardheader */
    Q9_MHFMT_OS9000_BE,  /* Sync $4AFC, Big-Endian, 88-Byte-Standardheader (PowerPC/ARM/...) */
    Q9_MHFMT_OS9000_LE   /* Sync $FC4A on-disk (=$4AFC), Little-Endian, 88-Byte-Standardheader (x86) */
} Q9_ModHeadFormat;

Q9_ModHeadFormat Q9_DetectModuleHeaderFormat(const uint8_t *rawBytes, uint32_t availableLen);

#endif /* Q9MODULEHEADER_H */
