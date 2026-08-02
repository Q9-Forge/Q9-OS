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
    # Neu bei der zweiten Gap-Runde gefunden: "ori.b #0x7c,(a6)" bei
    # 0x6e10 -- in Wahrheit ein 6-Byte move.w #1,(2,a6) (System-Global-
    # Flag setzen), das Ghidra falsch in ein 4-Byte ori.b + Folgebyte
    # zerlegt hat. Der Branch-Zieladresse 0x6e12 (mitten in diesem Feld)
    # wird per EQU-Alias in kernel.r aufgeloest.
    "006e10",
    "0020c2",  # andi.b #0x18,(A4)+ -- oberes Immediate-Byte 0x03 sonst verloren, dieselbe Fehlerklasse
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
    "001390": "Q9_alarm_dispatch_1390",  # F$Alarm-Dispatcher: D1=Alarm-Funktionscode (A$Delete/A$Set/A$Cycle/A$AtDate/A$AtJul)
    "001424": "Q9_alarm_delete_1424",  # A$Delete: durchlaeuft die Alarm-Deskriptorliste (0x37c,A4), prueft 0xB0BD-Signatur, haengt aus
    "0014ae": "Q9_alarm_unlink_14ae",  # Alarm-Deskriptor aus zwei parallelen verketteten Listen aushaengen + freigeben
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

    # Zweite Runde (verbliebene "Gap"-Inseln, per gezielter
    # Offset-Diagnose als echter Code entlarvt statt Datenmuell).
    "002398": "Q9_table_lookup_2398",       # generischer, tag-geprueft Lookup in einem grenzgeprueften 32-Byte-Eintrags-Array
    "00244c": "Q9_scheduler_caller_244c",   # 11. Aufrufer von Q9_scheduler_183a, Listendurchlauf + Pruefwert-Berechnung
    "002dfc": "Q9_irq_chain_lookup_2dfc",   # IRQ-Handler-Ketten-Slot-Lookup (dieselben Offsets 0x384/-0x5c wie Q9_disp_180)
    "002eb2": "Q9_irq_chain_insert_2eb2",   # durchsucht die IRQ-Handler-Kette und haengt einen neuen Deskriptor ein
    "002c84": "Q9_proc_priority_calc_2c84", # berechnet einen Prioritaets-/Sortierwert (Q9_proc_id_lookup_2cee + Scheduler-Sortierschluessel)
    "001b4c": "Q9_field_tag_set_1b4c",      # setzt ein getaggtes 24-Bit-Feld (Tag-Byte per ST erzwungen)
    "0035f6": "Q9_scheduler_caller_35f6",   # 12. Aufrufer von Q9_scheduler_183a, weckt einen schlafenden Prozess
    "00362c": "Q9_module_patch_362c",       # weiterer 0x4AFC-Platzhalter-Patch-Mechanismus (wie Q9_reschedule_trampolin_3140), andere Zielstruktur
    "0039b2": "Q9_err_ab_stub_39b2",        # Fehler-0xAB-Rueckgabe-Stub (Speicher-Allokationsfehler, siehe Q9_mem_alloc_5440)
    "006de4": "Q9_boot_finalize_6de4",      # Abschluss des Kernel-Bootstraps, springt zuletzt in Q9_reschedule_trampolin_3140

    # Frueher schon gelesene, aber nie benannte Helferfunktionen
    # (Vergleichs-/Suchhelfer von Q9_syscall_27d6, alternative
    # Freigabe-Variante aus dem Speicherverwaltungs-Fund).
    "0027d6": "Q9_syscall_27d6",       # Modultabellen-Suche nach Signatur-Bytes (F$Find-artig)
    "0032fa": "Q9_module_name_match_32fa",  # Zeichen-fuer-Zeichen-Vergleichshelfer, von Q9_syscall_27d6 aufgerufen
    "001ab8": "Q9_pattern_match_1ab8",      # Musterabgleich mit '*'-Wildcard, von Q9_syscall_27d6 aufgerufen
    "005cd2": "Q9_dealloc_owned_5cd2",      # alternative Freigabe-Variante: prueft Eigentum via Q9_owns_range_5d68 vor der Freigabe

    # Alarm-Funktionen 1-5 von Q9_alarm_dispatch_1390 (Kategorie 0 war
    # schon vorher als Q9_alarm_delete_1424 benannt). Identifiziert als
    # F$Alarm-Implementierung: Registerkonvention (D3=Intervall,
    # D4=Datum) und 5 Unterfunktionsnamen aus dem Technical Manual
    # (Appendix D, Table D-5) passen zu den 6 gefundenen Kategorien.
    # Echte Sprungtabelle von Q9_alarm_dispatch_1390 bei 0x13c6 direkt
    # dekodiert (PC-relative Displacement-Worte, Basis 0x13c6): Kat.0
    # ->0x1424, Kat.1->0x157e, Kat.2->0x1580, Kat.3->0x1534, Kat.4->
    # 0x1540, Kat.5->0x16a0 (=bra.w 0x1380, der Fehler-Stub -- Kategorie
    # 5 ist in diesem Kernel-Build schlicht NICHT implementiert).
    "00157e": "Q9_alarm_set_157e",     # Kategorie 1 (D1 vorbelegt mit 0), faellt in Q9_alarm_set_1580 durch
    "001580": "Q9_alarm_set_1580",     # Kategorie 2 = A$Set: Signal nach Zeitintervall D3
    "001534": "Q9_alarm_cycle_1534",   # Kategorie 3 = A$Cycle: periodisches Signal
    "001540": "Q9_alarm_cycle_1540",   # Kategorie 4: wie Kategorie 3, ueberspringt nur den Aufruf von 0x2eea (D3/D4 direkt uebernommen)
    "00162c": "Q9_alarm_desc_alloc_162c",  # gemeinsamer Alarm-Deskriptor-Allocator (0x74 Byte), von mehreren Alarm-Funktionen genutzt
    "0015c4": "Q9_alarm_insert_15c4",  # sortiertes Einfuegen in die Alarm-Liste nach Faelligkeit + Lazy-Registrierung des Uhr-Tick-Hooks
    "00161a": "Q9_alarm_insert_wrap_161a",  # duenner Wrapper um Q9_alarm_insert_15c4
    "0016a0": "Q9_alarm_unimplemented_16a0",  # echtes Kategorie-5-Ziel: nur bra.w zum Fehler-Stub (A$AtDate/A$AtJul nicht implementiert)
    "0012b4": "Q9_fixed_alloc_wrap_12b4",  # generischer Wrapper: alloziert ueber Q9_arena_alloc_526c, Groesse vom Aufrufer via Stack

    # Laufzeitverifizierte Syscall-Tabelle (docs/REVERSE_ENGINEERING.md,
    # "Komplette Syscall-Tabelle namentlich zugeordnet") vollstaendig im
    # Quellcode verankert -- alle F$/I$-Einstiegspunkte, die dort noch keinen
    # eigenen Namen hatten, plus drei zentrale, mehrfach genutzte Helfer.
    "00128e": "Q9_trans_128e",
    "0012a6": "Q9_srqmem_12a6",
    "0012f0": "Q9_srqcmem_12f0",
    "0012fa": "Q9_srqmem_user_12fa",
    "001312": "Q9_srqcmem_user_1312",
    "001330": "Q9_srtmem_user_1330",
    "00133c": "Q9_mem_query_133c",
    "001366": "Q9_gblkmp_1366",
    "00137c": "Q9_chkmem_user_137c",
    "0013d2": "Q9_alarm_dispatch_core_13d2",
    "0016a8": "Q9_allprc_16a8",
    "0016b6": "Q9_procdesc_alloc_16b6",
    "00171a": "Q9_desc_slot_alloc_171a",
    "001708": "Q9_allpd_1708",
    "001928": "Q9_chain_1928",
    "001b48": "Q9_cpymem_1b48",
    "001b88": "Q9_crc_1b88",
    "001cd0": "Q9_datmod_1cd0",
    "001e30": "Q9_deltsk_user_1e30",
    "001e38": "Q9_dexec_1e38",
    "002000": "Q9_dexit_2000",
    "002038": "Q9_dfork_2038",
    "0020d0": "Q9_event_20d0",
    "0026e8": "Q9_findpd_26e8",
    "002718": "Q9_firq_2718",
    "0027c0": "Q9_fmodul_27c0",
    "002868": "Q9_fork_2868",
    "002c48": "Q9_gmoddr_2c48",
    "002c68": "Q9_gprdbt_2c68",
    "002c88": "Q9_gprdsc_2c88",
    "002ce8": "Q9_gprocp_2ce8",
    "002d18": "Q9_gregor_2d18",
    "002eea": "Q9_date_decompose_2eea",
    "002dd0": "Q9_icpt_2dd0",
    "002de0": "Q9_id_2de0",
    "002e00": "Q9_irq_2e00",
    "002ee0": "Q9_julian_2ee0",
    "002f70": "Q9_link_2f70",
    "002fba": "Q9_link_user_2fba",
    "0030a0": "Q9_move_30a0",
    "0032e0": "Q9_prsnam_32e0",
    "0033c8": "Q9_rte_33c8",
    "0034fe": "Q9_send_34fe",
    "003630": "Q9_setcrc_3630",
    "003678": "Q9_ssvc_3678",
    "003840": "Q9_setsys_3840",
    "003960": "Q9_sigmask_3960",
    "0039a8": "Q9_sigreset_39a8",
    "0039bc": "Q9_sigreset_user_39bc",
    "0039f0": "Q9_sleep_user_39f0",
    "003a06": "Q9_sleep_3a06",
    "003bc8": "Q9_sprior_3bc8",
    "003c08": "Q9_stime_3c08",
    "003d4a": "Q9_strap_3d4a",
    "003d60": "Q9_suser_3d60",
    "003d98": "Q9_sysdbg_user_3d98",
    "003da0": "Q9_sysdbg_3da0",
    "003e00": "Q9_sysid_3e00",
    "003eb0": "Q9_time_3eb0",
    "003f48": "Q9_tlink_user_3f48",
    "004070": "Q9_uacct_4070",
    "0040b4": "Q9_unlink_40b4",
    "004228": "Q9_unload_4228",
    "004258": "Q9_vmodul_4258",
    "004488": "Q9_wait_user_4488",
    "00449e": "Q9_wait_449e",
    "004828": "Q9_sema_4828",
}

# Kurze Funktions-Header-Kommentare (eine Zeile Zusammenfassung, ggf.
# Adressbereich) fuer dieselben Einstiegspunkte -- eigene Formulierung
# aus docs/REVERSE_ENGINEERING.md, keine Uebernahme von Microware-Text.
# Richtige Funktions-Header (mehrzeilig): Zweck, wo bekannt Register-
# Konvention/Fehlercodes/Aufrufer, und immer ein Verweis auf den
# ausfuehrlichen Fund-Abschnitt in docs/REVERSE_ENGINEERING.md fuer
# alle Details, die hier nicht dupliziert werden sollen. Eigene
# Formulierung, keine Uebernahme von Microware-Text.
FUNC_HEADER = {
    "000180": [
        "IRQ-Dispatcher: Sammel-Handler fuer Interrupt-Autovektoren 1-7 und alle User-Defined Vectors (199 von 256 Tabelleneintraegen).",
        "Register: D0=Vektornummer (aus Exception-Frame). Ruft jeden Handler der Kette per JSR (A0) auf; Carry=1 heisst 'naechster in der Kette', Carry=0 'behandelt'.",
        "Enthaelt zusaetzlich den optionalen Scheduler-Tick-Hook (0x8c0,A6) und den Reschedule-Aufruf von Q9_scheduler_183a.",
    ],
    "000452": [
        "Spurious/Uninitialized-Interrupt-Handler (CPU-Vektoren 15 und 24).",
        "Erhoeht einen Spurious-Zaehler (0x84,A6) mit Saettigung; ignoriert das Ereignis falls Flag-Bit 6 in (0x2e,A6) gesetzt ist, sonst Sprung in die Panik-Infrastruktur bei 0x804.",
    ],
    "000488": [
        "TRAP #0 -- der eigentliche OS-9-Syscall-Dispatcher, hier landet jeder F$-Aufruf.",
        "Liest die Funktionsnummer als inline Wort direkt nach der TRAP-Instruktion (aus dem geretteten PC), D7=Funktionsnummer.",
        "Zwei parallele Syscall-Tabellen (0x3a4,A6)/(0x3a8,A6), Auswahl ueber Bit 5 des geretteten Statusworts. Dispatch per PEA+RTS-Trampolin.",
        "Fehlercode 0xD0 bei Funktionsnummer >= 0x100. Stack-Kanarienvogel 'Jimi' (0x4A696D69) wird nach Handler-Rueckkehr geprueft.",
    ],
    "0005d0": [
        "TRAP #1-15 -- Dispatcher fuer prozesseigene, selbst installierte Trap-Handler (z.B. fuer Sprach-Laufzeiten/Debugger).",
        "Trap-Nummer indiziert eine Tabelle bei D_Proc+8; Zustellung ueber einen synthetischen Rueckkehr-Frame auf dem User-Stack.",
        "Fallback-Kette ueber D_Proc+0x38 falls kein eigener Handler; Fehlercode 0x85 wenn niemand zustaendig ist.",
    ],
    "000888": [
        "Bus/Address-Error-Handler: sichert die zusaetzlichen 68030-Langformat-Frame-Felder (D3-D5), faellt dann direkt durch in Q9_disp_8d0.",
    ],
    "0008d0": [
        "Sammel-Handler fuer Illegal Instr/Zero Div/CHK/TRAPV/Priv.Violation/Line-A-F/reservierte Vektoren/FPU-Exceptions/MMU-Fehler.",
        "Bedient: FPU-Exception-Vorverarbeitung (ruft Q9_fpu_save_fe0 + FPSP-Einstieg Q9_fpsp_handler_b04), Software-Breakpoints ueber Illegal Instruction,",
        "generisches pro-Prozess-Vektor-Handler-System, und als Fallback Signal-Zustellung (Signalnummer = (Vektor>>2)+0x64) oder Sprung nach Q9_exc_no_handler_fc4.",
    ],
    "000ba4": [
        "Trace-Exception-Handler (Single-Step-Debugging), minimaler Epilog: loescht bei Bedarf das Trace-Bit im geretteten Statuswort, sonst Fallthrough nach Q9_signal_pending_bc0.",
    ],
    "000bc0": [
        "Signal-/Breakpoint-Pending-Verwaltung: durchsucht die Deskriptorkette (0x2ac,A5) nach einem passenden Eintrag, markiert Treffer, wendet auf Nichttreffer dasselbe Aging-Muster wie Q9_scheduler_183a an.",
    ],
    "000fc4": [
        "Fallback ohne installierten Vektor-/Signal-Handler -- hinterlegt die Signalnummer in (0x26,A4) und springt nach Q9_exc_default_action_24d8.",
    ],
    "000fe0": [
        "FPU-Kontext sichern (Lazy-Context-Switch, Save-Haelfte). Parameter: A1=betroffener Prozess.",
        "Falls (0x334,A1) gesetzt: FSAVE nach (0x74,A1), loescht D_FProc (0x58,A6), sichert bei Bedarf Daten-/Kontrollregister.",
    ],
    "001004": [
        "Migriert den Inhalt eines bereits belegten FPU-Save-Bereichs, bevor er ueberschrieben wird. Von Q9_fpu_save_fe0 aufgerufen.",
    ],
    "001034": [
        "FPU-Kontext wiederherstellen (Restore-Haelfte, Gegenstueck zu Q9_fpu_save_fe0): FRESTORE (0x74,A1), setzt D_FProc = A4.",
    ],
    "00134a": [
        "Wrapper um Q9_owns_range_5d68 -- setzt Carry-Flag bei ungueltigem Adressbereich. Von Q9_disp_8d0s Breakpoint-Pfad genutzt.",
    ],
    "0067a0": [
        "Zentraler Kernel-Bootstrap (~1600 Byte). Alloziert die 256-Eintrags-Exception-Sprungtabelle D_ExcJmp (0x68,A6) und befuellt sie aus einer",
        "kompakten Quelltabelle im Modul (0x3802). Alloziert und kopiert die zwei Syscall-Tabellen (0x3a4,A6)/(0x3a8,A6) aus derselben PC-relativen",
        "Quelle bei 0x1380 (Fehler-Stub + F$Alarm-Dispatcher Q9_alarm_dispatch_1390, nicht Adresslisten). Fortsetzung: Q9_boot_finalize_6de4.",
    ],
    "000b04": [
        "FPSP-Handler (Floating-Point Software Package) fuer Vektor 48 (Branch/Set on Unordered), nutzt den EA-Decoder bei 0xb3a zur Zieladressberechnung.",
    ],
    "00183a": [
        "Scheduler: fuegt einen Prozess in die Ready-Queue ein (zirkulaere doppelt verkettete Liste, Sentinel bei (0x37c,A6)). Parameter: A0=Prozessdeskriptor.",
        "Prioritaets-Aging ueber globalen Countdown (0x3c4,A6); Sortier-Schluessel aus Prioritaet (0x18,A0) + Echtzeit-/Boost-Flag (Bit 7 in (0x1c,A0)).",
        "Kehrt sofort zurueck, falls der Prozess laut Zustandsbyte (0x20,A0) schon aktiv ('a') ist. 12 bekannte Aufrufer (siehe REVERSE_ENGINEERING.md).",
    ],
    "003140": [
        "Cache-Flush-Schleife (patcht selbstmodifizierten Code, erkennbar am 0x4AFC-Platzhalterwort, invalidiert die Datencache-Zeile einzeln) +",
        "Trampolin-Sprung in Syscall-Tabellen-Slot 90 (Kontextwechsel-Einstieg, Zieladresse nicht statisch im Modul sichtbar).",
    ],
    "001390": [
        "F$Alarm-Dispatcher (Register-Konvention passend zum Handbuch: D0.L=Alarm-ID, D1.W=Alarm-Funktionscode, D2.L=Signalcode, D3.L=Zeitintervall/-punkt, D4.L=Datum).",
        "D1 waehlt eine von 6 Operationen (echte Sprungtabelle bei 0x13c6 dekodiert): 0=A$Delete, 1/2=A$Set, 3/4=A$Cycle, 5=nicht implementiert (Fehler-Stub). Ueber Tabellen-Slot 8 der Syscall-Tabellen erreichbar.",
        "Bestaetigt durch das 0xB0BD-Signaturwort, das sowohl beim Anlegen (Q9_alarm_desc_alloc_162c-Umfeld) als auch beim Loeschen (Q9_alarm_delete_1424) verwendet wird.",
    ],
    "001424": [
        "A$Delete: durchlaeuft die Alarm-Deskriptorliste (0x37c,A4, prozessrelativ), prueft je Eintrag die 0xB0BD-Struktursignatur",
        "(loest damit das Modul-Header-Raetsel bei Offset 0x40 -- dieselbe Signatur, kein Zufall), ruft dann Q9_alarm_unlink_14ae.",
    ],
    "0014ae": [
        "Haengt einen Alarm-Deskriptor aus zwei parallelen verketteten Listen gleichzeitig aus (Felder 0xc/0x10 und 0x14/0x18) und gibt ihn frei (bra.w 0x131c).",
    ],
    "0025f8": [
        "Prozessdeskriptor-Slot aufraeumen: Ressourcenliste 1 (0xc8-0x100,A0), Tabelle offener Pfade (0x1a8,A0 abwaerts, echtes TRAP #0-Close),",
        "FPU-Ownership-Aufraeumen, Ressourcenliste 2 (0x38,A0). Parameter: A0/A4=zu bereinigender Prozessdeskriptor.",
        "Verwendet sowohl bei echter Terminierung (0x25f0/0x2966) als auch bei Prozess-Neuerzeugung auf einem wiederverwendeten Slot (0x19c4).",
    ],
    "001e18": [
        "Duenner Weiterreicher zu Q9_proc_id_free_3370 (A0 = (0x44,A6)).",
    ],
    "003370": [
        "Gibt eine Prozess-ID in der ID-Tabelle (0x44,A6) frei, eigene Freiliste wiederverwendbarer IDs. Fehlercode 0xE0 bei ungueltiger ID.",
    ],
    "002590": [
        "Trampolin Tabellen-Slot 89 -- Vorbereitung kurz vor dem Sterben eines Prozesses, aus Q9_exc_default_action_24d8 und der 0x25f0-Umgebung genutzt.",
    ],
    "0024d8": [
        "Standardaktion fuer unbehandelte Exceptions (aus Q9_exc_no_handler_fc4 erreicht). Zwei Faelle:",
        "Fall A (kein Signal-Handler): Prozess stirbt -- Zustand '-' (Zombie), Eltern-Benachrichtigung via Q9_parent_notify_4518, Prozess-ID-Freigabe, endet mit Q9_reschedule_trampolin_3140.",
        "Fall B (Signal-Handler vorhanden): normale Signal-Zustellung statt Terminierung.",
    ],
    "0062da": [
        "Gibt die pro Prozess gehaltenen Ressourcen beim Exit frei: Speicherblock-Chunk-Liste (0x2d8,A4) und Fixgroessen-Ressourcenliste (0x390,A4),",
        "je Eintrag ueber Q9_mem_free_5a22. Parameter: D0=Prozessdeskriptor. Aufgerufen aus Q9_proc_slot_cleanup_25f8.",
    ],
    "002cee": [
        "Prozess-ID-Lookup/-Validierung (Index+Generation-Schema gegen versehentliche Wiederverwendung). Fehlercode 0xE0 bei ungueltiger ID (derselbe wie Q9_proc_id_free_3370).",
    ],
    "004518": [
        "Eltern-Benachrichtigung beim Kindprozess-Tod: durchsucht die Wait-Deskriptor-Liste des Elternprozesses via Q9_proc_id_lookup_2cee,",
        "weckt einen wartenden Elternprozess (Zustand 'w') ueber Q9_scheduler_183a -- SIGCHLD/wait()-artiges Muster.",
    ],
    "004078": [
        "Internes Trampolin, Tabellen-Slot 88 (feste Parameter D0=0x30, D1=1). Der uebergebene Ressourcenzeiger dient nur als Null-Check, nicht als Adressparameter.",
    ],
    "00131c": [
        "Zwei separate, benachbarte Freigabe-Einstiegspunkte (kein Parameter-Dispatch, wie zunaechst vermutet): 0x131c ruft Q9_mem_free_5a22 direkt",
        "(mit Flag 1 auf dem Stack), 0x1330 ruft stattdessen die eigentumsgeprueften Q9_dealloc_owned_5cd2. Beide springen zum selben Austrittspunkt 0x12c6.",
        "Von Q9_arena_lookup_5bac und Q9_alarm_unlink_14ae genutzt.",
    ],
    "004978": [
        "Initialisiert feste System-Global-Konstanten: (0x70,A6)=0x10 (Speicher-Alignment), (0x7c,A6)=0x100 (Groessenkonstante). Kein Allocator (fruehere Fehlannahme korrigiert).",
    ],
    "005440": [
        "Speicher-Allokations-Primitive (Gegenstueck zu Q9_mem_free_5a22). Register: D0=Groesse, D1=Klassen-/Typ-Tag, Stack: Ausgabe-Zeiger, Arena-Listenkopf, Interrupt-Maskieren-Flag.",
        "Algorithmus: First-Fit ueber die Arena-Kette (passender Klassen-Tag + aktiviert + nicht gesperrt), innerhalb der Arena First-Fit in deren Freiliste.",
        "Exakter Treffer: Block komplett aushaengen. Restflaeche: Split von hinten (Freilisten-Eintrag behaelt seine Adresse, Ergebnis ist das hintere Ende).",
        "Fehlercodes: 0xAB (keine Arena mit genug Platz), 0xE1 (Groesse 0), 0xED (Arena-Liste leer).",
    ],
    "00526c": [
        "Arena-Deskriptor-Allocator: rundet die Zielgroesse aus, alloziert ueber Q9_mem_alloc_5440, mit Fallback auf den zweiten Speicherpool bei Fehlercode 0xED.",
    ],
    "0055a4": [
        "Pool-Lookup: findet den zustaendigen Speicherpool-Deskriptor fuer eine Adresse/Groesse (Bitmasken-Vergleich gegen Pool-Grenzen). Fehlercode 0xDB bei Nichttreffer.",
    ],
    "005712": [
        "Groessen-/klassensortierte Freiliste auf Arena-Ebene: Parameter <=0 schrumpft einen bestehenden Eintrag, >0 fuegt sortiert (Klasse, dann Groesse) neu ein.",
    ],
    "005a22": [
        "Zentrale Speicherfreigabe-Primitive (Gegenstueck zu Q9_mem_alloc_5440). Register: D0=Groesse, D1=Adresse, Stack-Parameter 1 (Pool-Typ, vermutet).",
        "Rundet die Groesse aus, versucht Pool (0x3fc,A6), bei Ablehnung (Fehlercode 0xDB) Pool (0x50,A6)+0x390. Boundary-Tag-Coalescing beim Einfuegen.",
        "Interrupt-Maskierung sauber auf allen Ausstiegspunkten via Q9_irq_mask_10e6/Q9_irq_unmask_10f2.",
    ],
    "005bac": [
        "Arena-Lookup-oder-Erzeugen fuer eine freizugebende Adresse. Register: D0=Groesse, D1=Adresse, Stack: Pool-Header, Ausgabe-Zeiger.",
        "Sucht die Arena, deren Adressbereich die Adresse abdeckt; bei Bedarf neuer 42-Byte-Arena-Deskriptor via Q9_dealloc_tail_131c. Fehlercode 0xD2 bei unbekannter Region.",
    ],
    "005c7c": [
        "Genereller, nach Klassen-/Typ-Tag sortierter Doppelverkettungs-Insert (auch fuer Arenen in Q9_arena_lookup_5bac genutzt).",
    ],
    "005d68": [
        "Prueft, ob ein Adressbereich zu den vom aktuellen Prozess gehaltenen Speicherbloecken gehoert (durchlaeuft dieselbe Chunk-Liste (0x2d8,A0) wie Q9_proc_resource_free_62da). Fehlercode 0xD2 bei Bereichen ausserhalb.",
    ],
    "0010e6": [
        "Interrupts bedingt maskieren. Register: D0=Boolean ('ueberhaupt maskieren?'). Hebt IPL auf 7 an falls D0!=0. Rueckgabe: alte SR in D0w.",
    ],
    "0010f2": [
        "SR wiederherstellen -- Gegenstueck zu Q9_irq_mask_10e6. Register: D1w=alte SR (von Q9_irq_mask_10e6 zurueckgegeben).",
    ],
    "0007f6": [
        "Panik-/Diagnose-Reporter mit zwei Einstiegspunkten: 0x7f6 (sichert Kontext selbst) und 0x804 (Kontext schon vom Aufrufer gesichert, druckt zusaetzlich Vektor-Offset + fehlerhafte PC-Adresse in Hex).",
        "Kein echter Halt -- protokolliert (ueber Q9_console_puts_850/Q9_console_puthex_868) und kehrt per RTS zum Aufrufer zurueck.",
    ],
    "0007be": [
        "Rettungsanker: bricht einen unterbrochenen internen Trampolin-Aufruf (erkennbar an (0x144,A4), demselben Feld wie in Q9_disp_488) kontrolliert mit synthetisiertem",
        "Fehlercode ab, statt in die volle Panik-Ausgabe (Q9_panic_report_7f6) zu fallen. Bedingungen: 0x4AFC-Platzhalter an (0,A6), Master-Stack-Bit gesetzt, gueltiger Ruecksprungzeiger.",
    ],
    "000834": [
        "Verzoegerungs-/Timeout-Schleife, pollt auf Konsolen-Bereitschaft (D0=0x320000 als Timeout-Zaehler). Von Q9_panic_report_7f6 genutzt.",
    ],
    "000850": [
        "Gibt einen nullterminierten ASCII-String (A0=Zeiger) auf der Systemkonsole aus, ueber die Treiber-Aufruftabelle (0x64,A6)+8.",
    ],
    "000868": [
        "Gibt einen 32-Bit-Wert (D0) hexadezimal aus -- rekursiv, ein Nibble pro Aufruf ueber ROR.L, Ausgabe via Q9_console_puts_850s Treiberzeiger.",
    ],
    "0006a8": [
        "Periodischer Uhr-Tick-Handler: erhoeht den Tick-Zaehler (0x54,A6), schreibt Sekunden-/Tageszaehler fort, erkennt Zeitscheiben-Ablauf",
        "und setzt bei Bedarf das Reschedule-Flag (Bit 5 in (0x1c,A2)) -- die Zeitscheiben-Ablauf-Erkennung des Multitasking-Schedulers.",
    ],
    "00070c": [
        "Selbstregistrierung als IRQ-Dispatcher-Tick-Hook: traegt sich in (0x8c0,A6) ein (der Hook, den Q9_disp_180 pro Interrupt aufruft), oder haengt sich ans Ende einer bestehenden Kette.",
    ],
    "0000b2": [
        "TRAPF.L-Padding (6 Byte, Ausrichtung) gefolgt von einem indizierten Trampolin-Dispatch ueber Tabelle (0x8e4,A6). Index kommt unskaliert vom Aufrufer-Stack.",
        "Aufrufer/genauer Zweck von (0x8e4,A6) noch nicht geklaert (siehe REVERSE_ENGINEERING.md).",
    ],
    "002398": [
        "Generischer Tabellen-Lookup: Index*32 in ein grenzgeprueftes Array (0x3cc,A6), Grenze (0x3d0,A6), prueft ein Tag-Wort im Treffer gegen (0,A5).",
        "Rueckgabe ueber Carry (0=Treffer, 1=Fehler: ausserhalb der Grenze oder Tag-Mismatch, beides Sprung nach 0x2432).",
    ],
    "00244c": [
        "11. bekannter Aufrufer von Q9_scheduler_183a: laeuft eine verkettete Liste ab, berechnet ueber Q9_table_lookup_2398 einen Pruefwert",
        "(Vergleich gegen einen PC-relativen Tabellen-Anker), und weckt/reiht den gefundenen Prozess per Q9_scheduler_183a neu ein.",
    ],
    "002dfc": [
        "IRQ-Handler-Ketten-Slot-Lookup: bildet einen Vektorwert (D0) auf einen Kettenkopf ab -- dieselben Offsets A6+D0+0x384 (Vektor <0x80)",
        "bzw. A6+D0-0x5c (Vektor >=0x80) wie in Q9_disp_180. Vorstufe zur Registrierung eines Handlers, Fallthrough nach Q9_irq_chain_insert_2eb2.",
    ],
    "002eb2": [
        "Durchsucht die von Q9_irq_chain_lookup_2dfc gefundene Kette und haengt einen neuen IRQ-Handler-Deskriptor ein.",
        "Interrupt-Maskierung hier inline (move SR,-(SP) / ori #0x700,SR) statt ueber Q9_irq_mask_10e6/Q9_irq_unmask_10f2.",
    ],
    "002c84": [
        "Berechnet einen Prioritaets-/Sortierwert fuer einen Prozess: Q9_proc_id_lookup_2cee, dann derselbe Sortierschluessel (0x2e0,A1) und",
        "Aging-Zaehler (0x3c4,A6) wie in Q9_scheduler_183a, geklemmt gegen die zweite Schwelle (0x8a8,A6). Springt danach in Q9_field_tag_set_1b4c.",
    ],
    "001b4c": [
        "Setzt ein getaggtes 24-Bit-Feld: Wert per ANDI.L #$ffffff auf 24 Bit maskiert, danach das hohe Byte per ST (Set-Byte) auf 0xFF erzwungen",
        "(klassisches Tag+Wert-Packing in einem Langwort). Enthaelt die bekannten Ueberlappungs-Sprungziele 0x1b90/0x1b9c/0x1ba8.",
    ],
    "0035f6": [
        "12. bekannter Aufrufer von Q9_scheduler_183a: prueft Prozesszustand 0x61 ('a', aktiv) und Listenende, setzt Flag-Bit 7 in (0x371,A1),",
        "weckt einen schlafenden Prozess und reiht ihn per Q9_scheduler_183a neu in die Ready-Queue ein.",
    ],
    "00362c": [
        "Weiterer 0x4AFC-Platzhalter-Patch-Mechanismus wie Q9_reschedule_trampolin_3140 (prueft (A0) gegen 0x4AFC, patcht bei Bedarf),",
        "aber mit anderer Zielstruktur (Checksummen-/Namensfeld-Manipulation statt reinem Cache-Flush) -- Details nicht letztgueltig verifiziert.",
    ],
    "0039b2": [
        "Fehler-0xAB-Rueckgabe-Stub (Speicher-Allokationsfehler 'keine Arena mit ausreichend freiem Speicher', siehe Q9_mem_alloc_5440).",
    ],
    "006de4": [
        "Letzter Abschnitt des Kernel-Bootstraps (Fortsetzung von Q9_kernel_init_67a0): Prozess-ID-Validierung via Q9_proc_id_lookup_2cee,",
        "invalidiert zwei Deskriptorfelder, bedingter TRAP-#0-Modulaufruf, setzt System-Global-Flag bei Offset 0x2, raeumt Tabellen-Slot-90-Bereich",
        "auf (0x168/0x16a/0x16c, je per TRAP #0), und endet mit Sprung in Q9_reschedule_trampolin_3140 -- der Kernel startet damit den Scheduler.",
    ],
    "0027d6": [
        "Modultabellen-Suche nach Signatur-Bytes (F$Find-artig): durchlaeuft eine 16-Byte-Eintrags-Tabelle (0x3c,A6) bis (0x40,A6),",
        "vergleicht zwei Filter-Bytes gegen Eintragsfelder ueber Q9_module_name_match_32fa und Q9_pattern_match_1ab8.",
    ],
    "0032fa": [
        "Zeichen-fuer-Zeichen-Vergleichshelfer (A0=String), zaehlt in D1 die verglichenen Zeichen, Rueckgabe ueber Carry/Bit 31. Von Q9_syscall_27d6 genutzt.",
    ],
    "001ab8": [
        "Musterabgleich mit '*'-Wildcard-Unterstuetzung (0x2a='*' im Code erkennbar). Von Q9_syscall_27d6 genutzt.",
    ],
    "005cd2": [
        "Alternative Freigabe-Variante: rundet Groesse/Adresse aus wie Q9_mem_alloc_5440, prueft Eigentum via Q9_owns_range_5d68 (Fehlercode 0xD2 bei Nicht-Eigentum)",
        "bevor tatsaechlich freigegeben wird. Von Q9_dealloc_tail_131c als Alternative zu Q9_mem_free_5a22 angesprungen.",
    ],
    "00157e": [
        "Kategorie 1 (echtes Sprungtabellenziel, direkt bei 0x13c6 im Code dekodiert -- nicht geraten): setzt nur D1=0 vor, faellt dann in Q9_alarm_set_1580 durch.",
    ],
    "001580": [
        "A$Set: Kategorie 2 von Q9_alarm_dispatch_1390 (echtes Sprungtabellenziel). Sendet ein Signal nach Ablauf des Zeitintervalls D3.",
        "Prueft zwei Konfigurationsworte (0x2,A6)/(0x28,A6), validiert D0 (Vorzeichen-Test + Fehlerpfad, ruft 0x3b96),",
        "ruft Q9_alarm_desc_alloc_162c und dann Q9_alarm_insert_15c4 zum Einsortieren. Fehlercodes ueber Sprung nach 0x2e4a bzw. 0x146a.",
    ],
    "001534": [
        "A$Cycle: Kategorie 3 von Q9_alarm_dispatch_1390 (echtes Sprungtabellenziel; Kategorie 4 = Q9_alarm_cycle_1540, ueberspringt nur den Anfang).",
        "Sendet ein Signal periodisch bei jedem Ablauf des Intervalls. Berechnet ueber die Konstante 0x15180 (86400 = Sekunden/Tag) und den Sekunden-/",
        "Tageszaehler (0x34,A6)/(0x30,A6) einen Tick-Wert (dieselbe Formel wie im Uhr-Tick-Handler Q9_clock_tick_6a8), validiert gegen ein Limit, ruft ebenfalls Q9_alarm_desc_alloc_162c.",
    ],
    "001540": [
        "Kategorie 4 von Q9_alarm_dispatch_1390 (echtes Sprungtabellenziel): identisch zu Q9_alarm_cycle_1534, ueberspringt nur den Aufruf von 0x2eea",
        "(D3/D4 werden direkt uebernommen statt ueber 0x2eea aufbereitet zu werden) -- vermutlich Variante fuer einen bereits vorbereiteten Aufrufer.",
    ],
    "00162c": [
        "Gemeinsamer Alarm-Deskriptor-Allocator (0x74=116 Byte): alloziert ueber Q9_fixed_alloc_wrap_12b4, nullt Statusfelder, haengt den neuen",
        "Deskriptor an eine per-Prozess-Liste bei Offset 0x37c (A4, dieselbe Liste, die Q9_alarm_delete_1424 durchlaeuft) an, kopiert zuletzt",
        "72 Byte einer Aufrufer-Vorlage in den Deskriptor. Setzt NICHT die 0xB0BD-Signatur -- das passiert erst in Q9_alarm_insert_15c4.",
    ],
    "0015c4": [
        "Sortiertes Einfuegen eines Alarm-Deskriptors nach Faelligkeit (Vergleich gegen Felder 0x20/0x24 bestehender Eintraege), setzt dabei die",
        "0xB0BD-Struktursignatur. Registriert bei Bedarf einmalig (Lazy-Init) den periodischen Uhr-Tick-Hook Q9_clock_hook_install_70c --",
        "der Kernel installiert den Zeitgeber also erst, wenn tatsaechlich ein Alarm existiert.",
    ],
    "00161a": [
        "Duenner Wrapper um Q9_alarm_insert_15c4 (Register D0/D1 -> A0/A2 umgesetzt, Rueckgabe D0=0).",
    ],
    "0016a0": [
        "Echtes Sprungtabellenziel fuer Kategorie 5 von Q9_alarm_dispatch_1390: nur 'bra.w 0x1380' (der gemeinsame Fehler-Stub) --",
        "Kategorie 5 (vermutlich A$AtDate/A$AtJul) ist in diesem Kernel-Build schlicht NICHT implementiert.",
    ],
    "0016aa": [
        "KORREKTUR: entgegen frueherer Annahme NICHT ueber Q9_alarm_dispatch_1390 erreichbar (das echte Kategorie-5-Ziel ist Q9_alarm_unimplemented_16a0, ein reiner Fehler-Stub).",
        "Liest trotzdem die ID-Tabelle (0x44,A6), schreibt den Kanarienvogel 'Jimi' (0x4A696D69) und setzt zwei Zeitfelder aus Tick-/Tages-Systemzaehlern --",
        "Zweck und tatsaechlicher Aufrufer wieder offen, nicht mehr als A$AtDate/A$AtJul einzuordnen.",
    ],
    "0012b4": [
        "Generischer Wrapper: alloziert einen Block ueber Q9_arena_alloc_526c (Klassen-Tag D1=0, Groesse vom Aufrufer per Stack-Parameter),",
        "setzt Carry bei Fehlschlag. Direkter Aufrufer von Q9_alarm_desc_alloc_162c; benachbarte Varianten (0x12d8, 0x12fa) allozieren+kopieren",
        "bzw. nutzen einen anderen Allocator-Einstieg (0x57be) -- eine kleine Familie generischer Alloc-Wrapper, nicht selbst timerspezifisch.",
    ],

    "00128e": [
        "Baut D0=A5 (Basisadresse), D1=A5+0x20 auf und ruft eine Hilfsroutine bei 0x63f0 auf; Fehler wird ueber das Carry-Ori-Muster ($ori #1) signalisiert.",
        "Entspricht F$Trans (Code 0x60), Adressuebersetzung fuer den aufrufenden Prozess.",
    ],
    "0012a6": [
        "Duenner Wrapper: ruft Q9_fixed_alloc_wrap_12b4 auf und uebertraegt Adresse (D0) und Groesse/Restwert (A2) in den Parameterblock ((0,A5)/(0x28,A5)).",
        "Entspricht F$SRqMem (Code 0x28), Supervisor-Tabellen-Eintrag.",
    ],
    "0012f0": [
        "Pusht eine PC-relative Adresse als Parameter, sichert Register und springt in eine gemeinsame Fortsetzung bei 0x12ba (nicht separat benannt).",
        "Entspricht F$SRqCMem (Code 0x5c), Supervisor-Tabellen-Eintrag -- die kontinuierliche/nicht-kontinuierliche Variante von F$SRqMem.",
    ],
    "0012fa": [
        "User-Tabellen-Variante von F$SRqMem: baut den Aufrufparameterblock auf dem Stack auf (D0=SP) und ruft die volle Allokationsroutine bei 0x57be auf,",
        "statt des Kurzwrappers, den die Supervisor-Variante (Q9_srqmem_12a6) nutzt. Entspricht F$SRqMem (Code 0x28), User-Tabellen-Eintrag.",
    ],
    "001312": [
        "User-Tabellen-Variante von F$SRqCMem: pusht eigene Parameter und springt dann mitten in den Koerper von Q9_srqmem_user_12fa (ab dessen Parameteraufbau),",
        "nutzt also dieselbe Allokationsroutine 0x57be wie F$SRqMem. Entspricht F$SRqCMem (Code 0x5c), User-Tabellen-Eintrag.",
    ],
    "001330": [
        "User-Tabellen-Variante von F$SRtMem: ruft statt der direkten Freigabe Q9_dealloc_owned_5cd2 auf, das vorher per Q9_owns_range_5d68 prueft,",
        "ob der freizugebende Bereich dem aufrufenden Prozess tatsaechlich gehoert. Entspricht F$SRtMem (Code 0x29), User-Tabellen-Eintrag.",
    ],
    "00133c": [
        "Liefert D0=A5 (Basisadresse) und D1=A5+0x24 (obere Grenze) an eine Hilfsroutine und teilt sich danach die Fehlerbehandlungs-Rueckkehr mit Q9_trans_128e.",
        "Entspricht F$Mem (Code 0x07): liefert Speichergrenzen/-bedarf des aufrufenden Prozesses.",
    ],
    "001366": [
        "Ruft eine Hilfsroutine bei 0x6232 mit Parametern A5 (Blockadresse) und A0 auf; Fehler ueber das uebliche Carry-Ori-Muster.",
        "Entspricht F$GBlkMp (Code 0x19): liefert Block-Map-Information (Speicherbelegung) zurueck.",
    ],
    "00137c": [
        "Trivialer Stub: setzt D1=0 (kein Fehlercode) und kehrt sofort zurueck, ohne den Speicherbereich tatsaechlich zu pruefen.",
        "Entspricht F$ChkMem (Code 0x58), User-Tabellen-Eintrag -- in diesem Kernel-Build fuer User-Aufrufe faktisch eine Nulloperation.",
    ],
    "0013d2": [
        "Der eigentliche Kategorie-Dispatch-Kern von F$Alarm (D1.W=Alarm-Funktionscode, Sprungtabelle bei 0x13c6, siehe Q9_alarm_dispatch_1390).",
        "Supervisor-Tabellen-Eintrag: verschachtelte F$Alarm-Aufrufe springen direkt hierher und ueberspringen den aeusseren Parameterblock-Aufbau,",
        "den der User-Tabellen-Eintrag (Q9_alarm_dispatch_1390, per bsr von dort aus aufgerufen) zusaetzlich durchlaeuft.",
    ],
    "0016a8": [
        "Tauscht A1/A2, ruft Q9_procdesc_alloc_16b6 auf und schreibt bei Erfolg den neuen Deskriptor nach (0x28,A5).",
        "Entspricht F$AllPrc (Code 0x4b), Supervisor-Tabellen-Eintrag: alloziert einen neuen Prozessdeskriptor.",
    ],
    "0016b6": [
        "Gemeinsame Prozessdeskriptor-Allokationsroutine: liest den Modulzeiger aus (0x44,A6), ruft mit einer Typkennung (D1.W, hier 0xe5) Q9_desc_slot_alloc_171a auf",
        "und verknuepft den neuen Deskriptor. Von F$AllPrc, F$Fork und F$DFork gemeinsam genutzt (nicht selbst ueber die Syscall-Tabelle erreichbar).",
    ],
    "00171a": [
        "Allociert einen Tabellenplatz fuer einen getaggten Deskriptor, parametrisiert ueber eine Typkennung in D1.W (0xc8 bei F$AllPD, 0xe5 bei Prozesserzeugung),",
        "ruft dafuer 0x12d8 auf (nicht separat benannt). Von Q9_procdesc_alloc_16b6 und F$AllPD (Q9_allpd_1708) gemeinsam genutzt.",
    ],
    "001708": [
        "Ruft Q9_desc_slot_alloc_171a mit Typkennung 0xc8 auf und schreibt bei Erfolg Adresse (D0) und Deskriptorzeiger (A1) in den Parameterblock.",
        "Entspricht F$AllPD (Code 0x30), Supervisor-Tabellen-Eintrag.",
    ],
    "001928": [
        "Erhoeht einen Verschachtelungszaehler (0x3ac,A4), markiert ggf. einen Eintrag in der Prozessliste (0x2ac,A4) und alloziert ueber Q9_fixed_alloc_wrap_12b4.",
        "Entspricht F$Chain (Code 0x05): laedt ein neues Programmabbild in den aktuellen Prozess (Chain to New Program).",
    ],
    "001b48": [
        "Dispatcht ueber die Systemglobal-Tabelle ((0x160,A3)/(0x560,A3), A3=(0x3a4,A6)) an eine dort registrierte Handlerroutine (dasselbe Muster wie F$Sema).",
        "Entspricht F$CpyMem (Code 0x1b): kopiert Speicher zwischen Adressraeumen (MMU-bewusst).",
    ],
    "001b88": [
        "Maskiert D1 auf 24 Bit (typisches OS-9-CRC24-Format) und ruft eine CRC-Berechnungsroutine bei ca. 0x1bc4 auf.",
        "Entspricht F$CRC (Code 0x17): berechnet/prueft eine 24-Bit-Pruefsumme ueber einen Speicherbereich.",
    ],
    "001cd0": [
        "Sucht per Q9_syscall_27d6 (Modultabellen-Suche) und Q9_module_name_match_32fa nach einem Modul und liefert dessen Datenbereichsadresse ((0x20,A5)).",
        "Entspricht F$DatMod (Code 0x25): liefert den Datenbereich eines geladenen Moduls.",
    ],
    "001e30": [
        "Sehr kurzer Stub (loescht ein CCR-Flag, kehrt sofort zurueck) -- der eigentliche Aufgaben-Loeschcode wird hier nicht aufgerufen.",
        "Entspricht F$DelTsk (Code 0x40), User-Tabellen-Eintrag; wirkt in diesem Kernel-Build fuer User-Aufrufe faktisch als Nulloperation.",
    ],
    "001e38": [
        "Loest per Q9_proc_id_lookup_2cee einen Prozessdeskriptor auf, vergleicht die Eltern-Kind-Beziehung ((0x2ac,A1) gegen A4) und mehrere Statusbits.",
        "Entspricht F$DExec (Code 0x23): liefert Debug-/Ausfuehrungsstatus eines Prozesses.",
    ],
    "002000": [
        "Aehnliches Muster wie Q9_dexec_1e38 (Prozessdeskriptor-Validierung ueber Q9_proc_id_lookup_2cee), endet aber mit Sprung in Q9_proc_id_free_wrap_1e18.",
        "Entspricht F$DExit (Code 0x24): beendet einen unter Debugger-Kontrolle laufenden Prozess.",
    ],
    "002038": [
        "Alloziert einen neuen Prozessdeskriptor ueber Q9_procdesc_alloc_16b6, verknuepft ihn mit dem Elternprozess (0x2ac,A1) und ruft eine weitere,",
        "hier nicht separat benannte Initialisierungsroutine (0x28aa) auf -- dieselbe, die auch F$Fork nutzt. Entspricht F$DFork (Code 0x22): Kindprozess unter Debugger-Kontrolle erzeugen.",
    ],
    "0020d0": [
        "Adressiert eine PC-relative Tabellenstruktur ueber A1 mit D1 als Wortindex (Bereichspruefung gegen 0x18).",
        "Entspricht F$Event (Code 0x53): OS-9-Event-Mechanismus (Signal-/Wartepunkt-Verwaltung).",
    ],
    "0026e8": [
        "Sucht in einer Tabelle (A1-Basis) per bereichsgeprueftem Index D0 und liefert den zugehoerigen Deskriptorzeiger.",
        "Entspricht F$FindPD (Code 0x2f), Supervisor-Tabellen-Eintrag: findet einen Prozessdeskriptor anhand einer ID.",
    ],
    "002718": [
        "Waehlt ueber einen bereichsgeprueften Index D0 (Grenzen 0x64/0x80/0x100) einen Eintrag in der IRQ-Tabelle bei (0x8e4,A6) aus.",
        "Entspricht F$FIRQ (Code 0x61), Supervisor-Tabellen-Eintrag: registriert einen schnellen (Fast-)IRQ-Handler.",
    ],
    "0027c0": [
        "Ruft eine kleine Vorbereitungsroutine (0x27d4, setzt A2=0) auf, die direkt in Q9_syscall_27d6 (Modultabellen-Suche) durchfaellt,",
        "und uebernimmt das Ergebnis in den Parameterblock. Entspricht F$FModul (Code 0x4e), Supervisor-Tabellen-Eintrag.",
    ],
    "002868": [
        "Alloziert einen Prozessdeskriptor (Q9_procdesc_alloc_16b6), initialisiert ihn ueber dieselbe Hilfsroutine (0x28aa) wie F$DFork und reiht den",
        "neuen Prozess direkt in die Scheduler-Ready-Queue ein (Sprung in Q9_scheduler_183a bei fehlender Debug-Kontrolle). Entspricht F$Fork (Code 0x03).",
    ],
    "002c48": [
        "Berechnet eine Groesse aus zwei Systemglobal-Feldern ((0x40,A6)-(0x3c,A6)) und uebergibt sie an Q9_field_tag_set_1b4c.",
        "Entspricht F$GModDr (Code 0x1a): liefert Modulverzeichnis-Information.",
    ],
    "002c68": [
        "Aehnliches Muster wie Q9_gmoddr_2c48, nutzt aber (0x44,A6) als Basis und ruft ebenfalls Q9_field_tag_set_1b4c.",
        "Entspricht F$GPrDBT (Code 0x1f): liefert die Basistabelle der Prozessdeskriptoren.",
    ],
    "002c88": [
        "Loest per Q9_proc_id_lookup_2cee einen Prozessdeskriptor auf und liest Status- sowie Typfelder ((0x18,A1)/(0x20,A1)) aus.",
        "Entspricht F$GPrDsc (Code 0x18): liefert Prozessdeskriptor-Daten zu einer Prozess-ID.",
    ],
    "002ce8": [
        "Duenner Wrapper: ruft unmittelbar Q9_proc_id_lookup_2cee auf und springt in eine gemeinsame Fortsetzung.",
        "Entspricht F$GProcP (Code 0x37): liefert den internen Zeiger auf einen Prozessdeskriptor.",
    ],
    "002d18": [
        "Wandelt eine Tageszeit in Sekunden per Division durch 3600 (Stunden) und 60 (Minuten/Sekunden) in Kalenderfelder um.",
        "Entspricht F$Gregor (Code 0x54): Julianisches Datum in Gregorianische Kalenderfelder umrechnen.",
    ],
    "002eea": [
        "Gemeinsame Datums-/Zeit-Zerlegungsroutine, von F$Julian (Q9_julian_2ee0) und F$STime (Q9_stime_3c08) genutzt (nicht selbst ueber die Syscall-Tabelle erreichbar).",
    ],
    "002dd0": [
        "Speichert einen Interrupt-/Signal-Handler-Zeiger (A0) und einen weiteren Parameter aus dem Aufruferblock im Prozessdeskriptor (A4).",
        "Entspricht F$Icpt (Code 0x09): setzt einen Signal-Intercept-Handler.",
    ],
    "002de0": [
        "Kopiert mehrere Felder (Prozess-ID, Owner, Zugriffsrechte) aus dem Prozessdeskriptor (A4) in den Parameterblock (A5).",
        "Entspricht F$ID (Code 0x0c): liefert Prozess-Identifikationsdaten des aufrufenden Prozesses.",
    ],
    "002e00": [
        "Waehlt analog zu Q9_firq_2718 einen Tabelleneintrag in der IRQ-Vektor-Struktur bei (0x8e4,A6) aus, mit erweitertem Bereichscheck (bis 0x100).",
        "Entspricht F$IRQ (Code 0x2a), Supervisor-Tabellen-Eintrag: registriert/entfernt einen normalen IRQ-Handler.",
    ],
    "002ee0": [
        "Ruft die gemeinsame Datums-Zerlegungsroutine Q9_date_decompose_2eea auf und sichert das Ergebnis.",
        "Entspricht F$Julian (Code 0x20): wandelt eine Sekunden-Zeitangabe in ein Julianisches Datum um.",
    ],
    "002f70": [
        "Ruft eine Modul-Verknuepfungsroutine (0x2fa4, nicht separat analysiert) auf und uebernimmt die Rueckgabewerte (D0/D1, A0/A1/A2) in den Parameterblock.",
        "Entspricht F$Link (Code 0x00), Supervisor-Tabellen-Eintrag.",
    ],
    "002fba": [
        "User-Tabellen-Variante von F$Link: erhoeht einen Verschachtelungszaehler (0x3ac,A4), sucht das Modul ueber Q9_syscall_27d6 und ruft zusaetzlich",
        "eine Statusroutine (0x2fdc) auf, mit Interrupt-Sperre waehrend der kritischen Sektion. Entspricht F$Link (Code 0x00), User-Tabellen-Eintrag.",
    ],
    "0030a0": [
        "Kopiert einen Speicherbereich byteweise mit Ausrichtungsbehandlung (Alignment-Test per btst/scs), Quell-/Zieladresse werden vorab verglichen.",
        "Entspricht F$Move (Code 0x38): kopiert Speicher innerhalb eines Adressraums.",
    ],
    "0032e0": [
        "Parst einen Pfadnamen ueber die Hilfsroutine 0x32f0 (ueberspringt ein fuehrendes '/'-Zeichen) und liefert Zeiger/Laenge im Parameterblock zurueck.",
        "Entspricht F$PrsNam (Code 0x10): zerlegt einen Pfadnamen in seine Bestandteile.",
    ],
    "0033c8": [
        "Dekrementiert unter Interrupt-Sperre einen Verschachtelungszaehler (0x370,A4) und stellt bei Erreichen von Null gesicherte Register (D5/D6)",
        "sowie den Ausfuehrungskontext wieder her (Sprung nach 0xeda). Entspricht F$RTE (Code 0x1e): kehrt aus einer verschachtelten Systemroutine zurueck.",
    ],
    "0034fe": [
        "Loest per Q9_proc_id_lookup_2cee den Zielprozess auf, prueft ein Statusbit und traegt unter Interrupt-Sperre in dessen Signalwarteschlange",
        "((0x37c,A1) ff.) ein. Entspricht F$Send (Code 0x08): sendet ein Signal an einen anderen Prozess.",
    ],
    "003630": [
        "Prueft die Modul-ID-Signatur (0x4AFC) eines Moduls, berechnet dessen CRC ueber Hilfsroutinen (0x3660/0x1ba4, siehe auch F$CRC) und schreibt sie in den Header.",
        "Entspricht F$SetCRC (Code 0x26): berechnet und setzt die Modul-Pruefsumme neu (z. B. nach einem Patch).",
    ],
    "003678": [
        "Decodiert einen Dienstindex (D1) und schlaegt ihn in einer Tabelle nach ((0x3a4,A6)-relativ); bei ungueltigem Index Sprung zum gemeinsamen Fehler-Stub (0x1380).",
        "Entspricht F$SSvc (Code 0x32), Supervisor-Tabellen-Eintrag: indirekter Aufruf eines registrierten Systemdienstes.",
    ],
    "003840": [
        "Prueft mehrere Statusfelder des Prozessdeskriptors, bevor ein Systemglobal-Wert geschrieben wird (Details nicht vollstaendig nachvollzogen).",
        "Entspricht F$SetSys (Code 0x27): setzt einen Systemglobal-Wert.",
    ],
    "003960": [
        "Manipuliert eine Signalmaske im Prozessdeskriptor abhaengig von Vorzeichen und Wert der Aufrufparameter (Details nicht vollstaendig nachvollzogen).",
        "Entspricht F$SigMask (Code 0x57): setzt/liest die Signalmaske eines Prozesses.",
    ],
    "0039a8": [
        "Gibt bedingungslos den Fehlercode 0xAC zurueck.",
        "Entspricht F$SigReset (Code 0x63), Supervisor-Tabellen-Eintrag -- in diesem Kernel-Build nicht implementiert.",
    ],
    "0039bc": [
        "Prueft ein Feld im Prozessdeskriptor (0x3b4,A4); der Fehlerpfad fuehrt in Richtung des bereits dokumentierten Fehler-Stubs Q9_err_ab_stub_39b2",
        "(Details nicht vollstaendig nachvollzogen). Entspricht F$SigReset (Code 0x63), User-Tabellen-Eintrag.",
    ],
    "0039f0": [
        "Aehnliches Verzoegerungs-/Statusmuster wie Q9_wait_user_4488: loescht/setzt ein Statusbit (0x371,A4) und faellt in den Rest von Q9_sleep_3a06.",
        "Entspricht F$Sleep (Code 0x0a), User-Tabellen-Eintrag.",
    ],
    "003a06": [
        "Prueft, ob der aktuelle Prozess (A4) mit dem Aufrufer-Kontext (0x50,A6) uebereinstimmt, und haengt ihn ueber eine Hilfsroutine (0x3b96)",
        "in eine Warteliste ein, mit Interrupt-Sperre waehrend der Statusaktualisierung. Entspricht F$Sleep (Code 0x0a), Supervisor-Tabellen-Eintrag.",
    ],
    "003bc8": [
        "Loest den Zielprozess per Q9_proc_id_lookup_2cee auf, aktualisiert dessen Prioritaetsfelder ((0x18,A1)/(0x1a,A1)) und stoesst bei aktiven",
        "Prozessen (Statusbyte 0x61) eine Neueinordnung an (Sprung in den F$AProc-Bereich bei 0x1844). Entspricht F$SPrior (Code 0x0d).",
    ],
    "003c08": [
        "Zerlegt die neue Systemzeit ueber dieselbe Hilfsroutine wie F$Julian (Q9_date_decompose_2eea) und schreibt mehrere Systemzeit-Globalfelder",
        "((0x2a/0x30/0x34,A6)). Entspricht F$STime (Code 0x16): setzt die Systemzeit.",
    ],
    "003d4a": [
        "Durchsucht eine Trap-Handler-Tabelle (Abbruchwert -1) und liefert bei Nichtfinden Fehlercode 0x85.",
        "Entspricht F$STrap (Code 0x0e): installiert einen Handler fuer TRAP #1-15.",
    ],
    "003d60": [
        "Prueft die Elternbeziehung ((0x38,A4)) und einen Berechtigungswert ((0x3a0,A4)), bevor die User-ID im Prozessdeskriptor ((0x14,A4)) geaendert wird.",
        "Entspricht F$SUser (Code 0x1c): setzt die User-ID eines Prozesses.",
    ],
    "003d98": [
        "Bereitet den Sprung in den residenten Debugger vor: sichert A4-A6/D0 und den User-Programmzaehler ((0x8ec,A6)), stellt anschliessend alle",
        "Register aus dem Parameterblock wieder her. Entspricht F$SysDbg (Code 0x52), User-Tabellen-Eintrag (mit vorgeschalteter Berechtigungspruefung).",
        "Dieselbe Route loest die im Rahmen der RomBug-Untersuchung dieser Sitzung beobachtete 'Timesharing HALTED'-Meldung aus (siehe docs/REVERSE_ENGINEERING.md).",
    ],
    "003da0": [
        "Supervisor-Tabellen-Eintrag fuer denselben Debugger-Einsprung wie Q9_sysdbg_user_3d98, springt aber direkt in den gemeinsamen Kern",
        "und ueberspringt die dortige Berechtigungspruefung. Entspricht F$SysDbg (Code 0x52), Supervisor-Tabellen-Eintrag.",
    ],
    "003e00": [
        "Liest mehrere Systemglobal-Felder aus (Details nicht vollstaendig nachvollzogen).",
        "Entspricht F$SysID (Code 0x55): liefert System-Identifikationsdaten.",
    ],
    "003eb0": [
        "Liest die Systemzeit-Felder zurueck (Gegenstueck zu F$STime, siehe Q9_stime_3c08); Details nicht vollstaendig nachvollzogen.",
        "Entspricht F$Time (Code 0x15): liefert die aktuelle Systemzeit.",
    ],
    "003f48": [
        "Prueft den angeforderten Traptyp (Vergleich gegen 0xf) und liefert bei Erfolg eine Kennung (0xe3) zurueck.",
        "Entspricht F$TLink (Code 0x21) -- laut Syscall-Tabellen-Befund nur in der User-Tabelle registriert, die Supervisor-Tabelle zeigt hier den Fehler-Stub.",
    ],
    "004070": [
        "Sehr kurzer Stub (loescht ein CCR-Flag, kehrt sofort zurueck), aehnlich Q9_chkmem_user_137c.",
        "Entspricht F$UAcct (Code 0x59) -- wirkt in diesem Kernel-Build praktisch als Nulloperation.",
    ],
    "0040b4": [
        "Durchsucht eine Modul-Tabelle ((0x3c,A6) bis (0x40,A6), 16-Byte-Eintraege) linear nach einer Adresse und meldet Fehlercode 0xDD, falls nicht gefunden.",
        "Entspricht F$UnLink (Code 0x02), Supervisor-Tabellen-Eintrag.",
    ],
    "004228": [
        "Sucht das Modul per Q9_syscall_27d6-Kette und ruft eine Entlade-/Freigaberoutine (0x410e, nicht separat benannt) auf, mit einem",
        "Verschachtelungszaehler (0x3ac,A4) waehrend der Operation. Entspricht F$UnLoad (Code 0x1d).",
    ],
    "004258": [
        "Ruft eine Validierungsroutine (0x429a, nicht separat benannt) auf und behandelt speziell den Fehlercode 0xE7.",
        "Entspricht F$VModul (Code 0x2e), Supervisor-Tabellen-Eintrag: prueft ein Modul auf Gueltigkeit (CRC etc.).",
    ],
    "004488": [
        "Durchsucht ueber eine Hilfsroutine (0x3984) offenbar die Kindprozessliste und setzt/loescht ein Statusbit (0x371,A4) je nach Ergebnis,",
        "bevor es in den gemeinsamen Rest (Q9_wait_449e) faellt. Entspricht F$Wait (Code 0x04), User-Tabellen-Eintrag.",
    ],
    "00449e": [
        "Wartet auf ein bestimmtes Kind (D0=Kind-Prozess-ID) via Q9_proc_id_lookup_2cee, benachrichtigt bei Fund den Elternprozess",
        "(Q9_parent_notify_4518) und gibt dessen Prozess-ID frei (Q9_proc_id_free_wrap_1e18). Entspricht F$Wait (Code 0x04), Supervisor-Tabellen-Eintrag.",
    ],
    "004828": [
        "Dispatcht analog zu F$CpyMem (Q9_cpymem_1b48) ueber die Systemglobal-Tabelle ((0x160,A3)/(0x560,A3)) an eine dort registrierte Handlerroutine.",
        "Entspricht F$Sema (Code 0x62): Semaphor-Operation (Signal/Warten).",
    ],
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

def emit_func_header(off):
    if off not in FUNC_HEADER:
        return
    name = LABEL_NAMES.get(off, "L" + off)
    out.append("*" + "-" * 70)
    out.append(f"* {name}  (0x{off})")
    for ln in FUNC_HEADER[off]:
        out.append(f"* {ln}")
    out.append("* Details/Kontext: docs/REVERSE_ENGINEERING.md")
    out.append("*" + "-" * 70)

def flush_data_run():
    global data_run
    if not data_run:
        return
    # Jede einzelne Ghidra-Code-Unit bekommt ihr eigenes Label (nicht
    # nur der Beginn des gesamten Laufs) -- sonst koennen PC-relative
    # Referenzen, die mitten in einen mehrbytigen Datenblock zeigen,
    # vom Linker nicht aufgeloest werden.
    for r in data_run:
        emit_func_header(r["off"])
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
    emit_func_header(r["off"])

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
