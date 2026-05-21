#!/bin/sh
# Build a compressed multi-program cart from three fixtures into
# build/THREE_Z.ROM. Same input set as tests/build-multi-rom.sh but
# with -z (LZSS compression) enabled globally. Each entry compresses
# independently; the auto-fallback ships any entry that would grow
# under LZSS uncompressed, so a single cart can legitimately mix
# compressed and uncompressed entries.
#
# Run from anywhere.

set -e

cd "$(dirname "$0")/.."

if [ ! -x ./usm ]; then
    echo "==> Building usm"
    ./build_mac_linux.sh
fi

mkdir -p build

out=build/THREE_Z.ROM
binaries="tests/binaries/MONST2.PRG tests/binaries/RAID.PRG tests/binaries/SILYCMP.TOS"

for src in $binaries; do
    if [ ! -f "$src" ]; then
        echo "Missing fixture: $src" >&2
        exit 1
    fi
done

printf '==> %s\n' "$out"
for src in $binaries; do
    printf '      + %s\n' "$src"
done

# -z compresses each PRG and auto-falls-back to uncompressed when the
# compressed entry would be larger than the original. usm prints a
# one-line per-entry note describing what happened.
# shellcheck disable=SC2086
./usm -z "$out" $binaries
