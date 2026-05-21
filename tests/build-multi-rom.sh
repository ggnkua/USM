#!/bin/sh
# Build a multi-program cart from three fixtures into build/THREE.ROM.
# Exercises the CA_NEXT chain that single-program carts skip (the chain
# terminator overwrites the only CA_NEXT in those).
#
# Run from anywhere.

set -e

cd "$(dirname "$0")/.."

if [ ! -x ./usm ]; then
    echo "==> Building usm"
    ./build_mac_linux.sh
fi

mkdir -p build

out=build/THREE.ROM
binaries="tests/binaries/MONST2.PRG tests/binaries/RAID.PRG tests/binaries/SILYCMP.TOS"

# Sanity: make sure every input exists before we burn the cart.
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

# No -f flag: cart appears on the cart's c: drive with all three
# entries listed; user launches each from the desktop.
# shellcheck disable=SC2086
./usm "$out" $binaries
