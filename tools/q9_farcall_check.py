#!/usr/bin/env python3
"""q9_farcall_check.py -- Arbeitsplan A3: kein `bsr` auf eine C-Funktion mehr in q9kernel_entry.a.

bsr reicht nur +-32 KB; der Kernel sitzt an dieser Grenze. Jeder Aufruf einer Funktion, die nicht in
q9kernel_entry.a selbst definiert ist (also aus einer C-Datei kommt), muss Q9K_FARCALL benutzen.
Exit 1 mit Liste der Verstoesse. Aufruf: tools/q9_farcall_check.py [q9kernel_entry.a]
"""
import re, sys, os
here = os.path.dirname(os.path.abspath(__file__))
path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "Q9-KERNEL", "68k", "src", "kernel", "q9kernel_entry.a")
L = open(path, encoding="latin-1").read().split("\n")
labels = set()
for l in L:
    m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):", l) or re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s+(equ|macro)\b", l)
    if m: labels.add(m.group(1))
bad = [(i + 1, m.group(1)) for i, l in enumerate(L)
       for m in [re.match(r"^\s+bsr\s+([A-Za-z_][A-Za-z0-9_]*)", l)] if m and m.group(1) not in labels]
macro = sum(1 for l in L if l.startswith("Q9K_FARCALL macro"))
for ln, t in bad:
    print("FAIL  %s:%d  bsr %s -- Funktion ausserhalb der Datei, Q9K_FARCALL verwenden" % (os.path.basename(path), ln, t))
if not macro:
    print("FAIL  Makro Q9K_FARCALL fehlt in %s" % os.path.basename(path)); bad.append((0, "macro"))
if bad: sys.exit(1)
print("ok    Fernaufrufe: kein bsr auf Funktionen ausserhalb von %s (%d Q9K_FARCALL-Stellen)"
      % (os.path.basename(path), sum(1 for l in L if re.match(r"^\s+Q9K_FARCALL\s", l))))
