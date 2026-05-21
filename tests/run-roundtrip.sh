#!/bin/sh
# Host-side round-trip test for the LZSS-12-4 compressor.
#
# For each fixture in tests/binaries/ and tests/fixtures/, runs
#   ./usm -T <file>
# which compresses the file in memory, decompresses it back, and
# byte-compares against the original. usm exits non-zero on any
# mismatch; this script aggregates the exits and reports.
#
# `usm` already runs its own short-input self-test (lzss_selftest)
# at the top of main() on every invocation, so this script's first
# successful -T call also implicitly verifies that built-in check.
#
# Run from anywhere.

set -e

cd "$(dirname "$0")/.."

if [ ! -x ./usm ]; then
    echo "==> Building usm"
    ./build_mac_linux.sh
fi

status=0
fixtures=""

# tests/binaries/* is gitignored (third-party PRGs the user provides).
for f in tests/binaries/*.PRG tests/binaries/*.TOS; do
    [ -e "$f" ] && fixtures="$fixtures $f"
done
# tests/fixtures/* is tracked (our own hello-world etc.).
for f in tests/fixtures/*.prg tests/fixtures/*.PRG tests/fixtures/*.tos tests/fixtures/*.TOS; do
    [ -e "$f" ] && fixtures="$fixtures $f"
done

if [ -z "$fixtures" ]; then
    echo "No fixtures found in tests/binaries/ or tests/fixtures/" >&2
    exit 1
fi

for src in $fixtures; do
    if ! ./usm -T "$src"; then
        status=1
    fi
done

if [ "$status" -eq 0 ]; then
    echo
    echo "All round-trips OK"
else
    echo
    echo "FAILED: at least one round-trip diverged" >&2
fi

exit "$status"
