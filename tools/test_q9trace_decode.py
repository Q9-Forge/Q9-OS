#!/usr/bin/env python3
"""Unit-Tests fuer tools/q9trace_decode.py (synthetische Trace-Saetze).

Aufruf: python3 -m unittest tools/test_q9trace_decode.py
"""
import contextlib
import io
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import q9trace_decode as d  # noqa: E402


def rec(rtype, code, pid=1, tick=0, fine=0, depth=0, carry=False, trunc=False,
        payload=b''):
    """Baut einen Satz exakt nach q9trace.h (12-Byte-Kopf, big-endian)."""
    flags = (depth & 0x0F) | (0x10 if carry else 0) | (0x20 if trunc else 0)
    n = d.HDR_LEN + len(payload)
    return (bytes([rtype, n, code, flags]) + pid.to_bytes(2, 'big')
            + tick.to_bytes(4, 'big') + fine.to_bytes(2, 'big') + payload)


def text_dump(stream, size=None, rd=0, used=None, chunk=16):
    """Erzeugt einen Q9-Flux-Textdump; der Puffer wird bei rd 'rotiert'."""
    size = size or len(stream)
    used = len(stream) if used is None else used
    buf = bytearray(size)
    for i, b in enumerate(stream):
        buf[(rd + i) % size] = b
    lines = ['--- Q9-Trace-Puffer addr=00100000 size=%x wr=%x rd=%x used=%x written=%x '
             'lost=0 enabled=1 ---' % (size, (rd + used) % size, rd, used, used)]
    for off in range(0, size, chunk):
        lines.append('T %x %s' % (off, bytes(buf[off:off + chunk]).hex()))
    return '\n'.join(lines) + '\n'


class Tmp(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)

    def write(self, name, content):
        path = os.path.join(self.dir.name, name)
        with open(path, 'wb' if isinstance(content, bytes) else 'w') as f:
            f.write(content)
        return path

    def run_main(self, *argv):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            rc = d.main(list(argv))
        return rc, out.getvalue()


class TestParse(unittest.TestCase):
    def test_header_fields(self):
        r = d.parse(rec(2, 0x84, pid=0x0107, tick=0x01020304, fine=0xBEEF,
                        depth=5, carry=True, trunc=True))
        self.assertEqual((r['type'], r['code'], r['pid']), (2, 0x84, 0x0107))
        self.assertEqual((r['tick'], r['fine']), (0x01020304, 0xBEEF))
        self.assertEqual((r['depth'], r['carry'], r['trunc']), (5, True, True))

    def test_depth_mask_is_four_bits(self):
        self.assertEqual(d.parse(rec(1, 0, depth=15))['depth'], 15)
        self.assertFalse(d.parse(rec(1, 0, depth=15))['carry'])

    def test_payload(self):
        r = d.parse(rec(5, 0, payload=b'\x00\x00\x00\x2a'))
        self.assertEqual(r['payload'], b'\x00\x00\x00\x2a')

    def test_call_names(self):
        self.assertEqual(d.call_name(d.parse(rec(1, 0x84))), 'I$Open')
        self.assertEqual(d.call_name(d.parse(rec(1, 0xEE))), '$EE')
        self.assertEqual(d.call_name(d.parse(rec(3, 0x07))), 'FN$07')


class TestRecords(unittest.TestCase):
    def test_empty_stream(self):
        self.assertEqual(list(d.records(b'')), [])

    def test_two_records(self):
        s = rec(1, 0x03) + rec(2, 0x03, payload=b'ab')
        got = list(d.records(s))
        self.assertEqual([g[0] for g in got], ['ok', 'ok'])
        self.assertEqual(got[1][1], 12)
        self.assertEqual(len(got[1][2]), 14)

    def test_bad_length_stops(self):
        bad = bytearray(rec(1, 0x03))
        bad[1] = 4
        got = list(d.records(rec(1, 1) + bytes(bad) + rec(1, 2)))
        self.assertEqual([g[0] for g in got], ['ok', 'kaputt'])

    def test_zero_length_does_not_loop(self):
        got = list(d.records(bytes(24)))
        self.assertEqual([g[0] for g in got], ['kaputt'])

    def test_truncated_tail_short_header(self):
        got = list(d.records(rec(1, 1) + b'\x01\x0c\x03'))
        self.assertEqual([g[0] for g in got], ['ok', 'abgeschnitten'])

    def test_truncated_tail_short_payload(self):
        got = list(d.records(rec(1, 1) + rec(1, 2, payload=b'abcd')[:-2]))
        self.assertEqual([g[0] for g in got], ['ok', 'abgeschnitten'])


def sample():
    return [d.parse(r) for r in (
        rec(1, 0x03, pid=7, tick=100, depth=0),                 # F$Fork
        rec(3, 0x05, pid=7, tick=101, depth=1),                 # intern
        rec(1, 0x01, pid=7, tick=102, depth=1),                 # F$Load
        rec(1, 0x84, pid=7, tick=103, depth=2),                 # I$Open
        rec(2, 0x84, pid=7, tick=104, depth=2, carry=True),     # I$Open Fehler
        rec(1, 0x89, pid=9, tick=105, depth=0),                 # I$Read
        rec(2, 0x89, pid=9, tick=106, depth=0),
        rec(5, 0, pid=0, tick=107, payload=(3).to_bytes(4, 'big')),
        rec(6, 0, pid=9, tick=108, payload=bytes([0, 5])),
    )]


class TestSelect(unittest.TestCase):
    def codes(self, recs):
        return [(r['type'], r['code']) for r in recs]

    def test_no_filter_keeps_all(self):
        self.assertEqual(len(d.select(sample())), 9)

    def test_pid(self):
        out = d.select(sample(), pid=9)
        self.assertEqual(self.codes(out), [(1, 0x89), (2, 0x89), (5, 0), (6, 0)])

    def test_pid_unknown_leaves_only_markers(self):
        self.assertEqual(self.codes(d.select(sample(), pid=99)), [(5, 0)])

    def test_call_by_name_case_insensitive(self):
        for name in ('I$Open', 'i$open', 'iopen'):
            out = d.select(sample(), call=name)
            self.assertEqual(self.codes(out), [(1, 0x84), (2, 0x84), (5, 0)], name)

    def test_call_by_number(self):
        for w in ('0x84', '132', '$84'):
            out = d.select(sample(), call=w)
            self.assertEqual(self.codes(out)[:2], [(1, 0x84), (2, 0x84)], w)

    def test_call_internal_name(self):
        self.assertEqual(self.codes(d.select(sample(), call='FN$05'))[:1], [(3, 0x05)])

    def test_call_unknown(self):
        self.assertEqual(self.codes(d.select(sample(), call='X$Nix')), [(5, 0)])

    def test_min_depth(self):
        out = d.select(sample(), min_depth=2)
        self.assertEqual(self.codes(out), [(1, 0x84), (2, 0x84), (5, 0)])

    def test_min_depth_zero_is_noop_on_records(self):
        self.assertEqual(len(d.select(sample(), min_depth=0)), 8)

    def test_last(self):
        out = d.select(sample(), last=3)
        self.assertEqual(self.codes(out), [(2, 0x89), (5, 0), (6, 0)])

    def test_last_larger_than_list(self):
        self.assertEqual(len(d.select(sample(), last=100)), 9)

    def test_last_zero(self):
        self.assertEqual(d.select(sample(), last=0), [])

    def test_last_applies_after_other_filters(self):
        out = d.select(sample(), pid=7, last=1)
        self.assertEqual(self.codes(out), [(5, 0)])
        out = d.select(sample(), pid=7, call='I$Open', last=2)
        self.assertEqual(self.codes(out), [(2, 0x84), (5, 0)])

    def test_combined(self):
        out = d.select(sample(), pid=7, call='I$Open', min_depth=2)
        self.assertEqual(self.codes(out), [(1, 0x84), (2, 0x84), (5, 0)])


class TestFormat(unittest.TestCase):
    def fmt(self, *a, **k):
        return d.format_record(d.parse(rec(*a, **k)))

    def test_indent_by_depth(self):
        for depth in (0, 1, 3):
            line = self.fmt(1, 0x84, pid=7, tick=5, depth=depth)
            self.assertTrue(line.endswith('pid  7  ' + '  ' * depth + '-> I$Open'), line)

    def test_timestamp_and_pid(self):
        line = self.fmt(1, 0x03, pid=12, tick=1234, fine=0x00AB)
        self.assertTrue(line.startswith('    1234.00ab  pid 12'))

    def test_return_carry_status(self):
        self.assertTrue(self.fmt(2, 0x84, carry=True).endswith('<- I$Open CARRY'))
        self.assertTrue(self.fmt(2, 0x84).endswith('<- I$Open ok'))

    def test_entry_has_no_status(self):
        self.assertTrue(self.fmt(1, 0x84, carry=True).endswith('-> I$Open'))

    def test_truncated_marker(self):
        self.assertIn('(gekuerzt)', self.fmt(1, 0x84, trunc=True))

    def test_lost(self):
        line = self.fmt(5, 0, tick=9, payload=(1234).to_bytes(4, 'big'))
        self.assertIn('!! 1234 Saetze verloren', line)

    def test_lost_short_payload_does_not_crash(self):
        self.assertIn('!! 0 Saetze', self.fmt(5, 0))

    def test_procinfo(self):
        self.assertIn('Benutzer 1.5', self.fmt(6, 0, pid=3, payload=bytes([1, 5])))

    def test_timebase(self):
        line = self.fmt(7, 0, payload=bytes.fromhex('aabbccdd') + bytes(6))
        self.assertIn('Zeitbasis raw=aabbccdd', line)

    def test_internal(self):
        self.assertIn('+> FN$05', self.fmt(3, 5, depth=1))
        self.assertIn('<+ FN$05 CARRY', self.fmt(4, 5, depth=1, carry=True))


class TestFiles(Tmp):
    STREAM = (rec(1, 0x03, pid=7, tick=1) + rec(1, 0x84, pid=7, tick=2, depth=1)
              + rec(2, 0x84, pid=7, tick=3, depth=1, carry=True)
              + rec(2, 0x03, pid=7, tick=4))

    def test_bin_by_extension(self):
        p = self.write('t.bin', self.STREAM)
        rc, out = self.run_main(p)
        self.assertEqual(rc, 0)
        self.assertIn('Binaerdump, %d Byte' % len(self.STREAM), out)
        self.assertIn('-> F$Fork', out)
        self.assertIn('<- I$Open CARRY', out)

    def test_bin_forced_with_other_extension(self):
        p = self.write('t.dat', self.STREAM)
        rc, out = self.run_main('--bin', p)
        self.assertIn('-> I$Open', out)

    def test_bin_autodetect_without_extension(self):
        p = self.write('trace', self.STREAM)
        self.assertIn('Binaerdump', self.run_main(p)[1])

    def test_empty_bin(self):
        p = self.write('e.bin', b'')
        rc, out = self.run_main(p)
        self.assertEqual(rc, 0)
        self.assertEqual(out.strip(), 'Binaerdump, 0 Byte')

    def test_bin_truncated_reports_warning(self):
        p = self.write('t.bin', self.STREAM[:-5])
        out = self.run_main(p)[1]
        self.assertIn('abgeschnittener Satz', out)
        self.assertIn('-> F$Fork', out)

    def test_text_dump(self):
        p = self.write('q9dbg_dump.txt', 'vorspann\n' + text_dump(self.STREAM, size=256))
        rc, out = self.run_main(p)
        self.assertEqual(rc, 0)
        self.assertIn('Puffer 00100000, 256 Byte', out)
        self.assertIn('    1.0000  pid  7  -> F$Fork', out)
        self.assertIn('<- I$Open CARRY', out)

    def test_text_dump_wraps_around_ring(self):
        p = self.write('w.txt', text_dump(self.STREAM, size=128, rd=100))
        out = self.run_main(p)[1]
        self.assertEqual(out.count('pid  7'), 4)
        self.assertNotIn('??', out)

    def test_text_dump_without_buffer_header(self):
        p = self.write('x.txt', 'T 0 0102\n')
        rc, out = self.run_main('--text', p)
        self.assertEqual(rc, 1)
        self.assertIn('kein Trace-Puffer', out)

    def test_text_dump_respects_used(self):
        s = self.STREAM
        p = self.write('u.txt', text_dump(s, size=256, used=len(rec(1, 3))))
        self.assertEqual(self.run_main(p)[1].count('pid  7'), 1)

    def test_cli_filters(self):
        p = self.write('t.bin', self.STREAM)
        out = self.run_main('--call', 'i$open', p)[1]
        self.assertIn('I$Open', out)
        self.assertNotIn('F$Fork', out)
        out = self.run_main('--min-depth', '1', p)[1]
        self.assertNotIn('F$Fork', out)
        out = self.run_main('--last', '1', p)[1]
        self.assertEqual(out.count('pid  7'), 1)
        self.assertIn('<- F$Fork', out)
        out = self.run_main('--pid', '8', p)[1]
        self.assertNotIn('pid', out.replace('Binaerdump', ''))

    def test_bin_and_text_flags_exclusive(self):
        p = self.write('t.bin', b'')
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            d.main(['--bin', '--text', p])


if __name__ == '__main__':
    unittest.main()
