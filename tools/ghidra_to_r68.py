#!/usr/bin/env python3
import re, sys

IN = "/Volumes/SSD1TB/projects/Q9-OS-ghidra/full_dump2.tsv"
OUT = "/Volumes/SSD1TB/projects/Q9-OS/src/kernel/kernel_body_auto.r"

rows = []
with open(IN, encoding="utf-8") as f:
    for line in f:
        parts = line.rstrip("\n").split("\t")
        while len(parts) < 6:
            parts.append("")
        off, length, hexb, kind, mnem, rest = parts[0], int(parts[1]), parts[2], parts[3], parts[4], parts[5]
        ops = parts[5] if kind == "I" else ""
        ref = parts[6] if len(parts) > 6 else ""
        rows.append(dict(off=off, length=length, hexb=hexb, kind=kind, mnem=mnem, ops=ops, ref=ref))

# Korrektur: Spaltenaufteilung robuster nachbauen, da Instruction-Zeilen
# eine Tab-Spalte mehr haben (mnem, ops, ref) als Data-Zeilen (mnem, "", "")
rows = []
with open(IN, encoding="utf-8") as f:
    for line in f:
        p = line.rstrip("\n").split("\t")
        off = p[0]; length = int(p[1]); hexb = p[2]; kind = p[3]
        if kind == "I":
            mnem = p[4]; ops = p[5] if len(p) > 5 else ""; ref = p[6] if len(p) > 6 else ""
        else:
            mnem = p[4] if len(p) > 4 else ""; ops = ""; ref = ""
        rows.append(dict(off=off, length=length, hexb=hexb, kind=kind, mnem=mnem, ops=ops, ref=ref))

addr2idx = {r["off"]: i for i, r in enumerate(rows)}

# Manuelle Aufloesung fuer PC-relativ-indizierte Operanden, bei denen
# Ghidra keine Referenz liefert (dynamisches Sprungziel). Von Hand per
# Adressrechnung bestimmt, siehe REVERSE_ENGINEERING.md fuer den
# jeweiligen Kontext.
PCREL_INDEXED_OVERRIDE = {
    "000944": "0008c0",  # Format-Groessen-Tabelle (Q9_disp_8d0-Kontext)
    "000b18": "000b3a",  # EA-Decoder-Tabelle (siehe 0xb3a-Fund)
    "000b1c": "000b3a",
}
CURRENT_INSTR_OFF = ""

# BTST/BCHG/BCLR/BSET mit Immediate verwenden intern immer ein volles
# Wort, auch wenn nur das niederwertige Byte semantisch die Bitnummer
# ist. Ghidra zeigt manchmal nur das niederwertige Byte vorzeichen-
# behaftet an und "verschluckt" damit ein von Null verschiedenes
# oberes Byte -- hier per Hand aus den Rohbytes korrigiert.
BIT_IMMEDIATE_OVERRIDE = {
    "003c6e": "0d8e",  # btst.b #0x0d8e,(0x1c,A4) -- oberes Byte 0x0d sonst verloren
}

# Adressen, an denen Ghidra vermutlich Datenbereiche als Code
# fehlinterpretiert hat (z.B. BCLR mit PC-relativem Ziel -- architek-
# tonisch unmoeglich als Schreibziel). Werden unabhaengig vom Mnemonic
# als reine Rohbytes ausgegeben, damit trotzdem exakt dieselben Bytes
# an der richtigen Adresse landen.
FORCE_RAW_BYTES = {
    "0036e2",  # bclr.b mit PC-relativem Ziel -- architektonisch unmoeglich, Datenregion
    "003c6e",  # btst.b #0x0d8e -- Bitnummer > 31, r68 lehnt Literal semantisch ab
    "006f30",  # bmi.b auf Ziel ausserhalb der Moduldatei (0x6f97 > 0x6f3c) -- Datenregion
    # Per finalem Byte-Diff gegen das Original gefundene weitere
    # Datenregionen, die Ghidra faelschlich als ori.b/andi.b/cmpi.b/
    # subi.b/btst.b-Instruktionen disassembliert hat. Erkennbar daran,
    # dass die vermeintliche Byte-Immediate ein von Null verschiedenes
    # oberes Wort-Byte hat (bei einer echten .b-Instruktion waere das
    # immer 0) -- unser Assembler erzeugt korrekterweise 0 dort, das
    # Original hat aber den rohen Datenbyte-Wert. Siehe REBUILD.md.
    "001184", "00119c",  # btst.b #0x6,(...,PC) -- Ghidra-Referenz zudem falsch (Basis +2 statt +4)
    "0019b8", "0026b4",  # cmpi.b #-0x43,(0x22,An)
    "0020b8",             # ori #0x8 (vermeintlich ORI-to-CCR)
    "002a24", "002aa8", "0035e8",
    "0036c8", "0036d0", "0036d4", "0036d8", "0036dc", "0036e8",
    "00380a", "00380e", "003812",
    "003dd4", "003ddc", "003de4",
    "004698", "0046cc", "004790",
    "006d84", "006dde",
}

# Indizierte Adressierung (68020-Voll-/Speicher-indirekt-Erweiterungswort):
# r68 waehlt bei der einfachen "disp(An,Xn)"-Syntax immer das kuerzeste
# Brief-Format (8-Bit-Displacement im Erweiterungswort selbst), auch wenn
# das Original ein Voll-Erweiterungswort mit 16/32-Bit-Displacement nutzt.
# Per Symbolkarten-Drift-Analyse gefundene Faelle, wo das zu falscher
# Groesse fuehrt -- hier per Hand mit expliziter r68-Vollformat-Syntax
# (aeussere Klammern erzwingen Vollformat, "(disp).w" erzwingt 16 statt
# 32 Bit) auf exakt die Original-Bytes gebracht; siehe REBUILD.md.
FULL_EXT_OVERRIDE = {
    "000912": "#$3,($74,a1,d0.w*1)",           # bset.b -- Vollformat, 32-Bit-Displacement (Original: 10 statt 6 Bytes)
    "000d04": "([($2a8).w,a4],($6c).w),a2",    # lea -- Speicher-indirekt, 16-Bit Basis+Outer (Original: 8 statt 12 Bytes)
    "001fa2": "([($334).w,a1],($8).w),a2",     # lea -- Speicher-indirekt, 16-Bit Basis+Outer (Original: 8 statt 12 Bytes)
}

# Bedingte/unbedingte Branches mit explizitem .w-Groessensuffix: r68
# optimiert diese automatisch zu Kurzform (Brief-Branch, 2 Bytes), sobald
# das Ziel in ein vorzeichenbehaftetes Byte passt -- unabhaengig vom
# expliziten Suffix. Das Original nutzt an mehreren Stellen aber echt die
# Wortform (4 Bytes), vermutlich weil der Original-Compiler keine
# Branch-Groessenoptimierung durchfuehrt. Fix: Opcode-Wort direkt aus den
# Ghidra-Rohbytes uebernehmen (dc.w) + Displacement als eigenes
# berechnetes dc.w (TARGET-*), das umgeht r68s Groessenwahl vollstaendig.
BRANCH_WORD_RISK = {"bra","bsr","beq","bne","bcc","bcs","bge","bgt","ble","blt",
                     "bhi","bls","bmi","bpl","bvc","bvs"}

# Sprechende Namen fuer Einstiegspunkte, die in
# docs/REVERSE_ENGINEERING.md inhaltlich verstanden und dokumentiert
# sind (Adresse-im-Namen-Konvention beibehalten, damit die Label
# weiterhin eindeutig auf die Original-Adresse schliessen lassen).
# Nur Funktions-/Block-EINSTIEGSPUNKTE werden umbenannt, nicht jede
# einzelne Instruktion darin -- die bleiben generisch "Lxxxxxx".
LABEL_NAMES = {
    "000180": "Q9_disp_180",        # IRQ-Dispatcher (Autovektoren 1-7 + User-Defined Vectors)
    "000452": "Q9_disp_452",        # Spurious/Uninitialized-Interrupt-Handler (bedient auch Vektor 0x472)
    "000488": "Q9_disp_488",        # TRAP-#0 -- der eigentliche OS-9-Syscall-Dispatcher (F$-Aufrufe)
    "0005d0": "Q9_disp_5d0",        # TRAP #1-15 Dispatcher mit prozesseigenen Handlern
    "000888": "Q9_disp_888",        # Bus/Address-Error-Handler (faellt durch in Q9_disp_8d0)
    "0008d0": "Q9_disp_8d0",        # Sammel-Handler: FPU-Exceptions, Illegal Instr, Breakpoints, Signal-Zustellung
    "000ba4": "Q9_disp_ba4",        # Trace-Exception-Handler (Single-Step)
    "000bc0": "Q9_signal_pending_bc0",  # Signal-/Breakpoint-Pending-Verwaltung mit Prioritaets-Aging
    "000fc4": "Q9_exc_no_handler_fc4",  # Fallback ohne installierten Handler -> Prozess-Terminierung
    "000fe0": "Q9_fpu_save_fe0",    # FPU-Kontext sichern (Lazy-Context-Switch, Save-Haelfte)
    "001004": "Q9_fpu_migrate_1004",  # Hilfsfunktion: migriert belegten FPU-Save-Bereich vor Ueberschreiben
    "001034": "Q9_fpu_restore_1034",  # FPU-Kontext wiederherstellen (Restore-Haelfte)
    "00134a": "Q9_range_check_wrap_134a",  # kleiner Wrapper um Q9_owns_range_5d68, Carry bei Fehler
    "0067a0": "Q9_kernel_init_67a0",  # zentraler Kernel-Bootstrap: baut Exception-/Syscall-Tabellen im RAM auf
    "000b04": "Q9_fpsp_handler_b04",  # FPSP-Einstieg (Floating-Point Software Package), nutzt EA-Decoder bei 0xb3a
    "00183a": "Q9_scheduler_183a",  # Ready-Queue-Einfuegeroutine mit Priority Aging
    "003140": "Q9_reschedule_trampolin_3140",  # Cache-Flush-Schleife + Trampolin-Sprung zu Tabellen-Slot 90
    "001390": "Q9_category_dispatch_1390",  # genereller Kategorie-Dispatcher fuer kernel-interne Primitive (6 Kategorien)
    "001424": "Q9_category0_handler_1424",  # Kategorie-0-Handler: Liste angehaengter Module, 0xB0BD-Signaturpruefung
    "0014ae": "Q9_module_unlink_14ae",  # Modul aus zwei parallelen verketteten Listen aushaengen + freigeben
    "0025f8": "Q9_proc_slot_cleanup_25f8",  # Prozessdeskriptor-Slot aufraeumen (Ressourcen, offene Pfade, FPU-Ownership)
    "001e18": "Q9_proc_id_free_wrap_1e18",  # duenner Weiterreicher zu Q9_proc_id_free_3370
    "003370": "Q9_proc_id_free_3370",  # Prozess-ID aus der ID-Tabelle freigeben (mit eigener Freiliste)
    "002590": "Q9_proc_die_prep_2590",  # Trampolin Tabellen-Slot 89, "Prozess stirbt gleich"-Vorbereitung
    "0024d8": "Q9_exc_default_action_24d8",  # Standardaktion fuer unbehandelte Exceptions (Terminierung oder Signal)
    "0062da": "Q9_proc_resource_free_62da",  # Speicher-/Ressourcenfreigabe eines Prozesses beim Exit
    "002cee": "Q9_proc_id_lookup_2cee",  # Prozess-ID-Lookup/-Validierung (Index+Generation-Schema)
    "004518": "Q9_parent_notify_4518",  # Eltern-Benachrichtigung beim Kindprozess-Tod (SIGCHLD/wait-artig)
    "004078": "Q9_trampolin_slot88_4078",  # internes Trampolin, Tabellen-Slot 88
    "00131c": "Q9_dealloc_tail_131c",  # gemeinsamer Deallokations-Tail-Wrapper (free(ptr,flag)-artig)
    "004978": "Q9_const_init_4978",  # initialisiert feste System-Global-Konstanten (Alignment=16, Groesse=256)
    "005440": "Q9_mem_alloc_5440",  # Speicher-Allokations-Primitive (First-Fit, Split von hinten)
    "00526c": "Q9_arena_alloc_526c",  # Arena-Deskriptor-Allocator
    "0055a4": "Q9_pool_lookup_55a4",  # Pool-Lookup nach Adresse/Groesse
    "005712": "Q9_freelist_bysize_5712",  # groessensortierte Freiliste auf Arena-Ebene
    "005a22": "Q9_mem_free_5a22",  # zentrale Speicherfreigabe-Primitive (Boundary-Tag-Coalescing)
    "005bac": "Q9_arena_lookup_5bac",  # Arena-Lookup-oder-Erzeugen fuer eine freizugebende Adresse
    "005c7c": "Q9_sorted_list_insert_5c7c",  # generischer nach Klassen-Tag sortierter Doppelverkettungs-Insert
    "005d68": "Q9_owns_range_5d68",  # prueft, ob ein Adressbereich dem aktuellen Prozess gehoert
    "0010e6": "Q9_irq_mask_10e6",  # Interrupts bedingt maskieren (alte SR als Rueckgabewert)
    "0010f2": "Q9_irq_unmask_10f2",  # SR wiederherstellen (Gegenstueck zu Q9_irq_mask_10e6)
    "0007f6": "Q9_panic_report_7f6",  # Panik-/Diagnose-Reporter (zwei Einstiegspunkte, kein echter Halt)
    "0007be": "Q9_trampolin_rescue_7be",  # Rettungsanker fuer unterbrochene interne Trampolin-Aufrufe
    "000834": "Q9_panic_delay_834",  # Verzoegerungs-/Timeout-Schleife (Konsolen-Polling)
    "000850": "Q9_console_puts_850",  # Zeichenketten-Ausgabe auf die Systemkonsole
    "000868": "Q9_console_puthex_868",  # Hex-Ausgabe eines 32-Bit-Werts (rekursiv, ein Nibble pro Aufruf)
    "0006a8": "Q9_clock_tick_6a8",  # periodischer Uhr-Tick-Handler (Systemzeit, Zeitscheiben-Ablauf)
    "00070c": "Q9_clock_hook_install_70c",  # Selbstregistrierung als IRQ-Dispatcher-Tick-Hook
    "0000b2": "Q9_post_idstring_b2",  # Padding + indizierter Trampolin-Dispatch direkt nach dem ID-String
}

# Kurze Funktions-Header-Kommentare (eine Zeile Zusammenfassung, ggf.
# Adressbereich) fuer dieselben Einstiegspunkte -- eigene Formulierung
# aus docs/REVERSE_ENGINEERING.md, keine Uebernahme von Microware-Text.
FUNC_HEADER = {
    "000180": "IRQ-Dispatcher: Autovektoren 1-7 + User-Defined Vectors, verkettete Handler-Deskriptorlisten, Reschedule-Trigger.",
    "000452": "Spurious/Uninitialized-Interrupt-Handler (Vektoren 15, 24).",
    "000488": "TRAP #0 -- OS-9-Syscall-Dispatcher: liest Funktionsnummer aus dem Codestrom, Trampolin-Aufruf in eine der zwei Syscall-Tabellen.",
    "0005d0": "TRAP #1-15 -- Dispatcher fuer prozesseigene, selbst installierte Trap-Handler.",
    "000888": "Bus/Address-Error-Handler: sichert Fault-Frame-Zusatzfelder, faellt dann durch in Q9_disp_8d0.",
    "0008d0": "Sammel-Handler fuer Illegal Instr/Zero Div/CHK/TRAPV/Priv.Violation/Line-A-F/FPU/MMU: Software-Breakpoints, generisches Vektor-Handler-System, Signal-Zustellung.",
    "000ba4": "Trace-Exception-Handler (Single-Step-Debugging), minimaler Epilog.",
    "000bc0": "Signal-/Breakpoint-Pending-Verwaltung mit Prioritaets-Aging der Deskriptorkette.",
    "000fc4": "Fallback ohne installierten Vektor-/Signal-Handler -- fuehrt zur Standard-Terminierungslogik.",
    "000fe0": "FPU-Kontext sichern (Lazy-Context-Switch, Save-Haelfte).",
    "001004": "Migriert den Inhalt eines bereits belegten FPU-Save-Bereichs, bevor er ueberschrieben wird.",
    "001034": "FPU-Kontext wiederherstellen (Restore-Haelfte, Gegenstueck zu Q9_fpu_save_fe0).",
    "00134a": "Wrapper um Q9_owns_range_5d68 -- setzt Carry-Flag bei ungueltigem Adressbereich.",
    "0067a0": "Zentraler Kernel-Bootstrap: alloziert und befuellt die Exception-Sprungtabelle sowie die Syscall-Tabellen im RAM.",
    "000b04": "FPSP-Handler (Floating-Point Software Package) fuer Vektor 48 (Branch/Set on Unordered), nutzt den EA-Decoder bei 0xb3a.",
    "00183a": "Scheduler: fuegt einen Prozess in die Ready-Queue ein, mit Prioritaets-Aging und Sortier-Schluessel-Berechnung.",
    "003140": "Cache-Flush-Schleife (patcht/invalidiert selbstmodifizierten Code) + Trampolin-Sprung in Tabellen-Slot 90 (Kontextwechsel-Einstieg).",
    "001390": "Genereller Kategorie-Dispatcher fuer kernel-interne Primitive (6 Kategorien, ueber Slot 8 der Syscall-Tabellen erreichbar).",
    "001424": "Kategorie-0-Handler: durchlaeuft die Liste angehaengter Module/Deskriptoren, prueft die 0xB0BD-Struktursignatur.",
    "0014ae": "Haengt einen Modul-Deskriptor aus zwei parallelen verketteten Listen gleichzeitig aus und gibt ihn frei.",
    "0025f8": "Prozessdeskriptor-Slot aufraeumen: Ressourcenlisten, offene Pfade (echter TRAP #0-Close), FPU-Ownership.",
    "001e18": "Duenner Weiterreicher zu Q9_proc_id_free_3370.",
    "003370": "Gibt eine Prozess-ID in der ID-Tabelle frei (eigene Freiliste wiederverwendbarer IDs).",
    "002590": "Trampolin Tabellen-Slot 89 -- Vorbereitung kurz vor dem Sterben eines Prozesses.",
    "0024d8": "Standardaktion fuer unbehandelte Exceptions: Prozess-Terminierung inkl. Eltern-Benachrichtigung, oder Signal-Zustellung falls ein Handler existiert.",
    "0062da": "Gibt die pro Prozess gehaltenen Speicherblock- und Fixgroessen-Ressourcenlisten beim Exit frei.",
    "002cee": "Prozess-ID-Lookup/-Validierung (Index+Generation-Schema gegen versehentliche Wiederverwendung).",
    "004518": "Eltern-Benachrichtigung beim Kindprozess-Tod, weckt einen wartenden Elternprozess (SIGCHLD/wait-artig).",
    "004078": "Internes Trampolin, Tabellen-Slot 88.",
    "00131c": "Gemeinsamer Deallokations-Tail-Wrapper (dispatcht auf Q9_mem_free_5a22 oder Bereichsvalidierung).",
    "004978": "Initialisiert feste System-Global-Konstanten (Speicher-Alignment=16, Groessenkonstante=256) -- kein Allocator.",
    "005440": "Speicher-Allokations-Primitive: First-Fit ueber Arena-Ketten, Split-von-hinten bei Restflaeche.",
    "00526c": "Arena-Deskriptor-Allocator mit Fallback auf den zweiten Speicherpool.",
    "0055a4": "Pool-Lookup: findet den zustaendigen Speicherpool-Deskriptor fuer eine Adresse/Groesse.",
    "005712": "Groessen-/klassensortierte Freiliste auf Arena-Ebene (Einfuegen und Schrumpfen bestehender Eintraege).",
    "005a22": "Zentrale Speicherfreigabe-Primitive: zwei Pools nacheinander versucht, Boundary-Tag-Coalescing.",
    "005bac": "Arena-Lookup-oder-Erzeugen fuer eine freizugebende Adresse (legt bei Bedarf einen neuen Arena-Deskriptor an).",
    "005c7c": "Genereller, nach Klassen-/Typ-Tag sortierter Doppelverkettungs-Insert (auch fuer Arenen genutzt).",
    "005d68": "Prueft, ob ein Adressbereich zu den vom aktuellen Prozess gehaltenen Speicherbloecken gehoert.",
    "0010e6": "Interrupts bedingt maskieren (IPL 7), alte SR als Rueckgabewert.",
    "0010f2": "SR wiederherstellen -- Gegenstueck zu Q9_irq_mask_10e6.",
    "0007f6": "Panik-/Diagnose-Reporter mit zwei Einstiegspunkten -- protokolliert und kehrt zurueck, haelt das System nicht an.",
    "0007be": "Rettungsanker: bricht einen unterbrochenen internen Trampolin-Aufruf kontrolliert ab, statt in Panik zu enden.",
    "000834": "Verzoegerungs-/Timeout-Schleife, pollt auf Konsolen-Bereitschaft.",
    "000850": "Gibt einen nullterminierten ASCII-String auf der Systemkonsole aus.",
    "000868": "Gibt einen 32-Bit-Wert hexadezimal aus (rekursiv, ein Nibble pro Aufruf ueber ROR.L).",
    "0006a8": "Periodischer Uhr-Tick-Handler: Systemzeit-Fortschreibung, Zeitscheiben-Ablauf-Erkennung des Schedulers.",
    "00070c": "Selbstregistrierung als IRQ-Dispatcher-Tick-Hook (Scheduler-Tick).",
    "0000b2": "TRAPF.L-Padding (6 Byte, Ausrichtung) gefolgt von einem indizierten Trampolin-Dispatch ueber Tabelle (0x8e4,A6).",
}

def lbl(addr_hex):
    return LABEL_NAMES.get(addr_hex, "L" + addr_hex)

def reg(s):
    return s.lower()

SPECIAL_REGS = {"SP", "SR", "CCR", "USP", "VBR", "MSP", "ISP", "CACR", "CAAR",
                 "FPCR", "FPSR", "FPIAR",
                 "FP0","FP1","FP2","FP3","FP4","FP5","FP6","FP7"}

def convert_operand(op, ref, is_pcrel_target, mnem=""):
    op = re.sub(r"\s+", " ", op.strip())
    op_nospace = op.replace(", ", ",").replace(" ,", ",")
    had_hash = op_nospace.startswith("#")
    core = op_nospace[1:] if had_hash else op_nospace

    # Registerliste movem: "{  A6 A5 D0}" (Leerzeichen-getrennt)
    m = re.match(r"^\{\s*(.*?)\s*\}$", core)
    if m:
        regs = m.group(1).split()
        conv = []
        for rg in regs:
            rg2 = rg.rstrip("wbl") if re.match(r"^[AD][0-7][wbl]$", rg) else rg
            conv.append(rg2.lower())
        return "/".join(conv)
    # PC-relativ indiziert: (0xNN,PC,D1w*0x1) -- Groessensuffix optional (Default: l).
    # Ghidra liefert fuer diese Adressierungsart oft keine Referenz (dynamisches
    # Ziel), daher hier per Hand aufgeloeste Ausnahmen fuer die wenigen Faelle,
    # die im Modul vorkommen (siehe PCREL_INDEXED_OVERRIDE unten).
    m = re.match(r"^\((-?0x[0-9a-fA-F]+),PC,([AD][0-7])([wl])?\*0x([0-9a-fA-F]+)\)$", core)
    if m:
        idxreg, size, scale = m.group(2), m.group(3), m.group(4)
        if ref:
            base = lbl(ref)
        elif CURRENT_INSTR_OFF in PCREL_INDEXED_OVERRIDE:
            base = lbl(PCREL_INDEXED_OVERRIDE[CURRENT_INSTR_OFF])
        else:
            try:
                instr_addr = int(CURRENT_INSTR_OFF, 16)
                disp = int(m.group(1), 16)
                target = (instr_addr + 2 + disp) & 0xFFFFFFFF
                base = lbl(format(target, '06x'))
            except Exception:
                return None
        sizestr = "w" if size == "w" else "l"
        return f"{base}(pc,{idxreg.lower()}.{sizestr}*{scale})"
    # PC-relativ: (0xNN,PC) oder (-0xNN,PC)
    m = re.match(r"^\((-?0x[0-9a-fA-F]+),PC\)$", core)
    if m:
        if ref:
            return f"{lbl(ref)}(pc)"
        # Ghidra liefert fuer dieses (haeufige) Muster oft keine
        # Referenz, obwohl es sich um echten, gueltigen Code handelt
        # (typischerweise PEA fuer Trampolin-Ruecksprungadressen). Ziel
        # per Hand nach dem 68k-Kurzform-PC-relativ-Schema berechnen:
        # Basis = Adresse des Erweiterungsworts (Instruktionsadresse+2).
        try:
            instr_addr = int(CURRENT_INSTR_OFF, 16)
            disp = int(m.group(1), 16)
            target = (instr_addr + 2 + disp) & 0xFFFFFFFF
            return f"{lbl(format(target, '06x'))}(pc)"
        except Exception:
            return None
    # Displacement + Register + Index: (0xNN,A6,D0w*0x1) -- Groessensuffix optional (Default: l)
    m = re.match(r"^\((-?0x[0-9a-fA-F]+),([AD][0-7]|SP),([AD][0-7])([wl])?\*0x([0-9a-fA-F]+)\)$", core)
    if m:
        disp, r, idxreg, size, scale = m.groups()
        rr = "sp" if r == "SP" else r.lower()
        sizestr = "w" if size == "w" else "l"
        return f"{disp.replace('0x','$')}({rr},{idxreg.lower()}.{sizestr}*{scale})"
    # 68020-Speicher-indirekt mit eckigen Klammern: ([0xNN,A4],0xMM)
    m = re.match(r"^\(\[(-?0x[0-9a-fA-F]+),([AD][0-7]|SP)\],(-?0x[0-9a-fA-F]+)\)$", core)
    if m:
        d1, r, d2 = m.groups()
        rr = "sp" if r == "SP" else r.lower()
        return f"([{d1.replace('0x','$')},{rr}],{d2.replace('0x','$')})"
    # Absolutadresse ausserhalb des Modulbereichs (z.B. feste RAM-Adresse),
    # von Ghidra vorzeichenbehaftet dargestellt -- als reiner Hexwert uebernehmen.
    m = re.match(r"^\((-0x[0-9a-fA-F]+)\)\.l$", core)
    if m:
        val = int(m.group(1), 16) & 0xFFFFFFFF
        return f"${val:x}.l"
    # Displacement + Register: (0xNN,A6) oder (-0xNN,A6) oder (0xNN,SP)
    m = re.match(r"^\((-?0x[0-9a-fA-F]+),([AD][0-7]|SP)\)$", core)
    if m:
        disp, r = m.group(1), m.group(2)
        rr = "sp" if r == "SP" else r.lower()
        return f"{disp.replace('0x','$')}({rr})"
    # Register indirekt: (A0) / (SP)
    m = re.match(r"^\(([AD][0-7]|SP)\)$", core)
    if m:
        rr = "sp" if m.group(1) == "SP" else m.group(1).lower()
        return f"({rr})"
    # Post-Increment: (A0)+ / (SP)+
    m = re.match(r"^\(([AD][0-7]|SP)\)\+$", core)
    if m:
        rr = "sp" if m.group(1) == "SP" else m.group(1).lower()
        return f"({rr})+"
    # Prae-Dekrement: -(A0) / -(SP)
    m = re.match(r"^-\(([AD][0-7]|SP)\)$", core)
    if m:
        rr = "sp" if m.group(1) == "SP" else m.group(1).lower()
        return f"-({rr})"
    # Absolut: (0xNN).w oder (0xNN).l
    m = re.match(r"^\((0x[0-9a-fA-F]+)\)\.([wl])$", core)
    if m:
        return f"{m.group(1).replace('0x','$')}.{m.group(2)}"
    # Immediate: 0xNN oder -0xNN (mit oder ohne fuehrendes #)
    m = re.match(r"^(-?0x[0-9a-fA-F]+)$", core)
    if m:
        val_str = m.group(1)
        base_mnem_local = mnem.split(".")[0]
        # Bit-Instruktionen (BTST/BCHG/BCLR/BSET) verlangen eine
        # nicht-negative Bitnummer als Immediate -- Ghidra zeigt manche
        # Werte >0x7F aber vorzeichenbehaftet an. Byte-Muster ist bei
        # Zweierkomplement-Interpretation identisch, nur die Schreibweise
        # muss unsigned sein, sonst weist r68 den Wert zurueck.
        if val_str.startswith("-0x") and base_mnem_local in ("btst", "bchg", "bclr", "bset"):
            if CURRENT_INSTR_OFF in BIT_IMMEDIATE_OVERRIDE:
                return f"#${BIT_IMMEDIATE_OVERRIDE[CURRENT_INSTR_OFF]}"
            unsigned = (0x10000 + int(val_str, 16)) & 0xFFFF
            return f"#${unsigned:x}"
        return "#" + val_str.replace("-0x", "-$").replace("0x", "$")
    # Reines Register: A6, D0w, D0b, D0, SP
    if core == "SP":
        return "sp"
    m = re.match(r"^([AD][0-7])[wbl]?$", core)
    if m:
        return m.group(1).lower()
    # Sonderregister (MOVEC/FPU): VBR, USP, MSP, CACR, CAAR, SR, CCR, FP0..FP7
    if core in SPECIAL_REGS:
        return core.lower()
    # Bare absolute Adresse (Branch-Ziel wird separat behandelt)
    m = re.match(r"^(0x[0-9a-fA-F]+)$", core)
    if m:
        if ref:
            return lbl(ref)
        return m.group(1)
    return None  # unbekanntes Muster

BRANCHY = {"bra","bsr","beq","bne","bcc","bcs","bge","bgt","ble","blt","bhi","bls",
           "bmi","bpl","bvc","bvs","dbf","dbeq","dbne","dbcc","dbcs","dbge","dbgt",
           "dble","dblt","dbhi","dbls","dbmi","dbpl","dbvc","dbvs","dbt","dbra","jmp","jsr"}

out = []
skipped = []
i = 0
data_run = []

def flush_data_run():
    global data_run
    if not data_run:
        return
    # Jede einzelne Ghidra-Code-Unit bekommt ihr eigenes Label (nicht
    # nur der Beginn des gesamten Laufs) -- sonst koennen PC-relative
    # Referenzen, die mitten in einen mehrbytigen Datenblock zeigen,
    # vom Linker nicht aufgeloest werden.
    for r in data_run:
        if r["off"] in FUNC_HEADER:
            out.append(f"* {FUNC_HEADER[r['off']]}")
        out.append(f"{lbl(r['off'])}:")
        b = bytes.fromhex(r["hexb"])
        for j in range(0, len(b), 16):
            chunk = b[j:j+16]
            vals = ",".join(f"${x:02x}" for x in chunk)
            out.append(f"\tdc.b\t{vals}")
    data_run = []

for idx, r in enumerate(rows):
    if int(r["off"], 16) < 0x54:
        continue  # Header/Erweiterung schon manuell in kernel.r
    if r["kind"] != "I":
        data_run.append(r)
        continue
    else:
        flush_data_run()

    mnem = r["mnem"].strip().lower()
    ops_raw = r["ops"]
    ref = r["ref"].strip()

    CURRENT_INSTR_OFF = r['off']
    label = f"{lbl(r['off'])}:"
    if r["off"] in FUNC_HEADER:
        out.append(f"* {FUNC_HEADER[r['off']]}")

    if r["off"] in FULL_EXT_OVERRIDE:
        out.append(f"* Vollformat-Adressierung erzwungen -- siehe FULL_EXT_OVERRIDE im Konverter")
        out.append(label)
        out.append(f"\t{mnem}\t{FULL_EXT_OVERRIDE[r['off']]}")
        continue

    base_mnem_early = mnem.split(".")[0]
    if mnem.endswith(".w") and base_mnem_early in BRANCH_WORD_RISK and ref:
        out.append(f"* Wortform erzwungen (r68 wuerde sonst auf Kurzform optimieren) -- siehe BRANCH_WORD_RISK")
        out.append(label)
        opcode_word = r["hexb"][0:4]
        out.append(f"\tdc.w\t${opcode_word}")
        out.append(f"\tdc.w\t{lbl(ref)}-*")
        continue

    op_list = []
    if ops_raw:
        # grobe Trennung an Kommas ausserhalb von Klammern/geschweiften Klammern
        depth = 0
        cur = ""
        for ch in ops_raw:
            if ch in "({":
                depth += 1
            elif ch in ")}":
                depth -= 1
            if ch == "," and depth == 0:
                op_list.append(cur)
                cur = ""
            else:
                cur += ch
        if cur:
            op_list.append(cur)

    if r["off"] in FORCE_RAW_BYTES:
        skipped.append((r["off"], mnem, ops_raw))
        out.append(f"* Rohbytes statt Instruktion ({mnem} {ops_raw}) -- siehe FORCE_RAW_BYTES im Konverter")
        out.append(label)
        b = bytes.fromhex(r["hexb"])
        vals = ",".join(f"${x:02x}" for x in b)
        out.append(f"\tdc.b\t{vals}")
        continue

    base_mnem = mnem.split(".")[0]
    is_ctrl_flow = base_mnem in BRANCHY

    # Sonderfall: "ori"/"andi"/"eori" OHNE Groessensuffix mit genau einem
    # Operanden ist immer die dedizierte ORI/ANDI/EORI-to-CCR-Variante --
    # Ghidra zaehlt CCR dort nicht als eigenen Operanden mit.
    if mnem in ("ori", "andi", "eori") and len(op_list) == 1:
        op_list.append("CCR")
    # "move" ohne Groessensuffix mit nur einem Operanden ist immer die
    # dedizierte MOVE-to-CCR-Variante (Opcode-Praefix 0x44) -- ebenfalls
    # zaehlt Ghidra CCR dort nicht als eigenen Operanden mit. Die
    # MOVE-to-SR-Variante (Opcode-Praefix 0x46) liefert dagegen bereits
    # korrekt zwei Operanden inkl. ",SR".
    if mnem == "move" and len(op_list) == 1:
        op_list.append("CCR")

    conv_ops = []
    ok = True
    for oi, op in enumerate(op_list):
        op_stripped = op.strip()
        # Branch-/Jump-/Call-Ziel: bare Hex-Adresse bei Kontrollfluss-Mnemonic
        # IMMER als Label behandeln, nie als Immediate (Vorrang vor allen
        # anderen Mustern, insbesondere vor der generischen Immediate-Erkennung).
        if is_ctrl_flow and re.match(r"^0x[0-9a-fA-F]+$", op_stripped) and ref:
            conv_ops.append(lbl(ref))
            continue
        use_ref = ref if ("PC" in op or re.match(r"^0x[0-9a-fA-F]+$", op_stripped)) else ""
        c = convert_operand(op, use_ref, False, mnem)
        if c is None:
            ok = False
            break
        conv_ops.append(c)

    if not ok:
        skipped.append((r["off"], mnem, ops_raw))
        out.append(f"* UNKONVERTIERT: {mnem} {ops_raw}  (roh, siehe skipped-Liste)")
        out.append(label)
        allb = r["hexb"]
        b = bytes.fromhex(allb)
        vals = ",".join(f"${x:02x}" for x in b)
        out.append(f"\tdc.b\t{vals}")
        continue

    line = f"\t{mnem}\t" + ",".join(conv_ops) if conv_ops else f"\t{mnem}"
    out.append(label)
    out.append(line)

flush_data_run()

with open(OUT, "w", encoding="utf-8") as f:
    f.write("\n".join(out) + "\n")

print(f"Zeilen: {len(out)}, unkonvertiert: {len(skipped)}")
for s in skipped[:40]:
    print("  ", s)
