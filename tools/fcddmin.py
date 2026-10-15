#!/usr/bin/env python3
"""fcddmin.py -- Delta-Debugging ueber die FARCALL-Stellen (Arbeitsplan A2).

Sucht eine minimale Menge von bsr-Stellen, die als Q9K_FARCALL den Login-Test brechen.
Orakel: tools/fcbisect.sh <Stellen> (Exit 0 = gruen). Ergebnisse werden zwischengespeichert,
jede Frage steht mit Dauer im Log. Aufruf: fcddmin.py [--variant V] [--max 43]
"""
import subprocess, sys, time, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
variant = "development"; n = 43
a = sys.argv[1:]
while a:
    if a[0] == "--variant": variant = a[1]; a = a[2:]
    elif a[0] == "--max": n = int(a[1]); a = a[2:]
    else: sys.exit("unbekannt: " + a[0])

cache = {frozenset(): False}   # "none" ist gemessen gruen (fcbisect none, 15.10.2026)
def spec(s):
    s = sorted(s); out = []; i = 0
    while i < len(s):
        j = i
        while j + 1 < len(s) and s[j + 1] == s[j] + 1: j += 1
        out.append(str(s[i]) if i == j else "%d-%d" % (s[i], s[j])); i = j + 1
    return ",".join(out) if out else "none"

def fails(s):
    key = frozenset(s)
    if key in cache: return cache[key]
    t0 = time.time()
    r = subprocess.run(["bash", os.path.join(HERE, "fcbisect.sh"), "--variant", variant, spec(s)],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    bad = r.returncode != 0
    why = ""
    if bad:
        m = re.search(r"(FEHLER|FAIL)[^\n]*", r.stdout); why = m.group(0)[:90] if m else ""
    print("%-40s -> %s  (%d s) %s" % (spec(s)[:40], "ROT" if bad else "gruen", time.time() - t0, why), flush=True)
    cache[key] = bad
    return bad

def ddmin(items):
    k = 2
    while len(items) >= 2:
        size = len(items) // k or 1
        chunks = [items[i:i + size] for i in range(0, len(items), size)]
        reduced = False
        for c in chunks:                       # eine Teilmenge reicht schon
            if len(c) < len(items) and fails(c):
                items, k, reduced = c, 2, True; break
        if not reduced:
            for c in chunks:                   # Komplement reicht schon
                comp = [x for x in items if x not in c]
                if comp and len(comp) < len(items) and fails(comp):
                    items, k, reduced = comp, max(k - 1, 2), True; break
        if not reduced:
            if k >= len(items): break
            k = min(len(items), k * 2)
    return items

print("Vorpruefung: alle %d Stellen" % n, flush=True)
allsites = list(range(1, n + 1))
if not fails(allsites): sys.exit("alle Stellen gruen -- nichts zu suchen")
res = ddmin(allsites)
print("\nMINIMALE ROTE MENGE: %s" % spec(res), flush=True)
# 1-minimal bestaetigen: jede Stelle einzeln entfernt muss gruen werden (steckt schon im ddmin; Kontrolle)
for x in res:
    rest = [y for y in res if y != x]
    print("  ohne %2d: %s" % (x, "ROT (nicht minimal)" if rest and fails(rest) else "gruen"), flush=True)
