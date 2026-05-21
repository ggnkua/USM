#!/bin/sh
# Synthetic regression test for the CA_HEADER word-alignment fix.
#
# Builds two minimal valid PRGs in a tempdir and uses them to construct
# multi-program carts whose first entry footprint deliberately needs to
# be padded to a word boundary. Without the alignment fix, the second
# entry's CA_HEADER would land at an odd cart offset and TOS would crash
# at boot with an Address Error (TST.L on an odd pointer in its cart-init
# scanner).
#
# Two carts are built so the test is robust to changes in
# sizeof(prg_loader): one with an odd-sized first PRG, one with an
# even-sized first PRG. Whichever combination produces an odd unpadded
# footprint exercises the padding path; both must produce all-even
# CA_HEADER offsets post-fix.
#
# Run from anywhere; exits 0 on pass, non-zero on fail.

set -e

cd "$(dirname "$0")/.."

if [ ! -x ./usm ]; then
    echo "==> Building usm"
    ./build_mac_linux.sh
fi

python3 - <<'PY'
import os, struct, subprocess, sys, tempfile

def make_prg(text_bytes):
    """Construct a minimal valid PRG: 28-byte header + given TEXT bytes.

    PRG_magic = 0x601A is the only header field usm.c validates.
    ABSFLAG  = 1 tells usm.c to skip the relocation walker.
    TEXT content is never executed by the host tool; usm.c just memcpys
    the whole file into the cart. We use 0x4E75 (RTS) plus optional
    padding so the bytes would at least disassemble to a legal 68k
    instruction if anyone ever did run them.
    """
    header = struct.pack('>HIIIIIIH',
                         0x601a,           # PRG_magic
                         len(text_bytes),  # PRG_tsize
                         0,                # PRG_dsize
                         0,                # PRG_bsize
                         0,                # PRG_ssize
                         0,                # PRG_res1
                         0,                # PRGFLAGS
                         1)                # ABSFLAG (no fixups)
    assert len(header) == 28
    return header + text_bytes

def build_and_walk(prgs, label):
    """Build a cart from the given list of PRG byte-strings and walk the
    resulting CA_NEXT chain. Returns the list of CA_HEADER cart offsets.
    Fails the script on any odd offset, build failure, or chain malformation."""
    with tempfile.TemporaryDirectory() as td:
        paths = []
        for idx, prg_bytes in enumerate(prgs):
            p = os.path.join(td, f"P{idx}.PRG")
            open(p, 'wb').write(prg_bytes)
            paths.append(p)
        rom = os.path.join(td, "synth.ROM")
        r = subprocess.run(['./usm', rom] + paths, capture_output=True, text=True)
        if r.returncode != 0:
            print(f"FAIL [{label}]: usm exit {r.returncode}", file=sys.stderr)
            print(r.stdout, file=sys.stderr)
            print(r.stderr, file=sys.stderr)
            sys.exit(1)
        data = open(rom, 'rb').read()
        if len(data) != 131072:
            print(f"FAIL [{label}]: cart size = {len(data)}, expected 131072",
                  file=sys.stderr)
            sys.exit(1)
        if data[:4] != b'\xab\xcd\xef\x42':
            print(f"FAIL [{label}]: cart magic = {data[:4].hex()}", file=sys.stderr)
            sys.exit(1)
        offsets = []
        o = 4
        i = 0
        while o and o < len(data):
            if o & 1:
                print(f"FAIL [{label}]: CA_HEADER {i} at odd cart offset {o} "
                      f"(${0xfa0000 + o:06x}) - alignment fix regressed",
                      file=sys.stderr)
                sys.exit(1)
            offsets.append(o)
            ca_next = struct.unpack('>I', data[o:o+4])[0]
            o = (ca_next - 0xfa0000) if ca_next else 0
            i += 1
        return offsets

# Two minimal PRGs: 30 bytes (header + RTS) and 31 bytes (header + RTS + pad).
# Their cart entry footprints will differ by exactly 1 byte; the one whose
# footprint is odd exercises the alignment-padding path. Building both means
# we don't have to know sizeof(prg_loader)'s parity at test-write time.
prg_30 = make_prg(b'\x4e\x75')        # header + RTS = 30 bytes
prg_31 = make_prg(b'\x4e\x75\x00')    # header + RTS + 1 = 31 bytes

# Build two 2-entry carts; the second entry is always the even-sized PRG
# so the test is sensitive to misalignment of *that* entry's CA_HEADER.
offsets_a = build_and_walk([prg_30, prg_30], "30+30")
offsets_b = build_and_walk([prg_31, prg_30], "31+30")

# Sanity: both carts have exactly 2 entries, and the entry-1 cart-offset
# differs by exactly 2 between the two builds. The 1-byte size difference
# in entry 0's PRG plus 1 byte of alignment padding = 2 bytes total. If
# we saw a delta of 1, padding didn't fire when it should have.
if len(offsets_a) != 2 or len(offsets_b) != 2:
    print(f"FAIL: expected 2 entries each, got {offsets_a} and {offsets_b}",
          file=sys.stderr)
    sys.exit(1)

delta = offsets_b[1] - offsets_a[1]
if delta != 2:
    # delta == 1 means the 1-byte file-size difference passed through with
    # no compensating pad, leaving one of the carts with an odd CA_HEADER
    # offset (which the walk above would also have caught - this is a
    # belt-and-braces check).
    print(f"FAIL: entry-1 offset delta = {delta} between 31+30 and 30+30 carts, "
          f"expected 2 (1 byte file_size diff + 1 byte alignment pad). "
          f"Padding logic likely regressed.", file=sys.stderr)
    sys.exit(1)

print(f"PASS: 30+30 cart entries at {offsets_a} (both even)")
print(f"PASS: 31+30 cart entries at {offsets_b} (both even, +2 from 30+30)")
print(f"PASS: padding fired for the odd-footprint case")
PY
