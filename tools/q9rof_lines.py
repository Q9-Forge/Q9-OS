#!/usr/bin/env python3
"""q9rof_lines.py -- extract source line records from an xcc -g ROF file.

The debug section emitted by the Microware 68k toolchain starts with
``00 00 01 06`` and contains mostly aligned, but in places byte-packed,
records.  The records needed here are:

    00 01 <code offset:u32> <line<<12 | column:u32>
    00 02 <value:u32> <stabs name, NUL terminated>

The tool deliberately does not try to decode every ROF header field.  The
debug offset can be supplied explicitly (recommended for a linked/stripped
artifact), or found by a conservative scan for the debug-section signature.

Examples:

    q9rof_lines.py q9kernel_date.r --debug-offset 0x8b8
    q9rof_lines.py q9kernel_date.r --base 0x7100 --format table
    q9rof_lines.py q9kernel_date.r --json

The output offset is the code/module-relative offset unless ``--base`` is
given; with a base it also includes the runtime address.  A source record
(``file.c:SC:...``) selects the file for following line records.
"""

from __future__ import annotations

import argparse
import json
import re
import struct
import sys
from dataclasses import asdict, dataclass
from pathlib import Path


DEBUG_MAGIC = b"\x00\x00\x01\x06"


@dataclass(frozen=True)
class Line:
    offset: int
    line: int
    column: int
    source: str
    runtime: int | None = None
    symbol: str | None = None


def u32(data: bytes, pos: int) -> int:
    if pos + 4 > len(data):
        raise ValueError(f"truncated 32-bit value at 0x{pos:x}")
    return struct.unpack_from(">I", data, pos)[0]


def source_name(stabs_name: str) -> str | None:
    # The compiler emits e.g. ``q9kernel_date.c:SC:<timestamp>``.
    if ":SC:" in stabs_name:
        return stabs_name.split(":SC:", 1)[0]
    return None


def plausible_section(data: bytes, start: int) -> bool:
    """Return true for a magic candidate followed by at least one valid record."""
    pos = start + 4
    if pos + 2 > len(data):
        return False
    kind = struct.unpack_from(">H", data, pos)[0]
    return kind in (1, 2, 3, 4)


def find_debug_offset(data: bytes) -> int:
    candidates = []
    pos = data.find(DEBUG_MAGIC)
    while pos >= 0:
        if plausible_section(data, pos):
            candidates.append(pos)
        pos = data.find(DEBUG_MAGIC, pos + 1)
    if not candidates:
        raise ValueError("kein ROF-Debug-Abschnitt (00 00 01 06) gefunden")
    if len(candidates) > 1:
        raise ValueError(
            "mehrere plausible Debug-Abschnitte gefunden; "
            "bitte --debug-offset angeben: "
            + ", ".join(f"0x{x:x}" for x in candidates)
        )
    return candidates[0]


def parse(data: bytes, debug_offset: int, base: int | None = None,
          module_offset: int = 0) -> list[Line]:
    if data[debug_offset:debug_offset + 4] != DEBUG_MAGIC:
        raise ValueError(f"bei 0x{debug_offset:x} fehlt die Debug-Signatur 00 00 01 06")

    pos = debug_offset + 4
    current_source = "?"
    result: list[Line] = []
    while pos + 2 <= len(data):
        # Some symbol records are padded to an even boundary, while others
        # are byte-packed.  Recognize the optional single zero only when it
        # is unambiguously followed by a known record type.
        if (pos + 2 < len(data) and data[pos] == 0 and data[pos + 1] == 0
                and data[pos + 2] in (1, 2, 3, 4)):
            pos += 1
        kind = struct.unpack_from(">H", data, pos)[0]
        pos += 2
        if kind == 1:
            if pos + 8 > len(data):
                raise ValueError(f"abgeschnittener Zeilensatz bei 0x{pos - 2:x}")
            offset = u32(data, pos)
            line_col = u32(data, pos + 4)
            pos += 8
            line = line_col >> 12
            column = line_col & 0xFFF
            if line:
                result.append(Line(offset, line, column, current_source,
                                   None if base is None else base + module_offset + offset))
        elif kind == 2:
            # Parameter and local-variable stabs carry one extra 02 byte
            # before the value (e.g. ``02 ffffffc`` in the ROF stream).
            # Global/type symbols start with the value immediately.
            if pos < len(data) and data[pos] == 0x02:
                pos += 1
            if pos + 4 > len(data):
                raise ValueError(f"abgeschnittener Symbolsatz bei 0x{pos - 2:x}")
            pos += 4  # value; symbols are not needed for the line table
            end = data.find(b"\0", pos)
            if end < 0:
                raise ValueError(f"nicht terminierter Symbolname bei 0x{pos:x}")
            name = data[pos:end].decode("latin-1", errors="replace")
            current_source = source_name(name) or current_source
            # The toolchain's string terminator can either be followed by
            # the next record or serve as its zero high byte.  Accept both
            # forms (the latter occurs for local-variable stabs).
            pos = end if data[end:end + 2] == b"\0\2" else end + 1
        elif kind in (3, 4):
            if pos + 4 > len(data):
                raise ValueError(f"abgeschnittener Blocksatz bei 0x{pos - 2:x}")
            pos += 4
        else:
            # A zero-filled tail is normal in a fixed-size ROF section.
            if data[pos - 2:] == b"\0" * (len(data) - (pos - 2)):
                break
            raise ValueError(f"unbekannter Debug-Satztyp {kind} bei 0x{pos - 2:x}")
    return result


def scan_line_records(data: bytes, debug_offset: int, base: int | None = None,
                      module_offset: int = 0) -> list[Line]:
    """Fallback for ROFs whose symbol records use mixed padding conventions."""
    names = re.findall(rb"([ -~]+):SC:[^\0]*\0", data[debug_offset:])
    source = names[0].decode("latin-1", errors="replace") if names else "?"
    result: list[Line] = []
    for pos in range(debug_offset + 4, len(data) - 10):
        if data[pos:pos + 2] != b"\0\1":
            continue
        offset = u32(data, pos + 2)
        line_col = u32(data, pos + 6)
        line = line_col >> 12
        column = line_col & 0xFFF
        if line == 0 or line > 100000 or offset >= len(data):
            continue
        result.append(Line(offset, line, column, source,
                           None if base is None else base + module_offset + offset))
    return result


def load_symbols(path: Path) -> list[tuple[int, str]]:
    text = path.read_text(errors="replace")
    symbols = [(int(offset, 16), name)
               for name, kind, offset in re.findall(
                   r"(\S+)\s+(COD|DAT)\s+([0-9a-fA-F]{8})", text)
               if kind == "COD"]
    symbols.sort()
    return symbols


def annotate_symbols(lines: list[Line], symbols: list[tuple[int, str]],
                     module_offset: int = 0) -> list[Line]:
    for index, line in enumerate(lines):
        if line.runtime is None:
            continue
        offset = module_offset + line.offset
        best = None
        for symbol_offset, name in symbols:
            if symbol_offset > offset:
                break
            best = (symbol_offset, name)
        if best is not None:
            symbol_offset, name = best
            lines[index] = Line(line.offset, line.line, line.column, line.source,
                                 line.runtime, f"{name}+0x{offset - symbol_offset:x}")
    return lines


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rof", type=Path, help="ROF-Objektdatei")
    ap.add_argument("--debug-offset", type=lambda x: int(x, 0),
                    help="Dateioffset des Debug-Abschnitts, z.B. 0x8b8")
    ap.add_argument("--base", type=lambda x: int(x, 0),
                    help="Ladeadresse; ergänzt runtime zu jeder Zeile")
    ap.add_argument("--module-offset", type=lambda x: int(x, 0), default=0,
                    help="P-Segment-Offset des ROF im gelinkten Modul")
    ap.add_argument("--symbols", type=Path,
                    help="l68 -s Linkmap fuer die Symbolspalte")
    ap.add_argument("--format", choices=("table", "flux", "json"), default="table")
    ns = ap.parse_args(argv)
    data = ns.rof.read_bytes()
    try:
        debug_offset = ns.debug_offset if ns.debug_offset is not None else find_debug_offset(data)
        try:
            lines = parse(data, debug_offset, ns.base, ns.module_offset)
        except ValueError as exc:
            lines = scan_line_records(data, debug_offset, ns.base, ns.module_offset)
            if not lines:
                raise exc
            print(f"q9rof_lines.py: Hinweis: strukturierter Parser stoppte ({exc}); "
                  "Typ-1-Fallback verwendet", file=sys.stderr)
        if ns.symbols:
            lines = annotate_symbols(lines, load_symbols(ns.symbols), ns.module_offset)
    except ValueError as exc:
        print(f"q9rof_lines.py: Fehler: {exc}", file=sys.stderr)
        return 2

    if ns.format == "json":
        print(json.dumps({"debug_offset": debug_offset,
                          "lines": [asdict(line) for line in lines]},
                         ensure_ascii=False, indent=2))
        return 0

    if ns.format == "flux":
        if ns.base is None:
            print("q9rof_lines.py: Fehler: --format flux braucht --base",
                  file=sys.stderr)
            return 2
        for line in lines:
            print(f"0x{line.runtime:08x} {line.source}:{line.line}:{line.column}")
        return 0

    print(f"debug=0x{debug_offset:x} lines={len(lines)}")
    for line in lines:
        address = "" if line.runtime is None else f" runtime=0x{line.runtime:08x}"
        symbol = "" if line.symbol is None else f" symbol={line.symbol}"
        print(f"0x{line.offset:08x} {line.source}:{line.line}:{line.column}"
              f"{address}{symbol}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
