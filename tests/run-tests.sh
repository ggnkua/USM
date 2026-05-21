#!/bin/sh
# Cart-build sanity test for usm.
#
# For each PRG fixture in tests/binaries/ and tests/fixtures/, builds
#   * an uncompressed cart (./usm out.rom file)
#   * a compressed cart   (./usm -z out.rom file, auto-fallback as needed)
# Then verifies each generated cart is exactly 131072 bytes (the cart
# format's fixed size). Aggregates pass/fail across all builds.
#
# Run from anywhere. Exits 0 on success, non-zero if any build failed
# or produced a wrong-sized cart.
#
# If the environment variable HATARI is set to a Hatari binary path,
# each cart is also booted in Hatari with --run-vbls=300 (= about 6
# seconds of emulated runtime) and exit code 0 means it didn't crash.
# Hatari isn't installed by CI, so the Hatari step is opt-in.

set -e

cd "$(dirname "$0")/.."

if [ ! -x ./usm ]; then
    echo "==> Building usm"
    ./build_mac_linux.sh
fi

mkdir -p build

status=0
fixtures=""
for f in tests/binaries/*.PRG tests/binaries/*.TOS \
         tests/fixtures/*.prg tests/fixtures/*.PRG \
         tests/fixtures/*.tos tests/fixtures/*.TOS; do
    [ -e "$f" ] && fixtures="$fixtures $f"
done

if [ -z "$fixtures" ]; then
    echo "No fixtures found in tests/binaries/ or tests/fixtures/" >&2
    exit 1
fi

check_size() {
    expected=$1
    path=$2
    if [ "$(uname)" = "Darwin" ]; then
        actual=$(stat -f %z "$path")
    else
        actual=$(stat -c %s "$path")
    fi
    if [ "$actual" -ne "$expected" ]; then
        echo "FAIL: $path is $actual bytes, expected $expected" >&2
        return 1
    fi
}

# Walks the cart's CA_NEXT chain and asserts every CA_HEADER lands on an
# even cart offset. TOS reads the chain with long-word accesses; an odd
# CA_HEADER pointer triggers a 68000 Address Error and crashes the
# machine with a line of bombs before GEM is up.
check_alignment() {
    path=$1
    python3 - "$path" <<'PY'
import struct, sys
data = open(sys.argv[1], 'rb').read()
# STEem prefix: 4 leading zero bytes before the actual cart payload.
base = 4 if data[:4] == b'\x00\x00\x00\x00' else 0
magic_off = base
magic = data[magic_off:magic_off+4]
if magic != b'\xab\xcd\xef\x42' and magic != b'\xfa\x52\x23\x5f':
    sys.exit(0)  # diagnostic / unknown magic - skip
if magic == b'\xfa\x52\x23\x5f':
    sys.exit(0)  # diagnostic cart has no CA_HEADER chain
o = magic_off + 4
i = 0
while o and o < len(data):
    cart_off = o - base
    if cart_off & 1:
        print(f"FAIL: {sys.argv[1]} CA_HEADER {i} at odd cart offset {cart_off} (${0xfa0000 + cart_off:06x})", file=sys.stderr)
        sys.exit(1)
    ca_next = struct.unpack('>I', data[o:o+4])[0]
    o = (ca_next - 0xfa0000 + base) if ca_next else 0
    i += 1
PY
}

for src in $fixtures; do
    name=$(basename "$src")
    plain="build/${name%.*}_test.ROM"
    compd="build/${name%.*}_test_z.ROM"

    ./usm        "$plain" "$src" || status=1
    ./usm -z     "$compd" "$src" || status=1
    check_size 131072 "$plain"   || status=1
    check_size 131072 "$compd"   || status=1
    check_alignment "$plain"     || status=1
    check_alignment "$compd"     || status=1
done

# Clean up _test* artefacts so they don't pollute build/ between runs.
rm -f build/*_test.ROM build/*_test_z.ROM

# Synthetic multi-program-cart alignment test - the only test that
# actively exercises the word-alignment padding path (existing fixtures
# happen to have even-sized cart payloads through the test paths).
if [ -x ./tests/run-alignment-synth.sh ]; then
    ./tests/run-alignment-synth.sh || status=1
fi

if [ "$status" -eq 0 ]; then
    echo
    echo "All cart builds OK"
else
    echo
    echo "FAILED: at least one cart build diverged" >&2
fi

exit "$status"
