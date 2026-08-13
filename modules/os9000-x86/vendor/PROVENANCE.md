# Provenienz — `vendor/` (OS-9000/x86 v4.9, statischer Eval-CD-Build)

Diese Dateien sind **wortwörtlich** aus dem Original-Archiv entpackt, keine
Live-Extraktion (dafür siehe [`../vendor-live/PROVENANCE.md`](../vendor-live/PROVENANCE.md)).

## Quelldatei

| Feld | Wert |
|---|---|
| Pfad | `/Volumes/SSD1TB/#INFO/#Microware/OS9000/OS9Eval/RESIDENT/mw86.tar` |
| SHA-256 | `10fd71987d32f2ff271a9690520d015ed57d970dafebfd5da25a2c53f4fa7879` |
| Herkunft | Microware OS-9000-Evaluierungs-CD, ca. 1998/1999 (siehe `HOWTO.TXT` im selben Ordner) |
| Tar-internes Verzeichnis | `CMDS/BOOTOBJS/` |

## Extrahierte Dateien (SHA-256, unverändert seit Entpacken)

| Datei | SHA-256 |
|---|---|
| `cache386` | `fc0f092b41f4d718db1f6567ca24ffc481e1cd6cda5267591bd58103467b0e28` |
| `cdfm` | `91854c19755bcee92916d5964f6f13a95b1fb0e7fb2a831966aa1448fb496c4d` |
| `fpu` | `b39d29dcc44f62a4827457fd0a33a9b2272e44ab9e20ff8cbf07cb9e0d44b6dd` |
| `fpuem` | `ba3414a3b477c3c7a8cc048c50b42904859d1ef8e49e0987444f82b75b66a278` |
| `ioman` | `6d042acdd26a92944a0dd81f9bc48e4abffbfb0dc97f9255b026f2a7d3b2cee3` |
| `kernel` | `23bd1b2fe1f1d126d4ab5de2555744f4356f556c9d2c7837a3bdd7a5d38c6cdf` |
| `nil` | `27864850804cbca9fdbfd05eddbd03e177ca119bb1eb83943eea495d949930fb` |
| `null` | `0c41ff6c81bd2f9aa24216f481eb37468b5ade8a8586a8a5561b84700d00ff57` |
| `pcf` | `22455d6ca9b2c9c707853adf7fb3eb60ec8b37afb7091c1561bcc581ec9acdec` |
| `pipeman` | `db7c6d05901264240e12404946f0e73d00f046efff818487b1c7e1a3b3c91efa` |
| `rbf` | `aaaa8c1748301e491c339066826627e9a8b26d1264386bc3b35ce8b9317daced` |
| `sbf` | `bdf1c83e7038087060cbbb7c53718a685b080439a64dd907d096e73387a355de` |
| `scf` | `606973eb47dfd95e746a9a976f41f4b060a7a4451c36dba96dfde0fd45c53b57` |
| `scllio` | `908d9f5414128cb801b94c01c51003231c2e4b2d510c643ac796d779ca593902` |
| `ssm` | `450783d386a7a282e7ea1480a322a1c9c5c24e3bf0ec3b63a834ef5f5e5a3c0b` |
| `vectx86` | `362a0dfd53c67972e8c0d1b911720884c165506a6ae73b19f86ca47061f3b180` |

Nachvollziehen: `tar -xf mw86.tar CMDS/BOOTOBJS/<name>` und `shasum -a 256`
gegen die obige Tabelle prüfen.

**Wichtig:** Dies ist ein anderer (älterer) Build als der tatsächlich
auf `os9000-xibase.img` laufende — siehe
[`../vendor-live/`](../vendor-live/README.md) und
[`../docs/FINDINGS.md`](../docs/FINDINGS.md) Fund 4/6 für den Größenvergleich.
