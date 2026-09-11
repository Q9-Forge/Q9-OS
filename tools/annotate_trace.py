#!/usr/bin/env python3
"""
annotate_trace.py -- ordnet jede Zeile einer Q9_TRACE_INSTR-Instruktionsspur
dem richtigen Modul+Symbol zu (statt von Hand Adressen umzurechnen) und
verfolgt automatisch die Call/Return-Bilanz (bsr/jsr vs. rts/rte), um eine
Stelle zu finden, an der die tatsaechliche Rueckkehradresse NICHT zu der
zuletzt gepushten passt -- das IST der gesuchte Stack-Fehler, mechanisch
statt per Handarbeit gefunden.

Nutzung: python3 annotate_trace.py <dump.txt> [--tail N]
         [--mod NAME=PFAD ...] [--kernel-map PFAD]

Voraussetzung: der Dump wurde mit Q9_TRACE_INSTR=1 erzeugt (s.
docs/OWN_KERNEL_STATUS.md, "Werkzeug" Q9_TRACE_INSTR/Q9_FREEZE_PC).
Die Moduldirectory (Ladeadressen) wird IMMER frisch aus dem Dump
gelesen, nie von Hand eingetragen -- nur die Wahl, WELCHE .mod-Dateien
fuer die Disassemblierung herangezogen werden, ist Kommandozeilensache.

Kernel-Symbol-Map erzeugen (falls "--kernel-map" fehlt, wird sie NICHT
automatisch neu gebaut -- s. Q9-OS/src/kernel/build.sh, Abschnitt
"l68.exe -s=" fuer den l68-Aufruf):
  cd Q9-OS/src/kernel/build && <l68-Aufruf wie in build.sh> \\
      -s=q9kernel_symcheck_map.txt
"""
import re
import sys
from pathlib import Path

import capstone

# Default-Pfade -- ueber --mod/--kernel-map ueberschreibbar, da Job-
# Verzeichnisse (fuer echo.mod/csl.mod) sich zwischen Sitzungen aendern.
DEFAULT_KERNEL_PATH = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-OS/src/kernel/build/q9kernel"
DEFAULT_KERNEL_MAP = "/Volumes/SSD1TB/projects/Q9-Forge/Q9-OS/src/kernel/build/q9kernel_symcheck_map.txt"

MOD_FILES = {
    "q9kernel": DEFAULT_KERNEL_PATH,
}
KERNEL_MAP = DEFAULT_KERNEL_MAP


def load_moddir(dump_text):
    """Liest 'Slot=... HdrPtr=XXXXXXXX ... Name="NAME"' direkt aus dem Dump --
    IMMER aktuell fuer den jeweiligen Lauf, nie von Hand nachgeschlagen."""
    mods = []
    for m in re.finditer(r'HdrPtr=([0-9a-fA-F]+)\s+Groesse=([0-9a-fA-F]+)\s+\S+\s+Name="([^"]+)"', dump_text):
        base = int(m.group(1), 16)
        size = int(m.group(2), 16)
        name = m.group(3)
        mods.append((name, base, size))
    mods.sort(key=lambda x: x[1])
    return mods


def load_kernel_symbols():
    """Symboltabelle aus der l68-Map: (offset_int, name), aufsteigend sortiert.
    Offset ist PSECT-relativ = Datei-Offset = (Adresse - Kernel-Ladeadresse)."""
    text = Path(KERNEL_MAP).read_text()
    syms = re.findall(r'(\S+)\s+(COD|DAT)\s+([0-9a-fA-F]{8})', text)
    out = [(int(o, 16), n) for n, t, o in syms]
    out.sort()
    return out


def resolve_symbol(symtab, offset):
    lo, hi = 0, len(symtab)
    best = None
    while lo < hi:
        mid = (lo + hi) // 2
        if symtab[mid][0] <= offset:
            best = symtab[mid]
            lo = mid + 1
        else:
            hi = mid
    if best is None:
        return "?", offset
    return best[1], offset - best[0]


class ModuleMap:
    def __init__(self, mods):
        self.mods = mods  # (name, base, size) sorted by base
        self.data = {}
        self.symtab = load_kernel_symbols()
        for name, base, size in mods:
            path = MOD_FILES.get(name)
            if path and Path(path).exists():
                self.data[name] = (base, Path(path).read_bytes())

    def locate(self, addr):
        """Findet, in welchem Modul 'addr' liegt (per Basis+Groesse aus dem
        LIVE-Dump, nicht geraten)."""
        for name, base, size in self.mods:
            if base <= addr < base + size:
                return name, base, addr - base
        return None, None, None

    def annotate(self, addr):
        name, base, off = self.locate(addr)
        if name is None:
            return f"0x{addr:08x} [unbekanntes Modul]"
        label = f"0x{addr:08x} [{name}+0x{off:x}]"
        if name == "q9kernel":
            sym, delta = resolve_symbol(self.symtab, off)
            label += f" ~{sym}+0x{delta:x}"
        return label

    def disasm_one(self, addr):
        name, base, off = self.locate(addr)
        if name is None or name not in self.data:
            return None
        modbase, blob = self.data[name]
        chunk = blob[off:off + 12]
        if not chunk:
            return None
        md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_M68K_040)
        for insn in md.disasm(chunk, addr):
            return insn
        return None


CALL_MNEMONICS = {"jsr", "bsr"}
RET_MNEMONICS = {"rts", "rte"}


def main():
    global KERNEL_MAP
    args = sys.argv[1:]
    path = args[0]
    tail = None
    if "--tail" in args:
        tail = int(args[args.index("--tail") + 1])
    if "--kernel-map" in args:
        KERNEL_MAP = args[args.index("--kernel-map") + 1]
    i = 0
    while i < len(args):
        if args[i] == "--mod":
            name, modpath = args[i + 1].split("=", 1)
            MOD_FILES[name] = modpath
            i += 1
        i += 1

    text = Path(path).read_text(errors="replace")
    mods = load_moddir(text)
    print("=== Moduldirectory (aus Live-Dump) ===")
    for name, base, size in mods:
        print(f"  {name:10s} base=0x{base:08x} size=0x{size:x} end=0x{base+size:x}")
    mm = ModuleMap(mods)

    lines = []
    in_trace = False
    for line in text.splitlines():
        if line.startswith("--- Instruktionsspur"):
            in_trace = True
            continue
        if line.startswith("--- Ende Instruktionsspur"):
            break
        if in_trace and line.strip().startswith("pc="):
            lines.append(line.strip())

    print(f"\n=== {len(lines)} Trace-Zeilen geladen ===\n")

    entries = []
    for l in lines:
        d = dict(re.findall(r'(\w+)=([0-9a-fA-F]+)', l))
        entries.append({k: int(v, 16) for k, v in d.items()})

    if tail:
        entries = entries[-tail:]

    # --- Automatische Call/Return-Bilanz ---
    # WICHTIG: dieser Kernel benutzt an mehreren Stellen bewusst
    # "move.l <ziel>,-(sp) / rts" als getarnten Sprung (kein freies Register
    # fuer jsr/jmp uebrig, s. Q9K_TrapExtInvoke/Q9K_TrapCallForeignCaller/
    # Q9K_TCallDispatch-Kopfkommentare). SP sinkt dabei um 4 und die
    # unmittelbar folgende "rts" hebt ihn sofort wieder um 4 -- macht die
    # eigene call_stack-Bilanz NICHT kaputt, muss also NICHT als Call
    # gezaehlt werden. Erkennung: SP unmittelbar VOR der "rts" ist GENAU 4
    # weniger als SP zwei Zeilen davor (die vorletzte Instruktion war ein
    # Push).
    call_stack = []  # erwartete Ruecksprungadressen
    anomalies = []
    annotated = []
    for i, e in enumerate(entries):
        pc = e["pc"]
        sp = e.get("sp")
        insn = mm.disasm_one(pc)
        mnem = insn.mnemonic if insn else "?"
        ops = insn.op_str if insn else ""
        label = mm.annotate(pc)
        annotated.append((i, e, label, mnem, ops))

        if mnem in CALL_MNEMONICS and insn:
            ret_addr = pc + insn.size
            call_stack.append((ret_addr, sp, i))
        elif mnem in RET_MNEMONICS:
            is_disguised_jump = False
            if i > 0:
                prev = annotated[i - 1]
                prev_sp = entries[i - 1].get("sp")
                if prev[3] == "move.l" and "-(a7)" in prev[4] and prev_sp is not None and sp is not None:
                    if prev_sp - sp == 4:
                        is_disguised_jump = True
            if is_disguised_jump:
                pass  # SP-neutraler getarnter Sprung, keine echte Rueckkehr
            elif not call_stack:
                anomalies.append((i, "RETURN OHNE OFFENEN CALL", pc, None))
            else:
                expected_ret, call_sp, call_i = call_stack.pop()
                # naechste Zeile sollte pc == expected_ret zeigen
                if i + 1 < len(entries):
                    actual_next = entries[i + 1]["pc"]
                    if actual_next != expected_ret:
                        anomalies.append((i, "RUECKSPRUNG-MISMATCH", actual_next, expected_ret))

    print("=== Offene (nie zurueckgekehrte) Calls am Ende ===")
    for ret_addr, call_sp, call_i in call_stack[-15:]:
        print(f"  Zeile {call_i}: erwartete Rueckkehr {mm.annotate(ret_addr)} (sp beim Call=0x{call_sp:08x})")

    print(f"\n=== Anomalien: {len(anomalies)} ===")
    for i, kind, actual, expected in anomalies[-20:]:
        exp_s = mm.annotate(expected) if expected is not None else "-"
        print(f"  Zeile {i}: {kind}  ist={mm.annotate(actual)}  erwartet={exp_s}")

    print("\n=== Letzte 40 annotierte Zeilen ===")
    for i, e, label, mnem, ops in annotated[-40:]:
        print(f"  [{i:6d}] {label:55s} {mnem:8s} {ops}")


if __name__ == "__main__":
    main()
