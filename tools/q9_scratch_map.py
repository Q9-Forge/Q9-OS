#!/usr/bin/env python3
"""q9_scratch_map.py -- Belegungstabelle der absoluten Kernelzellen (B3).

Der Kernel haelt viele Zwischenwerte in festen Speicherzellen ($1000-$3FFF und
einem Diagnosebereich oberhalb von $150000). Jede Zelle ist an ZWEI Stellen
definiert: als `equ` in q9kernel_entry.a und (wo C sie braucht) als `#define`
in einer q9kernel_*.c. Zwei Zellen auf derselben Adresse sind nur dann
harmlos, wenn sie zu verschiedenen Zeiten leben -- das hat in der Vergangenheit
schon einmal einen echten Fehler verursacht ($3F0/$3F8, Fortsetzung 122).

Dieses Werkzeug liest beide Quellen, fuehrt die Namen zusammen (die
asm-Schreibweise Q9K_FModulScratch_TyLang und die C-Schreibweise
Q9K_FMODUL_SCRATCH_TYLANG sind dieselbe Zelle) und prueft:

  1. keine Adresse hat zwei verschiedene Zellen,
  2. keine Zelle ragt in die naechste (Groesse aus dem Kommentar, sonst 4 Byte),
  3. jede gewollte gemeinsame Nutzung steht mit Begruendung in
     tools/q9_scratch_shared.txt.

Aufruf:
    tools/q9_scratch_map.py --check [--doc <datei>]
                                               Pruefung (Exit 1 bei Verstoss); mit --doc
                                               zusaetzlich: Tabelle muss aktuell sein
    tools/q9_scratch_map.py --write <datei>    Tabelle als Markdown schreiben
    tools/q9_scratch_map.py --list             Tabelle auf stdout
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
KDIR = os.path.join(ROOT, "Q9-KERNEL", "68k", "src", "kernel")
SHARED = os.path.join(HERE, "q9_scratch_shared.txt")
TABLES = os.path.join(HERE, "q9_scratch_tables.txt")

# Adressfenster, in denen absolute Kernelzellen liegen.
WINDOWS = [(0x1000, 0x4000, "Scratch"), (0x150000, 0x160000, "Diagnose")]
# Namen, die zwar im Fenster liegen, aber Offsets/Konstanten sind, keine Zellen.
NOT_CELL = re.compile(r"(_OFF|_OFFSET|_SIZE|_BYTES|_MAX|_MASK|_COUNT_MAX|_DEPTH)$|^Q9K_E_|^Q9_D_")


def norm(name):
    """Schreibweisen asm/C zusammenfuehren: Kleinbuchstaben, ohne Unterstriche."""
    return re.sub(r"[_]", "", name).lower()


def window_of(addr):
    for lo, hi, w in WINDOWS:
        if lo <= addr < hi:
            return w
    return None


def size_from_comment(comment, addr):
    if not comment:
        return 4
    # Bereich "$1F94-$1FDF" bzw. "($1F94-$1FDF)" -> genaue Groesse
    m = re.search(r"\$([0-9A-Fa-f]{3,6})\s*-\s*\$([0-9A-Fa-f]{3,6})", comment)
    if m and int(m.group(1), 16) == addr and int(m.group(2), 16) >= addr:
        return int(m.group(2), 16) - addr + 1
    m = re.search(r"bis\s+(?:0x|\$)([0-9A-Fa-f]+)", comment)
    if m:
        end = int(m.group(1), 16)
        if end >= addr:
            return end - addr + 1
    # "19 x Q9_u32", "8 x 4 Byte"
    m = re.search(r"(\d+)\s*x\s*(?:Q9_u32|4 Byte)", comment)
    if m:
        return int(m.group(1)) * 4
    m = re.search(r"(\d+)\s*[- ]?Byte", comment)
    if m:
        return int(m.group(1))
    if re.search(r"Q9_u8\b|\.b\b", comment):
        return 1
    if re.search(r"Q9_u16\b|\.w\b", comment):
        return 2
    return 4


def read_asm():
    cells = []
    rx = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)[ \t]+equ[ \t]+\$([0-9A-Fa-f]{1,8})\b[ \t]*(\*.*)?$")
    path = os.path.join(KDIR, "q9kernel_entry.a")
    with open(path, encoding="latin-1") as f:
        for ln, line in enumerate(f, 1):
            m = rx.match(line.rstrip("\n"))
            if not m:
                continue
            name, addr, comment = m.group(1), int(m.group(2), 16), (m.group(3) or "")
            if window_of(addr) is None or NOT_CELL.search(name):
                continue
            cells.append(dict(name=name, addr=addr, size=size_from_comment(comment, addr),
                              src="q9kernel_entry.a", line=ln, note=comment.strip("* ").strip()))
    return cells


def read_c():
    cells = []
    rx = re.compile(r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s+\(?(?:\(\w+\))?\s*(0x[0-9A-Fa-f]+)[UL]*\)?\s*(?:/\*(.*?)\*/)?\s*$")
    for fn in sorted(os.listdir(KDIR)):
        if not fn.endswith((".c", ".h")) or fn.startswith("test_"):
            continue
        with open(os.path.join(KDIR, fn), encoding="latin-1") as f:
            for ln, line in enumerate(f, 1):
                m = rx.match(line.rstrip("\n"))
                if not m:
                    continue
                name, addr, comment = m.group(1), int(m.group(2), 16), (m.group(3) or "")
                if window_of(addr) is None or NOT_CELL.search(name):
                    continue
                cells.append(dict(name=name, addr=addr, size=size_from_comment(comment, addr),
                                  src=fn, line=ln, note=comment.strip()))
    return cells


def read_shared():
    """tools/q9_scratch_shared.txt: '$ADRESSE name1 name2 ... # Begruendung'."""
    shared = {}
    if os.path.exists(SHARED):
        for line in open(SHARED, encoding="utf-8"):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            body, _, why = line.partition("#")
            parts = body.split()
            addr = int(parts[0].lstrip("$"), 16) if parts else None
            shared[addr] = (set(norm(n) for n in parts[1:]), why.strip())
    return shared


def read_tables():
    """tools/q9_scratch_tables.txt: 'name $ADRESSE bytes  # Quelle'.
    Grosse Tabellen, deren Ausdehnung nur aus SLOTS*STRIDE im C-Code folgt."""
    out = []
    if os.path.exists(TABLES):
        for line in open(TABLES, encoding="utf-8"):
            body, _, why = line.partition("#")
            parts = body.split()
            if len(parts) >= 3:
                out.append(dict(name="Tabelle:" + parts[0], addr=int(parts[1].lstrip("$"), 16),
                                size=int(parts[2], 0), srcs=["q9_scratch_tables.txt"], note=why.strip(), asm=False, table=True))
    return out


def merge(asm, c, tables):
    """Identitaet = die asm-Zelle. C-Defines auf derselben Adresse sind Aliase
    (gleiche Zelle, andere Schreibweise); C-Defines ohne asm-Zelle sind eigene
    Zellen. Tabellen aus der Konfiguration ueberschreiben die Groesse einer
    Zelle an ihrer Basisadresse nicht, sondern kommen als eigener Bereich hinzu
    (sie decken die Zellen INNERHALB der Tabelle als Verstoss auf)."""
    cells = {}
    for a in asm:
        key = (a["addr"], norm(a["name"]))
        if key in cells:
            cells[key]["srcs"].append("%s:%d" % (a["src"], a["line"]))
            continue
        cells[key] = dict(name=a["name"], addr=a["addr"], size=a["size"], note=a["note"], asm=True,
                          srcs=["%s:%d" % (a["src"], a["line"])], aliases=[])
    asm_addrs = {}
    for e in cells.values():
        asm_addrs.setdefault(e["addr"], []).append(e)
    for cd in c:
        hit = asm_addrs.get(cd["addr"])
        if hit:
            hit[0]["aliases"].append("%s (%s:%d)" % (cd["name"], cd["src"], cd["line"]))
            hit[0]["size"] = max(hit[0]["size"], cd["size"])
            continue
        key = (cd["addr"], norm(cd["name"]))
        if key in cells:
            continue
        # C-Defines auf derselben Adresse mit verschiedenen Namen: eine Zelle.
        same = [e for e in cells.values() if e["addr"] == cd["addr"] and not e["asm"]]
        if same:
            same[0]["aliases"].append("%s (%s:%d)" % (cd["name"], cd["src"], cd["line"]))
            continue
        cells[key] = dict(name=cd["name"], addr=cd["addr"], size=cd["size"], note=cd["note"], asm=False,
                          srcs=["%s:%d" % (cd["src"], cd["line"])], aliases=[])
    # Eine Zelle genau an der Basisadresse einer Tabelle IST die Tabelle (z. B.
    # Q9K_EVENT_BASE): sie geht in der Tabelle auf, ihr Name wird Alias.
    base = {t["addr"]: t for t in tables}
    kept = []
    for e in cells.values():
        t = base.get(e["addr"])
        if t is not None and e["size"] <= 4 + 256 and "BASE" in e["name"].upper() or (t is not None and e["name"].lower().endswith(("base", "tabbase"))):
            t.setdefault("aliases", []).append("%s (%s)" % (e["name"], e["srcs"][0]))
            continue
        kept.append(e)
    out = sorted(kept + tables, key=lambda e: (e["addr"], e["name"]))
    return out


def check(cells, shared):
    problems = []
    by_addr = {}
    for e in cells:
        if e.get("table"):
            continue
        by_addr.setdefault(e["addr"], []).append(e)
    for addr, group in sorted(by_addr.items()):
        asm_group = [e for e in group if e["asm"]]
        if len(asm_group) > 1:
            names = set(norm(e["name"]) for e in asm_group)
            allowed = shared.get(addr)
            if allowed and names <= allowed[0]:
                continue
            problems.append("Adresse $%X doppelt belegt: %s" %
                            (addr, "; ".join("%s (%s)" % (e["name"], e["srcs"][0]) for e in asm_group)))
    # Ueberlappungen (inkl. Tabellen)
    for i, a in enumerate(cells):
        end = a["addr"] + a["size"]
        for b in cells[i + 1:]:
            if b["addr"] >= end:
                break
            if b["addr"] == a["addr"] and not (a.get("table") or b.get("table")):
                continue   # gleiche Startadresse: schon oben als Doppelname behandelt
            allowed = shared.get(a["addr"]) or shared.get(b["addr"])
            if allowed and {norm(a["name"]), norm(b["name"])} <= allowed[0]:
                continue
            problems.append("%s ($%X, %d Byte, %s) ueberlappt %s ($%X, %s)" %
                            (a["name"], a["addr"], a["size"], a["srcs"][0], b["name"], b["addr"], b["srcs"][0]))
    return problems


def gaps(cells):
    out = []
    for lo, hi, w in WINDOWS:
        last = lo
        for e in cells:
            if not (lo <= e["addr"] < hi):
                continue
            if e["addr"] > last:
                out.append((e["addr"] - last, last, e["addr"] - 1, w))
            last = max(last, e["addr"] + e["size"])
        if hi > last:
            out.append((hi - last, last, hi - 1, w))
    return out


def render(cells, shared):
    # Zeilennummern stehen bewusst NICHT in der Datei: jede Verschiebung im Quelltext wuerde die Tabelle
    # sonst "veraltet" machen und den Bau stoppen (nur Namen, Adressen, Groessen und Bemerkungen zaehlen).
    lines = ["# Kernel-Speicherbelegung (absolute Zellen)", "",
             "Diese Datei wird von `tools/q9_scratch_map.py --write` erzeugt und im Build",
             "gegen die Quellen geprueft (`--check`). Nicht von Hand aendern.", "",
             "**Regeln:** Jede Zelle gehoert genau einer Funktion. Zwei Namen auf derselben",
             "Adresse sind nur zulaessig, wenn sie in `tools/q9_scratch_shared.txt` mit",
             "Begruendung stehen (sie leben dann nie gleichzeitig). Wer eine Zelle",
             "hinzufuegt, erzeugt diese Tabelle neu und prueft die Luecken unten.", "",
             "| Adresse | Groesse | Name | Quelle | Bemerkung |", "|---|---:|---|---|---|"]
    for e in cells:
        shared_mark = " (gemeinsam, s. shared)" if e["addr"] in shared else ""
        al = ("; C: " + ", ".join(x.split(" (")[0] for x in e.get("aliases", [])[:3])) if e.get("aliases") else ""
        lines.append("| `$%06X` | %d | `%s`%s | %s%s | %s |" % (
            e["addr"], e["size"], e["name"], shared_mark, re.sub(r":\d+", "", ", ".join(e["srcs"][:1])), al, e["note"].replace("|", "/")[:70]))
    lines += ["", "## Freie Luecken (>= 16 Byte)", "", "| Bereich | Byte | Fenster |", "|---|---:|---|"]
    for size, a, b, w in sorted(gaps(cells), reverse=True):
        if size >= 16:
            lines.append("| `$%06X`-`$%06X` | %d | %s |" % (a, b, size, w))
    return "\n".join(lines) + "\n"


def main(argv):
    shared = read_shared()
    cells = merge(read_asm(), read_c(), read_tables())
    if "--list" in argv:
        sys.stdout.write(render(cells, shared))
        return 0
    if "--write" in argv:
        path = argv[argv.index("--write") + 1]
        with open(path, "w", encoding="utf-8") as f:
            f.write(render(cells, shared))
        print("geschrieben: %s (%d Zellen)" % (path, len(cells)))
        return 0
    problems = check(cells, shared)
    if "--doc" in argv:
        doc = argv[argv.index("--doc") + 1]
        want = render(cells, shared)
        have = open(doc, encoding="utf-8").read() if os.path.exists(doc) else ""
        if have != want:
            problems.append("Die Tabelle %s ist veraltet oder fehlt -- neu erzeugen mit: "
                            "tools/q9_scratch_map.py --write %s" % (doc, doc))
    for p in problems:
        print("FAIL  " + p)
    if problems:
        print("Belegungspruefung: %d Verstoesse bei %d Zellen" % (len(problems), len(cells)))
        return 1
    print("ok    Belegungspruefung: %d Zellen, keine Doppelbelegung, keine Ueberlappung" % len(cells))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
