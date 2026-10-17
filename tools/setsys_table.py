#!/usr/bin/env python3
"""setsys_table.py -- Klassifikation der System-Globals fuer F$SetSys (Arbeitsplan C5).

F$SetSys liest und aendert System-Globals ueber ihren OFFSET (d0.w, 68k_tech.pdf S. 492). Im Original liegen sie alle im Systemglobalbereich.
Dieser Kernel fuehrt nur einen Teil davon an den Originaloffsets -- die uebrigen liegen auf fremdem Speicher (live gemessen: $55E ist
nicht D_MinPty, s. q9kernel_sched.c). Q9K_ProcSetSys liefert deshalb nur Werte, die der Kernel wirklich fuehrt, und sonst E$UnkSvc.

Dieses Werkzeug haelt fest, was fuer JEDE Variable aus common/src/q9sysglob.h gilt: umgesetzt (lesbar / lesbar und schreibbar) oder
offen mit Grund. Es prueft:
  1. jede Variable des Headers ist klassifiziert, und jede Klassifikation gehoert zu einer Variable des Headers,
  2. jede als umgesetzt markierte Variable kommt mit ihrem Offset in q9kernel_setsys.c vor,
  3. mit --doc: die Tabelle docs/KERNEL_SETSYS_VARIABLEN_de.md ist aktuell.

Aufruf:  tools/setsys_table.py --check [--doc <datei>]      Pruefung (Exit 1 bei Verstoss)
         tools/setsys_table.py --write <datei>              Tabelle als Markdown schreiben
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
HDR = os.path.join(ROOT, "Q9-KERNEL", "common", "src", "q9sysglob.h")
SRC = os.path.join(ROOT, "Q9-KERNEL", "68k", "src", "kernel", "q9kernel_setsys.c")

# Gruende fuer "offen": Schluessel -> Text
REASON = {
    "A": "Q9 fuehrt Prozess-, Pfad- und Moduldaten in eigenen Pools und Listen (s. KERNEL_SPEICHERBELEGUNG_de.md); ein Zeiger auf die Microware-Struktur waere falsch",
    "B": "Die Q9-Uhr haelt Tageszahl und Sekunden (q9kernel_clock.c) und den Tickzaehler, nicht diese Felder; als virtuelle Variable moeglich, bisher nicht gebraucht",
    "C": "Hardware oder Funktion, die dieses System nicht hat oder nicht modelliert (MMU/SPU, Multiprozessor, Cache-Steuerung, FPU-Zustand, CPU32-Register)",
    "D": "Interrupt-Verwaltung: Q9 fuehrt eigene Tabellen (irqtab), das Originalformat wird mit C6 (F$IRQ/F$FIRQ) entschieden",
    "E": "Debugger, ROM und Boot: Rueckkehr- und Einstiegsfelder gehoeren zu C7 (F$SysDbg/RomBug) und sind noch nicht umgesetzt",
    "F": "Speicherverwaltung: Q9 hat einen eigenen Arena-Allokator, die Felder des Originals (Freiliste, Blockgroessen) gibt es nicht",
    "G": "Interner Scheduler- oder Verwaltungszustand, den Q9 anders fuehrt (Alterung, Warteschlangen, Alarm-/Thread-Listen, Fast-Dispatch, Idle)",
    "H": "Beim Boot gesetzte Konfiguration oder Kennung, die Q9 nicht als Variable bereithaelt; aus dem Init-Modul oder dem Bauschalter ableitbar, bisher nicht gebraucht",
}

# Name -> (Klasse, Hinweis).  Klassen:  R = lesbar (Q9 fuehrt den Wert an diesem Offset),  W = lesbar und schreibbar (Q9-Wert),
#   V = lesbar, Q9-eigener Wert,  O<Grund> = offen
CLS = {}
def put(cls, names, note=""):
    for n in names.split():
        CLS[n] = (cls, note)

put("R", "INIT COMPAT SYSCONF MODDIR PROC SYSPRC FPROC SYSROM EXCJMP TOTRAM SYSDIS USRDIS COMPAT2",
    "echter Speicherinhalt der Systemglobals, vom Kernel beim Boot bzw. laufend gesetzt")
put("R", "TICKS", "Wert des Q9-Tickzaehlers (D_Ticks selbst wird nicht gepflegt)")
put("V", "PROCSZ", "konstant $400 (Groesse des Q9-Prozessdeskriptors)")
put("W", "TCKSEC", "Q9-Wert 100 (10 ms je Tick); setzbar")
put("W", "TSLICE", "Q9-Wert 2 Ticks; setzbar; $76 ist der aeltere Code derselben Zelle")
put("W", "BLKSIZ", "Q9-Wert 4096, die Wachstumsgroesse, die csl abfragt; setzbar")
put("W", "MINPTY", "Scheduler-Feld (q9kernel_sched.c): Prozesse darunter werden nicht ausgewaehlt; setzbar, nur Super-User")
put("W", "MAXAGE", "Scheduler-Feld (q9kernel_sched.c): hohe Prioritaet altert nicht, niedrige bis D_MaxAge; setzbar, nur Super-User")
put("OA", "PRCDBT PTHDBT DEVTBL DEVCNT DEVSIZ CACHE EVTBL EVID FORKS")
put("OB", "CLOCK YEAR MONTH DAY JULIAN SECOND TICK SLICE ELAPSE CLKMEM")
put("OC", "FPU FPUSIZE FPUMEM MPUTYP SPUMEM ADDRLIM SNOOPD IPID CPUS IPCMD CACHMODE DISINST DISDATA MBAR CRYSTAL IOGLOB MOVEMIN BOOTRAM")
put("OD", "IRQFLAG UNKIRQ SPURIRQ AUTIRQ2 VCTIRQ POLTBL FIRQVCT VCTJMP ABTSTK SYSSTK SSTKLM INIRQ FIRQOFF IRQOFF IRQSPOFF IRQHEADS")
put("OE", "SYSDBG DBGMEM DBGFLG PROFMEM")
put("OF", "MINBLK FREMEM ARENA FREEMEM ALLOCTYPE")
put("OG", "ACTIVQ SLEEPQ WAITQ ACTAGE THREAD ALARTH ALMQ1 ALMQ2 SIEZE PREEMPT FDISPQ FDISP FDISPSYS IDLE IDLEDATA SWITCHES NOSLEEP CIGAR")
put("OH", "ID NUMSIGS PRCDESCSTACK KERTYP BOOTFLAGS UNKN8A6 UNKN8A8")
put("X", "END", "Endmarke des Bereichs ($1000), keine Variable")

def parse_header():
    s = open(HDR, encoding="utf-8").read()
    rows = []
    for m in re.finditer(r"#define\s+Q9_D_(\w+)\s+(0x[0-9A-Fa-f]+)\s*/\*(.*?)\*/", s, re.S):
        name, off, c = m.groups()
        c = " ".join(c.split())
        st = re.search(r"\[([A-ZÄÖÜ]+)", c)
        text = re.sub(r"\s*\[.*$", "", c)
        rows.append((name, int(off, 16), st.group(1) if st else "-", text))
    return rows

LEGACY = {"BLKSIZ": [0x7C], "TSLICE": [0x76, 0x42E]}   # zusaetzlich im Code benutzte Offsets

def check(rows):
    errs = []
    names = {r[0] for r in rows}
    for n in names:
        if n not in CLS:
            errs.append("nicht klassifiziert: D_%s" % n)
    for n in CLS:
        if n not in names:
            errs.append("Klassifikation ohne Header-Variable: D_%s" % n)
    src = open(SRC, encoding="utf-8").read()
    for name, off, st, text in rows:
        cls = CLS.get(name, ("?",))[0]
        if cls in ("R", "V", "W"):
            offs = [off] + LEGACY.get(name, [])
            if not any(re.search(r"0x0*%X(?![0-9A-Fa-f])" % o, src, re.I) for o in offs):
                errs.append("D_%s ist als umgesetzt markiert, ihr Offset 0x%X steht aber nicht in q9kernel_setsys.c" % (name, off))
    return errs

def render(rows):
    out = []
    out.append("# F$SetSys: Klassifikation der System-Globals (Arbeitsplan C5)\n")
    out.append("<!-- erzeugt von tools/setsys_table.py -- nicht von Hand aendern -->\n")
    out.append("`F$SetSys` liest und aendert System-Globals ueber ihren Offset (`d0.w`, `d1` = Groesse 1/2/4 plus Bit 31 fuer Lesen, `d2` = neuer Wert; "
               "Ausgabe `d2` = vorheriger Wert). Nach dem Handbuch (68k_tech.pdf S. 492 f.) darf nur ein Super-User (Gruppe 0) aendern, und "
               "veraenderbar sind nur `D_MinPty` und `D_MaxAge`.\n")
    out.append("Dieser Kernel fuehrt nur einen Teil der Globals an den Originaloffsets; die uebrigen liegen in seinem Speicher auf anderen Daten "
               "(live gemessen: `$55E` ist nicht `D_MinPty`, ein Scheduler, der dort las, sperrte jeden Prozess aus). `F$SetSys` liefert deshalb "
               "nur Werte, die der Kernel wirklich fuehrt, und sonst `E$UnkSvc` — eine bewusste Abweichung vom Handbuch (\"Any system variable may be "
               "examined\"), weil ein geratener Wert gefaehrlicher ist als ein klarer Fehler.\n")
    n_r = sum(1 for n, (c, _) in CLS.items() if c == "R")
    n_v = sum(1 for n, (c, _) in CLS.items() if c in ("V",))
    n_w = sum(1 for n, (c, _) in CLS.items() if c == "W")
    n_o = sum(1 for n, (c, _) in CLS.items() if c.startswith("O"))
    out.append("**Stand:** %d Variablen im Header, davon %d lesbar am Originaloffset, %d mit Q9-eigenem Wert lesbar, %d lesbar und schreibbar, "
               "%d offen (begruendet), 1 Endmarke.\n" % (len(rows), n_r, n_v, n_w, n_o))
    out.append("**Fehler:** Groesse nicht 1/2/4 → `E$Param`; Schreiben ohne Super-User → `E$Permit`; Schreiben einer lesbaren, aber nicht "
               "veraenderbaren Variable → `E$Param`; unbekannte oder offene Variable → `E$UnkSvc`.\n")
    out.append("## Gruende fuer „offen“\n")
    for k in sorted(REASON):
        out.append("- **%s** — %s" % (k, REASON[k]))
    out.append("")
    out.append("## Tabelle\n")
    out.append("Spalte „Header“ ist der Pruefstatus der Offsetangabe in `q9sysglob.h` (VERIFIZIERT = am Referenzsystem beobachtet, PLATZHALTER = nur abgeleitet).\n")
    out.append("| Variable | Offset | Q9 | Grund / Hinweis | Header |")
    out.append("|---|---|---|---|---|")
    label = {"R": "✅ lesbar", "V": "✅ lesbar (Q9-Wert)", "W": "✅ lesbar + schreibbar", "X": "—"}
    for name, off, st, text in sorted(rows, key=lambda r: r[1]):
        cls, note = CLS[name]
        if cls.startswith("O"):
            q9 = "⬜ offen %s" % cls[1]
            why = REASON[cls[1]].split(";")[0].split(" (")[0]
            note2 = "%s — %s" % (text, why[:0] or "")
            why = text
        else:
            q9 = label[cls]
            why = note or text
        out.append("| `D_%s` | `$%03X` | %s | %s | %s |" % (name, off, q9, why.replace("|", "/"), st))
    out.append("")
    return "\n".join(out)

def main(argv):
    rows = parse_header()
    if "--write" in argv:
        path = argv[argv.index("--write") + 1]
        open(path, "w", encoding="utf-8").write(render(rows))
        print("geschrieben: %s (%d Variablen)" % (path, len(rows)))
        return 0
    errs = check(rows)
    if "--doc" in argv:
        path = argv[argv.index("--doc") + 1]
        try:
            cur = open(path, encoding="utf-8").read()
        except OSError:
            cur = None
        if cur != render(rows):
            errs.append("die Tabelle %s ist veraltet oder fehlt -- neu erzeugen mit: tools/setsys_table.py --write %s" % (path, path))
    for e in errs:
        print("FAIL  " + e)
    if errs:
        return 1
    print("ok    F$SetSys-Variablen: %d klassifiziert, umgesetzte stehen im Code" % len(rows))
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
