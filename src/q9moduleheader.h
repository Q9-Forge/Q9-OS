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
#define Q9_MH_SYNC_Q9OWN    0x5139  /* Q9-eigenes Modulformat -- ASCII "Q9" ($51='Q', $39='9'), bewusst kollisionsfrei zu obigen beiden gewaehlt [ENTWURF, Planungsgespraech 2026-08-16]. Byte-Reihenfolge je nach abiClass-Endian-Bit -- Big-Endian-Ziele lesen im Hex-Dump woertlich "Q9", Little-Endian-Ziele "9Q", exakt dasselbe kosmetische Verhalten wie $4AFC/$FC4A oben. */

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
#define Q9_MH68K_MEM        0x38  /* 4 Byte, Speicherbedarf (M$Mem) [VERIFIZIERT, 2026-08-21 -- s. src/kernel/forkchild.a: reale r68/l68-Assemblierung mit genau EINEM vsect-Feld (ds.l 1 = 4 Byte) ergab im gelinkten Binary exakt 0x00000004 an diesem Offset -- vom Assembler AUTOMATISCH aus der Gesamtgroesse aller deklarierten vsect-Bloecke berechnet, NICHT ueber einen expliziten psect-Parameter (Gegenprobe am eigenen Kernel-Modul: q9kernel_entry.a's "psect ...,0,..." mit dessen einzigem vsect [_stklimit, ds.l 1] ergab ebenfalls 4, obwohl der psect-Parameter selbst 0 war)] */
#define Q9_MH68K_STACK      0x3C  /* 4 Byte, Stackgroesse (M$Stack) [VERIFIZIERT, 2026-08-21 -- s. src/kernel/forkchild.a: "psect forkchild,...,Q9K_ForkChildStackSize,..." mit Q9K_ForkChildStackSize=$800 ergab im real gelinkten Binary exakt 0x00000800 an diesem Offset -- die 5. psect-Zahl (in q9_cstart.a explizit "StackSize" genannt) setzt ALSO NICHT "mem/stack" gemeinsam, sondern NUR M$Stack; M$Mem kommt separat aus den vsects, s. o.] */

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
 * Header-Layout 4: Q9-eigenes Format [ENTWURF -- Planungsgespraech
 * 2026-08-16, NICHT reverse-engineered, keine Fremdquelle]. Uebernimmt
 * bewusst das Grundschema von Layout 2/3 (sync/hdrVersion/size/owner/
 * name/access/type/lang/attr/revs/edit/exec/except/data/stack + optionaler
 * Erweiterungsblock ueber hdExtOffset/hdExtSize -- dasselbe Prinzip, das
 * der 68K-Header schon selbst mitbringt), mit drei neuen Grundideen:
 *
 * 1. 16-Bit-taugliches `type`-Feld statt Nibble: Werte 0x00-0x0F sind
 *    WORT-IDENTISCH zu Q9_MT_* oben (0x0C-0x0F bleiben fuer den Dreiklang
 *    zwingend reserviert, siehe Abschnitt 1 von OWN_KERNEL_INIT_PLAN.md).
 *    0x10 = Q9_MT_BOOTLOADER, die bisher einzige zusaetzliche Top-Level-
 *    Modulart [ENTWURF, 2026-08-16] -- fuer alles andere Neue (Netzwerk,
 *    Systemmonitor-Streaming, /proc) reicht ein gewoehnliches Fmgr (0x0D)
 *    plus dem neuen `subType`-Feld (Q9_SUBTYPE_*, analog zu OS-9000s
 *    DT_*-Konzept, aber eigene Nummerierung), statt den Top-Level-
 *    Namensraum aufzublaehen.
 * 2. Ein `abiClass`-Byte direkt nach `hdrVersion`, noch VOR jedem
 *    breitenabhaengigen Feld (dasselbe Prinzip wie ELFs `EI_CLASS`),
 *    kodiert Pointerbreite UND Endianness in einem Feld ("zusammen
 *    verwaltet"):
 *      Bit [1:0]  Pointerbreite: 00=16 Bit (6809-Erbe), 01=32 Bit
 *                 (68K/RISC-V32/x86-32), 10=64 Bit (ARM64/x86-64/
 *                 kuenftig), 11=reserviert
 *      Bit [2]    Endianness: 0=Big-Endian, 1=Little-Endian
 *      Bit [7:3]  reserviert
 *    Fuenf real genutzte Kombinationen: 16BE/32BE/32LE/64BE/64LE (16LE
 *    absichtlich nicht vorgesehen -- keine bekannte Little-Endian-6809-
 *    Historie). Die aus dem Sync-Wort-Byte-Vergleich implizit erkannte
 *    Endianness kann gegen dieses Bit quergeprueft werden (billiger
 *    Konsistenz-Check gegen beschaedigte Header).
 * 3. Ein optionaler Q9-Erweiterungsblock (Q9_ModHeadOwnExt) fuer alles,
 *    was ueber die OS-9/9000-Vorgabe hinausgeht: erweiterte Rechte
 *    (Owner/Group/World, Bit-Layout an OS-9000s PERM_*-Konzept angelehnt,
 *    aber eigene, neu vergebene Bitwerte -- keine Uebernahme proprietaerer
 *    Microware-Konstanten), SMP-Metadaten, Zielarchitektur-Kennung,
 *    Adressierungsmodell-Flag.
 *
 * `nameOffset` bewusst NUL-terminiert (wie Layout 2/3), auch in der
 * 16-Bit-Variante -- KEINE Uebernahme von 6809s High-Bit-Terminierung,
 * einheitlicher Parser wichtiger als Werktreue zum 6809-Vorbild.
 *
 * Feldbreite skaliert mit abiClass fuer alle Offset-/Groessenfelder
 * (size/owner/nameOffset/execOffset/exceptOffset/dataSize/stackSize/
 * idataOffset/idrefOffset/hdExtOffset/hdExtSize); sync/hdrVersion/
 * abiClass/access/type/lang/subType/attr/revs/edit/parity bleiben in
 * allen drei Breitenvarianten gleich breit (reine Ein-Byte-/Zwei-Byte-
 * Werte, keine Zeiger). Deshalb DREI eigene Struct-Varianten statt eines
 * gemeinsamen
 * Offset-Schemas -- echte Byte-Offsets verschieben sich je Variante,
 * exakt wie bei ELFs Elf32_Ehdr/Elf64_Ehdr.
 * ==================================================================== */

/* abiClass-Bitmasken */
#define Q9_ABICLASS_WIDTH_MASK   0x03
#define Q9_ABICLASS_WIDTH_16     0x00
#define Q9_ABICLASS_WIDTH_32     0x01
#define Q9_ABICLASS_WIDTH_64     0x02
#define Q9_ABICLASS_ENDIAN_MASK  0x04
#define Q9_ABICLASS_ENDIAN_BE    0x00
#define Q9_ABICLASS_ENDIAN_LE    0x04

/* Typ-Codes: 0x00-0x0F identisch zu Q9_MT_* oben, 0x10+ frei fuer eigene
 * Q9-native Modularten (Abschnitt 5 des Plans) */
#define Q9_MT_LEGACY_MAX         0x0F
#define Q9_MT_BOOTLOADER         0x10  /* [ENTWURF, 2026-08-16] Q9-eigener Bootlader -- laeuft VOR dem Kernel, legt laut Thema 10 den System-Global-Bereich an, aber mit demselben Sync-/Pruefsummen-Verfahren validiert wie ein regulaeres Modul (ein Scanner fuer beide Faelle) -- keine Dreiklang-Rolle, deshalb eigener Top-Level-Typ statt Sub-Type */
/* 0x11+ weiterhin frei, noch keine weitere eigene Top-Level-Modulart als
 * noetig identifiziert -- die meisten neuen Ideen (Netzwerk, Systemmonitor-
 * Streaming, /proc) passen als gewoehnliches Fmgr (0x0D) + subType, s.u.,
 * statt einen eigenen Top-Level-Typ zu brauchen */

/* Sub-Type: zweite Klassifizierungsachse, analog zu OS-9000s DT_*-Konzept
 * (DT_NFM/DT_SOCK/... in os9k_tech.pdf), aber EIGENE Nummerierung -- keine
 * Microware-Werte uebernommen, gleiches Prinzip wie bei Q9_RIGHTS_* oben.
 * Nur fuer Fmgr-Module (Q9_MT_FILEMAN) relevant; bei allen anderen Typen
 * 0/Q9_SUBTYPE_NONE. Haelt den Top-Level-`type`-Namensraum klein: neue
 * "Arten von Datei-Manager" brauchen keinen neuen Top-Level-Typ, nur einen
 * neuen Sub-Type-Wert. */
#define Q9_SUBTYPE_NONE          0x00
#define Q9_SUBTYPE_RBF_COMPAT    0x01  /* eigenes RBF-Aequivalent, falls kein reales RBF-Modul wiederverwendet wird */
#define Q9_SUBTYPE_NETFM         0x02  /* Netzwerk-File-Manager, s. Abschnitt 6 -- bleibt Dreiklang-Ebene, nie Kernel */
#define Q9_SUBTYPE_SOCK          0x03  /* Socket-Communication-Manager */
#define Q9_SUBTYPE_SYSMON        0x04  /* Kernel-Table-Streaming/Lockless-Ringpuffer, "/pipe/sysmon"-Idee */
#define Q9_SUBTYPE_PROCFS        0x05  /* virtuelles /proc-artiges Text-Dateisystem */
/* 0x06+ frei */

/* Zielarchitektur-Kennung (Erweiterungsblock) */
#define Q9_ARCH_6809      0
#define Q9_ARCH_68K       1
#define Q9_ARCH_RISCV32   2
#define Q9_ARCH_ARM64     3
#define Q9_ARCH_X86_32    4
#define Q9_ARCH_X86_64    5

/* Adressierungsmodell (Erweiterungsblock) -- siehe OWN_KERNEL_INIT_PLAN.md
 * Abschnitt 2d/6 */
#define Q9_ADDRMODEL_FLAT_SSM_COMPAT  0  /* SSM-kompatibel, Legacy-Treiber */
#define Q9_ADDRMODEL_Q9_NATIVE_PAGED  1  /* eigenes Paging-Modell */

/* SMP-Flags (Erweiterungsblock) -- Praefix SMP_, bewusst NICHT MP_
 * (MP_ ist real schon Module Permission in os9k_tech.pdf, siehe
 * OWN_KERNEL_INIT_PLAN.md Abschnitt 6) */
#define Q9_SMP_SAFE            0x0001  /* Code ist selbst SMP-sicher */
#define Q9_SMP_NEEDS_LOCK      0x0002  /* braucht externe Synchronisation */
#define Q9_SMP_UP_ONLY         0x0004  /* nur Single-Core, z.B. Legacy-Shim */

/* Erweiterte Rechte (Erweiterungsblock) -- Owner/Group/World x Read/
 * Write/Search/Execute, Reihenfolge an OS-9000s PERM_*-Konzept angelehnt,
 * aber EIGENE, neu vergebene Bitwerte (keine Microware-Konstanten
 * uebernommen) */
#define Q9_RIGHTS_OWNER_READ    0x0001
#define Q9_RIGHTS_OWNER_WRITE   0x0002
#define Q9_RIGHTS_OWNER_SRCH    0x0004
#define Q9_RIGHTS_OWNER_EXEC    0x0008
#define Q9_RIGHTS_GROUP_READ    0x0010
#define Q9_RIGHTS_GROUP_WRITE   0x0020
#define Q9_RIGHTS_GROUP_SRCH    0x0040
#define Q9_RIGHTS_GROUP_EXEC    0x0080
#define Q9_RIGHTS_WORLD_READ    0x0100
#define Q9_RIGHTS_WORLD_WRITE   0x0200
#define Q9_RIGHTS_WORLD_SRCH    0x0400
#define Q9_RIGHTS_WORLD_EXEC    0x0800
/* Bit 12-31: reserviert fuer eine kuenftige Capability-Erweiterung,
 * s. rightsVersion im Erweiterungsblock */

typedef struct {
    uint16_t sync;
    uint8_t  hdrVersion;
    uint8_t  abiClass;
    uint16_t size;
    uint16_t owner;
    uint16_t nameOffset;
    uint16_t access;
    uint16_t type;
    uint16_t lang;
    uint16_t subType;   /* Q9_SUBTYPE_*, nur bei Q9_MT_FILEMAN relevant */
    uint8_t  attr;
    uint8_t  revs;
    uint16_t edit;
    uint16_t execOffset;
    uint16_t exceptOffset;
    uint16_t dataSize;
    uint16_t stackSize;
    uint16_t idataOffset;
    uint16_t idrefOffset;
    uint16_t hdExtOffset;
    uint16_t hdExtSize;
    uint16_t parity;
} Q9_ModHeadOwn16;   /* abiClass Breite = Q9_ABICLASS_WIDTH_16 */

typedef struct {
    uint16_t sync;
    uint8_t  hdrVersion;
    uint8_t  abiClass;
    uint32_t size;
    uint32_t owner;
    uint32_t nameOffset;
    uint16_t access;
    uint16_t type;
    uint16_t lang;
    uint16_t subType;   /* Q9_SUBTYPE_*, nur bei Q9_MT_FILEMAN relevant */
    uint8_t  attr;
    uint8_t  revs;
    uint16_t edit;
    uint32_t execOffset;
    uint32_t exceptOffset;
    uint32_t dataSize;
    uint32_t stackSize;
    uint32_t idataOffset;
    uint32_t idrefOffset;
    uint32_t hdExtOffset;
    uint32_t hdExtSize;
    uint16_t parity;
} Q9_ModHeadOwn32;   /* abiClass Breite = Q9_ABICLASS_WIDTH_32 */

typedef struct {
    uint16_t sync;
    uint8_t  hdrVersion;
    uint8_t  abiClass;
    uint64_t size;
    uint64_t owner;
    uint64_t nameOffset;
    uint16_t access;
    uint16_t type;
    uint16_t lang;
    uint16_t subType;   /* Q9_SUBTYPE_*, nur bei Q9_MT_FILEMAN relevant */
    uint8_t  attr;
    uint8_t  revs;
    uint16_t edit;
    uint64_t execOffset;
    uint64_t exceptOffset;
    uint64_t dataSize;
    uint64_t stackSize;
    uint64_t idataOffset;
    uint64_t idrefOffset;
    uint64_t hdExtOffset;
    uint64_t hdExtSize;
    uint16_t parity;
} Q9_ModHeadOwn64;   /* abiClass Breite = Q9_ABICLASS_WIDTH_64 */

/* Q9-Erweiterungsblock -- ueber hdExtOffset/hdExtSize erreichbar, fuer
 * alle drei Breitenvarianten identisch (enthaelt selbst keine breiten-
 * abhaengigen Zeigerfelder) */
typedef struct {
    uint8_t  extVersion;
    uint8_t  spare0;
    uint32_t rights;          /* Q9_RIGHTS_* */
    uint8_t  rightsVersion;
    uint8_t  spare1[3];
    uint64_t cpuAffinityMask; /* bitweise, bis zu 64 Kerne, unabhaengig von abiClass */
    uint16_t smpFlags;        /* Q9_SMP_* */
    uint16_t cpuArch;         /* Q9_ARCH_* */
    uint8_t  addrModel;       /* Q9_ADDRMODEL_* */
    uint8_t  spare2[7];
} Q9_ModHeadOwnExt;

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
    Q9_MHFMT_OS9000_LE,  /* Sync $FC4A on-disk (=$4AFC), Little-Endian, 88-Byte-Standardheader (x86) */
    Q9_MHFMT_Q9OWN       /* Sync $5139/$3951 ("Q9") [ENTWURF] -- Breite/Endianness NICHT ueber separate Enum-Werte unterschieden wie bei OS9000_BE/LE oben, sondern aus dem selbstbeschreibenden abiClass-Byte gelesen (s. Q9_ABICLASS_*) */
} Q9_ModHeadFormat;

Q9_ModHeadFormat Q9_DetectModuleHeaderFormat(const uint8_t *rawBytes, uint32_t availableLen);

/* Liest den NUL-terminierten Namensstring eines 68K-/OS-9000-/Q9-eigenen
 * Moduls (alle drei: Offset-Feld + NUL-terminiert, 6809 bewusst NICHT
 * unterstuetzt -- High-Bit-Terminierung, andere Fehlerbehandlung noetig).
 * Implementierung in q9moduleheader.c. */
uint32_t Q9_ReadModuleName(const uint8_t *rawBytes, uint32_t availableLen,
                            uint32_t nameOffset, int littleEndian,
                            char *dest, uint32_t destSize);

/* Pruefsummen-Berechnung/-Verifikation fuer klassische 68K-Header
 * (24-Word-XOR ueber die ersten Q9_MH68K_STDLEN Byte, muss $FFFF
 * ergeben). Q9_ComputeModuleChecksum68K = Verifikation (Ergebnis mit
 * echtem M$Parity == 0xFFFF pruefen); Q9_ComputeRequiredParity68K =
 * "Setzen" (liefert den noetigen M$Parity-Wert fuer ein neu gebautes
 * Modul). Implementierung in q9moduleheader.c. */
uint16_t Q9_ComputeModuleChecksum68K(const uint8_t *rawBytes, uint32_t availableLen);
uint16_t Q9_ComputeRequiredParity68K(const uint8_t *rawBytes, uint32_t availableLen);

#endif /* Q9MODULEHEADER_H */
